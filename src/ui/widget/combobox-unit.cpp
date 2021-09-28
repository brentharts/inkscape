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
 * Menu of units.
 *
 * Rewrite of UnitTracker as a Gtk::ComboBoxText.
 */

#include "combobox-unit.h"

#include <iostream>
#include <iomanip>

#include <glibmm/i18n.h>
#include <giomm/application.h>
#include <gtkmm.h>

#include "util/units.h"

namespace Inkscape {
namespace UI {
namespace Widget {

// Dummy constructor to register type.
ComboBoxUnit::ComboBoxUnit()
    : ScrollProtected<Gtk::ComboBoxText>()
    , Glib::ObjectBase("ComboBoxUnit")
    , prop_unit_type(*this, "unit-type")
#if !GTK_CHECK_VERSION(4, 0, 0)
    , prop_menu_label(*this, "menu-label")
#endif
{
}

ComboBoxUnit::ComboBoxUnit(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refUI)
    : ScrollProtected<Gtk::ComboBoxText>(cobject)
    , Glib::ObjectBase("ComboBoxUnit")
    , prop_unit_type(*this, "unit-type")
#if !GTK_CHECK_VERSION(4, 0, 0)
    , prop_menu_label(*this, "menu-label")
#endif
{
    // std::cout << "ComboBoxUnit::ComboBoxUnit(): UI constructor: "
    //           << get_name()
    //           << "  unit_type: " << prop_unit_type
    //           << std::endl;

    Inkscape::Util::UnitType unit_type = Inkscape::Util::UNIT_TYPE_NONE;
    if (prop_unit_type.get_value() == "Linear") {
        unit_type = Inkscape::Util::UNIT_TYPE_LINEAR;
    } else {
        std::cerr << "ComboBoxUnit::ComboBoxUnit: unhandled unit type!" << std::endl;
    }

    auto unit_table_map = Inkscape::Util::unit_table.units(unit_type);

#if !GTK_CHECK_VERSION(4, 0, 0)
    // Only for GTK3 Toolbar overflow menus.
    application = Gtk::Application::get_default(); // Used to access action.
    if (!application) {
        std::cerr << "ComboBoxUnit: no application!" << std::endl;
        return;
    }

    action_group = Gio::SimpleActionGroup::create();
    Glib::ustring action_name = "unit-menu";
    action =
        action_group->add_action_radio_string(action_name,
                                              sigc::mem_fun(*this, &ComboBoxUnit::set_unit), "");

    gmenu = Gio::Menu::create();
#endif

    for (auto& map_entry : unit_table_map) {
        append(map_entry.first);
#if !GTK_CHECK_VERSION(4, 0, 0)
        Glib::ustring full_action_name = "rect." + action_name + "('" + map_entry.first + "')";
        auto gitem = Gio::MenuItem::create(map_entry.first, full_action_name);
        gmenu->append_item(gitem);
#endif
    }

#if !GTK_CHECK_VERSION(4, 0, 0)
    // Actions are only needed for "overflow" toolbar menu.
    get_toplevel()->insert_action_group("rect", action_group);
    action->change_state(Glib::ustring("px"));
#endif

    set_active_text("px");
}

ComboBoxUnit::~ComboBoxUnit()
{
#if !GTK_CHECK_VERSION(4, 0, 0)
    // Don't need to do this as this only happens when toplevel is destroyed.
    // (If we do need to do this, then must be done earlier or we get a segfault.)
    // get_toplevel()->remove_action_group("rect");
#endif
}

// ------------ Class functions ------------

#if !GTK_CHECK_VERSION(4, 0, 0)
// Create menu for GTK3 ToolItemMenu
Gtk::MenuItem*
ComboBoxUnit::get_menu()
{
  Glib::ustring label = prop_menu_label.get_value();
  if (label == "") {
      label = _("Units");
  }

  action->change_state(get_active_text());
  auto menu_item = Gtk::manage(new Gtk::MenuItem(label));
  auto menu = Gtk::manage(new Gtk::Menu(gmenu));
  menu_item->set_submenu(*menu);
  return menu_item;
}
#endif

void
ComboBoxUnit::set_unit(Glib::ustring unit)
{
    static bool block = false; // Check if this is really needed.
    if (block) return;
    block = true;

    action->change_state(unit);
    set_active_text(unit);

    block = false;
}

const Inkscape::Util::Unit*
ComboBoxUnit::get_unit()
{
    Inkscape::Util::UnitType unit_type = Inkscape::Util::UNIT_TYPE_NONE;
    if (prop_unit_type.get_value() == "Linear") {
        unit_type = Inkscape::Util::UNIT_TYPE_LINEAR;
    } else {
        std::cerr << "ComboBoxUnit::get_unit: unhandled unit type!" << std::endl;
    }

    auto unit_table_map = Inkscape::Util::unit_table.units(unit_type);
    return Inkscape::Util::unit_table.getUnit(get_active_text());
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
