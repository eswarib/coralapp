#include "TranscriberThread.h"
#include "Transcriber.h"
#include "Logger.h"
#include "TextUtils.h"
#include <iostream>
#include <cstdio>

TranscriberThread::TranscriberThread(const Config& config,
                                     ConcurrentQueue<std::shared_ptr<AudioEvent>>& audioQueue,
                                     ConcurrentQueue<std::shared_ptr<TextEvent>>& textQueue)
    : _config(config), _audioQueue(audioQueue), _textQueue(textQueue), _running(false) {}

TranscriberThread::~TranscriberThread() {
    stop();
}

void TranscriberThread::start() {
    _running = true;
    _thread = std::thread(&TranscriberThread::run, this);
}

void TranscriberThread::stop() {
    _running = false;
    if (_thread.joinable()) {
        _thread.join();
    }
}

void TranscriberThread::run() 
{
    INFO("TranscriberThread started, waiting for audio events...");
    try {
        while (_running) 
        {
            INFO("TranscriberThread is going to pop from recorder");
            auto audioEvent = _audioQueue.waitAndPop();
            DEBUG(3, "Transcriber picked up audio event: " + audioEvent->getFileName() + ", trigger key: " + audioEvent->getTriggerKey());
            const std::string& audioFile = audioEvent->getFileName();
            DEBUG(3, "Transcribing audio file: [" + audioFile + "] + whisper model: [" + _config.getWhisperModelPath() + "] + language: [" + _config.getWhisperLanguage() + "]");
            std::string text = Transcriber::getInstance()->transcribeAudio(audioFile, _config.getWhisperModelPath(), _config.getWhisperLanguage());
            if (audioEvent->getTriggerKey() == _config.getCmdTriggerKey()) 
            {
                text = TextUtils::trim(text);
                std::vector<std::string> specialSubstrings{"...", "?", "!"};
                text = TextUtils::removeSpecialSubstrings(text, specialSubstrings);
                TextUtils::toLower(text);
                //for the command inputs we need to add \n to simulate the 'enter' key
                text += "\n";
                DEBUG(3, "Pushing transcribed text to textEventQueue (cmdTriggerKey)");
            } else 
            {
                DEBUG(3, "Pushing transcribed text to textEventQueue (triggerKey)");
            }

            
            _textQueue.push(std::make_shared<TextEvent>(text));
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
