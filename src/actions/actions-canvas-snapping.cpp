// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for toggling snapping preferences. Tied to a particular document.
 *
 * Copyright (C) 2019 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <iostream>
#include <unordered_map>
#include <vector>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include "actions-canvas-snapping.h"
#include "actions/actions-extra-data.h"
#include "inkscape-application.h"
#include "inkscape.h"

#include "document.h"

#include "attributes.h"
#include "desktop.h"
#include "document-undo.h"

#include "object/sp-namedview.h"
#include "snap-enums.h"

using namespace Inkscape;

// There are two snapping lists that must be connected:
// 1. The Inkscape::SNAPTARGET value:  e.g. Inkscape::SNAPTARGET_BBOX_CATEGORY.
// 2. The Gio::Action name:            e.g. "snap-bbox"

struct SnapInfo {
    Glib::ustring action_name; // action name without "doc." prefix
    SnapTargetType type;       // corresponding snapping type
    bool set;                  // this is default for when "simple snapping" is ON and also initial value when preferences are deleted
};

typedef std::vector<SnapInfo> SnapVector;
typedef std::unordered_map<SnapTargetType, Glib::ustring> SnapMap;

SnapVector snap_bbox = {
    { "snap-bbox",               SNAPTARGET_BBOX_CATEGORY,      true },
    { "snap-bbox-edge",          SNAPTARGET_BBOX_EDGE,          true },
    { "snap-bbox-corner",        SNAPTARGET_BBOX_CORNER,        true },
    { "snap-bbox-edge-midpoint", SNAPTARGET_BBOX_EDGE_MIDPOINT, false },
    { "snap-bbox-center",        SNAPTARGET_BBOX_MIDPOINT,      false },
};

SnapVector snap_node = {
    { "snap-node-category",      SNAPTARGET_NODE_CATEGORY,      true },
    { "snap-path",               SNAPTARGET_PATH,               true },
    { "snap-path-intersection",  SNAPTARGET_PATH_INTERSECTION,  true },
    { "snap-node-cusp",          SNAPTARGET_NODE_CUSP,          true },
    { "snap-node-smooth",        SNAPTARGET_NODE_SMOOTH,        true },
    { "snap-line-midpoint",      SNAPTARGET_LINE_MIDPOINT,      true },
    { "snap-line-tangential",    SNAPTARGET_PATH_TANGENTIAL,    true },
    { "snap-line-perpendicular", SNAPTARGET_PATH_PERPENDICULAR, true },
};

SnapVector snap_alignment = {
    { "snap-alignment",          SNAPTARGET_ALIGNMENT_CATEGORY,    true },
    { "snap-alignment-self",     SNAPTARGET_ALIGNMENT_HANDLE,      false },
    // separate category:
    { "snap-distribution",       SNAPTARGET_DISTRIBUTION_CATEGORY, true },
};

SnapVector snap_all_the_rest = {
    { "snap-others",             SNAPTARGET_OTHERS_CATEGORY,    true },
    { "snap-object-midpoint",    SNAPTARGET_OBJECT_MIDPOINT,    false },
    { "snap-rotation-center",    SNAPTARGET_ROTATION_CENTER,    false },
    { "snap-text-baseline",      SNAPTARGET_TEXT_BASELINE,      true },
    { "snap-path-mask",          SNAPTARGET_PATH_MASK,          true },
    { "snap-path-clip",          SNAPTARGET_PATH_CLIP,          true },

    { "snap-page-border",        SNAPTARGET_PAGE_BORDER,        false },
    { "snap-grid",               SNAPTARGET_GRID,               true },
    { "snap-guide",              SNAPTARGET_GUIDE,              true },
};

const struct {const char* action_name; SimpleSnap option; bool set;} simple_snap_options[] = {
    { "simple-snap-bbox",      SimpleSnap::BBox,      true },
    { "simple-snap-nodes",     SimpleSnap::Nodes,     true },
    { "simple-snap-alignment", SimpleSnap::Alignment, false }
};

const SnapMap& get_snap_map() {
    static SnapMap map;
    if (map.empty()) {
        for (auto&& snap : snap_bbox)           { map[snap.type] = snap.action_name; }
        for (auto&& snap : snap_node)           { map[snap.type] = snap.action_name; }
        for (auto&& snap : snap_alignment)      { map[snap.type] = snap.action_name; }
        for (auto&& snap : snap_all_the_rest)   { map[snap.type] = snap.action_name; }
    }
    return map;
}

const SnapVector& get_snap_vect() {
    static SnapVector vect;
    if (vect.empty()) {
        for (auto v : {&snap_bbox, &snap_node, &snap_alignment, &snap_all_the_rest}) {
            vect.insert(vect.end(), v->begin(), v->end());
        }
    }
    return vect;
}

Glib::ustring snap_pref_path = "/options/snapping/";

// global and single location of snapping preferences
Inkscape::SnapPreferences& get_snapping_preferences() {
    static Inkscape::SnapPreferences preferences;
    static bool initialized = false;

    if (!initialized) {
        for (auto&& info : get_snap_vect()) {
            bool enabled = Inkscape::Preferences::get()->getBool(snap_pref_path + info.action_name, info.set);
    // g_warning("act: %s  en: %d", info.action_name.c_str(), info.set?1:0);
            preferences.setTargetSnappable(info.type, enabled);
        }
        for (auto&& info : simple_snap_options) {
            bool enabled = Inkscape::Preferences::get()->getBool(snap_pref_path + info.action_name, info.set);
            preferences.set_simple_snap(info.option, enabled);
        }
        initialized = true;
    }

    return preferences;
}

// Turn requested snapping type on or off:
// * type - snap target
// * enabled - true to turn it on, false to turn it off
//
void set_canvas_snapping(SnapTargetType type, bool enabled) {
    get_snapping_preferences().setTargetSnappable(type, enabled);

    auto it = get_snap_map().find(type);
    if (it == get_snap_map().end()) {
        g_warning("No action for snap target type %d", int(type));
    }
    else {
        auto&& action_name = it->second;
        Inkscape::Preferences::get()->setBool(snap_pref_path + action_name, enabled);
    }
}

void update_actions(SPDocument* document) {
    // Some actions depend on others... we need to update everything!
    set_actions_canvas_snapping(document);
}

static void canvas_snapping_toggle(SPDocument* document, SnapTargetType type) {
    bool enabled = get_snapping_preferences().isSnapButtonEnabled(type);
    set_canvas_snapping(type, !enabled);
    update_actions(document);
}


void set_simple_snap(SimpleSnap option, bool value) {
    const SnapVector* vect = nullptr;
    switch (option) {
    case SimpleSnap::BBox:
        vect = &snap_bbox;
        break;
    case SimpleSnap::Nodes:
        vect = &snap_node;
        break;
    case SimpleSnap::Alignment:
        vect = &snap_alignment;
        break;
    case SimpleSnap::Rest:
        vect = &snap_all_the_rest;
        break;
    default:
        std::cerr << "missing case statement in " << __func__ << std::endl;
        break;
    }

    if (vect) {
        for (auto&& info : *vect) {
            bool enable = value && info.set;
            set_canvas_snapping(info.type, enable);
        }

        Glib::ustring action_name;
        for (auto&& info : simple_snap_options) {
            if (info.option == option) {
                action_name = info.action_name;
                break;
            }
        }
        assert(!action_name.empty());
        get_snapping_preferences().set_simple_snap(option, value);
        Inkscape::Preferences::get()->setBool(snap_pref_path + action_name, value);
    }
}

void toggle_simple_snap_option(SPDocument* document, SimpleSnap option) {
    // toggle desired option
    bool enabled = !get_snapping_preferences().get_simple_snap(option);
    set_simple_snap(option, enabled);

    // reset others not visible / not exposed to their "simple" defaults
    for (auto&& info : snap_all_the_rest) {
        set_canvas_snapping(info.type, info.set);
    }

    update_actions(document);
}

void apply_simple_snap_defaults(SPDocument* document) {
    set_simple_snap(SimpleSnap::BBox, true);
    set_simple_snap(SimpleSnap::Nodes, true);
    set_simple_snap(SimpleSnap::Alignment, false);
    set_simple_snap(SimpleSnap::Rest, true);
    update_actions(document);
}

std::vector<std::vector<Glib::ustring>> raw_data_canvas_snapping =
{
    {"doc.snap-global-toggle",        N_("Snapping"),                          "Snap",  N_("Toggle snapping on/off")                             },

    {"doc.snap-alignment",            N_("Snap Objects that Align"),           "Snap",  N_("Toggle alignment snapping")                          },
    {"doc.snap-alignment-self",       N_("Snap Nodes that Align"),             "Snap",  N_("Toggle alignment snapping to nodes in the same path")},

    {"doc.snap-distribution",         N_("Snap Objects at Equal Distances"),   "Snap",  N_("Toggle snapping objects at equal distances")},

    {"doc.snap-bbox",                 N_("Snap Bounding Boxes"),               "Snap",  N_("Toggle snapping to bounding boxes (global)")         },
    {"doc.snap-bbox-edge",            N_("Snap Bounding Box Edges"),           "Snap",  N_("Toggle snapping to bounding-box edges")              },
    {"doc.snap-bbox-corner",          N_("Snap Bounding Box Corners"),         "Snap",  N_("Toggle snapping to bounding-box corners")            },
    {"doc.snap-bbox-edge-midpoint",   N_("Snap Bounding Box Edge Midpoints"),  "Snap",  N_("Toggle snapping to bounding-box edge mid-points")    },
    {"doc.snap-bbox-center",          N_("Snap Bounding Box Centers"),         "Snap",  N_("Toggle snapping to bounding-box centers")            },

    {"doc.snap-node-category",        N_("Snap Nodes"),                        "Snap",  N_("Toggle snapping to nodes (global)")                  },
    {"doc.snap-path",                 N_("Snap Paths"),                        "Snap",  N_("Toggle snapping to paths")                           },
    {"doc.snap-path-intersection",    N_("Snap Path Intersections"),           "Snap",  N_("Toggle snapping to path intersections")              },
    {"doc.snap-node-cusp",            N_("Snap Cusp Nodes"),                   "Snap",  N_("Toggle snapping to cusp nodes, including rectangle corners")},
    {"doc.snap-node-smooth",          N_("Snap Smooth Node"),                  "Snap",  N_("Toggle snapping to smooth nodes, including quadrant points of ellipses")},
    {"doc.snap-line-midpoint",        N_("Snap Line Midpoints"),               "Snap",  N_("Toggle snapping to midpoints of lines")              },
    {"doc.snap-line-perpendicular",   N_("Snap Perpendicular Lines"),          "Snap",  N_("Toggle snapping to perpendicular lines")             },
    {"doc.snap-line-tangential",      N_("Snap Tangential Lines"),             "Snap",  N_("Toggle snapping to tangential lines")                },

    {"doc.snap-others",               N_("Snap Others"),                       "Snap",  N_("Toggle snapping to misc. points (global)")           },
    {"doc.snap-object-midpoint",      N_("Snap Object Midpoint"),              "Snap",  N_("Toggle snapping to object midpoint")                 },
    {"doc.snap-rotation-center",      N_("Snap Rotation Center"),              "Snap",  N_("Toggle snapping to object rotation center")          },
    {"doc.snap-text-baseline",        N_("Snap Text Baselines"),               "Snap",  N_("Toggle snapping to text baseline and text anchors")  },

    {"doc.snap-page-border",          N_("Snap Page Border"),                  "Snap",  N_("Toggle snapping to page border")                     },
    {"doc.snap-grid",                 N_("Snap Grids"),                        "Snap",  N_("Toggle snapping to grids")                           },
    {"doc.snap-guide",                N_("Snap Guide Lines"),                  "Snap",  N_("Toggle snapping to guide lines")                     },

    {"doc.snap-path-mask",            N_("Snap Mask Paths"),                   "Snap",  N_("Toggle snapping to mask paths")                      },
    {"doc.snap-path-clip",            N_("Snap Clip Paths"),                   "Snap",  N_("Toggle snapping to clip paths")                      },

    {"doc.simple-snap-bbox",          N_("Simple Snap Bounding Box"),          "Snap",  N_("Toggle snapping to bounding boxes")                  },
    {"doc.simple-snap-nodes",         N_("Simple Snap Nodes"),                 "Snap",  N_("Toggle snapping to nodes")                           },
    {"doc.simple-snap-alignment",     N_("Simple Snap Alignment"),             "Snap",  N_("Toggle alignment snapping")                          },
};

void
add_actions_canvas_snapping(SPDocument* document)
{
    Glib::RefPtr<Gio::SimpleActionGroup> map = document->getActionGroup();

    map->add_action_bool("snap-global-toggle", [=]() {
        auto& pref = get_snapping_preferences();
        pref.setSnapEnabledGlobally(!pref.getSnapEnabledGlobally());
        update_actions(document);
    });

    for (auto&& info : get_snap_vect()) {
        map->add_action_bool(info.action_name, [=](){ canvas_snapping_toggle(document, info.type); });
    }

    // Simple snapping popover
    for (auto&& info : simple_snap_options) {
        map->add_action_bool(info.action_name, [=](){ toggle_simple_snap_option(document, info.option); });
    }

    // Check if there is already an application instance (GUI or non-GUI).
    auto app = InkscapeApplication::instance();
    if (!app) {
        std::cerr << "add_actions_canvas_snapping: no app!" << std::endl;
        return;
    }
    app->get_action_extra_data().add_data(raw_data_canvas_snapping);

    update_actions(document);
}


void
set_actions_canvas_snapping_helper (Glib::RefPtr<Gio::SimpleActionGroup>& map, Glib::ustring action_name, bool state, bool enabled)
{
    // Glib::RefPtr<Gio::SimpleAction> saction = map->lookup_action(action_name); NOT POSSIBLE!

    // We can't enable/disable action directly! (Gio::Action can "get" enabled value but can not
    // "set" it! We need to cast to Gio::SimpleAction)
    Glib::RefPtr<Gio::Action> action = map->lookup_action(action_name);
    if (!action) {
        std::cerr << "set_actions_canvas_snapping_helper: action " << action_name << " missing!" << std::endl;
        return;
    }

    auto simple = Glib::RefPtr<Gio::SimpleAction>::cast_dynamic(action);
    if (!simple) {
        std::cerr << "set_actions_canvas_snapping_helper: action " << action_name << " not SimpleAction!" << std::endl;
        return;
    }

    simple->change_state(state);
    simple->set_enabled(enabled);
}

void set_actions_canvas_snapping(SPDocument* document) {
    Glib::RefPtr<Gio::SimpleActionGroup> map = document->getActionGroup();
    if (!map) {
        std::cerr << "set_actions_canvas_snapping: no ActionGroup!" << std::endl;
        return;
    }

    auto& snapprefs = get_snapping_preferences();
    bool global = snapprefs.getSnapEnabledGlobally();
    bool alignment = snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_ALIGNMENT_CATEGORY);
    bool distribution = snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_DISTRIBUTION_CATEGORY);
    bool bbox = snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_BBOX_CATEGORY);
    bool node = snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_NODE_CATEGORY);
    bool other = snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_OTHERS_CATEGORY);

    struct { const char* action; bool state; bool enabled; } snap_options[] = {
        { "snap-global-toggle", global, true }, // Always enabled

        { "snap-alignment", alignment, global },
        { "snap-alignment-self",     snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_ALIGNMENT_HANDLE),   global && alignment },

        { "snap-distribution", distribution, global },

        { "snap-bbox", bbox, global },
        { "snap-bbox-edge",          snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_EDGE),          global && bbox },
        { "snap-bbox-corner",        snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_CORNER),        global && bbox },
        { "snap-bbox-edge-midpoint", snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_EDGE_MIDPOINT), global && bbox },
        { "snap-bbox-center",        snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_MIDPOINT),      global && bbox },

        { "snap-node-category", node, global },
        { "snap-path",               snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH),               global && node },
        { "snap-path-intersection",  snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_INTERSECTION),  global && node },
        { "snap-node-cusp",          snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_NODE_CUSP),          global && node },
        { "snap-node-smooth",        snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_NODE_SMOOTH),        global && node },
        { "snap-line-midpoint",      snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_LINE_MIDPOINT),      global && node },
        { "snap-line-tangential",    snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_TANGENTIAL),    global && node },
        { "snap-line-perpendicular", snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_PERPENDICULAR), global && node },

        { "snap-others", other, global },
        { "snap-object-midpoint",    snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_OBJECT_MIDPOINT),    global && other },
        { "snap-rotation-center",    snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_ROTATION_CENTER),    global && other },
        { "snap-text-baseline",      snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_TEXT_BASELINE),      global && other },

        { "snap-page-border",        snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PAGE_BORDER),        global },
        { "snap-grid",               snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_GRID),               global },
        { "snap-guide",              snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_GUIDE),              global },

        { "snap-path-clip",          snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_CLIP),          global },
        { "snap-path-mask",          snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_MASK),          global },

        { "simple-snap-bbox", bbox, global },
        { "simple-snap-nodes", node, global },
        { "simple-snap-alignment", alignment, global },
    };

    for (auto&& snap : snap_options) {
        set_actions_canvas_snapping_helper(map, snap.action, snap.state, snap.enabled);
    }

//////////////////////////////////////////
/*
    bool global = nv->snap_manager.snapprefs.getSnapEnabledGlobally();
    set_actions_canvas_snapping_helper(map, "snap-global-toggle", global, true); // Always enabled

    bool alignment = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_ALIGNMENT_CATEGORY);
    set_actions_canvas_snapping_helper(map, "snap-alignment", alignment, global);
    set_actions_canvas_snapping_helper(map, "snap-alignment-self",     nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_ALIGNMENT_HANDLE),   global && alignment);

    bool distribution = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_DISTRIBUTION_CATEGORY);
    set_actions_canvas_snapping_helper(map, "snap-distribution", distribution, global);

    bool bbox = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_BBOX_CATEGORY);
    set_actions_canvas_snapping_helper(map, "snap-bbox", bbox, global);
    set_actions_canvas_snapping_helper(map, "snap-bbox-edge",          nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_EDGE),          global && bbox);
    set_actions_canvas_snapping_helper(map, "snap-bbox-corner",        nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_CORNER),        global && bbox);
    set_actions_canvas_snapping_helper(map, "snap-bbox-edge-midpoint", nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_EDGE_MIDPOINT), global && bbox);
    set_actions_canvas_snapping_helper(map, "snap-bbox-center",        nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_MIDPOINT),      global && bbox);

    bool node = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_NODE_CATEGORY);
    set_actions_canvas_snapping_helper(map, "snap-node-category", node, global);
    set_actions_canvas_snapping_helper(map, "snap-path",               nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH),               global && node);
    set_actions_canvas_snapping_helper(map, "snap-path-intersection",  nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_INTERSECTION),  global && node);
    set_actions_canvas_snapping_helper(map, "snap-node-cusp",          nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_NODE_CUSP),          global && node);
    set_actions_canvas_snapping_helper(map, "snap-node-smooth",        nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_NODE_SMOOTH),        global && node);
    set_actions_canvas_snapping_helper(map, "snap-line-midpoint",      nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_LINE_MIDPOINT),      global && node);

    bool other = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_OTHERS_CATEGORY);
    set_actions_canvas_snapping_helper(map, "snap-others", other, global);
    set_actions_canvas_snapping_helper(map, "snap-object-midpoint",    nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_OBJECT_MIDPOINT),    global && other);
    set_actions_canvas_snapping_helper(map, "snap-rotation-center",    nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_ROTATION_CENTER),    global && other);
    set_actions_canvas_snapping_helper(map, "snap-text-baseline",      nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_TEXT_BASELINE),      global && other);

    set_actions_canvas_snapping_helper(map, "snap-page-border",        nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PAGE_BORDER),        global);
    set_actions_canvas_snapping_helper(map, "snap-grid",               nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_GRID),               global);
    set_actions_canvas_snapping_helper(map, "snap-guide",              nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_GUIDE),              global);

    set_actions_canvas_snapping_helper(map, "snap-path-clip",          nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_CLIP),          global);
    set_actions_canvas_snapping_helper(map, "snap-path-mask",          nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_MASK),          global);

    set_actions_canvas_snapping_helper(map, "simple-snap-bbox", bbox, global);
    set_actions_canvas_snapping_helper(map, "simple-snap-nodes", node, global);
    set_actions_canvas_snapping_helper(map, "simple-snap-alignment", alignment, global);
*/
}

/**
 * Simple snapping groups existing "advanced" options into couple of easy to understand choices (bounding box, nodes).
 * Behind the scene the same snapping properties to used. When entering "simple" mode those snapping properties need to be set
 * to the correct default values; advanced mode affords complete freedom in selecting them, simple mode restricts them.
 * 
 * Simple mode is a global preference, whereas snapping options are per-document. This is a source of contention.
 * There will be situations where open document has snapping settings inconsistent with simple snapping defaults.
 */
void transition_to_simple_snapping() {
    std::list<SPDesktop*> desktop_list;
    INKSCAPE.get_all_desktops(desktop_list);
    for (auto desktop : desktop_list) {
        if (desktop) {
            if (auto document = desktop->getDocument()) {
                apply_simple_snap_defaults(document); 
            }
        }
    }
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
