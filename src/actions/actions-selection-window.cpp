// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 *
 * Actions related to selection wich require desktop
 *
 * Authors:
 *   Sushant A A <sushant.co19@gmail.com>
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <giomm.h>
#include <glibmm/i18n.h>

#include "actions-selection-window.h"
#include "inkscape-application.h"
#include "inkscape-window.h"
#include "desktop.h"
#include "ui/dialog/dialog-container.h"
#include "path/path-offset.h"
#include "actions/actions-tools.h"
#include "selection-chemistry.h"

void
select_all(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Select All
    Inkscape::SelectionHelper::selectAll(dt);
}

void
select_all_layers(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Select All in All Layers
    Inkscape::SelectionHelper::selectAllInAll(dt);
}

void
select_same_fill_and_stroke(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Fill and Stroke
    Inkscape::SelectionHelper::selectSameFillStroke(dt);
}

void
select_same_fill(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Fill Color
    Inkscape::SelectionHelper::selectSameFillColor(dt);
}

void
select_same_stroke_color(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Stroke Color
    Inkscape::SelectionHelper::selectSameStrokeColor(dt);
}

void
select_same_stroke_style(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Stroke Style
    Inkscape::SelectionHelper::selectSameStrokeStyle(dt);
}

void
select_same_object_type(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Object Type
    Inkscape::SelectionHelper::selectSameObjectType(dt);
}

void
select_invert(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Invert Selection
    Inkscape::SelectionHelper::invert(dt);
}

void
select_none(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Deselect
    Inkscape::SelectionHelper::selectNone(dt);
}

void
select_path_inset(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Inset selected paths
    dt->selection->removeLPESRecursive(true);
    dt->selection->unlinkRecursive(true);
    sp_selected_path_inset(dt);
}

void
select_path_outset(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Outset selected paths
    dt->selection->removeLPESRecursive(true);
    dt->selection->unlinkRecursive(true);
    sp_selected_path_offset(dt);
}

void
select_path_offset_dynamic(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Dynamic Offset
    dt->selection->removeLPESRecursive(true);
    dt->selection->unlinkRecursive(true);
    sp_selected_path_create_offset_object_zero(dt);
    set_active_tool(dt,"Node");
}

void
select_path_offset_linked(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Linked Offset
    dt->selection->removeLPESRecursive(true);
    dt->selection->unlinkRecursive(true);
    sp_selected_path_create_updating_offset_object_zero(dt);
    set_active_tool(dt, "Node");
}

void
select_path_reverse(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Reverse
    Inkscape::SelectionHelper::reverse(dt);
}

std::vector<std::vector<Glib::ustring>> raw_selection_dekstop_data =
{
    // clang-format off
    {"win.select-all",                          N_("Select All"),                   "Selection",        N_("Select all objects or all nodes")},
    {"win.select-all-layers",                   N_("Select All in All Layers"),     "Selection",        N_("Select all objects in all visible and unlocked layers")},
    {"win.select-same-fill-and-stroke",         N_("Fill and Stroke"),              "Selection",        N_("Select all objects with the same fill and stroke as the selected objects")},
    {"win.select-same-fill",                    N_("Fill Color"),                   "Selection",        N_("Select all objects with the same fill as the selected objects")},
    {"win.select-same-stroke-color",            N_("Stroke Color"),                 "Selection",        N_("Select all objects with the same stroke as the selected objects")},
    {"win.select-same-stroke-style",            N_("Stroke Style"),                 "Selection",        N_("Select all objects with the same stroke style (width, dash, markers) as the selected objects")},
    {"win.select-same-object-type",             N_("Object Type"),                  "Selection",        N_("Select all objects with the same object type (rect, arc, text, path, bitmap etc) as the selected objects")},
    {"win.select-invert",                       N_("Invert Selection"),             "Selection",        N_("Invert selection (unselect what is selected and select everything else)")},
    {"win.select-none",                         N_("Deselect"),                     "Selection",        N_("Deselect any selected objects or nodes")},
    {"win.select-path-inset",                   N_("Inset"),                        "Selection",        N_("Inset selected paths")},
    {"win.select-path-outset",                  N_("Outset"),                       "Selection",        N_("Outset selected paths")},
    {"win.select-path-offset-dynamic",          N_("Dynamic Offset"),               "Selection",        N_("Create a dynamic offset object")},
    {"win.select-path-offset-linked",           N_("Linked Offset"),                "Selection",        N_("Create a dynamic offset object linked to the original path")},
    {"win.select-path-reverse",                 N_("Reverse"),                      "Selection",        N_("Reverse the direction of selected paths (useful for flipping markers)")}
    // clang-format on
};

void
add_actions_select_window(InkscapeWindow* win)
{
    // clang-format off
    win->add_action( "select-all",                      sigc::bind<InkscapeWindow*>(sigc::ptr_fun(&select_all), win));
    win->add_action( "select-all-layers",               sigc::bind<InkscapeWindow*>(sigc::ptr_fun(&select_all_layers), win));
    win->add_action( "select-same-fill-and-stroke",     sigc::bind<InkscapeWindow*>(sigc::ptr_fun(&select_same_fill_and_stroke), win));
    win->add_action( "select-same-fill",                sigc::bind<InkscapeWindow*>(sigc::ptr_fun(&select_same_fill), win));
    win->add_action( "select-same-stroke-color",        sigc::bind<InkscapeWindow*>(sigc::ptr_fun(&select_same_stroke_color), win));
    win->add_action( "select-same-stroke-style",        sigc::bind<InkscapeWindow*>(sigc::ptr_fun(&select_same_stroke_style), win));
    win->add_action( "select-same-object-type",         sigc::bind<InkscapeWindow*>(sigc::ptr_fun(&select_same_object_type), win));
    win->add_action( "select-invert",                   sigc::bind<InkscapeWindow*>(sigc::ptr_fun(&select_invert), win));
    win->add_action( "select-none",                     sigc::bind<InkscapeWindow*>(sigc::ptr_fun(&select_none), win));
    win->add_action( "select-path-inset",               sigc::bind<InkscapeWindow*>(sigc::ptr_fun(&select_path_inset),win));
    win->add_action( "select-path-outset",              sigc::bind<InkscapeWindow*>(sigc::ptr_fun(&select_path_outset),win));
    win->add_action( "select-path-offset-dynamic",      sigc::bind<InkscapeWindow*>(sigc::ptr_fun(&select_path_offset_dynamic),win));
    win->add_action( "select-path-offset-linked",       sigc::bind<InkscapeWindow*>(sigc::ptr_fun(&select_path_offset_linked),win));
    win->add_action( "select-path-reverse",             sigc::bind<InkscapeWindow*>(sigc::ptr_fun(&select_path_reverse),win));
    // clang-format on

    auto app = InkscapeApplication::instance();
    if (!app) {
        std::cerr << "add_actions_edit: no app!" << std::endl;
        return;
    }
    app->get_action_extra_data().add_data(raw_selection_dekstop_data);
}