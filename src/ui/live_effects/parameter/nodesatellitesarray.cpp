// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author(s):
 *   Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "nodesatellitesarray.h"

#include "helper/geom.h"
#include "inkscape.h"
#include "preferences.h"
#include "live_effects/lpe-fillet-chamfer.h"
#include "ui/dialog/lpe-fillet-chamfer-properties.h"
#include "ui/knot/knot-holder.h"
#include "ui/shape-editor.h"
#include "ui/tools/node-tool.h"

// TODO due to internal breakage in glibmm headers,
// this has to be included last.
#include <glibmm/i18n.h>

namespace Inkscape {
namespace UI {
namespace LivePathEffect {

NodeSatelliteArrayParam::NodeSatelliteArrayParam(Inkscape::LivePathEffect::NodeSatelliteArrayParam *_lpeparam_nodesatellites)
    : ParameterUI(dynamic_cast<Inkscape::LivePathEffect::Parameter *>(_lpeparam_nodesatellites)),
    lpeparam_nodesatellites(_lpeparam_nodesatellites)   
{   
};

void NodeSatelliteArrayParam::set_oncanvas_looks(Inkscape::CanvasItemCtrlShape shape, Inkscape::CanvasItemCtrlMode mode,
                                                 guint32 color)
{
    _knot_shape = shape;
    _knot_mode = mode;
    _knot_color = color;
}

void NodeSatelliteArrayParam::reloadKnots()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop) {
        Inkscape::UI::Tools::NodeTool *nt = dynamic_cast<Inkscape::UI::Tools::NodeTool *>(desktop->event_context);
        if (nt) {
            for (auto &_shape_editor : nt->_shape_editors) {
                Inkscape::UI::ShapeEditor *shape_editor = _shape_editor.second.get();
                if (shape_editor && shape_editor->lpeknotholder) {
                    SPItem *item = shape_editor->knotholder->item;
                    shape_editor->unset_item(true);
                    shape_editor->set_item(item);
                }
            }
        }
    }
}

void NodeSatelliteArrayParam::setCurrentZoom(double current_zoom)
{
    _current_zoom = current_zoom;
}

void NodeSatelliteArrayParam::setGlobalKnotHide(bool global_knot_hide)
{
    _global_knot_hide = global_knot_hide;
}

void NodeSatelliteArrayParam::addKnotHolderEntities(KnotHolder *knotholder, SPItem *item, bool mirror)
{
    auto last_pathvector_nodesatellites = lpeparam_nodesatellites->_last_pathvector_nodesatellites;
    if (!last_pathvector_nodesatellites) {
        return;
    }
    size_t index = 0;
    auto vector = lpeparam_nodesatellites->_vector;
    for (size_t i = 0; i < vector.size(); ++i) {
        for (size_t j = 0; j < vector[i].size(); ++j) {
            if (!vector[i][j].has_mirror && mirror) {
                continue;
            }
            NodeSatelliteType type = vector[i][j].nodesatellite_type;
            if (mirror && i == 0 && j == 0) {
                index += last_pathvector_nodesatellites->getTotalNodeSatellites();
            }
            using namespace Geom;
            //If is for filletChamfer effect...
            if (lpeparam_nodesatellites->_effectType == FILLET_CHAMFER) {
                const gchar *tip;
                auto typefc = type;
                if (typefc == CHAMFER) {
                    tip = _("<b>Chamfer</b>: <b>Ctrl+Click</b> toggles type, "
                            "<b>Shift+Click</b> open dialog, "
                            "<b>Ctrl+Alt+Click</b> reset");
                } else if (typefc == INVERSE_CHAMFER) {
                    tip = _("<b>Inverse Chamfer</b>: <b>Ctrl+Click</b> toggles type, "
                            "<b>Shift+Click</b> open dialog, "
                            "<b>Ctrl+Alt+Click</b> reset");
                } else if (typefc == INVERSE_FILLET) {
                    tip = _("<b>Inverse Fillet</b>: <b>Ctrl+Click</b> toggles type, "
                            "<b>Shift+Click</b> open dialog, "
                            "<b>Ctrl+Alt+Click</b> reset");
                } else {
                    tip = _("<b>Fillet</b>: <b>Ctrl+Click</b> toggles type, "
                            "<b>Shift+Click</b> open dialog, "
                            "<b>Ctrl+Alt+Click</b> reset");
                }
                FilletChamferKnotHolderEntity *e = new FilletChamferKnotHolderEntity(this->lpeparam_nodesatellites, index);
                e->create(nullptr, item, knotholder, Inkscape::CANVAS_ITEM_CTRL_TYPE_LPE, "LPE:Chamfer",
                          _(tip), _knot_color);
                knotholder->add(e);
            }
            index++;
        }
    }
    if (mirror) {
        addKnotHolderEntities(knotholder, item, false);
    }
}

void NodeSatelliteArrayParam::addKnotHolderEntities(KnotHolder *knotholder, SPItem *item)
{
    _knoth = knotholder;
    addKnotHolderEntities(knotholder, item, true);
}

} /* namespace LivePathEffect */
} /* namespace UI */
} /* namespace Inkscape */

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
