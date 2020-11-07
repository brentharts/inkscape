// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A simple panel for objects (originally developed for Ponyscape, an Inkscape derivative)
 *
 * Authors:
 *   Theodore Janeczko
 *   Tweaked by Liam P White for use in Inkscape
 *   Tavmjong Bah
 *
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *               Tavmjong Bah 2017
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "objects.h"

#include <gtkmm/icontheme.h>
#include <gtkmm/imagemenuitem.h>
#include <gtkmm/separatormenuitem.h>
#include <glibmm/main.h>
#include <glibmm/i18n.h>

#include "desktop-style.h"
#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "filter-chemistry.h"
#include "inkscape.h"
#include "layer-manager.h"
#include "verbs.h"

#include "actions/actions-tools.h"

#include "helper/action.h"

#include "include/gtkmm_version.h"

#include "object/filters/blend.h"
#include "object/filters/gaussian-blur.h"
#include "object/sp-clippath.h"
#include "object/sp-mask.h"
#include "object/sp-root.h"
#include "object/sp-shape.h"
#include "style.h"

#include "ui/dialog-events.h"
#include "ui/icon-loader.h"
#include "ui/icon-names.h"
#include "ui/selected-color.h"
#include "ui/shortcuts.h"
#include "ui/desktop/menu-icon-shift.h"
#include "ui/tools/node-tool.h"

#include "ui/widget/canvas.h"
#include "ui/widget/clipmaskicon.h"
#include "ui/widget/color-notebook.h"
#include "ui/widget/highlight-picker.h"
#include "ui/widget/imagetoggler.h"
#include "ui/widget/insertordericon.h"
#include "ui/widget/layertypeicon.h"
#include "ui/widget/shapeicon.h"

//#define DUMP_LAYERS 1

namespace Inkscape {
namespace UI {
namespace Dialog {

class ObjectsPanel::ModelColumns : public Gtk::TreeModel::ColumnRecord
{
public:
    ModelColumns()
    {
        add(_colObject);
        add(_colLabel);
        add(_colType);
        add(_colIconColor);
        add(_colBgColor);
        add(_colVisible);
        add(_colLocked);
    }
    ~ModelColumns() override = default;
    Gtk::TreeModelColumn<SPItem*> _colObject;
    Gtk::TreeModelColumn<Glib::ustring> _colLabel;
    Gtk::TreeModelColumn<Glib::ustring> _colType;
    Gtk::TreeModelColumn<unsigned int> _colIconColor;
    Gtk::TreeModelColumn<Gdk::RGBA> _colBgColor;
    Gtk::TreeModelColumn<bool> _colVisible;
    Gtk::TreeModelColumn<bool> _colLocked;
};

/**
 * Gets an instance of the Objects panel
 */
ObjectsPanel& ObjectsPanel::getInstance()
{
    return *new ObjectsPanel();
}

/**
 * Creates a new ObjectWatcher, a gtk TreeView interated watching device.
 *
 * @param panel The panel to which the object watcher belongs
 * @param obj The object to watch
 * @param iter The optional list store iter for the item, if not provided,
 *             assumes this is the root 'document' object.
 */
ObjectWatcher::ObjectWatcher(ObjectsPanel* panel, SPObject* obj, Gtk::TreeRow *row) :
    panel(panel),
    row_ref(),
    selection_state(0),
    node(obj->getRepr())
{
    if(row != nullptr) {
        auto path = panel->_store->get_path(*row);
        row_ref = Gtk::TreeModel::RowReference(panel->_store, path);
        updateRowInfo();
    }
    node->addObserver(*this);
    for (auto& child: obj->children) {
        addChild(*(child.getRepr()));
    }
}
ObjectWatcher::~ObjectWatcher()
{
    node->removeObserver(*this);
    Gtk::TreeModel::Path path;
    if (bool(row_ref) && (path = row_ref.get_path())) {
        auto iter = panel->_store->get_iter(path);
        if(iter) {
            panel->_store->erase(iter);
        }
    }
    child_watchers.clear();
}

/**
 * Update the information in the row from the stored node
 */
void ObjectWatcher::updateRowInfo() {
    auto item = dynamic_cast<SPItem *>(panel->getObject(node));
    auto row = *panel->_store->get_iter(row_ref.get_path());

    if (item) {
        gchar const * label = item->label() ? item->label() : item->getId();
        row[panel->_model->_colObject] = item;
        row[panel->_model->_colLabel] = label ? label : item->defaultLabel();
        row[panel->_model->_colType] = item->typeName();
        row[panel->_model->_colIconColor] = item->highlight_color();
        row[panel->_model->_colVisible] = !item->isHidden();
        row[panel->_model->_colLocked] = !item->isSensitive();
    }
}

/**
 * Updates the row's background colour as indicated by it's selection.
 */
void ObjectWatcher::updateRowBg()
{
    auto row = *panel->_store->get_iter(row_ref.get_path());
    if (row) {
        auto alpha = SELECTED_ALPHA[selection_state];
        if (alpha == 0.0) {
            row[panel->_model->_colBgColor] = Gdk::RGBA();
            return;
        }

        auto color = ColorRGBA(row[panel->_model->_colIconColor]);
        auto gdk_color = Gdk::RGBA();
        gdk_color.set_red(color[0]);
        gdk_color.set_green(color[1]);
        gdk_color.set_blue(color[2]);
        gdk_color.set_alpha(color[3] / alpha);
        row[panel->_model->_colBgColor] = gdk_color;
    }
}

/**
 * Flip the selected state bit on or off as needed, calls updateRowBg if changed.
 *
 * @param mask - The selection bit to set or unset
 * @param enabled - If the bit should be set or unset
 */
void ObjectWatcher::setSelectedBit(SelectionState mask, bool enabled) {
    if (!row_ref) return;
    auto row = *panel->_store->get_iter(row_ref.get_path());
    if (row) {
        SelectionState value = selection_state;
        SelectionState original = value;
        if (enabled) {
            value |= mask;
        } else {
            value &= ~mask;
        }
        if (value != original) {
            selection_state = value;
            updateRowBg();
        }
    }
}


/**
 * Add the child object to this node.
 *
 * @param child - SPObject to be added
 */
void ObjectWatcher::addChild(Node &node)
{
    auto obj = panel->getObject(&node);
    if (!obj || !dynamic_cast<SPItem *>(obj))
        return; // Object must be a viable SPItem.
    Gtk::TreeModel::Row row = *(panel->_store->append(getParentIter()));
    auto watcher = new ObjectWatcher(panel, obj, &row);
    child_watchers.insert(std::make_pair(&node, watcher));
    // Make sure new children have the right focus set.
    if ((selection_state & LAYER_FOCUSED) != 0) {
        watcher->setSelectedBit(LAYER_FOCUS_CHILD, true);
    }
}

/**
 * Move the child to just after the given sibling
 *
 * @param child - SPObject to be moved
 * @param sibling - Optional sibling Object to add next to, if nullptr the
 *                  object is moved to BEFORE the first item.
 */
void ObjectWatcher::moveChild(SPObject *child, SPObject *sibling)
{
    auto child_iter = getChildIter(child);
    if (!child_iter)
        return; // This means the child was never added, probably not an SPItem.
    auto sibling_iter = getChildIter(sibling);
    auto child_const = const_cast<GtkTreeIter*>(child_iter.get_gobject_if_not_end());
    GtkTreeIter* sibling_const = nullptr;
    if (sibling_iter) {
        sibling_const = const_cast<GtkTreeIter*>(sibling_iter.get_gobject_if_not_end());
    }
    // Gtkmm can move to before, but not move to 'after'
    gtk_tree_store_move_after(panel->_store->gobj(), child_const, sibling_const);
}

/**
 * Get the parent TreeRow's children iterator to this object
 *
 * @returns Gtk Tree Node Children iterator
 */
Gtk::TreeNodeChildren ObjectWatcher::getParentIter()
{
    Gtk::TreeModel::Path path;
    if (row_ref && (path = row_ref.get_path())) {
        const Gtk::TreeRow row = **panel->_store->get_iter(path);
        return row->children();
    }
    return panel->_store->children();
}

/**
 * Convert SPObject to TreeView Row, assuming the object is a child.
 *
 * @param child - The child object to find in this branch
 * @returns Gtk TreeRow for the child, or nullptr if not found
 */
const Gtk::TreeRow ObjectWatcher::getChildIter(SPObject *child)
{
    Gtk::TreeRow ret;
    for (auto &iter : getParentIter()) {
        if (iter) {
            Gtk::TreeModel::Row row = *iter;
            if(row[panel->_model->_colObject] == child) {
                ret = *iter;
            }
        }
    }
    return ret;
}

void ObjectWatcher::notifyChildAdded( Node &node, Node &child, Node *prev )
{
    addChild(child);
    moveChild(panel->getObject(&child), panel->getObject(prev));
}
void ObjectWatcher::notifyChildRemoved( Node &/*node*/, Node &child, Node* /*prev*/ )
{
    child_watchers.erase(&child);
}
void ObjectWatcher::notifyChildOrderChanged( Node &parent, Node &child, Node */*old_prev*/, Node *new_prev )
{
    moveChild(panel->getObject(&child), panel->getObject(new_prev));
}
void ObjectWatcher::notifyAttributeChanged( Node &node, GQuark name, Util::ptr_shared /*old_value*/, Util::ptr_shared /*new_value*/ )
{
    // Almost anything could change the icon, so update upon any change, defer for lots of updates.
    updateRowInfo();
}


/**
 * Get the object from the node.
 *
 * @param node - XML Node involved in the signal.
 * @returns SPObject matching the node, returns nullptr if not found.
 */
SPObject *ObjectsPanel::getObject(Node *node) {
    if (node != nullptr)
        return _document->getObjectByRepr(node);
    return nullptr;
}

/**
 * Get the object watcher from the xml node (reverse lookup), it uses a ancesstor
 * recursive pattern to match up with the root_watcher.
 *
 * @param node - The node to look up.
 * @return the ObjectWatcher object if it's possible to find.
 */
ObjectWatcher* ObjectsPanel::getWatcher(Node *node)
{
    if (root_watcher->node == node) {
        return root_watcher;
    } else if (node->parent()) {
        auto parent_watcher = getWatcher(node->parent());
        if (parent_watcher) {
            return &(*parent_watcher->child_watchers[node]);
        }
    }
    g_warning("Can't find node in reverse lookup.");
    return nullptr;
}

class ObjectsPanel::InternalUIBounce
{
public:
    int _actionCode;
    int _actionCode2;
    sigc::connection _signal;
};

/**
 * Constructor
 */
ObjectsPanel::ObjectsPanel() :
    UI::Widget::Panel("/dialogs/objects", SP_VERB_DIALOG_OBJECTS),
    root_watcher(nullptr),
    _desktop(nullptr),
    _document(nullptr),
    _model(nullptr),
    _layer(nullptr),
    _page(Gtk::ORIENTATION_VERTICAL)
{
    //Create the tree model and store
    ModelColumns *zoop = new ModelColumns();
    _model = zoop;

    _store = Gtk::TreeStore::create( *zoop );

    //Set up the tree
    _tree.set_model( _store );
    _tree.set_headers_visible(false);
    // Reorderable means that we allow drag-and-drop, but we only allow that
    // when at least one row is selected
    _tree.set_reorderable(true);
    _tree.enable_model_drag_dest (Gdk::ACTION_MOVE);

    //Label
    _name_column = Gtk::manage(new Gtk::TreeViewColumn());
    _text_renderer = Gtk::manage(new Gtk::CellRendererText());
    _text_renderer->property_editable() = true;
    auto icon_renderer = Gtk::manage(new Inkscape::UI::Widget::CellRendererItemIcon());
    icon_renderer->property_xpad() = 2;
    icon_renderer->property_width() = 24;
    _tree.append_column(*_name_column);
    _name_column->set_expand(true);
    _name_column->pack_start(*icon_renderer, false);
    _name_column->pack_start(*_text_renderer, true);
    _name_column->add_attribute(_text_renderer->property_text(), _model->_colLabel);
    _name_column->add_attribute(_text_renderer->property_cell_background_rgba(), _model->_colBgColor);
    _name_column->add_attribute(icon_renderer->property_shape_type(), _model->_colType);
    _name_column->add_attribute(icon_renderer->property_color(), _model->_colIconColor);
    _name_column->add_attribute(icon_renderer->property_cell_background_rgba(), _model->_colBgColor);

    //Visible
    auto *eyeRenderer = Gtk::manage( new Inkscape::UI::Widget::ImageToggler(INKSCAPE_ICON("object-visible"), INKSCAPE_ICON("object-hidden")));

    int visibleColNum = _tree.append_column("vis", *eyeRenderer) - 1;
    eyeRenderer->signal_toggled().connect(sigc::mem_fun(*this, &ObjectsPanel::toggleVisible));
    Gtk::TreeViewColumn* col = _tree.get_column(visibleColNum);
    if ( col ) {
        col->add_attribute( eyeRenderer->property_active(), _model->_colVisible );
        col->add_attribute( eyeRenderer->property_cell_background_rgba(), _model->_colBgColor);
    }

    //Locked
    Inkscape::UI::Widget::ImageToggler * renderer = Gtk::manage( new Inkscape::UI::Widget::ImageToggler(
        INKSCAPE_ICON("object-locked"), INKSCAPE_ICON("object-unlocked")));
    int lockedColNum = _tree.append_column("lock", *renderer) - 1;
    renderer->signal_toggled().connect(sigc::mem_fun(*this, &ObjectsPanel::toggleLocked));
    col = _tree.get_column(lockedColNum);
    if ( col ) {
        col->add_attribute( renderer->property_active(), _model->_colLocked );
        col->add_attribute( renderer->property_cell_background_rgba(), _model->_colBgColor);
    }

    //Set the expander and search columns
    _tree.set_expander_column( *_name_column );
    _tree.set_search_column(_model->_colLabel);
    _tree.set_enable_search(true);

    //_tree.get_selection()->set_select_function(sigc::mem_fun(*this, &ObjectsPanel::select_row));
    _tree.get_selection()->set_mode(Gtk::SELECTION_NONE);

    //Set up tree signals
    _tree.signal_button_release_event().connect( sigc::mem_fun(*this, &ObjectsPanel::_handleButtonEvent), false );
    _tree.signal_key_press_event().connect( sigc::mem_fun(*this, &ObjectsPanel::_handleKeyEvent), false );
    _tree.signal_row_collapsed().connect( sigc::bind<bool>(sigc::mem_fun(*this, &ObjectsPanel::_setExpanded), false));
    _tree.signal_row_expanded().connect( sigc::bind<bool>(sigc::mem_fun(*this, &ObjectsPanel::_setExpanded), true));

    //Set up the label editing signals
    _text_renderer->signal_edited().connect( sigc::mem_fun(*this, &ObjectsPanel::_handleEdited));

    //Set up the scroller window and pack the page
    _scroller.add(_tree);
    _scroller.set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC );
    _scroller.set_shadow_type(Gtk::SHADOW_IN);
    Gtk::Requisition sreq;
    Gtk::Requisition sreq_natural;
    _scroller.get_preferred_size(sreq_natural, sreq);
    int minHeight = 70;
    if (sreq.height < minHeight) {
        // Set a min height to see the layers when used with Ubuntu liboverlay-scrollbar
        _scroller.set_size_request(sreq.width, minHeight);
    }

    _page.pack_start( _scroller, Gtk::PACK_EXPAND_WIDGET );
    _page.pack_end(_buttonsRow, Gtk::PACK_SHRINK);
    _getContents()->pack_start(_page, Gtk::PACK_EXPAND_WIDGET);

    _addBarButton(INKSCAPE_ICON("list-add"), _("Add layer..."), (int)SP_VERB_LAYER_NEW);
    _addBarButton(INKSCAPE_ICON("list-remove"), _("Remove object"), (int)SP_VERB_EDIT_DELETE);
    _addBarButton(INKSCAPE_ICON("go-bottom"), _("Move To Bottom"), (int)SP_VERB_SELECTION_TO_BACK);
    _addBarButton(INKSCAPE_ICON("go-down"), _("Move Down"), (int)SP_VERB_SELECTION_STACK_DOWN);
    _addBarButton(INKSCAPE_ICON("go-up"), _("Move Up"), (int)SP_VERB_SELECTION_STACK_UP);
    _addBarButton(INKSCAPE_ICON("go-top"), _("Move To Top"), (int)SP_VERB_SELECTION_TO_FRONT);

    _buttonsRow.pack_start(_buttonsSecondary, Gtk::PACK_EXPAND_WIDGET);
    _buttonsRow.pack_end(_buttonsPrimary, Gtk::PACK_EXPAND_WIDGET);

    setDesktop(getDesktop());
    show_all_children();
}

/**
 * Destructor
 */
ObjectsPanel::~ObjectsPanel()
{
    //Set the desktop to null, which will disconnect all object watchers
    setDesktop(nullptr);
    if (_model) {
        delete _model;
        _model = nullptr;
    }
}

/**
 * Sets the current document
 */
void ObjectsPanel::setDocument(SPDesktop* desktop, SPDocument* document)
{
    g_assert(desktop == _desktop);

    if (root_watcher) {
        delete root_watcher;
    }

    _document = document;
    root_watcher = nullptr;

    if (document && document->getRoot()) {
        root_watcher = new ObjectWatcher(this, document->getRoot(), nullptr);
        setLayer(_desktop->currentLayer());
    }
}

/**
 * Set the current panel desktop
 */
void ObjectsPanel::setDesktop( SPDesktop* desktop )
{
    Panel::setDesktop(desktop);

    if ( desktop != _desktop ) {
        document_changed.disconnect();
        selection_changed.disconnect();
        layer_changed.disconnect();
        if (_desktop ) {
            _desktop = nullptr;
        }

        _desktop = Panel::getDesktop();
        if (_desktop ) {
            document_changed = _desktop->connectDocumentReplaced( sigc::mem_fun(*this, &ObjectsPanel::setDocument));
            selection_changed = _desktop->selection->connectChanged( sigc::mem_fun(*this, &ObjectsPanel::setSelection));
            layer_changed = _desktop->connectCurrentLayerChanged( sigc::mem_fun(*this, &ObjectsPanel::setLayer));
            setDocument(_desktop, _desktop->doc());
            connectPopupItems();
        } else {
            setDocument(_desktop, nullptr);
        }
    }
}

/**
 * Occurs when the current desktop selection changes
 *
 * @param sel The current selection
 */
void ObjectsPanel::setSelection(Selection *selected)
{
    for (auto item : selection.items()) {
        auto watcher = getWatcher(item->getRepr());
        if (watcher) {
            watcher->setSelectedBit(SELECTED_OBJECT, false);
        }
    }
    selection.clear();
    for (auto item : selected->items()) {
        auto watcher = getWatcher(item->getRepr());
        if (watcher) {
            watcher->setSelectedBit(SELECTED_OBJECT, true);
        }
        selection.add(item);
    }
}

/**
 * Happens when the layer selected is changed.
 *
 * @param layer - The layer now selected
 */
void ObjectsPanel::setLayer(SPObject *layer)
{
    if (_layer) {
        auto old_watcher = getWatcher(_layer->getRepr());
        old_watcher->setSelectedBit(LAYER_FOCUSED, false);
        for (const auto &iter : old_watcher->child_watchers) {
            iter.second->setSelectedBit(LAYER_FOCUS_CHILD, false);
        }
    }
    if (!layer) return;
    auto watcher = getWatcher(layer->getRepr());
    if (watcher) {
        watcher->setSelectedBit(LAYER_FOCUSED, true);
        for (const auto &iter : watcher->child_watchers) {
            iter.second->setSelectedBit(LAYER_FOCUS_CHILD, true);
        }
    }
    _layer = layer;
}


/**
 * Stylizes a button using the given icon name and tooltip
 */
void ObjectsPanel::_addBarButton(char const* iconName, char const* tooltip, int verb_id)
{
    Gtk::Button* btn = Gtk::manage(new Gtk::Button());
    auto child = Glib::wrap(sp_get_icon_image(iconName, GTK_ICON_SIZE_SMALL_TOOLBAR));
    child->show();
    btn->add(*child);
    btn->set_relief(Gtk::RELIEF_NONE);
    btn->set_tooltip_text(tooltip);
    btn->signal_clicked().connect(sigc::bind( sigc::mem_fun(*this, &ObjectsPanel::_takeAction), verb_id));
    _buttonsSecondary.pack_start(*btn, Gtk::PACK_SHRINK);
}

/**
 * Adds an item to the pop-up (right-click) menu
 * @param desktop The active destktop
 * @param code Action code
 * @return The generated menu item
 */
Gtk::MenuItem& ObjectsPanel::_addPopupItem( SPDesktop *desktop, unsigned int code)
{
    Verb *verb = Verb::get( code );
    g_assert(verb);
    SPAction *action = verb->get_action(Inkscape::ActionContext(desktop));

    Gtk::MenuItem* item = Gtk::manage(new Gtk::MenuItem());

    Gtk::Label *label = Gtk::manage(new Gtk::Label(action->name, true));
    label->set_xalign(0.0);

    if (_show_contextmenu_icons && action->image) {
        item->set_name("ImageMenuItem");  // custom name to identify our "ImageMenuItems"
        Gtk::Image *icon = Gtk::manage(sp_get_icon_image(action->image, Gtk::ICON_SIZE_MENU));

        // Create a box to hold icon and label as Gtk::MenuItem derives from GtkBin and can only hold one child
        Gtk::Box *box = Gtk::manage(new Gtk::Box());
        box->pack_start(*icon, false, false, 0);
        box->pack_start(*label, true,  true,  0);
        item->add(*box);
    } else {
        item->add(*label);
    }

    item->signal_activate().connect(sigc::bind(sigc::mem_fun(*this, &ObjectsPanel::_takeAction), code));
    _popupMenu.append(*item);

    return *item;
}


/**
 * Sets visibility of items in the tree
 * @param iter Current item in the tree
 */
void ObjectsPanel::toggleVisible(const Glib::ustring& path)
{
    Gtk::TreeModel::Row row = *_store->get_iter(path);
    SPItem* item = row[_model->_colObject];
    if (item) {
        item->setHidden(row[_model->_colVisible]);
    }
}

/**
 * Sets sensitivity of items in the tree
 * @param iter Current item in the tree
 * @param locked Whether the item should be locked
 */
void ObjectsPanel::toggleLocked(const Glib::ustring& path)
{
    Gtk::TreeModel::Row row = *_store->get_iter(path);
    SPItem* item = row[_model->_colObject];
    if (item) {
        item->setLocked(!row[_model->_colLocked]);
    }
}

/**
 * Handles keyboard events
 * @param event Keyboard event passed in from GDK
 * @return Whether the event should be eaten (om nom nom)
 */
bool ObjectsPanel::_handleKeyEvent(GdkEventKey *event)
{
    if (!_desktop)
        return false;

    Gtk::AccelKey shortcut = Inkscape::Shortcuts::get_from_event(event);
    switch (shortcut.get_key()) {
        case GDK_KEY_Escape:
            if (_desktop->canvas) {
                _desktop->canvas->grab_focus();
                return true;
            }
            break;
    }

    // invoke user defined shortcuts first
    if (Inkscape::Shortcuts::getInstance().invoke_verb(event, _desktop))
        return true;
    return false;
}

/**
 * Handles mouse events
 * @param event Mouse event from GDK
 * @return whether to eat the event (om nom nom)
 */
bool ObjectsPanel::_handleButtonEvent(GdkEventButton* event)
{
    Gtk::TreeModel::Path path;
    Gtk::TreeViewColumn* col = nullptr;
    int x, y;
    if (_tree.get_path_at_pos((int)event->x, (int)event->y, path, col, x, y)) {
        // This doesn't work, it might be being eaten.
        if (event->type == GDK_2BUTTON_PRESS) {
            _tree.set_cursor(path, *col, true);
            return true;
        }

        auto selection = _getSelection();
        auto row = *_store->get_iter(path);
        if (!row) return false;
        SPItem *item = row[_model->_colObject];
        SPGroup *group = SP_GROUP(item);

        // Clicking on layers firstly switches to that layer.
        if (group && group->layerMode() == SPGroup::LAYER) {
            if(selection->includes(item)) {
                selection->clear();
            } else if (_layer != item) {
                selection->clear();
                _desktop->setCurrentLayer(item);
            }
        } else if (event->state & GDK_SHIFT_MASK) {
            selection->toggle(item);
        } else {
            selection->set(item);
        }
    }
    return false;
}

/**
 * Executes the given button action during the idle time
 */
void ObjectsPanel::_takeAction(int val)
{
    if (!_document) return;

    Verb *verb = Verb::get(val);
    if (verb) {
        SPAction *action = verb->get_action(_desktop);
        if (action) {
            sp_action_perform( action, nullptr );
        }
    }
}

/**
 * Handle a successful item label edit
 * @param path Tree path of the item currently being edited
 * @param new_text New label text
 */
void ObjectsPanel::_handleEdited(const Glib::ustring& path, const Glib::ustring& new_text)
{
    auto row = *_store->get_iter(path);
    if (row && !new_text.empty()) {
        SPItem *item = row[_model->_colObject];
        if (!item->label() || new_text != item->label()) {
            item->setLabel(new_text.c_str());
            DocumentUndo::done(_document, SP_VERB_NONE, _("Rename object"));
        }
    }
}

/**
 * Take over the select row functionality from the TreeView, this is because
 * we have two selections (layer and object selection) and require a custom
 * method of rendering the result to the treeview.
 */
bool ObjectsPanel::select_row( Glib::RefPtr<Gtk::TreeModel> const & /*model*/, Gtk::TreeModel::Path const &path, bool /*sel*/ )
{
    //g_warning("select_row!");
    auto row = *_store->get_iter(path);
    if (_desktop && row) {
    }
    return true;
}

/**
 * Sets a group to be collapsed and recursively collapses its children
 * @param group The group to collapse
 */
void ObjectsPanel::_setCollapsed(SPGroup * group)
{
    /*group->setExpanded(false);
    group->updateRepr(SP_OBJECT_WRITE_NO_CHILDREN | SP_OBJECT_WRITE_EXT);
    for (auto& iter: group->children) {
        if (SP_IS_GROUP(&iter)) {
            _setCollapsed(SP_GROUP(&iter));
        }
    }*/
}

/**
 * Sets a group to be expanded or collapsed
 * @param iter Current tree item
 * @param isexpanded Whether to expand or collapse
 */
void ObjectsPanel::_setExpanded(const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& /*path*/, bool isexpanded)
{
    /*Gtk::TreeModel::Row row = *iter;

    SPItem* item = row[_model->_colObject];
    if (item && SP_IS_GROUP(item))
    {
        if (isexpanded)
        {
            //If we're expanding, simply perform the expansion
            SP_GROUP(item)->setExpanded(isexpanded);
            item->updateRepr(SP_OBJECT_WRITE_NO_CHILDREN | SP_OBJECT_WRITE_EXT);
        }
        else
        {
            //If we're collapsing, we need to recursively collapse, so call our helper function
            _setCollapsed(SP_GROUP(item));
        }
    }*/
}

void ObjectsPanel::connectPopupItems()
{
    _watching.clear();
    _watchingNonTop.clear();
    _watchingNonBottom.clear();
    _popupMenu = Gtk::Menu();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _show_contextmenu_icons = prefs->getBool("/theme/menuIcons_objects", true);

    _watching.push_back( &_addPopupItem(_desktop, SP_VERB_LAYER_RENAME));
    _watching.push_back( &_addPopupItem(_desktop, SP_VERB_LAYER_NEW));

    _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    _watching.push_back( &_addPopupItem(_desktop, SP_VERB_LAYER_SOLO));
    _watching.push_back( &_addPopupItem(_desktop, SP_VERB_LAYER_SHOW_ALL));
    _watching.push_back( &_addPopupItem(_desktop, SP_VERB_LAYER_HIDE_ALL));

    _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    _watching.push_back( &_addPopupItem(_desktop, SP_VERB_LAYER_LOCK_OTHERS));
    _watching.push_back( &_addPopupItem(_desktop, SP_VERB_LAYER_LOCK_ALL));
    _watching.push_back( &_addPopupItem(_desktop, SP_VERB_LAYER_UNLOCK_ALL));

    _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    _watchingNonTop.push_back( &_addPopupItem(_desktop, SP_VERB_SELECTION_STACK_UP));
    _watchingNonBottom.push_back( &_addPopupItem(_desktop, SP_VERB_SELECTION_STACK_DOWN));

    _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    _watching.push_back( &_addPopupItem(_desktop, SP_VERB_SELECTION_GROUP));
    _watching.push_back( &_addPopupItem(_desktop, SP_VERB_SELECTION_UNGROUP));

    _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    _watching.push_back( &_addPopupItem(_desktop, SP_VERB_OBJECT_SET_CLIPPATH));
    _watching.push_back( &_addPopupItem(_desktop, SP_VERB_OBJECT_CREATE_CLIP_GROUP));
    _watching.push_back( &_addPopupItem(_desktop, SP_VERB_OBJECT_UNSET_CLIPPATH));

    _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    _watching.push_back( &_addPopupItem(_desktop, SP_VERB_OBJECT_SET_MASK));
    _watching.push_back( &_addPopupItem(_desktop, SP_VERB_OBJECT_UNSET_MASK));

    _watching.push_back( &_addPopupItem(_desktop, SP_VERB_EDIT_DUPLICATE));
    _watching.push_back( &_addPopupItem(_desktop, SP_VERB_EDIT_DELETE));

    _popupMenu.show_all_children();

    // Install CSS to shift icons into the space reserved for toggles (i.e. check and radio items).
    _popupMenu.signal_map().connect(sigc::bind<Gtk::MenuShell *>(sigc::ptr_fun(shift_icons), &_popupMenu));

    //Set initial sensitivity of buttons
    for (auto & it : _watching) {
        it->set_sensitive( false );
    }
    for (auto & it : _watchingNonTop) {
        it->set_sensitive( false );
    }
    for (auto & it : _watchingNonBottom) {
        it->set_sensitive( false );
    }

}

} //namespace Dialogs
} //namespace UI
} //namespace Inkscape

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
