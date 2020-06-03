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

#include <glibmm/refptr.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/listbox.h>
#include <gtkmm/searchentry.h>

#include "verbs.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

class CommandPalette
{
public:
    CommandPalette();
    ~CommandPalette() = default;

    CommandPalette(CommandPalette const &) = delete;            // no copy
    CommandPalette &operator=(CommandPalette const &) = delete; // no assignment

    void show();

    Gtk::Box *get_widget();

protected:
    /**
     * When close button is clicked
     */
    void close();
    void on_search();

    /**
     * when search bar is empty
     */
    void hide_suggestions();

    /**
     * when search bar isn't empty
     */
    void show_suggestions();

    /**
     * Executes verb: To be used with signals
     */
    static bool execute_verb(Verb *verb);

    /**
     * Returns the Command Palette base widget to add to overlay
     */

    Glib::RefPtr<Gtk::Builder> _builder;

    Gtk::Box *_CPBase;
    Gtk::Box *_CPHeader;
    Gtk::SearchEntry *_CPFilter;
    Gtk::ListBox *_CPSuggestionsBox;
    /* std::unique_ptr<Gtk::Box> _CPBase; */
    /* std::unique_ptr<Gtk::Box> _CPHeader; */
    /* std::unique_ptr<Gtk::SearchEntry> _CPFilter; */
    /* std::unique_ptr<Gtk::ListBox> _CPSuggestionsBox; */
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
