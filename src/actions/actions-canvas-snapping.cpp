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

// There are four snapping lists that must be connected:
// 1. The attribute name in NamedView: e.g. "inkscape:snap-bbox".
// 2. The SPAttr value:       e.g. SPAttr::INKSCAPE_SNAP_BBOX.
// 3. The Inkscape::SNAPTARGET value:  e.g. Inkscape::SNAPTARGET_BBOX_CATEGORY.
// 4. The Gio::Action name:            e.g. "snap-bbox"
// It seems we could simplify this somehow.

// This might work better as a class.

// record 'option' in a corresponding document attribute;
// set it to a value of 'set' if given, or toggle if there's no value specified
void set_canvas_snapping(SPDocument* document, const SPAttr option, std::optional<bool> set) {
    Inkscape::XML::Node* repr = document->getReprNamedView();

    if (repr == nullptr) {
        std::cerr << __func__ << ": namedview XML repr missing!" << std::endl;
        return;
    }

    // This is a bit awkward.
    SPObject* obj = document->getObjectByRepr(repr);
    SPNamedView* nv = dynamic_cast<SPNamedView *> (obj);
    if (nv == nullptr) {
        std::cerr << __func__ << ": no namedview!" << std::endl;
        return;
    }

    // Disable undo
    Inkscape::DocumentUndo::ScopedInsensitive _no_undo(document);

    bool v = false;
    auto value = [&](bool val){ return set.value_or(val); };

    switch (option) {
        case SPAttr::INKSCAPE_SNAP_GLOBAL:
            v = nv->snap_manager.snapprefs.getSnapEnabledGlobally();
            repr->setAttributeBoolean("inkscape:snap-global", value(!v));
            break;

        case SPAttr::INKSCAPE_SNAP_ALIGNMENT:
            v = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_ALIGNMENT_CATEGORY);
            repr->setAttributeBoolean("inkscape:snap-alignment", value(!v));
            break;
        case SPAttr::INKSCAPE_SNAP_ALIGNMENT_SELF:
            v = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_ALIGNMENT_HANDLE);
            repr->setAttributeBoolean("inkscape:snap-alignment-self", value(!v));
            break;

        case SPAttr::INKSCAPE_SNAP_DISTRIBUTION:
            v = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_DISTRIBUTION_CATEGORY);
            repr->setAttributeBoolean("inkscape:snap-distribution", value(!v));
            break;

        // BBox
        case SPAttr::INKSCAPE_SNAP_BBOX:
            v = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_BBOX_CATEGORY);
            repr->setAttributeBoolean("inkscape:snap-bbox", value(!v));
            break;

        case SPAttr::INKSCAPE_SNAP_BBOX_EDGE:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_EDGE);
            repr->setAttributeBoolean("inkscape:bbox-paths", value(!v));
            break;

        case SPAttr::INKSCAPE_SNAP_BBOX_CORNER:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_CORNER);
            repr->setAttributeBoolean("inkscape:bbox-nodes", value(!v));
            break;

        case SPAttr::INKSCAPE_SNAP_BBOX_EDGE_MIDPOINT:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_EDGE_MIDPOINT);
            repr->setAttributeBoolean("inkscape:snap-bbox-edge-midpoints", value(!v));
            break;

        case SPAttr::INKSCAPE_SNAP_BBOX_MIDPOINT:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_MIDPOINT);
            repr->setAttributeBoolean("inkscape:snap-bbox-midpoints", value(!v));
            break;

        // Nodes
        case SPAttr::INKSCAPE_SNAP_NODE:
            v = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_NODE_CATEGORY);
            repr->setAttributeBoolean("inkscape:snap-nodes", value(!v));
            break;

        case SPAttr::INKSCAPE_SNAP_PATH:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH);
            repr->setAttributeBoolean("inkscape:object-paths", value(!v));
            break;

        case SPAttr::INKSCAPE_SNAP_PATH_INTERSECTION:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_INTERSECTION);
            repr->setAttributeBoolean("inkscape:snap-intersection-paths", value(!v));
            break;

        case SPAttr::INKSCAPE_SNAP_NODE_CUSP:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_NODE_CUSP);
            repr->setAttributeBoolean("inkscape:object-nodes", value(!v));
            break;

        case SPAttr::INKSCAPE_SNAP_NODE_SMOOTH:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_NODE_SMOOTH);
            repr->setAttributeBoolean("inkscape:snap-smooth-nodes", value(!v));
            break;


        case SPAttr::INKSCAPE_SNAP_LINE_MIDPOINT:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_LINE_MIDPOINT);
            repr->setAttributeBoolean("inkscape:snap-midpoints", value(!v));
            break;

        // Others
        case SPAttr::INKSCAPE_SNAP_OTHERS:
            v = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_OTHERS_CATEGORY);
            repr->setAttributeBoolean("inkscape:snap-others", value(!v));
            break;

        case SPAttr::INKSCAPE_SNAP_OBJECT_MIDPOINT:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_OBJECT_MIDPOINT);
            repr->setAttributeBoolean("inkscape:snap-object-midpoints", value(!v));
            break;

        case SPAttr::INKSCAPE_SNAP_ROTATION_CENTER:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_ROTATION_CENTER);
            repr->setAttributeBoolean("inkscape:snap-center", value(!v));
            break;

        case SPAttr::INKSCAPE_SNAP_TEXT_BASELINE:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_TEXT_BASELINE);
            repr->setAttributeBoolean("inkscape:snap-text-baseline", value(!v));
            break;

        // Page/Grid/Guides
        case SPAttr::INKSCAPE_SNAP_PAGE_BORDER:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PAGE_BORDER);
            repr->setAttributeBoolean("inkscape:snap-page", value(!v));
            break;

        case SPAttr::INKSCAPE_SNAP_GRID:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_GRID);
            repr->setAttributeBoolean("inkscape:snap-grids", value(!v));
            break;

        case SPAttr::INKSCAPE_SNAP_GUIDE:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_GUIDE);
            repr->setAttributeBoolean("inkscape:snap-to-guides", value(!v));
            break;

        // Not used in default snap toolbar
        case SPAttr::INKSCAPE_SNAP_PATH_CLIP:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_CLIP);
            repr->setAttributeBoolean("inkscape:snap-path-clip", value(!v));
            break;
        case SPAttr::INKSCAPE_SNAP_PATH_MASK:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_MASK);
            repr->setAttributeBoolean("inkscape:snap-path-mask", value(!v));
            break;

        case SPAttr::INKSCAPE_SNAP_PERP:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_PERPENDICULAR);
            repr->setAttributeBoolean("inkscape:snap-perpendicular", value(!v));
            //TODO: global preferences
            break;
        case SPAttr::INKSCAPE_SNAP_TANG:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_TANGENTIAL);
            repr->setAttributeBoolean("inkscape:snap-tangential", value(!v));
            //TODO: global preferences
            break;

        default:
            std::cerr << "canvas_snapping_toggle: unhandled option: " << (int)option << std::endl;
    }
}

void update_actions(SPDocument* document) {
    // Some actions depend on others... we need to update everything!
    set_actions_canvas_snapping(document);

    // The snapping preferences are stored in the document, and therefore toggling makes the document dirty.
    document->setModifiedSinceSave();
}

static void
canvas_snapping_toggle(SPDocument* document, const SPAttr option)
{
    set_canvas_snapping(document, option, std::optional<bool>());
    update_actions(document);
}

struct SnapInfo {
    Glib::ustring name;         // action name without "doc." prefix
    SPAttr attr;                // corresponding attribute
    // for simple snapping use only:
    std::optional<bool> set;    // if given this is default for when "simple snapping" is ON and this option is not exposed in the UI
                                // if not present it can be toggled by simple snapping dialog (and this option is exposed in the UI)
};

typedef std::vector<SnapInfo> SnapVector;

SnapVector snap_bbox = {
    { "snap-bbox",               SPAttr::INKSCAPE_SNAP_BBOX },
    { "snap-bbox-edge",          SPAttr::INKSCAPE_SNAP_BBOX_EDGE,          true },
    { "snap-bbox-corner",        SPAttr::INKSCAPE_SNAP_BBOX_CORNER,        true },
    { "snap-bbox-edge-midpoint", SPAttr::INKSCAPE_SNAP_BBOX_EDGE_MIDPOINT, false },
    { "snap-bbox-center",        SPAttr::INKSCAPE_SNAP_BBOX_MIDPOINT,      false },
};

SnapVector snap_node = {
    { "snap-node-category",      SPAttr::INKSCAPE_SNAP_NODE },
    { "snap-path",               SPAttr::INKSCAPE_SNAP_PATH,               false },
    { "snap-path-intersection",  SPAttr::INKSCAPE_SNAP_PATH_INTERSECTION,  false },
    { "snap-node-cusp",          SPAttr::INKSCAPE_SNAP_NODE_CUSP,          true },
    { "snap-node-smooth",        SPAttr::INKSCAPE_SNAP_NODE_SMOOTH,        true },
    { "snap-line-midpoint",      SPAttr::INKSCAPE_SNAP_LINE_MIDPOINT,      false },
    { "snap-line-tangential",    SPAttr::INKSCAPE_SNAP_TANG,               false },
    { "snap-line-perpendicular", SPAttr::INKSCAPE_SNAP_PERP,               false },
};

SnapVector snap_alignment = {
    { "snap-alignment",          SPAttr::INKSCAPE_SNAP_ALIGNMENT },
    { "snap-distribution",       SPAttr::INKSCAPE_SNAP_DISTRIBUTION,       true },
    { "snap-alignment-self",     SPAttr::INKSCAPE_SNAP_ALIGNMENT_SELF,     false },
    { "snap-bbox",               SPAttr::INKSCAPE_SNAP_BBOX,               true },
    { "snap-bbox-edge",          SPAttr::INKSCAPE_SNAP_BBOX_EDGE,          true },
    { "snap-bbox-corner",        SPAttr::INKSCAPE_SNAP_BBOX_CORNER,        false },
    { "snap-bbox-center",        SPAttr::INKSCAPE_SNAP_BBOX_MIDPOINT,      false },
    { "snap-bbox-edge-midpoint", SPAttr::INKSCAPE_SNAP_BBOX_EDGE_MIDPOINT, false },
};

SnapVector snap_all_the_rest = {
    { "snap-others",             SPAttr::INKSCAPE_SNAP_OTHERS,            true },
    { "snap-object-midpoint",    SPAttr::INKSCAPE_SNAP_OBJECT_MIDPOINT,   false },
    { "snap-rotation-center",    SPAttr::INKSCAPE_SNAP_ROTATION_CENTER,   false },
    { "snap-text-baseline",      SPAttr::INKSCAPE_SNAP_TEXT_BASELINE,     true },
    { "snap-path-mask",          SPAttr::INKSCAPE_SNAP_PATH_MASK,         true },
    { "snap-path-clip",          SPAttr::INKSCAPE_SNAP_PATH_CLIP,         true },

    { "snap-page-border",        SPAttr::INKSCAPE_SNAP_PAGE_BORDER,       false },
    { "snap-grid",               SPAttr::INKSCAPE_SNAP_GRID,              true },
    { "snap-guide",              SPAttr::INKSCAPE_SNAP_GUIDE,             true },
};

enum class SimpleSnap { BBox, Nodes, Alignment, Rest };

void set_simple_snap(SPDocument* document, SimpleSnap option, bool toggle) {
    Inkscape::XML::Node* repr = document->getReprNamedView();

    if (repr == nullptr) {
        std::cerr << __func__ << ": namedview XML repr missing!" << std::endl;
        return;
    }

    SPObject* obj = document->getObjectByRepr(repr);
    SPNamedView* nv = dynamic_cast<SPNamedView*>(obj);
    if (nv == nullptr) {
        std::cerr << __func__ << ": no namedview!" << std::endl;
        return;
    }

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
            if (toggle) {
                set_canvas_snapping(document, info.attr, info.set);
            }
            else {
                set_canvas_snapping(document, info.attr, info.set.value_or(false));
            }
        }

        update_actions(document);
    }
}

void toggle_simple_snap_option(SPDocument* document, SimpleSnap option) {
    // toggle desired option
    set_simple_snap(document, option, true);

    // reset others not visible / not exposed to their "simple" defaults
    for (auto&& info : snap_all_the_rest) {
        set_canvas_snapping(document, info.attr, info.set);
    }
}

void apply_simple_snap_defaults(SPDocument* document) {
    set_simple_snap(document, SimpleSnap::BBox, false);
    set_simple_snap(document, SimpleSnap::Nodes, false);
    set_simple_snap(document, SimpleSnap::Alignment, false);
    set_simple_snap(document, SimpleSnap::Rest, false);
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

    map->add_action_bool( "snap-global-toggle",      sigc::bind<SPDocument*, SPAttr>(sigc::ptr_fun(&canvas_snapping_toggle),  document, SPAttr::INKSCAPE_SNAP_GLOBAL));

    for (auto&& info : snap_bbox) {
        map->add_action_bool(info.name, [=](){ canvas_snapping_toggle(document, info.attr); });
    }

    for (auto&& info : snap_node) {
        map->add_action_bool(info.name, [=](){ canvas_snapping_toggle(document, info.attr); });
    }

    for (auto&& info : snap_alignment) {
        map->add_action_bool(info.name, [=](){ canvas_snapping_toggle(document, info.attr); });
    }

    for (auto&& info : snap_all_the_rest) {
        map->add_action_bool(info.name, [=](){ canvas_snapping_toggle(document, info.attr); });
    }

    // Simple snapping popover
    map->add_action_bool("simple-snap-bbox",      [=](){ toggle_simple_snap_option(document, SimpleSnap::BBox); });
    map->add_action_bool("simple-snap-nodes",     [=](){ toggle_simple_snap_option(document, SimpleSnap::Nodes); });
    map->add_action_bool("simple-snap-alignment", [=](){ toggle_simple_snap_option(document, SimpleSnap::Alignment); });

    // Check if there is already an application instance (GUI or non-GUI).
    auto app = InkscapeApplication::instance();
    if (!app) {
        std::cerr << "add_actions_canvas_snapping: no app!" << std::endl;
        return;
    }
    app->get_action_extra_data().add_data(raw_data_canvas_snapping);
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

void
set_actions_canvas_snapping(SPDocument* document)
{
    Inkscape::XML::Node* repr = document->getReprNamedView();

    if (repr == nullptr) {
        std::cerr << "set_actions_canvas_snapping: namedview XML repr missing!" << std::endl;
        return;
    }

    // This is a bit awkward.
    SPObject* obj = document->getObjectByRepr(repr);
    SPNamedView* nv = dynamic_cast<SPNamedView *> (obj);

    if (nv == nullptr) {
        std::cerr << "set_actions_canvas_snapping: no namedview!" << std::endl;
        return;
    }

    Glib::RefPtr<Gio::SimpleActionGroup> map = document->getActionGroup();
    if (!map) {
        std::cerr << "set_actions_canvas_snapping: no ActionGroup!" << std::endl;
        return;
    }

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
    set_actions_canvas_snapping_helper(map, "simple-snap-alignment", false, global); // not available just yet
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
