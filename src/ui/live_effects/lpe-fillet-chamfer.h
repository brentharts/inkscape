// SPDX-License-Identifier: GPL-2.0-or-later

/** @file
 * @brief fillet chamfer UI
 */
/* Author:
 *   Javier Arraiza <jabier.arraiza@marker.es> (code)
 *   Alvaro Maldonado<alvaropaplona@gmail.com> (coments)
 *
 * Copyright (C) 2023 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#ifndef INKSCAPE_LPE_FILLET_CHAMFER_UI_H
#define INKSCAPE_LPE_FILLET_CHAMFER_UI_H

#include "live_effects/effect.h"
#include "live_effects/lpe-fillet-chamfer.h"
#include "live_effects/parameter/parameter.h"
#include "ui/live_effects/effect_ui.h"
#include "ui/live_effects/parameter/parameter_ui.h"
#include "ui/live_effects/parameter/nodesatellitesarray.h"
#include <gtkmm/spinbutton.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/expander.h>

namespace Inkscape {
    namespace LivePathEffect {
        class Effect;
        class Parameter;
        class LPEFilletChamfer;
        class FilletChamferKnotHolderEntity;
        class NodeSatelliteArrayParam;
    }
namespace UI {
namespace LivePathEffect {

using namespace Inkscape::LivePathEffect;

class LPEFilletChamfer : public EffectUI {
public:
    /* virtual ~LPEFilletChamfer(){};
    LPEFilletChamfer(const LPEFilletChamfer&) = delete;
    LPEFilletChamfer& operator=(const LPEFilletChamfer&) = delete; */
    LPEFilletChamfer(Inkscape::LivePathEffect::LPEFilletChamfer *_lpe_filletchamfer);
    Gtk::Widget *newWidget();
private:
    Glib::RefPtr<Gtk::Builder> _builder;
    Gtk::SpinButton   * _radius = nullptr;
    Gtk::ComboBox     * _unit = nullptr;
    Gtk::ToggleButton * _auto = nullptr;
    Gtk::ToggleButton * _arc = nullptr;
    Gtk::ToggleButton * _bezier = nullptr;
    Gtk::ToggleButton * _fillet = nullptr;
    Gtk::ToggleButton * _chamfer = nullptr;
    Gtk::ToggleButton * _filletinverse = nullptr;
    Gtk::ToggleButton * _chamferstepsinverse = nullptr;
    Gtk::ToggleButton * _chamfersteps = nullptr;
    Gtk::ToggleButton * _controldistance = nullptr;
    Gtk::ToggleButton * _radiusdistance = nullptr;
    Gtk::CheckButton  * _nonchanged = nullptr;
    Gtk::CheckButton  * _changed = nullptr;
    Gtk::SpinButton   * _steps = nullptr;
    Gtk::Expander     * _avancedexpander = nullptr;
    void reload_method(LPEFilletChamfer *lpe);
    void reload_mode(LPEFilletChamfer *lpe);
    void reloadUI();
    void update_radius();
    bool _updating = false;
    bool _avancedopen = false;
    Inkscape::LivePathEffect::LPEFilletChamfer *lpe_filletchamfer;
};

} // namespace LivePathEffect
} // namespace UI
} // namespace Inkscape

#endif

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