#include "Config.h"
#include "Utils.h"
#include "concurrentQueue.h"
#include "AudioEvent.h"
#include "TranscriberThread.h"
#include "InjectorThread.h"
#include "RecorderThread.h"
#include "KeyDetector.h"
#include "TextInjector.h"
#include <memory>
#include <iostream>
#include "Logger.h"
#include <filesystem>
#include <cstdlib>
#include "version.h"
#include <iostream>
#if !defined(_WIN32)
#include <fcntl.h>
#include <unistd.h>
#endif

int main(int argc, char* argv[]) 
{
    std::cout << "Backend version: " << APP_VERSION
              << ", Build date: " << BUILD_DATE
              << std::endl;

    try {
        std::string configPath;
        if (argc > 1) {
            configPath = argv[1];
        } 
        else 
        {
            std::string home = Utils::getHomeDir();
            if (!home.empty()) {
                configPath = home + "/.coral/conf/config.json";
            }
            else
            {
                std::cerr << "Could not determine home directory for default config path." << std::endl;
                return 1;
            } 
        }
        if (!std::filesystem::exists(configPath)) {
            // This INFO call will be buffered by the logger until init() is called.
            INFO("Config file does not exist at $HOME/.coral, this may be the first run of the application, copying the default config file");
            Config::copyConfigFileOnFirstRun();
        }
        
        // First, create the config object
        Config config(configPath);

        // Initialize the logger using the configuration. This will open the log file
        // and flush any buffered messages.
        Logger::getInstance().init(config.getLogFilePath(), config.getDebugLevel());

        // Now it's safe to log, and all previous logs will be in the correct file.
        INFO("Reading configuration from: " + configPath);
        INFO("coral is up and running");
        ConcurrentQueue<std::shared_ptr<AudioEvent>> audioEventQueue;
        ConcurrentQueue<std::shared_ptr<TextEvent>> textEventQueue;
        KeyDetector keyDetector;

        // Signal the Electron frontend if permissions are missing.
        // Two independent checks:
        //   NEED_INPUT_GROUP  → user not in 'input' group (evdev key detection fails)
        //   NEED_UINPUT_RULE  → /dev/uinput not accessible (text injection fails)
#if !defined(_WIN32)
        if (!keyDetector.hasEvdevAccess())
        {
            std::cout << "NEED_INPUT_GROUP" << std::endl;
            WARN("evdev not available — key detection may not work on Wayland. "
                 "Add user to 'input' group: sudo usermod -aG input $USER");
        }

        // Check uinput access separately — user may be in 'input' group
        // but /dev/uinput still has no udev rule.
        {
            int uinputFd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
            if (uinputFd < 0)
            {
                std::cout << "NEED_UINPUT_RULE" << std::endl;
                WARN("/dev/uinput not accessible — text injection via virtual keyboard unavailable. "
                     "A udev rule is needed: KERNEL==\"uinput\", GROUP=\"input\", MODE=\"0660\"");
            }
            else
            {
                close(uinputFd);
            }
        }
#endif

        RecorderThread recorderThread(config, audioEventQueue, keyDetector);
        TranscriberThread transcriberThread(config, audioEventQueue, textEventQueue);
        InjectorThread injectorThread(config, textEventQueue);

        recorderThread.start();
        transcriberThread.start();
        injectorThread.start();

        // Signal frontend that backend is up and all threads are running
        std::cout << "BACKEND_READY" << std::endl;
        INFO("All threads started — backend is ready");

        // Main thread waits indefinitely until externally interrupted
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        recorderThread.stop();
        transcriberThread.stop();
        injectorThread.stop();
        INFO("coral shutting down.");
    } catch (const std::exception& ex) {
        ERROR(std::string("model file is not found. Shutting down the application: ") + ex.what());
        return 1;
    }
    return 0;
}

