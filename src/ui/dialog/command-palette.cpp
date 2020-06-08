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

CommandPalette::CommandPalette()
{
    // TODO: Remove when https://gitlab.com/inkscape/inkscape/-/merge_requests/1987 is merged
    Glib::RefPtr<Gio::Application> app = Gio::Application::get_default();

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
        auto iapp = dynamic_cast<InkscapeApplication *>(app.get());
        InkActionExtraData &action_data = iapp->get_action_extra_data();

        auto gladefile = get_filename_string(Inkscape::IO::Resource::UIS, "command-palette-operation.glade");

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

            Gtk::Image *CPIcon;
            Gtk::Image *CPTypeIcon;
            Gtk::Label *CPName;
            Gtk::Label *CPDescription;

            // Reading widgets
            operation_builder->get_widget("CPOperation", CPOperation);
            operation_builder->get_widget("CPBaseBox", CPBaseBox);

            operation_builder->get_widget("CPIcon", CPIcon);
            operation_builder->get_widget("CPTypeIcon", CPTypeIcon);
            operation_builder->get_widget("CPName", CPName);
            operation_builder->get_widget("CPDescription", CPDescription);

            CPName->set_text(action_data.get_label_for_action(action));
            CPDescription->set_text(action_data.get_tooltip_for_action(action));

            // Add to suggestions
            _CPSuggestions->append(*CPOperation);
        }
    }
    _CPBase->show_all();
}

// Get a list of all actions (application, window, and document), properly prefixed.
// We need to do this ourselves as Gtk::Application does not have a function for this.
std::vector<Glib::ustring> CommandPalette::list_all_actions()
{
    auto gapp = dynamic_cast<Gtk::Application *>(app.get());
    std::vector<Glib::ustring> all_actions;

    std::cerr << "CP 1" << std::endl; // Error is caused by statement below
    std::vector<Glib::ustring> actions = gapp->list_actions();
    std::cerr << "CP 2" << std::endl; // this isn't printed
    std::sort(actions.begin(), actions.end());
    for (auto action : actions) {
        all_actions.emplace_back("app." + action);
    }

    auto gwindow = gapp->get_active_window();
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
                std::cerr << "Shortcuts::list_all_actions: No document map!" << std::endl;
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
