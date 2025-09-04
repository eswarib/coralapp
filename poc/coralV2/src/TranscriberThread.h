#ifndef TRANSCRIBER_THREAD_H
#define TRANSCRIBER_THREAD_H

#include <thread>
#include <atomic>
#include <memory>
#include "Config.h"
#include "concurrentQueue.h"
#include "AudioEvent.h"
#include "TextEvent.h"

class TranscriberThread {
public:
    TranscriberThread(const Config& config,
                      ConcurrentQueue<std::shared_ptr<AudioEvent>>& audioQueue,
                      ConcurrentQueue<std::shared_ptr<TextEvent>>& textQueue,
                      ConcurrentQueue<std::shared_ptr<TextEvent>>& cmdTextEventQueue);
    ~TranscriberThread();
    void start();
    void stop();
private:
    void run();
    std::thread thread_;
    std::atomic<bool> running_;
    const Config& config_;
    ConcurrentQueue<std::shared_ptr<AudioEvent>>& audioQueue_;
    ConcurrentQueue<std::shared_ptr<TextEvent>>& textQueue_;
    ConcurrentQueue<std::shared_ptr<TextEvent>>& cmdTextEventQueue_;
};

#endif // TRANSCRIBER_THREAD_H 