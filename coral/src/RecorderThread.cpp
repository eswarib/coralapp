#include "RecorderThread.h"
#include "Recorder.h"
#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <memory>
#include <filesystem>

namespace {
std::string generateUniqueFilename() {
    // Get cross-platform temp directory
    std::filesystem::path baseTmp;
    try {
        baseTmp = std::filesystem::temp_directory_path();
    } catch (...) {
        baseTmp = std::filesystem::path("/tmp");
    }
    
    // Create coral subfolder in temp
    std::filesystem::path coralTmpDir = baseTmp / "coral";
    try { 
        std::filesystem::create_directories(coralTmpDir); 
    } catch (...) {}
    
    // Generate unique filename with timestamp
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << "input_" << std::put_time(std::localtime(&now_time_t), "%Y%m%d_%H%M%S") << "_" << now_ms.count() << ".wav";
    
    // Return full path
    return (coralTmpDir / oss.str()).string();
}
}

RecorderThread::RecorderThread(const Config& config,
                               ConcurrentQueue<std::shared_ptr<AudioEvent>>& audioQueue,
                               KeyDetector& keyDetector)
    : _config(config), _audioQueue(audioQueue), _keyDetector(keyDetector), _running(false) {}

RecorderThread::~RecorderThread() {
    stop();
}

void RecorderThread::start() {
    _running = true;
    _thread = std::thread(&RecorderThread::run, this);
}

void RecorderThread::stop() {
    _running = false;
    if (_thread.joinable()) {
        _thread.join();
    }
}

void RecorderThread::run() {
    INFO("RecorderThread started, waiting for trigger keys...");
    while (_running) {
        std::string pressedKey;
        if (_keyDetector.isTriggerKeyPressed(_config.getTriggerKey())) {
            pressedKey = _config.getTriggerKey();
        } 
        else if (_keyDetector.isTriggerKeyPressed(_config.getCmdTriggerKey())) 
        {
            DEBUG(3, "cmdTriggerKey detected");
            pressedKey = _config.getCmdTriggerKey();
        }
        if (!pressedKey.empty()) 
        {
            std::cout << "TRIGGER_DOWN" << std::endl;
            std::string audioFile = generateUniqueFilename();
            DEBUG(3, "Starting recording to: " + audioFile);
            Recorder::getInstance()->startRecording();
            // Wait for key release
            while ((_keyDetector.isTriggerKeyPressed(pressedKey)) && _running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            std::cout << "TRIGGER_UP" << std::endl;
            Recorder::getInstance()->stopRecording(audioFile);
            DEBUG(3, "Finished recording: " + audioFile);
            DEBUG(3, "Inserting audio file into queue: " + audioFile + ", triggered by: " + pressedKey);
            DEBUG(3, "AudioEvent created with trigger key: " + pressedKey);
            _audioQueue.push(std::make_shared<AudioEvent>(audioFile, pressedKey));
            DEBUG(3, "Queue size after push: " + std::to_string(_audioQueue.size()));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

