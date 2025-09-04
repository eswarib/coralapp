#include "TextInjector.h"
#include <cstdlib>

TextInjector* TextInjector::sInstance = nullptr;

TextInjector* TextInjector::getInstance()
{
    if (sInstance) {
        return sInstance;
    }
    sInstance = new TextInjector();
    return sInstance;
}

TextInjector::~TextInjector() {}

void TextInjector::typeText(const std::string& text) {
    std::string escapedText;
    for (char c : text) {
        if (c == '"' || c == '\\') escapedText += '\\';
        escapedText += c;
    }
   

    std::string cmd = "xdotool type --delay 20 \"" + escapedText + "\"";
    std::system(cmd.c_str());
} 