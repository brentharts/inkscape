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

// Clean selection on tool change
void MarkerTool::finish() 
{
    this->_selected_nodes->clear();
    this->desktop->selection->clear();
    ToolBase::finish();
}

/** Recursively get all shapes within a marker when marker mode is entered **/
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
        // TODO add support for objectBoundingBox
        r.edit_transform = base ? base->i2doc_affine() : Geom::identity();
        //r.edit_transform = tr;
        r.role = role;

        if (s.insert(r).second) {
            sel->add(obj);
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

Geom::Affine MarkerTool::get_marker_transform(Geom::Curve const & c, SPItem *item)
{
    Geom::Point p = c.pointAt(0);
    Geom::Point t = Geom::Point(Inkscape::Util::Quantity::convert(p[Geom::X], "mm", "px"), 
                    Inkscape::Util::Quantity::convert(p[Geom::Y], "mm", "px")); // ??? Come back
    Geom::Affine ret = Geom::Translate(t);

    if ( !c.isDegenerate() ) {
        Geom::Point tang = c.unitTangentAt(0);
        double const angle = Geom::atan2(tang);
        ret = Geom::Rotate(angle) * ret;
    }
    
    return ret;
}


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

                        if(enter_marker_mode) {
                            /* Scale marker transform with parent stroke width */
                            SPMarker *sp_marker = dynamic_cast<SPMarker *>(marker_obj);

                            SPStyle *style = shape->style;
                            Geom::Scale scale(Inkscape::Util::Quantity::convert(style->stroke_width.computed, "mm", "px")); // TODO!! come back and check

                            Geom::PathVector const &pathv = shape->curve()->get_pathvector();
                            Geom::Affine const marker_transform(get_marker_transform(pathv.begin()->front(), item));
                            Geom::Affine tr(marker_transform);
                            tr = scale * tr;
                            tr = sp_marker->c2p * tr;

                            if (gather_items(this, nullptr, marker_item, SHAPE_ROLE_NORMAL, shapes, sel, tr)) {
                                // remove
                            }
                        } else {
                            ShapeRecord sr;
                            sr.object = marker_item;
                            sr.edit_transform = Geom::identity();
                            sr.role = SHAPE_ROLE_NORMAL;
                            if (shapes.insert(sr).second) {;
                                sel->add(marker_item);
                                //sel->remove(item);
                            }
                        }

                        /* Trying to get transform in right place :'(  (works kinda unless parents/children have transforms too) */
                        
                        //tr =  (Geom::Affine)sp_marker->c2p * tr;

                        // tr.setTranslation(Geom::Point(
                        //     Inkscape::Util::Quantity::convert(x[Geom::X], "mm", "px"),
                        //     Inkscape::Util::Quantity::convert(x[Geom::X], "mm", "px")
                        // ));
                        //std::cout << tr << std::endl;
                        //tr = scale * tr;
                        //std::cout << tr << std::endl;
                        //tr = tr->i2doc_affine();
                        
                        //tr= tr * Geom::Scale(Inkscape::Util::Quantity::convert(x[Geom::X], "mm", "px"));
                    
                        //tr.setTranslation(Geom::Point(t[Geom::X], t[Geom::Y]));

                        //std::cout << tr << std::endl;

                        /*************************/

                        //gather_marker_items(marker_item, SHAPE_ROLE_NORMAL, shapes, tr, selection);
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
            auto si = std::make_unique<ShapeEditor>(this->desktop, r.edit_transform);
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

/* Toggles between two marker modes */
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

