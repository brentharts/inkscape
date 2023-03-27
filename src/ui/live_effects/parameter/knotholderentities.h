// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_FILLETCHANFER_KNOTHOLDER_ENTITY_UI_H
#define INKSCAPE_LIVEPATHEFFECT_FILLETCHANFER_KNOTHOLDER_ENTITY_UI_H

/*
 * Here we pot all parameters knotholders of UI, split into files if grow too much
 * This parameter acts as a bridge from pathVectorNodeSatellites class to serialize it as a LPE
 * parameter
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib.h>

#include "helper/geom-pathvector_nodesatellites.h"
//#include "live_effects/parameter/parameter.h"
//#include "live_effects/parameter/nodesatellitesarray.h"
#include "ui/live_effects/parameter/parameter_ui.h"
#include "ui/live_effects/parameter/nodesatellitesarray.h"
#include "ui/knot/knot-holder-entity.h"

namespace Inkscape {
namespace LivePathEffect {
    class NodeSatelliteArrayParam;
}
namespace UI {
namespace LivePathEffect {

class FilletChamferKnotHolderEntity : public KnotHolderEntity {
public:
    FilletChamferKnotHolderEntity(Inkscape::LivePathEffect::NodeSatelliteArrayParam *p, size_t index);
    ~FilletChamferKnotHolderEntity() override
    {
        _pparam->paramui->_knoth = nullptr;
    }
    void knot_set(Geom::Point const &p, Geom::Point const &origin,
                          guint state) override;
    Geom::Point knot_get() const override;
    void knot_click(guint state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override;
    void knot_set_offset(NodeSatellite);
    /** Checks whether the index falls within the size of the parameter's vector
     */
    bool valid_index(size_t index, size_t subindex) const
    {
        return (_pparam->_vector.size() > index && _pparam->_vector[index].size() > subindex);
    };

private:
    Inkscape::LivePathEffect::NodeSatelliteArrayParam *_pparam;
    size_t _index;
};

} //namespace LivePathEffect
} //namespace UI
} //namespace Inkscape

#endif
