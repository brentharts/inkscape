// SPDX-License-Identifier: GPL-2.0-or-later
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

#include "arc-toolbar.h"

#include <glibmm.h>
#include <glibmm/i18n.h>


#include "desktop.h"
#include "document-undo.h"
#include "inkscape-application.h" // Until GTK4
#include "mod360.h"
#include "selection.h"
#include "verbs.h"

#include "object/sp-ellipse.h"
#include "object/sp-namedview.h"

#include "ui/icon-names.h"
#include "ui/tools/arc-tool.h"

#include "ui/widget/combobox-unit.h"
#include "ui/widget/spinbutton-action.h"
#include "ui/widget/toolitem-menu.h"

#include "xml/node-event-vector.h"

using Inkscape::DocumentUndo;
using Inkscape::Util::Unit;
using Inkscape::Util::Quantity;

static Inkscape::XML::NodeEventVector arc_tb_repr_events = {
    nullptr, /* child_added */
    nullptr, /* child_removed */
    Inkscape::UI::Toolbar::ArcToolbar::event_attr_changed,
    nullptr, /* content_changed */
    nullptr  /* order_changed */
};

namespace Inkscape {
namespace UI {
namespace Toolbar {

ArcToolbar::ArcToolbar()
    : Gtk::Toolbar()
    , Glib::ObjectBase("ArcToolbar")
{
}

ArcToolbar::ArcToolbar(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, SPDesktop* desktop)
    : Gtk::Toolbar(cobject)
    , Glib::ObjectBase("ArcToolbar")
    , _desktop(desktop) // To be replaced
{
    // Custom widgets need to be constructed via a call to get_widget_derived(). Quite annoying!
    // But we need references to some anyway so that we can enable/disable depending on selection.
    builder->get_widget(        "ToolbarArcLabel",              _label);
    builder->get_widget_derived("ToolbarArcRx",                 _spinbutton_rx);
    builder->get_widget_derived("ToolbarArcRy",                 _spinbutton_ry);
    builder->get_widget_derived("ToolbarArcUnits",              _combobox_unit);
    builder->get_widget_derived("ToolbarArcStart",              _spinbutton_start);
    builder->get_widget_derived("ToolbarArcEnd",                _spinbutton_end);
    Inkscape::UI::Widget::ToolItemMenu* dummy2 = nullptr;
    builder->get_widget_derived("ToolbarArcMenuRx",              dummy2);
    builder->get_widget_derived("ToolbarArcMenuRy",              dummy2);
    builder->get_widget_derived("ToolbarArcMenuUnits",           dummy2);
    builder->get_widget_derived("ToolbarArcMenuStart",           dummy2);
    builder->get_widget_derived("ToolbarArcMenuEnd",             dummy2);
    builder->get_widget_derived("ToolbarArcMenuArcTypeSlice",    dummy2);
    builder->get_widget_derived("ToolbarArcMenuArcTypeArc",      dummy2);
    builder->get_widget_derived("ToolbarArcMenuArcTypeChord",    dummy2);
    builder->get_widget_derived("ToolbarArcMenuSetWhole",        dummy2);
    builder->get_widget(        "ToolbarArcButtonArcTypeSlice", _togglebutton_slice);
    builder->get_widget(        "ToolbarArcButtonArcTypeArc",   _togglebutton_arc);
    builder->get_widget(        "ToolbarArcButtonArcTypeChord", _togglebutton_chord);
    builder->get_widget(        "ToolbarArcButtonSetWhole",     _button_set_whole);

    if (_combobox_unit) {
        Glib::ustring unit = _desktop->getNamedView()->getDisplayUnit()->abbr;
        _combobox_unit->set_unit(unit);
    }

    // Update on selection changes.
    _desktop->connectEventContextChanged(sigc::mem_fun(*this, &ArcToolbar::watch_ec));
}

ArcToolbar::~ArcToolbar()
{
    if (_repr) { // Remove old listener.
        _repr->removeListenerByData(this);
        Inkscape::GC::release(_repr);
        _repr = nullptr;
    }
}

void
ArcToolbar::watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec)
{
    if (dynamic_cast<Inkscape::UI::Tools::ArcTool *>(ec)) {
        Inkscape::Selection *selection = desktop->getSelection();
        _changed = selection->connectChanged(sigc::mem_fun(*this, &ArcToolbar::selection_changed));
        selection_changed(selection);
    } else {
        if (_changed) {
            _changed.disconnect();
            if(_repr) {
                _repr->removeListenerByData(this);
                Inkscape::GC::release(_repr);
                _repr = nullptr;
            }
        }
    }
}

void
ArcToolbar::sensitivize(double v1, double v2, const Glib::ustring& arc_type)
{
    // If start is zero and end is zero or 2PI, we have a circle or ellipse, don't enable buttons,
    // but only if we have just one object.
    bool enable = true;
    if (
        (_n_selected == 0                      ) ||
        (_n_selected == 1 && v1 == 0 && (v2 == 0 || v2 == 2.0 * M_PI))
        ) {
        enable = false;
    }

    if (_button_set_whole)       _button_set_whole->set_sensitive(enable);

    // We must enable/disable action to enable/disable buttons.
    auto app = InkscapeApplication::instance()->gtk_app();
    auto action = app->lookup_action("object-ellipse-arc-type");
    if (action) {
        auto saction = Glib::RefPtr<Gio::SimpleAction>::cast_dynamic(action);
        saction->change_state(arc_type);
        saction->set_enabled(enable);
    }
}

Glib::ustring get_arc_type(XML::Node *repr)
{
    char const *arctypestr = repr->attribute("sodipodi:arc-type");
    if (!arctypestr) { // For old files.
        char const *openstr = repr->attribute("sodipodi:open");
        arctypestr = (openstr ? "arc" : "slice");
    }
    Glib::ustring arc_type = "slice";
    if (arctypestr) {
        arc_type = arctypestr;
    }
    return arc_type;
}

void
ArcToolbar::selection_changed(Inkscape::Selection *selection)
{
    if (_freeze) {
        return;
    }

    _freeze = true;

    if ( _repr ) {
        _ellipse = nullptr;
        _repr->removeListenerByData(this);
        GC::release(_repr);
        _repr = nullptr;
    }

    SPGenericEllipse* last_ellipse = nullptr;
    _n_selected = 0;

    for (auto item : selection->items()) {
        auto ellipse = dynamic_cast<SPGenericEllipse*>(item);
        if (ellipse) {
            _n_selected++;
            last_ellipse = ellipse;
        }
    }

    double start = 0.0;
    double end = 0.0;
    Glib::ustring arc_type = "slice";
    if (_n_selected == 0) {
        if (_label) {
            _label->set_markup(_("<b>New:</b>"));
        }
        if (_spinbutton_rx) {
            _spinbutton_rx->set_sensitive(false);
        }
        if (_spinbutton_ry) {
            _spinbutton_ry->set_sensitive(false);
        }
    } else if (_n_selected == 1) {
        if (_label) {
            _label->set_markup(_("<b>Change:</b>"));
        }
        if (_spinbutton_rx) {
            _spinbutton_rx->set_sensitive(true);
        }
        if (_spinbutton_ry) {
            _spinbutton_ry->set_sensitive(true);
        }

        if (last_ellipse) {
            // Uses last ellipse in itemlist... but that's OK.
            _ellipse = last_ellipse;
            _repr = last_ellipse->getRepr();
            Inkscape::GC::anchor(_repr);
            _repr->addListener(&arc_tb_repr_events, this);
            _repr->synthesizeEvents(&arc_tb_repr_events, this);
            start = _ellipse->start;
            end   = _ellipse->end;
            arc_type = get_arc_type(_repr);
        }
    } else {
        // FIXME: implement averaging of all parameters for multiple selected
        if (_label) {
            _label->set_markup(_("<b>Change:</b>"));
        }
    }
    sensitivize(start, end, arc_type);
    _freeze = false;
}

void
ArcToolbar::event_attr_changed(Inkscape::XML::Node *repr, gchar const * /*name*/,
                               gchar const * /*old_value*/, gchar const * /*new_value*/,
                               bool /*is_interactive*/, gpointer data)
{
    auto toolbar = reinterpret_cast<ArcToolbar *>(data);

    // quit if run by the _changed callbacks
    if (toolbar->_freeze) {
        return;
    }

    // in turn, prevent callbacks from responding
    toolbar->_freeze = true;

    static Unit default_unit; // Default unit is "dimensionless" with value of 1, which is compatible with "px".
    Unit const *unit = &default_unit;
    if (toolbar->_combobox_unit) {
        unit = toolbar->_combobox_unit->get_unit();
    }

    double start = 0.0;
    double end = 0.0;
    if (toolbar->_ellipse) {

        double rx = toolbar->_ellipse->getVisibleRx();
        if (toolbar->_spinbutton_rx) {
            toolbar->_spinbutton_rx->set_value_gui(Quantity::convert(rx, "px", unit));
        }

        double ry = toolbar->_ellipse->getVisibleRy();
        if (toolbar->_spinbutton_ry) {
            toolbar->_spinbutton_ry->set_value_gui(Quantity::convert(ry, "px", unit));
        }

        start = repr->getAttributeDouble("sodipodi:start", 0.0);;
        if (toolbar->_spinbutton_start) {
            toolbar->_spinbutton_start->set_value_gui(start * 180.0 / M_PI);
        }

        end   = repr->getAttributeDouble("sodipodi:end",   0.0);
        if (toolbar->_spinbutton_end) {
            toolbar->_spinbutton_end->set_value_gui(end * 180.0 / M_PI);
        }
    }

    Glib::ustring arc_type = get_arc_type(repr);
    toolbar->sensitivize(start, end, arc_type);

    toolbar->_freeze = false;
}

}
}
}


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
