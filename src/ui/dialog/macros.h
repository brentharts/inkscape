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
#include <gtkmm/paned.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treeiter.h>
#include <gtkmm/treemodelcolumn.h>
#include <gtkmm/treepath.h>
#include <gtkmm/treeselection.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>

#include "preferences.h"
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

    class MacrosModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        MacrosModelColumns()
        {
            // Order should be same as in glade file
            add(icon);
            add(name);
        }
        // Order should be same as in glade file
        Gtk::TreeModelColumn<Glib::ustring> icon;
        Gtk::TreeModelColumn<Glib::ustring> name;
    };

    MacrosModelColumns _tree_columns;

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

    /**
     * called when pane orientation needs to be switched
     */
    void on_toggle_direction();
    /**
     * Will be called when pane partitions are resized
     * It remebers the partition sizes using preferences
     */
    void on_resize();
    /**
     * toggle steps
     */
    void on_toggle_steps();

    /**
     * Called when rows expanded/collapsed changes group icon to match
     * folder-open when expanded
     * folder      when closed
     */
    void on_tree_row_expanded_collapsed(const Gtk::TreeIter &expanded_row, const Gtk::TreePath &tree_path,
                                        const bool is_expanded);

    // Workers
    /**
     * load the XML file and read to get macros data
     */
    void load_macros();

private: // Variables
    // Widgets
    Gtk::Button *_MacrosCreate;
    Gtk::Button *_MacrosDelete;
    Gtk::Button *_MacrosImport;
    Gtk::Button *_MacrosExport;

    Gtk::Button *_MacrosRecord;
    Gtk::Button *_MacrosPlay;

    Gtk::Button *_MacrosStepAdd;
    Gtk::Button *_MacrosStepRemove;
    Gtk::Button *_MacrosStepEdit;

    Gtk::RadioButton *_MacrosPanedHorizontal;
    Gtk::RadioButton *_MacrosPanedVertical;

    Gtk::TreeView *_MacrosTree;
    Gtk::TreeView *_MacrosStepsTree;

    Gtk::Image *_record_button_icon;

    Gtk::Box *_MacrosBase;
    Gtk::Paned *_MacrosPaned;
    Gtk::ScrolledWindow *_MacrosScrolled;

    Glib::RefPtr<Gtk::TreeStore> _MacrosTreeStore;
    Glib::RefPtr<Gtk::TreeStore> _MacrosStepStore;
    Glib::RefPtr<Gtk::TreeSelection> _MacrosTreeSelection;

    // states
    bool _is_recording = false;

    // others
    Inkscape::Preferences *_prefs;
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_DIALOG_MACROS_H
