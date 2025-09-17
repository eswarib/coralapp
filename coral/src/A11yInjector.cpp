#include "A11yInjector.h"
#include "Logger.h"

#ifdef HAVE_ATSPI
#include <atspi/atspi.h>
#endif

A11yInjector& A11yInjector::getInstance()
{
    static A11yInjector instance;
    return instance;
}

bool A11yInjector::typeText(const std::string& text)
{
#ifdef HAVE_ATSPI
    GError* error = nullptr;
    if (!atspi_init())
    {
        DEBUG(3, "A11yInjector: atspi_init failed");
        return false;
    }

    AtspiAccessible* desktop = atspi_get_desktop(0);
    if (!desktop)
    {
        DEBUG(3, "A11yInjector: no desktop accessible");
        return false;
    }

    // Get the currently focused accessible via event cache
    AtspiAccessible* focus = atspi_get_focus(&error);
    if (!focus)
    {
        DEBUG(3, std::string("A11yInjector: atspi_get_focus failed: ") + (error ? error->message : "unknown"));
        if (error) g_error_free(error);
        return false;
    }

    // Try EditableText first
    AtspiEditableText* editable = atspi_accessible_get_editable_text_iface(focus);
    if (editable)
    {
        bool ok = false;
        // Replace contents if possible; fall back to insert
        if (atspi_editable_text_set_text_contents(editable, text.c_str(), &error))
        {
            ok = true;
        }
        else
        {
            if (error) { g_error_free(error); error = nullptr; }
            // Insert at caret position 0 as a fallback
            if (atspi_editable_text_insert_text(editable, 0, text.c_str(), text.size(), &error))
            {
                ok = true;
            }
            else
            {
                DEBUG(3, std::string("A11yInjector: insert_text failed: ") + (error ? error->message : "unknown"));
            }
        }
        g_object_unref(focus);
        g_object_unref(desktop);
        return ok;
    }

    g_object_unref(focus);
    g_object_unref(desktop);
    return false;
#else
    return false;
#endif
} 