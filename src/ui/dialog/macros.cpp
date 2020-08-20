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

#include "macros.h"

#include <glib/gi18n.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/enums.h>
#include <gtkmm/hvbox.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/treepath.h>
#include <gtkmm/treeselection.h>
#include <gtkmm/treestore.h>
#include <iostream>
#include <sigc++/functors/mem_fun.h>

#include "gc-anchored.h"
#include "io/resource.h"
#include "preferences.h"
#include "ui/widget/panel.h"
#include "verbs.h"
#include "xml/repr.h"
#include "xml/simple-document.h"

namespace {
/**
 * Set the orientation of `paned` to vertical or horizontal, and make the first child resizable
 * if vertical, and the second child resizable if horizontal.
 * @pre `paned` has two children
 */
void paned_set_vertical(Gtk::Paned *paned, bool vertical)
{
    paned->child_property_resize(*paned->get_child1()) = vertical;
    assert(paned->child_property_resize(*paned->get_child2()));
    paned->set_orientation(vertical ? Gtk::ORIENTATION_VERTICAL : Gtk::ORIENTATION_HORIZONTAL);
}

template <typename T>
void debug_print(T var)
{
    std::cerr << var << std::endl;
}
} // namespace

namespace Inkscape {
namespace UI {
namespace Dialog {

Macros::Macros()
    : UI::Widget::Panel("/dialogs/macros", SP_VERB_DIALOG_MACROS)
    , _prefs(Inkscape::Preferences::get())
    , _macros_data_filename(IO::Resource::get_path_string(IO::Resource::USER, IO::Resource::NONE,
                                                          "macros-data.xml")) // ~/.config/inkscape/doc/
{
    std::string gladefile = IO::Resource::get_filename_string(Inkscape::IO::Resource::UIS, "dialog-macros.glade");
    Glib::RefPtr<Gtk::Builder> builder;
    try {
        builder = Gtk::Builder::create_from_file(gladefile);
    } catch (const Glib::Error &ex) {
        g_warning("GtkBuilder file loading failed for Macros dialog");
        return;
    }

    // Linking UI
    builder->get_widget("MacrosCreate", _MacrosCreate);
    builder->get_widget("MacrosNewGroup", _MacrosNewGroup);
    builder->get_widget("MacrosDelete", _MacrosDelete);
    builder->get_widget("MacrosImport", _MacrosImport);
    builder->get_widget("MacrosExport", _MacrosExport);

    builder->get_widget("MacrosRecord", _MacrosRecord);
    builder->get_widget("MacrosPlay", _MacrosPlay);

    builder->get_widget("MacrosStepAdd", _MacrosStepAdd);
    builder->get_widget("MacrosStepRemove", _MacrosStepRemove);
    builder->get_widget("MacrosStepEdit", _MacrosStepEdit);

    builder->get_widget("MacrosPanedHorizontal", _MacrosPanedHorizontal);
    builder->get_widget("MacrosPanedVertical", _MacrosPanedVertical);
    builder->get_widget("MacrosPanedSwitch", _MacrosPanedSwitch);

    builder->get_widget("MacrosTree", _MacrosTree);
    builder->get_widget("MacrosTree", _MacrosStepsTree);

    builder->get_widget("record-icon", _record_button_icon);

    builder->get_widget("MacrosBase", _MacrosBase);
    builder->get_widget("MacrosPaned", _MacrosPaned);
    builder->get_widget("MacrosSteps", _MacrosSteps);
    builder->get_widget("MacrosScrolled", _MacrosScrolled);

    _MacrosTreeStore = MacrosDragAndDropStore::create();

    _MacrosStepStore = Glib::RefPtr<Gtk::TreeStore>::cast_dynamic(builder->get_object("MacrosStepStore"));
    _MacrosTreeSelection = Glib::RefPtr<Gtk::TreeSelection>::cast_dynamic(builder->get_object("MacrosTreeSelection"));

    // XML loading
    load_xml();

    // Initialize marcos tree (actual macros)
    load_macros(); // First load into tree store for performace, avoiding frequent updates of tree
    _MacrosTree->set_model(_MacrosTreeStore);

    // enable drag and drop
    _MacrosTree->enable_model_drag_dest();
    _MacrosTree->enable_model_drag_source();

    // Search
    _MacrosTree->set_enable_search();
    _MacrosTree->set_search_column(_MacrosTreeStore->_tree_columns.name);

    // Cosmetics
    _MacrosTree->set_enable_tree_lines();

    // Setup panes
    {
        const bool is_vertical = _prefs->getBool("/dialogs/macros/orientation", true);
        paned_set_vertical(_MacrosPaned, is_vertical);
        if (is_vertical) {
            _MacrosPanedVertical->set_active();
        } else {
            _MacrosPanedHorizontal->set_active();
        }

        const bool show_steps = _prefs->getBool("/dialogs/macros/showsteps", true);
        _MacrosPanedSwitch->set_state(show_steps);
        on_toggle_steps_pane();

        const int panedpos = _prefs->getInt("/dialogs/macros/panedpos", 180);
        _MacrosPaned->property_position() = panedpos;
    }

    // Adding signals
    _MacrosCreate->signal_clicked().connect(sigc::mem_fun(*this, &Macros::on_macro_create));
    _MacrosNewGroup->signal_clicked().connect(sigc::mem_fun(*this, &Macros::on_macro_new_group));
    _MacrosDelete->signal_clicked().connect(sigc::mem_fun(*this, &Macros::on_macro_delete));
    _MacrosImport->signal_clicked().connect(sigc::mem_fun(*this, &Macros::on_macro_import));
    _MacrosExport->signal_clicked().connect(sigc::mem_fun(*this, &Macros::on_macro_export));

    _MacrosRecord->signal_clicked().connect(sigc::mem_fun(*this, &Macros::on_macro_record));
    _MacrosPlay->signal_clicked().connect(sigc::mem_fun(*this, &Macros::on_macro_play));
    _MacrosStepEdit->signal_clicked().connect(sigc::mem_fun(*this, &Macros::on_macro_edit));

    _MacrosPanedVertical->signal_toggled().connect(sigc::mem_fun(*this, &Macros::on_toggle_direction));
    _MacrosPanedSwitch->property_active().signal_changed().connect(sigc::mem_fun(*this, &Macros::on_toggle_steps_pane));
    _MacrosPaned->property_position().signal_changed().connect(sigc::mem_fun(*this, &Macros::on_resize));

    _MacrosTree->signal_row_expanded().connect(
        sigc::bind(sigc::mem_fun(*this, &Macros::on_tree_row_expanded_collapsed), true));
    _MacrosTree->signal_row_collapsed().connect(
        sigc::bind(sigc::mem_fun(*this, &Macros::on_tree_row_expanded_collapsed), false));

    // disable till something is selected
    _MacrosDelete->set_sensitive(false);
    _MacrosTreeSelection->signal_changed().connect(sigc::mem_fun(*this, &Macros::on_selection_changed));

    _setContents(_MacrosBase);
    show_all_children();
}

Macros::~Macros() {}

// Listeners
void Macros::on_macro_create()
{
    Gtk::Dialog dialog(_("Create new Macro"), Gtk::DIALOG_MODAL | Gtk::DIALOG_USE_HEADER_BAR);
    Gtk::Entry name_entry;
    Gtk::ComboBoxText group_combo(true);

    // pick groups from macro tree
    for (const auto &iter : _MacrosTreeStore->children()) {
        group_combo.append(iter[_MacrosTreeStore->_tree_columns.name]);
    }

    auto box = dialog.get_content_area();

    name_entry.set_placeholder_text(_("Enter macro name"));
    name_entry.property_margin_bottom() = 12;

    group_combo.get_entry()->set_placeholder_text(_("Enter or select group name"));

    Gtk::Label name_label(_("Macro name"), Gtk::ALIGN_START);
    box->pack_start(name_label);
    box->pack_start(name_entry);

    Gtk::Label group_label(_("Group name"), Gtk::ALIGN_START);
    box->pack_start(group_label);
    box->pack_start(group_combo);

    box->set_valign(Gtk::ALIGN_START);

    box->set_size_request(300);
    box->property_margin() = 12;

    dialog.add_button(_("Cancel"), Gtk::RESPONSE_CANCEL);
    dialog.add_button(_("Create"), Gtk::RESPONSE_OK);

    dialog.show_all();

    int result = dialog.run();

    if (result == Gtk::RESPONSE_OK and not name_entry.get_text().empty()) {
        Glib::ustring macro_name = name_entry.get_text();

        // Set folder as "Default" if combo box is empty
        Glib::ustring macro_group =
            (not group_combo.get_entry_text().empty() ? group_combo.get_entry_text() : _("Default"));

        auto macro_iter = create_macro(macro_name, macro_group);
        // TODO: Also create in XML tree

        _MacrosTree->expand_to_path(_MacrosTreeStore->get_path(macro_iter));
        _MacrosTreeSelection->select(macro_iter);
    }
}

void Macros::on_macro_new_group()
{
    Gtk::Dialog dialog(_("Create new group"), Gtk::DIALOG_MODAL | Gtk::DIALOG_USE_HEADER_BAR);

    Gtk::Entry group_name_entry;

    dialog.get_content_area()->pack_start(group_name_entry);
    dialog.set_size_request(300);
    dialog.get_content_area()->property_margin() = 12;

    dialog.add_button(_("Cancel"), Gtk::RESPONSE_CANCEL);
    dialog.add_button(_("Create"), Gtk::RESPONSE_OK);

    dialog.show_all();

    int result = dialog.run();
    if (result == Gtk::RESPONSE_OK and not group_name_entry.get_text().empty()) {
        // TODO: create in XML too
        auto iter = create_group(group_name_entry.get_text());
        _MacrosTreeSelection->select(iter);
    }
}

void Macros::on_macro_delete()
{
    // TODO: Determine the selection first
    Gtk::MessageDialog dialog(_("Delete selected macros permanently?"), true, Gtk::MESSAGE_QUESTION,
                              Gtk::BUTTONS_OK_CANCEL);

    int result = dialog.run();
    if (result == Gtk::RESPONSE_OK) {
        // TODO: Support multiple deletions
        auto iter = _MacrosTreeSelection->get_selected();
        auto parent = iter->parent();
        _MacrosTreeStore->erase(iter);

        // colapse(change icon to collapsed) parent(group) if no child left
        if (parent->children().empty()) {
            (*parent)[_MacrosTreeStore->_tree_columns.icon] = "folder";
        }

        // TODO: Also remove from macro tree
    }
}

void Macros::on_macro_import()
{
    std::cout << "Macro import not implemented" << std::endl;
}

void Macros::on_macro_export()
{
    std::cout << "Macro export not implemented" << std::endl;
}

void Macros::on_macro_record()
{
    // Add recording logic
    if (_is_recording) {
        // In recording, stop and change button to Record
        _MacrosRecord->set_tooltip_text(_("Record"));
        _record_button_icon->set_from_icon_name("media-record", Gtk::ICON_SIZE_BUTTON);

        _is_recording = false;
        return;
    }

    // Not recording , start recording and change button to Record
    _MacrosRecord->set_tooltip_text(_("Stop Recording"));
    _record_button_icon->set_from_icon_name("media-playback-stop", Gtk::ICON_SIZE_BUTTON);
    _is_recording = true;
}

void Macros::on_macro_play()
{
    std::cout << "Macro play not implemented" << std::endl;
}

void Macros::on_macro_edit()
{
    std::cout << "Macro edit not implemented" << std::endl;
}

void Macros::on_toggle_direction()
{
    const bool is_vertical = _MacrosPanedVertical->get_active();
    _prefs->setBool("/dialogs/macros/orientation", is_vertical);
    paned_set_vertical(_MacrosPaned, is_vertical);
    _prefs->setInt("/dialogs/macros/panedpos", _MacrosPaned->property_position());
}

void Macros::on_resize()
{
    _prefs->setInt("/dialogs/macros/panedpos", _MacrosPaned->property_position());
}

void Macros::on_toggle_steps_pane()
{
    const bool show_steps = _MacrosPanedSwitch->get_state();
    _prefs->setBool("/dialogs/macros/showsteps", show_steps);
    if (show_steps) {
        _MacrosSteps->set_no_show_all(false);
        _MacrosSteps->show_all();

        _MacrosPanedVertical->set_sensitive();
        _MacrosPanedHorizontal->set_sensitive();

        return;
    }
    _MacrosSteps->set_no_show_all();
    _MacrosSteps->hide();

    _MacrosPanedVertical->set_sensitive(false);
    _MacrosPanedHorizontal->set_sensitive(false);
}

void Macros::on_selection_changed()
{
    const auto iter = _MacrosTreeSelection->get_selected();
    if (iter) { // something is selected
        _MacrosDelete->set_sensitive();
        return;
    }
    _MacrosDelete->set_sensitive(false);
}

void Macros::on_tree_row_expanded_collapsed(const Gtk::TreeIter &expanded_row, const Gtk::TreePath &tree_path,
                                            const bool is_expanded)
{
    (*expanded_row)[_MacrosTreeStore->_tree_columns.icon] = is_expanded ? "folder-open" : "folder";
}

void Macros::load_xml()
{
    auto doc = sp_repr_read_file(_macros_data_filename.c_str(), nullptr);
    if (not doc) {
        debug_print("Creating new file");
        doc = sp_repr_document_new("macros");

        // Add the default group
        auto group_default = doc->createElement("group");
        group_default->setAttribute("name", "default");

        // Just a pointer, we don't own it, don't free/release/delete
        auto root = doc->root();
        root->appendChild(group_default);

        // This was created by new
        Inkscape::GC::release(group_default);

        sp_repr_save_file(doc, _macros_data_filename.c_str());
    }
    _macros_xml = doc;
}

void Macros::load_macros()
{
    auto root = _macros_xml->root();

    auto group = root->firstChild();
    while (group) {
        auto group_tree_iter = create_group(group->attribute("name"), false);

        // Read macros
        auto macro = group->firstChild();
        while (macro) {
            create_macro(macro->attribute("name"), group_tree_iter, false);
            macro = macro->next();
        }

        group = group->next();
    }
}

bool Macros::save_xml()
{
    return sp_repr_save_file(_macros_xml, _macros_data_filename.c_str());
}

Gtk::TreeIter Macros::create_group(const Glib::ustring &group_name, bool create_in_xml)
{
    if (auto group_iter = find_group(group_name); group_iter) {
        return group_iter;
    }

    auto row = *(_MacrosTreeStore->append());
    row[_MacrosTreeStore->_tree_columns.icon] = "folder";
    row[_MacrosTreeStore->_tree_columns.name] = group_name;

    // add in XML file
    if (create_in_xml) {
        auto group = _macros_xml->createElement("group");
        group->setAttribute("name", group_name);

        _macros_xml->root()->appendChild(group);
        Inkscape::GC::release(group);

        save_xml();
    }

    return row;
}

Gtk::TreeIter Macros::find_group(const Glib::ustring &group_name) const
{
    auto groups = _MacrosTreeStore->children();

    // FIXME: Use std::find
    for (const auto &group : groups) {
        if (group[_MacrosTreeStore->_tree_columns.name] == group_name) {
            return group;
        }
    }

    return Gtk::TreeIter(); // invalid
}

Gtk::TreeIter Macros::find_macro(const Glib::ustring &macro_name, Gtk::TreeIter group_iter) const
{
    auto group_children = group_iter->children();

    // FIXME: Use std::find
    for (const auto &child : group_children) {
        if (child[_MacrosTreeStore->_tree_columns.name] == macro_name) {
            return child;
        }
    }

    return Gtk::TreeIter(); // invalid
}

Gtk::TreeIter Macros::find_macro(const Glib::ustring &macro_name, const Glib::ustring &group_name) const
{
    return find_macro(macro_name, find_group(group_name));
}

Gtk::TreeIter Macros::create_macro(const Glib::ustring &macro_name, Gtk::TreeIter group_iter, bool create_in_xml)
{
    if (auto macro = find_macro(macro_name, group_iter); macro) {
        return macro;
    }

    auto macro = *(_MacrosTreeStore->append(group_iter->children()));
    macro[_MacrosTreeStore->_tree_columns.icon] = "system-run";
    macro[_MacrosTreeStore->_tree_columns.name] = macro_name;

    // add in XML file
    if (create_in_xml) {
        auto macro_node = _macros_xml->createElement("macro");
        macro_node->setAttribute("name", macro_name);

        // find the group to append to
        auto group = _macros_xml->root()->firstChild();
        for (; group; group = group->next()) {
            if (group->attribute("name") == (*group_iter)[_MacrosTreeStore->_tree_columns.name]) {
                break;
            }
        }

        group->appendChild(macro_node);
        Inkscape::GC::release(macro_node);

        save_xml();
    }

    return macro;
} // namespace Dialog

Gtk::TreeIter Macros::create_macro(const Glib::ustring &macro_name, const Glib::ustring &group_name, bool create_in_xml)
{
    // create_group will work same as find if the group exists
    return create_macro(macro_name, create_group(group_name), create_in_xml);
}

// MacrosDragAndDropStore ------------------------------------------------------------
MacrosDragAndDropStore::MacrosDragAndDropStore()
{
    // We can't just call Gtk::TreeModel(m_Columns) in the initializer list
    // because m_Columns does not exist when the base class constructor runs.
    // And we can't have a static m_Columns instance, because that would be
    // instantiated before the gtkmm type system.
    // So, we use this method, which should only be used just after creation:
    set_column_types(_tree_columns);
}

Glib::RefPtr<MacrosDragAndDropStore> MacrosDragAndDropStore::create()
{
    return Glib::RefPtr<MacrosDragAndDropStore>(new MacrosDragAndDropStore());
}

bool MacrosDragAndDropStore::row_draggable_vfunc(const Gtk::TreeModel::Path &path) const
{
    return path.size() == 2; // Row nested in rows, macros in groups/folder
}

bool MacrosDragAndDropStore::row_drop_possible_vfunc(const Gtk::TreeModel::Path &dest_path,
                                                     const Gtk::SelectionData &selection_data) const
{
    return dest_path.size() == 2; // within folder: here dest path means, path achieved when drop successful
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape
