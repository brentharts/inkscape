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

#include "object/sp-namedview.h"
#include "object/sp-rect.h"

#include "ui/icon-names.h"
#include "ui/tools/rect-tool.h"
#include "ui/widget/canvas.h"
#include "ui/widget/combo-tool-item.h"
#include "ui/widget/label-tool-item.h"
#include "ui/widget/spinbutton.h"
#include "ui/widget/spin-button-tool-item.h"
#include "ui/widget/unit-tracker.h"

#include "widgets/widget-sizes.h"

using Inkscape::UI::Widget::UnitTracker;
using Inkscape::DocumentUndo;
using Inkscape::Util::Unit;
using Inkscape::Util::Quantity;
using Inkscape::Util::unit_table;

namespace Inkscape::UI::Toolbar {

RectToolbar::RectToolbar(SPDesktop *desktop)
    : Toolbar(desktop)
    , _tracker(new UnitTracker(Inkscape::Util::UNIT_TYPE_LINEAR))
    , _mode_item(Gtk::make_managed<UI::Widget::LabelToolItem>(_("<b>New:</b>")))
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // rx/ry units menu: create
    //tracker->addUnit( SP_UNIT_PERCENT, 0 );
    // fixme: add % meaning per cent of the width/height
    auto init_units = desktop->getNamedView()->display_units;
    _tracker->setActiveUnit(init_units);
    _mode_item->set_use_markup(true);

    /* W */
    {
        auto width_val = prefs->getDouble("/tools/shapes/rect/width", 0);
        width_val = Quantity::convert(width_val, "px", init_units);

        _width_adj = Gtk::Adjustment::create(width_val, 0, 1e6, SPIN_STEP, SPIN_PAGE_STEP);
        _width_item = Gtk::make_managed<UI::Widget::SpinButtonToolItem>("rect-width", _("W:"), _width_adj);
        _width_item->get_spin_button()->addUnitTracker(_tracker);
        _width_item->set_focus_widget(_desktop->canvas);
        _width_item->set_all_tooltip_text(_("Width of rectangle"));
    
        _width_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &RectToolbar::value_changed),
                                                              _width_adj,
                                                              "width",
                                                              &SPRect::setVisibleWidth));
        _tracker->addAdjustment(_width_adj->gobj());
        _width_item->set_sensitive(false);

        std::vector<double> values = {1, 2, 3, 5, 10, 20, 50, 100, 200, 500};
        _width_item->set_custom_numeric_menu_data(values);
    }

    /* H */
    {
        auto height_val = prefs->getDouble("/tools/shapes/rect/height", 0);
        height_val = Quantity::convert(height_val, "px", init_units);

        _height_adj = Gtk::Adjustment::create(height_val, 0, 1e6, SPIN_STEP, SPIN_PAGE_STEP);
        _height_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &RectToolbar::value_changed),
                                                               _height_adj,
                                                               "height",
                                                               &SPRect::setVisibleHeight));
        _tracker->addAdjustment(_height_adj->gobj());

        std::vector<double> values = { 1,  2,  3,  5, 10, 20, 50, 100, 200, 500};
        _height_item = Gtk::make_managed<UI::Widget::SpinButtonToolItem>("rect-height", _("H:"), _height_adj);
        _height_item->get_spin_button()->addUnitTracker(_tracker);
        _height_item->set_custom_numeric_menu_data(values);
        _height_item->set_all_tooltip_text(_("Height of rectangle"));
        _height_item->set_focus_widget(_desktop->canvas);
        _height_item->set_sensitive(false);
    }

    /* rx */
    {
        std::vector<Glib::ustring> labels = {_("not rounded"), "", "", "", "", "", "", "",  ""};
        std::vector<double>        values = {             0.5,  1,  2,  3,  5, 10, 20, 50, 100};
        auto rx_val = prefs->getDouble("/tools/shapes/rect/rx", 0);
        rx_val = Quantity::convert(rx_val, "px", init_units);

        _rx_adj = Gtk::Adjustment::create(rx_val, 0, 1e6, SPIN_STEP, SPIN_PAGE_STEP);
        _rx_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &RectToolbar::value_changed),
                                                           _rx_adj,
                                                           "rx",
                                                           &SPRect::setVisibleRx));
        _tracker->addAdjustment(_rx_adj->gobj());
        _rx_item = Gtk::make_managed<UI::Widget::SpinButtonToolItem>("rect-rx", _("Rx:"), _rx_adj);
        _rx_item->get_spin_button()->addUnitTracker(_tracker);
        _rx_item->set_all_tooltip_text(_("Horizontal radius of rounded corners"));
        _rx_item->set_focus_widget(_desktop->canvas);
        _rx_item->set_custom_numeric_menu_data(values, labels);
    }

    /* ry */
    {
        std::vector<Glib::ustring> labels = {_("not rounded"), "", "", "", "", "", "", "",  ""};
        std::vector<double>        values = {             0.5,  1,  2,  3,  5, 10, 20, 50, 100};
        auto ry_val = prefs->getDouble("/tools/shapes/rect/ry", 0);
        ry_val = Quantity::convert(ry_val, "px", init_units);

        _ry_adj = Gtk::Adjustment::create(ry_val, 0, 1e6, SPIN_STEP, SPIN_PAGE_STEP);
        _ry_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &RectToolbar::value_changed),
                                                           _ry_adj,
                                                           "ry",
                                                           &SPRect::setVisibleRy));
        _tracker->addAdjustment(_ry_adj->gobj());
        _ry_item = Gtk::make_managed<UI::Widget::SpinButtonToolItem>("rect-ry", _("Ry:"), _ry_adj);
        _ry_item->get_spin_button()->addUnitTracker(_tracker);
        _ry_item->set_all_tooltip_text(_("Vertical radius of rounded corners"));
        _ry_item->set_focus_widget(_desktop->canvas);
        _ry_item->set_custom_numeric_menu_data(values, labels);
    }

    // add the units menu
    auto unit_menu_ti = _tracker->create_tool_item(_("Units"), (""));

    /* Reset */
    {
        _not_rounded = Gtk::make_managed<Gtk::ToolButton>(_("Not rounded"));
        _not_rounded->set_tooltip_text(_("Make corners sharp"));
        _not_rounded->set_icon_name(INKSCAPE_ICON("rectangle-make-corners-sharp"));
        _not_rounded->signal_clicked().connect(sigc::mem_fun(*this, &RectToolbar::defaults));
        _not_rounded->set_sensitive(true);
    }

    add(*_mode_item);
    add(*_width_item);
    add(*_height_item);
    add(*_rx_item);
    add(*_ry_item);
    add(*unit_menu_ti);
    add(* Gtk::make_managed<Gtk::SeparatorToolItem>());
    add(*_not_rounded);
    show_all();
    
    sensitivize();

    _desktop->connectEventContextChanged(sigc::mem_fun(*this, &RectToolbar::watch_ec));
}

RectToolbar::~RectToolbar()
{
    if (_repr) { // remove old listener
        _repr->removeObserver(*this);
        Inkscape::GC::release(_repr);
        _repr = nullptr;
    }
    _changed.disconnect();
}

GtkWidget *
RectToolbar::create(SPDesktop *desktop)
{
    auto toolbar = new RectToolbar(desktop);
    return GTK_WIDGET(toolbar->gobj());
}

void
RectToolbar::value_changed(Glib::RefPtr<Gtk::Adjustment>&  adj,
                           gchar const                    *value_name,
                           void (SPRect::*setter)(gdouble))
{
    Unit const *unit = _tracker->getActiveUnit();
    g_return_if_fail(unit != nullptr);

    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble(Glib::ustring("/tools/shapes/rect/") + value_name,
            Quantity::convert(adj->get_value(), unit, "px"));
    }

    // quit if run by the attr_changed listener
    if (_freeze || _tracker->isUpdating()) {
        return;
    }

    // in turn, prevent listener from responding
    _freeze = true;

    bool modmade = false;
    Inkscape::Selection *selection = _desktop->getSelection();
    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        if (is<SPRect>(*i)) {
            if (adj->get_value() != 0) {
                (cast<SPRect>(*i)->*setter)(Quantity::convert(adj->get_value(), unit, "px"));
            } else {
                (*i)->removeAttribute(value_name);
            }
            modmade = true;
        }
    }

    sensitivize();

    if (modmade) {
        DocumentUndo::done(_desktop->getDocument(), _("Change rectangle"), INKSCAPE_ICON("draw-rectangle"));
    }

    _freeze = false;
}

void
RectToolbar::sensitivize()
{
    if (_rx_adj->get_value() == 0 && _ry_adj->get_value() == 0 && _single) { // only for a single selected rect (for now)
        _not_rounded->set_sensitive(false);
    } else {
        _not_rounded->set_sensitive(true);
    }
}

void
RectToolbar::defaults()
{
    _rx_adj->set_value(0.0);
    _ry_adj->set_value(0.0);

    sensitivize();
}

void
RectToolbar::watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec)
{
    // use of dynamic_cast<> seems wrong here -- we just need to check the current tool

    if (dynamic_cast<Inkscape::UI::Tools::RectTool *>(ec)) {
        Inkscape::Selection *sel = desktop->getSelection();

        _changed = sel->connectChanged(sigc::mem_fun(*this, &RectToolbar::selection_changed));

        // Synthesize an emission to trigger the update
        selection_changed(sel);
    } else {
        if (_changed) {
            _changed.disconnect();
        
            if (_repr) { // remove old listener
                _repr->removeObserver(*this);
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
    int n_selected = 0;
    Inkscape::XML::Node *repr = nullptr;
    SPItem *item = nullptr;

    if (_repr) { // remove old listener
        _item = nullptr;
        _repr->removeObserver(*this);
        Inkscape::GC::release(_repr);
        _repr = nullptr;
    }

    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        if (is<SPRect>(*i)) {
            n_selected++;
            item = *i;
            repr = item->getRepr();
        }
    }

    _single = false;

    if (n_selected == 0) {
        _mode_item->set_markup(_("<b>New:</b>"));
        _width_item->set_sensitive(false);
        _height_item->set_sensitive(false);
    } else if (n_selected == 1) {
        _mode_item->set_markup(_("<b>Change:</b>"));
        _single = true;
        _width_item->set_sensitive(true);
        _height_item->set_sensitive(true);

        if (repr) {
            _repr = repr;
            _item = item;
            Inkscape::GC::anchor(_repr);
            _repr->addObserver(*this);
            _repr->synthesizeEvents(*this);
        }
    } else {
        // FIXME: implement averaging of all parameters for multiple selected
        //gtk_label_set_markup(GTK_LABEL(l), _("<b>Average:</b>"));
        _mode_item->set_markup(_("<b>Change:</b>"));
        sensitivize();
    }
}

void RectToolbar::notifyAttributeChanged(Inkscape::XML::Node &repr, GQuark,
                                         Inkscape::Util::ptr_shared,
                                         Inkscape::Util::ptr_shared)
{
    // quit if run by the _changed callbacks
    if (_freeze) {
        return;
    }

    // in turn, prevent callbacks from responding
    _freeze = true;

    auto unit = _tracker->getActiveUnit();
    if (!unit) {
        return;
    }

    if (auto rect = cast<SPRect>(_item)) {
        _rx_adj    ->set_value(Quantity::convert(rect->getVisibleRx(),     "px", unit));
        _ry_adj    ->set_value(Quantity::convert(rect->getVisibleRy(),     "px", unit));
        _width_adj ->set_value(Quantity::convert(rect->getVisibleWidth(),  "px", unit));
        _height_adj->set_value(Quantity::convert(rect->getVisibleHeight(), "px", unit));
    }

    sensitivize();
    _freeze = false;
}

} // namespace Inkscape::UI::Toolbar

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
