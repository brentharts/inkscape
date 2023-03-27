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
#include "live_effects/effect.h"
#include "live_effects/lpe-fillet-chamfer.h"
#include "preferences.h"
#include "ui/dialog/lpe-fillet-chamfer-properties.h"
#include "ui/knot/knot-holder.h"
#include "ui/shape-editor.h"
#include "ui/tools/node-tool.h"

// TODO due to internal breakage in glibmm headers,
// this has to be included last.
#include <glibmm/i18n.h>

namespace Inkscape {

namespace LivePathEffect {

NodeSatelliteArrayParam::NodeSatelliteArrayParam(const Glib::ustring &label, const Glib::ustring &tip,
                                                 const Glib::ustring &key, Inkscape::UI::Widget::Registry *wr,
                                                 Effect *effect)
    : ArrayParam<std::vector<NodeSatellite>>(label, tip, key, wr, effect, 0)
    , _knoth(nullptr)
    , paramui(new Inkscape::UI::LivePathEffect::NodeSatelliteArrayParam(this))
{
    param_widget_is_visible(false);
}

void NodeSatelliteArrayParam::set_oncanvas_looks(Inkscape::CanvasItemCtrlShape shape, Inkscape::CanvasItemCtrlMode mode,
                                                 guint32 color)
{
    _knot_shape = shape;
    _knot_mode = mode;
    _knot_color = color;
}

void NodeSatelliteArrayParam::setPathVectorNodeSatellites(PathVectorNodeSatellites *pathVectorNodeSatellites,
                                                          bool write)
{
    _last_pathvector_nodesatellites = pathVectorNodeSatellites;
    if (write) {
        param_set_and_write_new_value(_last_pathvector_nodesatellites->getNodeSatellites());
    } else {
        param_setValue(_last_pathvector_nodesatellites->getNodeSatellites());
    }
}

void NodeSatelliteArrayParam::reloadKnots()
{
    paramui->reloadKnots();
}
void NodeSatelliteArrayParam::setUseDistance(bool use_knot_distance)
{
    _use_distance = use_knot_distance;
}

void NodeSatelliteArrayParam::setCurrentZoom(double current_zoom)
{
    _current_zoom = current_zoom;
}

void NodeSatelliteArrayParam::setGlobalKnotHide(bool global_knot_hide)
{
    _global_knot_hide = global_knot_hide;
}

void NodeSatelliteArrayParam::setEffectType(EffectType et)
{
    _effectType = et;
}

void NodeSatelliteArrayParam::updateCanvasIndicators(bool mirror)
{
    if (!_last_pathvector_nodesatellites) {
        return;
    }

    if (!_hp.empty()) {
        _hp.clear();
    }
    Geom::PathVector pathv = _last_pathvector_nodesatellites->getPathVector();
    if (pathv.empty()) {
        return;
    }
    if (mirror == true) {
        _hp.clear();
    }
    if (_effectType == FILLET_CHAMFER) {
        for (size_t i = 0; i < _vector.size(); ++i) {
            for (size_t j = 0; j < _vector[i].size(); ++j) {
                if (_vector[i][j].hidden ||                          // Ignore if hidden
                    (!_vector[i][j].has_mirror && mirror == true) || // Ignore if not have mirror and we are in mirror
                                                                     // loop
                    _vector[i][j].amount == 0 ||       // no helper in 0 value
                    j >= count_path_nodes(pathv[i]) || // ignore last nodesatellite in open paths with fillet chamfer
                                                       // effect
                    (!pathv[i].closed() && j == 0) || // ignore first nodesatellites on open paths
                    count_path_nodes(pathv[i]) == 2) {
                    continue;
                }
                Geom::Curve *curve_in = pathv[i][j].duplicate();
                double pos = 0;
                bool overflow = false;
                double size_out = _vector[i][j].arcDistance(*curve_in);
                double length_out = curve_in->length();
                gint previous_index =
                    j - 1; // Always are previous index because we skip first nodesatellite on open paths
                if (j == 0 && pathv[i].closed()) {
                    previous_index = count_path_nodes(pathv[i]) - 1;
                }
                if ( previous_index < 0 ) {
                    return;
                }
                double length_in = pathv.curveAt(previous_index).length();
                if (mirror) {
                    curve_in = const_cast<Geom::Curve *>(&pathv.curveAt(previous_index));
                    pos = _vector[i][j].time(size_out, true, *curve_in);
                    if (length_out < size_out) {
                        overflow = true;
                    }
                } else {
                    pos = _vector[i][j].time(*curve_in);
                    if (length_in < size_out) {
                        overflow = true;
                    }
                }
                if (pos <= 0 || pos >= 1) {
                    continue;
                }
            }
        }
    }
    if (mirror) {
        updateCanvasIndicators(false);
    }
}
void NodeSatelliteArrayParam::updateCanvasIndicators()
{
    updateCanvasIndicators(true);
}

void NodeSatelliteArrayParam::addCanvasIndicators(SPLPEItem const * /*lpeitem*/, std::vector<Geom::PathVector> &hp_vec)
{
    hp_vec.push_back(_hp);
}

void NodeSatelliteArrayParam::param_transform_multiply(Geom::Affine const &postmul, bool /*set*/)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    if (prefs->getBool("/options/transform/rectcorners", true)) {
        for (auto & i : _vector) {
            for (auto & j : i) {
                if (!j.is_time && j.amount > 0) {
                    j.amount = j.amount * ((postmul.expansionX() + postmul.expansionY()) / 2);
                }
            }
        }
        param_set_and_write_new_value(_vector);
    }
}

void NodeSatelliteArrayParam::updateAmmount(double amount)
{
    Geom::PathVector const pathv = _last_pathvector_nodesatellites->getPathVector();
    NodeSatellites nodesatellites = _last_pathvector_nodesatellites->getNodeSatellites();
    for (size_t i = 0; i < nodesatellites.size(); ++i) {
        for (size_t j = 0; j < nodesatellites[i].size(); ++j) {
            Geom::Curve const &curve_in = pathv[i][j];
            if (param_effect->isNodePointSelected(curve_in.initialPoint()) ){
                _vector[i][j].amount = amount;
                _vector[i][j].setSelected(true);
            } else {
                _vector[i][j].setSelected(false);
            }
        }
    }
}

void NodeSatelliteArrayParam::addKnotHolderEntities(KnotHolder *knotholder, SPItem *item)
{
    paramui->addKnotHolderEntities(knotholder, item);
}
} /* namespace LivePathEffect */

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
