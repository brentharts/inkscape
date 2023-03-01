// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file Object properties dialog.
 */
/*
 * Inkscape, an Open Source vector graphics editor
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Copyright (C) 2012 Kris De Gussem <Kris.DeGussem@gmail.com>
 * c++version based on former C-version (GPL v2+) with authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <goejendaagh@zonnet.nl>
 *   Abhishek Sharma
 */

#ifndef SEEN_DIALOGS_ITEM_PROPERTIES_H
#define SEEN_DIALOGS_ITEM_PROPERTIES_H

#include <gtkmm/comboboxtext.h>
#include <gtkmm/entry.h>
#include <gtkmm/expander.h>
#include <gtkmm/frame.h>
#include <gtkmm/grid.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/textview.h>

#include "ui/dialog/dialog-base.h"
#include "ui/widget/color-picker.h"
#include "ui/widget/scrollprotected.h"
#include "ui/widget/spinbutton.h"

class SPAttributeTable;
class SPItem;

using Inkscape::UI::Widget::ScrollProtected;
using Inkscape::UI::Widget::SpinButton;
using Inkscape::UI::Widget::ColorPicker;

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * A dialog widget to show object properties.
 *
 * A widget to enter an ID, label, title and description for an object.
 * In addition it allows to edit the properties of an object.
 */
class ObjectProperties : public DialogBase
{
public:
    ObjectProperties();
    ~ObjectProperties() override = default;

    /// Updates entries and other child widgets on selection change, object modification, etc.
    void update_entries();
    void selectionChanged(Selection *selection) override;

private:

    bool _blocked;
    SPItem *_current_item = nullptr; //to store the current item, for not wasting resources
    std::vector<Glib::ustring> _int_attrs;
    std::vector<Glib::ustring> _int_labels;

    Glib::RefPtr<Gtk::Builder> _builder; // Builder for glade widgets, declare before widgets

    Gtk::Expander &_expander_interactivity;

    Gtk::Grid &_grid_main;
    Gtk::Grid &_grid_top;
    Gtk::Grid &_grid_bottom;
    Gtk::Grid &_grid_interactivity;

    Gtk::Label &_label_id;
    Gtk::Entry &_entry_id;
    Gtk::Entry &_entry_label;
    Gtk::Entry &_entry_title;

    Gtk::Entry &_entry_onclick;
    Gtk::Entry &_entry_onmouseover;
    Gtk::Entry &_entry_onmouseout;
    Gtk::Entry &_entry_onmousedown;
    Gtk::Entry &_entry_onmouseup;
    Gtk::Entry &_entry_onmousemove;
    Gtk::Entry &_entry_onfocusin;
    Gtk::Entry &_entry_onfocusout;
    Gtk::Entry &_entry_onload;

    std::unique_ptr<ColorPicker> _picker_highlight_color;
    ScrollProtected<Gtk::TextView> &_textview_description;

    Gtk::ComboBoxText &_combo_image_rendering;
    Gtk::Label &_label_image_rendering;

    Gtk::CheckButton &_checkbox_hide;
    Gtk::CheckButton &_checkbox_lock;
    Gtk::CheckButton &_checkbox_preserve_ratio;

    SpinButton &_spin_dpi;
    Gtk::Label &_label_dpi;
    Glib::RefPtr<Gtk::Adjustment> _adjustment_spin_dpi;

    Gtk::Button &_button_set;

    SPAttributeTable *_attr_table; //the widget for showing the on... names at the bottom

    void _setButtonCallback();

    /// Sets object properties ID on user input.
    void _idChanged();

    /// Callback for object's title
    void _titleChanged();

    /// Callback for object's description
    void _descriptionChanged();

    // Callback for highlight color
    void _highlightChanged(guint rgba);

    /// Callback for 'image-rendering'.
    void _imageRenderingChanged();

    /// Callback for DPI spin button
    void _dpiChanged();

    /// Callback for checkbox Lock.
    void _sensitivityToggled();

    /// Callback for checkbox Hide.
    void _hiddenToggled();

    /// Callback for checkbox Preserve Aspect Ratio.
    void _aspectRatioToggled();

    void desktopReplaced() override;
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
