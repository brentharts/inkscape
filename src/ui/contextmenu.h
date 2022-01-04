// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CONTEXTMENU_H
#define SEEN_CONTEXTMENU_H

/*
 * Context menu
 *
 * Authors:
 *   Tavmjong Bah
 *
 * Copyright (C) 2022 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm/menu.h>
#include <giomm.h>

class SPDesktop;
class SPItem;

/**
 * Implements the Inkscape context menu.
 */
class ContextMenu : public Gtk::Menu
{
    public:
        ContextMenu(SPDesktop *desktop, SPItem *item);
        ~ContextMenu() override = default;

    private:
        void AppendItemFromAction(Glib::RefPtr<Gio::Menu> gmenu, Glib::ustring action, Glib::ustring label, Glib::ustring icon = "");

};
#endif // SEEN_CONTEXT_MENU_H

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
