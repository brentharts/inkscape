// SPDX-License-Identifier: GPL-2.0-or-later
#include "ui/live_effects/lpe-fillet-chamfer.h"
#include "live_effects/effect.h"
#include "ui/builder-utils.h"
#include "util/units.h"
#include "ui/icon-names.h"

namespace Inkscape {
    namespace LivePathEffect {
        enum Filletmethod;
        class Effect;
        class LPEFilletChamfer;
        class NodeSatelliteArrayParam;
    }
namespace UI {
namespace LivePathEffect {

using namespace Inkscape::LivePathEffect;

LPEFilletChamfer::LPEFilletChamfer(Inkscape::LivePathEffect::LPEFilletChamfer *_lpe_filletchamfer)
    : EffectUI(dynamic_cast<Inkscape::LivePathEffect::Effect *>(_lpe_filletchamfer)),
    lpe_filletchamfer(_lpe_filletchamfer)   
{   
};

void LPEFilletChamfer::update_radius() {
    lpe_filletchamfer->radius.param_set_value(_radius->get_value());
    lpe_filletchamfer->radius.write_to_SVG();
    lpe_filletchamfer->updateAmount();
}

void LPEFilletChamfer::reload_method(LPEFilletChamfer *lpe) {
    switch (lpe->lpe_filletchamfer->method) {
        case Inkscape::LivePathEffect::Filletmethod::FM_AUTO: {
            lpe->_auto->set_active(true); 
            lpe->_arc->set_active(false);
            lpe->_bezier->set_active(false);
            break;
        }
        case Inkscape::LivePathEffect::Filletmethod::FM_ARC: {
            lpe->_auto->set_active(false); 
            lpe->_arc->set_active(true);
            lpe->_bezier->set_active(false);
            break;
        }
        case Inkscape::LivePathEffect::Filletmethod::FM_BEZIER: {
            lpe->_auto->set_active(false); 
            lpe->_arc->set_active(false);
            lpe->_bezier->set_active(true);
            break;
        }
        default: {
            lpe->_auto->set_active(true); 
            lpe->_arc->set_active(false);
            lpe->_bezier->set_active(false);
            break;
        }
    }
}

void LPEFilletChamfer::reload_mode(LPEFilletChamfer *lpe) {
    //TODO move mode to enum anf remove this ifs
    Glib::ustring mode = lpe->lpe_filletchamfer->mode.param_getSVGValue();
    _steps->set_value(lpe->lpe_filletchamfer->chamfer_steps);
    if (mode == "F") {
        lpe->_fillet->set_active(true); 
        lpe->_filletinverse->set_active(false);
        lpe->_chamfer->set_active(false);
        lpe->_chamfersteps->set_active(false);
        lpe->_chamferstepsinverse->set_active(false);
    } else if ( mode == "IF" ) {
        lpe->_fillet->set_active(false); 
        lpe->_filletinverse->set_active(true);
        lpe->_chamfer->set_active(false);
        lpe->_chamfersteps->set_active(false);
        lpe->_chamferstepsinverse->set_active(false);
    } else if ( mode == "C" ) {
        lpe->_fillet->set_active(false); 
        lpe->_filletinverse->set_active(false);
        lpe->_chamfer->set_active(false);
        lpe->_chamferstepsinverse->set_active(false);
        if (lpe->_steps->get_value() > 1) {
            lpe->_chamfersteps->set_active(true);
            lpe->_chamfer->set_active(false);
        } else {
            lpe->_chamfersteps->set_active(false);
            lpe->_chamfer->set_active(true);
        }
    } else if ( mode == "IC" ) {
        lpe->_fillet->set_active(false); 
        lpe->_filletinverse->set_active(false);
        lpe->_chamfer->set_active(false);
        lpe->_chamfersteps->set_active(false);
        lpe->_chamferstepsinverse->set_active(true);
    }
}

void LPEFilletChamfer::reloadUI() {
    _radius = &get_widget<Gtk::SpinButton>(_builder, "radius");
    _nonchanged = &get_widget<Gtk::CheckButton>(_builder, "nonchanged");
    _changed = &get_widget<Gtk::CheckButton>(_builder, "changed");
    _unit = &get_widget<Gtk::ComboBox>(_builder, "unit");
    _auto   = &get_widget<Gtk::ToggleButton>(_builder, "auto");
    _bezier = &get_widget<Gtk::ToggleButton>(_builder, "bezier");
    _arc    = &get_widget<Gtk::ToggleButton>(_builder, "arc");
    _fillet              = &get_widget<Gtk::ToggleButton>(_builder, "fillet");
    _filletinverse       = &get_widget<Gtk::ToggleButton>(_builder, "filletinverse");
    _chamfer             = &get_widget<Gtk::ToggleButton>(_builder, "chamfer");
    _chamferstepsinverse = &get_widget<Gtk::ToggleButton>(_builder, "chamferstepsinverse");
    _chamfersteps        = &get_widget<Gtk::ToggleButton>(_builder, "chamfersteps");
    _steps               = &get_widget<Gtk::SpinButton>(_builder, "steps");
    _avancedexpander     = &get_widget<Gtk::Expander>(_builder, "avancedexpander");
    _radiusdistance      = &get_widget<Gtk::ToggleButton>(_builder, "radiusdistance");
    _controldistance     = &get_widget<Gtk::ToggleButton>(_builder, "controldistance");
    _radius->set_value(lpe_filletchamfer->radius);
    _radius->set_tooltip_text(*lpe_filletchamfer->radius.param_getTooltip());
    _radius->signal_value_changed().connect([=](){
        lpe_filletchamfer->only_selected.param_setValue(lpe_filletchamfer->selectedNodesPoints.size() > 0);
        lpe_filletchamfer->only_selected.write_to_SVG();
        update_radius();
        DocumentUndo::done(lpe_filletchamfer->getSPDoc(), _("Radius changed on LPE corners"), INKSCAPE_ICON("lpe-fillet-chamfer")); // TODO Fix description.
    });
    _nonchanged->set_active(lpe_filletchamfer->apply_no_radius);
    _nonchanged->set_tooltip_text(*lpe_filletchamfer->apply_no_radius.param_getTooltip());
    _nonchanged->signal_toggled().connect([=]() {
        if (!_updating) {
            _updating = true;
            lpe_filletchamfer->apply_no_radius.param_setValue(_nonchanged->get_active());
            lpe_filletchamfer->writeParamsToSVG();
            DocumentUndo::done(lpe_filletchamfer->getSPDoc(), _("Apply to no changed nodes"), INKSCAPE_ICON("lpe-fillet-chamfer")); // TODO Fix description.
            _updating = false;
        }
    });
    _changed->set_active(lpe_filletchamfer->apply_with_radius);
    _changed->set_tooltip_text(*lpe_filletchamfer->apply_with_radius.param_getTooltip());
    _changed->signal_toggled().connect([=]() {
        if (!_updating) {
            _updating = true;
            lpe_filletchamfer->apply_with_radius.param_setValue(_changed->get_active());
            lpe_filletchamfer->writeParamsToSVG();
            DocumentUndo::done(lpe_filletchamfer->getSPDoc(), _("Apply to changed nodes"), INKSCAPE_ICON("lpe-fillet-chamfer")); // TODO Fix description.
            _updating = false;
        }
    });
/*     auto model = _unit->get_model();
    for (auto u : unit_table.units(UNIT_TYPE_LINEAR)) {
        model->append({u.first,u.first});
    } */
    _unit->set_active_id(lpe_filletchamfer->unit.get_abbreviation());
    _unit->set_tooltip_text(*lpe_filletchamfer->unit.param_getTooltip());
    _unit->signal_changed().connect([=](){
        auto u = *unit_table.getUnit(_unit->get_active_id());
        lpe_filletchamfer->flexible.param_setValue(u.abbr == "%");
        lpe_filletchamfer->unit.param_set_value(u);
        lpe_filletchamfer->writeParamsToSVG();
        DocumentUndo::done(lpe_filletchamfer->getSPDoc(), _("Unit changed on LPE corners"), INKSCAPE_ICON("lpe-fillet-chamfer")); // TODO Fix description.
   });
    reload_method(this);
    auto lpeui = this;
    _auto->set_tooltip_text(_("Path will automaticaly chose archs or Bezier depending on context"));
    _auto->signal_clicked().connect([=]() {
        // TODO: 3 DRY
        if (!_updating) {
            _updating = true;
            lpe_filletchamfer->method.param_set_value(Inkscape::LivePathEffect::Filletmethod::FM_AUTO);
            lpe_filletchamfer->method.write_to_SVG();
            DocumentUndo::done(lpe_filletchamfer->getSPDoc(), _("Method changed to auto mode"), INKSCAPE_ICON("lpe-fillet-chamfer")); // TODO Fix description.
            lpeui->reload_method(lpeui);
            _updating = false;
        }
    });
    _bezier->set_tooltip_text(_("All corners will use bezier mode"));
    _bezier->signal_clicked().connect([=]() {
        if (!_updating) {
            _updating = true;
            lpe_filletchamfer->method.param_set_value(Inkscape::LivePathEffect::Filletmethod::FM_BEZIER);
            lpe_filletchamfer->method.write_to_SVG();
            DocumentUndo::done(lpe_filletchamfer->getSPDoc(), _("Method changed to auto mode"), INKSCAPE_ICON("lpe-fillet-chamfer")); // TODO Fix description.
            lpeui->reload_method(lpeui);
            _updating = false;
        }
    });
    _arc->set_tooltip_text(_("All corners will use arc mode"));
    _arc->signal_clicked().connect([=]() {
        if (!_updating) {
            _updating = true;
            lpe_filletchamfer->method.param_set_value(Inkscape::LivePathEffect::Filletmethod::FM_ARC);
            lpe_filletchamfer->method.write_to_SVG();
            DocumentUndo::done(lpe_filletchamfer->getSPDoc(), _("Method changed to auto mode"), INKSCAPE_ICON("lpe-fillet-chamfer")); // TODO Fix description.
            lpeui->reload_method(lpeui);
            _updating = false;
        }
    });
    reload_mode(this);
    _fillet->set_tooltip_text(_("Use fillet on selection"));
    _fillet->signal_clicked().connect([=]() {
        if (!_updating) {
            _updating = true;
            lpe_filletchamfer->mode.param_setValue("F");
            lpe_filletchamfer->only_selected.param_setValue(lpe_filletchamfer->selectedNodesPoints.size() > 0);
            lpe_filletchamfer->only_selected.write_to_SVG();
            lpe_filletchamfer->writeParamsToSVG();
            lpe_filletchamfer->updateNodeSatelliteType(FILLET);
            DocumentUndo::done(lpe_filletchamfer->getSPDoc(), _("Type changed to fillet mode"), INKSCAPE_ICON("lpe-fillet-chamfer")); // TODO Fix description.
            lpeui->reload_mode(lpeui);
            _updating = false;
        }
    });
    _filletinverse->set_tooltip_text(_("Use inverse fillet on selection"));
    _filletinverse->signal_clicked().connect([=]() {
        if (!_updating) {
            _updating = true;
            lpe_filletchamfer->mode.param_setValue("IF");
            lpe_filletchamfer->only_selected.param_setValue(lpe_filletchamfer->selectedNodesPoints.size() > 0);
            lpe_filletchamfer->only_selected.write_to_SVG();
            lpe_filletchamfer->writeParamsToSVG();
            lpe_filletchamfer->updateNodeSatelliteType(INVERSE_FILLET);
            DocumentUndo::done(lpe_filletchamfer->getSPDoc(), _("Type changed to fillet inverse mode"), INKSCAPE_ICON("lpe-fillet-chamfer")); // TODO Fix description.
            lpeui->reload_mode(lpeui);
            _updating = false;
        }
    });
    _chamfer->set_tooltip_text(_("Use chamfer on selection"));
    _chamfer->signal_clicked().connect([=]() {
        if (!_updating) {
            _updating = true;
            lpe_filletchamfer->mode.param_setValue("C");
            lpe_filletchamfer->only_selected.param_setValue(lpe_filletchamfer->selectedNodesPoints.size() > 0);
            lpe_filletchamfer->only_selected.write_to_SVG();
            lpe_filletchamfer->updateNodeSatelliteType(CHAMFER);
            if (_steps->get_value() > 1) {
                lpe_filletchamfer->chamfer_steps.param_set_value(1);
                _steps->set_value(1);
                lpe_filletchamfer->updateChamferSteps();
            }
            lpe_filletchamfer->writeParamsToSVG();
            DocumentUndo::done(lpe_filletchamfer->getSPDoc(), _("Method changed to chamfer mode"), INKSCAPE_ICON("lpe-fillet-chamfer")); // TODO Fix description.
            lpeui->reload_mode(lpeui);
            _updating = false;
        }
    });
    _chamfersteps->set_tooltip_text(_("Use steped chamfer on selection"));
    _chamfersteps->signal_clicked().connect([=]() {
        if (!_updating) {
            _updating = true;
            lpe_filletchamfer->mode.param_setValue("C");
            lpe_filletchamfer->only_selected.param_setValue(lpe_filletchamfer->selectedNodesPoints.size() > 0);
            lpe_filletchamfer->only_selected.write_to_SVG();
            lpe_filletchamfer->updateNodeSatelliteType(CHAMFER);
            if (_steps->get_value() == 1) {
                lpe_filletchamfer->chamfer_steps.param_set_value(2);
                _steps->set_value(2);
                lpe_filletchamfer->updateChamferSteps();
            }
            lpe_filletchamfer->writeParamsToSVG();
            DocumentUndo::done(lpe_filletchamfer->getSPDoc(), _("Method changed to chamfer mode"), INKSCAPE_ICON("lpe-fillet-chamfer")); // TODO Fix description.
            lpeui->reload_mode(lpeui);
            _updating = false;
        }
    });
    _chamferstepsinverse->set_tooltip_text(_("Use inverted steped chamfer on selection"));
    _chamferstepsinverse->signal_clicked().connect([=]() {
        if (!_updating) {
            _updating = true;
            lpe_filletchamfer->mode.param_setValue("IC");
            lpe_filletchamfer->only_selected.param_setValue(lpe_filletchamfer->selectedNodesPoints.size() > 0);
            lpe_filletchamfer->only_selected.write_to_SVG();
            lpe_filletchamfer->writeParamsToSVG();
            lpe_filletchamfer->updateNodeSatelliteType(INVERSE_CHAMFER);
            if (_steps->get_value() == 1) {
                lpe_filletchamfer->chamfer_steps.param_set_value(2);
                _steps->set_value(2);
                lpe_filletchamfer->updateChamferSteps();
            }
            DocumentUndo::done(lpe_filletchamfer->getSPDoc(), _("Method changed to chamfer inverse mode"), INKSCAPE_ICON("lpe-fillet-chamfer")); // TODO Fix description.
            lpeui->reload_mode(lpeui);
            _updating = false;
        }
    });
    _steps->set_tooltip_text(*lpe_filletchamfer->chamfer_steps.param_getTooltip());
    _steps->signal_changed().connect([=]() {
        if (!_updating) {
            _updating = true;
            lpe_filletchamfer->chamfer_steps.param_set_value(_steps->get_value());
            lpe_filletchamfer->updateChamferSteps();
            DocumentUndo::done(lpe_filletchamfer->getSPDoc(), _("Steps changed"), INKSCAPE_ICON("lpe-fillet-chamfer")); // TODO Fix description.
            lpeui->reload_mode(lpeui);
            _updating = false;
        }
    });
    _avancedexpander->property_expanded().signal_changed().connect([=](){
        _avancedopen = _avancedexpander->get_expanded();
    });
    _radiusdistance->set_active(!lpe_filletchamfer->use_knot_distance);
    _controldistance->set_active(lpe_filletchamfer->use_knot_distance);
    _radiusdistance->set_tooltip_text(_("Use radius to calculate position"));
    _radiusdistance->signal_clicked().connect([=]() {
        if (!_updating) {
            _updating = true;
            _controldistance->set_active(false);
            lpe_filletchamfer->use_knot_distance.param_setValue(false);
            lpe_filletchamfer->use_knot_distance.write_to_SVG();
            lpe_filletchamfer->updateAmount();
            DocumentUndo::done(lpe_filletchamfer->getSPDoc(), _("Distance changed to radius"), INKSCAPE_ICON("lpe-fillet-chamfer")); // TODO Fix description.
            _updating = false;
        }
    });
    _controldistance->set_tooltip_text(_("Use knot distance to calculate position (% size ignore it)"));
    _controldistance->signal_clicked().connect([=]() {
        if (!_updating) {
            _updating = true;
            _radiusdistance->set_active(false);
            lpe_filletchamfer->use_knot_distance.param_setValue(true);
            lpe_filletchamfer->use_knot_distance.write_to_SVG();
            lpe_filletchamfer->updateAmount();
            DocumentUndo::done(lpe_filletchamfer->getSPDoc(), _("Distance changed to use knot distance"), INKSCAPE_ICON("lpe-fillet-chamfer")); // TODO Fix description.
            _updating = false;
        }
    });
    _avancedexpander->set_expanded(_avancedopen);  
}


Gtk::Widget *LPEFilletChamfer::newWidget()
{
    _builder = create_builder("lpe-fillet-chamfer.ui");
    Gtk::Widget * widget = dynamic_cast<Gtk::Widget *>(&get_widget<Gtk::Box>(_builder, "filletchamferLPE"));
    reloadUI();
    return widget;
}

} // namespace LivePathEffect
} // namespace UI
} // namespace Inkscape