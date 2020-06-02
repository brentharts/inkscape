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
#include <gtkmm/box.h>
#include <gtkmm/eventbox.h>

#include "io/resource.h"
#include "verbs.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

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
    {
        _builder->get_widget("CPBase", _CPBase);
        _builder->get_widget("CPFilter", _CPFilter);
        _builder->get_widget("CPHeader", _CPHeader);
        _builder->get_widget("CPSuggestionsBox", _CPSuggestionsBox);
    }

    _CPBase->add_events(Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
                        Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK | Gdk::KEY_PRESS_MASK);

    _CPFilter->signal_search_changed().connect(sigc::mem_fun(*this, &CommandPalette::on_search));

    // Setup operations [actins, verbs, extenstion]
    {
        auto gladefile = get_filename_string(Inkscape::IO::Resource::UIS, "command-palette-operation.glade");
        auto all_verbs = Verb::getList();

        for (const auto &verb : all_verbs) {
            Glib::RefPtr<Gtk::Builder> operation_builder;
            try {
                operation_builder = Gtk::Builder::create_from_file(gladefile);
            } catch (const Glib::Error &ex) {
                g_warning("Glade file loading failed for filter effect dialog");
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

            // Name
            {
                const Glib::ustring untranslated_name(verb->get_name());
                const Glib::ustring name(_(verb->get_name()));
                if (untranslated_name == name) {
                    CPName->set_text(name);
                } else {
                    CPName->set_markup((name + "\n<span size='x-small'>" + untranslated_name + "</span>").c_str());
                }
            }

            // Description
            CPDescription->set_text(_(verb->get_tip()));

	    // Icon
	    CPIcon->set_from_icon_name(verb->get_image(), Gtk::IconSize(Gtk::ICON_SIZE_LARGE_TOOLBAR));
        }
    }
}
} // namespace Dialog
} // namespace UI
} // namespace Inkscape
