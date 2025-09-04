#include "Config.h"

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
    audioSampleRate = j.value("audioSampleRate", DefaultAudioSampleRate);
    audioChannels = j.value("audioChannels", DefaultAudioChannels);
    triggerKey = j.value("triggerKey", DefaultTriggerKey);
    whisperModelPath = j.value("whisperModelPath", DefaultWhisperModelPath);
    debugLevel = j.value("debugLevel", 0);
    logFilePath = j.value("logFilePath", std::string("voiceOut.log"));
    logToConsole = j.value("logToConsole", true);
    cmdTriggerKey = j.value("cmdTriggerKey", std::string());
    whisperLanguage = j.value("whisperLanguage", std::string("en"));
}
