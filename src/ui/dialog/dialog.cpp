// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Base class for dialogs in Inkscape - implementation.
 */
/* Authors:
 *   Bryce W. Harrington <bryce@bryceharrington.org>
 *   buliabyak@gmail.com
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Gustav Broberg <broberg@kth.se>
 *
 * Copyright (C) 2004--2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm/dialog.h>

#include <gdk/gdkkeysyms.h>

#include "dialog-manager.h"

#include <utility>

#include "desktop.h"
#include "inkscape.h"
#include "verbs.h"

#include "helper/action-context.h"
#include "helper/action.h"

#include "ui/shortcuts.h"
#include "ui/interface.h"
#include "ui/monitor.h"
#include "ui/tool/event-utils.h"
#include "ui/tools/tool-base.h"
#include "ui/widget/canvas.h"

#define MIN_ONSCREEN_DISTANCE 50


namespace Inkscape {
namespace UI {
namespace Dialog {

gboolean Dialog::retransientize_again(gpointer dlgPtr)
{
    Dialog *dlg = static_cast<Dialog *>(dlgPtr);
    g_assert(dlg->_retransientize_again_timeout != 0);
    g_assert(dlg->_retransientize_suppress);
    dlg->_retransientize_again_timeout = 0;
    dlg->_retransientize_suppress = false;
    return FALSE; // so that it is only called once
}

void Dialog::retransientize_again_timeout_add()
{
    g_assert(_retransientize_again_timeout == 0);
    g_assert(_retransientize_suppress);
    _retransientize_again_timeout = g_timeout_add(120, (GSourceFunc)Dialog::retransientize_again, (gpointer)this);
}

bool Dialog::retransientize_suppress()
{
    if (_retransientize_suppress) {
        return false;
    }
    _retransientize_suppress = true;
    return true;
}

//=====================================================================

Dialog::Dialog(Behavior::BehaviorFactory behavior_factory, const char *prefs_path, int verb_num,
               Glib::ustring apply_label)
    : _user_hidden(false), 
      _hiddenF12(false),
      _prefs_path(prefs_path),
      _verb_num(verb_num),
      _title(),
      _apply_label(std::move(apply_label)),
      _desktop(nullptr),
      _is_active_desktop(true),
      _behavior(nullptr)
{

    // Set dialog title with shortcut.
    if (verb_num) {
        Inkscape::Verb *verb = Inkscape::Verb::get(verb_num);

        SPAction *action = verb->get_action(Inkscape::ActionContext());
        if (action) {

            gchar *atitle = sp_action_get_title(action);
            if (atitle) {
                _title = atitle;
            }

            Gtk::AccelKey shortcut = Inkscape::Shortcuts::getInstance().get_shortcut_from_verb(action->verb);
            if (!shortcut.is_null()) {
                Glib::ustring key = Inkscape::Shortcuts::get_label(shortcut);
                if (!key.empty()) {
                    _title += " (" + key + ")";
                }
            }
        }
    }

    _behavior = behavior_factory(*this);
    _desktop = SP_ACTIVE_DESKTOP;

    Gtk::Widget *widg = &static_cast<Gtk::Widget &>(*_behavior);
    _connections.emplace_back(
        INKSCAPE.signal_activate_desktop.connect(sigc::mem_fun(*this, &Dialog::onDesktopActivated)));
    _connections.emplace_back(INKSCAPE.signal_shut_down.connect(sigc::mem_fun(*this, &Dialog::onShutdown)));
    _connections.emplace_back(
        INKSCAPE.signal_change_theme.connect(sigc::bind(sigc::ptr_fun(&sp_add_top_window_classes), widg)));

    _connections.emplace_back(widg->signal_event().connect(sigc::mem_fun(*this, &Dialog::_onEvent)));
    _connections.emplace_back(widg->signal_key_press_event().connect(sigc::mem_fun(*this, &Dialog::_onKeyPress)));

    read_geometry();
    sp_add_top_window_classes(widg);
}

Dialog::~Dialog()
{
    if (_retransientize_again_timeout) {
        g_source_remove(_retransientize_again_timeout);
    }

    for (auto &conn : _connections) {
        conn.disconnect();
    }

    save_geometry();
    delete _behavior;
    _behavior = nullptr;
}


//---------------------------------------------------------------------


void Dialog::onDesktopActivated(SPDesktop *desktop)
{
    _is_active_desktop = (desktop == _desktop);
    _behavior->onDesktopActivated(desktop);
}

void Dialog::onShutdown()
{
    save_geometry();
    //_user_hidden = true;
    _behavior->onShutdown();
}

void Dialog::onHideF12()
{
    _hiddenF12 = true;
    _behavior->onHideF12();
}

void Dialog::onShowF12()
{
    if (_user_hidden)
        return;

    if (_hiddenF12) {
        _behavior->onShowF12();
    }

    _hiddenF12 = false;
}


inline Dialog::operator Gtk::Widget &()                          { return *_behavior; }
inline GtkWidget *Dialog::gobj()                                 { return _behavior->gobj(); }
inline void Dialog::present()                                    { _behavior->present(); }
inline Gtk::Box *Dialog::get_vbox()                             {  return _behavior->get_vbox(); }
inline void Dialog::hide()                                       { _behavior->hide(); }
inline void Dialog::show()                                       { _behavior->show(); }
inline void Dialog::show_all_children()                          { _behavior->show_all_children(); }
inline void Dialog::set_size_request(int width, int height)      { _behavior->set_size_request(width, height); }
inline void Dialog::size_request(Gtk::Requisition &requisition)  { _behavior->size_request(requisition); }
inline void Dialog::get_position(int &x, int &y)                 { _behavior->get_position(x, y); }
inline void Dialog::get_size(int &width, int &height)            { _behavior->get_size(width, height); }
inline void Dialog::resize(int width, int height)                { _behavior->resize(width, height); }
inline void Dialog::move(int x, int y)                           { _behavior->move(x, y); }
inline void Dialog::set_position(Gtk::WindowPosition position)   { _behavior->set_position(position); }
inline void Dialog::set_title(Glib::ustring title)               { _behavior->set_title(title); }
inline void Dialog::set_sensitive(bool sensitive)                { _behavior->set_sensitive(sensitive); }

Glib::SignalProxy0<void> Dialog::signal_show() { return _behavior->signal_show(); }
Glib::SignalProxy0<void> Dialog::signal_hide() { return _behavior->signal_hide(); }

void Dialog::read_geometry()
{
    _user_hidden = false;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int x = prefs->getInt(_prefs_path + "/x", -1000);
    int y = prefs->getInt(_prefs_path + "/y", -1000);
    int w = prefs->getInt(_prefs_path + "/w", 0);
    int h = prefs->getInt(_prefs_path + "/h", 0);

    // g_print ("read %d %d %d %d\n", x, y, w, h);

    // If there are stored height and width values for the dialog,
    // resize the window to match; otherwise we leave it at its default
    if (w != 0 && h != 0) {
        resize(w, h);
    }

    Gdk::Rectangle monitor_geometry = Inkscape::UI::get_monitor_geometry_primary();
    auto const screen_width  = monitor_geometry.get_width();
    auto const screen_height = monitor_geometry.get_height();

    // If there are stored values for where the dialog should be
    // located, then restore the dialog to that position.
    // also check if (x,y) is actually onscreen with the current screen dimensions
    if ( (x >= 0) && (y >= 0) && (x < (screen_width-MIN_ONSCREEN_DISTANCE)) && (y < (screen_height-MIN_ONSCREEN_DISTANCE)) ) {
        move(x, y);
    } else {
        // ...otherwise just put it in the middle of the screen
        set_position(Gtk::WIN_POS_CENTER);
    }

}


void Dialog::save_geometry()
{
    int y, x, w, h;

    get_position(x, y);
    get_size(w, h);

    // g_print ("write %d %d %d %d\n", x, y, w, h);

    if (x<0) x=0;
    if (y<0) y=0;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt(_prefs_path + "/x", x);
    prefs->setInt(_prefs_path + "/y", y);
    prefs->setInt(_prefs_path + "/w", w);
    prefs->setInt(_prefs_path + "/h", h);

}

void
Dialog::save_status(int visible, int state, int placement)
{
   // Only save dialog status for dialogs on the "last document"
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop != nullptr || !_is_active_desktop ) {
        return;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs) {
        prefs->setInt(_prefs_path + "/visible", visible);
        prefs->setInt(_prefs_path + "/state", state);
        prefs->setInt(_prefs_path + "/placement", placement);
    }
}


void Dialog::_handleResponse(int response_id)
{
    switch (response_id) {
        case Gtk::RESPONSE_CLOSE: {
            _close();
            break;
        }
    }
}

bool Dialog::_onDeleteEvent(GdkEventAny */*event*/)
{
    save_geometry();
    _user_hidden = true;

    return false;
}

bool Dialog::_onEvent(GdkEvent *event)
{
    bool ret = false;

    switch (event->type) {
        case GDK_KEY_PRESS: {
            switch (Inkscape::UI::Tools::get_latin_keyval (&event->key)) {
                case GDK_KEY_Escape: {
                    _defocus();
                    ret = true;
                    break;
                }
                case GDK_KEY_F4:
                case GDK_KEY_w:
                case GDK_KEY_W: {
                    if (Inkscape::UI::held_only_control(event->key)) {
                        _close();
                        ret = true;
                    }
                    break;
                }
                default: { // pass keypress to the canvas
                    break;
                }
            }
        }
        default:
            ;
    }

    return ret;
}

bool Dialog::_onKeyPress(GdkEventKey *event)
{
    return Inkscape::Shortcuts::getInstance().invoke_verb(event, SP_ACTIVE_DESKTOP);
}

void Dialog::_apply()
{
    g_warning("Apply button clicked for dialog [Dialog::_apply()]");
}

void Dialog::_close()
{
    _behavior->hide();
    _onDeleteEvent(nullptr);
}

void Dialog::_defocus()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;

    if (desktop) {
        Gtk::Widget *canvas = desktop->canvas;

        // make sure the canvas window is present before giving it focus
        Gtk::Window *toplevel_window = dynamic_cast<Gtk::Window *>(canvas->get_toplevel());
        if (toplevel_window)
            toplevel_window->present();

        canvas->grab_focus();
    }
}

Inkscape::Selection*
Dialog::_getSelection()
{
    return SP_ACTIVE_DESKTOP->getSelection();
}

void sp_add_top_window_classes_callback(Gtk::Widget *widg)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop) {
        Gtk::Widget *canvas = desktop->canvas;
        Gtk::Window *toplevel_window = dynamic_cast<Gtk::Window *>(canvas->get_toplevel());
        if (toplevel_window) {
            Gtk::Window *current_window = dynamic_cast<Gtk::Window *>(widg);
            if (!current_window) {
                current_window = dynamic_cast<Gtk::Window *>(widg->get_toplevel());
            }
            if (current_window) {
                if (toplevel_window->get_style_context()->has_class("dark")) {
                    current_window->get_style_context()->add_class("dark");
                    current_window->get_style_context()->remove_class("bright");
                } else {
                    current_window->get_style_context()->add_class("bright");
                    current_window->get_style_context()->remove_class("dark");
                }
                if (toplevel_window->get_style_context()->has_class("symbolic")) {
                    current_window->get_style_context()->add_class("symbolic");
                    current_window->get_style_context()->remove_class("regular");
                } else {
                    current_window->get_style_context()->remove_class("symbolic");
                    current_window->get_style_context()->add_class("regular");
                }
            }
        }
    }
}

void sp_add_top_window_classes(Gtk::Widget *widg)
{
    if (!widg) {
        return;
    }
    if (!widg->get_realized()) {
        widg->signal_realize().connect(sigc::bind(sigc::ptr_fun(&sp_add_top_window_classes_callback), widg));
    } else {
        sp_add_top_window_classes_callback(widg);
    }
}

} // namespace Dialog
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
