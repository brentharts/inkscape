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
#include "ui/widget/imagetoggler.h"
#include "ui/widget/shapeicon.h"

static double const SELECTED_ALPHA[8] = {0.0, 2.5, 4.0, 2.0, 8.0, 2.5, 1.0, 1.0};

//#define DUMP_LAYERS 1

namespace Inkscape {
namespace UI {
namespace Dialog {

class ObjectWatcher : public Inkscape::XML::NodeObserver
{
public:
    ObjectWatcher() = delete;
    ObjectWatcher(ObjectsPanel *panel, SPItem *, Gtk::TreeRow *row);
    ~ObjectWatcher() override;

    void updateRowInfo();
    void updateRowBg();

    void addDummyChild();
    void addChild(SPItem *);
    void addChildren();
    void setSelectedBit(SelectionState mask, bool enabled);
    void setSelectedBitRecursive(SelectionState mask, bool enabled);
    void moveChild(Node &child, Node *sibling);

    Gtk::TreeNodeChildren getChildren() const;
    Gtk::TreeIter getChildIter(Node *) const;

    void notifyChildAdded(Node &, Node &, Node *) override;
    void notifyChildRemoved(Node &, Node &, Node *) override;
    void notifyChildOrderChanged(Node &, Node &child, Node *, Node *) override;
    void notifyAttributeChanged(Node &, GQuark, Util::ptr_shared, Util::ptr_shared) override;

    /// Associate this watcher with a tree row
    void setRow(const Gtk::TreeModel::Path &path)
    {
        assert(path);
        row_ref = Gtk::TreeModel::RowReference(panel->_store, path);
    }
    // Get the path out of this watcher
    Gtk::TreeModel::Path getRow() const {
        return row_ref.get_path();
    }
    void setRow(const Gtk::TreeModel::Row &row) { setRow(panel->_store->get_path(row)); }

    /// True if this watchr has a valid row reference.
    bool hasRow() const { return bool(row_ref); }

    /// Transfer a child watcher to its new parent
    void transferChild(Node *childnode)
    {
        auto *target = panel->getWatcher(childnode->parent());
        assert(target != this);
        auto nh = child_watchers.extract(childnode);
        assert(nh);
        bool inserted = target->child_watchers.insert(std::move(nh)).inserted;
        assert(inserted);
    }

    /// The XML node associated with this watcher.
    Node *getRepr() const { return node; }

    std::unordered_map<Node const *, std::unique_ptr<ObjectWatcher>> child_watchers;

private:
    Node *node;
    Gtk::TreeModel::RowReference row_ref;
    ObjectsPanel *panel;
    SelectionState selection_state;
};

class ObjectsPanel::ModelColumns : public Gtk::TreeModel::ColumnRecord
{
public:
    ModelColumns()
    {
        add(_colNode);
        add(_colLabel);
        add(_colType);
        add(_colIconColor);
        add(_colClipMask);
        add(_colBgColor);
        add(_colVisible);
        add(_colLocked);
    }
    ~ModelColumns() override = default;
    Gtk::TreeModelColumn<Node*> _colNode;
    Gtk::TreeModelColumn<Glib::ustring> _colLabel;
    Gtk::TreeModelColumn<Glib::ustring> _colType;
    Gtk::TreeModelColumn<unsigned int> _colIconColor;
    Gtk::TreeModelColumn<unsigned int> _colClipMask;
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
ObjectWatcher::ObjectWatcher(ObjectsPanel* panel, SPItem* obj, Gtk::TreeRow *row) :
    panel(panel),
    row_ref(),
    selection_state(0),
    node(obj->getRepr())
{
    if(row != nullptr) {
        assert(row->children().empty());
        setRow(*row);
        updateRowInfo();
    }
    node->addObserver(*this);

    // Only show children for groups (and their subclasses like SPAnchor or SPRoot)
    if (!dynamic_cast<SPGroup const*>(obj)) {
        return;
    }

    // We'll add children for the root node (row == NULL), for all other
    // nodes we'll just add a dummy child and wait until the user expands
    // the row.

    if (!row) {
        addChildren();
    } else {
        for (auto &child : obj->children) {
            if (dynamic_cast<SPItem const *>(&child)) {
                addDummyChild();

                // one dummy child is enough to make the group expandable
                break;
            }
        }
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
    assert(item);

    if (item) {
        assert(row_ref);
        assert(row_ref.get_path());

        auto row = *panel->_store->get_iter(row_ref.get_path());
        row[panel->_model->_colNode] = node;

        // show ids without "#"
        char const *id = item->getId();
        row[panel->_model->_colLabel] = (id && !item->label()) ? id : item->defaultLabel();

        row[panel->_model->_colType] = item->typeName();
        row[panel->_model->_colIconColor] = item->highlight_color();
        row[panel->_model->_colClipMask] = 0;
            (item->getClipObject() ? Inkscape::UI::Widget::OVERLAY_CLIP : 0) |
            (item->getMaskObject() ? Inkscape::UI::Widget::OVERLAY_MASK : 0);
        row[panel->_model->_colVisible] = !item->isHidden();
        row[panel->_model->_colLocked] = !item->isSensitive();
    }
}

/**
 * Updates the row's background colour as indicated by it's selection.
 */
void ObjectWatcher::updateRowBg()
{
    assert(row_ref);
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
    {
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
 * Flip the selected state bit on or off as needed, on this watcher and all
 * its direct and indirect children.
 */
void ObjectWatcher::setSelectedBitRecursive(SelectionState mask, bool enabled)
{
    setSelectedBit(mask, enabled);

    for (auto &pair : child_watchers) {
        pair.second->setSelectedBitRecursive(mask, enabled);
    }
}

void ObjectWatcher::addDummyChild()
{
    auto const children = getChildren();
    assert(!children || children.empty());
    auto const iter = panel->_store->append(children);
    assert(panel->isDummy(*iter));
}

/**
 * Add the child object to this node.
 *
 * @param child - SPObject to be added
 */
void ObjectWatcher::addChild(SPItem *child)
{
    auto *node = child->getRepr();
    assert(node);
    Gtk::TreeModel::Row row = *(panel->_store->append(getChildren()));

    auto &watcher = child_watchers[node];
    assert(!watcher);
    watcher.reset(new ObjectWatcher(panel, child, &row));

    // Make sure new children have the right focus set.
    if ((selection_state & LAYER_FOCUSED) != 0) {
        watcher->setSelectedBit(LAYER_FOCUS_CHILD, true);
    }
}

/**
 * Add all SPItem children as child rows.
 */
void ObjectWatcher::addChildren()
{
    assert(child_watchers.empty());

    auto obj = panel->getObject(node);

    for (auto it = obj->children.rbegin(), it_end = obj->children.rend(); it != it_end; ++it) {
        if (auto child = dynamic_cast<SPItem *>(&*it)) {
            addChild(child);
        }
    }
}

/**
 * Move the child to just after the given sibling
 *
 * @param child - SPObject to be moved
 * @param sibling - Optional sibling Object to add next to, if nullptr the
 *                  object is moved to BEFORE the first item.
 */
void ObjectWatcher::moveChild(Node &child, Node *sibling)
{
    auto child_iter = getChildIter(&child);
    if (!child_iter)
        return; // This means the child was never added, probably not an SPItem.

    // sibling might not be an SPItem and thus not be represented in the
    // TreeView. Find the closest SPItem and use that for the reordering.
    while (sibling && !dynamic_cast<SPItem const *>(panel->getObject(sibling))) {
        sibling = sibling->prev();
    }

    auto sibling_iter = getChildIter(sibling);
    panel->_store->move(child_iter, sibling_iter);
}

/**
 * Get the TreeRow's children iterator
 *
 * @returns Gtk Tree Node Children iterator
 */
Gtk::TreeNodeChildren ObjectWatcher::getChildren() const
{
    Gtk::TreeModel::Path path;
    if (row_ref && (path = row_ref.get_path())) {
        return panel->_store->get_iter(path)->children();
    }
    assert(!row_ref);
    return panel->_store->children();
}

/**
 * Convert SPObject to TreeView Row, assuming the object is a child.
 *
 * @param child - The child object to find in this branch
 * @returns Gtk TreeRow for the child, or end() if not found
 */
Gtk::TreeIter ObjectWatcher::getChildIter(Node *node) const
{
    auto childrows = getChildren();

    if (!node) {
        return childrows.end();
    }

    // Note: TreeRow inherits from TreeIter, so this `row` variable is
    // also an iterator and a valid return value.
    for (auto &row : childrows) {
        if (panel->getRepr(row) == node) {
            return row;
        }
    }

    g_warning("%s cound not find child <%s %p>", __func__, node->name(), node);
    return childrows.begin();
}

void ObjectWatcher::notifyChildAdded( Node &node, Node &child, Node *prev )
{
    assert(this->node == &node);

    if (panel->isObserverBlocked()) {
        return;
    }

    // Ignore XML nodes which are not displayable items
    auto item = dynamic_cast<SPItem *>(panel->getObject(&child));
    if (!item) {
        return;
    }

    auto const childrows = getChildren();

    if (childrows.empty()) {
        addDummyChild();
        return;
    }

    if (panel->isDummy(childrows[0])) {
        return;
    }

    addChild(item);
    moveChild(child, prev);
}
void ObjectWatcher::notifyChildRemoved( Node &node, Node &child, Node* /*prev*/ )
{
    assert(this->node == &node);

    if (panel->isObserverBlocked()) {
        return;
    }

    if (child_watchers.erase(&child) > 0) {
        return;
    }

    if (node.firstChild() == nullptr) {
        assert(row_ref);
        auto iter = panel->_store->get_iter(row_ref.get_path());
        panel->removeDummyChildren(*iter);
    }
}
void ObjectWatcher::notifyChildOrderChanged( Node &parent, Node &child, Node */*old_prev*/, Node *new_prev )
{
    assert(this->node == &parent);

    if (panel->isObserverBlocked()) {
        return;
    }

    moveChild(child, new_prev);
}
void ObjectWatcher::notifyAttributeChanged( Node &node, GQuark name, Util::ptr_shared /*old_value*/, Util::ptr_shared /*new_value*/ )
{
    assert(this->node == &node);

    if (panel->isObserverBlocked()) {
        return;
    }

    // The root <svg> node doesn't have a row
    if (this == panel->getRootWatcher()) {
        return;
    }

    // Almost anything could change the icon, so update upon any change, defer for lots of updates.

    // examples of not-so-obvious cases:
    // - width/height: Can change type "circle" to an "ellipse"

    static std::set<GQuark> const excluded{
        g_quark_from_static_string("transform"),
        g_quark_from_static_string("x"),
        g_quark_from_static_string("y"),
        g_quark_from_static_string("d"),
        g_quark_from_static_string("sodipodi:nodetypes"),
    };

    if (excluded.count(name)) {
        return;
    }

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
    assert(node);
    if (root_watcher->getRepr() == node) {
        return root_watcher;
    } else if (node->parent()) {
        auto parent_watcher = getWatcher(node->parent());
        if (parent_watcher) {
            auto it = parent_watcher->child_watchers.find(node);
            if (it != parent_watcher->child_watchers.end()) {
                return it->second.get();
            }
        }
    }
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
    DialogBase("/dialogs/objects", SP_VERB_DIALOG_OBJECTS),
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
    _text_renderer->property_ellipsize().set_value(Pango::ELLIPSIZE_END);

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
    _name_column->add_attribute(icon_renderer->property_clipmask(), _model->_colClipMask);
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

    _tree.get_selection()->set_mode(Gtk::SELECTION_NONE);

    //Set up tree signals
    _tree.signal_button_press_event().connect(sigc::mem_fun(*this, &ObjectsPanel::_handleButtonEvent), false);
    _tree.signal_button_release_event().connect(sigc::mem_fun(*this, &ObjectsPanel::_handleButtonEvent), false);
    _tree.signal_key_press_event().connect( sigc::mem_fun(*this, &ObjectsPanel::_handleKeyEvent), false );
    _tree.signal_row_collapsed().connect( sigc::bind<bool>(sigc::mem_fun(*this, &ObjectsPanel::_setExpanded), false));
    _tree.signal_row_expanded().connect( sigc::bind<bool>(sigc::mem_fun(*this, &ObjectsPanel::_setExpanded), true));

    // Before expanding a row, replace the dummy child with the actual children
    _tree.signal_test_expand_row().connect([this](const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &) {
        if (removeDummyChildren(*iter)) {
            assert(iter);
            assert(*iter);
            getWatcher(getRepr(*iter))->addChildren();
            setSelection(_desktop->selection);
        }
        return false;
    });

    _tree.signal_drag_motion().connect(sigc::mem_fun(*this, &ObjectsPanel::on_drag_motion), false);
    _tree.signal_drag_drop().connect(sigc::mem_fun(*this, &ObjectsPanel::on_drag_drop), false);
    _tree.signal_drag_begin().connect(sigc::mem_fun(*this, &ObjectsPanel::on_drag_start), false);
    _tree.signal_drag_end().connect(sigc::mem_fun(*this, &ObjectsPanel::on_drag_end), false);

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
    pack_start(_page, Gtk::PACK_EXPAND_WIDGET);

    _addBarButton(INKSCAPE_ICON("list-add"), _("Add layer..."), (int)SP_VERB_LAYER_NEW);
    _addBarButton(INKSCAPE_ICON("list-remove"), _("Remove object"), (int)SP_VERB_EDIT_DELETE);
    _addBarButton(INKSCAPE_ICON("go-bottom"), _("Move To Bottom"), (int)SP_VERB_SELECTION_TO_BACK);
    _addBarButton(INKSCAPE_ICON("go-down"), _("Move Down"), (int)SP_VERB_SELECTION_STACK_DOWN);
    _addBarButton(INKSCAPE_ICON("go-up"), _("Move Up"), (int)SP_VERB_SELECTION_STACK_UP);
    _addBarButton(INKSCAPE_ICON("go-top"), _("Move To Top"), (int)SP_VERB_SELECTION_TO_FRONT);

    _buttonsRow.pack_start(_buttonsSecondary, Gtk::PACK_EXPAND_WIDGET);
    _buttonsRow.pack_end(_buttonsPrimary, Gtk::PACK_EXPAND_WIDGET);

    update();
    show_all_children();
}

/**
 * Destructor
 */
ObjectsPanel::~ObjectsPanel()
{
    document_changed.disconnect();
    selection_changed.disconnect();
    layer_changed.disconnect();

    _desktop = nullptr;
    setDocument(nullptr, nullptr);

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
void ObjectsPanel::update()
{
    if (!_app) {
        std::cerr << "ObjectsPanel::update(): _app is null" << std::endl;
        return;
    }
    SPDesktop *desktop = getDesktop();

    if ( desktop != _desktop ) {
        document_changed.disconnect();
        selection_changed.disconnect();
        layer_changed.disconnect();

        _desktop = desktop;

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
    root_watcher->setSelectedBitRecursive(SELECTED_OBJECT, false);

    for (auto item : selected->items()) {
        auto watcher = getWatcher(item->getRepr());
        if (watcher) {
            watcher->setSelectedBit(SELECTED_OBJECT, true);
        }
    }
}

/**
 * Happens when the layer selected is changed.
 *
 * @param layer - The layer now selected
 */
void ObjectsPanel::setLayer(SPObject *layer)
{
    root_watcher->setSelectedBitRecursive(LAYER_FOCUS_CHILD | LAYER_FOCUSED, false);

    if (!layer) return;
    auto watcher = getWatcher(layer->getRepr());
    if (watcher && watcher != root_watcher) {
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
    SPItem* item = getItem(row);
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
    SPItem* item = getItem(row);
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
 * Handles mouse up events
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

        auto selection = _desktop->getSelection();
        auto row = *_store->get_iter(path);
        if (!row) return false;
        SPItem *item = getItem(row);
        SPGroup *group = SP_GROUP(item);

        // Select items on button release to not confuse drag
        if (event->type == GDK_BUTTON_RELEASE) {
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
        } else {
            current_item = item;
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
        SPItem *item = getItem(row);
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

/**
 * Get the XML node which is associated with a row. Can be NULL for dummy children.
 */
Node *ObjectsPanel::getRepr(Gtk::TreeModel::Row const &row) const
{
    return row[_model->_colNode];
}

/**
 * Get the item which is associated with a row. If getRepr(row) is not NULL,
 * then this call is expected to also not be NULL.
 */
SPItem *ObjectsPanel::getItem(Gtk::TreeModel::Row const &row) const
{
    auto const this_const = const_cast<ObjectsPanel *>(this);
    return dynamic_cast<SPItem *>(this_const->getObject(getRepr(row)));
}

/**
 * Return true if this row has dummy children.
 */
bool ObjectsPanel::hasDummyChildren(Gtk::TreeModel::Row const &row) const
{
    for (auto &c : row.children()) {
        if (isDummy(c)) {
            return true;
        }
    }
    return false;
}

/**
 * If the given row has dummy children, remove them.
 * @pre Eiter all, or no children are dummies
 * @post If the function returns true, the row has no children
 * @return False if there are children and they are not dummies
 */
bool ObjectsPanel::removeDummyChildren(Gtk::TreeModel::Row const &row)
{
    auto &children = row.children();
    if (!children.empty()) {
        Gtk::TreeStore::iterator child = children[0];
        if (!isDummy(*child)) {
            assert(!hasDummyChildren(row));
            return false;
        }

        do {
            assert(child->parent() == row);
            assert(isDummy(*child));
            child = _store->erase(child);
        } while (child && child->parent() == row);
    }
    return true;
}

/**
 * Signal handler for "drag-motion"
 *
 * Refuses drops into non-group items.
 */
bool ObjectsPanel::on_drag_motion(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time)
{
    Gtk::TreeModel::Path path;
    Gtk::TreeViewDropPosition pos;

    if (!_desktop->getSelection()) {
        goto finally;
    }

    _tree.get_dest_row_at_pos(x, y, path, pos);

    if (path) {
        auto iter = _store->get_iter(path);
        auto repr = getRepr(*iter);
        auto obj = _document->getObjectByRepr(repr);

        bool const drop_into = pos != Gtk::TREE_VIEW_DROP_BEFORE && //
                               pos != Gtk::TREE_VIEW_DROP_AFTER;

        // don't drop on self
        if (_desktop->getSelection()->includes(obj)) {
            goto finally;
        }

        auto item = getItem(*iter);

        // only groups can have children
        if (drop_into && !dynamic_cast<SPGroup const *>(item)) {
            goto finally;
        }

        context->drag_status(Gdk::ACTION_MOVE, time);
        return false;
    }

finally:
    // remove drop highlight
    _tree.unset_drag_dest_row();
    context->drag_refuse(time);
    return true;
}

/**
 * Signal handler for "drag-drop".
 *
 * Do the actual work of drag-and-drop.
 */
bool ObjectsPanel::on_drag_drop(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time)
{
    //auto dragging_iter = _store->get_iter(dragging_path);
    //Node *dragging_repr = dragging_iter ? getRepr(*dragging_iter) : nullptr;

    Gtk::TreeModel::Path path;
    Gtk::TreeViewDropPosition pos;
    _tree.get_dest_row_at_pos(x, y, path, pos);

    if (!path) {
        return true;
    }

    auto oset = ObjectSet(_document);
    auto drop_repr = getRepr(*_store->get_iter(path));
    bool const drop_into = pos != Gtk::TREE_VIEW_DROP_BEFORE && //
                           pos != Gtk::TREE_VIEW_DROP_AFTER;

    for (auto item : _desktop->getSelection()->items()) {
        auto dragging_repr = item->getRepr();
        if (drop_into) {
            oset.add(dragging_repr);
        } else {
            if (drop_repr->parent() != dragging_repr->parent()) {
                oset.add(dragging_repr);

                // Switching layers has invalidated the pointer
                dragging_repr = oset.singleRepr();
                assert(dragging_repr);
            }

            if (pos == Gtk::TREE_VIEW_DROP_AFTER) {
                drop_repr = drop_repr->prev();
            }

            if (dragging_repr != drop_repr) {
                dragging_repr->parent()->changeOrder(dragging_repr, drop_repr);
            }
        }

    }
    if (!oset.isEmpty()) {
        if (drop_into) {
            oset.toLayer(_document->getObjectByRepr(drop_repr));
        } else {
            oset.toLayer(_document->getObjectByRepr(drop_repr->parent()));
        }
    }

    on_drag_end(context);
    return true;
}

void ObjectsPanel::on_drag_start(const Glib::RefPtr<Gdk::DragContext> &context)
{
    auto selection = _tree.get_selection();
    selection->set_mode(Gtk::SELECTION_MULTIPLE);
    selection->unselect_all();

    auto obj_selection = _desktop->getSelection();

    if (current_item && !obj_selection->includes(current_item)) {
        // This means the item the user started to drag is not one that is selected
        // So we'll deselect everything and start draging this item instead.
        auto watcher = getWatcher(current_item->getRepr());
        if (watcher) {
            auto path = watcher->getRow();
            selection->select(path);
            obj_selection->set(current_item);
        }
    } else {
        // Drag all the items currently selected (multi-row)
        for (auto item : obj_selection->items()) {
            auto watcher = getWatcher(item->getRepr());
            if (watcher) {
                auto path = watcher->getRow();
                selection->select(path);
            }
        }
    }
}

void ObjectsPanel::on_drag_end(const Glib::RefPtr<Gdk::DragContext> &context)
{
    auto selection = _tree.get_selection();
    selection->unselect_all();
    selection->set_mode(Gtk::SELECTION_NONE);
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
