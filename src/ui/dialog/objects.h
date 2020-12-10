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

#include "ui/dialog/dialog-base.h"
#include "ui/widget/style-subject.h"

#include "selection.h"
#include "color-rgba.h"

using Inkscape::XML::Node;

class SPObject;
class SPGroup;
struct SPColorSelector;

namespace Inkscape {
namespace UI {
namespace Dialog {

class ObjectsPanel;
class ObjectWatcher;

enum {COL_LABEL, COL_VISIBLE, COL_LOCKED};

using SelectionState = int;
enum SelectionStates : SelectionState {
    SELECTED_NOT = 0,     // Object is NOT in desktop's selection
    SELECTED_OBJECT = 1,  // Object is in the desktop's selection
    LAYER_FOCUSED = 2,    // This layer is the desktop's focused layer
    LAYER_FOCUS_CHILD = 4 // This object is a child of the focused layer
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
    ObjectWatcher *getRootWatcher() const { return root_watcher; };

    Node *getRepr(Gtk::TreeModel::Row const &row) const;
    SPItem *getItem(Gtk::TreeModel::Row const &row) const;

    bool isDummy(Gtk::TreeModel::Row const &row) const { return getRepr(row) == nullptr; }
    bool hasDummyChildren(Gtk::TreeModel::Row const &row) const;
    bool removeDummyChildren(Gtk::TreeModel::Row const &row);

    Glib::RefPtr<Gtk::TreeStore> _store;
    ModelColumns* _model;

    bool isObserverBlocked() const { return observer_blocked != 0; }

private:
    //Internal Classes:
    class InternalUIBounce;

    ObjectWatcher* root_watcher;

    unsigned observer_blocked = 0;

    sigc::connection document_changed;
    sigc::connection selection_changed;
    sigc::connection layer_changed;

    //The current desktop, document, tree-data
    SPDesktop* _desktop;
    SPDocument* _document;
    SPObject *_layer;
    
    //Show icons in the context menu
    bool _show_contextmenu_icons;

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

    void _addBarButton(char const* iconName, char const* tooltip, int verb_id);
    void _fireAction( unsigned int code );
    
    Gtk::MenuItem& _addPopupItem(SPDesktop *desktop, unsigned int code);
    
    void toggleVisible(const Glib::ustring& path);
    void toggleLocked(const Glib::ustring& path);
    
    bool _handleButtonEvent(GdkEventButton *event);
    bool _handleKeyEvent(GdkEventKey *event);
    
    void _handleEdited(const Glib::ustring& path, const Glib::ustring& new_text);

    void _takeAction(int val);
    void _setExpanded( const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& path, bool isexpanded );
    void _setCollapsed(SPGroup * group);
    
    bool select_row( Glib::RefPtr<Gtk::TreeModel> const & model, Gtk::TreeModel::Path const & path, bool b );

    void connectPopupItems();

    bool on_drag_motion(const Glib::RefPtr<Gdk::DragContext> &, int, int, guint);
    bool on_drag_drop(const Glib::RefPtr<Gdk::DragContext> &, int, int, guint);
    void on_drag_start(const Glib::RefPtr<Gdk::DragContext> &);
    void on_drag_end(const Glib::RefPtr<Gdk::DragContext> &);
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
