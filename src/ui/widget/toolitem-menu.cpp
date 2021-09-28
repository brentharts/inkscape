// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2021 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

/*
 * A Gtk::ToolItem with the ability to set the menu via a property.
 */

#include "toolitem-menu.h"

#include <iostream>
#include <iomanip>

#include <giomm/application.h>
#include <gtkmm.h>

#include "spinbutton-action.h"
#include "combobox-unit.h"

namespace Inkscape {
namespace UI {
namespace Widget {

// Dummy constructor to register type.
// Dummy constructor to register type.
ToolItemMenu::ToolItemMenu()
    : Gtk::ToolItem()
    , Glib::ObjectBase("ToolItemMenu")
    , prop_menuitem_name(*this, "menuitem-name")
{
}

ToolItemMenu::ToolItemMenu(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
    : Gtk::ToolItem(cobject)
    , Glib::ObjectBase("ToolItemMenu")
    , prop_menuitem_name(*this, "menuitem-name")
{
    // Get menu from .ui file (if one is defined there).
    Glib::ustring menuitem_name = prop_menuitem_name.get_value();
    if (menuitem_name != "") {
        builder->get_widget(menuitem_name, menuitem_ui);
        if (menuitem_ui) {
            menuitem_ui->reference(); // Menu item is not attached to a top level so if builder is
                                      // deleted, this is deleted too unless we reference it.
        } else {
            std::cerr << "ToolItemMenu::ToolItemMenu: did not find proxy menu from .ui file: "
                      << menuitem_name << std::endl;
        }
    }
}

ToolItemMenu::~ToolItemMenu()
{
    if (menuitem_ui) {
        menuitem_ui->unreference();
    }
}

bool ToolItemMenu::on_create_menu_proxy()
{
    Gtk::MenuItem* menuitem = nullptr;

    if (menuitem_ui) {
        // Use menu defined in a .ui file.
        menuitem = menuitem_ui;
    } else {
        // Use menu defined in a child widget (only some widgets).
        auto child = get_child();
        if (child) {

            auto spinbutton = dynamic_cast<SpinButtonAction*>(child);
            if (spinbutton) {
                menuitem = spinbutton->get_menu();
            }

            auto comboboxunit = dynamic_cast<ComboBoxUnit*>(child);
            if (comboboxunit) {
                menuitem = comboboxunit->get_menu();
            }
        }
    }

    if (menuitem) {
        set_proxy_menu_item(prop_menuitem_name.get_value(), *menuitem);
    } else {
        std::cerr << "ToolItemMenu::on_create_menu_proxy(): No proxy menu!" << std::endl;
    }

    return true;
}

bool ToolItemMenu::process_tab(int increment)
{
    bool handled = false;

    auto toolbar = dynamic_cast<Gtk::Toolbar *>(get_parent());
    if (toolbar) {
        auto index   = toolbar->get_item_index(*this);
        auto n_items = toolbar->get_n_items();

        auto test_index = index + increment;
        while (0 < test_index && test_index < n_items) {
            auto item = toolbar->get_nth_item(test_index);
            auto tool_item = dynamic_cast<ToolItemMenu *>(item);
            if (tool_item) {
                auto child = tool_item->get_child();
                if (child && child->get_can_focus() && child->is_sensitive()) {
                    child->grab_focus();
                    handled = true;
                    break;
                }
            }
            test_index += increment;
        }
    }
    return handled;
}

// Tabbing between widgets appears broken in Gtk Toolbars. This partially fixes it.
bool ToolItemMenu::on_key_press_event(GdkEventKey* event)
{
    bool handled = false;
    switch (event->keyval) {
        case GDK_KEY_Tab:
            handled = process_tab(1); // Forward one widget.
            break;
        case GDK_KEY_ISO_Left_Tab:
            handled = process_tab(-1); // Backwards one widget.
            break;
        default:
            break;
    }

    if (handled) {
        return true;
    }
    return parent_type::on_key_press_event(event); // We've overridden parent class's function.
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
