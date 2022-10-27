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
 * c++ version based on former C-version (GPL v2+) with authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <goejendaagh@zonnet.nl>
 *   Abhishek Sharma
 */

#include "object-properties.h"

#include <glibmm/i18n.h>

#include "document-undo.h"
#include "document.h"
#include "inkscape.h"
#include "style.h"
#include "style-enums.h"

#include "object/sp-image.h"
#include "ui/builder-utils.h"
#include "ui/icon-names.h"
#include "ui/widget/color-picker.h"
#include "widgets/sp-attribute-widget.h"

using Inkscape::UI::get_widget;
using Inkscape::UI::get_derived_widget;

namespace Inkscape {
namespace UI {
namespace Dialog {

ObjectProperties::ObjectProperties()
        : DialogBase("/dialogs/object/", "ObjectProperties")
        , _builder(create_builder("object-properties.glade"))
        , _blocked(false)
        , _attr_table(Gtk::manage(new SPAttributeTable()))
        , _grid_top                 (get_widget<Gtk::Grid>(_builder, "grid-top"))
        , _grid_bottom              (get_widget<Gtk::Grid>(_builder, "grid-bottom"))
        , _grid_interactivity       (get_widget<Gtk::Grid>(_builder, "grid-interactivity"))
        , _grid_main                (get_widget<Gtk::Grid>(_builder, "grid-main"))
        , _entry_id                 (get_widget<Gtk::Entry>(_builder, "entry-id"))
        , _entry_label              (get_widget<Gtk::Entry>(_builder, "entry-label"))
        , _entry_title              (get_widget<Gtk::Entry>(_builder, "entry-title"))
        , _combo_image_rendering    (get_widget<Gtk::ComboBoxText>(_builder, "combo-image-rendering"))
        , _checkbox_hide            (get_widget<Gtk::CheckButton>(_builder, "checkbox-hide"))
        , _checkbox_lock            (get_widget<Gtk::CheckButton>(_builder, "checkbox-lock"))
        , _checkbox_preserve_ratio  (get_widget<Gtk::CheckButton>(_builder, "checkbox-preserve-ratio"))
        , _button_set               (get_widget<Gtk::Button>(_builder, "button-set"))
        , _expander_interactivity   (get_widget<Gtk::Expander>(_builder, "expander-interactivity"))
        , _entry_onclick            (get_widget<Gtk::Entry>(_builder, "entry-onclick"))
        , _entry_onmouseover        (get_widget<Gtk::Entry>(_builder, "entry-onmouseover"))
        , _entry_onmouseout         (get_widget<Gtk::Entry>(_builder, "entry-onmouseout"))
        , _entry_onmousedown        (get_widget<Gtk::Entry>(_builder, "entry-onmousedown"))
        , _entry_onmouseup          (get_widget<Gtk::Entry>(_builder, "entry-onmouseup"))
        , _entry_onmousemove        (get_widget<Gtk::Entry>(_builder, "entry-onmousemove"))
        , _entry_onfocusin          (get_widget<Gtk::Entry>(_builder, "entry-onfocusin"))
        , _entry_onfocusout         (get_widget<Gtk::Entry>(_builder, "entry-onfocusout"))
        , _entry_onload             (get_widget<Gtk::Entry>(_builder, "entry-onload"))
        , _label_id                 (get_widget<Gtk::Label>(_builder, "label-id"))
        , _label_dpi                (get_widget<Gtk::Label>(_builder, "label-dpi"))
        , _label_image_rendering    (get_widget<Gtk::Label>(_builder, "label-image-rendering"))
        , _adjustment_spin_dpi      (get_object<Gtk::Adjustment>(_builder, "adjustment-spin-dpi"))
        , _textview_description     (get_derived_widget<ScrollProtected<Gtk::TextView>>(_builder, "textview-description"))
        , _spin_dpi                 (get_derived_widget<SpinButton>(_builder, "spin-dpi"))
{
    // initialize labels for the table at the bottom of the dialog
    _int_attrs.emplace_back("onclick");
    _int_attrs.emplace_back("onmouseover");
    _int_attrs.emplace_back("onmouseout");
    _int_attrs.emplace_back("onmousedown");
    _int_attrs.emplace_back("onmouseup");
    _int_attrs.emplace_back("onmousemove");
    _int_attrs.emplace_back("onfocusin");
    _int_attrs.emplace_back("onfocusout");
    _int_attrs.emplace_back("onload");

    _int_labels.emplace_back("onclick:");
    _int_labels.emplace_back("onmouseover:");
    _int_labels.emplace_back("onmouseout:");
    _int_labels.emplace_back("onmousedown:");
    _int_labels.emplace_back("onmouseup:");
    _int_labels.emplace_back("onmousemove:");
    _int_labels.emplace_back("onfocusin:");
    _int_labels.emplace_back("onfocusout:");
    _int_labels.emplace_back("onload:");

    _picker_highlight_color = std::make_unique<Inkscape::UI::Widget::ColorPicker>(
            _("Highlight Color"), "", 0xff0000ff, true,
            &get_widget<Gtk::Button>(_builder, "highlight-color"));

    _spin_dpi.set_adjustment(_adjustment_spin_dpi);

    // connecting signals
    _entry_id.signal_activate().connect(sigc::mem_fun(*this, &ObjectProperties::_idChanged));
    _entry_title.signal_activate().connect(sigc::mem_fun(*this, &ObjectProperties::_titleChanged));
    _picker_highlight_color->connectChanged(sigc::mem_fun(*this, &ObjectProperties::_highlightChanged));
    _spin_dpi.signal_activate().connect(sigc::mem_fun(*this, &ObjectProperties::_dpiChanged));
    _combo_image_rendering.signal_changed().connect(sigc::mem_fun(*this, &ObjectProperties::_imageRenderingChanged));
    _checkbox_hide.signal_toggled().connect(sigc::mem_fun(*this, &ObjectProperties::_hiddenToggled));
    _checkbox_lock.signal_toggled().connect(sigc::mem_fun(*this, &ObjectProperties::_sensitivityToggled));
    _checkbox_preserve_ratio.signal_toggled().connect(sigc::mem_fun(*this, &ObjectProperties::_aspectRatioToggled));
    _button_set.signal_clicked().connect(sigc::mem_fun(*this, &ObjectProperties::_setButtonCallback));

    add(_grid_main);
    show();
}

void ObjectProperties::update_entries()
{
    if (_blocked || !getDesktop()) {
        return;
    }

    auto selection = getSelection();
    if (!selection) return;

    if (!selection->singleItem()) {
        set_sensitive (false);
        _current_item = nullptr;
        //no selection anymore or multiple objects selected, means that we need
        //to close the connections to the previously selected object
        _attr_table->clear();
        _picker_highlight_color->setRgba32(0x0);
        return;
    } else {
        set_sensitive (true);
    }

    SPItem *item = selection->singleItem();
    if (_current_item == item)
    {
        //otherwise we would end up wasting resources through the modify selection
        //callback when moving an object (endlessly setting the labels and recreating _attr_table)
        return;
    }
    _blocked = true;
    _checkbox_preserve_ratio.set_active(g_strcmp0(item->getAttribute("preserveAspectRatio"), "none") != 0);
    _checkbox_lock.set_active(item->isLocked());           /* Sensitive */
    _checkbox_hide.set_active(item->isExplicitlyHidden()); /* Hidden */
    _picker_highlight_color->setRgba32(item->highlight_color());
    _picker_highlight_color->closeWindow();

    if (item->cloned) {
        /* ID */
        _entry_id.set_text("");
        _entry_id.set_sensitive(FALSE);

        /* Label */
        _entry_label.set_text("");
        _entry_label.set_sensitive(FALSE);

    } else {
        SPObject *obj = static_cast<SPObject*>(item);

        /* ID */
        _entry_id.set_text(obj->getId() ? obj->getId() : "");
        _entry_id.set_sensitive(TRUE);

        /* Label */
        _entry_label.set_text(obj->defaultLabel());
        _entry_label.set_placeholder_text("");
        _entry_label.set_sensitive(TRUE);

        /* Title */
        gchar *title = obj->title();
        if (title) {
            _entry_title.set_text(title);
            g_free(title);
        }
        else {
            _entry_title.set_text("");
        }
        _entry_title.set_sensitive(TRUE);

        /* Image Rendering */
        if (is<SPImage>(item)) {
            _combo_image_rendering.show();
            _combo_image_rendering.set_active(obj->style->image_rendering.value);
            _label_image_rendering.show();
            if (obj->getAttribute("inkscape:svg-dpi")) {
                _spin_dpi.set_value(std::stod(obj->getAttribute("inkscape:svg-dpi")));
                _spin_dpi.show();
                _label_dpi.show();
            } else {
                _spin_dpi.hide();
                _label_dpi.hide();
            }
        } else {
            _combo_image_rendering.hide();
            _combo_image_rendering.unset_active();
            _label_image_rendering.hide();
            _spin_dpi.hide();
            _label_dpi.hide();
        }

        /* Description */
        gchar *desc = obj->desc();
        if (desc) {
            _textview_description.get_buffer()->set_text(desc);
            g_free(desc);
        } else {
            _textview_description.get_buffer()->set_text("");
        }

        if (_current_item == nullptr) {
            _attr_table->set_object(obj, _int_labels, _int_attrs, (GtkWidget*) _expander_interactivity.gobj());
        } else {
            _attr_table->change_object(obj);
        }
        _attr_table->show_all();
    }
    _current_item = item;
    _blocked = false;
}

// update all fields
void ObjectProperties::_setButtonCallback()
{
    _titleChanged();
    _idChanged();
    _imageRenderingChanged();
    _dpiChanged();
    _descriptionChanged();
}

void ObjectProperties::_titleChanged()
{
    if (_blocked)
        return;

    _blocked = true;

    if (auto obj = static_cast<SPObject *>(getSelection()->singleItem())) {
        if (obj->setTitle(_entry_title.get_text().c_str())) {
            DocumentUndo::done(getDocument(), _("Set object title"), INKSCAPE_ICON("dialog-object-properties"));
        }
    }

    _blocked = false;
}

// the ID and the label are tied together
// the label for the ID entry updates based on the validity of the entered ID
void ObjectProperties::_idChanged()
{
    if (_blocked) {
        return;
    }

    SPItem *item = getSelection()->singleItem();
    g_return_if_fail (item != nullptr);

    _blocked = true;

    /* Retrieve the label widget for the object's id */
    gchar *id = g_strdup(_entry_id.get_text().c_str());
    g_strcanon(id, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.:", '_');
    if (g_strcmp0(id, item->getId()) == 0) {
        _label_id.set_markup_with_mnemonic(_("_ID:") + Glib::ustring(" "));
    } else if (!*id || !isalnum (*id)) {
        _label_id.set_text(_("Id invalid! "));
    } else if (getDocument()->getObjectById(id) != nullptr) {
        _label_id.set_text(_("Id exists! "));
    } else {
        _label_id.set_markup_with_mnemonic(_("_ID:") + Glib::ustring(" "));
        item->setAttribute("id", id);
        DocumentUndo::done(getDocument(), _("Set object ID"), INKSCAPE_ICON("dialog-object-properties"));
    }
    g_free(id);

    /* Retrieve the label widget for the object's label */
    Glib::ustring label = _entry_label.get_text();

    /* Give feedback on success of setting the drawing object's label
     * using the widget's label text
     */
    SPObject *obj = static_cast<SPObject*>(item);
    auto currentlabel = obj->label();
    if (label.compare(currentlabel ? currentlabel : "")) {
        obj->setLabel(label.c_str());
        DocumentUndo::done(getDocument(), _("Set object label"), INKSCAPE_ICON("dialog-object-properties"));
    }


    _blocked = false;
}

void ObjectProperties::_descriptionChanged()
{
    if (_blocked)
        return;

    if (auto obj = static_cast<SPObject *>(getSelection()->singleItem())) {
        Gtk::TextBuffer::iterator start, end;
        _textview_description.get_buffer()->get_bounds(start, end);
        auto desc = _textview_description.get_buffer()->get_text(start, end, TRUE);
        if (obj->setDesc(desc.c_str())) {
            DocumentUndo::done(getDocument(), _("Set object description"), INKSCAPE_ICON("dialog-object-properties"));
        }
    }
}

void ObjectProperties::_highlightChanged(guint rgba)
{
    if (_blocked)
        return;
    
    if (auto item = getSelection()->singleItem()) {
        item->setHighlight(rgba);
        DocumentUndo::done(getDocument(), _("Set item highlight color"), INKSCAPE_ICON("dialog-object-properties"));
    }
}

void ObjectProperties::_dpiChanged()
{
    if (_blocked)
        return;

    _blocked = true;

    if (auto obj = static_cast<SPObject *>(getSelection()->singleItem())) {
        if (is<SPImage>(obj)) {
            auto dpi_value = Glib::ustring::format(_spin_dpi.get_value());
            g_warning("%s", dpi_value.c_str());
            obj->setAttribute("inkscape:svg-dpi", dpi_value);
            DocumentUndo::done(getDocument(), _("Set image DPI"), INKSCAPE_ICON("dialog-object-properties"));
        }
    }

    _blocked = false;

}

void ObjectProperties::_imageRenderingChanged()
{
    if (_blocked) {
        return;
    }

    SPItem *item = getSelection()->singleItem();
    g_return_if_fail (item != nullptr);

    _blocked = true;

    Glib::ustring scale = _combo_image_rendering.get_active_text();

    // We should unset if the parent computed value is auto and the desired value is auto.
    SPCSSAttr *css = sp_repr_css_attr_new();
    sp_repr_css_set_property(css, "image-rendering", scale.c_str());
    Inkscape::XML::Node *image_node = item->getRepr();
    if (image_node) {
        sp_repr_css_change(image_node, css, "style");
        DocumentUndo::done(getDocument(), _("Set image rendering option"), INKSCAPE_ICON("dialog-object-properties"));
    }
    sp_repr_css_attr_unref(css);

    _blocked = false;
}

void ObjectProperties::_sensitivityToggled()
{
    if (_blocked) {
        return;
    }

    SPItem *item = getSelection()->singleItem();
    g_return_if_fail(item != nullptr);

    _blocked = true;
    item->setLocked(_checkbox_lock.get_active());
    DocumentUndo::done(getDocument(), _checkbox_lock.get_active() ? _("Lock object") : _("Unlock object"), INKSCAPE_ICON("dialog-object-properties"));
    _blocked = false;
}

void ObjectProperties::_aspectRatioToggled()
{
    if (_blocked) {
        return;
    }

    SPItem *item = getSelection()->singleItem();
    g_return_if_fail(item != nullptr);

    _blocked = true;

    const char *active;
    if (_checkbox_preserve_ratio.get_active()) {
        active = "xMidYMid";
    }
    else {
        active = "none";
    }
    /* Retrieve the DPI */
    if (is<SPImage>(item)) {
        Glib::ustring dpi_value = Glib::ustring::format(_spin_dpi.get_value());
        item->setAttribute("preserveAspectRatio", active);
        DocumentUndo::done(getDocument(), _("Set preserve ratio"), INKSCAPE_ICON("dialog-object-properties"));
    }
    _blocked = false;
}

void ObjectProperties::_hiddenToggled()
{
    if (_blocked) {
        return;
    }

    SPItem *item = getSelection()->singleItem();
    g_return_if_fail(item != nullptr);

    _blocked = true;
    item->setExplicitlyHidden(_checkbox_hide.get_active());
    DocumentUndo::done(getDocument(), _checkbox_hide.get_active() ? _("Hide object") : _("Unhide object"), INKSCAPE_ICON("dialog-object-properties"));
    _blocked = false;
}

void ObjectProperties::selectionChanged(Selection *selection)
{
    update_entries();
}

void ObjectProperties::desktopReplaced()
{
    update_entries();
}

} // Dialog
} // UI
} // Inkscape

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
