#ifndef TEXT_EVENT_H
#define TEXT_EVENT_H

#include <string>

class TextEvent
{
public:
    explicit TextEvent(std::string t) : text_(std::move(t)) {}
    const std::string& getText() const
    {
        return text_;
    }
    void setText(const std::string& t)
    {
        text_ = t;
    }
private:
    std::string text_;
};

#endif // TEXT_EVENT_H