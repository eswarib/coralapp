#include "TranscriberThread.h"
#include "Transcriber.h"
#include "Logger.h"
#include <iostream>
#include <cstdio>
#include <algorithm>
#include <cctype>

TranscriberThread::TranscriberThread(const Config& config,
                                     ConcurrentQueue<std::shared_ptr<AudioEvent>>& audioQueue,
                                     ConcurrentQueue<std::shared_ptr<TextEvent>>& textQueue,
                                     ConcurrentQueue<std::shared_ptr<TextEvent>>& cmdTextEventQueue)
    : config_(config), audioQueue_(audioQueue), textQueue_(textQueue), cmdTextEventQueue_(cmdTextEventQueue), running_(false) {}

TranscriberThread::~TranscriberThread() {
    stop();
}

void TranscriberThread::start() {
    running_ = true;
    thread_ = std::thread(&TranscriberThread::run, this);
}

void TranscriberThread::stop() {
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

void TranscriberThread::run() {
    try {
        while (running_) {
            auto audioEvent = audioQueue_.waitAndPop();
            DEBUG(3, "Transcriber picked up audio event: " + audioEvent->getFileName());
            const std::string& audioFile = audioEvent->getFileName();
            std::string text = Transcriber::getInstance()->transcribeAudio(audioFile);
            // Check which key triggered the event
            if (audioEvent->getTriggerKey() == config_.cmdTriggerKey) {
                // Remove '.', '?', '!' and lowercase the first letter
                text.erase(std::remove_if(text.begin(), text.end(), [](char c) { return c == '.' || c == '?' || c == '!'; }), text.end());
                if (!text.empty()) text[0] = std::tolower(text[0]);
                cmdTextEventQueue_.push(std::make_shared<TextEvent>(text));
            } else {
                textQueue_.push(std::make_shared<TextEvent>(text));
            }
            // Delete audio file after transcription
            if (std::remove(audioFile.c_str()) == 0) {
                DEBUG(3, "Deleted audio file: " + audioFile);
            } else {
                WARN("Failed to delete audio file: " + audioFile);
            }
        }
    } catch (const std::exception& ex) {
        ERROR(std::string("Exception in TranscriberThread: ") + ex.what());
    }
} 