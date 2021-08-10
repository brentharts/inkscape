// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Marker edit mode - onCanvas marker editing of marker orientation, position, scale
 *//*
 * Authors:
 * see git history
 * Rachana Podaralla <rpodaralla3@gatech.edu>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include <iomanip>

#include <glibmm/ustring.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

#include "desktop.h"
#include "document.h"
#include "message-context.h"
#include "selection-chemistry.h"
#include "selection.h"
#include "snap.h"

#include "display/curve.h"
#include "display/control/canvas-item-bpath.h"
#include "display/control/canvas-item-group.h"

#include "live_effects/effect.h"
#include "live_effects/lpeobject.h"

#include "include/macros.h"

#include "object/sp-clippath.h"
#include "object/sp-item-group.h"
#include "object/sp-mask.h"
#include "object/sp-namedview.h"
#include "object/sp-path.h"
#include "object/sp-shape.h"
#include "object/sp-text.h"

#include "ui/shape-editor.h"
#include "ui/tool/control-point-selection.h"
#include "ui/tool/curve-drag-point.h"
#include "ui/tool/event-utils.h"
#include "ui/tool/multi-path-manipulator.h"
#include "ui/tool/path-manipulator.h"
#include "ui/tool/selector.h"
#include "ui/tools/marker-tool.h"

#include "object/sp-marker.h"
#include "style.h"

using Inkscape::DocumentUndo;
namespace Inkscape {
namespace UI {
namespace Tools {

const std::string& MarkerTool::getPrefsPath() {
	return MarkerTool::prefsPath;
}

const std::string MarkerTool::prefsPath = "/tools/marker";

MarkerTool::MarkerTool()
    : ToolBase("select.svg")
{}

/* This function uses similar logic that exists in sp_shape_update_marker_view, to calculate exactly where
the knotholders need to go and returns the edit_transform that is then loaded into the
ShapeEditor/PathManipulator/MultipathManipulator  */
ShapeRecord MarkerTool::get_marker_transform(SPShape* shape, SPItem *parent_item, SPItem *marker_item, SPMarkerLoc marker_type)
{
    SPObject *marker_obj = shape->_marker[marker_type];
    SPMarker *sp_marker = dynamic_cast<SPMarker *>(marker_obj);

    /* scale marker transform with parent stroke width */
    SPStyle *style = shape->style;
    Geom::Scale scale(style->stroke_width.computed * this->desktop->getDocument()->getDocumentScale()[Geom::X]);

    Geom::PathVector const &pathv = shape->curve()->get_pathvector();
    Geom::Affine ret = Geom::identity();
    double angle = 0.0;
    
    if(marker_type == SP_MARKER_LOC_START) {
        /* start marker location */
        Geom::Curve const &c = pathv.begin()->front();
        Geom::Point p = c.pointAt(0);
        ret = Geom::Translate(p * parent_item->i2dt_affine());

        if (!c.isDegenerate()) {
            Geom::Point tang = c.unitTangentAt(0);
            angle = Geom::atan2(tang);
            ret = Geom::Rotate(angle) * ret;
        }

    } else if(marker_type == SP_MARKER_LOC_MID) {
        /* mid marker - following the same logic from "sp_shape_update_marker_view" in sp-shape to calculate where markers 
        are being rendered for an object. For a mid marker - as soon as a location is found, exiting and passing that one single
        transform into the shape_record.
        */
        for(Geom::PathVector::const_iterator path_it = pathv.begin(); path_it != pathv.end(); ++path_it) {
            // mid marker start position
            if (path_it != pathv.begin() && ! ((path_it == (pathv.end()-1)) && (path_it->size_default() == 0)) ) // if this is the last path and it is a moveto-only, don't draw mid marker there
            {
                Geom::Curve const &c = path_it->front();
                Geom::Point p = c.pointAt(0);
                ret = Geom::Translate(p * parent_item->i2dt_affine());

                if (!c.isDegenerate()) {
                    Geom::Point tang = c.unitTangentAt(0);
                    angle = Geom::atan2(tang);
                    ret = Geom::Rotate(angle) * ret;
                    break;
                }
            }
            // mid marker mid positions
            if ( path_it->size_default() > 1) {
                Geom::Path::const_iterator curve_it1 = path_it->begin();      // incoming curve
                Geom::Path::const_iterator curve_it2 = ++(path_it->begin());  // outgoing curve
                while (curve_it2 != path_it->end_default())
                {
                    Geom::Curve const & c1 = *curve_it1;
                    Geom::Curve const & c2 = *curve_it2;

                    Geom::Point p = c1.pointAt(1);
                    
                    Geom::Curve * c1_reverse = c1.reverse();
                    Geom::Point tang1 = - c1_reverse->unitTangentAt(0);
                    delete c1_reverse;
                    Geom::Point tang2 = c2.unitTangentAt(0);

                    double const angle1 = Geom::atan2(tang1);
                    double const angle2 = Geom::atan2(tang2);

                    angle = .5 * (angle1 + angle2);

                    if ( fabs( angle2 - angle1 ) > M_PI ) {
                        angle += M_PI;
                    }

                    ret = Geom::Rotate(angle) * Geom::Translate(p * parent_item->i2dt_affine());

                    ++curve_it1;
                    ++curve_it2;
                    break;
                }
            }
            // mid marker end position
            if ( path_it != (pathv.end()-1) && !path_it->empty()) {
                Geom::Curve const &c = path_it->back_default();
                Geom::Point p = c.pointAt(1);
                ret = Geom::Translate(p * parent_item->i2dt_affine());

                if ( !c.isDegenerate() ) {
                    Geom::Curve * c_reverse = c.reverse();
                    Geom::Point tang = - c_reverse->unitTangentAt(0);
                    delete c_reverse;
                    angle = Geom::atan2(tang);
                    ret = Geom::Rotate(angle) * ret;
                    break;
                } 
            }
        }

    } else if (marker_type == SP_MARKER_LOC_END) {
        /* end marker location */
        Geom::Path const &path_last = pathv.back();
        unsigned int index = path_last.size_default();
        if (index > 0) {
            index--;
        }
        Geom::Curve const &c = path_last[index];
        Geom::Point p = c.pointAt(1);
        ret = Geom::Translate(p * parent_item->i2dt_affine());

        if ( !c.isDegenerate() ) {
            Geom::Curve * c_reverse = c.reverse();
            Geom::Point tang = - c_reverse->unitTangentAt(0);
            delete c_reverse;
            angle = Geom::atan2(tang);
            ret = Geom::Rotate(angle) * ret;
        } 
    }

    /* scale by stroke width */
    ret = scale * ret;
    /* account for parent transform */
    ret = parent_item->transform * ret;

    ShapeRecord sr;
    sr.object = marker_item;
    sr.edit_transform = ret;
    sr.edit_rotation = angle * 180.0/M_PI;
    sr.role = SHAPE_ROLE_NORMAL;
    return sr;
}

void MarkerTool::finish() {
    ungrabCanvasEvents();
    this->message_context->clear();
    this->sel_changed_connection.disconnect();
    ToolBase::finish();
}


MarkerTool::~MarkerTool() {
    this->enableGrDrag(false);
    this->sel_changed_connection.disconnect();
}

/* When a selection changes, if any selected objects have start/end/mid markers,
their markers are loaded into the ShapeEditor */
void MarkerTool::selection_changed(Inkscape::Selection *sel) {
    using namespace Inkscape::UI;

    std::set<ShapeRecord> shapes;
    auto selected_items = this->desktop->getSelection()->items();

    for(auto i = selected_items.begin(); i != selected_items.end(); ++i){
        SPItem *item = *i;
        if(item) {
            SPShape* shape = dynamic_cast<SPShape*>(item);

            if(shape && shape->hasMarkers() && (editMarkerMode != -1)) {

                SPObject *marker_obj = shape->_marker[editMarkerMode];

                if(marker_obj) {
                    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
                    SPDocument *doc = desktop->getDocument();

                    Inkscape::XML::Node *marker_repr = marker_obj->getRepr();
                    SPItem* marker_item = dynamic_cast<SPItem *>(this->desktop->getDocument()->getObjectByRepr(marker_repr));
                    SPMarker::validateMarker(marker_item, doc);

                    ShapeRecord sr;
                    switch(editMarkerMode) {
                        case SP_MARKER_LOC_START:
                            sr  = get_marker_transform(shape, item, marker_item, SP_MARKER_LOC_START);
                            break;
                        case SP_MARKER_LOC_MID:
                            sr  = get_marker_transform(shape, item, marker_item, SP_MARKER_LOC_MID);
                            break;
                        case SP_MARKER_LOC_END:
                            sr  = get_marker_transform(shape, item, marker_item, SP_MARKER_LOC_END);
                            break;
                        default:
                            break;
                    }  

                    shapes.insert(sr);
                    // exit after first found marker - force user to just select one marker at a time for editing
                    break;
                }
            }
        }
    }
    
    for (auto i = this->_shape_editors.begin(); i != this->_shape_editors.end();) {
        ShapeRecord s;
        s.object = dynamic_cast<SPObject *>(i->first);

        if (shapes.find(s) == shapes.end()) {
            this->_shape_editors.erase(i++);
        } else {
            ++i;
        }
    }

    for (const auto & r : shapes) {
        if (this->_shape_editors.find(SP_ITEM(r.object)) == this->_shape_editors.end()) {
            auto si = std::make_unique<ShapeEditor>(this->desktop, r.edit_transform, r.edit_rotation);
            SPItem *item = SP_ITEM(r.object);
            si->set_item(item);
            this->_shape_editors.insert({item, std::move(si)});
        }
    }
}

void MarkerTool::setup() {
    ToolBase::setup();
    Inkscape::Selection *selection = this->desktop->getSelection();

    this->sel_changed_connection.disconnect();
    this->sel_changed_connection = selection->connectChanged(
    	sigc::mem_fun(this, &MarkerTool::selection_changed)
    );

    this->selection_changed(selection);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/tools/marker/selcue")) this->enableSelectionCue();
    if (prefs->getBool("/tools/marker/gradientdrag")) this->enableGrDrag();
}

/* handles selection of items */
bool MarkerTool::root_handler(GdkEvent* event) {
    SPDesktop *desktop = this->desktop;
    Inkscape::Selection *selection = desktop->getSelection();
    gint ret = false;
    
    switch (event->type) {
        case GDK_BUTTON_PRESS:
            if (event->button.button == 1) {
                Geom::Point const button_w(event->button.x, event->button.y);  
                this->item_to_select = sp_event_context_find_item (desktop, button_w, event->button.state & GDK_MOD1_MASK, TRUE);
                grabCanvasEvents();
                ret = true;
            }
            break;
        case GDK_BUTTON_RELEASE:
            if (event->button.button == 1) {
                if (this->item_to_select) {
                    selection->toggle(this->item_to_select);
                } else {
                    selection->clear();
                }
                this->item_to_select = nullptr;
                ungrabCanvasEvents();
                ret = true;
            }
            break;
        default:
            break;
    }

    if (!ret) ret = ToolBase::root_handler(event);
    return ret;
}


}
}
}
