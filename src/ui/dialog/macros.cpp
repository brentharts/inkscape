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

#include <boost/range/adaptor/reversed.hpp>
#include <functional>
#include <glib/gi18n.h>
#include <glibmm/convert.h>
#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/enums.h>
#include <gtkmm/hvbox.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/treedragdest.h>
#include <gtkmm/treepath.h>
#include <gtkmm/treeselection.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeviewcolumn.h>
#include <iostream>
#include <memory>
#include <sigc++/adaptors/bind.h>
#include <sigc++/functors/mem_fun.h>
#include <vector>

#include "gc-anchored.h"
#include "inkscape.h"
#include "io/resource.h"
#include "io/sys.h"
#include "preferences.h"
#include "ui/dialog/filedialog.h"
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
int warn(const Glib::ustring &message)
{
    Gtk::MessageDialog warn_empty(message, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
    return warn_empty.run();
};

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
    , _macros_tree_xml(Inkscape::IO::Resource::profile_path("macros-data.xml"), CREATE | READ)
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

    _CRName = Glib::RefPtr<Gtk::CellRendererText>::cast_dynamic(builder->get_object("CRName"));

    // Initialize marcos tree (actual macros)
    load_macros(); // First load into tree store for performace, avoiding frequent updates of tree
    _MacrosTree->set_model(_MacrosTreeStore);

    // Drag and Drop
    _MacrosTree->enable_model_drag_dest();
    _MacrosTree->enable_model_drag_source();
    _MacrosTreeStore->macro_drag_recieved_signal().connect(sigc::mem_fun(*this, &Macros::on_macro_drag_recieved));
    _MacrosTree->signal_drag_end().connect(sigc::mem_fun(*this, &Macros::on_macro_drag_end));

    // Search
    _MacrosTree->set_enable_search();
    _MacrosTree->set_search_column(_MacrosTreeStore->_tree_columns.name);

    // Cosmetics
    _MacrosTree->set_enable_tree_lines();
    _MacrosTreeSelection->set_mode(Gtk::SELECTION_MULTIPLE);

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

    _CRName->signal_edited().connect(sigc::mem_fun(*this, &Macros::on_group_macro_name_edited));

    // disable till something is selected
    _MacrosDelete->set_sensitive(false);
    _MacrosExport->set_sensitive(false);
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

        _MacrosTree->expand_to_path(_MacrosTreeStore->get_path(macro_iter));
        _MacrosTreeSelection->select(macro_iter);
    }
}

void Macros::on_macro_new_group()
{
    const auto new_group_name = find_available_name("group");
    const auto iter = create_group(new_group_name);
    _MacrosTree->set_cursor(_MacrosTreeStore->get_path(iter), *_MacrosTree->get_column(1), true);
}

void Macros::on_macro_delete()
{
    Gtk::MessageDialog dialog(_("Delete selected macros and groups permanently?"), true, Gtk::MESSAGE_QUESTION,
                              Gtk::BUTTONS_OK_CANCEL);

    int result = dialog.run();
    if (result == Gtk::RESPONSE_OK) {
        const auto selected_paths = remove_children_if_contains_parent(_MacrosTreeSelection->get_selected_rows());

        // For multiple deletions, we need to delete multiple rows, iterators become invalid when and preceding row is
        // deleted so iterating in reverse order
        for (const auto &selected_path : boost::adaptors::reverse(selected_paths)) {
            const auto iter = _MacrosTreeStore->get_iter(selected_path);
            const auto parent = iter->parent();
            const bool is_group = _MacrosTreeStore->iter_depth(iter) == 0;

            // only update the tree when updating the XML was successful
            if (_macros_tree_xml.remove_node(iter->get_value(_MacrosTreeStore->_tree_columns.node))) {
                _MacrosTreeStore->erase(iter);
            }

            if (not is_group and parent->children().empty()) {
                // colapse(change icon to closed parent(group) if empty
                parent->get_value(_MacrosTreeStore->_tree_columns.icon) = CLOSED_GROUP_ICON_NAME;
                // FIXME: can't use set value because ICON_name is const char * and not ustring
            }
        }
    }
}

void Macros::on_macro_import()
{
    std::cout << "Macro import not implemented" << std::endl;
}

void Macros::on_macro_export() const
{
    Glib::ustring open_path = _prefs->getString("/dialogs/macros/exportpath");

    const auto save_dialog =
        std::unique_ptr<Inkscape::UI::Dialog::FileSaveDialog>(Inkscape::UI::Dialog::FileSaveDialog::create(
            *(SP_ACTIVE_DESKTOP->getToplevel()), open_path, Inkscape::UI::Dialog::CUSTOM_TYPE,
            _("Select a filename for exporting"), "", "", Inkscape::Extension::FILE_SAVE_METHOD_EXPORT));
    save_dialog->addFileType(_("Inkscape macros (*.xml)"), ".xml");

    bool result = save_dialog->show();
    if (not result) {
        return;
    }
    Glib::ustring file_name = save_dialog->getFilename();

    _prefs->setString("/dialogs/macros/exportpath", file_name);
    MacrosXML export_xml(file_name, CREATE);

    const auto selected_paths = remove_children_if_contains_parent(_MacrosTreeSelection->get_selected_rows());

    Gtk::TreePath _last_macros_parent_path;
    XML::Node *_last_macros_new_doc_xml_ptr = nullptr;
    for (const auto &selected_path : selected_paths) {
        const auto row = *(_MacrosTreeStore->get_iter(selected_path));
        if (selected_path.size() == 1) {
            // group there won't be any children of thies group, remove_children_if_contains_parent filters them out
            XML::Node *group_xml_ptr = row[_MacrosTreeStore->_tree_columns.node];
            XML::Node *duplicate_group = group_xml_ptr->duplicate(export_xml.get_doc());

            export_xml.get_root()->appendChild(duplicate_group);
            GC::release(duplicate_group);
        } else {
            // macro
            if (_last_macros_parent_path.empty() or not _last_macros_parent_path.is_ancestor(selected_path)) {
                _last_macros_parent_path = selected_path;
                _last_macros_parent_path.up();

                const auto parent_row = *(_MacrosTreeStore->get_iter(_last_macros_parent_path));

                // create new group of same name to avoid copying children too
                _last_macros_new_doc_xml_ptr =
                    export_xml.create_group(parent_row[_MacrosTreeStore->_tree_columns.name]);
            }

            XML::Node *macro_xml_ptr = row[_MacrosTreeStore->_tree_columns.node];
            XML::Node *duplicate_macro = macro_xml_ptr->duplicate(export_xml.get_doc());

            _last_macros_new_doc_xml_ptr->appendChild(duplicate_macro);

            GC::release(duplicate_macro);
        }
    }
    export_xml.save_xml();
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
    const auto selected_paths = _MacrosTreeSelection->get_selected_rows();
    if (not selected_paths.empty()) {
        // something is selected
        _MacrosDelete->set_sensitive();
        _MacrosExport->set_sensitive();
        return;
    }
    _MacrosDelete->set_sensitive(false);
    _MacrosExport->set_sensitive(false);
}

void Macros::on_tree_row_expanded_collapsed(const Gtk::TreeIter &expanded_row, const Gtk::TreePath &tree_path,
                                            const bool is_expanded)
{
    (*expanded_row)[_MacrosTreeStore->_tree_columns.icon] = is_expanded ? OPEN_GROUP_ICON_NAME : CLOSED_GROUP_ICON_NAME;
}

void Macros::on_group_macro_name_edited(const Glib::ustring &path_string, const Glib::ustring &new_text)
{
    Gtk::TreeIter edited_row_iter = _MacrosTreeStore->get_iter(path_string);

    // is there no change?
    const auto &old_name = (*edited_row_iter)[_MacrosTreeStore->_tree_columns.name];

    if (new_text == old_name) {
        return;
    }

    if (new_text.empty()) {
        warn(_("New name can't be empty"));
        return;
    }

    Glib::ustring new_name = find_available_name(new_text, old_name, edited_row_iter->parent());

    if (_macros_tree_xml.rename_node((*edited_row_iter)[_MacrosTreeStore->_tree_columns.node], new_name)) {
        (*edited_row_iter)[_MacrosTreeStore->_tree_columns.name] = new_name;
    }
    return;
}

bool Macros::on_macro_drag_recieved(const Gtk::TreeModel::Path &dest, Gtk::TreeModel::Path &source_path)
{
    _new_drag_path = dest; // Using it in Macros::on_macro_drag_end(...)

    const Gtk::TreeIter macro_iter = _MacrosTreeStore->get_iter(source_path);

    auto new_parent_path(_new_drag_path);
    new_parent_path.up();
    Gtk::TreeIter new_group_iter = _MacrosTreeStore->get_iter(new_parent_path);

    // new and old parents are same, dragged in the same group
    if (new_group_iter.equal(macro_iter->parent())) {
        return false;
    }

    // colapse old, change old group icon to closed if it was the last child left
    if (const auto old_parent_row = *(macro_iter->parent()); old_parent_row->children().size() == 1) {
        // only single child left, which will be deleted
        old_parent_row[_MacrosTreeStore->_tree_columns.icon] = CLOSED_GROUP_ICON_NAME;
    }

    // Pickup xml pointers from tree
    XML::Node *macro_xml_ptr = macro_iter->get_value(_MacrosTreeStore->_tree_columns.node);
    XML::Node *new_group_xml_ptr = new_group_iter->get_value(_MacrosTreeStore->_tree_columns.node);

    // updating with new pointer
    macro_xml_ptr = _macros_tree_xml.move_macro(macro_xml_ptr, new_group_xml_ptr);
    macro_iter->set_value(_MacrosTreeStore->_tree_columns.node, macro_xml_ptr);

    const Glib::ustring &old_name = macro_iter->get_value(_MacrosTreeStore->_tree_columns.name);
    Glib::ustring new_name = find_available_name(old_name, "", new_group_iter);

    // if name not changed no need to rename
    if (new_name != old_name and _macros_tree_xml.rename_node(macro_xml_ptr, new_name)) {
        macro_iter->set_value(_MacrosTreeStore->_tree_columns.name, new_name);
    }

    return true;
}

void Macros::on_macro_drag_end(const Glib::RefPtr<Gdk::DragContext> & /*context*/)
{
    _MacrosTree->expand_to_path(_new_drag_path);
    _MacrosTreeSelection->select(_new_drag_path);
}

void Macros::load_macros()
{
    // needed for iterations
    const auto root = _macros_tree_xml.get_root();

    for (auto group = root->firstChild(); group; group = group->next()) {
        auto group_tree_iter = create_group(group->attribute("name"), group);
        for (auto macro = group->firstChild(); macro; macro = macro->next()) {
            // Read macros
            create_macro(macro->attribute("name"), group_tree_iter, macro);
        };
    }
}

std::vector<Gtk::TreePath> Macros::remove_children_if_contains_parent(const std::vector<Gtk::TreePath> &paths,
                                                                      bool all_siblings_equal_parent) const
{
    // TODO: Implement all_siblings_equal_parent functionality
    std::vector<Gtk::TreePath> filtered_paths;
    filtered_paths.reserve(paths.size());

    Gtk::TreePath current_parent_group;
    for (const auto &path : paths) {
        if (not current_parent_group) {
            // We haven't encountered any group that can be parent of following rows
            if (path.size() == 1) {
                // This can be a potential parent group
                current_parent_group = path;
            }
            // push in filtered_paths as it has no parent
            filtered_paths.push_back(path);
        } else {
            // we have a candidate for parent of following rows
            if (not current_parent_group.is_ancestor(path)) {
                current_parent_group.clear();
                filtered_paths.push_back(path);
            }
        }
    }

    filtered_paths.shrink_to_fit();
    return filtered_paths;
}

Gtk::TreeIter Macros::create_group(const Glib::ustring &group_name, XML::Node *xml_node)
{
    if (auto group_iter = find_group(group_name); group_iter) {
        return group_iter;
    }

    auto row = *(_MacrosTreeStore->append());
    row[_MacrosTreeStore->_tree_columns.icon] = CLOSED_GROUP_ICON_NAME;
    row[_MacrosTreeStore->_tree_columns.name] = group_name;

    // add in XML file if xml_node, append the relevant node pointer to me
    row[_MacrosTreeStore->_tree_columns.node] = (xml_node ? xml_node : _macros_tree_xml.create_group(group_name));

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

Gtk::TreeIter Macros::create_macro(const Glib::ustring &macro_name, Gtk::TreeIter group_iter, XML::Node *xml_node)
{
    if (auto macro = find_macro(macro_name, group_iter); macro) {
        return macro;
    }

    auto macro = *(_MacrosTreeStore->append(group_iter->children()));
    macro[_MacrosTreeStore->_tree_columns.icon] = MACRO_ICON_NAME;
    macro[_MacrosTreeStore->_tree_columns.name] = macro_name;

    // add in XML file
    macro[_MacrosTreeStore->_tree_columns.node] =
        (xml_node ? xml_node
                  : _macros_tree_xml.create_macro(macro_name, (*group_iter)[_MacrosTreeStore->_tree_columns.node]));

    return macro;
} // namespace Dialog

Gtk::TreeIter Macros::create_macro(const Glib::ustring &macro_name, const Glib::ustring &group_name,
                                   XML::Node *xml_node)
{
    // create_group will work same as find if the group exists
    return create_macro(macro_name, create_group(group_name), xml_node);
}

Glib::ustring Macros::find_available_name(const Glib::ustring &new_name_hint, const Glib::ustring &old_name,
                                          Gtk::TreeIter parent_iter)
{
    // determine function used for, checking name aready exists, different for macros and groups
    const auto finding_func =
        not parent_iter
            ? std::function<Gtk::TreeIter(const Glib::ustring &)>(
                  [this](const Glib::ustring &name) { return find_group(name); })
            : std::function<Gtk::TreeIter(const Glib::ustring &)>(
                  [this, &parent_iter](const Glib::ustring &name) { return find_macro(name, parent_iter); });

    auto new_name = new_name_hint;
    if (finding_func(new_name)) {
        int name_tries = 1;
        do {
            new_name = new_name_hint + " " + std::to_string(name_tries);
            if (new_name == old_name) {
                // this is for fixing a bug when an the name is already a renamed duplicate of a name of other entry
                // example:
                // a group has has children
                //
                // group
                //   - a
                //   - a(1)
                //   - a(2)
                //
                // if we rename a(2) to a, it will be renamed to a(3) finding_func searches in TreeStore where a(2)
                // still exists hence we get
                //
                // group
                //   - a
                //   - a(1)
                //   - a(3)
                return old_name;
            }
            name_tries++;
        } while (finding_func(new_name));
    }
    return new_name;
}

// MacrosXML ------------------------------------------------------------------------
MacrosXML::MacrosXML(std::string &&file_name, unsigned file_mode)
    : _macros_data_filename(file_name)
{
    Inkscape::XML::Document *doc = nullptr;
    // If read mode then execute this
    if (file_mode & READ and Inkscape::IO::file_test(_macros_data_filename.c_str(), G_FILE_TEST_EXISTS)) {
        doc = sp_repr_read_file(_macros_data_filename.c_str(), nullptr);
        // make sure it's a macro file
        if (strcmp(doc->root()->name(), "macros")) {
            GC::release(doc);
            doc = nullptr;
        }
    }

    // If create mode then execute this
    if (file_mode & CREATE and not doc) {
        // some error in parsing, create new
        doc = sp_repr_document_new("macros");

        // Add the default group when its read mode as in Macros::_macros_tree_xml
        if (file_mode & READ) {
            auto group_default = doc->createElement("group");
            group_default->setAttribute("name", _("Default"));

            doc->root()->appendChild(group_default);

            // This was created by new
            Inkscape::GC::release(group_default);

            sp_repr_save_file(doc, _macros_data_filename.c_str());
        }
    }
    // will be null if READ failed
    _xml_doc = doc;
}

MacrosXML::~MacrosXML()
{
    Inkscape::GC::release(_xml_doc);
}

bool MacrosXML::save_xml()
{
    return sp_repr_save_file(_xml_doc, _macros_data_filename.c_str());
}

XML::Node *MacrosXML::find_macro(const Glib::ustring &macro_name, const Glib::ustring &group_name) const
{
    return find_macro(macro_name, find_group(group_name));
}
XML::Node *MacrosXML::find_macro(const Glib::ustring &macro_name, XML::Node *group_ptr) const
{
    if (group_ptr) {
        auto macro_xml_iter = group_ptr->firstChild();
        for (; macro_xml_iter; macro_xml_iter = macro_xml_iter->next()) {
            if (macro_name == macro_xml_iter->attribute("name")) {
                break;
            }
        }
        return macro_xml_iter;
    }
    return nullptr;
}

XML::Node *MacrosXML::find_group(const Glib::ustring &group_name) const
{
    auto group_xml_iter = _xml_doc->root()->firstChild();
    for (; group_xml_iter; group_xml_iter = group_xml_iter->next()) {
        if (group_name == group_xml_iter->attribute("name")) {
            break;
        }
    }
    return group_xml_iter;
}
XML::Node *MacrosXML::create_macro(const Glib::ustring &macro_name, const Glib::ustring &group_name)
{
    if (auto group_ptr = find_group(group_name); group_ptr) {
        return create_macro(macro_name, group_ptr);
    }
    return create_macro(macro_name, create_group(group_name));
}
XML::Node *MacrosXML::create_macro(const Glib::ustring &macro_name, XML::Node *group_ptr)
{
    auto macro = _xml_doc->createElement("macro");
    macro->setAttribute("name", macro_name);

    group_ptr->appendChild(macro);
    save_xml();

    return Inkscape::GC::release(macro);
}

XML::Node *MacrosXML::create_group(const Glib::ustring &group_name)
{
    // not checking if group exists in XML already, as it's done already by the dialog
    auto group = _xml_doc->createElement("group");
    group->setAttribute("name", group_name);

    _xml_doc->root()->appendChild(group);
    save_xml();

    return Inkscape::GC::release(group);
}

bool MacrosXML::rename_node(XML::Node *node, const Glib::ustring &new_name)
{
    if (node) {
        node->setAttribute("name", new_name);
        save_xml();
        return true;
    }
    return false;
}

bool MacrosXML::remove_node(XML::Node *node)
{
    if (node) {
        node->parent()->removeChild(node);
        save_xml();
        return true;
    }
    return false;
}

bool MacrosXML::remove_group(const Glib::ustring &group_name)
{
    return remove_node(find_group(group_name));
}
bool MacrosXML::remove_macro(const Glib::ustring &macro_name, const Glib::ustring &group_name)
{
    return remove_node(find_macro(macro_name, group_name));
}

XML::Node *MacrosXML::move_macro(const Glib::ustring &macro_name, const Glib::ustring &old_group_name,
                                 const Glib::ustring &new_group_name)
{
    auto new_group_ptr = find_group(new_group_name);
    auto macro_ptr = find_macro(macro_name, old_group_name);

    return move_macro(macro_ptr, new_group_ptr);
}

XML::Node *MacrosXML::move_macro(XML::Node *macro_ptr, XML::Node *new_group_ptr)
{
    auto dup_macro_ptr = macro_ptr->duplicate(_xml_doc);

    new_group_ptr->appendChild(dup_macro_ptr);
    macro_ptr->parent()->removeChild(macro_ptr);

    save_xml();
    return Inkscape::GC::release(dup_macro_ptr);
}

XML::Node *MacrosXML::get_root()
{
    return _xml_doc->root();
}
XML::Document *MacrosXML::get_doc()
{
    return _xml_doc;
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

bool MacrosDragAndDropStore::drag_data_received_vfunc(const Gtk::TreeModel::Path &dest,
                                                      const Gtk::SelectionData &selection_data)
{
    Gtk::TreeModel::Path source_path;
    Gtk::TreeModel::Path::get_from_selection_data(selection_data, source_path);

    _macro_drag_recieved_signal.emit(dest, source_path);
    return Gtk::TreeDragDest::drag_data_received_vfunc(dest, selection_data);
}
} // namespace Dialog
} // namespace UI
} // namespace Inkscape
