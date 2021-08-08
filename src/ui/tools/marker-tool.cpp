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

namespace Inkscape {
namespace UI {
namespace Tools {

MarkerTool::MarkerTool()
    : NodeTool()
{
}

void MarkerTool::finish() 
{
    this->_selected_nodes->clear();
    // Clear markers selected on exit from marker tool
    this->desktop->selection->clear();
    ToolBase::finish();
}

/** Recursively get all shapes within a marker when marker mode 2 is entered
This function is very similar to the one that exists in node-tool.cpp
TODO: look at case when marker has group transforms underneath **/
static bool gather_items(NodeTool *nt, SPItem *base, SPObject *obj, Inkscape::UI::ShapeRole role,
    std::set<Inkscape::UI::ShapeRecord> &s, Inkscape::Selection *sel, Geom::Affine &tr)
{
    using namespace Inkscape::UI;

    if (!obj) {
        return false;
    }

    if (SP_IS_GROUP(obj) || SP_IS_OBJECTGROUP(obj) || SP_IS_MARKER(obj)) {
        for (auto& c: obj->children) {
            gather_items(nt, base, &c, role, s, sel, tr);
        }
    } else if (SP_IS_ITEM(obj)) {
        SPObject *object = obj;
        SPItem *item = dynamic_cast<SPItem *>(obj);
        ShapeRecord r;
        r.object = object;
        r.edit_transform = tr;
        r.role = role;

        if (s.insert(r).second) {
            //sel->add(obj);
            // this item was encountered the first time
            if (nt->edit_clipping_paths) {
                gather_items(nt, item, item->getClipObject(), SHAPE_ROLE_CLIPPING_PATH, s, sel, tr);
            }

            if (nt->edit_masks) {
                gather_items(nt, item, item->getMaskObject(), SHAPE_ROLE_MASK, s, sel, tr);
            }
        } else return false;
    }

    return true;
}

/* This function uses similar logic that exists in sp_shape_update_marker_view, to calculate exactly where
the knotholders need to go and returns the edit_transform that is then loaded into the
ShapeEditor/PathManipulator/MultipathManipulator  */
Geom::Affine MarkerTool::get_marker_transform(SPShape* shape, SPItem *parent_item, SPItem *marker_item, SPMarkerLoc marker_type)
{
    SPObject *marker_obj = shape->_marker[marker_type];
    SPMarker *sp_marker = dynamic_cast<SPMarker *>(marker_obj);

    /* scale marker transform with parent stroke width */
    SPStyle *style = shape->style;
    Geom::Scale scale(style->stroke_width.computed * this->desktop->getDocument()->getDocumentScale()[Geom::X]);

    Geom::PathVector const &pathv = shape->curve()->get_pathvector();
    Geom::Affine ret = Geom::identity();
    
    if(marker_type == SP_MARKER_LOC_START) {
        /* start marker location */
        Geom::Curve const &c = pathv.begin()->front();
        Geom::Point p = c.pointAt(0);
        ret = Geom::Translate(p * parent_item->i2dt_affine());

        if (!c.isDegenerate()) {
            Geom::Point tang = c.unitTangentAt(0);
            double const angle = Geom::atan2(tang);
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
                    double const angle = Geom::atan2(tang);
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

                    double ret_angle = .5 * (angle1 + angle2);

                    if ( fabs( angle2 - angle1 ) > M_PI ) {
                        ret_angle += M_PI;
                    }

                    ret = Geom::Rotate(ret_angle) * Geom::Translate(p * parent_item->i2dt_affine());

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
                    double const angle = Geom::atan2(tang);
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
            double const angle = Geom::atan2(tang);
            ret = Geom::Rotate(angle) * ret;
        } 
    }

    /* scale by stroke width */
    ret = scale * ret;
    /* account for parent transform */
    ret = parent_item->transform * ret;
    return ret;
}

/* When a selection changes, if any selected objects have start/end/mid markers,
they are loaded into the ShapeEditor/PathManipulator/MultipathManipulator */
void MarkerTool::selection_changed(Inkscape::Selection *sel) {
    using namespace Inkscape::UI;

    std::set<ShapeRecord> shapes;
    auto selected_items = this->desktop->getSelection()->items();

    for(auto i = selected_items.begin(); i != selected_items.end(); ++i){
        SPItem *item = *i;
        if(item) {
            SPShape* shape = dynamic_cast<SPShape*>(item);

            if(shape && shape->hasMarkers()) {
                for(int i = 0; i < SP_MARKER_LOC_QTY; i++) {
                    SPObject *marker_obj = shape->_marker[i];

                    if(marker_obj) {
                        Inkscape::XML::Node *marker_repr = marker_obj->getRepr();
                        SPItem* marker_item = dynamic_cast<SPItem *>(this->desktop->getDocument()->getObjectByRepr(marker_repr));

                        Geom::Affine marker_tr = Geom::identity();

                        switch(i) {
                            case SP_MARKER_LOC_START:
                                marker_tr  = get_marker_transform(shape, item, marker_item, SP_MARKER_LOC_START);
                                break;
                            case SP_MARKER_LOC_MID:
                                marker_tr  = get_marker_transform(shape, item, marker_item, SP_MARKER_LOC_MID);
                                break;
                            case SP_MARKER_LOC_END:
                                marker_tr  = get_marker_transform(shape, item, marker_item, SP_MARKER_LOC_END);
                                break;
                            default:
                                break;
                        }  

                        if(enter_marker_mode) {
                            if (gather_items(this, nullptr, marker_item, SHAPE_ROLE_NORMAL, shapes, sel, marker_tr)) {
                                // remove parent item here maybe ? - this is extra stuff related to selection that might be remove - ignore for now
                            }

                        } else {
                            ShapeRecord sr;
                            sr.object = marker_item;
                            sr.edit_transform = marker_tr;
                            sr.role = SHAPE_ROLE_NORMAL;
                            if (shapes.insert(sr).second) {;
                                //sel->add(marker_item);
                                //sel->remove(item); remove parent item here maybe ?
                            }
                        }
                    }
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
            auto si = std::make_unique<ShapeEditor>(this->desktop, r.edit_transform, true);
            SPItem *item = SP_ITEM(r.object);
            si->set_item(item);
            this->_shape_editors.insert({item, std::move(si)});
        }
    }

    std::vector<SPItem *> vec(sel->items().begin(), sel->items().end());
    _previous_selection = _current_selection;
    _current_selection = vec;
    this->_multipath->setItems(shapes);
    this->update_tip(nullptr);
    sp_update_helperpath(desktop);
}

/* Toggles between two marker modes 1 and 2 using shift+z */
bool MarkerTool::root_handler_extended(GdkEvent* event) {
    Inkscape::Selection *selection = this->desktop->getSelection();

    if(enter_marker_mode) {
        enter_marker_mode = false;
        this->selection_changed(selection);
    } else {
        enter_marker_mode = true;
        this->selection_changed(selection);
    }
    return TRUE;
}

}
}
}

