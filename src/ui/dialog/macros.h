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

#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <gtkmm/paned.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/switch.h>
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

class MacrosDragAndDropStore;

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

protected:
private:
    // Event Handlers
    /**
     * creates a new macro in the macro tree and asks name
     */
    void on_macro_create();

    /**
     * creates a new group in the macro tree and asks name
     */
    void on_macro_new_group();

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
    void on_toggle_steps_pane();

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

    /**
     * If the group name exist returns an iterator to it
     */
    Gtk::TreeIter find_group(const Glib::ustring &group_name) const;
    /**
     * same as find, but create the group if it doesn't exist
     */
    Gtk::TreeIter create_group(const Glib::ustring &group_name);

    /**
     * Finds macro of given name in the group, and returns iterator to it
     */
    Gtk::TreeIter find_macro(const Glib::ustring &macro_name, Gtk::TreeIter group_iter) const;
    Gtk::TreeIter find_macro(const Glib::ustring &macro_name, const Glib::ustring &group_name) const;

    /**
     * Creates a new macro and return an iterator to it
     */
    Gtk::TreeIter create_macro(const Glib::ustring &macro_name, const Glib::ustring &group_name);

private: // Variables
    // Widgets
    Gtk::Button *_MacrosCreate;
    Gtk::Button *_MacrosNewGroup;
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
    Gtk::Switch *_MacrosPanedSwitch;

    Gtk::TreeView *_MacrosTree;
    Gtk::TreeView *_MacrosStepsTree;

    Gtk::Image *_record_button_icon;

    Gtk::Box *_MacrosBase;
    Gtk::Paned *_MacrosPaned;
    Gtk::Box *_MacrosSteps;
    Gtk::ScrolledWindow *_MacrosScrolled;

    Glib::RefPtr<MacrosDragAndDropStore> _MacrosTreeStore;
    Glib::RefPtr<Gtk::TreeStore> _MacrosStepStore;
    Glib::RefPtr<Gtk::TreeSelection> _MacrosTreeSelection;

    // states
    bool _is_recording = false;

    // others
    Inkscape::Preferences *_prefs;
};

class MacrosDragAndDropStore : public Gtk::TreeStore
{
protected:
    MacrosDragAndDropStore();

public:
    // FIXME: Playing safe for now make it private once final

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

    static Glib::RefPtr<MacrosDragAndDropStore> create();

protected:
    // Overridden virtual functions:
    bool row_draggable_vfunc(const Gtk::TreeModel::Path &path) const override;
    bool row_drop_possible_vfunc(const Gtk::TreeModel::Path &dest,
                                 const Gtk::SelectionData &selection_data) const override;
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_DIALOG_MACROS_H
