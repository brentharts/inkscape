// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Dialog for adding a live path effect.
 *
 * Author:
 * Abhay Raj Singh <abhayonlyone@gmail.com>
 *
 * Copyright (C) 2020 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "command-palette.h"

#include <cstddef>
#include <cstring>
#include <ctime>
#include <gdk/gdkkeysyms.h>
#include <giomm/action.h>
#include <giomm/application.h>
#include <giomm/file.h>
#include <giomm/fileinfo.h>
#include <glib/gi18n.h>
#include <glibconfig.h>
#include <glibmm/convert.h>
#include <glibmm/date.h>
#include <glibmm/error.h>
#include <glibmm/i18n.h>
#include <glibmm/markup.h>
#include <glibmm/ustring.h>
#include <goo/gmem.h>
#include <gtkmm/application.h>
#include <gtkmm/box.h>
#include <gtkmm/enums.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/label.h>
#include <gtkmm/recentinfo.h>
#include <iostream>
#include <iterator>
#include <memory>
#include <ostream>
#include <readline/history.h>
#include <sigc++/adaptors/bind.h>
#include <sigc++/functors/mem_fun.h>
#include <string>

#include "actions/actions-extra-data.h"
#include "file.h"
#include "gc-anchored.h"
#include "inkscape-application.h"
#include "inkscape-window.h"
#include "inkscape.h"
#include "io/resource.h"
#include "object/uri.h"
#include "preferences.h"
#include "ui/interface.h"
#include "verbs.h"
#include "xml/repr.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

namespace {
template <typename T>
void debug_print(T variable)
{
    std::cerr << variable << std::endl;
}
} // namespace

// constructor
CommandPalette::CommandPalette()
{
    // setup _builder
    {
        auto gladefile = get_filename_string(Inkscape::IO::Resource::UIS, "command-palette-main.glade");
        try {
            _builder = Gtk::Builder::create_from_file(gladefile);
        } catch (const Glib::Error &ex) {
            g_warning("Glade file loading failed for command palette dialog");
            return;
        }
    }

    // Setup Base UI Components
    _builder->get_widget("CPBase", _CPBase);
    _builder->get_widget("CPHeader", _CPHeader);
    _builder->get_widget("CPListBase", _CPListBase);

    _builder->get_widget("CPSearchBar", _CPSearchBar);
    _builder->get_widget("CPFilter", _CPFilter);

    _builder->get_widget("CPSuggestions", _CPSuggestions);
    _builder->get_widget("CPHistory", _CPHistory);

    _builder->get_widget("CPSuggestionsScroll", _CPSuggestionsScroll);
    _builder->get_widget("CPHistoryScroll", _CPHistoryScroll);

    _CPBase->add_events(Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
                        Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK | Gdk::KEY_PRESS_MASK);

    // TODO: Customise on user language RTL, LTR or better user preference
    _CPBase->set_halign(Gtk::ALIGN_CENTER);
    _CPBase->set_valign(Gtk::ALIGN_START);

    _CPFilter->signal_key_press_event().connect(sigc::mem_fun(*this, &CommandPalette::on_key_press_cpfilter_escape),
                                                false);
    set_mode(CPMode::SEARCH);

    _CPSuggestions->set_activate_on_single_click();
    _CPSuggestions->set_selection_mode(Gtk::SELECTION_SINGLE);

    // Preferences load
    auto prefs = Inkscape::Preferences::get();
    const auto app = Gio::Application::get_default();
    const auto iapp = dynamic_cast<InkscapeApplication *>(app.get());
    const auto gapp = dynamic_cast<Gtk::Application *>(app.get());

    // Setup operations [actions, verbs, extenstions]
    {
        auto all_actions_ptr_name = list_all_actions();

        // setup actions - can’t do const
        for (/*const*/ auto &action_ptr_name : all_actions_ptr_name) {
            generate_action_operation(action_ptr_name, true);
        }

        // setup recent files
        {
            auto recent_manager = Gtk::RecentManager::get_default();
            auto recent_files = recent_manager->get_items();

            int max_files = Inkscape::Preferences::get()->getInt("/options/maxrecentdocuments/value");

            for (auto const &recent_file : recent_files) {
                bool valid_file = recent_file->has_application(g_get_prgname()) or
                                  recent_file->has_application("org.inkscape.Inkscape") or
                                  recent_file->has_application("inkscape") or
                                  recent_file->has_application("inkscape.exe");

                valid_file = valid_file and recent_file->exists();

                if (not valid_file) {
                    continue;
                }

                if (max_files-- <= 0) {
                    break;
                }

                append_recent_file_operation(recent_file->get_uri_display(), true, false); // open
                append_recent_file_operation(recent_file->get_uri_display(), true, true);
            }
        }
    }

    // History managment
    {
        auto file_name = Inkscape::IO::Resource::profile_path("cphistory.xml");
        auto file = Gio::File::create_for_path(file_name);
        if (file->query_exists()) {
            char *contents = nullptr;
            gsize length = 0;

            file->load_contents(contents, length);
            // length is set by the function ignoring last '\0' hence contents[length] is '\0'

            if (length != 0 and contents != nullptr) {
                // most recent first
                std::istringstream lines(contents);
                for (std::string line; std::getline(lines, line, '\n');) {
                    auto type = line.substr(0, line.find(':'));
                    auto data = line.substr(line.find(':') + 1);

                    if (type == "ACTION") {
                        generate_action_operation(get_action_ptr_name(data), false);
                    } else if (type == "OPEN_FILE") {
                        append_recent_file_operation(data, false, false);
                    } else if (type == "IMPORT_FILE") {
                        append_recent_file_operation(data, false, true);
                    }
                }
            }

            g_free(contents);
        }
        _history_file_output_stream = file->append_to();
    }
    _CPSuggestions->signal_row_activated().connect(sigc::mem_fun(*this, &CommandPalette::on_row_activated));
}

void CommandPalette::open()
{
    _CPBase->show_all();
    _CPFilter->grab_focus();
    _is_open = true;
}

void CommandPalette::close()
{
    _CPBase->hide();

    // Reset filtering
    _CPFilter->set_text("");
    _CPSuggestions->invalidate_filter();

    set_mode(CPMode::SEARCH);

    _is_open = false;
}

void CommandPalette::toggle()
{
    if (not _is_open) {
        open();
        return;
    } else {
        close();
    }
}

void CommandPalette::append_recent_file_operation(const Glib::ustring &path, bool is_suggestion, bool is_import)
{
    static const auto gladefile =
        get_filename_string(Inkscape::IO::Resource::UIS, "command-palette-operation-next-full-action.glade");
    Glib::RefPtr<Gtk::Builder> operation_builder;
    try {
        operation_builder = Gtk::Builder::create_from_file(gladefile);
    } catch (const Glib::Error &ex) {
        g_warning("Glade file loading failed for Command Palette operation dialog");
    }

    // declaring required widgets pointers
    Gtk::EventBox *CPOperation;
    Gtk::Box *CPSynapseBox;

    Gtk::Label *CPGroup;
    Gtk::Label *CPName;
    Gtk::Label *CPShortcut;
    Gtk::Button *CPActionFullName;
    Gtk::Label *CPDescription;

    // Reading widgets
    operation_builder->get_widget("CPOperation", CPOperation);
    operation_builder->get_widget("CPSynapseBox", CPSynapseBox);

    operation_builder->get_widget("CPGroup", CPGroup);
    operation_builder->get_widget("CPName", CPName);
    operation_builder->get_widget("CPShortcut", CPShortcut);
    operation_builder->get_widget("CPActionFullName", CPActionFullName);
    operation_builder->get_widget("CPDescription", CPDescription);

    const auto file = Gio::File::create_for_path(path);
    if (file->query_exists()) {
        const Glib::ustring file_name = file->get_basename();

        if (is_import) {
            // Used for Activate row signal of listbox and not
            CPGroup->set_text("import");
            CPActionFullName->set_label("import"); // For filtering only

        } else {
            CPGroup->set_text("open");
            CPActionFullName->set_label("open"); // For filtering only
        }

        // Hide for recent_file, not required
        CPActionFullName->set_no_show_all();
        CPActionFullName->hide();

        CPName->set_text((is_import ? _("Import") : _("Open")) + (": " + file_name));
        CPDescription->set_text(path);
        CPDescription->set_tooltip_text(path);

        {
            auto mod_time = file->query_info()->get_modification_date_time();
            // Using this to reduce instead of ActionFullName widget because fullname is searched
            CPShortcut->set_text(mod_time.format("%d %b %R"));
        }
        // Add to suggestions
        if (is_suggestion) {
            _CPSuggestions->append(*CPOperation);
        } else {
            _CPHistory->append(*CPOperation);
        }
    }
}

bool CommandPalette::generate_action_operation(const ActionPtrName &action_ptr_name, bool is_suggestion)
{
    static const auto app = dynamic_cast<InkscapeApplication *>(Gio::Application::get_default().get());
    static const auto gapp = dynamic_cast<Gtk::Application *>(Gio::Application::get_default().get());
    static InkActionExtraData &action_data = app->get_action_extra_data();
    static const bool show_full_action_name =
        Inkscape::Preferences::get()->getBool("/options/commandpalette/showfullactionname/value");
    static const auto gladefile =
        get_filename_string(Inkscape::IO::Resource::UIS, "command-palette-operation-next-full-action.glade");

    Glib::RefPtr<Gtk::Builder> operation_builder;
    try {
        operation_builder = Gtk::Builder::create_from_file(gladefile);
    } catch (const Glib::Error &ex) {
        g_warning("Glade file loading failed for Command Palette operation dialog");
        return false;
    }

    // declaring required widgets pointers
    Gtk::EventBox *CPOperation;
    Gtk::Box *CPSynapseBox;

    Gtk::Label *CPGroup;
    Gtk::Label *CPName;
    Gtk::Label *CPShortcut;
    Gtk::Label *CPDescription;
    Gtk::Button *CPActionFullName;

    // Reading widgets
    operation_builder->get_widget("CPOperation", CPOperation);
    operation_builder->get_widget("CPSynapseBox", CPSynapseBox);

    operation_builder->get_widget("CPGroup", CPGroup);
    operation_builder->get_widget("CPName", CPName);
    operation_builder->get_widget("CPShortcut", CPShortcut);
    operation_builder->get_widget("CPActionFullName", CPActionFullName);
    operation_builder->get_widget("CPDescription", CPDescription);

    CPGroup->set_text(action_data.get_section_for_action(action_ptr_name.second));

    // Setting CPName
    {
        auto name = action_data.get_label_for_action(action_ptr_name.second);
        if (name.empty()) {
            // If action doesn't have a label, set the name = full action name
            name = action_ptr_name.second;
        }

        // FIXME: Apply actual logic
        auto untranslated_name = name;

        CPName->set_text(name);
        CPName->set_tooltip_text(untranslated_name);
    }

    {
        CPActionFullName->set_label(action_ptr_name.second);

        if (not show_full_action_name) {
            CPActionFullName->set_no_show_all();
            CPActionFullName->hide();
        } else {
            CPActionFullName->signal_clicked().connect(
                sigc::bind<Glib::ustring>(sigc::mem_fun(*this, &CommandPalette::on_action_fullname_clicked),
                                          action_ptr_name.second),
                false);
        }
    }

    {
        std::vector<Glib::ustring> accels = gapp->get_accels_for_action(action_ptr_name.second);
        std::stringstream ss;
        for (const auto &accel : accels) {
            ss << accel << ',';
        }
        std::string accel_label = ss.str();

        if (not accel_label.empty()) {
            accel_label.pop_back();
            CPShortcut->set_text(accel_label);
        } else {
            CPShortcut->set_no_show_all();
            CPShortcut->hide();
        }
    }

    CPDescription->set_text(action_data.get_tooltip_for_action(action_ptr_name.second));
    CPDescription->set_tooltip_text(action_data.get_tooltip_for_action(action_ptr_name.second));

    // Add to suggestions
    if (is_suggestion) {
        _CPSuggestions->append(*CPOperation);
    } else {
        _CPHistory->append(*CPOperation);
    }

    return true;
}

void CommandPalette::on_search()
{
    _search_text = _CPFilter->get_text();
    _CPSuggestions->invalidate_filter();
    if (auto top_row = _CPSuggestions->get_row_at_y(0); top_row) {
        _CPSuggestions->select_row(*top_row); // select top row
    }
}

bool CommandPalette::on_filter_general(Gtk::ListBoxRow *child)
{
    if (_search_text.empty()) {
        return true;
    } // Every operation is visible

    auto [CPName, CPDescription] = get_name_desc(child);

    if (CPName && match_search(CPName->get_text(), _search_text)) {
        return true;
    }
    if (CPName && match_search(CPName->get_tooltip_text(), _search_text)) {
        // untranslated name
        return true;
    }
    if (CPDescription && match_search(CPDescription->get_text(), _search_text)) {
        return true;
    }
    return false;
}

bool CommandPalette::on_filter_full_action_name(Gtk::ListBoxRow *child)
{
    if (auto CPActionFullName = get_full_action_name(child);
        CPActionFullName and _search_text == CPActionFullName->get_label()) {
        return true;
    }
    return false;
}

bool CommandPalette::on_filter_recent_file(Gtk::ListBoxRow *child, bool const is_import)
{
    auto CPActionFullName = get_full_action_name(child);
    if (is_import) {
        if (CPActionFullName and CPActionFullName->get_label() == "import") {
            auto [CPName, CPDescription] = get_name_desc(child);
            if (CPDescription && CPDescription->get_text() == _search_text) {
                return true;
            }
        }
        return false;
    }
    if (CPActionFullName and CPActionFullName->get_label() == "open") {
        auto [CPName, CPDescription] = get_name_desc(child);
        if (CPDescription && CPDescription->get_text() == _search_text) {
            return true;
        }
    }
    return false;
}

bool CommandPalette::on_key_press_cpfilter_escape(GdkEventKey *evt)
{
    if (evt->keyval == GDK_KEY_Escape || evt->keyval == GDK_KEY_question) {
        close();
        return true; // stop propagation of key press, not needed anymore
    }
    return false; // Pass the key event which are not used
}

bool CommandPalette::on_key_press_cpfilter_search_mode(GdkEventKey *evt)
{
    auto key = evt->keyval;
    if (key == GDK_KEY_Return or key == GDK_KEY_Linefeed) {
        if (auto selected_row = _CPSuggestions->get_selected_row(); selected_row) {
            selected_row->activate();
        }
        return true;
    } else if (key == GDK_KEY_Up) {
        if (not _CPHistory->get_children().empty()) {
            set_mode(CPMode::HISTORY);
            return true;
        }
    }
    return false;
}

bool CommandPalette::on_key_press_cpfilter_history_mode(GdkEventKey *evt)
{
    if (evt->keyval == GDK_KEY_BackSpace) {
        return true;
    }
    return false;
}

/**
 * Executes action when enter pressed
 */
bool CommandPalette::on_key_press_cpfilter_input_mode(GdkEventKey *evt, const ActionPtrName &action_ptr_name)
{
    switch (evt->keyval) {
        case GDK_KEY_Return:
            [[fallthrough]];
        case GDK_KEY_Linefeed:
            execute_action(action_ptr_name, _CPFilter->get_text());
            close();
            return true;
    }
    return false;
}

void CommandPalette::hide_suggestions()
{
    _CPBase->set_size_request(-1, 10);
    _CPListBase->hide();
}
void CommandPalette::show_suggestions()
{
    _CPBase->set_size_request(-1, _max_height_requestable);
    _CPListBase->show_all();
}

void CommandPalette::on_action_fullname_clicked(const Glib::ustring &action_fullname)
{
    static auto clipboard = Gtk::Clipboard::get();
    clipboard->set_text(action_fullname);
    clipboard->store();
}

void CommandPalette::on_row_activated(Gtk::ListBoxRow *activated_row)
{
    // this is set to import/export or full action name
    const auto full_action_name = get_full_action_name(activated_row)->get_label();
    if (full_action_name == "import" or full_action_name == "open") {
        const auto [name, description] = get_name_desc(activated_row);
        operate_recent_file(description->get_text(), full_action_name == "import");
    } else {
        ask_action_parameter(get_action_ptr_name(full_action_name));
        // this is an action
    }
}

void CommandPalette::on_history_selection_changed(Gtk::ListBoxRow *lb)
{
    // set the search box text to current selection
    if (const auto name_label = get_name_desc(lb).first; name_label) {
        _CPFilter->set_text(name_label->get_text());
    }
}

bool CommandPalette::operate_recent_file(Glib::ustring const &uri, bool const import)
{
    static auto prefs = Inkscape::Preferences::get();

    bool write_to_history = true;

    // if the last element in CPHistory is already this, don't update history file
    if (_CPHistory->get_children().empty()) {
        const auto last_of_history = _CPHistory->get_row_at_index(_CPHistory->get_children().size() - 1);

        // picks from action button contains either import or export or full_action_name
        const auto last_operation_import_export_indicator = get_full_action_name(last_of_history)->get_label();
        // uri is stored in description field
        const auto last_description = get_name_desc(last_of_history).second->get_text();
        if (last_description == uri) {
            // uri is the same as last operation
            // we only want to store to history, if last operation was different from the one we are going to execute
            // so we only want to store a import if last operation was an export and vice-versa, for the same file
            write_to_history = not import xor last_operation_import_export_indicator == "import";
        }
    }
    if (import) {
        prefs->setBool("/options/onimport", true);
        file_import(SP_ACTIVE_DOCUMENT, uri, nullptr);
        prefs->setBool("/options/onimport", true);

        if (write_to_history) {
            _history_file_output_stream->write("IMPORT_FILE:" + uri + "\n");
        }

        close();
        return true;
    }

    // open
    auto app = &(ConcreteInkscapeApplication<Gtk::Application>::get_instance());
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(uri);
    app->create_window(file);
    if (write_to_history) {
        _history_file_output_stream->write("OPEN_FILE:" + uri + "\n");
    }

    close();
    return true;
}

/**
 * Maybe replaced by: Temporary arrangement may be replaced by snippets
 * This can help us provide parameters for multiple argument function
 * whose actions take a sring as param
 */
bool CommandPalette::ask_action_parameter(const ActionPtrName &action_ptr_name)
{
    // Avoid writing same last action again
    if (_CPHistory->get_children().empty()) {
        const auto last_of_history = _CPHistory->get_row_at_index(_CPHistory->get_children().size() - 1);
        const auto last_full_action_name = get_full_action_name(last_of_history)->get_label();
        if (last_full_action_name == action_ptr_name.second) {
            // last action is the same
            _history_file_output_stream->write("ACTION:" + action_ptr_name.second + "\n");
        }
    }

    // Checking if action has handleable parameter type
    TypeOfVariant action_param_type = get_action_variant_type(action_ptr_name.first);
    if (action_param_type == TypeOfVariant::UNKNOWN) {
        std::cerr << "CommandPalette::ask_action_parameter: unhandled action value type (Unknown Type) "
                  << action_ptr_name.second << std::endl;
        return false;
    }

    if (action_param_type != TypeOfVariant::NONE) {
        set_mode(CPMode::INPUT);

        _cpfilter_key_press_connection = _CPFilter->signal_key_press_event().connect(
            sigc::bind<ActionPtrName>(sigc::mem_fun(*this, &CommandPalette::on_key_press_cpfilter_input_mode),
                                      action_ptr_name),
            false);

        // get type string NOTE: Temporary should be replaced by adding some data to InkActionExtraDataj
        Glib::ustring type_string;
        switch (action_param_type) {
            case TypeOfVariant::BOOL:
                type_string = "bool";
                break;
            case TypeOfVariant::INT:
                type_string = "integer";
                break;
            case TypeOfVariant::DOUBLE:
                type_string = "double";
                break;
            case TypeOfVariant::STRING:
                type_string = "string";
                break;
            default:
                break;
        }
        _CPFilter->set_placeholder_text("Enter a " + type_string + "...");
        _CPFilter->set_tooltip_text("Enter a " + type_string + "...");
        return true;
    }

    execute_action(action_ptr_name, "");
    close();

    return true;
}

bool CommandPalette::match_search(const Glib::ustring &subject, const Glib::ustring &search)
{
    // TODO: Better matching algorithm take inspiration from VS code
    if (subject.lowercase().find(search.lowercase()) != -1) {
        return true;
    }
    return false;
}

void CommandPalette::set_mode(CPMode mode)
{
    switch (mode) {
        case CPMode::SEARCH:
            if (_mode == CPMode::SEARCH) {
                return;
            }

            _CPFilter->set_text("");
            _CPFilter->set_icon_from_icon_name("edit-find-symbolic");
            _CPFilter->set_placeholder_text("Search operation...");
            _CPFilter->set_tooltip_text("Search operation...");
            show_suggestions();

            // Show Suggestions instead of history
            _CPHistoryScroll->set_no_show_all();
            _CPHistoryScroll->hide();

            _CPSuggestionsScroll->set_no_show_all(false);
            _CPSuggestionsScroll->show_all();

            _CPSuggestions->unset_filter_func();
            _CPSuggestions->set_filter_func(sigc::mem_fun(*this, &CommandPalette::on_filter_general));

            _cpfilter_search_connection.disconnect(); // to be sure
            _cpfilter_key_press_connection.disconnect();

            _cpfilter_search_connection =
                _CPFilter->signal_search_changed().connect(sigc::mem_fun(*this, &CommandPalette::on_search));
            _cpfilter_key_press_connection = _CPFilter->signal_key_press_event().connect(
                sigc::mem_fun(*this, &CommandPalette::on_key_press_cpfilter_search_mode), false);

            _search_text = "";
            _CPSuggestions->invalidate_filter();
            break;

        case CPMode::INPUT:
            if (_mode == CPMode::INPUT) {
                return;
            }
            _cpfilter_search_connection.disconnect();
            _cpfilter_key_press_connection.disconnect();

            hide_suggestions();
            _CPFilter->set_text("");
            _CPFilter->grab_focus();

            _CPFilter->set_icon_from_icon_name("input-keyboard");
            _CPFilter->set_placeholder_text("Enter action argument");
            _CPFilter->set_tooltip_text("Enter action argument");

            break;

        case CPMode::SHELL:
            if (_mode == CPMode::SHELL) {
                return;
            }

            hide_suggestions();
            _CPFilter->set_icon_from_icon_name("gtk-search");
            _cpfilter_search_connection.disconnect();
            _cpfilter_key_press_connection.disconnect();

            break;

        case CPMode::HISTORY:
            if (_mode == CPMode::HISTORY) {
                return;
            }

            if (_CPHistory->get_children().empty()) {
                return;
            }

            // Show history instead of suggestions
            _CPSuggestionsScroll->set_no_show_all();
            _CPHistoryScroll->set_no_show_all(false);

            _CPSuggestionsScroll->hide();
            _CPHistoryScroll->show_all();

            _CPFilter->set_icon_from_icon_name("format-justify-fill");
            _CPFilter->set_icon_tooltip_text(N_("History mode"));
            _cpfilter_search_connection.disconnect();
            _cpfilter_key_press_connection.disconnect();

            _cpfilter_key_press_connection = _CPFilter->signal_key_press_event().connect(
                sigc::mem_fun(*this, &CommandPalette::on_key_press_cpfilter_history_mode), false);

            _CPHistory->signal_row_selected().connect(
                sigc::mem_fun(*this, &CommandPalette::on_history_selection_changed));
            _CPHistory->signal_row_activated().connect(sigc::mem_fun(*this, &CommandPalette::on_row_activated));

            {
                // select last row
                const auto last_row = _CPHistory->get_row_at_index(_CPHistory->get_children().size() - 1);
                _CPHistory->select_row(*last_row);
                last_row->grab_focus();
            }

            {
                // FIXME: scroll to bottom
                const auto adjustment = _CPHistoryScroll->get_vadjustment();
                adjustment->set_value(adjustment->get_upper());
            }

            break;
    }
    _mode = mode;
}

/**
 * Calls actions with parameters
 */
CommandPalette::ActionPtrName CommandPalette::get_action_ptr_name(const Glib::ustring &full_action_name)
{
    static auto app = dynamic_cast<Gtk::Application *>(Gio::Application::get_default().get());
    static auto win = dynamic_cast<InkscapeWindow *>(app->get_active_window());
    static auto doc = win->get_document()->getActionGroup();
    auto action_domain_string = full_action_name.substr(0, full_action_name.find('.')); // app, win, doc
    auto action_name = full_action_name.substr(full_action_name.find('.') + 1);

    ActionPtr action_ptr;
    if (action_domain_string == "app") {
        action_ptr = app->lookup_action(action_name);
    } else if (action_domain_string == "win") {
        action_ptr = win->lookup_action(action_name);
    } else if (action_domain_string == "doc") {
        action_ptr = doc->lookup_action(action_name);
    }

    return {action_ptr, full_action_name};
}

bool CommandPalette::execute_action(const ActionPtrName &action_ptr_name, const Glib::ustring &value)
{
    auto [action_ptr, action_name] = action_ptr_name;

    switch (get_action_variant_type(action_ptr)) {
        case TypeOfVariant::BOOL:
            if (value == "1" || value == "true" || value.empty()) {
                action_ptr->activate(Glib::Variant<bool>::create(true));
            } else if (value == "0" || value == "false") {
                action_ptr->activate(Glib::Variant<bool>::create(false));
            } else {
                std::cerr << "CommandPalette::execute_action: Invalid boolean value: " << action_name << ":" << value
                          << std::endl;
            }
            break;
        case TypeOfVariant::INT:
            action_ptr->activate(Glib::Variant<int>::create(std::stoi(value)));
            break;
        case TypeOfVariant::DOUBLE:
            action_ptr->activate(Glib::Variant<double>::create(std::stod(value)));
            break;
        case TypeOfVariant::STRING:
            action_ptr->activate(Glib::Variant<Glib::ustring>::create(value));
            break;
        case TypeOfVariant::UNKNOWN:
            std::cerr << "CommandPalette::execute_action: unhandled action value type (Unknown Type) " << action_name
                      << std::endl;
            break;
        case TypeOfVariant::NONE:
        default:
            action_ptr->activate();
            break;
    }
    return false;
}

TypeOfVariant CommandPalette::get_action_variant_type(const ActionPtr &action_ptr)
{
    const GVariantType *gtype = g_action_get_parameter_type(action_ptr->gobj());
    if (gtype) {
        Glib::VariantType type = action_ptr->get_parameter_type();
        if (type.get_string() == "b") {
            return TypeOfVariant::BOOL;
        } else if (type.get_string() == "i") {
            return TypeOfVariant::INT;
        } else if (type.get_string() == "d") {
            return TypeOfVariant::DOUBLE;
        } else if (type.get_string() == "s") {
            return TypeOfVariant::STRING;
        } else {
            return TypeOfVariant::UNKNOWN;
        }
    }
    // With value.
    return TypeOfVariant::NONE;
}

std::pair<Gtk::Label *, Gtk::Label *> CommandPalette::get_name_desc(Gtk::ListBoxRow *child)
{
    auto event_box = dynamic_cast<Gtk::EventBox *>(child->get_child());
    if (event_box) {
        // NOTE: These variables have same name as in the glade file command-operation-lite.glade
        // FIXME: When structure of Gladefile of CPOperation changes, refactor this
        auto CPSynapseBox = dynamic_cast<Gtk::Box *>(event_box->get_child());
        if (CPSynapseBox) {
            auto synapse_children = CPSynapseBox->get_children();
            auto CPName = dynamic_cast<Gtk::Label *>(synapse_children[0]);
            auto CPDescription = dynamic_cast<Gtk::Label *>(synapse_children[1]);

            return std::pair(CPName, CPDescription);
        }
    }

    return std::pair(nullptr, nullptr);
}

Gtk::Button *CommandPalette::get_full_action_name(Gtk::ListBoxRow *child)
{
    auto event_box = dynamic_cast<Gtk::EventBox *>(child->get_child());
    if (event_box) {
        auto CPSynapseBox = dynamic_cast<Gtk::Box *>(event_box->get_child());
        if (CPSynapseBox) {
            auto synapse_children = CPSynapseBox->get_children();
            auto CPActionFullName = dynamic_cast<Gtk::Button *>(synapse_children[2]);

            return CPActionFullName;
        }
    }

    return nullptr;
}

// Get a list of all actions (application, window, and document), properly prefixed.
// We need to do this ourselves as Gtk::Application does not have a function for this.
// TODO: Remove when Shortcuts branch merge
// NOTE: It has deviated a bit from the shortcuts branch code to fit the needs discuss with @Tavmjong
//       and @rathod-sahaab regarding this.
std::vector<CommandPalette::ActionPtrName> CommandPalette::list_all_actions()
{
    auto app = dynamic_cast<Gtk::Application *>(Gio::Application::get_default().get());
    std::vector<ActionPtrName> all_actions_info;

    std::vector<Glib::ustring> actions = app->list_actions();
    std::sort(actions.begin(), actions.end());

    for (auto action : actions) {
        all_actions_info.emplace_back(app->lookup_action(action), "app." + action);
    }

    auto gwindow = app->get_active_window();
    auto window = dynamic_cast<InkscapeWindow *>(gwindow);
    if (window) {
        std::vector<Glib::ustring> actions = window->list_actions();
        std::sort(actions.begin(), actions.end());
        for (auto action : actions) {
            all_actions_info.emplace_back(window->lookup_action(action), "win." + action);
        }

        auto document = window->get_document();
        if (document) {
            auto map = document->getActionGroup();
            if (map) {
                std::vector<Glib::ustring> actions = map->list_actions();
                for (auto action : actions) {
                    all_actions_info.emplace_back(map->lookup_action(action), "doc." + action);
                }
            } else {
                std::cerr << "CommandPalette::list_all_actions: No document map!" << std::endl;
            }
        }
    }

    return all_actions_info;
}

Gtk::Box *CommandPalette::get_base_widget()
{
    return _CPBase;
}

// CPHistoryXML ---------------------------------------------------------------
CPHistoryXML::CPHistoryXML()
    : _file_path(IO::Resource::profile_path("cphistory.xml"))
{
    _xml_doc = sp_repr_read_file(_file_path.c_str(), nullptr);
    if (not _xml_doc) {
        _xml_doc = sp_repr_document_new("cphistory");

        /* STRUCTURE EXAMPLE ------------------ Illustration 1
        <cphistory>
            <operations>
                <action> full.action_name </action>
                <import> uri </import>
                <export> uri </export>
            </operations>
            <params>
                <action name="app.transfor-rotate">
                    <param> 30 </param>
                    <param> 23.5 </param>
                </action>
            </params>
        </cphistory>
        */

        // Just a pointer, we don't own it, don't free/release/delete
        auto root = _xml_doc->root();

        // add operation history in this element
        auto operations = _xml_doc->createElement("operations");
        root->appendChild(operations);

        // add param history in this element
        auto params = _xml_doc->createElement("params");
        root->appendChild(params);

        // This was created by allocated
        Inkscape::GC::release(operations);
        Inkscape::GC::release(params);

        // only save if created new
        save();
    }

    // Only two children :) check and ensure Illustration 1
    _operations = _xml_doc->firstChild();
    _params = _xml_doc->lastChild();
}

void CPHistoryXML::add_action(const std::string &full_action_name)
{
    add_operation(HistoryType::ACTION, full_action_name);
}

void CPHistoryXML::add_import(const std::string &uri)
{
    add_operation(HistoryType::IMPORT_FILE, uri);
}
void CPHistoryXML::add_open(const std::string &uri)
{
    add_operation(HistoryType::OPEN_FILE, uri);
}

void CPHistoryXML::add_action_parameter(const std::string &full_action_name, const std::string &param)
{
    const auto parameter_node = _xml_doc->createElement("param");
    parameter_node->setContent(param.c_str());

    for (auto action_iter = _params->firstChild(); action_iter; action_iter = action_iter->next()) {
        // If this action's node already exists
        if (full_action_name == action_iter->attribute("name")) {
            // If the last parameter was the same don't do anything
            if (action_iter->lastChild()->content() == param) {
                Inkscape::GC::release(parameter_node);
                return;
            }

            // If last current than parameter is different, add current
            action_iter->appendChild(parameter_node);
            Inkscape::GC::release(parameter_node);
            return;
        }
    }

    // only encountered when the actions element doesn't already exists,so we create that action's element
    const auto action_node = _xml_doc->createElement("action");
    action_node->setAttribute("name", full_action_name.c_str());
    action_node->appendChild(parameter_node);

    Inkscape::GC::release(action_node);
    Inkscape::GC::release(parameter_node);
}
std::vector<History> CPHistoryXML::get_operation_history() const
{
    std::vector<History> history;
    for (auto operation_iter = _operations->firstChild(); operation_iter; operation_iter->next()) {
        const std::string operation_type_name = operation_iter->name();

        HistoryType ht;
        if (operation_type_name == "action") {
            ht = HistoryType::ACTION;
        } else if (operation_type_name == "import") {
            ht = HistoryType::IMPORT_FILE;
        } else if (operation_type_name == "open") {
            ht = HistoryType::OPEN_FILE;
        } else {
            // unknown history_type
            continue;
        }
        history.emplace_back(ht, operation_iter->content());
    }
    return history;
}

std::vector<std::string> CPHistoryXML::get_action_parameter_history(const std::string &full_action_name) const
{
    std::vector<std::string> params;
    for (auto action_iter = _params->firstChild(); action_iter; action_iter = action_iter->prev()) {
        // If this action's node already exists
        if (full_action_name == action_iter->attribute("name")) {
            // lastChild and prev for LIFO order
            for (auto param_iter = _params->lastChild(); param_iter; param_iter = param_iter->prev()) {
                params.emplace_back(param_iter->content());
            }
            return params;
        }
    }
    // action not used previously so no params;
    return {};
}

void CPHistoryXML::save() const
{
    sp_repr_save_file(_xml_doc, _file_path.c_str());
}

void CPHistoryXML::add_operation(const HistoryType history_type, const std::string &data)
{
    std::string operation_type_name;
    switch (history_type) {
        // see Illustration 1
        case HistoryType::ACTION:
            operation_type_name = "action";
        case HistoryType::IMPORT_FILE:
            operation_type_name = "import";
        case HistoryType::OPEN_FILE:
            operation_type_name = "open";
        default:
            return;
    }
    auto operation_to_add = _xml_doc->createElement(operation_type_name.c_str());
    operation_to_add->setContent(data.c_str());

    _operations->appendChild(operation_to_add);
    Inkscape::GC::release(operation_to_add);

    save();
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape
