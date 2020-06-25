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
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/searchbar.h>
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

enum class CPFilterMode
{
    SEARCH,
    INPUT, // Input arguments
    SHELL
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

    bool on_filter_escape_key_press(GdkEventKey *evt);
    bool on_filter_search_mode_key_press(GdkEventKey *evt);
    bool on_filter_input_mode_key_press(GdkEventKey *evt, const ActionPtrName &action_ptr_name);

    /**
     * when search bar is empty
     */
    void hide_suggestions();

    /**
     * when search bar isn't empty
     */
    void show_suggestions();

    bool on_operation_clicked(GdkEventButton *evt, const ActionPtrName &action);
    bool on_operation_key_press(GdkEventKey *evt, const ActionPtrName &action);

    bool on_action_fullname_clicked(GdkEventButton *evt, const Glib::ustring &action_fullname);

    /**
     * Implements text matching logic
     */
    bool match_search(const Glib::ustring &subject, const Glib::ustring &search);
    void change_cp_fiter_mode(CPFilterMode mode);

    /**
     * Executes Action
     */
    bool ask_action_parameter(const ActionPtrName &action);
    static bool execute_action(const ActionPtrName &action, const Glib::ustring &value);

    static TypeOfVariant get_action_variant_type(const ActionPtr &action_ptr);

    static std::tuple<Gtk::Label *, Gtk::Label *, Gtk::Label *> get_name_utranslated_name_desc(Gtk::ListBoxRow *child);

private: // variables
    // Widgets
    Glib::RefPtr<Gtk::Builder> _builder;

    Gtk::Box *_CPBase;
    Gtk::Box *_CPHeader;
    Gtk::ScrolledWindow *_CPScrolled;
    Gtk::Viewport *_CPViewPort;

    Gtk::SearchBar *_CPSearchBar;
    Gtk::SearchEntry *_CPFilter;
    Gtk::ListBox *_CPSuggestions;

    // Data
    int _max_height_requestable = 360;

    // States
    bool _is_open = false;

    /**
     * Remember the mode we are in helps in unecessary signal disconnection and reconnection
     * Used by change_cp_fiter_mode()
     */
    CPFilterMode _mode = CPFilterMode::INPUT;
    // Default value other than SEARCH required
    // change_cp_fiter_mode() switches between mode hence checks if it already in the target mode.
    // Constructed value is sometimes SEARCH being the first Item for now
    // change_cp_fiter_mode() never attaches the on search listener then 
    // This initialising value can be any thing ohter than the initial required mode
    // Example currently it's open in search mode

    /**
     * Stores the search connection to deactivate when not needed
     */
    sigc::connection _cp_filter_search_connection;
    /**
     * Stores the key_press connection to deactivate when not needed
     */
    sigc::connection _cp_filter_key_press_connection;
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
