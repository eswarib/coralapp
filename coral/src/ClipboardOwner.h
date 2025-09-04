#ifndef CLIPBOARD_OWNER_H
#define CLIPBOARD_OWNER_H

#include <X11/Xlib.h>
#include <string>

class ClipboardOwner {
public:
    ClipboardOwner(Display* display, Window ownerWindow);

    void serveRequests(const std::string& text,
                       int timeoutMs,
                       int pollIntervalMs,
                       int idleExitMs);

private:
    Display* _display;
    Window _window;
    Atom _clipboardAtom;
    Atom _primaryAtom;
    Atom _utf8Atom;
    Atom _targetsAtom;
};

#endif // CLIPBOARD_OWNER_H

