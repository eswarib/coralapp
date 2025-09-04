#ifndef TEXT_INJECTOR_H
#define TEXT_INJECTOR_H
#include <string>

class TextInjector {
public:
    static TextInjector* getInstance();
    void typeText(const std::string& text);
    virtual ~TextInjector();
private:
    static TextInjector* sInstance;
    TextInjector() = default;
    TextInjector(const TextInjector&) = delete;
    TextInjector& operator=(const TextInjector&) = delete;
};

#endif // TEXT_INJECTOR_H 