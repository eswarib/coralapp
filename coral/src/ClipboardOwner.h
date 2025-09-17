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
    Atom _textAtom;                 // "TEXT"
    Atom _compoundTextAtom;         // "COMPOUND_TEXT"
    Atom _textPlainAtom;            // "text/plain"
    Atom _textPlainUtf8Atom;        // "text/plain;charset=utf-8"
};

#endif // CLIPBOARD_OWNER_H

