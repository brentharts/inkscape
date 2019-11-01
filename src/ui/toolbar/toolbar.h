// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_TOOLBAR_H
#define SEEN_TOOLBAR_H

#include <gtkmm/toolbar.h>
#include "ui/ink-icon-size.h"
#include "ui/icon-loader.h"

class SPDesktop;

namespace Gtk {
    class Label;
    class ToggleToolButton;
}

namespace Inkscape {
namespace UI {
namespace Toolbar {
/**
 * \brief An abstract definition for a toolbar within Inkscape
 *
 * \detail This is basically the same as a Gtk::Toolbar but contains a
 *         few convenience functions. All toolbars must define a "create"
 *         function that adds all the required tool-items and returns the
 *         toolbar as a GtkWidget
 */
class Toolbar : public Gtk::Toolbar {
protected:
    SPDesktop *_desktop;

    /**
     * \brief A default constructor that just assigns the desktop
     */
    Toolbar(SPDesktop *desktop);
    Toolbar(SPDesktop* desktop, Inkscape::UI::InkIconSize icon_size);

    Gtk::ToolItem         * add_label(const Glib::ustring &label_text);
    Gtk::ToggleToolButton * add_toggle_button(const Glib::ustring &label_text,
                                              const Glib::ustring &tooltip_text);
    Gtk::ToolButton * add_toolbutton_for_verb(unsigned int verb_code);
    void add_separator();

    Gtk::ToggleToolButton* add_toggle_button(const Glib::ustring& label_text,
       const Glib::ustring& tooltip_text, const Glib::ustring& icon_name);//, Inkscape::UI::InkIconSize size);

    template<class BTN>
    BTN* add_button(const Glib::ustring& label_text, const Glib::ustring& tooltip_text, const Glib::ustring& icon_name) {
       Gtk::Widget* image = sp_get_sized_icon_image(icon_name, _icon_size);
       auto btn = Gtk::manage(image ? new BTN(*image, label_text) : new BTN(label_text));
       btn->set_tooltip_text(tooltip_text);
       add(*btn);
       return btn;
    }
protected:
    static GtkWidget * create(SPDesktop *desktop);
    Inkscape::UI::InkIconSize _icon_size;
};
}
}
}

#endif
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
