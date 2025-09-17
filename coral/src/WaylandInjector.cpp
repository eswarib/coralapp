#include "WaylandInjector.h"
#include "WaylandSession.h"
#include "Logger.h"

WaylandInjector& WaylandInjector::getInstance()
{
    static WaylandInjector instance;
    return instance;
}

bool WaylandInjector::typeText(const std::string& text)
{
    auto& session = WaylandSession::getInstance();
    if (!session.ensureInitialized())
    {
        DEBUG(3, "WaylandInjector: session unavailable; falling back to X11");
        return false;
    }

    // Preferred path: clipboard + Ctrl+V
    if (session.setClipboard(text))
    {
        if (session.sendCtrlV())
        {
            DEBUG(3, "WaylandInjector: clipboard set and Ctrl+V sent");
            return true;
        }
        WARN("WaylandInjector: clipboard set but Ctrl+V failed; will try typing fallback");
    }
    else
    {
        DEBUG(3, "WaylandInjector: clipboard write not available; will type text");
    }

    // Fallback: type the text via portal input
    if (session.typeText(text))
    {
        DEBUG(3, "WaylandInjector: typed text via portal input");
        return true;
    }

    ERROR("WaylandInjector: failed to inject text under Wayland");
    return false;
} 