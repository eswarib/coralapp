#if defined(_WIN32)
#include "WindowsInjector.h"
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#endif

#include "TextInjector.h"
#include "Logger.h"
#include "X11WindowUtils.h"
#include "ClipboardOwner.h"
#include "WaylandDetector.h"
#include "WaylandInjector.h"
#include "X11Injector.h"
#include "A11yInjector.h"
#include <cstring>
#include <vector>
#include <string>
#include <algorithm> // For std::transform
#include <thread>
#include <chrono>


extern "C" 
{
#include "xdo.h"
}

TextInjector* TextInjector::sInstance = nullptr;

TextInjector* TextInjector::getInstance()
{
    if (sInstance)
    {
        return sInstance;
    }
    sInstance = new TextInjector();
    return sInstance;
}

TextInjector::~TextInjector() 
{
}

// removed local helpers; now using X11WindowUtils and ClipboardOwner

void TextInjector::typeText(const std::string& text)
{
    // AT-SPI first (cleanest when available)
    if (A11yInjector::getInstance().typeText(text))
    {
        DEBUG(3, "TextInjector: injected via AT-SPI EditableText");
        return;
    }
    // Windows path
    #if defined(_WIN32)
    WindowsInjector::getInstance().typeText(text);
    return;
    #endif

    // Wayland path first (Linux)
    if (isWaylandSession())
    {
        DEBUG(3, "Wayland session detected; attempting WaylandInjector");
        if (WaylandInjector::getInstance().typeText(text))
        {
            return;
        }
        DEBUG(3, "WaylandInjector not available/failed; falling back to X11 path if possible");
    }

    // X11 fallback / default (Linux)
    X11Injector::getInstance().typeText(text);
}
