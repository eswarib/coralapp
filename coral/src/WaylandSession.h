#pragma once

#include <string>

class WaylandSession
{
public:
    static WaylandSession& getInstance();

    // Ensure the portal session is initialized (lazy)
    bool ensureInitialized();

    // Clipboard write via portal; returns false if unsupported or failed
    bool setClipboard(const std::string& text);

    // Send Ctrl+V via portal input; returns false if unsupported or failed
    bool sendCtrlV();

    // Fallback typing via portal input; returns false if unsupported or failed
    bool typeText(const std::string& text);

private:
    WaylandSession() = default;

    bool _initialized = false;

#ifdef HAVE_LIBPORTAL
    // Forward typedefs instead of including headers here
    typedef struct _XdpPortal XdpPortal;
    typedef struct _XdpSession XdpSession;
    typedef struct _GDBusConnection GDBusConnection;
    typedef struct _GDBusProxy GDBusProxy;

    XdpPortal* _portal = nullptr;
    XdpSession* _rdpSession = nullptr; // RemoteDesktop/Input session

    // GDBus objects for direct portal access
    GDBusConnection* _sessionBus = nullptr;
    GDBusProxy* _remoteDesktopProxy = nullptr; // org.freedesktop.portal.RemoteDesktop
    GDBusProxy* _clipboardProxy = nullptr;     // org.freedesktop.portal.Clipboard (if available)
#endif
}; 