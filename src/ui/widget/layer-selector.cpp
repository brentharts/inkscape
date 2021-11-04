// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::Widgets::LayerSelector - layer selector widget
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Abhishek Sharma
 *
 * Copyright (C) 2004 MenTaLguY
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstring>
#include <string>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <glibmm/i18n.h>

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "layer-manager.h"
#include "verbs.h"

#include "ui/dialog/layer-properties.h"
#include "ui/icon-loader.h"
#include "ui/icon-names.h"
#include "ui/util.h"
#include "ui/widget/canvas.h" // Focus widget

#include "object/sp-item-group.h"

#include "xml/node-event-vector.h"

namespace Inkscape {
namespace UI {
namespace Widget {

namespace {

class AlternateIcons : public Gtk::Box {
public:
    AlternateIcons(Gtk::BuiltinIconSize size, Glib::ustring const &a, Glib::ustring const &b)
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL)
    , _a(nullptr)
    , _b(nullptr)
    {
        set_name("AlternateIcons");
        if (!a.empty()) {
            _a = Gtk::manage(sp_get_icon_image(a, size));
            _a->set_no_show_all(true);
            add(*_a);
        }
        if (!b.empty()) {
            _b = Gtk::manage(sp_get_icon_image(b, size));
            _b->set_no_show_all(true);
            add(*_b);
        }
        setState(false);
    }

    bool state() const { return _state; }
    void setState(bool state) {
        _state = state;
        if (_state) {
            if (_a) {
                _a->hide();
            }
            if (_b) {
                _b->show();
            }
        } else {
            if (_a) {
                _a->show();
            }
            if (_b) {
                _b->hide();
            }
        }
    }

private:
    Gtk::Image *_a;
    Gtk::Image *_b;
    bool _state;
};

}

/** LayerSelector constructor.  Creates lock and hide buttons,
 *  initializes the layer dropdown selector with a label renderer,
 *  and hooks up signal for setting the desktop layer when the
 *  selector is changed.
 */
LayerSelector::LayerSelector(SPDesktop *desktop)
: Gtk::Box(Gtk::ORIENTATION_HORIZONTAL)
, _desktop(nullptr)
, _layer(nullptr)
{
    set_name("LayerSelector");
    AlternateIcons *label;

    label = Gtk::manage(new AlternateIcons(Gtk::ICON_SIZE_MENU,
        INKSCAPE_ICON("object-visible"), INKSCAPE_ICON("object-hidden")));
    _visibility_toggle.add(*label);
    _visibility_toggle.signal_toggled().connect(
        sigc::compose(
            sigc::mem_fun(*label, &AlternateIcons::setState),
            sigc::mem_fun(_visibility_toggle, &Gtk::ToggleButton::get_active)
        )
    );
    _visibility_toggled_connection = _visibility_toggle.signal_toggled().connect(
        sigc::compose(
            sigc::mem_fun(*this, &LayerSelector::_hideLayer),
            sigc::mem_fun(_visibility_toggle, &Gtk::ToggleButton::get_active)
        )
    );

    _visibility_toggle.set_relief(Gtk::RELIEF_NONE);
    _visibility_toggle.set_tooltip_text(_("Toggle current layer visibility"));
    pack_start(_visibility_toggle, Gtk::PACK_EXPAND_PADDING);

    label = Gtk::manage(new AlternateIcons(Gtk::ICON_SIZE_MENU,
        INKSCAPE_ICON("object-unlocked"), INKSCAPE_ICON("object-locked")));
    _lock_toggle.add(*label);
    _lock_toggle.signal_toggled().connect(
        sigc::compose(
            sigc::mem_fun(*label, &AlternateIcons::setState),
            sigc::mem_fun(_lock_toggle, &Gtk::ToggleButton::get_active)
        )
    );
    _lock_toggled_connection = _lock_toggle.signal_toggled().connect(
        sigc::compose(
            sigc::mem_fun(*this, &LayerSelector::_lockLayer),
            sigc::mem_fun(_lock_toggle, &Gtk::ToggleButton::get_active)
        )
    );

    _lock_toggle.set_relief(Gtk::RELIEF_NONE);
    _lock_toggle.set_tooltip_text(_("Lock or unlock current layer"));
    pack_start(_lock_toggle, Gtk::PACK_EXPAND_PADDING);

    _selector.set_tooltip_text(_("Current layer"));
    pack_start(_selector, Gtk::PACK_EXPAND_WIDGET);

    _layer_model = Gtk::ListStore::create(_model_columns);
    _selector.set_model(_layer_model);
    _selector.pack_start(_label_renderer);
    _selector.set_cell_data_func(
        _label_renderer,
        sigc::mem_fun(*this, &LayerSelector::_prepareLabelRenderer)
    );

    _selection_changed_connection = _selector.signal_changed().connect(
        sigc::mem_fun(*this, &LayerSelector::_setDesktopLayer)
    );
    setDesktop(desktop);
}

/**  Destructor - disconnects signal handler
 */
LayerSelector::~LayerSelector() {
    setDesktop(nullptr);
    _selection_changed_connection.disconnect();
}

/** Sets the desktop for the widget.  First disconnects signals
 *  for the current desktop, then stores the pointer to the
 *  given \a desktop, and attaches its signals to this one.
 *  Then it selects the current layer for the desktop.
 */
void LayerSelector::setDesktop(SPDesktop *desktop) {
    if ( desktop == _desktop )
        return;
    if (_current_layer_changed_connection)
        _current_layer_changed_connection.disconnect();

    _desktop = desktop;

    if (_desktop) {
        _current_layer_changed_connection = _desktop->layerManager().connectCurrentLayerChanged(sigc::mem_fun(*this, &LayerSelector::_selectLayer));
        _selectLayer(dynamic_cast<SPObject *>(_desktop->layerManager().currentLayer()));
    }
}

namespace {

class is_layer {
public:
    is_layer(SPDesktop *desktop) : _desktop(desktop) {}
    bool operator()(SPObject &object) const {
        return _desktop->layerManager().isLayer(&object);
    }
private:
    SPDesktop *_desktop;
};

class column_matches_object {
public:
    column_matches_object(Gtk::TreeModelColumn<SPObject *> const &column,
                          SPObject &object)
    : _column(column), _object(object) {}
    bool operator()(Gtk::TreeModel::const_iterator const &iter) const {
        SPObject *current=(*iter)[_column];
        return current == &_object;
    }
private:
    Gtk::TreeModelColumn<SPObject *> const &_column;
    SPObject &_object;
};

}

/** Selects the given layer in the dropdown selector.
 */
void LayerSelector::_selectLayer(SPObject *layer)
{
    _selection_changed_connection.block();
    _visibility_toggled_connection.block();
    _lock_toggled_connection.block();

    while (!_layer_model->children().empty()) {
        Gtk::ListStore::iterator first_row(_layer_model->children().begin());
        _destroyEntry(first_row);
        _layer_model->erase(first_row);
    }

    SPGroup *root = _desktop->layerManager().currentRoot();
    if (_layer) {
        sp_object_unref(_layer, nullptr);
        _layer = nullptr;
    }

    if (layer) {
        
        std::vector<SPObject*> hierarchy; 
        hierarchy.push_back(layer);
        while(hierarchy.back() != root) hierarchy.push_back(hierarchy.back()->parent);

        if ( layer == root ) {
            _buildEntries(0, hierarchy);
        } else if (!hierarchy.empty()) {
            hierarchy.pop_back();
            _buildSiblingEntries(0, *root, hierarchy);
        }

        Gtk::TreeIter row(
            std::find_if(
                _layer_model->children().begin(),
                _layer_model->children().end(),
                column_matches_object(_model_columns.object, *layer)
            )
        );
        if ( row != _layer_model->children().end() ) {
            _selector.set_active(row);
        }

        _layer = layer;
        sp_object_ref(_layer, nullptr);
    }

    if ( !layer || layer == root ) {
        _visibility_toggle.set_sensitive(false);
        _visibility_toggle.set_active(false);
        _lock_toggle.set_sensitive(false);
        _lock_toggle.set_active(false);
    } else {
        _visibility_toggle.set_sensitive(true);
        _visibility_toggle.set_active(( SP_IS_ITEM(layer) ? SP_ITEM(layer)->isHidden() : false ));
        _lock_toggle.set_sensitive(true);
        _lock_toggle.set_active(( SP_IS_ITEM(layer) ? SP_ITEM(layer)->isLocked() : false ));
    }

    _lock_toggled_connection.unblock();
    _visibility_toggled_connection.unblock();
    _selection_changed_connection.unblock();
}

/** Sets the current desktop layer to the actively selected layer.
 */
void LayerSelector::_setDesktopLayer() {
    SPObject *layer = _selector.get_active()->get_value(_model_columns.object);
    if ( _desktop && layer ) {
        _current_layer_changed_connection.block();
        _layers_changed_connection.block();

        _desktop->layerManager().setCurrentLayer(layer);

        _current_layer_changed_connection.unblock();
        _layers_changed_connection.unblock();

        _selectLayer(dynamic_cast<SPObject *>(_desktop->layerManager().currentLayer()));
    }
    if (_desktop && _desktop->canvas) {
        _desktop->canvas->grab_focus();
    }
}

/** Creates rows in the _layer_model data structure for each item
 *  in \a hierarchy, to a given \a depth.
 */
void LayerSelector::_buildEntries(unsigned depth, std::vector<SPObject*> hierarchy)
{
    auto highest = hierarchy.back();
    hierarchy.pop_back();
    _buildEntry(depth, *highest);

    if (!hierarchy.empty()) {
        _buildEntries(depth+1, hierarchy);
    } else {
        _buildSiblingEntries(depth+1, *highest, hierarchy);
    }
}

/** Creates entries in the _layer_model data structure for
 *  all siblings of the first child in \a parent.
 */
void LayerSelector::_buildSiblingEntries(
    unsigned depth, SPObject &parent,
    std::vector<SPObject*> hierarchy
) {
    auto siblings = parent.children | boost::adaptors::filtered(is_layer(_desktop)) | boost::adaptors::reversed;

    SPObject *layer = ( hierarchy.empty() ? nullptr : hierarchy.back() );

    for (auto& sib: siblings) {
        _buildEntry(depth, sib);
        if ( &sib == layer ) {
            hierarchy.pop_back();
            _buildSiblingEntries(depth+1, *layer, hierarchy);
        }
    }
}

namespace {

struct Callbacks {
    sigc::slot<void> update_row;
    sigc::slot<void> update_list;
};

void attribute_changed(Inkscape::XML::Node */*repr*/, gchar const *name,
                       gchar const */*old_value*/, gchar const */*new_value*/,
                       bool /*is_interactive*/, void *data)
{
    if ( !std::strcmp(name, "inkscape:groupmode") ) {
        reinterpret_cast<Callbacks *>(data)->update_list();
    } else {
        reinterpret_cast<Callbacks *>(data)->update_row();
    }
}

void node_added(Inkscape::XML::Node */*parent*/, Inkscape::XML::Node *child, Inkscape::XML::Node */*ref*/, void *data) {
    gchar const *mode=child->attribute("inkscape:groupmode");
    if ( mode && !std::strcmp(mode, "layer") ) {
        reinterpret_cast<Callbacks *>(data)->update_list();
    }
}

void node_removed(Inkscape::XML::Node */*parent*/, Inkscape::XML::Node *child, Inkscape::XML::Node */*ref*/, void *data) {
    gchar const *mode=child->attribute("inkscape:groupmode");
    if ( mode && !std::strcmp(mode, "layer") ) {
        reinterpret_cast<Callbacks *>(data)->update_list();
    }
}

void node_reordered(Inkscape::XML::Node */*parent*/, Inkscape::XML::Node *child,
                    Inkscape::XML::Node */*old_ref*/, Inkscape::XML::Node */*new_ref*/,
                    void *data)
{
    gchar const *mode=child->attribute("inkscape:groupmode");
    if ( mode && !std::strcmp(mode, "layer") ) {
        reinterpret_cast<Callbacks *>(data)->update_list();
    }
}

void update_row_for_object(SPObject *object,
                           Gtk::TreeModelColumn<SPObject *> const &column,
                           Glib::RefPtr<Gtk::ListStore> const &model)
{
    Gtk::TreeIter row(
        std::find_if(
            model->children().begin(),
            model->children().end(),
            column_matches_object(column, *object)
        )
    );
    if ( row != model->children().end() ) {
        model->row_changed(model->get_path(row), row);
    }
}

void rebuild_all_rows(sigc::slot<void, SPObject *> rebuild, SPDesktop *desktop)
{
    rebuild(desktop->layerManager().currentLayer());
}

}

void LayerSelector::_protectUpdate(sigc::slot<void> slot) {
    bool visibility_blocked=_visibility_toggled_connection.blocked();
    bool lock_blocked=_lock_toggled_connection.blocked();
    _visibility_toggled_connection.block(true);
    _lock_toggled_connection.block(true);
    slot();

    auto layer = _desktop ? _desktop->layerManager().currentLayer() : nullptr;
    if (layer) {
        bool wantedValue = layer->isLocked();
        if ( _lock_toggle.get_active() != wantedValue ) {
            _lock_toggle.set_active( wantedValue );
        }
        wantedValue = layer->isHidden();
        if ( _visibility_toggle.get_active() != wantedValue ) {
            _visibility_toggle.set_active( wantedValue );
        }
    }
    _visibility_toggled_connection.block(visibility_blocked);
    _lock_toggled_connection.block(lock_blocked);
}

/** Builds and appends a row in the layer model object.
 */
void LayerSelector::_buildEntry(unsigned depth, SPObject &object) {
    Inkscape::XML::NodeEventVector *vector;

    Callbacks *callbacks=new Callbacks();

    callbacks->update_row = sigc::bind(
        sigc::mem_fun(*this, &LayerSelector::_protectUpdate),
        sigc::bind(
            sigc::ptr_fun(&update_row_for_object),
            &object, _model_columns.object, _layer_model
        )
    );

    auto layer = _desktop->layerManager().currentLayer();
    if ( (&object == layer) || (&object == layer->parent) ) {
        callbacks->update_list = sigc::bind(
            sigc::mem_fun(*this, &LayerSelector::_protectUpdate),
            sigc::bind(
                sigc::ptr_fun(&rebuild_all_rows),
                sigc::mem_fun(*this, &LayerSelector::_selectLayer),
                _desktop
            )
        );

        Inkscape::XML::NodeEventVector events = {
            &node_added,
            &node_removed,
            &attribute_changed,
            nullptr,
            &node_reordered
        };

        vector = new Inkscape::XML::NodeEventVector(events);
    } else {
        Inkscape::XML::NodeEventVector events = {
            nullptr,
            nullptr,
            &attribute_changed,
            nullptr,
            nullptr
        };

        vector = new Inkscape::XML::NodeEventVector(events);
    }

    Gtk::ListStore::iterator row(_layer_model->append());

    row->set_value(_model_columns.depth, depth);

    sp_object_ref(&object, nullptr);
    row->set_value(_model_columns.object, &object);

    Inkscape::GC::anchor(object.getRepr());
    row->set_value(_model_columns.repr, object.getRepr());

    row->set_value(_model_columns.callbacks, reinterpret_cast<void *>(callbacks));

    sp_repr_add_listener(object.getRepr(), vector, callbacks);
}

/** Removes a row from the _model_columns object, disconnecting listeners
 *  on the slot.
 */
void LayerSelector::_destroyEntry(Gtk::ListStore::iterator const &row) {
    Callbacks *callbacks=reinterpret_cast<Callbacks *>(row->get_value(_model_columns.callbacks));
    SPObject *object=row->get_value(_model_columns.object);
    if (object) {
        sp_object_unref(object, nullptr);
    }
    Inkscape::XML::Node *repr=row->get_value(_model_columns.repr);
    if (repr) {
        sp_repr_remove_listener_by_data(repr, callbacks);
        Inkscape::GC::release(repr);
    }
    delete callbacks;
}

/** Formats the label for a given layer row
 */
void LayerSelector::_prepareLabelRenderer(
    Gtk::TreeModel::const_iterator const &row
) {
    unsigned depth=(*row)[_model_columns.depth];
    SPObject *object=(*row)[_model_columns.object];
    bool label_defaulted(false);

    // TODO: when the currently selected row is removed,
    //       (or before one has been selected) something appears to
    //       "invent" an iterator with null data and try to render it;
    //       where does it come from, and how can we avoid it?
    if ( object && object->getRepr() ) {
        auto layer = _desktop ? _desktop->layerManager().currentLayer() : nullptr;
        auto root = _desktop ? _desktop->layerManager().currentRoot() : nullptr;

        bool isancestor = !( (layer && (object->parent == layer->parent)) || ((layer == root) && (object->parent == root)));

        bool iscurrent = ( (object == layer) && (object != root) );

        gchar *format = g_strdup_printf (
            "<span size=\"smaller\" %s><tt>%*s%s</tt>%s%s%s%%s%s%s%s</span>",
            ( _desktop && _desktop->itemIsHidden (SP_ITEM(object)) ? "foreground=\"gray50\"" : "" ),
            depth, "", ( iscurrent ? "&#8226;" : " " ),
            ( iscurrent ? "<b>" : "" ),
            ( SP_ITEM(object)->isLocked() ? "[" : "" ),
            ( isancestor ? "<small>" : "" ),
            ( isancestor ? "</small>" : "" ),
            ( SP_ITEM(object)->isLocked() ? "]" : "" ),
            ( iscurrent ? "</b>" : "" )
            );

        gchar const *label;
        if ( object != root ) {
            label = object->label();
            if (!object->label()) {
                label = object->defaultLabel();
                label_defaulted = true;
            }
        } else {
            label = _("(root)");
        }

        gchar *text = g_markup_printf_escaped(format, ink_ellipsize_text (label, 50).c_str());
        _label_renderer.property_markup() = text;
        g_free(text);
        g_free(format);
    } else {
        _label_renderer.property_markup() = "<small> </small>";
    }

    _label_renderer.property_ypad() = 1;
    _label_renderer.property_style() = ( label_defaulted ?
                                         Pango::STYLE_ITALIC :
                                         Pango::STYLE_NORMAL );

}

void LayerSelector::_lockLayer(bool lock) {
    if ( _layer && SP_IS_ITEM(_layer) ) {
        SP_ITEM(_layer)->setLocked(lock);
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_NONE,
                           lock? _("Lock layer") : _("Unlock layer"));
    }
}

void LayerSelector::_hideLayer(bool hide) {
    if ( _layer && SP_IS_ITEM(_layer) ) {
        SP_ITEM(_layer)->setHidden(hide);
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_NONE,
                           hide? _("Hide layer") : _("Unhide layer"));
    }
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

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
