#include "../src/InjectorThread.h"
#include "../src/TranscriberThread.h" // For TextEvent
#include "../src/Config.h"
#include "../src/concurrentQueue.h"
#include <cassert>
#include <iostream>
#include <memory>

// This test verifies that the InjectorThread consumes events from the queue.
// Note: We cannot easily check the side-effect (text injection via xdotool)
// in a simple unit test.
void testInjectorThread() {
    Config config("../src/config.json");
    ConcurrentQueue<std::shared_ptr<TextEvent>> textQueue;

    // Note: this test calls the real textInjector singleton.
    InjectorThread injector(config, textQueue);
    
    injector.start();

    textQueue.push(std::make_shared<TextEvent>("hello world"));

    // Allow time for the thread to process the event.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    injector.stop();

    // The event should have been consumed from the queue.
    assert(textQueue.empty());
}

int main() {
    testInjectorThread();
    std::cout << "InjectorThread tests passed.\n";
    return 0;
} 