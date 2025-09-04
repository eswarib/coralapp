#include "InjectorThread.h"
#include "TextInjector.h"
#include "Logger.h"
#include <iostream>

InjectorThread::InjectorThread(const Config& config,
                               ConcurrentQueue<std::shared_ptr<TextEvent>>& textQueue
                               )
    : config_(config), textQueue_(textQueue), running_(false) {}

InjectorThread::~InjectorThread() {
    stop();
}

void InjectorThread::start() {
    running_ = true;
    thread_ = std::thread(&InjectorThread::run, this);
}

void InjectorThread::stop() {
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

void InjectorThread::run() {
    while (running_) {
        std::shared_ptr<TextEvent> textEvent;
        textEvent = textQueue_.waitAndPop();
        DEBUG(3, "Typing text: '" + textEvent->getText() + "'");
        TextInjector::getInstance()->typeText(textEvent->getText());
        DEBUG(3, "Finished typing text.");
    }
} 
