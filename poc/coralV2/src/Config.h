#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <fstream>
#include <stdexcept>
#include "json.hpp"

// Config class for application settings loaded from config.json
class Config {
public:
    static constexpr int DefaultSilenceTimeoutSeconds = 300;
    static constexpr int DefaultAudioSampleRate = 16000;
    static constexpr int DefaultAudioChannels = 1;
    static constexpr const char* DefaultTriggerKey = "Fn";
    static constexpr const char* DefaultWhisperModelPath = "../../whispercpp/models/ggml-base.en.bin";

    // Loads config from the given file path
    explicit Config(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file) {
            throw std::runtime_error("Could not open config file: " + filePath);
        }
        nlohmann::json j;
        file >> j;
        silenceTimeoutSeconds = j.value("silenceTimeoutSeconds", DefaultSilenceTimeoutSeconds);
        audioSampleRate = j.value("audioSampleRate", DefaultAudioSampleRate);
        audioChannels = j.value("audioChannels", DefaultAudioChannels);
        triggerKey = j.value("triggerKey", DefaultTriggerKey);
        whisperModelPath = j.value("whisperModelPath", DefaultWhisperModelPath);
        debugLevel = j.value("debugLevel", 0);
        logFilePath = j.value("logFilePath", std::string("voiceOut.log"));
        logToConsole = j.value("logToConsole", true);
        cmdTriggerKey = j.value("cmdTriggerKey", std::string());
    }

    int silenceTimeoutSeconds;
    int audioSampleRate;
    int audioChannels;
    std::string triggerKey;
    std::string whisperModelPath;
    int debugLevel;
    std::string logFilePath;
    bool logToConsole;
    std::string cmdTriggerKey;
};

#endif // CONFIG_H 