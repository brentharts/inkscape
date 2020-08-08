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

#include <gtkmm/box.h>
#include <gtkmm/button.h>
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

    /* void present() override; */
    /* void setDesktop(SPDesktop *desktop) override; */

    /* Signal accessors */
    /* sigc::signal<void, int> &signalResponse() override; */
    /* sigc::signal<void> &signalPresent() override; */

    /* sigc::signal<void, SPDesktop *, SPDocument *> &signalDocumentReplaced() override; */
    /* sigc::signal<void, SPDesktop *> &signalActivateDesktop() override; */
    /* sigc::signal<void, SPDesktop *> &signalDeactiveDesktop() override; */

protected:
    /* void _apply() override; */
    /* void _handleResponse(int response_id) override; */

private:
    // Event Handlers
    /**
     * creates a new macro in the macro tree and asks name
     */
    void on_macro_create();

    /**
     * deletes selected macro via a confirmation dialog
     */
    void on_macro_delete();

    /**
     * Pops a file dialog to load a macro file
     */
    void on_macro_import();
    /**
     * Pops a file dialog to export a macro file
     */
    void on_macro_export();

    /**
     * Record new steps after currently selected operation in macro
     */
    void on_macro_record();
    /**
     * Plays all steps of a macro
     */
    void on_macro_play();
    /**
     * Edits a step in the macro
     */
    void on_macro_edit();

private: // Variables

    // Widgets
    Gtk::Button *_MacrosCreate;
    Gtk::Button *_MacrosDelete;
    Gtk::Button *_MacrosImport;
    Gtk::Button *_MacrosExport;
    Gtk::Button *_MacrosRecord;
    Gtk::Button *_MacrosPlay;
    Gtk::Button *_MacrosEdit;

    Gtk::Image* _record_button_icon;

    Gtk::Box *_MacrosBase;

    // states
    bool _is_recording = false;
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_DIALOG_MACROS_H
