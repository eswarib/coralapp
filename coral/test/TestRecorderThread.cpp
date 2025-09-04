#include "../src/RecorderThread.h"
#include "../src/KeyDetector.h"
#include "../src/Config.h"
#include "../src/concurrentQueue.h"
#include "../src/AudioEvent.h"
#include <cassert>
#include <iostream>
#include <atomic>
#include <memory>

// A mock KeyDetector to control key presses for testing.
class MockKeyDetector : public KeyDetector {
public:
    std::atomic<bool> isPressed{false};
    bool isTriggerKeyPressed(const std::string& /*keyName*/) override {
        return isPressed.load();
    }
};

void testRecorderThread() {
    Config config("../src/config.json");
    ConcurrentQueue<std::shared_ptr<AudioEvent>> audioQueue;
    MockKeyDetector mockDetector;
    
    // Note: This test will call the real recorder singleton, which is not ideal for a pure unit test.
    // For better isolation, the recorder should be refactored to use an interface.
    RecorderThread recorder(config, audioQueue, mockDetector);
    
    recorder.start();
    
    // Simulate a key press and release cycle.
    mockDetector.isPressed = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    mockDetector.isPressed = false;
    
    // Allow time for the thread to process the event and push to the queue.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    recorder.stop();

    assert(audioQueue.size() == 1);
    auto event = audioQueue.tryPop();
    assert(event.has_value());
    assert((*event)->getFileName() == "input.wav");
}

int main() {
    testRecorderThread();
    std::cout << "RecorderThread tests passed.\n";
    return 0;
} 