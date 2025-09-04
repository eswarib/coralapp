#ifndef TEXT_INJECTOR_H
#define TEXT_INJECTOR_H
#include <string>

class TextInjector
{
public:
    static TextInjector* getInstance();
    void typeText(const std::string& text);
    virtual ~TextInjector();
    // Timing parameters (tunable)
    static constexpr int kServeIdleExitMs = 100;           // increased from 50ms for better stability
    static constexpr int kInitialSleepBeforePasteMs = 150;  // increased from 60ms to ensure clipboard is ready
    static constexpr int kKeySequenceDelayUs = 12000;      // increased from 8000us for more reliable key injection
    static constexpr int kClipboardServeTimeoutMs = 3000;  // increased from 2000ms for longer clipboard serving
    static constexpr int kClipboardPollIntervalMs = 5;     // increased from 2ms for better event processing
private:
    static TextInjector* sInstance;
    TextInjector() = default;
    TextInjector(const TextInjector&) = delete;
    TextInjector& operator=(const TextInjector&) = delete;
};

#endif // TEXT_INJECTOR_H