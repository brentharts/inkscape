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
#include <giomm/application.h>
#include <glibmm/i18n.h>
#include <glibmm/markup.h>
#include <gtkmm/application.h>
#include <gtkmm/box.h>
#include <gtkmm/enums.h>
#include <gtkmm/eventbox.h>
#include <iostream>
#include <ostream>

#include "actions/actions-extra-data.h"
#include "inkscape-application.h"
#include "inkscape-window.h"
#include "io/resource.h"
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

        auto all_actions = list_all_actions();
        // can’t do const
        for (/*const*/ auto &action : all_actions) {
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
                auto name = camel_case_to_space_separated(action_data.get_label_for_action(action));
                if (name.empty()) {
                    name = action;
                }
                CPName->set_text(name);
            }
            CPDescription->set_text(action_data.get_tooltip_for_action(action));

            /* CPIcon->hide(); */
            /* CPIconBox->hide(); */

            // Add to suggestions
            _CPSuggestions->append(*CPOperation);
        }
    }
    _CPBase->show_all();
}

/**
 * checks if actions has parameters and asks values from user
 */
bool CommandPalette::execute_action(const Glib::ustring &action)
{
    auto app = Gio::Application::get_default();

    auto action_ptr = app->lookup_action(action);
    if (action_ptr) {
        // Doesn't seem to be a way to test this using the C++ binding without Glib-CRITICAL errors.
        const GVariantType *gtype = g_action_get_parameter_type(action_ptr->gobj());
        if (gtype) {
            // With value.
            Glib::VariantType type = action_ptr->get_parameter_type();
            if (type.get_string() == "b") {
                // Get a boolean from user
            } else if (type.get_string() == "i") {
                // get an int from user
            } else if (type.get_string() == "d") {
                // get a double from user
            } else if (type.get_string() == "s") {
                // get a string from user
            } else {
                std::cerr << "InkscapeApplication::parse_actions: unhandled action value: " << action << ": "
                          << type.get_string() << std::endl;
            }
        } else {
            // Stateless (i.e. no value).
            app->activate_action(action, Glib::VariantBase());
            return true;
        }
    }
    return false;
}

// Get a list of all actions (application, window, and document), properly prefixed.
// We need to do this ourselves as Gtk::Application does not have a function for this.
// TODO: Remove when Shortcuts branch merge
std::vector<Glib::ustring> CommandPalette::list_all_actions()
{
    auto app = dynamic_cast<Gtk::Application *>(Gio::Application::get_default().get());
    std::vector<Glib::ustring> all_actions;

    std::vector<Glib::ustring> actions = app->list_actions();
    std::sort(actions.begin(), actions.end());

    for (auto action : actions) {
        all_actions.emplace_back("app." + action);
    }

    auto gwindow = app->get_active_window();
    auto window = dynamic_cast<InkscapeWindow *>(gwindow);
    if (window) {
        std::vector<Glib::ustring> actions = window->list_actions();
        std::sort(actions.begin(), actions.end());
        for (auto action : actions) {
            all_actions.emplace_back("win." + action);
        }

        auto document = window->get_document();
        if (document) {
            auto map = document->getActionGroup();
            if (map) {
                std::vector<Glib::ustring> actions = map->list_actions();
                for (auto action : actions) {
                    all_actions.emplace_back("doc." + action);
                }
            } else {
                std::cerr << "CommandPalette::list_all_actions: No document map!" << std::endl;
            }
        }
    }

    return all_actions;
}

Gtk::Box *CommandPalette::get_base_widget()
{
    return _CPBase;
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape
