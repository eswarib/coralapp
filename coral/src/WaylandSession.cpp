#include "WaylandSession.h"
#include "Logger.h"

#ifdef HAVE_LIBPORTAL
#include <libportal/portal.h>
#include <gio/gio.h>
#include <glib.h>
#include <thread>
#include <atomic>

static std::atomic<bool> s_glibLoopStarted{false};
static GMainLoop* s_mainLoop = nullptr;
static std::thread s_glibThread;

static void ensure_glib_main_loop()
{
    if (s_glibLoopStarted.load()) return;
    s_glibLoopStarted.store(true);
    s_glibThread = std::thread([](){
        s_mainLoop = g_main_loop_new(nullptr, FALSE);
        DEBUG(3, "WaylandSession: GLib main loop starting");
        g_main_loop_run(s_mainLoop);
        DEBUG(3, "WaylandSession: GLib main loop exited");
        g_main_loop_unref(s_mainLoop);
        s_mainLoop = nullptr;
    });
    s_glibThread.detach();
}
#endif

WaylandSession& WaylandSession::getInstance()
{
    static WaylandSession instance;
    return instance;
}

bool WaylandSession::ensureInitialized()
{
#ifdef HAVE_LIBPORTAL
    if (_initialized) return true;

    ensure_glib_main_loop();

    // Create a portal context
    _portal = xdp_portal_new();
    if (!_portal)
    {
        ERROR("WaylandSession: failed to create XdpPortal context");
        return false;
    }

    GError* error = nullptr;
    _sessionBus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
    if (!_sessionBus)
    {
        ERROR(std::string("WaylandSession: failed to get session bus: ") + (error ? error->message : "unknown"));
        if (error) g_error_free(error);
        return false;
    }

    // Create proxies to the portals we need
    _remoteDesktopProxy = g_dbus_proxy_new_sync(
        _sessionBus,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.RemoteDesktop",
        nullptr,
        &error);
    if (!_remoteDesktopProxy)
    {
        ERROR(std::string("WaylandSession: failed to create RemoteDesktop proxy: ") + (error ? error->message : "unknown"));
        if (error) g_error_free(error);
        return false;
    }

    _clipboardProxy = g_dbus_proxy_new_sync(
        _sessionBus,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.Clipboard",
        nullptr,
        &error);
    if (!_clipboardProxy)
    {
        WARN(std::string("WaylandSession: Clipboard portal not available: ") + (error ? error->message : "unknown"));
        if (error) { g_error_free(error); error = nullptr; }
    }

    DEBUG(3, "WaylandSession: Session bus and portal proxies initialized");
    _initialized = true;
    return true;
#else
    DEBUG(3, "WaylandSession: libportal not available at build time");
    return false;
#endif
}

bool WaylandSession::setClipboard(const std::string&)
{
#ifdef HAVE_LIBPORTAL
    WARN("WaylandSession::setClipboard: not implemented yet");
    return false;
#else
    return false;
#endif
}

bool WaylandSession::sendCtrlV()
{
#ifdef HAVE_LIBPORTAL
    WARN("WaylandSession::sendCtrlV: not implemented yet");
    return false;
#else
    return false;
#endif
}

bool WaylandSession::typeText(const std::string&)
{
#ifdef HAVE_LIBPORTAL
    WARN("WaylandSession::typeText: not implemented yet");
    return false;
#else
    return false;
#endif
} 