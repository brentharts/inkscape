// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_NODESATELLITES_ARRAY_UI_H
#define INKSCAPE_LIVEPATHEFFECT_NODESATELLITES_ARRAY_UI_H

/*
 * Inkscape::LivePathEffectParameters
 * Copyright (C) Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 * Special thanks to Johan Engelen for the base of the effect -powerstroke-
 * Also to ScislaC for pointing me to the idea
 * Also su_v for his constructive feedback and time
 * To Nathan Hurst for his review and help on refactor
 * and finally to Liam P. White for his big help on coding,
 * that saved me a lot of hours
 *
 *
 * This parameter acts as a bridge from pathVectorNodeSatellites class to serialize it as a LPE
 * parameter
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib.h>

#include "helper/geom-pathvector_nodesatellites.h"
#include "live_effects/effect.h"
#include "live_effects/parameter/parameter.h"
#include "live_effects/parameter/nodesatellitesarray.h"
#include "ui/live_effects/parameter/parameter_ui.h"
#include "ui/knot/knot-holder-entity.h"
namespace Inkscape {
namespace LivePathEffect {
    class Effect;
    class Parameter;
    class NodeSatelliteArrayParam;
    class LPEFilletChamfer;
}
namespace UI {
namespace LivePathEffect {

class FilletChamferKnotHolderEntity;

class NodeSatelliteArrayParam : public ParameterUI
{
public:
    /* virtual ~NodeSatelliteArrayParam(){};
    NodeSatelliteArrayParam(const NodeSatelliteArrayParam&) = delete;
    NodeSatelliteArrayParam& operator=(const NodeSatelliteArrayParam&) = delete; */
    NodeSatelliteArrayParam(Inkscape::LivePathEffect::NodeSatelliteArrayParam *_lpe_filletchamfer);
    void addKnotHolderEntities(KnotHolder *knotholder, SPItem *item);
    void addKnotHolderEntities(KnotHolder *knotholder, SPItem *item, bool mirror);
    void setCurrentZoom(double current_zoom);
    void setGlobalKnotHide(bool global_knot_hide);
    void reloadKnots();
    void set_oncanvas_looks(Inkscape::CanvasItemCtrlShape shape,
                            Inkscape::CanvasItemCtrlMode mode,
                            guint32 color);

    KnotHolder *_knoth;

    friend class FilletChamferKnotHolderEntity;
    friend class Inkscape::LivePathEffect::NodeSatelliteArrayParam;
protected:
    Inkscape::LivePathEffect::NodeSatelliteArrayParam *lpeparam_nodesatellites;
private:
    NodeSatelliteArrayParam(const NodeSatelliteArrayParam &) = delete;
    NodeSatelliteArrayParam &operator=(const NodeSatelliteArrayParam &) = delete;

    Inkscape::CanvasItemCtrlShape _knot_shape = Inkscape::CANVAS_ITEM_CTRL_SHAPE_DIAMOND;
    Inkscape::CanvasItemCtrlMode  _knot_mode = Inkscape::CANVAS_ITEM_CTRL_MODE_XOR;
    guint32 _knot_color = 0xaaff8800;
    Geom::PathVector _hp;
    bool _global_knot_hide = false;
    double _current_zoom = 0;
};

} //namespace LivePathEffect
} //namespace UI
} //namespace Inkscape

#endif
