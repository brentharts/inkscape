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

#include "xml/node-observer.h"

//#define DUMP_LAYERS 1

namespace Inkscape {
namespace UI {
namespace Dialog {

using Inkscape::XML::Node;

/* Rendering functions for custom cell renderers */
void CellRendererItemIcon::render_vfunc(const Cairo::RefPtr<Cairo::Context>& cr, 
                                      Gtk::Widget& widget,
                                      const Gdk::Rectangle& background_area,
                                      const Gdk::Rectangle& cell_area,
                                      Gtk::CellRendererState flags)
{
    std::string shape_type = _property_shape_type.get_value();
    std::string highlight = "red"; // getHighlightColor
    std::string cache_id = shape_type + "-" + highlight;

    // if the icon isn't cached, render it to a pixbuf
    if ( !_icon_cache[cache_id] ) { 
        _property_icon = sp_get_shape_icon(shape_type, Gdk::RGBA(highlight), 16);
        property_pixbuf() = _icon_cache[cache_id] = _property_icon.get_value();
    } else {
        property_pixbuf() = _icon_cache[cache_id];
    }
  
    Gtk::CellRendererPixbuf::render_vfunc(cr, widget, background_area,
                                          cell_area, flags);
}


class ObjectsPanel::ModelColumns : public Gtk::TreeModel::ColumnRecord
{
public:
    ModelColumns()
    {
        add(_colObject);
        add(_colType);
        add(_colVisible);
        add(_colLocked);
        add(_colLabel);
        add(_colPrevSelectionState);
    }
    ~ModelColumns() override = default;
    Gtk::TreeModelColumn<SPItem*> _colObject;
    Gtk::TreeModelColumn<Glib::ustring> _colLabel;
    Gtk::TreeModelColumn<Glib::ustring> _colType;
    Gtk::TreeModelColumn<bool> _colVisible;
    Gtk::TreeModelColumn<bool> _colLocked;
    Gtk::TreeModelColumn<bool> _colPrevSelectionState;
};

/**
 * Gets an instance of the Objects panel
 */
ObjectsPanel& ObjectsPanel::getInstance()
{
    return *new ObjectsPanel();
}

// GtkTreeView Column enumeration
enum { COL_LABEL, COL_VISIBLE, COL_LOCKED };

/**
 * Button enumeration
 */
enum {
    BUTTON_NEW = 0,
    BUTTON_RENAME,
    BUTTON_TOP,
    BUTTON_BOTTOM,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_DUPLICATE,
    BUTTON_DELETE,
    BUTTON_SOLO,
    BUTTON_SHOW_ALL,
    BUTTON_HIDE_ALL,
    BUTTON_LOCK_OTHERS,
    BUTTON_LOCK_ALL,
    BUTTON_UNLOCK_ALL,
    BUTTON_SETCLIP,
    BUTTON_CLIPGROUP,
    BUTTON_UNSETCLIP,
    BUTTON_SETMASK,
    BUTTON_UNSETMASK,
    BUTTON_GROUP,
    BUTTON_UNGROUP,
    BUTTON_COLLAPSE_ALL,
    DRAGNDROP
};

class ObjectsPanel::ObjectWatcher : public Inkscape::XML::NodeObserver {
public:
    /**
     * Creates a new ObjectWatcher, a gtk TreeView interated watching device.
     *
     * @param panel The panel to which the object watcher belongs
     * @param obj The object to watch
     * @param iter The optional list store iter for the item, if not provided,
     *             assumes this is the root 'document' object.
     */
    ObjectWatcher(ObjectsPanel* panel, SPObject* obj, Gtk::TreeRow *row) :
        panel(panel),
        row_ref(nullptr),
        node(obj->getRepr()),
        _lockedAttr(g_quark_from_string("sodipodi:insensitive")),
        _labelAttr(g_quark_from_string("inkscape:label")),
        _groupAttr(g_quark_from_string("inkscape:groupmode")),
        _styleAttr(g_quark_from_string("style"))
    {
        if(row != nullptr) {
            auto path = panel->_store->get_path(*row);
            row_ref = new Gtk::TreeModel::RowReference(panel->_store, path);
            updateRowInfo();
        }
        node->addObserver(*this);
        for (auto& child: obj->children) {
            // Check the object is a visible SPItem
            if (dynamic_cast<SPItem *>(&child)) {
                addChild(&child);
            }
        }
    }

    ~ObjectWatcher() override {
        node->removeObserver(*this);
        if (row_ref) {
            auto path = row_ref->get_path();
            if (path) {
                auto iter = panel->_store->get_iter(path);
                if(iter) {
                    panel->_store->erase(iter);
                }
            }
            row_ref = nullptr;
        }
        child_watchers.clear();
    }


    /**
     * Update the information in the row from the stored node
     */
    void updateRowInfo() {
        auto item = dynamic_cast<SPItem *>(getObject(node));
        auto row = *panel->_store->get_iter(row_ref->get_path());

        if (item) {
            gchar const * label = item->label() ? item->label() : item->getId();
            row[panel->_model->_colObject] = item;
            row[panel->_model->_colLabel] = label ? label : item->defaultLabel();
            row[panel->_model->_colType] = item->typeName();
            row[panel->_model->_colVisible] = !item->isHidden();
            row[panel->_model->_colLocked] = !item->isSensitive();
        }
    }

    /**
     * Add the child object to this node.
     *
     * @param child - SPObject to be added
     */
    void addChild(SPObject *child)
    {
        Gtk::TreeModel::Row row = *(panel->_store->append(getParentIter()));
        child_watchers.emplace_back(new ObjectWatcher(panel, child, &row));
    }

    /**
     * Move the child to just after the given sibling
     *
     * @param child - SPObject to be moved
     * @param sibling - Optional sibling Object to add next to, if nullptr the
     *                  object is moved to BEFORE the first item.
     */
    void moveChild(SPObject *child, SPObject *sibling)
    {
        auto child_iter = getChildIter(child);
        auto sibling_iter = getChildIter(sibling);
        auto child_const = const_cast<GtkTreeIter*>(child_iter->get_gobject_if_not_end());
        GtkTreeIter* sibling_const = nullptr;
        if (sibling_iter) {
            sibling_const = const_cast<GtkTreeIter*>(sibling_iter->get_gobject_if_not_end());
        }
        // Gtkmm can move to before, but not move to 'after'
        gtk_tree_store_move_after(panel->_store->gobj(), child_const, sibling_const);
    }

    /**
     * Get the parent TreeRow's children iterator to this object
     *
     * @returns Gtk Tree Node Children iterator
     */
    Gtk::TreeNodeChildren getParentIter()
    {
        if (row_ref) {
            const Gtk::TreeRow row = **panel->_store->get_iter(row_ref->get_path());
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
    const Gtk::TreeRow* getChildIter(SPObject *child)
    {
        for (auto &iter : getParentIter()) {
            Gtk::TreeModel::Row row = *iter;
            if(row[panel->_model->_colObject] == child) {
                return &(*iter);
            }
        }
        return nullptr;
    }
    /**
      * Get the object from the node.
      *
      * @param node - XML Node involved in the signal.
      * @returns SPObject matching the node, returns nullptr if not found.
      */
    SPObject *getObject(Node *node) {
        if (node != nullptr)
            return panel->_document->getObjectByRepr(node);
        return nullptr;
    }

    void notifyChildAdded( Node &node, Node &child, Node *prev ) override
    {
        addChild(getObject(&child));
        moveChild(getObject(&child), getObject(prev));
    }
    void notifyChildRemoved( Node &/*node*/, Node &child, Node* /*prev*/ ) override
    {
        auto iter = child_watchers.begin();
        while (iter != child_watchers.end()) {
            // child doesn't have a valid SPObject, so comparing Nodes
            auto node1 = (*iter)->node;
            auto node2 = &child;
            if (!node1 || !node2) continue;
            if(node1 == node2) {
                // It's VERY important this is done so ghost attribute signals are
                // stopped before they are emitted and cause crashes (object is gone)
                child_watchers.erase(iter);
            } else {
                iter++;
            }
        }
    }
    void notifyChildOrderChanged( Node &parent, Node &child, Node */*old_prev*/, Node *new_prev ) override
    {
        moveChild(getObject(&child), getObject(new_prev));
    }
    void notifyContentChanged( Node &/*node*/, Util::ptr_shared /*old_content*/, Util::ptr_shared /*new_content*/ ) override {}
    void notifyAttributeChanged( Node &node, GQuark name, Util::ptr_shared /*old_value*/, Util::ptr_shared /*new_value*/ ) override {
        if ( name == _lockedAttr || name == _labelAttr || name == _groupAttr || name == _styleAttr ) {
            updateRowInfo();
        }
    }

    std::vector<std::unique_ptr<ObjectWatcher>> child_watchers;
    Gtk::TreeModel::RowReference* row_ref;
    ObjectsPanel* panel;
    Node* node;
    
    /* These are quarks which define the attributes that we are observing */
    GQuark _lockedAttr;
    GQuark _labelAttr;
    GQuark _groupAttr;
    GQuark _styleAttr;
};

class ObjectsPanel::InternalUIBounce
{
public:
    int _actionCode;
    sigc::connection _signal;
};

/**
 * Stylizes a button using the given icon name and tooltip
 */
void ObjectsPanel::_styleButton(Gtk::Button& btn, char const* iconName, char const* tooltip)
{
    auto child = Glib::wrap(sp_get_icon_image(iconName, GTK_ICON_SIZE_SMALL_TOOLBAR));
    child->show();
    btn.add(*child);
    btn.set_relief(Gtk::RELIEF_NONE);
    btn.set_tooltip_text(tooltip);
}

/**
 * Adds an item to the pop-up (right-click) menu
 * @param desktop The active destktop
 * @param code Action code
 * @param id Button id for callback function
 * @return The generated menu item
 */
Gtk::MenuItem& ObjectsPanel::_addPopupItem( SPDesktop *desktop, unsigned int code, int id )
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

    item->signal_activate().connect(sigc::bind(sigc::mem_fun(*this, &ObjectsPanel::_takeAction), id));
    _popupMenu.append(*item);

    return *item;
}

/**
 * Occurs when the current desktop selection changes
 * @param sel The current selection
 */
void ObjectsPanel::_objectsSelected( Selection *sel ) {

    _selectedConnection.block();

    _tree.get_selection()->unselect_all();
    _store->foreach_iter(sigc::mem_fun(*this, &ObjectsPanel::_clearPrevSelectionState));

    SPItem *item = nullptr;
    auto items = sel->items();
    for(auto i=items.begin(); i!=items.end(); ++i){
        item = *i;
        _updateObjectSelected(item, (*i)==items.back(), false);
    }
    if (!item) {
        if (_desktop->currentLayer() && SP_IS_ITEM(_desktop->currentLayer())) {
            item = SP_ITEM(_desktop->currentLayer());
            _updateObjectSelected(item, false, true);
        }
    }
    _selectedConnection.unblock();
    _checkTreeSelection();
}

/**
 * Find the specified item in the tree store and (de)select it, optionally scrolling to the item
 * @param item Item to select in the tree
 * @param scrollto Whether to scroll to the item
 * @param expand If true, the path in the tree towards item will be expanded
 */
void ObjectsPanel::_updateObjectSelected(SPItem* item, bool scrollto, bool expand)
{
    /*Gtk::TreeModel::iterator tree_iter;
    if (_findInTreeCache(item, tree_iter)) {
        Gtk::TreeModel::Row row = *tree_iter;

        //We found the item! Expand to the path and select it in the tree.
        Gtk::TreePath path = _store->get_path(tree_iter);
        _tree.expand_to_path( path );
        if (!expand)
            // but don't expand itself, just the path
            _tree.collapse_row(path);

        Glib::RefPtr<Gtk::TreeSelection> select = _tree.get_selection();

        select->select(tree_iter);
        row[_model->_colPrevSelectionState] = true;
        if (scrollto) {
            //Scroll to the item in the tree
            _tree.scroll_to_row(path, 0.5);
        }
    }*/
}

/**
 * Pushes the current tree selection to the canvas
 */
void ObjectsPanel::_pushTreeSelectionToCurrent()
{
    if (auto selection = getSelection()) {
        //Clear the selection and then iterate over the tree selection, pushing each item to the desktop
        selection->clear();
        if (_tree.get_selection()->count_selected_rows() == 0) {
            _store->foreach_iter(sigc::mem_fun(*this, &ObjectsPanel::_clearPrevSelectionState));
        }
        bool setOpacity = true;
        bool first_pass = true;
        _store->foreach_iter(sigc::bind<bool *>(sigc::mem_fun(*this, &ObjectsPanel::_selectItemCallback), &setOpacity, &first_pass));
        first_pass = false;
        _store->foreach_iter(sigc::bind<bool *>(sigc::mem_fun(*this, &ObjectsPanel::_selectItemCallback), &setOpacity, &first_pass));

        _checkTreeSelection();
    }
}

/**
 * Helper function for pushing the current tree selection to the current desktop
 * @param iter Current tree item
 * @param setCompositingValues Whether to set the compositing values
 */
bool ObjectsPanel::_selectItemCallback(const Gtk::TreeModel::iterator& iter, bool *setCompositingValues, bool *first_pass)
{
    auto desktop = getDesktop();
    if (!desktop)
        return false;
    Gtk::TreeModel::Row row = *iter;
    bool selected = _tree.get_selection()->is_selected(iter);
    if (selected) { // All items selected in the treeview will be added to the current selection
        /* Adding/removing only the items that were selected or deselected since the previous call to _pushTreeSelectionToCurrent()
         * is very slow on large documents, because desktop->selection->remove(item) needs to traverse the whole ObjectSet to find
         * the item to be removed. When all N objects are selected in a document, clearing the whole selection would require O(N^2)
         * That's why we simply clear the complete selection using desktop->selection->clear(), and re-add all items one by one.
         * This is much faster.
         */

        /* On the first pass, we will add only the items that were selected before too. Then, on the second pass, we will add the
         * newly selected items such that the last selected items will be actually last. This is needed for example when the user
         * wants to align relative to the last selected item.
         */
        if (*first_pass == row[_model->_colPrevSelectionState]) {
            SPItem *item = row[_model->_colObject];
            //If the item is not a layer, then select it and set the current layer to its parent (if it's the first item)
            if (_desktop->selection->isEmpty()) {
                _desktop->setCurrentLayer(item->parent);
            }
            _desktop->selection->add(item);

            if (SP_IS_GROUP(item) && SP_GROUP(item)->layerMode() != SPGroup::LAYER) {
                //If the item is a layer, set the current layer
                if (desktop->selection->isEmpty()) {
                    desktop->setCurrentLayer(item);
                }
            }
        }
    }

    if (not *first_pass) {
        row[_model->_colPrevSelectionState] = selected;
    }

    return false;
}

bool ObjectsPanel::_clearPrevSelectionState( const Gtk::TreeModel::iterator& iter) {
    Gtk::TreeModel::Row row = *iter;
    row[_model->_colPrevSelectionState] = false;
    return false;
}

/**
 * Handles button sensitivity
 */
void ObjectsPanel::_checkTreeSelection()
{
    bool sensitive = _tree.get_selection()->count_selected_rows() > 0;
    //TODO: top/bottom sensitivity
    bool sensitiveNonTop = true;
    bool sensitiveNonBottom = true;

    for (auto & it : _watching) {
        it->set_sensitive( sensitive );
    }
    for (auto & it : _watchingNonTop) {
        it->set_sensitive( sensitiveNonTop );
    }
    for (auto & it : _watchingNonBottom) {
        it->set_sensitive( sensitiveNonBottom );
    }

    _tree.set_reorderable(sensitive); // Reorderable means that we allow drag-and-drop, but we only allow that when at least one row is selected
}

/**
 * Sets visibility of items in the tree
 * @param iter Current item in the tree
 * @param visible Whether the item should be visible or not
 */
void ObjectsPanel::_setVisibleIter( const Gtk::TreeModel::iterator& iter, const bool visible )
{
    Gtk::TreeModel::Row row = *iter;
    SPItem* item = row[_model->_colObject];
    if (item)
    {
        item->setHidden( !visible );
        row[_model->_colVisible] = visible;
        item->updateRepr(SP_OBJECT_WRITE_NO_CHILDREN | SP_OBJECT_WRITE_EXT);
    }
}

/**
 * Sets sensitivity of items in the tree
 * @param iter Current item in the tree
 * @param locked Whether the item should be locked
 */
void ObjectsPanel::_setLockedIter( const Gtk::TreeModel::iterator& iter, const bool locked )
{
    Gtk::TreeModel::Row row = *iter;
    SPItem* item = row[_model->_colObject];
    if (item)
    {
        item->setLocked( locked );
        row[_model->_colLocked] = locked;
        item->updateRepr(SP_OBJECT_WRITE_NO_CHILDREN | SP_OBJECT_WRITE_EXT);
    }
}

/**
 * Handles keyboard events
 * @param event Keyboard event passed in from GDK
 * @return Whether the event should be eaten (om nom nom)
 */
bool ObjectsPanel::_handleKeyEvent(GdkEventKey *event)
{
    auto desktop = getDesktop();
    if (!desktop)
        return false;

    Gtk::AccelKey shortcut = Inkscape::Shortcuts::get_from_event(event);

    switch (shortcut.get_key()) {
        // how to get users key binding for the action “start-interactive-search” ??
        // ctrl+f is just the default
        case GDK_KEY_f:
            if (shortcut.get_mod() | Gdk::CONTROL_MASK) return false;
            break;
        // shall we slurp ctrl+w to close panel?

        // defocus:
        case GDK_KEY_Escape:
            if (desktop->canvas) {
                desktop->canvas->grab_focus();
                return true;
            }
            break;
    }

    // invoke user defined shortcuts first
    bool done = Inkscape::Shortcuts::getInstance().invoke_verb(event, desktop);
    if (done) {
        return true;
    }

    // handle events for the treeview
    //  bool empty = desktop->selection->isEmpty();

    switch (Inkscape::UI::Tools::get_latin_keyval(event)) {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        {
            Gtk::TreeModel::Path path;
            Gtk::TreeViewColumn *focus_column = nullptr;

            _tree.get_cursor(path, focus_column);
            if (focus_column == _name_column && !_text_renderer->property_editable()) {
                //Rename item
                _text_renderer->property_editable() = true;
                _tree.set_cursor(path, *_name_column, true);
                grab_focus();
                return true;
            }
            return false;
            break;
        }
    }
    return false;
}

/**
 * Handles mouse events
 * @param event Mouse event from GDK
 * @return whether to eat the event (om nom nom)
 */
bool ObjectsPanel::_handleButtonEvent(GdkEventButton* event)
{
    static unsigned doubleclick = 0;
    static bool overVisible = false;
    Gtk::TreeModel::Path _defer_target;

    //Right mouse button was clicked, launch the pop-up menu
    if ( (event->type == GDK_BUTTON_PRESS) && (event->button == 3) ) {
        Gtk::TreeModel::Path path;
        int x = static_cast<int>(event->x);
        int y = static_cast<int>(event->y);
        if ( _tree.get_path_at_pos( x, y, path ) ) {
            _checkTreeSelection();
            _popupMenu.popup_at_pointer(reinterpret_cast<GdkEvent *>(event));

            if (_tree.get_selection()->is_selected(path)) {
                return true;
            }
        }
    }

    //Left mouse button was pressed!  In order to handle multiple item drag & drop,
    //we need to defer selection by setting the select function so that the tree doesn't
    //automatically select anything.  In order to handle multiple item icon clicking,
    //we need to eat the event.  There might be a better way to do both of these...
    if ( (event->type == GDK_BUTTON_PRESS) && (event->button == 1)) {
        overVisible = false;
        Gtk::TreeModel::Path path;
        Gtk::TreeViewColumn* col = nullptr;
        int x = static_cast<int>(event->x);
        int y = static_cast<int>(event->y);
        int x2 = 0;
        int y2 = 0;
        if ( _tree.get_path_at_pos( x, y, path, col, x2, y2 ) ) {
            if (col == _tree.get_column(COL_VISIBLE)) {
                //Click on visible column, eat this event to keep row selection
                overVisible = true;
                return true;
            } else if (col == _tree.get_column(COL_LOCKED)) {
                //Click on an icon column, eat this event to keep row selection
                return true;
            } else if ( !(event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) & _tree.get_selection()->is_selected(path) ) {
                //Click on a selected item with no modifiers, defer selection to the mouse-up by
                //setting the select function to _noSelection
                _tree.get_selection()->set_select_function(sigc::mem_fun(*this, &ObjectsPanel::_noSelection));
                _defer_target = path;
            }
        }
    }

    //Restore the selection function to allow tree selection on mouse button release
    if ( event->type == GDK_BUTTON_RELEASE) {
        _tree.get_selection()->set_select_function(sigc::mem_fun(*this, &ObjectsPanel::_rowSelectFunction));
    }
    
    //CellRenderers do not have good support for dealing with multiple items, so
    //we handle all events on them here
    if ( (event->type == GDK_BUTTON_RELEASE) && (event->button == 1)) {

        Gtk::TreeModel::Path path;
        Gtk::TreeViewColumn* col = nullptr;
        int x = static_cast<int>(event->x);
        int y = static_cast<int>(event->y);
        int x2 = 0;
        int y2 = 0;
        if ( _tree.get_path_at_pos( x, y, path, col, x2, y2 ) ) {
            if (_defer_target) {
                //We had deferred a selection target, select it here (assuming no drag & drop)
                if (_defer_target == path && !(event->x == 0 && event->y == 0))
                {
                    _tree.set_cursor(path, *col, false);
                }
                _defer_target = Gtk::TreeModel::Path();
            }
            else {
                if (event->state & GDK_SHIFT_MASK) {
                    // Shift left click on the visible/lock columns toggles "solo" mode
                    if (col == _tree.get_column(COL_VISIBLE)) {
                        _takeAction(BUTTON_SOLO);
                    } else if (col == _tree.get_column(COL_LOCKED)) {
                        _takeAction(BUTTON_LOCK_OTHERS);
                    }
                } else if (event->state & GDK_MOD1_MASK) {
                    // Alt+left click on the visible/lock columns toggles "solo" mode and preserves selection
                    Gtk::TreeModel::iterator iter = _store->get_iter(path);
                    if (_store->iter_is_valid(iter)) {
                        Gtk::TreeModel::Row row = *iter;
                        SPItem *item = row[_model->_colObject];
                        if (col == _tree.get_column(COL_VISIBLE)) {
                            _desktop->toggleLayerSolo( item );
                            DocumentUndo::maybeDone(_desktop->doc(), "layer:solo", SP_VERB_LAYER_SOLO, _("Toggle layer solo"));
                        } else if (col == _tree.get_column(COL_LOCKED)) {
                            _desktop->toggleLockOtherLayers( item );
                            DocumentUndo::maybeDone(_desktop->doc(), "layer:lockothers", SP_VERB_LAYER_LOCK_OTHERS, _("Lock other layers"));
                        }
                    }
                } else {
                    Gtk::TreeModel::Children::iterator iter = _tree.get_model()->get_iter(path);
                    Gtk::TreeModel::Row row = *iter;

                    SPItem* item = row[_model->_colObject];

                    if (col == _tree.get_column(COL_VISIBLE)) {
                        if (overVisible) {
                            //Toggle visibility
                            bool newValue = !row[_model->_colVisible];
                            if (_tree.get_selection()->is_selected(path))
                            {
                                //If the current row is selected, toggle the visibility
                                //for all selected items
                                _tree.get_selection()->selected_foreach_iter(sigc::bind<bool>(sigc::mem_fun(*this, &ObjectsPanel::_setVisibleIter), newValue));
                            }
                            else
                            {
                                //If the current row is not selected, toggle just its visibility
                                row[_model->_colVisible] = newValue;
                                item->setHidden(!newValue);
                                item->updateRepr(SP_OBJECT_WRITE_NO_CHILDREN | SP_OBJECT_WRITE_EXT);
                            }
                            DocumentUndo::done( desktop->doc() , SP_VERB_DIALOG_OBJECTS,
                                            newValue? _("Unhide objects") : _("Hide objects"));
                            overVisible = false;
                        }
                    } else if (col == _tree.get_column(COL_LOCKED)) {
                        //Toggle locking
                        bool newValue = !row[_model->_colLocked];
                        if (_tree.get_selection()->is_selected(path))
                        {
                            //If the current row is selected, toggle the sensitivity for
                            //all selected items
                            _tree.get_selection()->selected_foreach_iter(sigc::bind<bool>(sigc::mem_fun(*this, &ObjectsPanel::_setLockedIter), newValue));
                        }
                        else
                        {
                            //If the current row is not selected, toggle just its sensitivity
                            row[_model->_colLocked] = newValue;
                            item->setLocked( newValue );
                            item->updateRepr(SP_OBJECT_WRITE_NO_CHILDREN | SP_OBJECT_WRITE_EXT);
                        }
                        DocumentUndo::done( desktop->doc() , SP_VERB_DIALOG_OBJECTS,
                                            newValue? _("Lock objects") : _("Unlock objects"));
                    }
                }
            }
        }
    }

    //Second mouse button press, set double click status for when the mouse is released
    if ( (event->type == GDK_2BUTTON_PRESS) && (event->button == 1) ) {
        doubleclick = 1;
    }

    //Double click on mouse button release, if we're over the label column, edit
    //the item name
    if ( event->type == GDK_BUTTON_RELEASE && doubleclick) {
        doubleclick = 0;
        Gtk::TreeModel::Path path;
        Gtk::TreeViewColumn* col = nullptr;
        int x = static_cast<int>(event->x);
        int y = static_cast<int>(event->y);
        int x2 = 0;
        int y2 = 0;
        if ( _tree.get_path_at_pos( x, y, path, col, x2, y2 ) && col == _name_column) {
            // Double click on the Layer name, enable editing
            _text_renderer->property_editable() = true;
            _tree.set_cursor (path, *_name_column, true);
            grab_focus();
        }
    }
   
    return false;
}

/*
 * Drag and drop within the tree
 */
bool ObjectsPanel::_handleDragDrop(const Glib::RefPtr<Gdk::DragContext>& /*context*/, int x, int y, guint /*time*/)
{
    //Set up our defaults and clear the source vector
    _dnd_into = false;
    _dnd_target = nullptr;
    _dnd_source.clear();
    _dnd_source_includes_layer = false;

    //Add all selected items to the source vector
    _tree.get_selection()->selected_foreach_iter(sigc::mem_fun(*this, &ObjectsPanel::_storeDragSource));

    bool cancel_dnd = false;
    bool dnd_to_top_at_end = false;

    Gtk::TreeModel::Path target_path;
    Gtk::TreeViewDropPosition pos;
    if (_tree.get_dest_row_at_pos(x, y, target_path, pos)) {
        // SPItem::moveTo() will be used to move the selected items to their new position, but
        // moveTo() can only "drop before"; we therefore need to find the next path and drop
        // the selection just before it, instead of "dropping after" the target path
        if (pos == Gtk::TREE_VIEW_DROP_AFTER) {
            Gtk::TreeModel::Path next_path = target_path;
            if (_tree.row_expanded(next_path)) {
                next_path.down(); // The next path is at a lower level in the hierarchy, i.e. in a layer or group
            } else {
                next_path.next(); // The next path is at the same level
            }
            // A next path might however not be present, if we're dropping at the end of the tree view
            if (_store->iter_is_valid(_store->get_iter(next_path))) {
                target_path = next_path;
            } else {
                // Dragging to the "end" of the treeview ; we'll get the parent group or layer of the last
                // item, and drop into that parent
                Gtk::TreeModel::Path up_path = target_path;
                up_path.up();
                if (_store->iter_is_valid(_store->get_iter(up_path))) {
                    // Drop into the parent of the last item
                    target_path = up_path;
                    _dnd_into = true;
                } else {
                    // Drop into the top level, completely at the end of the treeview;
                    dnd_to_top_at_end = true;
                }
            }
        }

        if (dnd_to_top_at_end) {
            g_assert(_dnd_target == nullptr);
        } else {
            // Find the SPItem corresponding to the target_path/row at which we're dropping our selection
            Gtk::TreeModel::iterator iter = _store->get_iter(target_path);
            if (_store->iter_is_valid(iter)) {
                Gtk::TreeModel::Row row = *iter;
                _dnd_target = row[_model->_colObject]; //Set the drop target
                if ((pos == Gtk::TREE_VIEW_DROP_INTO_OR_BEFORE) || (pos == Gtk::TREE_VIEW_DROP_INTO_OR_AFTER)) {
                    // Trying to drop into a layer or group
                    if (SP_IS_GROUP(_dnd_target)) {
                        _dnd_into = true;
                    } else {
                        // If the target is not a group (or layer), then we cannot drop into it (unless we
                        // would create a group on the fly), so we will cancel the drag and drop action.
                        cancel_dnd = true;
                    }
                }
                // If the source selection contains a layer however, then it can not be dropped ...
                bool c1 = target_path.size() > 1;                   // .. below the top-level
                bool c2 = SP_IS_GROUP(_dnd_target) && _dnd_into;   // .. or in any group (at the top level)
                if (SP_IS_GROUP(_dnd_target) && SP_GROUP(_dnd_target)->layerMode() != SPGroup::LAYER && 
                    _dnd_source_includes_layer && 
                    (c1 || c2)) 
                {
                    cancel_dnd = true;
                }
            } else {
                cancel_dnd = true;
            }
        }
    }

    if (not cancel_dnd) {
        _takeAction(DRAGNDROP);
    }

    return true; // If True: then we're signaling here that nothing needs to be done by the TreeView; we're updating ourselves..
}

/**
 * Stores all selected items as the drag source
 * @param iter Current tree item
 */
void ObjectsPanel::_storeDragSource(const Gtk::TreeModel::iterator& iter)
{
    Gtk::TreeModel::Row row = *iter;
    SPItem* item = row[_model->_colObject];
    if (item) {
        _dnd_source.push_back(item);
        if (SP_IS_GROUP(item) && (SP_GROUP(item)->layerMode() == SPGroup::LAYER)) {
            _dnd_source_includes_layer = true;
        }
    }
}

/*
 * Move a selection of items in response to a drag & drop action
 */
void ObjectsPanel::_doTreeMove()
{
    auto desktop = getDesktop();
    auto document = getDocument();
    if (!document)
        return;

    std::vector<gchar *> idvector;

    //Clear the desktop selection
    desktop->selection->clear();
    while (!_dnd_source.empty())
    {
        SPItem *obj = _dnd_source.back();
        _dnd_source.pop_back();

        if (obj != _dnd_target) {
            //Store the object id (for selection later) and move the object
            idvector.push_back(g_strdup(obj->getId()));
            obj->moveTo(_dnd_target, _dnd_into);
        }
    }
    //Select items
    while (!idvector.empty()) {
        //Grab the id from the vector, get the item in the document and select it
        gchar * id = idvector.back();
        idvector.pop_back();
        SPObject *obj = document->getObjectById(id);
        g_free(id);
        if (obj && SP_IS_ITEM(obj)) {
            SPItem *item = SP_ITEM(obj);
            if (!SP_IS_GROUP(item) || SP_GROUP(item)->layerMode() != SPGroup::LAYER)
            {
                if (desktop->selection->isEmpty()) desktop->setCurrentLayer(item->parent);
                desktop->selection->add(item);
            }
            else
            {
                if (desktop->selection->isEmpty()) desktop->setCurrentLayer(item);
            }
        }
    }

    DocumentUndo::done(document, SP_VERB_NONE, _("Moved objects"));
}

/**
 * Fires the action verb
 */
void ObjectsPanel::_fireAction( unsigned int code )
{
    if (auto desktop = getDesktop()) {
        Verb *verb = Verb::get( code );
        if ( verb ) {
            SPAction *action = verb->get_action(desktop);
            if ( action ) {
                sp_action_perform( action, nullptr );
            }
        }
    }
}

/**
 * Executes the given button action during the idle time
 */
void ObjectsPanel::_takeAction( int val )
{
    _pending = new InternalUIBounce();
    _pending->_actionCode = val;
    _pending->_signal = Glib::signal_timeout().connect( sigc::mem_fun(*this, &ObjectsPanel::_executeAction), 0 );
}

/**
 * Executes the pending button action
 */
bool ObjectsPanel::_executeAction()
{
    // Make sure selected layer hasn't changed since the action was triggered
    if ( _document && _pending) 
    {
        int val = _pending->_actionCode;

        auto selection = getSelection();
        int val = _pending->_actionCode;
        switch ( val ) {
            case BUTTON_NEW:
            {
                _fireAction( SP_VERB_LAYER_NEW );
            }
            break;
            case BUTTON_RENAME:
            {
                _fireAction( SP_VERB_LAYER_RENAME );
            }
            break;
            case BUTTON_TOP:
            {
                if (selection->isEmpty()) {
                    _fireAction( SP_VERB_LAYER_TO_TOP );
                } else {
                    _fireAction( SP_VERB_SELECTION_TO_FRONT);
                }
            }
            break;
            case BUTTON_BOTTOM:
            {
                if (selection->isEmpty()) {
                    _fireAction( SP_VERB_LAYER_TO_BOTTOM );
                } else {
                    _fireAction( SP_VERB_SELECTION_TO_BACK);
                }
            }
            break;
            case BUTTON_UP:
            {
                if (selection->isEmpty()) {
                    _fireAction( SP_VERB_LAYER_RAISE );
                } else {
                    _fireAction( SP_VERB_SELECTION_STACK_UP );
                }
            }
            break;
            case BUTTON_DOWN:
            {
                if (selection->isEmpty()) {
                    _fireAction( SP_VERB_LAYER_LOWER );
                } else {
                    _fireAction( SP_VERB_SELECTION_STACK_DOWN );
                }
            }
            break;
            case BUTTON_DUPLICATE:
            {
                if (selection->isEmpty()) {
                    _fireAction( SP_VERB_LAYER_DUPLICATE );
                } else {
                    _fireAction( SP_VERB_EDIT_DUPLICATE );
                }
            }
            break;
            case BUTTON_DELETE:
            {
                if (selection->isEmpty()) {
                    _fireAction( SP_VERB_LAYER_DELETE );
                } else {
                    _fireAction( SP_VERB_EDIT_DELETE );
                }
            }
            break;
            case BUTTON_SOLO:
            {
                _fireAction( SP_VERB_LAYER_SOLO );
            }
            break;
            case BUTTON_SHOW_ALL:
            {
                _fireAction( SP_VERB_LAYER_SHOW_ALL );
            }
            break;
            case BUTTON_HIDE_ALL:
            {
                _fireAction( SP_VERB_LAYER_HIDE_ALL );
            }
            break;
            case BUTTON_LOCK_OTHERS:
            {
                _fireAction( SP_VERB_LAYER_LOCK_OTHERS );
            }
            break;
            case BUTTON_LOCK_ALL:
            {
                _fireAction( SP_VERB_LAYER_LOCK_ALL );
            }
            break;
            case BUTTON_UNLOCK_ALL:
            {
                _fireAction( SP_VERB_LAYER_UNLOCK_ALL );
            }
            break;
            case BUTTON_CLIPGROUP:
            {
               _fireAction ( SP_VERB_OBJECT_CREATE_CLIP_GROUP );
            }
            case BUTTON_SETCLIP:
            {
                _fireAction( SP_VERB_OBJECT_SET_CLIPPATH );
            }
            break;
            case BUTTON_UNSETCLIP:
            {
                _fireAction( SP_VERB_OBJECT_UNSET_CLIPPATH );
            }
            break;
            case BUTTON_SETMASK:
            {
                _fireAction( SP_VERB_OBJECT_SET_MASK );
            }
            break;
            case BUTTON_UNSETMASK:
            {
                _fireAction( SP_VERB_OBJECT_UNSET_MASK );
            }
            break;
            case BUTTON_GROUP:
            {
                _fireAction( SP_VERB_SELECTION_GROUP );
            }
            break;
            case BUTTON_UNGROUP:
            {
                _fireAction( SP_VERB_SELECTION_UNGROUP );
            }
            break;
            case BUTTON_COLLAPSE_ALL:
            {
                for (auto& obj: document->getRoot()->children) {
                    if (SP_IS_GROUP(&obj)) {
                        _setCollapsed(SP_GROUP(&obj));
                    }
                }
            }
            break;
            case DRAGNDROP:
            {
                _doTreeMove( );
                // The notifyChildOrderChanged signal will ensure that the TreeView gets updated
            }
            break;
        }

        delete _pending;
        _pending = nullptr;
    }

    return false;
}

/**
 * Handles an unsuccessful item label edit (escape pressed, etc.)
 */
void ObjectsPanel::_handleEditingCancelled()
{
    _text_renderer->property_editable() = false;
}

/**
 * Handle a successful item label edit
 * @param path Tree path of the item currently being edited
 * @param new_text New label text
 */
void ObjectsPanel::_handleEdited(const Glib::ustring& path, const Glib::ustring& new_text)
{
    Gtk::TreeModel::iterator iter = _tree.get_model()->get_iter(path);
    Gtk::TreeModel::Row row = *iter;

    _renameObject(row, new_text);
    _text_renderer->property_editable() = false;
}

/**
 * Renames an item in the tree
 * @param row Tree row
 * @param name New label to give to the item
 */
void ObjectsPanel::_renameObject(Gtk::TreeModel::Row row, const Glib::ustring& name)
{
    auto document = getDocument();
    if (row && document) {
        SPItem* item = row[_model->_colObject];
        if (item) {
            gchar const* oldLabel = item->label();
            if ( !name.empty() && (!oldLabel || name != oldLabel) ) {
                item->setLabel(name.c_str());
                DocumentUndo::done(document, SP_VERB_NONE,
                                                    _("Rename object"));
            }
        }
    }
}

/**
 * A row selection function used by the tree that doesn't allow any new items to be selected.
 * Currently, this is used to allow multi-item drag & drop.
 */
bool ObjectsPanel::_noSelection( Glib::RefPtr<Gtk::TreeModel> const & /*model*/, Gtk::TreeModel::Path const & /*path*/, bool /*currentlySelected*/ )
{
    return false;
}

/**
 * Default row selection function taken from the layers dialog
 */
bool ObjectsPanel::_rowSelectFunction( Glib::RefPtr<Gtk::TreeModel> const & /*model*/, Gtk::TreeModel::Path const & /*path*/, bool currentlySelected )
{
    bool val = true;
    if ( !currentlySelected && _toggleEvent )
    {
        GdkEvent* event = gtk_get_current_event();
        if ( event ) {
            // (keep these checks separate, so we know when to call gdk_event_free()
            if ( event->type == GDK_BUTTON_PRESS ) {
                GdkEventButton const* target = reinterpret_cast<GdkEventButton const*>(_toggleEvent);
                GdkEventButton const* evtb = reinterpret_cast<GdkEventButton const*>(event);

                if ( (evtb->window == target->window)
                     && (evtb->send_event == target->send_event)
                     && (evtb->time == target->time)
                     && (evtb->state == target->state)
                    )
                {
                    // Ooooh! It's a magic one
                    val = false;
                }
            }
            gdk_event_free(event);
        }
    }
    return val;
}

/**
 * Sets a group to be collapsed and recursively collapses its children
 * @param group The group to collapse
 */
void ObjectsPanel::_setCollapsed(SPGroup * group)
{
    group->setExpanded(false);
    group->updateRepr(SP_OBJECT_WRITE_NO_CHILDREN | SP_OBJECT_WRITE_EXT);
    for (auto& iter: group->children) {
        if (SP_IS_GROUP(&iter)) {
            _setCollapsed(SP_GROUP(&iter));
        }
    }
}

/**
 * Sets a group to be expanded or collapsed
 * @param iter Current tree item
 * @param isexpanded Whether to expand or collapse
 */
void ObjectsPanel::_setExpanded(const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& /*path*/, bool isexpanded)
{
    Gtk::TreeModel::Row row = *iter;

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
    }
}

/**
 * Constructor
 */
ObjectsPanel::ObjectsPanel() :
    UI::Widget::Panel("/dialogs/objects", SP_VERB_DIALOG_OBJECTS),
    _rootWatcher(nullptr),
    _model(nullptr),
    _pending(nullptr),
    _pending_update(false),
    _toggleEvent(nullptr),
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
    _tree.set_reorderable(false);
    _tree.enable_model_drag_dest (Gdk::ACTION_MOVE);

    //Label
    auto name_column = Gtk::manage(new Gtk::TreeViewColumn());
    _text_renderer = Gtk::manage(new Gtk::CellRendererText());
    _icon_renderer = Gtk::manage(new CellRendererItemIcon());
    _icon_renderer->property_xpad() = 2;
    _icon_renderer->property_width() = 24;
    _tree.append_column(*name_column);
    name_column->set_expand(true);
    name_column->pack_start(*_icon_renderer, false);
    name_column->pack_start(*_text_renderer, true);
    name_column->add_attribute(_text_renderer->property_text(), _model->_colLabel);
    name_column->add_attribute(_icon_renderer->property_shape_type(), _model->_colType);

    //Visible
    auto *eyeRenderer = Gtk::manage( new Inkscape::UI::Widget::ImageToggler(INKSCAPE_ICON("object-visible"), INKSCAPE_ICON("object-hidden")) );

    int visibleColNum = _tree.append_column("vis", *eyeRenderer) - 1;
    eyeRenderer->property_activatable() = true;
    Gtk::TreeViewColumn* col = _tree.get_column(visibleColNum);
    if ( col ) {
        col->add_attribute( eyeRenderer->property_active(), _model->_colVisible );
    }

    //Locked
    Inkscape::UI::Widget::ImageToggler * renderer = Gtk::manage( new Inkscape::UI::Widget::ImageToggler(
        INKSCAPE_ICON("object-locked"), INKSCAPE_ICON("object-unlocked")) );
    int lockedColNum = _tree.append_column("lock", *renderer) - 1;
    renderer->property_activatable() = true;
    col = _tree.get_column(lockedColNum);
    if ( col ) {
        col->add_attribute( renderer->property_active(), _model->_colLocked );
    }
    
    //Set the expander and search columns
    _tree.set_expander_column( *name_column );
    _tree.set_search_column(_model->_colLabel);
    // use ctrl+f to start search
    _tree.set_enable_search(false);

    //Set up the tree selection
    _tree.get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
    _selectedConnection = _tree.get_selection()->signal_changed().connect( sigc::mem_fun(*this, &ObjectsPanel::_pushTreeSelectionToCurrent) );
    _tree.get_selection()->set_select_function( sigc::mem_fun(*this, &ObjectsPanel::_rowSelectFunction) );

    //Set up tree signals
    _tree.signal_button_press_event().connect( sigc::mem_fun(*this, &ObjectsPanel::_handleButtonEvent), false );
    _tree.signal_button_release_event().connect( sigc::mem_fun(*this, &ObjectsPanel::_handleButtonEvent), false );
    _tree.signal_key_press_event().connect( sigc::mem_fun(*this, &ObjectsPanel::_handleKeyEvent), false );
    _tree.signal_drag_drop().connect( sigc::mem_fun(*this, &ObjectsPanel::_handleDragDrop), false);
    _tree.signal_row_collapsed().connect( sigc::bind<bool>(sigc::mem_fun(*this, &ObjectsPanel::_setExpanded), false));
    _tree.signal_row_expanded().connect( sigc::bind<bool>(sigc::mem_fun(*this, &ObjectsPanel::_setExpanded), true));

    //Set up the label editing signals
    _text_renderer->signal_edited().connect( sigc::mem_fun(*this, &ObjectsPanel::_handleEdited) );
    _text_renderer->signal_editing_canceled().connect( sigc::mem_fun(*this, &ObjectsPanel::_handleEditingCancelled) );

    //Set up the scroller window and pack the page
    _scroller.add( _tree );
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

    //Set up the compositing items
    _page.pack_end(_buttonsRow, Gtk::PACK_SHRINK);

    //Pack into the panel contents
    _getContents()->pack_start(_page, Gtk::PACK_EXPAND_WIDGET);

    SPDesktop* targetDesktop = getDesktop();

    //Set up the button row


    //Add object/layer
    Gtk::Button* btn = Gtk::manage( new Gtk::Button() );
    _styleButton(*btn, INKSCAPE_ICON("list-add"), _("Add layer..."));
    btn->set_relief(Gtk::RELIEF_NONE);
    btn->signal_clicked().connect( sigc::bind( sigc::mem_fun(*this, &ObjectsPanel::_takeAction), (int)BUTTON_NEW) );
    _buttonsSecondary.pack_start(*btn, Gtk::PACK_SHRINK);

    //Remove object
    btn = Gtk::manage( new Gtk::Button() );
    _styleButton(*btn, INKSCAPE_ICON("list-remove"), _("Remove object"));
    btn->set_relief(Gtk::RELIEF_NONE);
    btn->signal_clicked().connect( sigc::bind( sigc::mem_fun(*this, &ObjectsPanel::_takeAction), (int)BUTTON_DELETE) );
    _watching.push_back( btn );
    _buttonsSecondary.pack_start(*btn, Gtk::PACK_SHRINK);

    //Move to bottom
    btn = Gtk::manage( new Gtk::Button() );
    _styleButton(*btn, INKSCAPE_ICON("go-bottom"), _("Move To Bottom"));
    btn->set_relief(Gtk::RELIEF_NONE);
    btn->signal_clicked().connect( sigc::bind( sigc::mem_fun(*this, &ObjectsPanel::_takeAction), (int)BUTTON_BOTTOM) );
    _watchingNonBottom.push_back( btn );
    _buttonsPrimary.pack_end(*btn, Gtk::PACK_SHRINK);
    
    //Move down    
    btn = Gtk::manage( new Gtk::Button() );
    _styleButton(*btn, INKSCAPE_ICON("go-down"), _("Move Down"));
    btn->set_relief(Gtk::RELIEF_NONE);
    btn->signal_clicked().connect( sigc::bind( sigc::mem_fun(*this, &ObjectsPanel::_takeAction), (int)BUTTON_DOWN) );
    _watchingNonBottom.push_back( btn );
    _buttonsPrimary.pack_end(*btn, Gtk::PACK_SHRINK);
    
    //Move up
    btn = Gtk::manage( new Gtk::Button() );
    _styleButton(*btn, INKSCAPE_ICON("go-up"), _("Move Up"));
    btn->set_relief(Gtk::RELIEF_NONE);
    btn->signal_clicked().connect( sigc::bind( sigc::mem_fun(*this, &ObjectsPanel::_takeAction), (int)BUTTON_UP) );
    _watchingNonTop.push_back( btn );
    _buttonsPrimary.pack_end(*btn, Gtk::PACK_SHRINK);
    
    //Move to top
    btn = Gtk::manage( new Gtk::Button() );
    _styleButton(*btn, INKSCAPE_ICON("go-top"), _("Move To Top"));
    btn->set_relief(Gtk::RELIEF_NONE);
    btn->signal_clicked().connect( sigc::bind( sigc::mem_fun(*this, &ObjectsPanel::_takeAction), (int)BUTTON_TOP) );
    _watchingNonTop.push_back( btn );
    _buttonsPrimary.pack_end(*btn, Gtk::PACK_SHRINK);
    
    //Collapse all
    btn = Gtk::manage( new Gtk::Button() );
    _styleButton(*btn, INKSCAPE_ICON("format-indent-less"), _("Collapse All"));
    btn->set_relief(Gtk::RELIEF_NONE);
    btn->signal_clicked().connect( sigc::bind( sigc::mem_fun(*this, &ObjectsPanel::_takeAction), (int)BUTTON_COLLAPSE_ALL) );
    _watchingNonBottom.push_back( btn );
    _buttonsPrimary.pack_end(*btn, Gtk::PACK_SHRINK);
    
    _buttonsRow.pack_start(_buttonsSecondary, Gtk::PACK_EXPAND_WIDGET);
    _buttonsRow.pack_end(_buttonsPrimary, Gtk::PACK_EXPAND_WIDGET);

    setDesktop( targetDesktop );

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

    if (_pending) {
        delete _pending;
        _pending = nullptr;
    }

    if (_toggleEvent){
        gdk_event_free(_toggleEvent);
        _toggleEvent = nullptr;
    }
}

/**
 * Sets the current document
 */
void ObjectsPanel::setDocument(SPDesktop* desktop, SPDocument* document)
{
    g_assert(desktop == _desktop);

    if (_rootWatcher) {
        delete _rootWatcher;
    }

    _document = document;
    _rootWatcher = nullptr;

    if (document && document->getRoot()) {
        _rootWatcher = new ObjectsPanel::ObjectWatcher(this, document->getRoot(), nullptr);
    }
}

/**
 * Set the current panel desktop
 */
void ObjectsPanel::setDesktop( SPDesktop* desktop )
{
    Panel::setDesktop(desktop);

    if ( desktop != _desktop ) {
        _documentChangedConnection.disconnect();
        _documentChangedCurrentLayer.disconnect();
        _selectionChangedConnection.disconnect();
        if ( _desktop ) {
            _desktop = nullptr;
        }

        _desktop = Panel::getDesktop();
        if ( _desktop ) {
            //Connect desktop signals
            _documentChangedConnection = _desktop->connectDocumentReplaced( sigc::mem_fun(*this, &ObjectsPanel::setDocument));
            // XXX _documentChangedCurrentLayer = _desktop->connectCurrentLayerChanged( sigc::mem_fun(*this, &ObjectsPanel::_objectsChangedWrapper));
            _selectionChangedConnection = _desktop->selection->connectChanged( sigc::mem_fun(*this, &ObjectsPanel::_objectsSelected));
            _desktopDestroyedConnection = _desktop->connectDestroy( sigc::mem_fun(*this, &ObjectsPanel::_desktopDestroyed));
            setDocument(_desktop, _desktop->doc());
            connectPopupItems();
        } else {
            setDocument(_desktop, nullptr);
        }
    }
}

void ObjectsPanel::connectPopupItems()
{
    _watching.clear();
    _watchingNonTop.clear();
    _watchingNonBottom.clear();
    _popupMenu = Gtk::Menu();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _show_contextmenu_icons = prefs->getBool("/theme/menuIcons_objects", true);

    _watching.push_back( &_addPopupItem( _desktop, SP_VERB_LAYER_RENAME, (int)BUTTON_RENAME ) );
    _watching.push_back( &_addPopupItem( _desktop, SP_VERB_LAYER_NEW, (int)BUTTON_NEW ) );

    _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    _watching.push_back( &_addPopupItem( _desktop, SP_VERB_LAYER_SOLO, (int)BUTTON_SOLO ) );
    _watching.push_back( &_addPopupItem( _desktop, SP_VERB_LAYER_SHOW_ALL, (int)BUTTON_SHOW_ALL ) );
    _watching.push_back( &_addPopupItem( _desktop, SP_VERB_LAYER_HIDE_ALL, (int)BUTTON_HIDE_ALL ) );

    _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    _watching.push_back( &_addPopupItem( _desktop, SP_VERB_LAYER_LOCK_OTHERS, (int)BUTTON_LOCK_OTHERS ) );
    _watching.push_back( &_addPopupItem( _desktop, SP_VERB_LAYER_LOCK_ALL, (int)BUTTON_LOCK_ALL ) );
    _watching.push_back( &_addPopupItem( _desktop, SP_VERB_LAYER_UNLOCK_ALL, (int)BUTTON_UNLOCK_ALL ) );

    _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    _watchingNonTop.push_back( &_addPopupItem(_desktop, SP_VERB_SELECTION_STACK_UP, (int)BUTTON_UP) );
    _watchingNonBottom.push_back( &_addPopupItem(_desktop, SP_VERB_SELECTION_STACK_DOWN, (int)BUTTON_DOWN) );

    _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    
    _watching.push_back( &_addPopupItem( _desktop, SP_VERB_SELECTION_GROUP, (int)BUTTON_GROUP ) );
    _watching.push_back( &_addPopupItem( _desktop, SP_VERB_SELECTION_UNGROUP, (int)BUTTON_UNGROUP ) );
    
    _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    
    _watching.push_back( &_addPopupItem( _desktop, SP_VERB_OBJECT_SET_CLIPPATH, (int)BUTTON_SETCLIP ) );
    
    _watching.push_back( &_addPopupItem( _desktop, SP_VERB_OBJECT_CREATE_CLIP_GROUP, (int)BUTTON_CLIPGROUP ) );

    //will never be implemented
    //_watching.push_back( &_addPopupItem( _desktop, SP_VERB_OBJECT_SET_INVERSE_CLIPPATH, (int)BUTTON_SETINVCLIP ) );
    _watching.push_back( &_addPopupItem( _desktop, SP_VERB_OBJECT_UNSET_CLIPPATH, (int)BUTTON_UNSETCLIP ) );

    _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    
    _watching.push_back( &_addPopupItem( _desktop, SP_VERB_OBJECT_SET_MASK, (int)BUTTON_SETMASK ) );
    _watching.push_back( &_addPopupItem( _desktop, SP_VERB_OBJECT_UNSET_MASK, (int)BUTTON_UNSETMASK ) );

    _watching.push_back( &_addPopupItem( _desktop, SP_VERB_EDIT_DUPLICATE, (int)BUTTON_DUPLICATE ) );
    _watching.push_back( &_addPopupItem( _desktop, SP_VERB_EDIT_DELETE, (int)BUTTON_DELETE ) );

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
