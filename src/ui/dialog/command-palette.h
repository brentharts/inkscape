// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * CommandPalette: Class providing Command Palette feature
 *
 * Authors:
 *     Abhay Raj Singh <abhayonlyone@gmail.com>
 *
 * Copyright (C) 2020 Autors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_DIALOG_COMMAND_PALETTE_H
#define INKSCAPE_DIALOG_COMMAND_PALETTE_H

#include <giomm/action.h>
#include <giomm/application.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/listbox.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/viewport.h>
#include <utility>
#include <vector>

#include "inkscape.h"
#include "ui/dialog/align-and-distribute.h"
#include "verbs.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

// Enables using switch case
enum class TypeOfVariant
{
    NONE,
    UNKNOWN,
    BOOL,
    INT,
    DOUBLE,
    STRING,
};

class CommandPalette
{
public: // API
    CommandPalette();
    ~CommandPalette() = default;

    CommandPalette(CommandPalette const &) = delete;            // no copy
    CommandPalette &operator=(CommandPalette const &) = delete; // no assignment

    void open();
    void close();
    void toggle();

    Gtk::Box *get_base_widget();

private: // Helpers
    using ActionPtr = Glib::RefPtr<Gio::Action>;
    using ActionPtrName = std::pair<ActionPtr, Glib::ustring>;

    // TODO: Remove when https://gitlab.com/inkscape/inkscape/-/merge_requests/1987 is merged
    /**
     * Get a list of all actions
     */
    std::vector<ActionPtrName> list_all_actions();

private: // Signal handlers
    void on_search();
    bool on_filter(Gtk::ListBoxRow *child);

    bool on_filter_key_press(GdkEventKey *evt);

    /**
     * when search bar is empty
     */
    void hide_suggestions();

    /**
     * when search bar isn't empty
     */
    void show_suggestions();

    /**
     * Creates a dialog and asks for parameter of action
     */
    bool ask_action_parameter(GdkEventButton *evt, const ActionPtrName &action);

    /**
     * Implements text matching logic
     */
    bool match_search(const Glib::ustring &subject, const Glib::ustring &search);
    /**
     * Executes Action
     */
    static bool execute_action(const ActionPtrName &action, const Glib::ustring &value);

    static TypeOfVariant get_action_variant_type(const ActionPtr &action_ptr);

private: // variables
    // Widgets
    Glib::RefPtr<Gtk::Builder> _builder;

    Gtk::Box *_CPBase;
    Gtk::Box *_CPHeader;
    Gtk::ScrolledWindow *_CPScrolled;
    Gtk::Viewport *_CPViewPort;

    Gtk::SearchEntry *_CPFilter;
    Gtk::ListBox *_CPSuggestions;

    // States
    bool _is_open = false;
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_DIALOG_COMMAND_PALETTE_H

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
