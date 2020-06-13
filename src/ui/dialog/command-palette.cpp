// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Dialog for adding a live path effect.
 *
 * Author:
 *
 * Copyright (C) 2012 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "command-palette.h"

#include <cstddef>
#include <giomm/action.h>
#include <giomm/application.h>
#include <glibmm/i18n.h>
#include <glibmm/markup.h>
#include <glibmm/ustring.h>
#include <gtkmm/application.h>
#include <gtkmm/box.h>
#include <gtkmm/enums.h>
#include <gtkmm/eventbox.h>
#include <iostream>
#include <ostream>
#include <sigc++/adaptors/bind.h>
#include <sigc++/functors/mem_fun.h>

#include "actions/actions-extra-data.h"
#include "inkscape-application.h"
#include "inkscape-window.h"
#include "io/resource.h"
#include "ui/dialog/align-and-distribute.h"
#include "verbs.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

namespace {
Glib::ustring camel_case_to_space_separated(const Glib::ustring &camel_case_string)
{
    bool prev_char_uppercase = false;
    std::vector<std::size_t> transition_indices;

    // ignore first char
    for (std::size_t i = 1; i < camel_case_string.size(); ++i) {
        if (g_unichar_isupper(camel_case_string[i])) {
            if (not prev_char_uppercase) {
                transition_indices.push_back(i);
            }
            prev_char_uppercase = true;
        } else { // is lower case or number hence split before this
            if (prev_char_uppercase) {
                transition_indices.push_back(i - 1);
            }
            prev_char_uppercase = false;
        }
    }
    transition_indices.push_back(camel_case_string.size());

    Glib::ustring space_seperated_result;
    space_seperated_result.reserve(transition_indices.size() + camel_case_string.size());

    std::size_t prev_transition_point = 0;
    for (const auto &tindex : transition_indices) {
        for (size_t i = prev_transition_point; i < tindex; ++i) {
            space_seperated_result += camel_case_string[i];
        }
        prev_transition_point = tindex;
        if (tindex != camel_case_string.size()) {
            space_seperated_result += " ";
        }
    }

    return space_seperated_result;
}

template <typename T>
void debug_print(T variable)
{
    std::cerr << variable << std::endl;
}
} // namespace

CommandPalette::CommandPalette()
{
    // setup builder
    {
        auto gladefile = get_filename_string(Inkscape::IO::Resource::UIS, "command-palette-main.glade");
        try {
            _builder = Gtk::Builder::create_from_file(gladefile);
        } catch (const Glib::Error &ex) {
            g_warning("Glade file loading failed for command palette dialog");
            return;
        }
    }
    // Setup UI Components
    _builder->get_widget("CPBase", _CPBase);
    _builder->get_widget("CPFilter", _CPFilter);
    _builder->get_widget("CPHeader", _CPHeader);
    _builder->get_widget("CPScrolled", _CPScrolled);
    _builder->get_widget("CPViewPort", _CPViewPort);
    _builder->get_widget("CPSuggestions", _CPSuggestions);

    _CPBase->add_events(Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
                        Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK | Gdk::KEY_PRESS_MASK);

    // TODO: Customise on user language RTL, LTR or better user preference
    _CPBase->set_halign(Gtk::ALIGN_START);
    _CPBase->set_valign(Gtk::ALIGN_START);

    /* _CPFilter->signal_search_changed().connect(sigc::mem_fun(*this, &CommandPalette::on_search)); */

    // Setup operations [actions, verbs, extenstion]
    {
        auto app = dynamic_cast<InkscapeApplication *>(Gio::Application::get_default().get());
        InkActionExtraData &action_data = app->get_action_extra_data();

        auto gladefile = get_filename_string(Inkscape::IO::Resource::UIS, "command-palette-operation-lite.glade");

        auto all_actions_ptr_name = list_all_actions();
        // can’t do const
        for (/*const*/ auto &action_ptr_name : all_actions_ptr_name) {
            Glib::RefPtr<Gtk::Builder> operation_builder;
            try {
                operation_builder = Gtk::Builder::create_from_file(gladefile);
            } catch (const Glib::Error &ex) {
                g_warning("Glade file loading failed for Command Palette operation dialog");
                return;
            }

            Gtk::EventBox *CPOperation;
            Gtk::Box *CPBaseBox;
            /* Gtk::Box *CPIconBox; */

            /* Gtk::Image *CPIcon; */
            /* Gtk::Image *CPTypeIcon; */
            Gtk::Label *CPName;
            Gtk::Label *CPDescription;

            // Reading widgets
            operation_builder->get_widget("CPOperation", CPOperation);
            operation_builder->get_widget("CPBaseBox", CPBaseBox);
            /* operation_builder->get_widget("CPIconBox", CPIconBox); */

            /* operation_builder->get_widget("CPIcon", CPIcon); */
            /* operation_builder->get_widget("CPTypeIcon", CPTypeIcon); */
            operation_builder->get_widget("CPName", CPName);
            operation_builder->get_widget("CPDescription", CPDescription);

            {
                auto name = camel_case_to_space_separated(action_data.get_label_for_action(action_ptr_name.second));
                if (name.empty()) {
                    name = action_ptr_name.second;
                }
                CPName->set_text(name);
            }
            CPDescription->set_text(action_data.get_tooltip_for_action(action_ptr_name.second));

            /* CPIcon->hide(); */
            /* CPIconBox->hide(); */

            CPOperation->signal_button_press_event().connect(sigc::bind<ActionPtrName>(
                sigc::mem_fun(*this, &CommandPalette::ask_action_parameter), action_ptr_name));

            // Add to suggestions
            _CPSuggestions->append(*CPOperation);
        }
    }
    _CPBase->show_all();
}

/**
 * Maybe replaced by: Temporary arrangement may be replaced by snippets
 * This can help us provide parameters for multiple argument function
 * whose actions take a sring as param
 */
bool CommandPalette::ask_action_parameter(GdkEventButton * /*evt*/, const ActionPtrName &action_ptr_name)
{
    Gtk::Dialog dialog;
    Gtk::Entry entry;

    entry.set_placeholder_text("Enter action parameter");
    entry.set_size_request(500);

    dialog.get_content_area()->pack_start(entry);
    dialog.add_button("Apply", Gtk::RESPONSE_OK);
    dialog.show_all();

    TypeOfVariant action_param_type = get_action_variant_type(action_ptr_name.first);

    if (action_param_type == TypeOfVariant::UNKNOWN) {
        std::cerr << "CommandPalette::ask_action_parameter: unhandled action value type (Unknown Type) "
                  << action_ptr_name.second << std::endl;
        return false;
    }

    if (action_param_type != TypeOfVariant::NONE) {
        dialog.run();
    }
    Glib::ustring value = entry.get_text();
    debug_print("Value: " + value);
    return execute_action(action_ptr_name, value);
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
