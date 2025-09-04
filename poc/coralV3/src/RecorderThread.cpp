#include "RecorderThread.h"
#include "Recorder.h"
#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <memory>

namespace {
std::string generateUniqueFilename() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << "input_" << std::put_time(std::localtime(&now_time_t), "%Y%m%d_%H%M%S") << "_" << now_ms.count() << ".wav";
    return oss.str();
}
}

RecorderThread::RecorderThread(const Config& config,
                               ConcurrentQueue<std::shared_ptr<AudioEvent>>& audioQueue,
                               KeyDetector& keyDetector)
    : config_(config), audioQueue_(audioQueue), keyDetector_(keyDetector), running_(false) {}

RecorderThread::~RecorderThread() {
    stop();
}

void RecorderThread::start() {
    running_ = true;
    thread_ = std::thread(&RecorderThread::run, this);
}

void RecorderThread::stop() {
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

void RecorderThread::run() {
    while (running_) {
        std::string pressedKey;
        if (keyDetector_.isTriggerKeyPressed(config_.getTriggerKey())) {
            pressedKey = config_.getTriggerKey();
        } 
        else if (keyDetector_.isTriggerKeyPressed(config_.getCmdTriggerKey())) 
        {
            DEBUG(3, "cmdTriggerKey detected");
            pressedKey = config_.getCmdTriggerKey();
        }
        if (!pressedKey.empty()) 
        {
            std::string audioFile = generateUniqueFilename();
            DEBUG(3, "Starting recording to: " + audioFile);
            Recorder::getInstance()->startRecording();
            // Wait for key release
            while ((keyDetector_.isTriggerKeyPressed(pressedKey)) && running_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            Recorder::getInstance()->stopRecording(audioFile);
            DEBUG(3, "Finished recording: " + audioFile);
            DEBUG(3, "Inserting audio file into queue: " + audioFile + ", triggered by: " + pressedKey);
            DEBUG(3, "AudioEvent created with trigger key: " + pressedKey);
            audioQueue_.push(std::make_shared<AudioEvent>(audioFile, pressedKey));
            DEBUG(3, "Queue size after push: " + std::to_string(audioQueue_.size()));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

