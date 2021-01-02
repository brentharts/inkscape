// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for selection tied to the application and without GUI.
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <iostream>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include "actions-selection.h"
#include "actions-helper.h"
#include "inkscape-application.h"

#include "inkscape.h"             // Inkscape::Application
#include "selection.h"            // Selection

#include "object/sp-root.h"       // select_all: document->getRoot();
#include "object/sp-item-group.h" // select_all

void
select_clear(InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }
    selection->clear();
}

void
select_by_id(Glib::ustring ids, InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    auto tokens = Glib::Regex::split_simple("\\s*,\\s*", ids);
    for (auto id : tokens) {
        SPObject* object = document->getObjectById(id);
        if (object) {
            selection->add(object);
        } else {
            std::cerr << "select_by_id: Did not find object with id: " << id << std::endl;
        }
    }
}

void
unselect_by_id(Glib::ustring ids, InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    auto tokens = Glib::Regex::split_simple("\\s*,\\s*", ids);
    for (auto id : tokens) {
        SPObject* object = document->getObjectById(id);
        if (object) {
            selection->remove(object);
        } else {
            std::cerr << "unselect_by_id: Did not find object with id: " << id << std::endl;
        }
    }
}

void
select_by_class(Glib::ustring klass, InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    auto objects = document->getObjectsByClass(klass);
    selection->add(objects.begin(), objects.end());
}

void
select_by_element(Glib::ustring element, InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }
    auto objects = document->getObjectsByElement(element);
    selection->add(objects.begin(), objects.end());
}

void
select_by_selector(Glib::ustring selector, InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    auto objects = document->getObjectsBySelector(selector);
    selection->add(objects.begin(), objects.end());
}


// Helper
void
get_all_items_recursive(std::vector<SPObject *> &objects, SPObject *object, Glib::ustring &condition)
{
    for (auto &o : object->childList(false)) {
        if (dynamic_cast<SPItem *>(o)) {
            SPGroup *group = dynamic_cast<SPGroup *>(o);
            if (condition == "layers") {
                if (group && group->layerMode() == SPGroup::LAYER) {
                    objects.emplace_back(o);
                    continue; // Layers cannot contain layers.
                }
            } else if (condition == "no-layers") {
                if (group && group->layerMode() == SPGroup::LAYER) {
                    // recurse one level
                } else {
                    objects.emplace_back(o);
                    continue;
                }
            } else if (condition == "groups") {
                if (group) {
                    objects.emplace_back(o);
                }
            } else if (condition == "all") {
                objects.emplace_back(o);
            } else {
                // no-groups, default
                if (!group) {
                    objects.emplace_back(o);
                    continue; // Non-groups cannot contain items.
                }
            }
            get_all_items_recursive(objects, o, condition);
        }
    }
}


/*
 * 'layers':            All layers.
 * 'groups':            All groups (including layers).
 * 'no-layers':         All top level objects in all layers (matches GUI "Select All in All Layers").
 * 'no-groups':         All objects other than groups (and layers).
 * 'all':               All objects including groups and their decendents.
 *
 * Note: GUI "Select All" requires knowledge of selected layer, which is a desktop property.
 */
void
select_all(Glib::ustring condition, InkscapeApplication* app)
{
    if (condition != "" && condition != "layers" && condition != "no-layers" &&
        condition != "groups" && condition != "no-groups" && condition != "all") {
        std::cerr << "select_all: allowed options are '', 'all', 'layers', 'no-layers', 'groups', and 'no-groups'" << std::endl;
        return;
    }

    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    std::vector<SPObject *> objects;
    get_all_items_recursive(objects, document->getRoot(), condition);

    selection->setList(objects);
}

/* See above for conditions. */
void
select_invert(Glib::ustring condition, InkscapeApplication* app)
{
    if (condition != "" && condition != "layers" && condition != "no-layers" &&
        condition != "groups" && condition != "no-groups" && condition != "all") {
        std::cerr << "select_all: allowed options are '', 'all', 'layers', 'no-layers', 'groups', and 'no-groups'" << std::endl;
        return;
    }

    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    // Find all objects that match condition.
    std::vector<SPObject *> objects;
    get_all_items_recursive(objects, document->getRoot(), condition);

    // Get current selection.
    std::vector<SPObject *> current(selection->items().begin(), selection->items().end());

    // Remove current selection from object vector (using "erase remove_if idiom").
    objects.erase(
        std::remove_if(std::begin(objects), std::end(objects), [&current](const SPObject *x)
            {
                return (std::find(current.begin(), current.end(), x) != current.end());
            }), objects.end());

    // Set selection to object vector.
    selection->setList(objects);
}


// Debug... print selected items
void
select_list(InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    auto items = selection->items();
    for (auto i = items.begin(); i != items.end(); ++i) {
        std::cout << **i << std::endl;
    }
}

// SHOULD REALLY BE DOC ACTIONS
std::vector<std::vector<Glib::ustring>> raw_data_selection =
{
    // clang-format off
    {"app.select-clear",           N_("Clear Selection"),         "Select",   N_("Clear selection")                                    },
    {"app.select",                 N_("Select"),                  "Select",   N_("Select by ID (deprecated)")                          },
    {"app.unselect",               N_("Unselect"),                "Select",   N_("Unselect by ID (deprecated)")                        },
    {"app.select-by-id",           N_("Select by Id"),            "Select",   N_("Select by ID")                                       },
    {"app.unselect-by-id",         N_("Unselect by Id"),          "Select",   N_("Unselect by ID")                                     },
    {"app.select-by-class",        N_("Select by Class"),         "Select",   N_("Select by class.")                                    },
    {"app.select-by-element",      N_("Select by Element"),       "Select",   N_("Select by SVG element (e.g. 'rect')")                },
    {"app.select-by-selector",     N_("Select by Selector"),      "Select",   N_("Select by CSS selector")                             },
    {"app.select-all",             N_("Select All"),              "Select",   N_("Select all; options: 'all' (every object including groups), 'layers', 'no-layers' (top level objects in layers), 'groups' (all groups including layers), 'no-groups' (all objects other than groups and layers, default)")},
    {"app.select-invert",          N_("Invert Selection"),        "Select",   N_("Invert selection; options: 'all', 'layers', 'no-layers', 'groups', 'no-groups' (default)")},
    {"app.select-list",            N_("List Selection"),          "Select",   N_("Print a list of objects in current selection")       }
    // clang-format on
};

void
add_actions_selection(InkscapeApplication* app)
{
    auto *gapp = app->gio_app();

    // clang-format off
    gapp->add_action(               "select-clear",       sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&select_clear),              app)        );
    gapp->add_action_radio_string(  "select",             sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&select_by_id),              app), "null"); // Backwards compatible.
    gapp->add_action_radio_string(  "unselect",           sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&unselect_by_id),            app), "null"); // Match select.
    gapp->add_action_radio_string(  "select-by-id",       sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&select_by_id),              app), "null");
    gapp->add_action_radio_string(  "unselect-by-id",     sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&unselect_by_id),            app), "null");
    gapp->add_action_radio_string(  "select-by-class",    sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&select_by_class),           app), "null");
    gapp->add_action_radio_string(  "select-by-element",  sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&select_by_element),         app), "null");
    gapp->add_action_radio_string(  "select-by-selector", sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&select_by_selector),        app), "null");
    gapp->add_action_radio_string(  "select-all",         sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&select_all),                app), "null");
    gapp->add_action_radio_string(  "select-invert",      sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&select_invert),             app), "null");
    gapp->add_action(               "select-list",        sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&select_list),               app)        );
    // clang-format on

    app->get_action_extra_data().add_data(raw_data_selection);
}


/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
