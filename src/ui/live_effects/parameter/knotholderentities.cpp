// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author(s):
 *   Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "knotholderentities.h"

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

FilletChamferKnotHolderEntity::FilletChamferKnotHolderEntity(Inkscape::LivePathEffect::NodeSatelliteArrayParam *p, size_t index)
    : _pparam(p)
    , _index(index)
{}

void FilletChamferKnotHolderEntity::knot_set(Geom::Point const &p,
        Geom::Point const &/*origin*/,
        guint state)
{
    if (!_pparam->_last_pathvector_nodesatellites) {
        return;
    }
    size_t total_nodesatellites = _pparam->_last_pathvector_nodesatellites->getTotalNodeSatellites();
    bool is_mirror = false;
    size_t index = _index;
    if (_index >= total_nodesatellites) {
        index = _index - total_nodesatellites;
        is_mirror = true;
    }
    std::pair<size_t, size_t> index_data = _pparam->_last_pathvector_nodesatellites->getIndexData(index);
    size_t satelite_index = index_data.first;
    size_t subsatelite_index = index_data.second;
    
    Geom::Point s = snap_knot_position(p, state);
    if (!valid_index(satelite_index, subsatelite_index)) {
        return;
    }
    NodeSatellite nodesatellite = _pparam->_vector[satelite_index][subsatelite_index];
    Geom::PathVector pathv = _pparam->_last_pathvector_nodesatellites->getPathVector();
    if (nodesatellite.hidden ||
        (!pathv[satelite_index].closed() &&
         (subsatelite_index == 0 || // ignore first nodesatellites on open paths
          count_path_nodes(pathv[satelite_index]) - 1 == subsatelite_index))) // ignore last nodesatellite in open paths
                                                                              // with fillet chamfer effect
    {
        return;
    }
    gint previous_index = subsatelite_index - 1;
    if (subsatelite_index == 0 && pathv[satelite_index].closed()) {
        previous_index = count_path_nodes(pathv[satelite_index]) - 1;
    }
    if ( previous_index < 0 ) {
        return;
    }
    Geom::Curve const &curve_in = pathv[satelite_index][previous_index];
    double mirror_time = Geom::nearest_time(s, curve_in);
    Geom::Point mirror = curve_in.pointAt(mirror_time);
    double normal_time = Geom::nearest_time(s, pathv[satelite_index][subsatelite_index]);
    Geom::Point normal = pathv[satelite_index][subsatelite_index].pointAt(normal_time);
    double distance_mirror = Geom::distance(mirror,s);
    double distance_normal = Geom::distance(normal,s);
    if (Geom::are_near(s, pathv[satelite_index][subsatelite_index].initialPoint(), 1.5 / _pparam->_current_zoom)) {
        nodesatellite.amount = 0;
    } else if (distance_mirror < distance_normal) {
        double time_start = 0;
        NodeSatellites nodesatellites = _pparam->_last_pathvector_nodesatellites->getNodeSatellites();
        time_start = nodesatellites[satelite_index][previous_index].time(curve_in);
        if (time_start > mirror_time) {
            mirror_time = time_start;
        }
        double size = arcLengthAt(mirror_time, curve_in);
        double amount = curve_in.length() - size;
        if (nodesatellite.is_time) {
            amount = timeAtArcLength(amount, pathv[satelite_index][subsatelite_index]);
        }
        nodesatellite.amount = amount;
    } else {
        nodesatellite.setPosition(s, pathv[satelite_index][subsatelite_index]);
    }
    auto filletchamfer = dynamic_cast<Inkscape::LivePathEffect::LPEFilletChamfer *>(_pparam->param_effect);
    filletchamfer->helperpath = true;
    _pparam->updateAmmount(nodesatellite.amount);
    _pparam->_vector[satelite_index][subsatelite_index] = nodesatellite;
    sp_lpe_item_update_patheffect(cast<SPLPEItem>(item), false, false);
}

void
FilletChamferKnotHolderEntity::knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state)
{
    auto filletchamfer = dynamic_cast<Inkscape::LivePathEffect::LPEFilletChamfer *>(_pparam->param_effect);
    if (filletchamfer) {
        filletchamfer->refresh_widgets = true;
        filletchamfer->helperpath = false;
        filletchamfer->makeUndoDone(_("Move handle"));
    }
}

Geom::Point FilletChamferKnotHolderEntity::knot_get() const
{
    if (!_pparam->_last_pathvector_nodesatellites || _pparam->_global_knot_hide) {
        return Geom::Point(Geom::infinity(), Geom::infinity());
    }
    Geom::Point tmp_point;
    size_t total_nodesatellites = _pparam->_last_pathvector_nodesatellites->getTotalNodeSatellites();
    bool is_mirror = false;
    size_t index = _index;
    if (_index >= total_nodesatellites) {
        index = _index - total_nodesatellites;
        is_mirror = true;
    }
    std::pair<size_t, size_t> index_data = _pparam->_last_pathvector_nodesatellites->getIndexData(index);
    size_t satelite_index = index_data.first;
    size_t subsatelite_index = index_data.second;
    if (!valid_index(satelite_index, subsatelite_index)) {
        return Geom::Point(Geom::infinity(), Geom::infinity());
    }
    NodeSatellite nodesatellite = _pparam->_vector[satelite_index][subsatelite_index];
    Geom::PathVector pathv = _pparam->_last_pathvector_nodesatellites->getPathVector();
    if (nodesatellite.hidden ||
        (!pathv[satelite_index].closed() &&
         (subsatelite_index == 0 || subsatelite_index == count_path_nodes(pathv[satelite_index]) -
                                                             1))) // ignore first and last nodesatellites on open paths
    {
        return Geom::Point(Geom::infinity(), Geom::infinity());
    }
    this->knot->show();
    if (is_mirror) {
        gint previous_index = subsatelite_index - 1;
        if (subsatelite_index == 0 && pathv[satelite_index].closed()) {
            previous_index = count_path_nodes(pathv[satelite_index]) - 1;
        }
        if ( previous_index < 0 ) {
            return Geom::Point(Geom::infinity(), Geom::infinity());
        }
        Geom::Curve const &curve_in = pathv[satelite_index][previous_index];
        double s = nodesatellite.arcDistance(pathv[satelite_index][subsatelite_index]);
        double t = nodesatellite.time(s, true, curve_in);
        if (t > 1) {
            t = 1;
        }
        if (t < 0) {
            t = 0;
        }
        double time_start = 0;
        time_start = _pparam->_last_pathvector_nodesatellites->getNodeSatellites()[satelite_index][previous_index].time(
            curve_in);
        if (time_start > t) {
            t = time_start;
        }
        tmp_point = (curve_in).pointAt(t);
    } else {
        tmp_point = nodesatellite.getPosition(pathv[satelite_index][subsatelite_index]);
    }
    Geom::Point const canvas_point = tmp_point;
    return canvas_point;
}

void FilletChamferKnotHolderEntity::knot_click(guint state)
{
    if (!_pparam->_last_pathvector_nodesatellites) {
        return;
    }
    size_t total_nodesatellites = _pparam->_last_pathvector_nodesatellites->getTotalNodeSatellites();
    bool is_mirror = false;
    size_t index = _index;
    if (_index >= total_nodesatellites) {
        index = _index - total_nodesatellites;
        is_mirror = true;
    }
    std::pair<size_t, size_t> index_data = _pparam->_last_pathvector_nodesatellites->getIndexData(index);
    size_t satelite_index = index_data.first;
    size_t subsatelite_index = index_data.second;
    if (!valid_index(satelite_index, subsatelite_index)) {
        return;
    }
    Geom::PathVector pathv = _pparam->_last_pathvector_nodesatellites->getPathVector();
    if (!pathv[satelite_index].closed() &&
        (subsatelite_index == 0 ||
         count_path_nodes(pathv[satelite_index]) - 1 == subsatelite_index)) // ignore last nodesatellite in open paths
                                                                            // with fillet chamfer effect
    {
        return;
    }
    if (state & GDK_CONTROL_MASK) {
        if (state & GDK_MOD1_MASK) {
            _pparam->_vector[satelite_index][subsatelite_index].amount = 0.0;
            sp_lpe_item_update_patheffect(cast<SPLPEItem>(item), false, false);
        } else {
            using namespace Geom;
            NodeSatelliteType type = _pparam->_vector[satelite_index][subsatelite_index].nodesatellite_type;
            switch (type) {
            case FILLET:
                type = INVERSE_FILLET;
                break;
            case INVERSE_FILLET:
                type = CHAMFER;
                break;
            case CHAMFER:
                type = INVERSE_CHAMFER;
                break;
            default:
                type = FILLET;
                break;
            }
            _pparam->_vector[satelite_index][subsatelite_index].nodesatellite_type = type;
            sp_lpe_item_update_patheffect(cast<SPLPEItem>(item), false, false);
            const gchar *tip;
            if (type == CHAMFER) {
                tip = _("<b>Chamfer</b>: <b>Ctrl+Click</b> toggles type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> resets");
            } else if (type == INVERSE_CHAMFER) {
                tip = _("<b>Inverse Chamfer</b>: <b>Ctrl+Click</b> toggles type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> resets");
            } else if (type == INVERSE_FILLET) {
                tip = _("<b>Inverse Fillet</b>: <b>Ctrl+Click</b> toggles type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> resets");
            } else {
                tip = _("<b>Fillet</b>: <b>Ctrl+Click</b> toggles type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> resets");
            }
            this->knot->tip = g_strdup(tip);
            this->knot->show();
        }
    } else if (state & GDK_SHIFT_MASK) {
        double amount = _pparam->_vector[satelite_index][subsatelite_index].amount;
        gint previous_index = subsatelite_index - 1;
        if (subsatelite_index == 0 && pathv[satelite_index].closed()) {
            previous_index = count_path_nodes(pathv[satelite_index]) - 1;
        }
        if ( previous_index < 0 ) {
            return;
        }
        if (!_pparam->_use_distance && !_pparam->_vector[satelite_index][subsatelite_index].is_time) {
            amount = _pparam->_vector[satelite_index][subsatelite_index].lenToRad(
                amount, pathv[satelite_index][previous_index], pathv[satelite_index][subsatelite_index],
                _pparam->_vector[satelite_index][previous_index]);
        }
        bool aprox = false;
        Geom::D2<Geom::SBasis> d2_out = pathv[satelite_index][subsatelite_index].toSBasis();
        Geom::D2<Geom::SBasis> d2_in = pathv[satelite_index][previous_index].toSBasis();
        aprox = ((d2_in)[0].degreesOfFreedom() != 2 ||
                 d2_out[0].degreesOfFreedom() != 2) &&
                !_pparam->_use_distance
                ? true
                : false;
        Inkscape::UI::Dialogs::FilletChamferPropertiesDialog::showDialog(
            this->desktop, amount, this, _pparam->_use_distance, aprox,
            _pparam->_vector[satelite_index][subsatelite_index]);
    }
}

void FilletChamferKnotHolderEntity::knot_set_offset(NodeSatellite nodesatellite)
{
    if (!_pparam->_last_pathvector_nodesatellites) {
        return;
    }
    size_t total_nodesatellites = _pparam->_last_pathvector_nodesatellites->getTotalNodeSatellites();
    bool is_mirror = false;
    size_t index = _index;
    if (_index >= total_nodesatellites) {
        index = _index - total_nodesatellites;
        is_mirror = true;
    }
    std::pair<size_t, size_t> index_data = _pparam->_last_pathvector_nodesatellites->getIndexData(index);
    size_t satelite_index = index_data.first;
    size_t subsatelite_index = index_data.second;
    if (!valid_index(satelite_index, subsatelite_index)) {
        return;
    }
    Geom::PathVector pathv = _pparam->_last_pathvector_nodesatellites->getPathVector();
    if (nodesatellite.hidden ||
        (!pathv[satelite_index].closed() &&
         (subsatelite_index == 0 ||
          count_path_nodes(pathv[satelite_index]) - 1 == subsatelite_index))) // ignore last nodesatellite in open paths
                                                                              // with fillet chamfer effect
    {
        return;
    }
    double amount = nodesatellite.amount;
    double max_amount = amount;
    if (!_pparam->_use_distance && !nodesatellite.is_time) {
        gint previous_index = subsatelite_index - 1;
        if (subsatelite_index == 0 && pathv[satelite_index].closed()) {
            previous_index = count_path_nodes(pathv[satelite_index]) - 1;
        }
        if ( previous_index < 0 ) {
            return;
        }
        amount = _pparam->_vector[satelite_index][subsatelite_index].radToLen(
            amount, pathv[satelite_index][previous_index], pathv[satelite_index][subsatelite_index]);
        if (max_amount > 0 && amount == 0) {
            amount = _pparam->_vector[satelite_index][subsatelite_index].amount;
        }
    }
    nodesatellite.amount = amount;
    _pparam->_vector[satelite_index][subsatelite_index] = nodesatellite;
    this->parent_holder->knot_ungrabbed_handler(this->knot, 0);
    _pparam->write_to_SVG();
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
