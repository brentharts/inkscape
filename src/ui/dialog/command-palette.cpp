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
#include <ostream>
#include <readline/history.h>
#include <sigc++/adaptors/bind.h>
#include <sigc++/functors/mem_fun.h>
#include <string>

#include "actions/actions-extra-data.h"
#include "file.h"
#include "inkscape-application.h"
#include "inkscape-window.h"
#include "inkscape.h"
#include "io/resource.h"
#include "object/uri.h"
#include "preferences.h"
#include "ui/interface.h"
#include "verbs.h"

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
    _builder->get_widget("CPSearchBar", _CPSearchBar);
    _builder->get_widget("CPFilter", _CPFilter);
    _builder->get_widget("CPScrolled", _CPScrolled);
    _builder->get_widget("CPViewPort", _CPViewPort);
    _builder->get_widget("CPSuggestions", _CPSuggestions);

    _CPBase->add_events(Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
                        Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK | Gdk::KEY_PRESS_MASK);

    // TODO: Customise on user language RTL, LTR or better user preference
    _CPBase->set_halign(Gtk::ALIGN_CENTER);
    _CPBase->set_valign(Gtk::ALIGN_START);

    _CPFilter->signal_key_press_event().connect(sigc::mem_fun(*this, &CommandPalette::on_key_press_cpfilter_escape),
                                                false);
    set_cp_fiter_mode(CPFilterMode::SEARCH);

    _CPSuggestions->set_activate_on_single_click();
    _CPSuggestions->set_selection_mode(Gtk::SELECTION_SINGLE);

    // Preferences load
    auto prefs = Inkscape::Preferences::get();
    const auto app = Gio::Application::get_default();
    const auto iapp = dynamic_cast<InkscapeApplication *>(app.get());
    const auto gapp = dynamic_cast<Gtk::Application *>(app.get());

    // Setup operations [actions, verbs, extenstions]
    {
        auto app = dynamic_cast<InkscapeApplication *>(Gio::Application::get_default().get());
        InkActionExtraData &action_data = app->get_action_extra_data();

        const auto gladefile =
            get_filename_string(Inkscape::IO::Resource::UIS, "command-palette-operation-next-full-action.glade");

        auto all_actions_ptr_name = list_all_actions();

        // setup actions - can’t do const
        for (/*const*/ auto &action_ptr_name : all_actions_ptr_name) {
            Glib::RefPtr<Gtk::Builder> operation_builder;
            try {
                operation_builder = Gtk::Builder::create_from_file(gladefile);
            } catch (const Glib::Error &ex) {
                g_warning("Glade file loading failed for Command Palette operation dialog");
                return;
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
                bool show_full_action_name = prefs->getBool("/options/commandpalette/showfullactionname/value");
                CPActionFullName->set_label(action_ptr_name.second);

                if (not show_full_action_name) {
                    CPActionFullName->set_no_show_all();
                    CPActionFullName->hide();
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
            _CPSuggestions->append(*CPOperation);

            CPOperation->signal_button_press_event().connect(sigc::bind<ActionPtrName>(
                sigc::mem_fun(*this, &CommandPalette::on_clicked_operation_action), action_ptr_name));

            // Requires CPOperation added to _CPSuggestions
            CPOperation->get_parent()->signal_key_press_event().connect(sigc::bind<ActionPtrName>(
                sigc::mem_fun(*this, &CommandPalette::on_key_press_operation_action), action_ptr_name));
            CPActionFullName->signal_clicked().connect(
                sigc::bind<Glib::ustring>(sigc::mem_fun(*this, &CommandPalette::on_action_fullname_clicked),
                                          action_ptr_name.second),
                false);
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

                append_recent_file_operation(recent_file, false); // open
                append_recent_file_operation(recent_file, true);
            }
        }
    }

    // History managment
    {
        auto file_name = Inkscape::IO::Resource::profile_path("cp.history");
        auto file = Gio::File::create_for_path(file_name);
        if (file->query_exists()) {
            char *contents = nullptr;
            gsize length = 0;

            file->load_contents(contents, length);
            // length is set by the function ignoring last '\0' hence contents[length] is '\0'

            if (length != 0 and contents != nullptr) {
                // most recent first
                std::istringstream lines(contents);
                History full_history;
                for (std::string line; std::getline(lines, line, '\n');) {
                    auto type = line.substr(0, line.find(':'));
                    auto data = line.substr(line.find(':') + 1);

                    if (type == "ACTION") {
                        _history.emplace_back(HistoryType::ACTION, data);
                    } else if (type == "OPEN_FILE") {
                        _history.emplace_back(HistoryType::OPEN_FILE, data);
                    } else if (type == "IMPORT_FILE") {
                        _history.emplace_back(HistoryType::IMPORT_FILE, data);
                    } else if (type == "LPE") {
                        _history.emplace_back(HistoryType::LPE, data);
                    }
                }
            }

            g_free(contents);
        }
        _history_file_output_stream = file->append_to();
    }
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

    set_cp_fiter_mode(CPFilterMode::SEARCH);

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

/**
 * Highlights the current chapter
 * Set text to readable operation name
 * Highlights the ListBoxRow of the operation
 */
void CommandPalette::focus_current_chapter()
{
    static InkActionExtraData &action_data =
        dynamic_cast<InkscapeApplication *>(Gio::Application::get_default().get())->get_action_extra_data();
    switch (_current_chapter->first) {
        case HistoryType::ACTION: {
            _CPSuggestions->unset_filter_func();
            _CPSuggestions->set_filter_func(sigc::mem_fun(*this, &CommandPalette::on_filter_full_action_name));

            _search_text = _current_chapter->second;
            _CPSuggestions->invalidate_filter();

            Glib::ustring name = _current_chapter->second;
            _CPFilter->set_text(action_data.get_label_for_action(name));
        } break;
        case HistoryType::OPEN_FILE:
        case HistoryType::IMPORT_FILE: {
            auto uri = _current_chapter->second;
            bool is_import = _current_chapter->first == HistoryType::IMPORT_FILE;

            _CPSuggestions->unset_filter_func();
            _CPSuggestions->set_filter_func(
                sigc::bind<bool>(sigc::mem_fun(*this, &CommandPalette::on_filter_recent_file), is_import));

            _search_text = uri;
            _CPSuggestions->invalidate_filter();

            {
                Glib::ustring method = (is_import ? N_("Import") : N_("Open"));
                Glib::ustring file_name = Glib::filename_display_basename(uri);
                _CPFilter->set_text(method + " > " + file_name);
            }
        } break;
        default:
            break;
    }
}

void CommandPalette::repeat_current_chapter()
{
    static auto app = dynamic_cast<Gtk::Application *>(Gio::Application::get_default().get());
    static auto win = dynamic_cast<InkscapeWindow *>(app->get_active_window());
    static auto doc = win->get_document()->getActionGroup();

    switch (_current_chapter->first) {
        case HistoryType::ACTION: {
            auto action_domain_string =
                _current_chapter->second.substr(0, _current_chapter->second.find('.')); // app, win, doc
            auto action_name = _current_chapter->second.substr(_current_chapter->second.find('.') + 1);

            ActionPtr action_ptr;
            if (action_domain_string == "app") {
                action_ptr = app->lookup_action(action_name);
            } else if (action_domain_string == "win") {
                action_ptr = win->lookup_action(action_name);
            } else if (action_domain_string == "doc") {
                action_ptr = doc->lookup_action(action_name);
            }
            ask_action_parameter({action_ptr, _current_chapter->second});
        } break;

        case HistoryType::OPEN_FILE:
        case HistoryType::IMPORT_FILE: {
            auto uri = _current_chapter->second;
            on_clicked_operation_recent_file(nullptr, uri, _current_chapter->first == HistoryType::IMPORT_FILE);
        } break;

        default:
            close();
            break;
    }
}

void CommandPalette::append_recent_file_operation(Glib::RefPtr<Gtk::RecentInfo> recent_file, bool is_import)
{
    auto gladefile =
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

    auto file_name = recent_file->get_display_name();
    auto uri = recent_file->get_uri_display();

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
    CPDescription->set_text(uri);
    CPDescription->set_tooltip_text(uri);

    {
        auto mod_time_t = recent_file->get_modified();
        auto mod_time = Glib::DateTime::create_now_local(mod_time_t);

        // Using this to reduce instead of ActionFullName widget because fullname is searched
        CPShortcut->set_text(mod_time.format("%d %b %R"));
    }

    _CPSuggestions->append(*CPOperation);

    CPOperation->signal_button_press_event().connect(
        sigc::bind<Glib::ustring, bool const>(sigc::mem_fun(*this, &CommandPalette::on_clicked_operation_recent_file),
                                              recent_file->get_uri_display(), is_import));

    // Requires CPOperation added to _CPSuggestions
    CPOperation->get_parent()->signal_key_press_event().connect(
        sigc::bind<Glib::ustring, bool const>(sigc::mem_fun(*this, &CommandPalette::on_key_press_operation_recent_file),
                                              recent_file->get_uri_display(), is_import));
};

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
    if (auto CPActionFullName = get_full_action_name_label(child);
        CPActionFullName and _search_text == CPActionFullName->get_label()) {
        return true;
    }
    return false;
}

bool CommandPalette::on_filter_recent_file(Gtk::ListBoxRow *child, bool const is_import)
{
    auto CPActionFullName = get_full_action_name_label(child);
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
        if (not _history.empty()) {
            set_cp_fiter_mode(CPFilterMode::HISTORY);
            return true;
        }
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
        case GDK_KEY_Linefeed:
            execute_action(action_ptr_name, _CPFilter->get_text());
            close();
            return true;
    }
    return false;
}

bool CommandPalette::on_key_press_cpfilter_history_mode(GdkEventKey *evt)
{
    // perform the previous action again
    switch (evt->keyval) {
        case GDK_KEY_Up:
            if (_current_chapter != _history.begin()) {
                _current_chapter--;
                focus_current_chapter();
            }
            return true;
        case GDK_KEY_Down:
            if (_current_chapter != _history.end()) {
                _current_chapter++;
                focus_current_chapter();
            } else {
                set_cp_fiter_mode(CPFilterMode::SEARCH);
            }
            return true;
        case GDK_KEY_Return:
        case GDK_KEY_Linefeed:
            repeat_current_chapter();
            return true;
        default:
            break;
    }

    return false;
}

void CommandPalette::hide_suggestions()
{
    _CPBase->set_size_request(-1, 10);
    _CPScrolled->hide();
}
void CommandPalette::show_suggestions()
{
    _CPBase->set_size_request(-1, _max_height_requestable);
    _CPScrolled->show_all();
}

void CommandPalette::on_action_fullname_clicked(const Glib::ustring &action_fullname)
{
    static auto clipboard = Gtk::Clipboard::get();
    clipboard->set_text(action_fullname);
    clipboard->store();
}

bool CommandPalette::on_clicked_operation_action(GdkEventButton * /*evt*/, const ActionPtrName &action_ptr_name)
{
    ask_action_parameter(action_ptr_name);
    return true;
}

bool CommandPalette::on_key_press_operation_action(GdkEventKey *evt, const ActionPtrName &action_ptr_name)
{
    if (evt->keyval == GDK_KEY_Return || evt->keyval == GDK_KEY_Return) {
        return on_clicked_operation_action(nullptr, action_ptr_name);
    }
    return false;
}

bool CommandPalette::on_clicked_operation_recent_file(GdkEventButton *evt, Glib::ustring const &uri, bool const import)
{
    static auto prefs = Inkscape::Preferences::get();

    if (import) {
        prefs->setBool("/options/onimport", true);
        file_import(SP_ACTIVE_DOCUMENT, uri, nullptr);
        prefs->setBool("/options/onimport", true);

        if (_history.empty() or
            not(_history.back().first == HistoryType::IMPORT_FILE and _history.back().second == uri)) {
            _history.emplace_back(HistoryType::IMPORT_FILE, uri);
            _history_file_output_stream->write("IMPORT_FILE:" + uri + "\n");
        }

        close();
        return true;
    }

    // open
    auto app = &(ConcreteInkscapeApplication<Gtk::Application>::get_instance());
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(uri);
    app->create_window(file);
    if (_history.empty() or not(_history.back().first == HistoryType::OPEN_FILE and _history.back().second == uri)) {
        _history.emplace_back(HistoryType::OPEN_FILE, uri);
        _history_file_output_stream->write("OPEN_FILE:" + uri + "\n");
    }

    close();
    return true;
}

bool CommandPalette::on_key_press_operation_recent_file(GdkEventKey *evt, Glib::ustring const &uri, bool const import)
{
    if (auto key = evt->keyval; key == GDK_KEY_Linefeed or key == GDK_KEY_Return) {
        return on_clicked_operation_recent_file(nullptr, uri, import);
    }
    return false;
}
/**
 * Maybe replaced by: Temporary arrangement may be replaced by snippets
 * This can help us provide parameters for multiple argument function
 * whose actions take a sring as param
 */
bool CommandPalette::ask_action_parameter(const ActionPtrName &action_ptr_name)
{
    // Avoid writing same last action again
    if (_history.empty() or _history.back().second != action_ptr_name.second) {
        _history.emplace_back(HistoryType::ACTION, action_ptr_name.second);
        _history_file_output_stream->write("ACTION:" + action_ptr_name.second + "\n");
    }

    // Checking if action has handleable parameter type
    TypeOfVariant action_param_type = get_action_variant_type(action_ptr_name.first);
    if (action_param_type == TypeOfVariant::UNKNOWN) {
        std::cerr << "CommandPalette::ask_action_parameter: unhandled action value type (Unknown Type) "
                  << action_ptr_name.second << std::endl;
        return false;
    }

    if (action_param_type != TypeOfVariant::NONE) {
        set_cp_fiter_mode(CPFilterMode::INPUT);

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

void CommandPalette::set_cp_fiter_mode(CPFilterMode mode)
{
    switch (mode) {
        case CPFilterMode::SEARCH:
            if (_mode == CPFilterMode::SEARCH) {
                return;
            }

            _CPFilter->set_text("");
            _CPFilter->set_icon_from_icon_name("edit-find-symbolic");
            _CPFilter->set_placeholder_text("Search operation...");
            _CPFilter->set_tooltip_text("Search operation...");
            show_suggestions();

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

        case CPFilterMode::INPUT:
            if (_mode == CPFilterMode::INPUT) {
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

        case CPFilterMode::SHELL:
            if (_mode == CPFilterMode::SHELL) {
                return;
            }

            hide_suggestions();
            _CPFilter->set_icon_from_icon_name("gtk-search");
            _cpfilter_search_connection.disconnect();
            _cpfilter_key_press_connection.disconnect();

            break;

        case CPFilterMode::HISTORY:
            if (_mode == CPFilterMode::HISTORY) {
                return;
            }

            _CPFilter->set_icon_from_icon_name("format-justify-fill");
            _CPFilter->set_icon_tooltip_text(N_("History mode"));
            _cpfilter_search_connection.disconnect();
            _cpfilter_key_press_connection.disconnect();

            _current_chapter = _history.end() - 1;
            focus_current_chapter();

            _cpfilter_key_press_connection = _CPFilter->signal_key_press_event().connect(
                sigc::mem_fun(*this, &CommandPalette::on_key_press_cpfilter_history_mode), false);

            break;
    }
    _mode = mode;
}

/**
 * Calls actions with parameters
 */
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

Gtk::Button *CommandPalette::get_full_action_name_label(Gtk::ListBoxRow *child)
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

} // namespace Dialog
} // namespace UI
} // namespace Inkscape
