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

#ifdef HAVE_LIBPORTAL
static void on_request_response_signal(GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar*, GVariant* parameters, gpointer user_data)
{
    unsigned int response = 2;
    GVariant* dict = nullptr;
    g_variant_get(parameters, "(u@a{sv})", &response, &dict);
    if (!dict)
    {
        DEBUG(3, "WaylandSession: Request::Response with no dict");
        return;
    }
    GVariant* v = g_variant_lookup_value(dict, "session_handle", G_VARIANT_TYPE_STRING);
    if (v)
    {
        const char* path = g_variant_get_string(v, nullptr);
        if (path)
        {
            std::string* outPath = reinterpret_cast<std::string*>(user_data);
            *outPath = path;
        }
        g_variant_unref(v);
    }
    g_variant_unref(dict);
}

static std::string build_request_path(GDBusConnection* bus, const char* token)
{
    const char* unique = g_dbus_connection_get_unique_name(bus); // e.g. ":1.42"
    std::string s = unique ? unique : ":0.0";
    // strip leading ':' and replace '.' with '_'
    if (!s.empty() && s[0] == ':') s.erase(0, 1);
    for (char& c : s) if (c == '.') c = '_';
    std::string path = "/org/freedesktop/portal/desktop/request/";
    path += s;
    path += "/";
    path += token;
    return path;
}

static bool call_create_session(GDBusConnection* bus, std::string& outSessionPath)
{
    GError* error = nullptr;
    char token[64];
    snprintf(token, sizeof(token), "coral%u", (unsigned)g_random_int());

    // Predict the Request object path and subscribe before making the call to avoid race
    std::string reqPath = build_request_path(bus, token);
    guint sub_id = g_dbus_connection_signal_subscribe(
        bus,
        "org.freedesktop.portal.Desktop",
        "org.freedesktop.portal.Request",
        "Response",
        reqPath.c_str(),
        nullptr,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_request_response_signal,
        &outSessionPath,
        nullptr);

    // Build options dict with handle_token and session_handle_token
    GVariantBuilder opts;
    g_variant_builder_init(&opts, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&opts, "{sv}", "handle_token", g_variant_new_string(token));
    g_variant_builder_add(&opts, "{sv}", "session_handle_token", g_variant_new_string(token));

    // Call CreateSession -> returns Request object path
    GVariant* result = g_dbus_connection_call_sync(
        bus,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.RemoteDesktop",
        "CreateSession",
        g_variant_new("(a{sv})", &opts),
        G_VARIANT_TYPE("(o)"),
        G_DBUS_CALL_FLAGS_NONE,
        30000,
        nullptr,
        &error);

    if (!result)
    {
        ERROR(std::string("RemoteDesktop.CreateSession failed: ") + (error ? error->message : "unknown"));
        if (error) g_error_free(error);
        g_dbus_connection_signal_unsubscribe(bus, sub_id);
        return false;
    }
    g_variant_unref(result);

    // Wait for the signal to populate outSessionPath
    for (int i = 0; i < 400; ++i)
    {
        if (!outSessionPath.empty()) break;
        while (g_main_context_iteration(nullptr, false)) {}
        g_usleep(5000);
    }
    g_dbus_connection_signal_unsubscribe(bus, sub_id);

    if (outSessionPath.empty())
    {
        ERROR("RemoteDesktop.CreateSession did not return a session_handle in time");
        return false;
    }
    return true;
}

static bool call_select_devices(GDBusConnection* bus, const std::string& sessionPath)
{
    GError* error = nullptr;

    // options: types = KEYBOARD (1), persist_mode (1) persist while app runs
    GVariantBuilder opts;
    g_variant_builder_init(&opts, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&opts, "{sv}", "types", g_variant_new_uint32(1));
    g_variant_builder_add(&opts, "{sv}", "persist_mode", g_variant_new_uint32(1));

    GVariant* result = g_dbus_connection_call_sync(
        bus,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.RemoteDesktop",
        "SelectDevices",
        g_variant_new("(oa{sv})", sessionPath.c_str(), &opts),
        G_VARIANT_TYPE("(o)"),
        G_DBUS_CALL_FLAGS_NONE,
        30000,
        nullptr,
        &error);

    if (!result)
    {
        ERROR(std::string("RemoteDesktop.SelectDevices failed: ") + (error ? error->message : "unknown"));
        if (error) g_error_free(error);
        return false;
    }
    g_variant_unref(result);
    return true;
}

static bool call_start(GDBusConnection* bus, const std::string& sessionPath)
{
    GError* error = nullptr;

    // parent_window empty string; options empty
    GVariantBuilder opts;
    g_variant_builder_init(&opts, G_VARIANT_TYPE_VARDICT);

    GVariant* result = g_dbus_connection_call_sync(
        bus,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.RemoteDesktop",
        "Start",
        g_variant_new("(osa{sv})", sessionPath.c_str(), "", &opts),
        G_VARIANT_TYPE("(o)"),
        G_DBUS_CALL_FLAGS_NONE,
        60000,
        nullptr,
        &error);

    if (!result)
    {
        ERROR(std::string("RemoteDesktop.Start failed: ") + (error ? error->message : "unknown"));
        if (error) g_error_free(error);
        return false;
    }
    g_variant_unref(result);
    return true;
}

static bool notify_keysym(GDBusConnection* bus, const std::string& sessionPath, unsigned int keysym, unsigned int state)
{
    GError* error = nullptr;
    GVariantBuilder opts;
    g_variant_builder_init(&opts, G_VARIANT_TYPE_VARDICT);

    GVariant* result = g_dbus_connection_call_sync(
        bus,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.RemoteDesktop",
        "NotifyKeyboardKeysym",
        g_variant_new("(oa{s}uu)", sessionPath.c_str(), &opts, keysym, state),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        10000,
        nullptr,
        &error);

    if (error)
    {
        ERROR(std::string("NotifyKeyboardKeysym failed: ") + error->message);
        g_error_free(error);
        return false;
    }
    if (result) g_variant_unref(result);
    return true;
}
#endif

bool WaylandSession::ensureInitialized()
{
#ifdef HAVE_LIBPORTAL
    if (_initialized) return true;

    ensure_glib_main_loop();

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
        if (error) { DEBUG(3, std::string("WaylandSession: Clipboard portal not available: ") + error->message); g_error_free(error); }
    }

    _rdpSessionObjectPath.clear();
    if (!call_create_session(_sessionBus, _rdpSessionObjectPath))
    {
        return false;
    }

    if (!call_select_devices(_sessionBus, _rdpSessionObjectPath))
    {
        return false;
    }

    if (!call_start(_sessionBus, _rdpSessionObjectPath))
    {
        return false;
    }

    _rdpStarted = true;
    DEBUG(3, "WaylandSession: Session bus and portal proxies initialized");
    _initialized = true;
    return true;
#else
    DEBUG(3, "WaylandSession: libportal not available at build time");
    return false;
#endif
}

bool WaylandSession::setClipboard(const std::string& text)
{
#ifdef HAVE_LIBPORTAL
    if (!_initialized) return false;
    if (!_clipboardProxy)
    {
        DEBUG(3, "WaylandSession::setClipboard: Clipboard portal proxy is null (portal not available)");
        return false;
    }

    auto try_call = [&](const char* method)->bool{
        DEBUG(3, std::string("WaylandSession: trying Clipboard portal method ") + method);
        GError* error = nullptr;
        GVariantBuilder opts;
        g_variant_builder_init(&opts, G_VARIANT_TYPE_VARDICT);
        g_variant_builder_add(&opts, "{sv}", "text", g_variant_new_string(text.c_str()));
        GVariant* result = g_dbus_proxy_call_sync(
            _clipboardProxy,
            method,
            g_variant_new("(a{sv})", &opts),
            G_DBUS_CALL_FLAGS_NONE,
            10000,
            nullptr,
            &error);
        if (!result)
        {
            if (error)
            {
                DEBUG(3, std::string("WaylandSession::") + method + " failed: " + error->message);
                g_error_free(error);
            }
            return false;
        }
        g_variant_unref(result);
        return true;
    };

    if (try_call("SetSelection"))
    {
        DEBUG(3, "WaylandSession: Clipboard text set via SetSelection");
        return true;
    }
    if (try_call("SetClipboard"))
    {
        DEBUG(3, "WaylandSession: Clipboard text set via SetClipboard");
        return true;
    }

    DEBUG(3, "WaylandSession: Clipboard portal calls did not succeed; will fall back to typing if available");
    return false;
#else
    return false;
#endif
}

bool WaylandSession::sendCtrlV()
{
#ifdef HAVE_LIBPORTAL
    if (!_initialized || !_rdpStarted || _rdpSessionObjectPath.empty()) return false;

    const unsigned int XK_Control_L = 0xffe3;
    const unsigned int XK_v = 0x0076;
    bool ok = true;
    ok &= notify_keysym(_sessionBus, _rdpSessionObjectPath, XK_Control_L, 1);
    ok &= notify_keysym(_sessionBus, _rdpSessionObjectPath, XK_v, 1);
    ok &= notify_keysym(_sessionBus, _rdpSessionObjectPath, XK_v, 0);
    ok &= notify_keysym(_sessionBus, _rdpSessionObjectPath, XK_Control_L, 0);
    return ok;
#else
    return false;
#endif
}

bool WaylandSession::typeText(const std::string& text)
{
#ifdef HAVE_LIBPORTAL
    if (!_initialized || !_rdpStarted || _rdpSessionObjectPath.empty()) return false;

    auto send_char = [&](char c)->bool{
        unsigned int ks = 0;
        bool needsShift = false;
        if (c >= 'a' && c <= 'z') ks = (unsigned int)c;
        else if (c >= 'A' && c <= 'Z') { ks = (unsigned int)(c + 32); needsShift = true; }
        else if (c >= '0' && c <= '9') ks = (unsigned int)c;
        else if (c == ' ') ks = 0x0020;
        else if (c == '\n') { ks = 0xff0d; }
        else if (c == '\t') { ks = 0xff09; }
        else if (c == '.') { ks = 0x002e; }
        else if (c == ',') { ks = 0x002c; }
        else if (c == '-') { ks = 0x002d; }
        else if (c == '_') { ks = 0x002d; needsShift = true; }
        else if (c == '+') { ks = 0x003d; needsShift = true; }
        else if (c == '=') { ks = 0x003d; }
        else if (c == '/') { ks = 0x002f; }
        else if (c == ':') { ks = 0x003b; needsShift = true; }
        else if (c == ';') { ks = 0x003b; }
        else if (c == '\'') { ks = 0x0027; }
        else if (c == '"') { ks = 0x0027; needsShift = true; }
        else if (c == '[') { ks = 0x005b; }
        else if (c == ']') { ks = 0x005d; }
        else if (c == '(') { ks = 0x0039; needsShift = true; }
        else if (c == ')') { ks = 0x0030; needsShift = true; }
        else if (c == '!') { ks = 0x0031; needsShift = true; }
        else if (c == '?') { ks = 0x002f; needsShift = true; }
        else {
            return true;
        }
        const unsigned int XK_Shift_L = 0xffe1;
        bool ok = true;
        if (needsShift) ok &= notify_keysym(_sessionBus, _rdpSessionObjectPath, XK_Shift_L, 1);
        ok &= notify_keysym(_sessionBus, _rdpSessionObjectPath, ks, 1);
        ok &= notify_keysym(_sessionBus, _rdpSessionObjectPath, ks, 0);
        if (needsShift) ok &= notify_keysym(_sessionBus, _rdpSessionObjectPath, XK_Shift_L, 0);
        return ok;
    };

    bool okAll = true;
    for (char c : text)
    {
        if (!send_char(c)) okAll = false;
    }
    return okAll;
#else
    return false;
#endif
} 