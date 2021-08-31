// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Toolbar for oncanvas alignment and distribution
 *
 * Authors:
 *   Kavya Jaiswal
 *
 * Copyright (C) 2021 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "distribute-toolbar.h"

#include <2geom/rect.h>
#include <glibmm/i18n.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/separatortoolitem.h>

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "io/resource.h"
#include "message-stack.h"
#include "object/sp-item-transform.h"
#include "object/sp-namedview.h"
#include "selection-chemistry.h"
#include "selection.h"
#include "ui/icon-names.h"
#include "ui/widget/canvas.h" // Focus widget
#include "ui/widget/combo-tool-item.h"
#include "ui/widget/spin-button-tool-item.h"
#include "ui/widget/spinbutton.h"
#include "ui/widget/unit-tracker.h"
#include "verbs.h"
#include "widgets/widget-sizes.h"

namespace Inkscape {
namespace UI {
namespace Toolbar {

DistributeToolbar::DistributeToolbar(SPDesktop *desktop)
    : Toolbar(desktop)
{
    mode_buttons_init();
    add_separator();
    oncanvas_buttons_init();
    add_separator();
    show_all();
}

void DistributeToolbar::mode_buttons_init()
{
    const static std::vector<ButtonDescriptor> mode_buttons_descriptors = {
        {
            .label = _("Just Select"),
            .tooltip_text = _("Just select whatever the mouse moves over"),
            .icon_name = "tool-pointer",
            .handler = &DistributeToolbar::set_mode_just_select,
        },
        {
            .label = _("Mode Align"),
            .tooltip_text = _("On-canvas align mode"),
            .icon_name = "align-horizontal-left",
            .handler = &DistributeToolbar::set_mode_align,
        },
        {
            .label = _("Mode Distribute"),
            .tooltip_text = _("On-canvas distribute mode"),
            .icon_name = "distribute-vertical-gaps",
            .handler = &DistributeToolbar::set_mode_distribute,
        },
    };

    mode_buttons_init_create_buttons(mode_buttons_descriptors);
    mode_buttons_init_set_active_button();
    mode_buttons_init_add_buttons();
}

void DistributeToolbar::mode_buttons_init_create_buttons(const std::vector<ButtonDescriptor> &descriptors)
{
    Gtk::RadioToolButton::Group mode_group;

    for (auto &mode : descriptors) {
        auto button = Gtk::manage(new Gtk::RadioToolButton((mode.label)));
        button->set_tooltip_text((mode.tooltip_text));
        button->set_icon_name(INKSCAPE_ICON(mode.icon_name));
        button->set_group(mode_group);
        _mode_buttons.push_back(button);
        _mode_handlers.push_back(mode.handler);
    }
}

void DistributeToolbar::mode_buttons_init_set_active_button()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gint type = prefs->getInt("/tools/distribute/mode", 0);
    _mode_buttons[type]->set_active();
}

void DistributeToolbar::mode_buttons_init_add_buttons()
{
    int button_index = 0;
    for (auto button : _mode_buttons) {
        button->set_sensitive();
        button->signal_clicked().connect(
            sigc::bind(sigc::mem_fun(*this, &DistributeToolbar::mode_changed), button_index++));
        add(*button);
    }
}

void DistributeToolbar::mode_changed(int mode)
{
    auto handler = _mode_handlers[mode];
    (this->*handler)();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/distribute/mode", mode);
}

void DistributeToolbar::set_mode_just_select() {}

void DistributeToolbar::set_mode_align() {}

void DistributeToolbar::set_mode_distribute() {}

void DistributeToolbar::oncanvas_buttons_init()
{
    oncanvas_buttons_init_verbs();
    oncanvas_buttons_init_actions();
}

void DistributeToolbar::oncanvas_buttons_init_actions()
{
    // TODO find a better way than this. Using verbs is easier.
    const static std::vector<ButtonDescriptor> oncanvas_buttons_descriptors = {
        {
            .label = _("Horizontal Gaps"),
            .tooltip_text = _("Make horizontal gaps between objects equal"),
            .icon_name = "distribute-horizontal-gaps",
            .handler = &DistributeToolbar::perform_horizontal_distribution,
        },
        {
            .label = _("Vertical Gaps"),
            .tooltip_text = _("Make vertical gaps between objects equal"),
            .icon_name = "distribute-vertical-gaps",
            .handler = &DistributeToolbar::perform_vertical_distribution,
        },
    };

    oncanvas_buttons_init_actions_add_buttons(oncanvas_buttons_descriptors);
}

void DistributeToolbar::oncanvas_buttons_init_actions_add_buttons(const std::vector<ButtonDescriptor> &descriptors)
{
    for (auto &oncanvas : descriptors) {
        auto button = Gtk::manage(new Gtk::ToolButton(oncanvas.label));
        button->set_tooltip_text((oncanvas.tooltip_text));
        button->set_icon_name(INKSCAPE_ICON(oncanvas.icon_name));
        button->signal_clicked().connect(sigc::mem_fun(*this, oncanvas.handler));
        add(*button);
    }
}

void DistributeToolbar::perform_horizontal_distribution()
{
    std::cout << "Distribute Horizontal\n";
}

void DistributeToolbar::perform_vertical_distribution()
{
    std::cout << "Distribute Vertical\n";
}

void DistributeToolbar::oncanvas_buttons_init_verbs()
{
    add_toolbutton_for_verb(SP_VERB_OBJECT_FLIP_HORIZONTAL);
    add_toolbutton_for_verb(SP_VERB_OBJECT_FLIP_VERTICAL);
    add_separator();
    add_toolbutton_for_verb(SP_VERB_ALIGN_HORIZONTAL_LEFT);
    add_toolbutton_for_verb(SP_VERB_ALIGN_HORIZONTAL_CENTER);
    add_toolbutton_for_verb(SP_VERB_ALIGN_HORIZONTAL_RIGHT);
    add_separator();
    add_toolbutton_for_verb(SP_VERB_ALIGN_VERTICAL_TOP);
    add_toolbutton_for_verb(SP_VERB_ALIGN_VERTICAL_CENTER);
    add_toolbutton_for_verb(SP_VERB_ALIGN_VERTICAL_BOTTOM);
    add_separator();
}

void DistributeToolbar::add_separator()
{
    add(*Gtk::manage(new Gtk::SeparatorToolItem()));
}

GtkWidget *DistributeToolbar::create(SPDesktop *desktop)
{
    auto toolbar = new DistributeToolbar(desktop);
    return GTK_WIDGET(toolbar->gobj());
}

} // namespace Toolbar
} // namespace UI
} // namespace Inkscape
