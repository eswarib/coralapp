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
    static constexpr const char* DefaultWhisperModelPath = "/usr/share/coral/models/ggml-base.en.bin";

   
    explicit Config(const std::string& filePath);   

    int getSilenceTimeoutSeconds() const {return silenceTimeoutSeconds;} 
    int getAudioSampleRate() const { return audioSampleRate;}
    int getAudioChannels() const { return audioChannels; }
    std::string getTriggerKey() const { return triggerKey; }
    std::string getWhisperModelPath() const { return whisperModelPath; }
    int getDebugLevel() const { return debugLevel; }
    std::string getLogFilePath() const { return logFilePath; }
    bool getLogToConsole() const { return logToConsole; }
    std::string getCmdTriggerKey() const { return cmdTriggerKey; }
    std::string getWhisperLanguage() const { return whisperLanguage; }   


    private: 
    int silenceTimeoutSeconds;
    int audioSampleRate;
    int audioChannels;
    std::string triggerKey;
    std::string whisperModelPath;
    int debugLevel;
    std::string logFilePath;
    bool logToConsole;
    std::string cmdTriggerKey;
    std::string whisperLanguage;
};

#endif // CONFIG_H 