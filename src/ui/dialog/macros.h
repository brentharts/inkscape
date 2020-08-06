// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Macros dialog - implementation.
 * Macros group of action that can be repeated many times
 */
/* Author:
 *   Abhay Raj Singh <abhayonlyone@gmail.com>
 *
 * Copyright (C) 2020 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_MACROS_H
#define INKSCAPE_UI_DIALOG_MACROS_H

#include <gtkmm/image.h>

#include "ui/widget/panel.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

class Macros : public UI::Widget::Panel
{
public:
    Macros();
    ~Macros() override;

    Macros(Macros const &other) = delete;
    Macros operator=(Macros const &other) = delete;

    /**
     * Helper function which returns a new instance of the dialog.
     * getInstance is needed by the dialog manager (Inkscape::UI::Dialog::DialogManager).
     */
    static Macros &getInstance() { return *new Macros(); }

private:
    // Event Handlers
    /**
     * creates a new macro in the macro tree and asks name
     */
    bool on_macro_create();

    /**
     * deletes selected macro via a confirmation dialog
     */
    bool on_macro_delete();

    /**
     * Pops a file dialog to load a macro file
     */
    bool on_macro_import();
    /**
     * Pops a file dialog to export a macro file
     */
    bool on_macro_export();

    /**
     * Record new steps after currently selected operation in macro
     */
    bool on_macro_record();
    /**
     * Plays all steps of a macro
     */
    bool on_macro_play();
    /**
     * Edits a step in the macro
     */
    bool on_macro_edit();

private:
    // Variables

    // Widgets
    Gtk::Image *_MacroCreate;
    Gtk::Image *_MacroDelete;
    Gtk::Image *_MacroImport;
    Gtk::Image *_MacroExport;
    Gtk::Image *_MacroRecord;
    Gtk::Image *_MacroPlay;
    Gtk::Image *_MacroEdit;

    Gtk::Box *_MacroBase;
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_DIALOG_MACROS_H
