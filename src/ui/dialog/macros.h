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

#include <glibmm/refptr.h>
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
#include <sigc++/signal.h>

#include "io/resource.h"
#include "preferences.h"
#include "ui/widget/panel.h"
#include "xml/document.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

// Forward decaration as they are defined below
class MacrosDragAndDropStore;

// Read only, create only or something else
enum MacrosXMLFileMODE
{
    CREATE = 1,
    READ = 2
};
/**
 * This manages XML for macros dialog
 */
class MacrosXML
{
public:
    MacrosXML(std::string &&file_name, unsigned file_mode);
    ~MacrosXML();

    /**
     * For testing if creation was successful
     */
    operator bool() const { return _xml_doc; }

    /**
     * Saves macro xml to the macros data file
     */
    bool save_xml();

    XML::Node *find_macro(const Glib::ustring &macro_name, const Glib::ustring &group_name) const;
    XML::Node *find_macro(const Glib::ustring &macro_name, XML::Node *group_ptr) const;
    XML::Node *find_group(const Glib::ustring &group_name) const;

    XML::Node *create_macro(const Glib::ustring &macro_name, const Glib::ustring &group_name);
    XML::Node *create_macro(const Glib::ustring &macro_name, XML::Node *group_ptr);
    XML::Node *create_group(const Glib::ustring &group_name);

    bool rename_node(XML::Node *node, const Glib::ustring &new_name);
    /**
     * Removes the group or macro from XML, prefer when pointer is available and valid
     */
    bool remove_node(XML::Node *node);

    /**
     * Removes the group from XML, slower due to search
     */
    bool remove_group(const Glib::ustring &group_name);
    /**
     * Removes the macro from XML, slower due to search
     */
    bool remove_macro(const Glib::ustring &macro_name, const Glib::ustring &group_name);

    XML::Node *move_macro(const Glib::ustring &macro_name, const Glib::ustring &old_group_name,
                          const Glib::ustring &new_group_name);
    XML::Node *move_macro(XML::Node *macro_ptr, XML::Node *new_group_ptr);

    XML::Node *get_root();
    XML::Document *get_doc();

private:
    Inkscape::XML::Document *_xml_doc;
    std::string _macros_data_filename;
};

class Macros : public UI::Widget::Panel
{
    static constexpr auto MACRO_ICON_NAME = "system-run";
    static constexpr auto CLOSED_GROUP_ICON_NAME = "folder";
    static constexpr auto OPEN_GROUP_ICON_NAME = "folder-open";

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
    void on_macro_export() const;

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
     * When selection changed make buttons active/inactive accordingly etc
     */
    void on_selection_changed();

    /**
     * Called when rows expanded/collapsed changes group icon to match
     * folder-open when expanded
     * folder      when closed
     */
    void on_tree_row_expanded_collapsed(const Gtk::TreeIter &expanded_row, const Gtk::TreePath &tree_path,
                                        const bool is_expanded);
    void on_group_macro_name_edited(const Glib::ustring &path_string, const Glib::ustring &new_text);

    bool on_macro_drag_recieved(const Gtk::TreeModel::Path &dest, Gtk::TreeModel::Path &source_path);
    bool on_macro_drag_delete(const Gtk::TreeModel::Path &source_path);
    void on_macro_drag_end(const Glib::RefPtr<Gdk::DragContext> &context);

    // Workers
    /**
     * generate macros tree from XML doc
     */
    void load_macros();

    /**
     * Removes children from the vector if parent is aready in the vector
     * @param paths sorted vector of tree paths
     * @param all_siblings_equal_parent, cosiders parent is selected all macros of a group selected
     */
    std::vector<Gtk::TreePath> remove_children_if_contains_parent(const std::vector<Gtk::TreePath> &paths,
                                                                  bool all_siblings_equal_parent = false) const;

    /**
     * If the group name exist returns an iterator to it
     */
    Gtk::TreeIter find_group(const Glib::ustring &group_name) const;
    /**
     * same as find, but create the group if it doesn't exist, if no xml_node is provided a new node will be created in
     * XML xml_node is generally provided when Gtk::TreeView is being field using XML xml_node is not provided when
     * creating a group/macro not already in XML file as in on_macro_create and in on_macro_new_group
     */
    Gtk::TreeIter create_group(const Glib::ustring &group_name, XML::Node *xml_node = nullptr);

    /**
     * Finds macro of given name in the group, and returns iterator to it
     */
    Gtk::TreeIter find_macro(const Glib::ustring &macro_name, Gtk::TreeIter group_iter) const;
    Gtk::TreeIter find_macro(const Glib::ustring &macro_name, const Glib::ustring &group_name) const;

    /**
     * Creates a new macro and return an iterator to it, if create_in_xml false only tree is updated
     */
    Gtk::TreeIter create_macro(const Glib::ustring &macro_name, Gtk::TreeIter group_iter,
                               XML::Node *xml_node = nullptr);
    Gtk::TreeIter create_macro(const Glib::ustring &macro_name, const Glib::ustring &group_name,
                               XML::Node *xml_node = nullptr);

    /**
     * If parent iter is not provided then it's a group
     */
    Glib::ustring find_available_name(const Glib::ustring &new_name, const Glib::ustring &old_name = "",
                                      Gtk::TreeIter parent_iter = Gtk::TreeIter());

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

    Glib::RefPtr<Gtk::CellRendererText> _CRName;

    MacrosXML _macros_tree_xml;

    // states
    bool _is_recording = false;

    // data
    Gtk::TreeModel::Path _new_drag_path;

    // others
    Inkscape::Preferences *_prefs;
};

class MacrosDragAndDropStore : public Gtk::TreeStore
{
protected:
    MacrosDragAndDropStore();

private:
    class MacrosModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        MacrosModelColumns()
        {
            // Order should be same as colums in glade file, and extras can be appended
            add(icon);
            add(name);
            add(node);
        }
        // Order should be same as in glade file
        Gtk::TreeModelColumn<Glib::ustring> icon;
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<XML::Node *> node;
    };

    sigc::signal<bool, const Gtk::TreeModel::Path &, Gtk::TreeModel::Path &> _macro_drag_recieved_signal;

public:
    MacrosModelColumns _tree_columns;

    static Glib::RefPtr<MacrosDragAndDropStore> create();
    sigc::signal<bool, const Gtk::TreeModel::Path &, Gtk::TreeModel::Path &> macro_drag_recieved_signal()
    {
        return _macro_drag_recieved_signal;
    }

protected:
    // Overridden virtual functions:
    bool row_draggable_vfunc(const Gtk::TreeModel::Path &path) const override;
    bool row_drop_possible_vfunc(const Gtk::TreeModel::Path &dest,
                                 const Gtk::SelectionData &selection_data) const override;

    bool drag_data_received_vfunc(const Gtk::TreeModel::Path &dest, const Gtk::SelectionData &selection_data) override;
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_DIALOG_MACROS_H
