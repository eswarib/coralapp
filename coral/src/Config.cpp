#include "Config.h"
#include <filesystem>
#include <cstdlib>  // for getenv
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>
#include "Logger.h"
#include "Utils.h"

namespace fs = std::filesystem;

const std::string Config::WhisperModelNameSmallEnQ8 = "ggml-small.en-q8_0.bin";
const std::string Config::WhisperModelNameSmallEn = "ggml-small.en.bin";
const std::string Config::WhisperModelNameBaseEn = "ggml-base.en.bin";
    
 // Loads config from the given file path
Config::Config(const std::string& filePath) 
{
    std::ifstream file(filePath);
    if (!file) 
    {
        throw std::runtime_error("Could not open config file: " + filePath);
    }
        
    nlohmann::json j;
    file >> j;
    silenceTimeoutSeconds = j.value("silenceTimeoutSeconds", DefaultSilenceTimeoutSeconds);
    INFO("silenceTimeoutSeconds:  [" + std::to_string(silenceTimeoutSeconds) + "]");
    audioSampleRate = j.value("audioSampleRate", DefaultAudioSampleRate);
    INFO("audioSampleRate:  [" + std::to_string(audioSampleRate) + "]");
    audioChannels = j.value("audioChannels", DefaultAudioChannels);
    INFO("audioChannels:  [" + std::to_string(audioChannels) + "]");
    triggerKey = j.value("triggerKey", DefaultTriggerKey);
    INFO("triggerKey:  [" + triggerKey + "]");
    whisperModelPath = j.value("whisperModelPath", DefaultWhisperModelPath);
    INFO("whisperModelPath:  [" + whisperModelPath + "]");
    debugLevel = j.value("debugLevel", 0);
    INFO("debugLevel:  [" + std::to_string(debugLevel) + "]");
    
    // Default log path is now in the user's .coral directory
    const char* home = std::getenv("HOME");
    if (!home) {
        throw std::runtime_error("HOME environment variable not set");
    }
    std::string defaultLogPath = std::string(home) + "/.coral/logs/coral.log";
    logFilePath = j.value("logFilePath", defaultLogPath);
    
    // Always expand tilde for the log file path
    logFilePath = Utils::expandTilde(logFilePath);
    
    // Ensure the log directory exists
    try 
    {
        std::filesystem::path logDir = std::filesystem::path(logFilePath).parent_path();
        if (!std::filesystem::exists(logDir)) 
        {
            std::filesystem::create_directories(logDir);
        }
    } 
    catch (const std::exception& e) 
    {
        // Log directory creation failed, but don't crash the app
        std::cerr << "Warning: Could not create log directory: " << e.what() << std::endl;
    }

    INFO("logFilePath:  [" + logFilePath + "]");
    logToConsole = j.value("logToConsole", true);
    INFO("logToConsole:  [" + std::string(logToConsole ? "true" : "false") + "]");
    cmdTriggerKey = j.value("cmdTriggerKey", std::string());
    INFO("cmdTriggerKey:  [" + cmdTriggerKey + "]");
    whisperLanguage = j.value("whisperLanguage", std::string("en"));
    INFO("whisperLanguage:  [" + whisperLanguage + "]");
}



std::string Config::getWhisperModelPath() const {
    namespace fs = std::filesystem;
    
    // 1) Config path (with tilde expansion)
    std::string expandedModelPath = Utils::expandTilde(whisperModelPath);
    if (!expandedModelPath.empty() && fs::exists(expandedModelPath)) 
    {
        return expandedModelPath;
    }

    // 2) Build a set of candidate directories and filenames
    std::vector<std::string> candidates;
    auto addCandidatesInDir = [&](const fs::path& dir){
        candidates.push_back((dir / WhisperModelNameBaseEn).string());
        candidates.push_back((dir / WhisperModelNameSmallEn).string());
        candidates.push_back((dir / WhisperModelNameSmallEnQ8).string());
    };

    const char* appdir = std::getenv("APPDIR");
    if (appdir && *appdir)
    {
        addCandidatesInDir(fs::path(appdir) / "usr/share/coral/models");
    }

    // System path
    addCandidatesInDir(fs::path("/usr/share/coral/models"));

    // Executable-relative paths
    char exePath[4096];
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len != -1) {
        exePath[len] = '\0';
        fs::path exeDir = fs::path(exePath).parent_path();
        addCandidatesInDir(exeDir / "../share/coral/models");
        addCandidatesInDir(exeDir / "models");
    }

    // User models directory
    const char* home = std::getenv("HOME");
    if (home && *home)
    {
        addCandidatesInDir(fs::path(home) / ".coral/models");
    }

    for (const auto& c : candidates)
    {
        if (fs::exists(c))
        {
            INFO("Resolved whisper model path: [" + c + "]");
            return c;
        }
    }

    // Final fallback: prefer APPDIR if available, else system default
    std::string fallback = (appdir && *appdir)
        ? (fs::path(appdir) / "usr/share/coral/models" / WhisperModelNameBaseEn).string()
        : (fs::path("/usr/share/coral/models") / WhisperModelNameBaseEn).string();

    if (!fs::exists(fallback))
    {
        ERROR("Whisper model not found. Expected at: " + fallback);
    }
    else
    {
        INFO("Using fallback whisper model: [" + fallback + "]");
    }
    return fallback;
}


void Config::copyConfigFileOnFirstRun() 
{
    const char* home = std::getenv("HOME");
    if (!home) {
        throw std::runtime_error("HOME environment variable not set");
    }
    std::string userConfigDir = std::string(home) + "/.coral";
    std::string userConfigPath = userConfigDir + "/config.json";

    if (!fs::exists(userConfigPath)) {
        fs::create_directories(userConfigDir);

        // Prefer $APPDIR if set, else fallback to system path
        const char* appdir = std::getenv("APPDIR");
        std::string defaultConfigPath = appdir
            ? std::string(appdir) + "/usr/share/coral/config/config.json"
            : "/usr/share/coral/config/config.json";

        fs::copy_file(defaultConfigPath, userConfigPath, fs::copy_options::overwrite_existing);

        // Set ownership to the current user
        uid_t uid = getuid();
        gid_t gid = getgid();
        chown(userConfigPath.c_str(), uid, gid);
    }
}


