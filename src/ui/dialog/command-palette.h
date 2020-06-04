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
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/viewport.h>
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
    Gtk::Box* get_widget();

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
    bool execute_verb(Verb* verb);
    Glib::RefPtr<Gtk::Builder> _builder;

    Gtk::Box* _CPBase;
    Gtk::Box* _CPHeader;
    Gtk::ScrolledWindow* _CPScrolled;
    Gtk::Viewport* _CPViewPort;

    Gtk::SearchEntry* _CPFilter;
    Gtk::ListBox* _CPSuggestions;
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
