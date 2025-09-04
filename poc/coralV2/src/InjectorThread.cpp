#include "InjectorThread.h"
#include "TextInjector.h"
#include "Logger.h"
#include <iostream>

InjectorThread::InjectorThread(const Config& config,
                               ConcurrentQueue<std::shared_ptr<TextEvent>>& textQueue,
                               ConcurrentQueue<std::shared_ptr<TextEvent>>& cmdTextEventQueue)
    : config_(config), textQueue_(textQueue), cmdTextEventQueue_(cmdTextEventQueue), running_(false) {}

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
        if (!cmdTextEventQueue_.empty()) {
            textEvent = cmdTextEventQueue_.waitAndPop();
        } else {
            textEvent = textQueue_.waitAndPop();
        }
        DEBUG(3, "Typing text: '" + textEvent->getText() + "'");
        TextInjector::getInstance()->typeText(textEvent->getText());
        DEBUG(3, "Finished typing text.");
    }
} 
