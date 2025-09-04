#include "../src/Config.h"
#include <cassert>
#include <iostream>

void testConfigLoad() {
    Config config("../src/config.json");
    assert(config.silenceTimeoutSeconds == 300);
    assert(config.audioSampleRate == 16000);
    assert(config.audioChannels == 1);
    assert(config.triggerKey == "Fn");
    assert(config.whisperModelPath == "../../whispercpp/models/ggml-base.en.bin");
    assert(config.cmdTriggerKey == "Ctrl+F2");
}

void testConfigDefaults() {
    // Simulate missing fields by creating a minimal JSON file
    std::ofstream out("test_config_minimal.json");
    out << "{}";
    out.close();
    Config config("test_config_minimal.json");
    assert(config.silenceTimeoutSeconds == Config::DefaultSilenceTimeoutSeconds);
    assert(config.audioSampleRate == Config::DefaultAudioSampleRate);
    assert(config.audioChannels == Config::DefaultAudioChannels);
    assert(config.triggerKey == Config::DefaultTriggerKey);
    assert(config.whisperModelPath == Config::DefaultWhisperModelPath);
    assert(config.cmdTriggerKey == "");
    std::remove("test_config_minimal.json");
}

int main() {
    testConfigLoad();
    testConfigDefaults();
    std::cout << "Config tests passed.\n";
    return 0;
} 