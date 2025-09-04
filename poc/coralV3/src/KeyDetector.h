#ifndef KEY_DETECTOR_H
#define KEY_DETECTOR_H

#include <string>

// KeyDetector: handles detection of trigger key press
class KeyDetector {
public:
    KeyDetector() = default;
    virtual ~KeyDetector() = default;
    // Returns true if the trigger key is currently pressed
    virtual bool isTriggerKeyPressed(const std::string& keyName);
};

#endif // KEY_DETECTOR_H 