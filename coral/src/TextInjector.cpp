#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include "TextInjector.h"
#include "Logger.h"
#include "X11WindowUtils.h"
#include "ClipboardOwner.h"
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
    // Use X11 directly for clipboard operations
    Display* display = XOpenDisplay(NULL);
    if (!display) 
    {
        ERROR("Failed to open X display");
        return;
    }

    // We need a window to own the clipboard selection
    Window window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 1, 1, 0, 0, 0);

    // Own the clipboard selections via RAII object
    ClipboardOwner clipboardOwner(display, window);

    // Wait longer to ensure clipboard is fully established
    std::this_thread::sleep_for(std::chrono::milliseconds(TextInjector::kInitialSleepBeforePasteMs));
    
    // Additional synchronization - ensure clipboard ownership is confirmed
    XFlush(display);
    XSync(display, False);
    
    // Small additional delay to ensure X11 events are processed
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Use xdo to simulate the paste command
    xdo_t* xdo = xdo_new_with_opened_display(display, NULL, 0);
    if (!xdo) 
    {
        ERROR("Failed to initialize xdo context");
        XDestroyWindow(display, window);
        XCloseDisplay(display);
        return;
    }

    Window focused_window = 0;
    if (xdo_get_focused_window(xdo, &focused_window) != 0 || focused_window == 0) 
    {
        ERROR("Failed to get focused window with xdo");
    } 
    else 
    {
        // Use focused window directly for key events to reduce WM routing issues
        Window top_window = focused_window;
        DEBUG(3, std::string("Focused window ID: ") + std::to_string(top_window));
        
        // Check if the focused window is a terminal to use the correct shortcut
        const char* paste_shortcut = X11WindowUtils::isTerminal(display, top_window) ? "ctrl+shift+v" : "ctrl+v";
        DEBUG(3, std::string("Pasting text into focused window using shortcut: ") + paste_shortcut);
        
        // Send paste command with increased delay for reliability
        int delay = TextInjector::kKeySequenceDelayUs; // microseconds
        xdo_send_keysequence_window(xdo, top_window, paste_shortcut, delay);
        DEBUG(3, "Paste command sent");
        
        // Flush display to ensure paste command is processed
        XFlush(display);
        
        // Wait a bit for the paste command to be processed
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // After initiating paste, serve SelectionRequest so the target can retrieve data
        clipboardOwner.serveRequests(text,
                                     TextInjector::kClipboardServeTimeoutMs,
                                     TextInjector::kClipboardPollIntervalMs,
                                     TextInjector::kServeIdleExitMs);
    }

    // Cleanup
    xdo_free(xdo); // This also closes the display via xdo_close_display_when_free
    XDestroyWindow(display, window);
}
