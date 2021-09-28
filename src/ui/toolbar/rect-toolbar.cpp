// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Rect aux toolbar
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

#include "rect-toolbar.h"

#include <glibmm/i18n.h>

#include <gtkmm/separatortoolitem.h>
#include <gtkmm/toolbutton.h>

#include "desktop.h"
#include "document-undo.h"
#include "selection.h"
#include "verbs.h"

#include "object/sp-namedview.h"
#include "object/sp-rect.h"

#include "io/resource.h"

#include "ui/icon-names.h"
#include "ui/tools/rect-tool.h"
#include "ui/uxmanager.h"

#include "ui/widget/canvas.h"
#include "ui/widget/combobox-unit.h"
#include "ui/widget/spinbutton-action.h"
#include "ui/widget/toolitem-menu.h"

#include "widgets/widget-sizes.h"

#include "xml/node-event-vector.h"

using Inkscape::UI::UXManager;
using Inkscape::DocumentUndo;
using Inkscape::Util::Unit;
using Inkscape::Util::Quantity;
using Inkscape::Util::unit_table;

static Inkscape::XML::NodeEventVector rect_tb_repr_events = {
    nullptr, /* child_added */
    nullptr, /* child_removed */
    Inkscape::UI::Toolbar::RectToolbar::event_attr_changed,
    nullptr, /* content_changed */
    nullptr  /* order_changed */
};

namespace Inkscape {
namespace UI {
namespace Toolbar {

RectToolbar::RectToolbar()
    : Gtk::Toolbar()
    , Glib::ObjectBase("RectToolbar")
{
}

RectToolbar::RectToolbar(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, SPDesktop* desktop)
    : Gtk::Toolbar(cobject)
    , Glib::ObjectBase("RectToolbar")
    , _desktop(desktop) // To be replaced
{
    // Custom widgets need to be constructed via a call to get_widget_derived(). Quite annoying!
    // But we need references to some anyway so that we can enable/disable depending on selection.
    builder->get_widget_derived("ToolbarRectangleWidth",            _spinbutton_width);
    builder->get_widget_derived("ToolbarRectangleHeight",           _spinbutton_height);
    builder->get_widget_derived("ToolbarRectangleRx",               _spinbutton_rx);
    builder->get_widget_derived("ToolbarRectangleRy",               _spinbutton_ry);
    Inkscape::UI::Widget::ToolItemMenu* dummy2 = nullptr;
    builder->get_widget_derived("ToolbarRectangleMenuWidth",        dummy2);
    builder->get_widget_derived("ToolbarRectangleMenuHeight",       dummy2);
    builder->get_widget_derived("ToolbarRectangleMenuRx",           dummy2);
    builder->get_widget_derived("ToolbarRectangleMenuRy",           dummy2);
    builder->get_widget_derived("ToolbarRectangleMenuUnits",        dummy2);
    builder->get_widget_derived("ToolbarRectangleMenuResetCorners", _toolitem_reset_corners);
    builder->get_widget_derived("ToolbarRectangleUnits",            _combobox_unit);

    builder->get_widget("ToobarRectLabel", _label);

    sensitivize();

    if (_combobox_unit) {
        Glib::ustring unit = _desktop->getNamedView()->getDisplayUnit()->abbr;
        _combobox_unit->set_unit(unit);
    }

    // Update on selection changes.
    _desktop->connectEventContextChanged(sigc::mem_fun(*this, &RectToolbar::watch_ec));
}

RectToolbar::~RectToolbar()
{
    if (_repr) { // Remove old listener.
        _repr->removeListenerByData(this);
        Inkscape::GC::release(_repr);
        _repr = nullptr;
    }
}

void
RectToolbar::sensitivize()
{
    if (_toolitem_reset_corners) {
        if (_n_selected == 0 ||
            (_n_selected == 1 &&
             (_spinbutton_rx && _spinbutton_rx->get_value() == 0) &&
             (_spinbutton_ry && _spinbutton_ry->get_value() == 0))) {
            _toolitem_reset_corners->set_sensitive(false);
        } else {
            _toolitem_reset_corners->set_sensitive(true);
        }
    }
}

void
RectToolbar::watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec)
{
    static sigc::connection changed;

    // use of dynamic_cast<> seems wrong here -- we just need to check the current tool

    if (dynamic_cast<Inkscape::UI::Tools::RectTool *>(ec)) {
        Inkscape::Selection *sel = desktop->getSelection();

        changed = sel->connectChanged(sigc::mem_fun(*this, &RectToolbar::selection_changed));

        // Synthesize an emission to trigger the update
        selection_changed(sel);
    } else {
        if (changed) {
            changed.disconnect();
        
            if (_repr) { // remove old listener
                _repr->removeListenerByData(this);
                Inkscape::GC::release(_repr);
                _repr = nullptr;
            }
        }
    }
}

/**
 *  \param selection should not be NULL.
 */
void
RectToolbar::selection_changed(Inkscape::Selection *selection)
{
    if (_repr) { // Remove old listener.
        _rect = nullptr;
        _repr->removeListenerByData(this);
        Inkscape::GC::release(_repr);
        _repr = nullptr;
    }

    SPRect* rect = nullptr;
    _n_selected = 0;
    auto itemlist= selection->items();
    for(auto item : itemlist) {
        rect = dynamic_cast<SPRect*>(item);
        if (rect) {
            _n_selected++;
        }
    }

    if (_n_selected == 0) {
        if (_label) {
            _label->set_markup(_("<b>New:</b>"));
        }
        if (_spinbutton_width) {
            _spinbutton_width->set_sensitive(false);
        }
        if (_spinbutton_height) {
            _spinbutton_height->set_sensitive(false);
        }
    } else if (_n_selected == 1) {
        if (_label) {
            _label->set_markup(_("<b>Change:</b>"));
        }
        if (_spinbutton_width) {
            _spinbutton_width->set_sensitive(true);
        }
        if (_spinbutton_height) {
            _spinbutton_height->set_sensitive(true);
        }

        if (rect) {
            // Uses last rect in itemlist... but that's OK.
            _rect = rect;
            _repr = rect->getRepr();
            Inkscape::GC::anchor(_repr);
            _repr->addListener(&rect_tb_repr_events, this);
            _repr->synthesizeEvents(&rect_tb_repr_events, this);
        }
    } else {
        // FIXME: implement averaging of all parameters for multiple selected
        if (_label) {
            _label->set_markup(_("<b>Change:</b>"));
        }
    }

    sensitivize();
}

// Update toolbar in response to changes to the selected rectangle from other events (e.g. XML editor).
void RectToolbar::event_attr_changed(Inkscape::XML::Node * /*repr*/, gchar const * /*name*/,
                                     gchar const * /*old_value*/, gchar const * /*new_value*/,
                                     bool /*is_interactive*/, gpointer data)
{
    auto toolbar = reinterpret_cast<RectToolbar*>(data);

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

    if (toolbar->_rect) {
        {
            double width = toolbar->_rect->getVisibleWidth();
            if (toolbar->_spinbutton_width) {
                toolbar->_spinbutton_width->set_value_gui(Quantity::convert(width, "px", unit));
            } else {
                std::cerr << "RectToolbar::event_attr_changed: no width spin button!" << std::endl;
            }
        }

        {
            double height = toolbar->_rect->getVisibleHeight();
            if (toolbar->_spinbutton_height) {
                toolbar->_spinbutton_height->set_value_gui(Quantity::convert(height, "px", unit));
            } else {
                std::cerr << "RectToolbar::event_attr_changed: no height spin button!" << std::endl;
            }
        }

        {
            double rx = toolbar->_rect->getVisibleRx();
            if (toolbar->_spinbutton_rx) {
                toolbar->_spinbutton_rx->set_value_gui(Quantity::convert(rx, "px", unit));
            } else {
                std::cerr << "RectToolbar::event_attr_changed: no rx spin button!" << std::endl;
            }
        }

        {
            double ry = toolbar->_rect->getVisibleRy();
            if (toolbar->_spinbutton_ry) {
                toolbar->_spinbutton_ry->set_value_gui(Quantity::convert(ry, "px", unit));
            } else {
                std::cerr << "RectToolbar::event_attr_changed: no ry spin button!" << std::endl;
            }
        }
    }

    toolbar->sensitivize();
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
