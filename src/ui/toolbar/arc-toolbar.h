// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_ARC_TOOLBAR_H
#define SEEN_ARC_TOOLBAR_H

/**
 * @file
 * Arc aux toolbar
 */
/* Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Frank Felfe <innerspace@iname.com>
 *   John Cliff <simarilius@yahoo.com>
 *   David Turner <novalis@gnu.org>
 *   Josh Andler <scislac@scislac.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2003 MenTaLguY
 * Copyright (C) 1999-2011 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "toolbar.h"

#include <gtkmm/builder.h>

namespace Gtk {
class Button;
class ToggleButton;
}

class SPDesktop;
class SPGenericEllipse;

namespace Inkscape {
class Selection;

namespace XML {
class Node;
}

namespace UI {
namespace Tools {
class ToolBase;
}

namespace Widget {
class SpinButtonAction;
class ToolItemMenu;
class ComboBoxUnit;
}

namespace Toolbar {
class ArcToolbar : public Gtk::Toolbar {

public:

    ArcToolbar(); // Dummy for declaring type.
    ArcToolbar(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refUI, SPDesktop* desktop);
    ~ArcToolbar() override;

    static void event_attr_changed(Inkscape::XML::Node *repr,
                                   gchar const         *name,
                                   gchar const         *old_value,
                                   gchar const         *new_value,
                                   bool                 is_interactive,
                                   gpointer             data);

private:

    // Functions
    void sensitivize( double v1, double v2, const Glib::ustring& arc_type);
    void watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec);
    void selection_changed(Inkscape::Selection *selection);

    // Widgets
    Gtk::Label                   *_label = nullptr;
    UI::Widget::SpinButtonAction *_spinbutton_rx = nullptr;
    UI::Widget::SpinButtonAction *_spinbutton_ry = nullptr;
    UI::Widget::ComboBoxUnit     *_combobox_unit = nullptr;
    UI::Widget::SpinButtonAction *_spinbutton_start = nullptr;
    UI::Widget::SpinButtonAction *_spinbutton_end = nullptr;
    UI::Widget::ToolItemMenu     *_toolitem_reset_corners = nullptr;
    Gtk::ToggleButton            *_togglebutton_slice = nullptr;
    Gtk::ToggleButton            *_togglebutton_arc = nullptr;
    Gtk::ToggleButton            *_togglebutton_chord = nullptr;
    Gtk::Button                  *_button_set_whole = nullptr;

    // Variables
    XML::Node *_repr = nullptr;
    SPGenericEllipse *_ellipse = nullptr;
    SPDesktop *_desktop = nullptr;
    bool _freeze = false;
    int _n_selected = 0;



    sigc::connection _changed;

};

}
}
}

#endif /* !SEEN_ARC_TOOLBAR_H */

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
