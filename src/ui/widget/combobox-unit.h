// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2021 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

/** Menu of units.
 *
 * Provides menu for GTK3 Inkscape::UI::Widget::ToolItemMenu class.
 *
 * GTK4 support is sketched in, may not be fully functional.
 *
 * Rewrite of UnitTracker as a Gtk::ComboBoxText.
 */

#ifndef COMBOBOX_UNIT_H
#define COMBOBOX_UNIT_H

#include <giomm/application.h>
#include <gtkmm.h>

// Prevents the widget from stealing events when the containing widget is being scrolled.
#include "scrollprotected.h"

namespace Inkscape::Util {
class Unit;
}

namespace Inkscape {
namespace UI {
namespace Widget {

class UnitMenu;
class UnitTracker;

/** A ComboBoxText for units.
 *
 */
class ComboBoxUnit : public ScrollProtected<Gtk::ComboBoxText>
{
    using parent_type = ScrollProtected<Gtk::ComboBoxText>;

public:

    ComboBoxUnit(); // Dummy for declaring type.
    ComboBoxUnit(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refUI);
    ~ComboBoxUnit() override;

#if !GTK_CHECK_VERSION(4, 0, 0)
    Gtk::MenuItem* get_menu(); // Used in GTK3 toolbars.
#endif

    void set_unit(Glib::ustring unit);
    const Inkscape::Util::Unit* get_unit();

    Glib::PropertyProxy<Glib::ustring> property_unit_type() { return prop_unit_type.get_proxy(); }

private:

    Glib::RefPtr<Gio::Application> application;

#if !GTK_CHECK_VERSION(4, 0, 0)
    Glib::RefPtr<Gio::Menu> gmenu;
    Glib::RefPtr<Gio::SimpleActionGroup> action_group;
    Glib::RefPtr<Gio::SimpleAction> action;
#endif

    // Properties
    Glib::Property<Glib::ustring> prop_unit_type;
#if !GTK_CHECK_VERSION(4, 0, 0)
    Glib::Property<Glib::ustring> prop_menu_label; // Used for GTK3 proxy menu.
#endif
};


} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif // COMBOBOX_UNIT_H

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
