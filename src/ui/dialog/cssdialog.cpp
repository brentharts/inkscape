// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief A dialog for CSS selectors
 */
/* Authors:
 *   Kamalpreet Kaur Grewal
 *   Tavmjong Bah
 *
 * Copyright (C) Kamalpreet Kaur Grewal 2016 <grewalkamal005@gmail.com>
 * Copyright (C) Tavmjong Bah 2017 <tavmjong@free.fr>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "cssdialog.h"

#include "message-context.h"
#include "message-stack.h"
#include "selection.h"
#include "style-internal.h"
#include "style.h"

#include "ui/icon-loader.h"
#include "ui/widget/iconrenderer.h"
#include "verbs.h"

#include "xml/attribute-record.h"
#include "xml/node-event-vector.h"
#include <glibmm/i18n.h>

#include <glibmm/i18n.h>

static void on_attr_changed(Inkscape::XML::Node *repr, const gchar *name, const gchar * /*old_value*/,
                            const gchar *new_value, bool /*is_interactive*/, gpointer data)
{
    CSS_DIALOG(data)->onAttrChanged(repr, name, new_value);
}

Inkscape::XML::NodeEventVector css_repr_events = {
    nullptr,                  /* child_added */
    nullptr,                  /* child_removed */
    on_attr_changed, nullptr, /* content_changed */
    nullptr                   /* order_changed */
};

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * Constructor
 * A treeview whose each row corresponds to a CSS property of selector selected.
 * New CSS property can be added by clicking '+' at bottom of the CSS pane. '-'
 * in front of the CSS property row can be clicked to delete the CSS property.
 * Besides clicking on an already selected property row makes the property editable
 * and clicking 'Enter' updates the property with changes reflected in the
 * drawing.
 */
CssDialog::CssDialog()
    : UI::Widget::Panel("/dialogs/css", SP_VERB_DIALOG_CSS)
    , _desktop(nullptr)
    , _repr(nullptr)
{
    set_size_request(20, 15);
    _treeView.set_headers_visible(true);
    auto _scrolledWindow = new Gtk::ScrolledWindow();
    _scrolledWindow->add(_treeView);
    _scrolledWindow->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    _store = Gtk::ListStore::create(_cssColumns);
    _treeView.set_model(_store);

    Inkscape::UI::Widget::IconRenderer *addRenderer = manage(new Inkscape::UI::Widget::IconRenderer());
    addRenderer->add_icon("edit-delete");
    addRenderer->signal_activated().connect(sigc::mem_fun(*this, &CssDialog::onPropertyDelete));

    _message_stack = std::make_shared<Inkscape::MessageStack>();
    _message_context = std::unique_ptr<Inkscape::MessageContext>(new Inkscape::MessageContext(_message_stack));
    _message_changed_connection =
        _message_stack->connectChanged(sigc::bind(sigc::ptr_fun(_set_status_message), GTK_WIDGET(status.gobj())));

    int addCol = _treeView.append_column("", *addRenderer) - 1;
    Gtk::TreeViewColumn *col = _treeView.get_column(addCol);
    if (col) {
        col->add_attribute(addRenderer->property_visible(), _cssColumns.deleteButton);
        col->set_sort_column(_cssColumns.deleteButton);

        Gtk::Image *add_icon = Gtk::manage(sp_get_icon_image("list-add", Gtk::ICON_SIZE_SMALL_TOOLBAR));
        col->set_clickable(true);
        col->set_widget(*add_icon);
        add_icon->set_tooltip_text(_("Add a new style property"));
        add_icon->show();
        // This gets the GtkButton inside the GtkBox, inside the GtkAlignment, inside the GtkImage icon.
        auto button = add_icon->get_parent()->get_parent()->get_parent();
        // Assign the button event so that create happens BEFORE delete. If this code
        // isn't in this exact way, the onAttrDelete is called when the header lines are pressed.
        button->signal_button_release_event().connect(sigc::mem_fun(*this, &CssDialog::onPropertyCreate), false);
    }

    Gtk::CellRendererText *renderer = Gtk::manage(new Gtk::CellRendererText());
    renderer->property_editable() = true;
    int nameColNum = _treeView.append_column("Property", *renderer) - 1;
    _propCol = _treeView.get_column(nameColNum);
    if (_propCol) {
        _propCol->add_attribute(renderer->property_text(), _cssColumns.label);
        _propCol->add_attribute(renderer->property_foreground_rgba(), _cssColumns.label_color);
        _propCol->set_sort_column(_cssColumns.label);
    }

    renderer = Gtk::manage(new Gtk::CellRendererText());
    renderer->property_editable() = false;
    int attrColNum = _treeView.append_column("Set", *renderer) - 1;
    _attrCol = _treeView.get_column(attrColNum);
    if (_attrCol) {
        _attrCol->add_attribute(renderer->property_text(), _cssColumns._styleAttrVal);
        _attrCol->add_attribute(renderer->property_foreground_rgba(), _cssColumns.attr_color);
        _attrCol->add_attribute(renderer->property_strikethrough(), _cssColumns.attr_strike);
        _attrCol->add_attribute(renderer->property_editable(), _cssColumns.editable);
        _attrCol->set_sort_column(_cssColumns._styleAttrVal);
    }

    renderer = Gtk::manage(new Gtk::CellRendererText());
    renderer->property_editable() = true;
    int sheetColNum = _treeView.append_column("Actual", *renderer) - 1;
    _sheetCol = _treeView.get_column(sheetColNum);
    if (_sheetCol) {
        _sheetCol->add_attribute(renderer->property_text(), _cssColumns._styleSheetVal);
        _sheetCol->add_attribute(renderer->property_foreground_rgba(), _cssColumns.label_color);
        _sheetCol->set_sort_column(_cssColumns._styleSheetVal);
    }

    // Set the inital sort column (and direction) to place real attributes at the top.
    _store->set_sort_column(_cssColumns.deleteButton, Gtk::SORT_DESCENDING);

    _getContents()->pack_start(*_scrolledWindow, Gtk::PACK_EXPAND_WIDGET);

    css_reset_context(0);
    setDesktop(getDesktop());
}

/**
 * @brief CssDialog::~CssDialog
 * Class destructor
 */
CssDialog::~CssDialog()
{
    setDesktop(nullptr);
    _repr = nullptr;
    _message_changed_connection.disconnect();
    _message_context = nullptr;
    _message_stack = nullptr;
    _message_changed_connection.~connection();
}

void CssDialog::_set_status_message(Inkscape::MessageType /*type*/, const gchar *message, GtkWidget *widget)
{
    if (widget) {
        gtk_label_set_markup(GTK_LABEL(widget), message ? message : "");
    }
}


/**
 * @brief CssDialog::setDesktop
 * @param desktop
 * This function sets the 'desktop' for the CSS pane.
 */
void CssDialog::setDesktop(SPDesktop* desktop)
{
    _desktop = desktop;
}

/**
 * @brief CssDialog::setRepr
 *
 * Set the internal xml object that I'm working on right now.
 */
void CssDialog::setRepr(Inkscape::XML::Node *repr)
{
    if (repr == _repr)
        return;
    if (_repr) {
        _store->clear();
        _repr->removeListenerByData(this);
        Inkscape::GC::release(_repr);
        _repr = nullptr;
    }
    _repr = repr;
    if (repr) {
        Inkscape::GC::anchor(_repr);
        _repr->addListener(&css_repr_events, this);
        _repr->synthesizeEvents(&css_repr_events, this);
    }
}

/**
 * @brief CssDialog::parseStyle
 *
 * Convert a style string into a vector map. This should be moved to style.cpp
 *
 */
std::map<Glib::ustring, Glib::ustring> CssDialog::parseStyle(Glib::ustring style_string)
{
    std::map<Glib::ustring, Glib::ustring> ret;

    REMOVE_SPACES(style_string); // We'd use const, but we need to trip spaces
    std::vector<Glib::ustring> props = r_props->split(style_string);

    for (auto const token : props) {
        if (token.empty())
            break;
        std::vector<Glib::ustring> pair = r_pair->split(token);

        if (pair.size() > 1) {
            ret[pair[0]] = pair[1];
        }
    }
    return ret;
}

/**
 * @brief CssDialog::compileStyle
 *
 * Turn a vector map back into a style string.
 *
 */
Glib::ustring CssDialog::compileStyle(std::map<Glib::ustring, Glib::ustring> props)
{
    auto ret = Glib::ustring("");
    for (auto const pair : props) {
        if (!pair.first.empty() && !pair.second.empty()) {
            ret += pair.first;
            ret += ":";
            ret += pair.second;
            ret += ";";
        }
    }
    return ret;
}


/**
 * @brief CssDialog::onAttrChanged
 *
 * This is called when the XML has an updated attribute (we only care about style)
 */
void CssDialog::onAttrChanged(Inkscape::XML::Node *repr, const gchar *name, const gchar *new_value)
{
    if (strcmp(name, "style") != 0)
        return;

    // Clear the list and return if the new_value is empty
    _store->clear();
    if (!new_value || new_value[0] == 0)
        return;

    // Get the object's style attribute and it's calculated properties
    SPDocument *document = this->_desktop->doc();
    SPObject *obj = document->getObjectByRepr(repr);
    // std::vector<SPIBase *> calc_prop = obj->style->properties();

    // Get a dictionary lookup of the style in the attribute
    std::map<Glib::ustring, Glib::ustring> attr_prop = parseStyle(new_value);

    for (auto iter : obj->style->properties()) {
        if (iter->style && iter->style_src != SP_STYLE_SRC_UNSET) {
            Gtk::TreeModel::Row row = *(_store->append());
            // Delete is available to attribute properties only in attr mode.
            row[_cssColumns.deleteButton] = iter->style_src == SP_STYLE_SRC_ATTRIBUTE;
            row[_cssColumns.label] = iter->name;
            if (attr_prop.count(iter->name)) {
                row[_cssColumns._styleAttrVal] = attr_prop[iter->name];
                if (attr_prop[iter->name] != iter->get_value()) {
                    row[_cssColumns._styleSheetVal] = iter->get_value();
                    row[_cssColumns.attr_color] = Gdk::RGBA("gray");
                    row[_cssColumns.attr_strike] = true;
                }
                row[_cssColumns.deleteButton] = true;
            } else {
                row[_cssColumns._styleSheetVal] = iter->get_value();
                row[_cssColumns.label_color] = Gdk::RGBA("gray");
                row[_cssColumns.attr_color] = Gdk::RGBA("gray");
                row[_cssColumns.deleteButton] = false;
            }
        }
    }
}

/*
 * Sets the CSSDialog status bar, depending on which attr is selected.
 */
void CssDialog::css_reset_context(gint css)
{
    if (css == 0) {
        _message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Click</b> CSS property to edit."));
    } else {
        const gchar *name = g_quark_to_string(css);
        _message_context->setF(
            Inkscape::NORMAL_MESSAGE,
            _("Property <b>%s</b> selected. Press <b>Ctrl+Enter</b> when done editing to commit changes."), name);
    }
}

/**
 * @brief CssDialog::setStyleProperty
 *
 * Set or delete a single property in the style attribute.
 */
bool CssDialog::setStyleProperty(Glib::ustring name, Glib::ustring value)
{
    auto original = this->_repr->attribute("style");
    std::map<Glib::ustring, Glib::ustring> properties = parseStyle(original);

    bool updated = false;
    if (value != nullptr && !value.empty()) {
        if (properties[name] != value) {
            // Set value (create or update)
            properties[name] = value;
            updated = true;
        }
    } else if (properties.count(name)) {
        // Delete value
        properties.erase(name);
        updated = true;
    }

    if (updated) {
        auto new_styles = this->compileStyle(properties);
        this->_repr->setAttribute("style", new_styles, false);
        // this->setUndo(_("Delete style property"));
    }
    return updated;
}

/**
 * @brief CssDialog::onPropertyDelete
 *
 * This function is a slot to signal_activated for '-' button panel.
 */
void CssDialog::onPropertyDelete(Glib::ustring path)
{
    Gtk::TreeModel::Row row = *_store->get_iter(path);
    if (row) {
        this->setStyleProperty(row[_cssColumns.label], nullptr);
    }
}

/**
 * @brief CssDialog::onPropertyCreate
 * This function is a slot to signal_clicked for '+' button panel.
 */
bool CssDialog::onPropertyCreate(GdkEventButton *event)
{
    if (event->type == GDK_BUTTON_RELEASE && event->button == 1 && this->_repr) {
        Gtk::TreeIter iter = _store->append();
        Gtk::TreeModel::Path path = (Gtk::TreeModel::Path)iter;
        _treeView.set_cursor(path, *_propCol, true);
        grab_focus();
        return true;
    }
    return false;
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape
