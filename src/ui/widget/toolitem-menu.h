// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2021 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

/**  A Gtk::ToolItem with the ability to set a proxy menu.
 *
 * Proxy menu can be set via a property pointing to a menu
 * or obtained from a child widget (e.g. SpinButtonAction).
 *
 * Only applicable to GTK3.
 */

#ifndef TOOLITEM_MENU_H
#define TOOLITEM_MENU_H

#include <giomm/application.h>
#include <gtkmm.h>

namespace Inkscape {
namespace UI {
namespace Widget {

class ToolItemMenu : public Gtk::ToolItem
{
    using parent_type = Gtk::ToolItem;

public:
    ToolItemMenu();
    ToolItemMenu(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
    virtual ~ToolItemMenu();

    Glib::PropertyProxy<Glib::ustring> property_menuitem_name() { return prop_menuitem_name.get_proxy(); }

private:
    bool on_create_menu_proxy() override;
    bool process_tab(int increment);
    bool on_key_press_event(GdkEventKey* event) override;

    // Properties
    Glib::Property<Glib::ustring> prop_menuitem_name;

    Gtk::MenuItem* menuitem_ui = nullptr;
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif // TOOLITEM_MENU_H

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
