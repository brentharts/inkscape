// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Inkscape - An SVG editor.
 */
/*
 * Authors:
 *   Tavmjong Bah
 *
 * Copyright (C) 2018 Authors
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 * Read the file 'COPYING' for more information.
 *
 */


#include "inkscape-window.h"
#include "inkscape.h"   // SP_ACTIVE_DESKTOP
#include "enums.h"      // PREFS_WINDOW_GEOMETRY_NONE
#include "shortcuts.h"
#include "inkscape-application.h"

#include "actions/actions-canvas-transform.h"

#include "object/sp-namedview.h"  // TODO Remove need for this!

#include "ui/dialog/dialog-container.h"
#include "ui/dialog/dialog-window.h"
#include "ui/drag-and-drop.h"  // Move to canvas?
#include "ui/interface.h" // main menu, sp_ui_close_view()

#include "ui/monitor.h" // get_monitor_geometry_at_point()

#include "ui/desktop/menubar.h"

#include "ui/drag-and-drop.h"

#include "widgets/desktop-widget.h"

using Inkscape::UI::Dialog::DialogContainer;
using Inkscape::UI::Dialog::DialogWindow;

InkscapeWindow::InkscapeWindow(SPDocument* document)
    : _document(document)
    , _app(nullptr)
{
    if (!_document) {
        std::cerr << "InkscapeWindow::InkscapeWindow: null document!" << std::endl;
        return;
    }

    _app = &(ConcreteInkscapeApplication<Gtk::Application>::get_instance());
    _app->add_window(*this);

    set_resizable(true);

    // =================== Actions ===================
    add_actions_canvas_transform(this);    // Actions to transform canvas view.

    insert_action_group("doc", document->getActionGroup());

    // =============== Build interface ===============

    // Main box
    _mainbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    _mainbox->set_name("DesktopMainBox");
    _mainbox->show();
    add(*_mainbox);

    // Desktop widget (=> MultiPaned)
    _desktop_widget = new SPDesktopWidget(_document);
    _desktop_widget->window = this;
    _desktop_widget->show();
    _desktop = _desktop_widget->desktop;

    ink_drag_setup(_desktop_widget);

    // Menu bar (must come after desktop widget creation as we need _desktop)
    // _menubar = build_menubar(_desktop);
    // _menubar->set_name("MenuBar");
    // _menubar->show_all();

    // Pallet

    // Status bar

    // _mainbox->pack_start(*_menubar, false, false);
    _mainbox->pack_start(*Gtk::manage(_desktop_widget), true, true);

    // ================== Callbacks ==================
    signal_delete_event().connect(      sigc::mem_fun(*_desktop, &SPDesktop::onDeleteUI));
    signal_window_state_event().connect(sigc::mem_fun(*_desktop, &SPDesktop::onWindowStateEvent));
    signal_focus_in_event().connect(    sigc::mem_fun(*_desktop_widget, &SPDesktopWidget::onFocusInEvent));


    // ================ Window Options ==============
    setup_view();
}

// Change a document, leaving desktop/view the same. (Eventually move all code here.)
void
InkscapeWindow::change_document(SPDocument* document)
{
    if (!_app) {
        std::cerr << "Inkscapewindow::change_document: app is nullptr!" << std::endl;
        return;
    }

    _document = document;
    _app->set_active_document(_document);

    setup_view();
    std::cout << "change_document" << std::endl;
    update_dialogs();
}

// Sets up the window and view according to user preferences and <namedview> of the just loaded document
void
InkscapeWindow::setup_view()
{
    // Make sure the GdkWindow is fully initialized before resizing/moving
    // (ensures the monitor it'll be shown on is known)
    realize();

    // Resize the window to match the document properties
    sp_namedview_window_from_document(_desktop); // This should probably be a member function here.

    // Must show before setting zoom and view! (crashes otherwise)
    //
    // Showing after resizing/moving allows the window manager to correct an invalid size/position of the window
    // TODO: This does *not* work when called from 'change_document()', i.e. when the window is already visible.
    //       This can result in off-screen windows! We previously worked around this by hiding and re-showing
    //       the window, but a call to hide() causes Inkscape to just exit since the migration to Gtk::Application
    show();

    sp_namedview_zoom_and_view_from_document(_desktop);
    sp_namedview_update_layers_from_document(_desktop);

    SPNamedView *nv = _desktop->namedview;
    if (nv && nv->lockguides) {
        nv->lockGuides();
    }
}

/**
 * Return true if this is the Cmd-Q shortcut on macOS
 */
inline bool is_Cmd_Q(GdkEventKey *event)
{
#ifdef GDK_WINDOWING_QUARTZ
    return (event->keyval == 'q' && event->state == (GDK_MOD2_MASK | GDK_META_MASK));
#else
    return false;
#endif
}

bool
InkscapeWindow::on_key_press_event(GdkEventKey* event)
{
    // Need to call base class method first or text tool won't work!
    // Intercept Cmd-Q on macOS to not bypass confirmation dialog
    bool done = !is_Cmd_Q(event) && Gtk::Window::on_key_press_event(event);
    if (done) {
        return true;
    }

    unsigned shortcut = sp_shortcut_get_for_event(event);
    return sp_shortcut_invoke (shortcut, _desktop);
}

bool
InkscapeWindow::on_focus_in_event(GdkEventFocus* event)
{
    if (_app) {
        _app->set_active_window(this);
        _app->set_active_document(_document);
        _app->set_active_view(_desktop);
        _app->set_active_selection(_desktop->selection);
        _app->windows_update(_document);
        update_dialogs();
    } else {
        std::cerr << "Inkscapewindow::on_focus_in_event: app is nullptr!" << std::endl;
    }

    return Gtk::ApplicationWindow::on_focus_in_event(event);
}

// Called when a window is closed via the 'X' in the window bar.
bool
InkscapeWindow::on_delete_event(GdkEventAny* event)
{
    if (_app) {
        _app->destroy_window(this);
    }
    return true;
};

void InkscapeWindow::on_selection_changed()
{
    if (_app) {
        _app->set_active_selection(_desktop->selection);
        update_dialogs();
    }
}

void InkscapeWindow::update_dialogs()
{
    std::vector<Gtk::Window *> windows = _app->get_windows();
    for (auto window : windows) {
        DialogWindow *dock = dynamic_cast<DialogWindow *>(window);
        if (dock) {
            dock->update_dialogs();
        }

        _desktop_widget->getContainer()->update_dialogs();
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
