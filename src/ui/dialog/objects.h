// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A simple dialog for objects UI.
 *
 * Authors:
 *   Theodore Janeczko
 *   Tavmjong Bah
 *
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *               Tavmjong Bah 2017
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_OBJECTS_PANEL_H
#define SEEN_OBJECTS_PANEL_H

#include <gtkmm/box.h>
#include <gtkmm/dialog.h>

#include "xml/node-observer.h"

#include "ui/widget/panel.h"
#include "ui/widget/style-subject.h"

#include "selection.h"

using Inkscape::XML::Node;

class SPObject;
class SPGroup;
struct SPColorSelector;

namespace Inkscape {
namespace UI {
namespace Dialog {

class ObjectsPanel;

enum {COL_LABEL, COL_VISIBLE, COL_LOCKED};

class ObjectWatcher : public Inkscape::XML::NodeObserver
{
public:

    ObjectWatcher(ObjectsPanel* panel, SPObject* obj, Gtk::TreeRow *row);
    ~ObjectWatcher() override;

    void updateRowInfo();
    void addChild(Node &node);
    void moveChild(SPObject *child, SPObject *sibling);

    Gtk::TreeNodeChildren getParentIter();
    const Gtk::TreeRow getChildIter(SPObject *child);

    void notifyChildAdded( Node &node, Node &child, Node *prev ) override;
    void notifyChildRemoved( Node &/*node*/, Node &child, Node* /*prev*/ ) override;
    void notifyChildOrderChanged( Node &parent, Node &child, Node */*old_prev*/, Node *new_prev ) override;
    void notifyAttributeChanged( Node &node, GQuark name, Util::ptr_shared /*old_value*/, Util::ptr_shared /*new_value*/ ) override;

    Node* node;
    std::unordered_map<Node const*, std::unique_ptr<ObjectWatcher>> child_watchers;
private:
    Gtk::TreeModel::RowReference row_ref;
    ObjectsPanel* panel;
};

/**
 * A panel that displays objects.
 */
class ObjectsPanel : public DialogBase
{
public:
    ObjectsPanel();
    ~ObjectsPanel() override;

    class ModelColumns;
    static ObjectsPanel& getInstance();

    void update() override;
    void setDocument(SPDesktop* desktop, SPDocument* document);
    void setSelection(Selection *sel);
    void setLayer(SPObject *obj);

    SPObject* getObject(Node *node);
    ObjectWatcher* getWatcher(Node *node);

    Glib::RefPtr<Gtk::TreeStore> _store;
    ModelColumns* _model;

private:
    //Internal Classes:
    class InternalUIBounce;

    ObjectWatcher* root_watcher;
    
    sigc::connection document_changed;
    sigc::connection selection_changed;
    sigc::connection layer_changed;

    //The current desktop, document, tree-data
    SPDesktop* _desktop;
    SPDocument* _document;
    
    InternalUIBounce* _pending;
    bool _pending_update;

    //Whether the drag & drop was dragged into an item
    gboolean _dnd_into;

    //List of drag & drop source items
    std::vector<SPItem*> _dnd_source;

    //Drag & drop target item
    SPItem* _dnd_target;

    // Whether the drag sources include a layer
    bool _dnd_source_includes_layer;

    //List of items to change the highlight on
    std::vector<SPItem*> _highlight_target;

    //Show icons in the context menu
    bool _show_contextmenu_icons;

    //GUI Members:

    GdkEvent* _toggleEvent;

    std::vector<Gtk::Widget*> _watching;
    std::vector<Gtk::Widget*> _watchingNonTop;
    std::vector<Gtk::Widget*> _watchingNonBottom;

    Gtk::TreeView _tree;
    Gtk::CellRendererText *_text_renderer;
    Gtk::TreeView::Column *_name_column;
    Gtk::Box _buttonsRow;
    Gtk::Box _buttonsPrimary;
    Gtk::Box _buttonsSecondary;
    Gtk::ScrolledWindow _scroller;
    Gtk::Menu _popupMenu;
    Gtk::Box _page;

    //Methods:

    ObjectsPanel(ObjectsPanel const &) = delete; // no copy
    ObjectsPanel &operator=(ObjectsPanel const &) = delete; // no assign

    void _styleButton( Gtk::Button& btn, char const* iconName, char const* tooltip );
    void _fireAction( unsigned int code );

    Gtk::MenuItem& _addPopupItem( SPDesktop *desktop, unsigned int code, int id );

    void _setVisibleIter( const Gtk::TreeModel::iterator& iter, const bool visible );
    void _setLockedIter( const Gtk::TreeModel::iterator& iter, const bool locked );

    bool _handleButtonEvent(GdkEventButton *event);
    bool _handleKeyEvent(GdkEventKey *event);
    
    void _handleEdited(const Glib::ustring& path, const Glib::ustring& new_text);
    void _handleEditingCancelled();

    void _renameObject(Gtk::TreeModel::Row row, const Glib::ustring& name);

    void _takeAction( int val );
    bool _executeAction();
    bool _executeUpdate();

    void _setExpanded( const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& path, bool isexpanded );
    void _setCollapsed(SPGroup * group);
    
    bool select_row( Glib::RefPtr<Gtk::TreeModel> const & model, Gtk::TreeModel::Path const & path, bool b );

    void connectPopupItems();
};



} //namespace Dialogs
} //namespace UI
} //namespace Inkscape



#endif // SEEN_OBJECTS_PANEL_H

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
