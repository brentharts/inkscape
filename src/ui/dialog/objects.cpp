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
    unsigned int item_type = 1;
     //_property_item->get_type();

    // if the icon isn't cached, render it to a pixbuf
    if ( !_icon_cache[item_type] ) { 
  
        // Get icon for item type...
        //Glib::ustring image_name = "gtk-missing";
        Glib::ustring image_name = Inkscape::Verb::get(SP_VERB_LAYER_DELETE)->get_image();
        Gtk::Image* icon = Gtk::manage(new Gtk::Image());
        icon = sp_get_icon_image(image_name, Gtk::ICON_SIZE_MENU);
  
        if (icon) {
            // check icon type (inkscape, gtk, none)
            if ( GTK_IS_IMAGE(icon->gobj()) ) {
                _property_icon = sp_get_icon_pixbuf(image_name, 16);
                property_pixbuf() = _icon_cache[item_type] = _property_icon.get_value  ();
                delete icon;
            } else {
                delete icon;
                return;
            }
        }

    } else {
        property_pixbuf() = _icon_cache[item_type];
    }
  
    Gtk::CellRendererPixbuf::render_vfunc(cr, widget, background_area,
                                          cell_area, flags);
}


/**
 * Gets an instance of the Objects panel
 */
ObjectsPanel& ObjectsPanel::getInstance()
{
    return *new ObjectsPanel();
}

/**
 * Column enumeration
 */
enum {
    COL_LABEL,
    COL_VISIBLE,
    COL_LOCKED,
};

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
//    BUTTON_SETINVCLIP,
    BUTTON_UNSETCLIP,
    BUTTON_SETMASK,
    BUTTON_UNSETMASK,
    BUTTON_GROUP,
    BUTTON_UNGROUP,
    BUTTON_COLLAPSE_ALL,
    DRAGNDROP,
    UPDATE_TREE
};

/**
 * Xml node observer for observing objects in the document
 */
class ObjectsPanel::ObjectWatcher : public Inkscape::XML::NodeObserver {
public:
    /**
     * Creates a new object watcher
     * @param pnl The panel to which the object watcher belongs
     * @param obj The object to watch
     */
    ObjectWatcher(ObjectsPanel* pnl, SPObject* obj) :
        _pnl(pnl),
        _obj(obj),
        _repr(obj->getRepr()),
        _highlightAttr(g_quark_from_string("inkscape:highlight-color")),
        _lockedAttr(g_quark_from_string("sodipodi:insensitive")),
        _labelAttr(g_quark_from_string("inkscape:label")),
        _groupAttr(g_quark_from_string("inkscape:groupmode")),
        _styleAttr(g_quark_from_string("style")),
        _clipAttr(g_quark_from_string("clip-path")),
        _maskAttr(g_quark_from_string("mask"))
    {
        _repr->addObserver(*this);
    }

    ~ObjectWatcher() override {
        _repr->removeObserver(*this);
    }

    void notifyChildAdded( Node &/*node*/, Node &/*child*/, Node */*prev*/ ) override
    {
        if ( _pnl && _obj ) {
            _pnl->_objectsChangedWrapper( _obj );
        }
    }
    void notifyChildRemoved( Node &/*node*/, Node &/*child*/, Node */*prev*/ ) override
    {
        if ( _pnl && _obj ) {
            _pnl->_objectsChangedWrapper( _obj );
        }
    }
    void notifyChildOrderChanged( Node &/*node*/, Node &/*child*/, Node */*old_prev*/, Node */*new_prev*/ ) override
    {
        if ( _pnl && _obj ) {
            _pnl->_objectsChangedWrapper( _obj );
        }
    }
    void notifyContentChanged( Node &/*node*/, Util::ptr_shared /*old_content*/, Util::ptr_shared /*new_content*/ ) override {}
    void notifyAttributeChanged( Node &node, GQuark name, Util::ptr_shared /*old_value*/, Util::ptr_shared /*new_value*/ ) override {
        /* Weird things happen on undo! we get notified about the child being removed, but after that we still get
         * notified for attributes being changed on this XML node! In that case the corresponding SPObject might already
         * have been deleted and the pointer to might be invalid, leading to a segfault if we're not careful.
         * So after we initiated the update of the treeview using _objectsChangedWrapper() in notifyChildRemoved(), the
         * _pending_update flag is set, and we will no longer process any notifyAttributeChanged()
         * Reproducing the crash: new document -> open objects panel -> draw freehand line -> undo -> segfault (but only
         * if we don't check for _pending_update) */
        if ( _pnl && (!_pnl->_pending_update) && _obj ) {
            if ( name == _lockedAttr || name == _labelAttr || name == _highlightAttr || name == _groupAttr || name == _styleAttr || name == _clipAttr || name == _maskAttr ) {
                _pnl->_updateObject(_obj, name == _highlightAttr);
            }
        }
    }

    /**
     * Objects panel to which this watcher belongs
     */
    ObjectsPanel* _pnl;
    
    /**
     * The object that is being observed
     */
    SPObject* _obj;
    
    /**
     * The xml representation of the object that is being observed
     */
    Inkscape::XML::Node* _repr;
    
    /* These are quarks which define the attributes that we are observing */
    GQuark _highlightAttr;
    GQuark _lockedAttr;
    GQuark _labelAttr;
    GQuark _groupAttr;
    GQuark _styleAttr;
    GQuark _clipAttr;
    GQuark _maskAttr;
};

class ObjectsPanel::InternalUIBounce
{
public:
    int _actionCode;
    sigc::connection _signal;
};

class ObjectsPanel::ModelColumns : public Gtk::TreeModel::ColumnRecord
{
public:

    ModelColumns()
    {
        add(_colObject);
        add(_colVisible);
        add(_colLocked);
        add(_colLabel);
        add(_colPrevSelectionState);
    }
    ~ModelColumns() override = default;

    Gtk::TreeModelColumn<SPItem*> _colObject;
    Gtk::TreeModelColumn<Glib::ustring> _colLabel;
    Gtk::TreeModelColumn<bool> _colVisible;
    Gtk::TreeModelColumn<bool> _colLocked;
    Gtk::TreeModelColumn<bool> _colPrevSelectionState;
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
 * Attach a watcher to the XML node of an item, which will signal us in case of changes to that item or node
 * @param item The item of which the XML node is to be watched
 */
void ObjectsPanel::_addWatcher(SPItem *item) {
    bool used = true; // Any newly created watcher is obviously being used
    auto iter = _objectWatchers.find(item);
    if (iter == _objectWatchers.end()) { // If not found then watcher doesn't exist yet
        ObjectsPanel::ObjectWatcher *w = new ObjectsPanel::ObjectWatcher(this, item);
        _objectWatchers.emplace(item, std::make_pair(w, used));
    } else { // Found; no need to create a new watcher; just flag it as "in use"
        (*iter).second.second = used;
    }
}

/**
 * Delete the watchers, which signal us in case of changes to the item being watched
 * @param only_unused Only delete those watchers that are no longer in use
 */
void ObjectsPanel::_removeWatchers(bool only_unused = false) {
    // Delete all watchers (optionally only those which are not in use)
    auto iter = _objectWatchers.begin();
    while (iter != _objectWatchers.end()) {
        bool used = (*iter).second.second;
        bool delete_watcher = (!only_unused) || (only_unused && !used);
        if ( delete_watcher ) {
            ObjectsPanel::ObjectWatcher *w = (*iter).second.first;
            delete w;
            iter = _objectWatchers.erase(iter);
        } else {
            // It must be in use, so the used "field" should be set to true;
            // However, when _removeWatchers is being called, we will already have processed the complete queue ...
            g_assert(_tree_update_queue.empty());
            // .. and we can preemptively flag it as unused for the processing of the next queue
            (*iter).second.second = false; // It will be set to true again by _addWatcher, if in use
            iter++;
        }
    }
    if (!only_unused) {
        //Delete the root watcher
        if (_rootWatcher) {
            _rootWatcher->_repr->removeObserver(*_rootWatcher);
            delete _rootWatcher;
            _rootWatcher = nullptr;
        }
    }
}
/**
 * Call function for asynchronous invocation of _objectsChanged
 */
void ObjectsPanel::_objectsChangedWrapper(SPObject */*obj*/) {
    // We used to call _objectsChanged with a reference to _obj,
    // but since _obj wasn't used, I'm dropping that for now
    _takeAction(UPDATE_TREE);
}

/**
 * Callback function for when an object changes.  Essentially refreshes the entire tree
 * @param obj Object which was changed (currently not used as the entire tree is recreated)
 */
void ObjectsPanel::_objectsChanged(SPObject */*obj*/)
{
    if (auto document = getDocument()) {
        if (auto root = document->getRoot()) {
            _selectedConnection.block(); // Will be unblocked after the queue has been processed fully

            //Clear the tree store
            _store->clear(); // This will increment it's stamp, making all old iterators
            _tree_cache.clear(); // invalid. So we will also clear our own cache, as well
            _tree_update_queue.clear(); // as any remaining update queue

            // Temporarily detach the TreeStore from the TreeView to slightly reduce flickering, and to speed up
            // Note: if we truly want to eliminate the flickering, we should implement double buffering on the _store,
            // but maybe this is a bit too much effort/bloat for too little gain?
            _tree.unset_model();

            //Add all items recursively; we will do this asynchronously, by first filling a queue, which is rather fast
            _queueObject( root, nullptr );
            //However, the processing of this queue is slow, so this is done at a low priority and in small chunks. Using
            //only small chunks keeps Inkscape responsive, for example while using the spray tool. After processing each
            //of the chunks, Inkscape will check if there are other tasks with a high priority, for example when user is
            //spraying. If so, the sprayed objects will be added first, and the whole updating will be restarted before
            //it even finished.
            _paths_to_be_expanded.clear();
            _processQueue_sig.disconnect(); // Might be needed in case objectsChanged is called directly, and not through objectsChangedWrapper()
            _processQueue_sig = Glib::signal_timeout().connect( sigc::mem_fun(*this, &ObjectsPanel::_processQueue), 0, Glib::PRIORITY_DEFAULT_IDLE+100);
        }
    }
}

/**
 * Recursively adds the children of the given item to the tree
 * @param obj Root object to add to the tree
 * @param parentRow Parent tree row (or NULL if adding to tree root)
 */
void ObjectsPanel::_queueObject(SPObject* obj, Gtk::TreeModel::Row* parentRow)
{
    bool already_expanded = false;

    for(auto& child: obj->children) {
        if (SP_IS_ITEM(&child)) {
            //Add the item to the tree, basically only creating an empty row in the tree view
            Gtk::TreeModel::iterator iter = parentRow ? _store->prepend(parentRow->children()) : _store->prepend();

            //Add the item to a queue, so we can fill in the data in each row asynchronously
            //at a later stage. See the comments in _objectsChanged() for more details
            bool expand = SP_IS_GROUP(obj) && SP_GROUP(obj)->expanded() && (not already_expanded);
            _tree_update_queue.emplace_back(SP_ITEM(&child), iter, expand);

            already_expanded = expand || already_expanded; // We need to expand only a single child in each group

            //If the item is a group, recursively add its children
            if (SP_IS_GROUP(&child)) {
                Gtk::TreeModel::Row row = *iter;
                _queueObject(&child, &row);
            }
        }
    }
}

/**
 * Walks through the queue in small chunks, and fills in the rows in the tree view accordingly
 * @return False if the queue has been fully emptied
 */
bool ObjectsPanel::_processQueue() {
    auto desktop = getDesktop();
    if (!desktop) {
        return false;
    }

    auto queue_iter = _tree_update_queue.begin();
    auto queue_end  = _tree_update_queue.end();
    int count = 0;

    while (queue_iter != queue_end) {
        //The queue is a list of tuples; expand the tuples
        SPItem *item                    = std::get<0>(*queue_iter);
        Gtk::TreeModel::iterator iter   = std::get<1>(*queue_iter);
        bool expanded                   = std::get<2>(*queue_iter);
        //Add the object to the tree view and tree cache
        _addObjectToTree(item, *iter, expanded);
        _tree_cache.emplace(item, *iter);

        /* Update the watchers; No watcher shall be deleted before the processing of the queue has
         * finished; we need to keep watching for items that might have been deleted while the queue,
         * which is being processed on idle, was not yet empty. This is because when an item is deleted, the
         * queue is still holding a pointer to it. The NotifyChildRemoved method of the watcher will stop the
         * processing of the queue and prevent a segmentation fault, but only if there is a watcher in place*/
        _addWatcher(item);

        queue_iter = _tree_update_queue.erase(queue_iter);
        count++;
        if (count == 100 && (!_tree_update_queue.empty())) {
            return true; // we have not yet reached the end of the queue, so return true to keep the timeout signal alive
        }
    }

    //We have reached the end of the queue, and it is safe to remove any watchers
    _removeWatchers(true); // ... but only remove those that are no longer in use

    // Now we can bring the tree view back to life safely
    _tree.set_model(_store); // Attach the store again to the tree view this sets search columns as -1
    _tree.set_search_column(_model->_colLabel);//set search column again 

    // Expand the tree; this is kept outside of _addObjectToTree() and _processQueue() to allow
    // temporarily detaching the store from the tree, which slightly reduces flickering
    for (auto path: _paths_to_be_expanded) {
        _tree.expand_to_path(path);
        _tree.collapse_row(path);
    }

    _blockAllSignals(false);
    _objectsSelected(desktop->selection); //Set the tree selection; will also invoke _checkTreeSelection()
    _pending_update = false;
    return false; // Return false to kill the timeout signal that kept calling _processQueue
}

/**
 * Fills in the details of an item in the already existing row of the tree view
 * @param item Item of which the name, visibility, lock status, etc, will be filled in
 * @param row Row where the item is residing
 * @param expanded True if the item is part of a group that is shown as expanded in the tree view
 */
void ObjectsPanel::_addObjectToTree(SPItem* item, const Gtk::TreeModel::Row &row, bool expanded)
{
    row[_model->_colObject] = item;
    gchar const * label = item->label() ? item->label() : item->getId();
    row[_model->_colLabel] = label ? label : item->defaultLabel();
    row[_model->_colVisible] = !item->isHidden();
    row[_model->_colLocked] = !item->isSensitive();
    //If our parent object is a group and it's expanded, expand the tree
    if (expanded) {
        _paths_to_be_expanded.emplace_back(_store->get_path(row));
    }
}

/**
 * Updates an item in the tree and optionally recursively updates the item's children
 * @param obj The item to update in the tree
 * @param recurse Whether to recurse through the item's children
 */
void ObjectsPanel::_updateObject( SPObject *obj, bool recurse ) {
    Gtk::TreeModel::iterator tree_iter;
    if (_findInTreeCache(SP_ITEM(obj), tree_iter)) {
        Gtk::TreeModel::Row row = *tree_iter;

        //We found our item in the tree; now update it!
        SPItem * item = SP_IS_ITEM(obj) ? SP_ITEM(obj) : nullptr;

        gchar const * label = obj->label() ? obj->label() : obj->getId();
        row[_model->_colLabel] = label ? label : obj->defaultLabel();
        row[_model->_colVisible] = item ? !item->isHidden() : false;
        row[_model->_colLocked] = item ? !item->isSensitive() : false;

        if (recurse){
            for (auto& iter: obj->children) {
                _updateObject(&iter, recurse);
            }
        }
    }
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

// See the comment in objects.h for _tree_cache
/**
 * Find the specified item in the tree cache
 * @param iter Current tree item
 * @param tree_iter Tree_iter will point to the row in which the tree item was found
 * @return True if found
 */
bool ObjectsPanel::_findInTreeCache(SPItem* item, Gtk::TreeModel::iterator &tree_iter) {
    if (not item) {
        return false;
    }

    try {
        tree_iter = _tree_cache.at(item);
    }
    catch (std::out_of_range) {
        // Apparently, item cannot be found in the tree_cache, which could mean that
        // - the tree and/or tree_cache are out-dated or in the process of being updated.
        // - a layer is selected, which is not visible in the objects panel (see _objectsSelected())
        // Anyway, this doesn't seem all that critical, so no warnings; just return false
        return false;
    }

    /* If the row in the tree has been deleted, and an old tree_cache is being used, then we will
     * get a segmentation fault crash somewhere here; so make sure iters don't linger around!
     * We can only check the validity as done below, but this is rather slow according to the
     * documentation (adds 0.25 s for a 2k long tree). But better safe than sorry
     */
    if (not _store->iter_is_valid(tree_iter)) {
        g_critical("Invalid iterator to Gtk::tree in objects panel; just prevented a segfault!");
        return false;
    }

    return true;
}


/**
 * Find the specified item in the tree store and (de)select it, optionally scrolling to the item
 * @param item Item to select in the tree
 * @param scrollto Whether to scroll to the item
 * @param expand If true, the path in the tree towards item will be expanded
 */
void ObjectsPanel::_updateObjectSelected(SPItem* item, bool scrollto, bool expand)
{
    Gtk::TreeModel::iterator tree_iter;
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
    }
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
            if (!SP_IS_GROUP(item) || SP_GROUP(item)->layerMode() != SPGroup::LAYER) {
                //If the item is not a layer, then select it and set the current layer to its parent (if it's the first item)
                if (desktop->selection->isEmpty()) {
                    desktop->setCurrentLayer(item->parent);
                }
                desktop->selection->add(item);
            } else {
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
    auto desktop = getDesktop();

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

/**
 * Stores items in the highlight target vector to manipulate with the color selector
 * @param iter Current tree item to store
 */
void ObjectsPanel::_storeHighlightTarget(const Gtk::TreeModel::iterator& iter)
{
    Gtk::TreeModel::Row row = *iter;
    SPItem* item = row[_model->_colObject];
    if (item)
    {
        _highlight_target.push_back(item);
    }
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
 * Prevents the treeview from emitting and responding to most signals; needed when it's not up to date
 */
void ObjectsPanel::_blockAllSignals(bool should_block = true) {

    // incoming signals
    _isolationConnection.block(should_block);
    _opacityConnection.block(should_block);
    _blendConnection.block(should_block);
    _blurConnection.block(should_block);
    if (_pending && should_block) {
        // Kill any pending UI event, e.g. a delete or drag 'n drop action, which could
        // become unpredictable after the tree has been updated
        _pending->_signal.disconnect();
    }
    // outgoing signal
    _selectedConnection.block(should_block);
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

bool ObjectsPanel::_executeUpdate() {
    _objectsChanged(nullptr);
    return false;
}

/**
 * Executes the given button action during the idle time
 */
void ObjectsPanel::_takeAction( int val )
{
    if (val == UPDATE_TREE) {
        _pending_update = true;
        // We might already have been updating the tree, but new data is available now
        // so we will then first cancel the old update before scheduling a new one
        _processQueue_sig.disconnect();
        _executeUpdate_sig.disconnect();
        _blockAllSignals(true);
        //_store->clear();
        _tree_cache.clear();
        _executeUpdate_sig = Glib::signal_timeout().connect( sigc::mem_fun(*this, &ObjectsPanel::_executeUpdate), 500, Glib::PRIORITY_DEFAULT_IDLE+50);
        // In the spray tool, updating the tree competes in priority with the redrawing of the canvas,
        // see SPCanvas::addIdle(), which is set to UPDATE_PRIORITY (=G_PRIORITY_DEFAULT_IDLE). We
        // should take a lower priority (= higher value) to keep the spray tool updating longer, and to prevent
        // the objects-panel from clogging the processor; however, once the spraying slows down, the tree might
        // get updated anyway.
    } else if ( !_pending ) {
        _pending = new InternalUIBounce();
        _pending->_actionCode = val;
        _pending->_signal = Glib::signal_timeout().connect( sigc::mem_fun(*this, &ObjectsPanel::_executeAction), 0 );
    }
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
//        SPObject* target = _pending->_target;

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
                _objectsChanged(document->getRoot());
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
    _defer_target(),
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
    //_name_column->add_attribute(_icon_renderer->property_item(), _model->_colObject);

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

    //Set up the pop-up menu
    // -------------------------------------------------------
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        _show_contextmenu_icons = prefs->getBool("/theme/menuIcons_objects", true);

        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_RENAME, (int)BUTTON_RENAME ) );
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_NEW, (int)BUTTON_NEW ) );

        _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_SOLO, (int)BUTTON_SOLO ) );
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_SHOW_ALL, (int)BUTTON_SHOW_ALL ) );
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_HIDE_ALL, (int)BUTTON_HIDE_ALL ) );

        _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_LOCK_OTHERS, (int)BUTTON_LOCK_OTHERS ) );
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_LOCK_ALL, (int)BUTTON_LOCK_ALL ) );
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_UNLOCK_ALL, (int)BUTTON_UNLOCK_ALL ) );

        _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

        _watchingNonTop.push_back( &_addPopupItem(targetDesktop, SP_VERB_SELECTION_STACK_UP, (int)BUTTON_UP) );
        _watchingNonBottom.push_back( &_addPopupItem(targetDesktop, SP_VERB_SELECTION_STACK_DOWN, (int)BUTTON_DOWN) );

        _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
        
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_SELECTION_GROUP, (int)BUTTON_GROUP ) );
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_SELECTION_UNGROUP, (int)BUTTON_UNGROUP ) );
        
        _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
        
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_OBJECT_SET_CLIPPATH, (int)BUTTON_SETCLIP ) );
        
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_OBJECT_CREATE_CLIP_GROUP, (int)BUTTON_CLIPGROUP ) );

        //will never be implemented
        //_watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_OBJECT_SET_INVERSE_CLIPPATH, (int)BUTTON_SETINVCLIP ) );
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_OBJECT_UNSET_CLIPPATH, (int)BUTTON_UNSETCLIP ) );

        _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
        
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_OBJECT_SET_MASK, (int)BUTTON_SETMASK ) );
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_OBJECT_UNSET_MASK, (int)BUTTON_UNSETMASK ) );

        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_EDIT_DUPLICATE, (int)BUTTON_DUPLICATE ) );
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_EDIT_DELETE, (int)BUTTON_DELETE ) );

        _popupMenu.show_all_children();

        // Install CSS to shift icons into the space reserved for toggles (i.e. check and radio items).
        _popupMenu.signal_map().connect(sigc::bind<Gtk::MenuShell *>(sigc::ptr_fun(shift_icons), &_popupMenu));
    }

    // -------------------------------------------------------

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

    setDesktop( targetDesktop );

    show_all_children();
}

/**
 * Destructor
 */
ObjectsPanel::~ObjectsPanel()
{
    // Never being called, not even when closing Inkscape?

    //Set the desktop to null, which will disconnect all object watchers
    setDesktop(nullptr);

    if ( _model )
    {
        delete _model;
        _model = nullptr;
    }

    if (_pending) {
        delete _pending;
        _pending = nullptr;
    }

    if (_toggleEvent) {
        gdk_event_free(_toggleEvent);
        _toggleEvent = nullptr;
    }
}

/**
 * Sets the current document
 */
void ObjectsPanel::documentReplaced()
{
    //Clear all object watchers
    _removeWatchers();

    //Delete the root watcher
    if (_rootWatcher)
    {
        _rootWatcher->_repr->removeObserver(*_rootWatcher);
        delete _rootWatcher;
        _rootWatcher = nullptr;
    }

    _document = document;

    if (document && document->getRoot() && document->getRoot()->getRepr())
    {
        //Create a new root watcher for the document and then call _objectsChanged to fill the tree
        _rootWatcher = new ObjectsPanel::ObjectWatcher(this, document->getRoot());
        document->getRoot()->getRepr()->addObserver(*_rootWatcher);
        _objectsChanged(document->getRoot());
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

            _documentChangedCurrentLayer = _desktop->connectCurrentLayerChanged( sigc::mem_fun(*this, &ObjectsPanel::_objectsChangedWrapper));

            _selectionChangedConnection = _desktop->selection->connectChanged( sigc::mem_fun(*this, &ObjectsPanel::_objectsSelected));

            _desktopDestroyedConnection = _desktop->connectDestroy( sigc::mem_fun(*this, &ObjectsPanel::_desktopDestroyed));

            setDocument(_desktop, _desktop->doc());
        } else {
            setDocument(nullptr, nullptr);
        }
    }
}

} //namespace Dialogs
} //namespace UI
} //namespace Inkscape

//should be okay to put these here because they are never referenced anywhere else
using namespace Inkscape::UI::Tools;

void SPItem::setHighlightColor(guint32 const color)
{
    g_free(_highlightColor);
    if (color & 0x000000ff)
    {
        _highlightColor = g_strdup_printf("%u", color);
    }
    else
    {
        _highlightColor = nullptr;
    }
    
    NodeTool *tool = nullptr;
    if (SP_ACTIVE_DESKTOP ) {
        Inkscape::UI::Tools::ToolBase *ec = SP_ACTIVE_DESKTOP->event_context;
        if (INK_IS_NODE_TOOL(ec)) {
            tool = static_cast<NodeTool*>(ec);
            set_active_tool(tool->getDesktop(), "Node");
        }
    }
}

void SPItem::unsetHighlightColor()
{
    g_free(_highlightColor);
    _highlightColor = nullptr;
    NodeTool *tool = nullptr;
    if (SP_ACTIVE_DESKTOP ) {
        Inkscape::UI::Tools::ToolBase *ec = SP_ACTIVE_DESKTOP->event_context;
        if (INK_IS_NODE_TOOL(ec)) {
            tool = static_cast<NodeTool*>(ec);
            set_active_tool(tool->getDesktop(), "Node");
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
