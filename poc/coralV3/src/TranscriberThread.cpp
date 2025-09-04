#include "TranscriberThread.h"
#include "Transcriber.h"
#include "Logger.h"
#include "TextUtils.h"
#include <iostream>
#include <cstdio>

TranscriberThread::TranscriberThread(const Config& config,
                                     ConcurrentQueue<std::shared_ptr<AudioEvent>>& audioQueue,
                                     ConcurrentQueue<std::shared_ptr<TextEvent>>& textQueue)
    : config_(config), audioQueue_(audioQueue), textQueue_(textQueue), running_(false) {}

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

void TranscriberThread::run() 
{
    try {
        std::unordered_set<char> specialChars{'.', '?', '!'};
        while (running_) 
        {
            auto audioEvent = audioQueue_.waitAndPop();
            DEBUG(3, "Transcriber picked up audio event: " + audioEvent->getFileName() + ", trigger key: " + audioEvent->getTriggerKey());
            const std::string& audioFile = audioEvent->getFileName();
            std::string text = Transcriber::getInstance()->transcribeAudio(audioFile, config_.getWhisperModelPath(), config_.getWhisperLanguage());
            if (audioEvent->getTriggerKey() == config_.getCmdTriggerKey()) 
            {
                text = TextUtils::trim(text);
                text = TextUtils::removeSpecialChars(text, specialChars);
                TextUtils::toLower(text);
                //for the command inputs we need to add \n to simulate the 'enter' key
                text += "\n";
                DEBUG(3, "Pushing transcribed text to textEventQueue (cmdTriggerKey)");
            } else 
            {
                DEBUG(3, "Pushing transcribed text to textEventQueue (triggerKey)");
            }
            textQueue_.push(std::make_shared<TextEvent>(text));
            // Delete audio file after transcription
            if (std::remove(audioFile.c_str()) == 0) 
            {
                DEBUG(3, "Deleted audio file: " + audioFile);
            } else 
            {
                WARN("Failed to delete audio file: " + audioFile);
            }
        }
    } catch (const std::exception& ex) {
        ERROR(std::string("Exception in TranscriberThread: ") + ex.what());
    }
} 
