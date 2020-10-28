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
#include <gtkmm/scale.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>

#include "selection.h"
#include "ui/dialog/dialog-base.h"
#include "ui/widget/filter-effect-chooser.h"
#include "ui/widget/spinbutton.h"
#include "ui/widget/style-subject.h"

class SPObject;
class SPGroup;
struct SPColorSelector;

namespace Inkscape {
namespace UI {

class SelectedColor;

namespace Dialog {

/* Custom cell renderer for type icon */
class CellRendererItemIcon : public Gtk::CellRendererPixbuf {
public:
  
    CellRendererItemIcon() :
        Glib::ObjectBase(typeid(CellRendererPixbuf)),
        Gtk::CellRendererPixbuf(),
        _property_icon(*this, "icon", Glib::RefPtr<Gdk::Pixbuf>(nullptr))
        //_property_item(*this, "item", 0)
    { } 
     
    //Glib::PropertyProxy<Glib::RefPtr<SPItem>> 
    //property_item() { return _property_item.get_proxy(); }
  
protected:
    void render_vfunc(const Cairo::RefPtr<Cairo::Context>& cr, 
                      Gtk::Widget& widget,
                      const Gdk::Rectangle& background_area,
                      const Gdk::Rectangle& cell_area,
                      Gtk::CellRendererState flags) override;
private:
  
    Glib::Property<Glib::RefPtr<Gdk::Pixbuf> > _property_icon;
    //Glib::Property<Glib::RefPtr<SPItem> > _property_item;
    std::map<const unsigned int, Glib::RefPtr<Gdk::Pixbuf> > _icon_cache;
  
};

/**
 * A panel that displays objects.
 */
class ObjectsPanel : public DialogBase
{
public:
    ObjectsPanel();
    ~ObjectsPanel() override;

    static ObjectsPanel& getInstance();

    void documentReplaced() override;

private:
    //Internal Classes:
    class ModelColumns;
    class InternalUIBounce;
    class ObjectWatcher;

    //Connections, Watchers, Trackers:

    //Document root watcher
    ObjectsPanel::ObjectWatcher* _rootWatcher;
    
    //Connection for when the desktop is destroyed (I.e. its deconstructor is called)
    sigc::connection _desktopDestroyedConnection;

    //Connection for when the document changes
    sigc::connection _documentChangedConnection;

    //Connection for when the active layer changes
    sigc::connection _documentChangedCurrentLayer;

    //Connection for when the active selection in the document changes
    sigc::connection _selectionChangedConnection;

    //Connection for when the selection in the dialog changes
    sigc::connection _selectedConnection;

    //Connections for when the opacity/blend/blur of the active selection in the document changes
    sigc::connection _isolationConnection;
    sigc::connection _opacityConnection;
    sigc::connection _blendConnection;
    sigc::connection _blurConnection;

    sigc::connection _processQueue_sig;
    sigc::connection _executeUpdate_sig;

    //Members:

    //Tree data model
    ModelColumns* _model;

    //Prevents the composite controls from updating
    bool _blockCompositeUpdate;

    //
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
    
    Glib::RefPtr<Gtk::TreeStore> _store;

    std::vector<Gtk::Widget*> _watching;
    std::vector<Gtk::Widget*> _watchingNonTop;
    std::vector<Gtk::Widget*> _watchingNonBottom;

    Gtk::TreeView _tree;
    CellRendererItemIcon *_icon_renderer;
    Gtk::CellRendererText *_text_renderer;
    Gtk::TreeView::Column *_name_column;
    Gtk::Box _buttonsRow;
    Gtk::Box _buttonsPrimary;
    Gtk::Box _buttonsSecondary;
    Gtk::ScrolledWindow _scroller;
    Gtk::Menu _popupMenu;
    Inkscape::UI::Widget::SpinButton _spinBtn;
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

    void _storeHighlightTarget(const Gtk::TreeModel::iterator& iter);
    void _storeDragSource(const Gtk::TreeModel::iterator& iter);
    bool _handleDragDrop(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time);
    void _handleEdited(const Glib::ustring& path, const Glib::ustring& new_text);
    void _handleEditingCancelled();

    void _doTreeMove();
    void _renameObject(Gtk::TreeModel::Row row, const Glib::ustring& name);

    void _pushTreeSelectionToCurrent();
    bool _selectItemCallback(const Gtk::TreeModel::iterator& iter, bool *setOpacity, bool *first_pass);
    bool _clearPrevSelectionState(const Gtk::TreeModel::iterator& iter);
    void _desktopDestroyed(SPDesktop* desktop);

    void _checkTreeSelection();

    void _blockAllSignals(bool should_block);
    void _takeAction( int val );
    bool _executeAction();
    bool _executeUpdate();

    void _setExpanded( const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& path, bool isexpanded );
    void _setCollapsed(SPGroup * group);

    bool _noSelection( Glib::RefPtr<Gtk::TreeModel> const & model, Gtk::TreeModel::Path const & path, bool b );
    bool _rowSelectFunction( Glib::RefPtr<Gtk::TreeModel> const & model, Gtk::TreeModel::Path const & path, bool b );

    void _objectsSelected(Selection *sel);
    void _updateObjectSelected(SPItem* item, bool scrollto, bool expand);
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
