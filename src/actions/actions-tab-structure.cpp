// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 *
 * Actions Related to Tab Structure
 *
 * Authors:
 *   Sushant A A <sushant.co19@gmail.com>
 *
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <iostream>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include "actions-tab-structure.h"

#include "inkscape-application.h"
#include "inkscape-window.h"

#include "ui/widget/canvas.h"
#include "widgets/desktop-widget.h"

/**
 * Add New Tab.
 */
void canvas_add_tab(InkscapeWindow *win)
{
    auto dt = win->get_desktop();
    dt->get_desktop_widget()->add_new_tab(true);
}

/**
 * Add New Tab with template.
 */
void canvas_add_tab_with_template(InkscapeWindow *win)
{
    auto dt = win->get_desktop();
    dt->get_desktop_widget()->add_new_tab_with_template();
}

/**
 * Open document in Tabs.
 */
void canvas_tab_open(InkscapeWindow *win)
{
    auto dt = win->get_desktop();
    dt->get_desktop_widget()->add_new_tab_open();
}

std::vector<std::vector<Glib::ustring>> raw_data_tab_structure =
{
    // clang-format off
    {"win.canvas-add-tab",                  N_("Tab Add"),                  "Tab Structure",       N_("Tab Add")               },
    {"win.canvas-add-tab-with-template",    N_("Tab Add with Template"),    "Tab Structure",       N_("Tab Add with Template") },
    {"win.canvas-tab-open",                 N_("Tab Open"),                 "Tab Structure",       N_("Tab Open")              }
    // clang-format on
};

void add_actions_tab_structure(InkscapeWindow *win)
{
    // clang-format off
    win->add_action("canvas-add-tab",               [win] { canvas_add_tab(win); });
    win->add_action("canvas-add-tab-with-template", [win] { canvas_add_tab_with_template(win); });
    win->add_action("canvas-tab-open",              [win] { canvas_tab_open(win); });
    // clang-format on

    auto app = InkscapeApplication::instance();
    if (!app) {
        std::cerr << "add_actions_tab_structure: no app!" << std::endl;
        return;
    }

    app->get_action_extra_data().add_data(raw_data_tab_structure);
}

