// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_UI_DIALOG_NOTEBOOK_H
#define INKSCAPE_UI_DIALOG_NOTEBOOK_H

/** @file
 * @brief A wrapper for Gtk::Notebook.
 *
 * Authors: see git history
 *   Tavmjong Bah
 *
 * Copyright (c) 2018 Tavmjong Bah, Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gdkmm/dragcontext.h>
#include <gtkmm/menu.h>
#include <gtkmm/notebook.h>
#include <gtkmm/radiomenuitem.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/widget.h>

namespace Inkscape {
namespace UI {
namespace Dialog {

class DialogContainer;

/**
 * A widget that wraps a Gtk::Notebook with dialogs as pages.
 *
 * A notebook is fixed to a specific DialogContainer which manages the dialogs inside the notebook.
 *
 * The notebook is inside a Gtk::Expander to be able to be hidden easily.
 */
class DialogNotebook : public Gtk::ScrolledWindow
{
public:
    DialogNotebook(DialogContainer *container);
    ~DialogNotebook() override;

    void add_page(Gtk::Widget &page, Gtk::Widget &tab, Glib::ustring label);
    void move_page(Gtk::Widget &page);

    // Getters
    Gtk::Notebook *get_notebook() { return &_notebook; }
    DialogContainer *get_container() { return _container; }

    // Signal handlers - Notebook
    void on_drag_end(const Glib::RefPtr<Gdk::DragContext> context);
    bool on_drag_failed(const Glib::RefPtr<Gdk::DragContext> context, Gtk::DragResult result);
    void on_page_added(Gtk::Widget *page, int page_num);
    void on_page_removed(Gtk::Widget *page, int page_num);

    // Signal handlers - Notebook menu
    void close_tab_callback();
    void close_notebook_callback();
    void move_tab_callback();

private:
    // Widgets
    DialogContainer *_container;
    Gtk::Menu _menu;
    Gtk::Notebook _notebook;
    Gtk::RadioMenuItem _labels_auto_button;

    // State variables
    int _dialog_menu_items;
    bool _labels_auto;
    std::vector<sigc::connection> _conn;
    std::multimap<Gtk::Widget *, sigc::connection> _tab_connections;

    // Signal handlers - private
    void on_menu_signal_activate(Glib::ustring);
    void on_size_allocate_scroll(Gtk::Allocation &allocation);
    void on_labels_toggled();
    bool on_button_click_event(GdkEventButton *event, Gtk::Widget *page);

    // Helpers
    void toggle_tab_labels_callback(bool show);
    void add_close_tab_callback(Gtk::Widget *page);
    void remove_close_tab_callback(Gtk::Widget *page);
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_DIALOG_NOTEBOOK_H

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
