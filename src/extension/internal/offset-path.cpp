/// SPDX-License-Identifier: GPL-2.0-or-later
/// @auth:sabagara <sabagara@caret-works.net>
/// @brief Size Specification Offset Extension

#include "offset-path.h"

#include <boost/lexical_cast.hpp>
#include <regex>
#include <string>
#include <vector>

#include "desktop.h"
#include "document.h"
#include "extension/effect.h"
#include "extension/system.h"
#include "message-stack.h"
#include "object/sp-item.h"
#include "path-chemistry.h"
#include "selection.h"
#include "splivarot.h"
#include "ui/interface.h"
#include "ui/view/view.h"
#include "util/units.h"

static char const *const OFFSET_DEFAULT_UNIT_NAME = DEFAULT_UNIT_NAME;

namespace Inkscape {
namespace Extension {
namespace Internal {

/// Load Extension
bool OffsetPath::load(Inkscape::Extension::Extension *)
{
    return TRUE;
}

/// Exec Offset
void OffsetPath::effect(Inkscape::Extension::Effect *module, Inkscape::UI::View::View *desktop, Inkscape::Extension::Implementation::ImplementationDocumentCache *)
{
    // Get parameter from extension form
    std::string offset_str;
    try {
        offset_str = module->get_param_string("offset");
    }
    catch (...) {
        g_error("Parameter <offset> might not exist");
        return;
    }

    // Paser offset (num/unit)
    double offset_size = 0.0f;
    std::string offset_unit = "px";
    std::regex regexNum("^(-?\\d+\\.?\\d*)\\s*(|px|in|mm|pc|cm|pt)$");
    std::smatch match;
    if (std::regex_match(offset_str, match, regexNum)) {
        if (match.size() == 3) {
            offset_size = boost::lexical_cast<double>(match[1]);
            offset_unit = match[2];
        }
    }
    else {
        gchar *const warning_dialog_message = g_strdup_printf("%s (%s)", N_("Invalid Offset Size specification."), offset_str.c_str());
        g_warning("%s", warning_dialog_message);
        sp_ui_error_dialog(warning_dialog_message);
        g_free(warning_dialog_message);
        return;
    }

    // Size Unit Change
    offset_size = Inkscape::Util::Quantity::convert(offset_size, offset_unit, OFFSET_DEFAULT_UNIT_NAME);
    g_debug("new offset_size(mm):%f", offset_size);

    // Put selected item into container
    Inkscape::Selection *const selection = static_cast<SPDesktop *>(desktop)->selection;
    std::vector<SPItem *> items(selection->items().begin(), selection->items().end());
    selection->clear();

    for (SPItem *const spitem : items) {
        // Duplicate target node
        Inkscape::XML::Document *const xml_doc = desktop->doc()->getReprDoc();
        Inkscape::XML::Node *const new_item = spitem->getRepr()->duplicate(xml_doc);
        spitem->getRepr()->parent()->appendChild(new_item);
        selection->add(new_item);
        selection->toCurves();

        // Offset (offset unit is mm)
        if (0.0 < offset_size) {
            sp_selected_path_do_offset(static_cast<SPDesktop *>(desktop), true, offset_size);
        }
        else if (offset_size < 0.0) {
            sp_selected_path_do_offset(static_cast<SPDesktop *>(desktop), false, -offset_size);
        }

        selection->clear();
    }
}

/// Preference effect
Gtk::Widget *OffsetPath::prefs_effect(Inkscape::Extension::Effect *const module, Inkscape::UI::View::View *, sigc::signal<void> *const changeSignal, Inkscape::Extension::Implementation::ImplementationDocumentCache *)
{
    return module->autogui(nullptr, nullptr, changeSignal);
}

#include "clear-n_.h"

/// Extension Setting
void OffsetPath::init()
{
    Inkscape::Preferences *const prefs = Inkscape::Preferences::get();
    double const pref_offset = prefs->getDouble("/options/defaultoffsetwidth/value", 1.0, OFFSET_DEFAULT_UNIT_NAME);

    gchar *const extension_manifest = g_strdup_printf(
                                          "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
                                          "<name>" N_("Offset Path") "</name>\n"
                                          "<id>org.inkscape.effect.offset-path</id>\n"
                                          "<param name=\"offset\" gui-text=\"" N_("Offset") " :\" gui-description=\"" N_("Offset Size") "\" type=\"string\">%.2f mm</param>\n"
                                          "<effect>\n"
                                          "<object-type>all</object-type>\n"
                                          "<effects-menu>\n"
                                          "<submenu name=\"" N_("Generate from Path") "\" />\n"
                                          "</effects-menu>\n"
                                          "</effect>\n"
                                          "</inkscape-extension>\n",
                                          pref_offset
                                      );

    Inkscape::Extension::build_from_mem(extension_manifest, new OffsetPath());
    g_free(extension_manifest);
}

}; // namespace Internal
}; // namespace Extension
}; // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
