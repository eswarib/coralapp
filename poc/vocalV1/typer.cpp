#include "typer.h"
#include <cstdlib>

void typeText(const std::string &text) {
    std::string escapedText;
    for (char c : text) {
        if (c == '"' || c == '\\') escapedText += '\\';
        escapedText += c;
    }

    std::string cmd = "xdotool type --delay 20 \"" + escapedText + "\"";
    std::system(cmd.c_str());
}

