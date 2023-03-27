// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_NODESATELLITES_ARRAY_H
#define INKSCAPE_LIVEPATHEFFECT_NODESATELLITES_ARRAY_H

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
#include "live_effects/effect-enum.h"
#include "live_effects/parameter/array.h"
#include "ui/live_effects/parameter/parameter_ui.h"
#include "ui/live_effects/effect_ui.h"
#include "ui/live_effects/parameter/nodesatellitesarray.h"
#include "ui/knot/knot-holder-entity.h"

namespace Inkscape {
    namespace UI {
        namespace LivePathEffect {
            class FilletChamferKnotHolderEntity;
            class NodeSatelliteArrayParam;
        }
    }

namespace LivePathEffect {


class NodeSatelliteArrayParam : public ArrayParam<std::vector<NodeSatellite>>
{
public:
    NodeSatelliteArrayParam(const Glib::ustring &label, const Glib::ustring &tip, const Glib::ustring &key,
                            Inkscape::UI::Widget::Registry *wr, Effect *effect);

    Gtk::Widget *param_newWidget() override
    {
        return nullptr;
    }
    void addKnotHolderEntities(KnotHolder *knotholder, SPItem *item) override;
    void addCanvasIndicators(SPLPEItem const *lpeitem, std::vector<Geom::PathVector> &hp_vec) override;
    virtual void updateCanvasIndicators();
    virtual void updateCanvasIndicators(bool mirror);
    bool providesKnotHolderEntities() const override
    {
        return true;
    }
    void param_transform_multiply(Geom::Affine const &postmul, bool /*set*/) override;
    void setUseDistance(bool use_knot_distance);
    void setCurrentZoom(double current_zoom);
    void setGlobalKnotHide(bool global_knot_hide);
    void setEffectType(EffectType et);
    void reloadKnots();
    void updateAmmount(double amount);
    void setPathVectorNodeSatellites(PathVectorNodeSatellites *pathVectorNodeSatellites, bool write = true);
    // I think is better move this function on finish LPE migration to UI
    void set_oncanvas_looks(Inkscape::CanvasItemCtrlShape shape,
                            Inkscape::CanvasItemCtrlMode mode,
                            guint32 color);


    friend class Inkscape::UI::LivePathEffect::FilletChamferKnotHolderEntity;
    friend class Inkscape::UI::LivePathEffect::NodeSatelliteArrayParam;
    friend class Inkscape::UI::LivePathEffect::LPEFilletChamfer;
    friend class LPEFilletChamfer;
    ParamType paramType() const override { return ParamType::NODE_SATELLITE_ARRAY; };
    Inkscape::UI::LivePathEffect::NodeSatelliteArrayParam *paramui;
protected:
    KnotHolder *_knoth;
    bool _use_distance = false;
    bool _global_knot_hide = false;
    PathVectorNodeSatellites *_last_pathvector_nodesatellites = nullptr;

private:
    NodeSatelliteArrayParam(const NodeSatelliteArrayParam &) = delete;
    NodeSatelliteArrayParam &operator=(const NodeSatelliteArrayParam &) = delete;

    Inkscape::CanvasItemCtrlShape _knot_shape = Inkscape::CANVAS_ITEM_CTRL_SHAPE_DIAMOND;
    Inkscape::CanvasItemCtrlMode  _knot_mode = Inkscape::CANVAS_ITEM_CTRL_MODE_XOR;
    guint32 _knot_color = 0xaaff8800;
    Geom::PathVector _hp;
    double _current_zoom = 0;
    EffectType _effectType = FILLET_CHAMFER;

};

} //namespace LivePathEffect

} //namespace Inkscape

#endif
