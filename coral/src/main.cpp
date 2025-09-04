#include "Config.h"
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
            const char* home = std::getenv("HOME");
            if (home) {
                configPath = std::string(home) + "/.coral/config.json";
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

        RecorderThread recorderThread(config, audioEventQueue, keyDetector);
        TranscriberThread transcriberThread(config, audioEventQueue, textEventQueue);
        InjectorThread injectorThread(config, textEventQueue);

        recorderThread.start();
        transcriberThread.start();
        injectorThread.start();

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

