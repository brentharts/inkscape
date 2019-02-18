// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright  (C) 2017 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


/** \file
    An action (menu/toolbar item) that allows selecting one choice out of many.

    The choices may be displayed as:

    1. A group of items in a toolbar with labels and/or icons.
    2. As a drop-down menu with a labels and/or icons.
*/

#include "ink-select-one-action.h"

#include <iostream>
#include <utility>
#include <gtkmm/toolitem.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/radioaction.h>
#include <gtkmm/radiomenuitem.h>
#include <gtkmm/combobox.h>
#include <gtkmm/menu.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/image.h>

InkSelectOneAction* InkSelectOneAction::create(const Glib::ustring &name,
                                               const Glib::ustring &group_label,
                                               const Glib::ustring &tooltip,
                                               const Glib::ustring &stock_id,
                                               Glib::RefPtr<Gtk::ListStore> store) {

    return new InkSelectOneAction(name, group_label, tooltip, stock_id, store);
}

InkSelectOneAction::InkSelectOneAction (const Glib::ustring &name,
                                        const Glib::ustring &group_label,
                                        const Glib::ustring &tooltip,
                                        const Glib::ustring &stock_id,
                                        Glib::RefPtr<Gtk::ListStore> store) :
    Gtk::Action(name, stock_id, group_label, tooltip),
    _name(name),
    _group_label(group_label),
    _tooltip(tooltip),
    _stock_id(stock_id),
    _store (std::move(store)),
    _use_radio (true),
    _use_label (true),
    _use_icon  (true),
    _use_pixbuf (false),
    _icon_size (Gtk::ICON_SIZE_LARGE_TOOLBAR),
    _combobox (nullptr),
    _radioaction (nullptr),
    _menuitem (nullptr)
{
}

void InkSelectOneAction::set_active (gint active) {

    if (active < 0) {
        std::cerr << "InkSelectOneAction::set_active: active < 0: " << active << std::endl;
        return;
    }

    if (_active != active) {

        _active = active;

        if (_combobox) {
            _combobox->set_active (active);
        }

        if (_radioaction) {
            _radioaction->set_current_value (active);
        }

        if (active < _radiomenuitems.size()) {
            _radiomenuitems[ active ]->set_active();
        }
    }
}

Glib::ustring InkSelectOneAction::get_active_text () {
    Gtk::TreeModel::Row row = _store->children()[_active];
    InkSelectOneActionColumns columns;
    Glib::ustring label = row[columns.col_label];
    return label;
}

Gtk::Widget* InkSelectOneAction::create_menu_item_vfunc() {

    if (_menuitem == nullptr) {

        _menuitem = Gtk::manage (new Gtk::MenuItem);
        Gtk::Menu *menu = Gtk::manage (new Gtk::Menu);

        Gtk::RadioButton::Group group;
        int index = 0;
        auto children = _store->children();
        for (auto row : children) {
            InkSelectOneActionColumns columns;
            Glib::ustring label     = row[columns.col_label     ];
            Glib::ustring icon      = row[columns.col_icon      ];
            Glib::ustring tooltip   = row[columns.col_tooltip   ];
            bool          sensitive = row[columns.col_sensitive ];

            Gtk::RadioMenuItem* button = Gtk::manage(new Gtk::RadioMenuItem(group));
            button->set_label (label);
            button->set_tooltip_text(tooltip);
            button->set_sensitive(sensitive);

            button->signal_toggled().connect(sigc::bind<0>(
              sigc::mem_fun(*this, &InkSelectOneAction::on_toggled_radiomenu), index++)
);

            menu->add (*button);

            _radiomenuitems.push_back(button);
        }

        if (_active < _radiomenuitems.size()) {
            _radiomenuitems[ _active ]->set_active();
        }
   
        _menuitem->set_submenu (*menu);
        _menuitem->show_all();
    }

    return _menuitem;
}


Gtk::Widget* InkSelectOneAction::create_tool_item_vfunc() {
    // Either a group of radio actions or a combobox with labels and/or icons.

    Gtk::ToolItem *tool_item = new Gtk::ToolItem;

    Gtk::Box* box = Gtk::manage(new Gtk::Box());
    tool_item->add (*box);

    if (_use_group_label) {
        Gtk::Label *group_label = Gtk::manage (new Gtk::Label(_group_label + ": "));
        box->add(*group_label);
    }

    if (_use_radio) {
        // Create radio actions (note: these are not radio buttons).

        Gtk::RadioAction::Group group;
        int index = 0;
        auto children = _store->children();
        for (auto row : children) {
            InkSelectOneActionColumns columns;
            Glib::ustring label     = row[columns.col_label    ];
            Glib::ustring icon      = row[columns.col_icon     ];
            Glib::ustring tooltip   = row[columns.col_tooltip  ];
            bool          sensitive = row[columns.col_sensitive];
            Glib::RefPtr<Gtk::RadioAction> action;
            if (_use_icon) {
                action =
                    Gtk::RadioAction::create_with_icon_name (group, "Anonymous", icon, label, tooltip);
            } else {
                action =
                    Gtk::RadioAction::create (group, "Anonymous", label, tooltip);
            }
            action->set_property("value", index++); // To identify uniquely each radioaction.
            action->set_sensitive(sensitive);

            // Save first action for use in setting/getting active value.
            if (!_radioaction) {
                _radioaction = action;
            }

            Gtk::ToolItem* item = action->create_tool_item(); 
            box->add (*item);
        }

        if (_radioaction) {
            _radioaction->set_current_value (_active);
        }

        _radioaction->signal_changed().connect(sigc::mem_fun(*this, &InkSelectOneAction::on_changed_radioaction));

    } else {
        // Create combobox

        _combobox = Gtk::manage (new Gtk::ComboBox());
        _combobox->set_model(_store);

        InkSelectOneActionColumns columns;
        if (_use_icon) {
            Gtk::CellRendererPixbuf *renderer = new Gtk::CellRendererPixbuf;
            renderer->set_property ("stock_size", Gtk::ICON_SIZE_LARGE_TOOLBAR);
            _combobox->pack_start (*renderer, false);
            _combobox->add_attribute (*renderer, "icon_name", columns.col_icon);
        } else if (_use_pixbuf) {
            Gtk::CellRendererPixbuf *renderer = new Gtk::CellRendererPixbuf;
            //renderer->set_property ("stock_size", Gtk::ICON_SIZE_LARGE_TOOLBAR);
            _combobox->pack_start (*renderer, false);
            _combobox->add_attribute (*renderer, "pixbuf", columns.col_pixbuf);
        }
  
        if (_use_label) {
            _combobox->pack_start(columns.col_label);
        }

        std::vector<Gtk::CellRenderer*> cells = _combobox->get_cells();
        for (auto & cell : cells) {
            _combobox->add_attribute (*cell, "sensitive", columns.col_sensitive);
        }

        _combobox->set_active (_active);

        _combobox->signal_changed().connect(
            sigc::mem_fun(*this, &InkSelectOneAction::on_changed_combobox));

        box->add (*_combobox);
    }

    tool_item->show_all();

    return tool_item;
}

void InkSelectOneAction::on_changed_combobox() {

    int row = _combobox->get_active_row_number();
    if (row < 0) row = 0;  // Happens when Gtk::ListStore reconstructed
    set_active(row);
    _changed.emit (_active);
    _changed_after.emit (_active);
}

void InkSelectOneAction::on_changed_radioaction(const Glib::RefPtr<Gtk::RadioAction>& current) {

    set_active(current->get_current_value());
    _changed.emit (_active);
    _changed_after.emit (_active);
}

void InkSelectOneAction::on_toggled_radiomenu(int n) {

    // toggled emitted twice, first for button toggled off, second for button toggled on.
    // We want to react only to the button turned on.
    if (n < _radiomenuitems.size() &&_radiomenuitems[ n ]->get_active()) {
        set_active (n);
        _changed.emit (_active);
        _changed_after.emit (_active);
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
