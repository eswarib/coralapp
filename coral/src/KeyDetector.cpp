#include "KeyDetector.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <algorithm>

// Helper: map string to X11 keysym
KeySym keyNameToKeySym(const std::string& keyName) {
    static const std::unordered_map<std::string, KeySym> keyMap = {
        {"F1", XK_F1}, {"F2", XK_F2}, {"F3", XK_F3}, {"F4", XK_F4},
        {"F5", XK_F5}, {"F6", XK_F6}, {"F7", XK_F7}, {"F8", XK_F8},
        {"F9", XK_F9}, {"F10", XK_F10}, {"F11", XK_F11}, {"F12", XK_F12},
        {"Ctrl", XK_Control_L}, {"Alt", XK_Alt_L}, {"Shift", XK_Shift_L},
        {"Super", XK_Super_L}, {"space", XK_space}, {"Num_Lock", XK_Num_Lock},
        // Add more as needed
    };
    auto it = keyMap.find(keyName);
    if (it != keyMap.end()) return it->second;
    if (keyName.length() == 1) return XStringToKeysym(keyName.c_str()); // single char
    return NoSymbol;
}

// Helper: split string by delimiter
std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        // Remove whitespace
        token.erase(std::remove_if(token.begin(), token.end(), ::isspace), token.end());
        tokens.push_back(token);
    }
    return tokens;
}

bool KeyDetector::isTriggerKeyPressed(const std::string& keyCombo) {
    if (keyCombo.find("Fn") != std::string::npos) {
        std::cerr << "[KeyDetector] 'Fn' key cannot be detected in software. Please use another key in config.json.\n";
        return false;
    }
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "[KeyDetector] Cannot open X display.\n";
        return false;
    }
    // Split the key combination (e.g., "Ctrl+1")
    std::vector<std::string> keys = split(keyCombo, '+');
    char keymap[32];
    XQueryKeymap(display, keymap);
    bool allPressed = true;
    for (const auto& key : keys) {
        KeySym keysym = keyNameToKeySym(key);
        if (keysym == NoSymbol) {
            std::cerr << "[KeyDetector] Unknown key name: '" << key << "'.\n";
            allPressed = false;
            break;
        }
        KeyCode keycode = XKeysymToKeycode(display, keysym);
        if (!(keymap[keycode / 8] & (1 << (keycode % 8)))) {
            allPressed = false;
            break;
        }
    }
    XCloseDisplay(display);
    return allPressed;
} 