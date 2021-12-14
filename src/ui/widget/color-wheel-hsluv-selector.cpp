// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * HSLuv color selector widget, based on the web implementation at
 * https://www.hsluv.org
 *
 * Authors:
 *   Massinissa Derriche <massinissa.derriche@gmail.com>
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "color-wheel-hsluv-selector.h"

#include <glibmm/i18n.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/label.h>
#include <gtkmm/spinbutton.h>
#include "ui/dialog-events.h"
#include "ui/widget/color-scales.h"
#include "ui/widget/color-slider.h"
#include "ui/widget/ink-color-wheel-hsluv.h"
#include "ui/widget/scrollprotected.h"
#include "hsluv.h"

namespace Inkscape {
namespace UI {
namespace Widget {

static const int XPAD = 4;
static const int YPAD = 4;

const gchar *ColorWheelHSLuvSelector::MODE_NAME = N_("HSLuv Wheel");

ColorWheelHSLuvSelector::ColorWheelHSLuvSelector(SelectedColor &color)
    : Gtk::Grid()
    , _color(color)
    , _updating(false)
    , _wheel(nullptr)
    , _lightness_slider(nullptr)
    , _alpha_slider(nullptr)
{
    set_name("ColorWheelHSLuvSelector");

    _initUI();
    _color_changed_connection = color.signal_changed.connect(sigc::mem_fun(
                this, &ColorWheelHSLuvSelector::_colorChanged));
    _color_dragged_connection = color.signal_dragged.connect(sigc::mem_fun(
                this, &ColorWheelHSLuvSelector::_colorChanged));
}

ColorWheelHSLuvSelector::~ColorWheelHSLuvSelector()
{
    _color_changed_connection.disconnect();
    _color_dragged_connection.disconnect();
}

void ColorWheelHSLuvSelector::_initUI()
{
    /* Create components */
    gint row = 0;

    /* Wheel */
    _wheel = Gtk::manage(new Inkscape::UI::Widget::ColorWheelHSLuv());
    _wheel->set_halign(Gtk::ALIGN_FILL);
    _wheel->set_valign(Gtk::ALIGN_FILL);
    _wheel->set_hexpand(true);
    _wheel->set_vexpand(true);
    attach(*_wheel, 0, row, 3, 1);

    // Signals
    _wheel->signal_color_changed().connect(sigc::mem_fun(*this,
        &ColorWheelHSLuvSelector::_wheelChanged));

    row++;

    /* Lightness */
    // Label
    Gtk::Label *label = Gtk::manage(new Gtk::Label(_("_L:"), true));
    label->set_halign(Gtk::ALIGN_END);
    label->set_valign(Gtk::ALIGN_CENTER);

    label->set_margin_start(XPAD);
    label->set_margin_end(XPAD);
    label->set_margin_top(YPAD);
    label->set_margin_bottom(YPAD);
    label->set_halign(Gtk::ALIGN_FILL);
    label->set_valign(Gtk::ALIGN_FILL);
    attach(*label, 0, row, 1, 1);

    // Adjustment
    _lightness_adjustment =
        Gtk::Adjustment::create(0.0, 0.0, 100.0, 1.0, 10.0, 10.0);

    // Slider
    _lightness_slider =
        Gtk::manage(new Inkscape::UI::Widget::ColorSlider(_lightness_adjustment));
    _lightness_slider->set_tooltip_text(_("Lightness"));

    _lightness_slider->set_margin_start(XPAD);
    _lightness_slider->set_margin_end(XPAD);
    _lightness_slider->set_margin_top(YPAD);
    _lightness_slider->set_margin_bottom(YPAD);
    _lightness_slider->set_hexpand(true);
    _lightness_slider->set_valign(Gtk::ALIGN_FILL);
    _lightness_slider->set_halign(Gtk::ALIGN_FILL);
    attach(*_lightness_slider, 1, row, 1, 1);

    double h, s;
    _wheel->get_hsluv(&h, &s, nullptr);
    _lightness_slider->setMap(ColorScales::hsluvLightnessMap(h/360, s/100, &_lightness_slider_map));

    // Spinbutton
    auto spin_button = Gtk::manage(
        new ScrollProtected<Gtk::SpinButton>(_lightness_adjustment, 1.0, 0));
    spin_button->set_tooltip_text(_("Lightness"));
    sp_dialog_defocus_on_enter(GTK_WIDGET(spin_button->gobj()));
    label->set_mnemonic_widget(*spin_button);

    spin_button->set_margin_start(XPAD);
    spin_button->set_margin_end(XPAD);
    spin_button->set_margin_top(YPAD);
    spin_button->set_margin_bottom(YPAD);
    spin_button->set_halign(Gtk::ALIGN_CENTER);
    spin_button->set_valign(Gtk::ALIGN_CENTER);
    attach(*spin_button, 2, row, 1, 1);

    // Signals
    _lightness_adjustment->signal_value_changed().connect(sigc::mem_fun(this,
        &ColorWheelHSLuvSelector::_adjustmentLightnessChanged));
    _lightness_slider->signal_grabbed.connect(sigc::mem_fun(*this,
        &ColorWheelHSLuvSelector::_sliderLightnessGrabbed));
    _lightness_slider->signal_released.connect(sigc::mem_fun(*this,
        &ColorWheelHSLuvSelector::_sliderLightnessReleased));

    row++;

    /* Alpha */
    // Label
    label = Gtk::manage(new Gtk::Label(_("_A:"), true));
    label->set_halign(Gtk::ALIGN_END);
    label->set_valign(Gtk::ALIGN_CENTER);

    label->set_margin_start(XPAD);
    label->set_margin_end(XPAD);
    label->set_margin_top(YPAD);
    label->set_margin_bottom(YPAD);
    label->set_halign(Gtk::ALIGN_FILL);
    label->set_valign(Gtk::ALIGN_FILL);
    attach(*label, 0, row, 1, 1);

    // Adjustment
    _alpha_adjustment = Gtk::Adjustment::create(0.0, 0.0, 100.0, 1.0, 10.0, 10.0);

    // Slider
    _alpha_slider =
        Gtk::manage(new Inkscape::UI::Widget::ColorSlider(_alpha_adjustment));
    _alpha_slider->set_tooltip_text(_("Alpha (opacity)"));

    _alpha_slider->set_margin_start(XPAD);
    _alpha_slider->set_margin_end(XPAD);
    _alpha_slider->set_margin_top(YPAD);
    _alpha_slider->set_margin_bottom(YPAD);
    _alpha_slider->set_hexpand(true);
    _alpha_slider->set_valign(Gtk::ALIGN_FILL);
    _alpha_slider->set_halign(Gtk::ALIGN_FILL);
    attach(*_alpha_slider, 1, row, 1, 1);

    _alpha_slider->setColors(SP_RGBA32_F_COMPOSE(1.0, 1.0, 1.0, 0.0),
                             SP_RGBA32_F_COMPOSE(1.0, 1.0, 1.0, 0.5),
                             SP_RGBA32_F_COMPOSE(1.0, 1.0, 1.0, 1.0));

    // Spinbutton
    spin_button = Gtk::manage(
        new ScrollProtected<Gtk::SpinButton>(_alpha_adjustment, 1.0, 0));
    spin_button->set_tooltip_text(_("Alpha (opacity)"));
    sp_dialog_defocus_on_enter(GTK_WIDGET(spin_button->gobj()));
    label->set_mnemonic_widget(*spin_button);

    spin_button->set_margin_start(XPAD);
    spin_button->set_margin_end(XPAD);
    spin_button->set_margin_top(YPAD);
    spin_button->set_margin_bottom(YPAD);
    spin_button->set_halign(Gtk::ALIGN_CENTER);
    spin_button->set_valign(Gtk::ALIGN_CENTER);
    attach(*spin_button, 2, row, 1, 1);

    // Signals
    _alpha_adjustment->signal_value_changed().connect(sigc::mem_fun(this,
        &ColorWheelHSLuvSelector::_adjustmentAlphaChanged));
    _alpha_slider->signal_grabbed.connect(sigc::mem_fun(*this,
        &ColorWheelHSLuvSelector::_sliderAlphaGrabbed));
    _alpha_slider->signal_released.connect(sigc::mem_fun(*this,
        &ColorWheelHSLuvSelector::_sliderAlphaReleased));

      show_all();
}

void ColorWheelHSLuvSelector::on_show()
{
    Gtk::Grid::on_show();
    _updateDisplay();
}

void ColorWheelHSLuvSelector::_colorChanged()
{
    _updateDisplay();
}

void ColorWheelHSLuvSelector::_adjustmentLightnessChanged()
{
    if (_updating) {
        return;
    }

    _color.preserveICC();

    double h, s, l = _lightness_adjustment->get_value();
    _wheel->get_hsluv(&h, &s, nullptr);
    double r, g, b;
    Hsluv::hsluv_to_rgb(h, s, l, &r, &g, &b);
    SPColor color(r, g, b);
    _color.setColor(color);
}

void ColorWheelHSLuvSelector::_sliderLightnessGrabbed()
{
    _color.preserveICC();
    _color.setHeld(true);
}

void ColorWheelHSLuvSelector::_sliderLightnessReleased()
{
    _color.preserveICC();
    _color.setHeld(false);
}

void ColorWheelHSLuvSelector::_adjustmentAlphaChanged()
{
    if (_updating) {
        return;
    }

    _color.preserveICC();
    _color.setAlpha(ColorScales::getScaled(_alpha_adjustment));
}

void ColorWheelHSLuvSelector::_sliderAlphaGrabbed()
{
    _color.preserveICC();
    _color.setHeld(true);
}
void ColorWheelHSLuvSelector::_sliderAlphaReleased()
{
    _color.preserveICC();
    _color.setHeld(false);
}

void ColorWheelHSLuvSelector::_wheelChanged()
{
    if (_updating) {
        return;
    }

    _updating = true;

    double rgb[3];
    _wheel->get_rgb(&rgb[0], &rgb[1], &rgb[2]);
    SPColor color (rgb[0], rgb[1], rgb[2]);

    // Alpha slider
    guint32 start = color.toRGBA32(0x00);
    guint32 mid = color.toRGBA32(0x7f);
    guint32 end = color.toRGBA32(0xff);
    _alpha_slider->setColors(start, mid, end);

    // Lightness slider
    double h, s;
    _wheel->get_hsluv(&h, &s, nullptr);
    _lightness_slider->setMap(ColorScales::hsluvLightnessMap(h/360, s/100, &_lightness_slider_map));

    // Color
    _color.preserveICC();
    _color.setHeld(_wheel->is_adjusting());
    _color.setColor(color);

    _updating = false;
}

void ColorWheelHSLuvSelector::_updateDisplay()
{
    if (_updating) {
        return;
    }

#ifdef DUMP_CHANGE_INFO
    g_message("ColorWheelHSLuvSelector::_colorChanged( this=%p, %f, %f, %f,   %f)",
        this, _color.color().v.c[0], _color.color().v.c[1], _color.color().v.c[2],
        alpha);
#endif

    _updating = true;

    _wheel->set_rgb(_color.color().v.c[0], _color.color().v.c[1],
            _color.color().v.c[2]);

    // Lightness
    double h, s, l;
    _wheel->get_hsluv(&h, &s, &l);
    _lightness_slider->setMap(ColorScales::hsluvLightnessMap(h/360, s/100, &_lightness_slider_map));
    ColorScales::setScaled(_lightness_adjustment, l / 100.0);

    // Alpha
    guint32 start = _color.color().toRGBA32(0x00);
    guint32 mid = _color.color().toRGBA32(0x7f);
    guint32 end = _color.color().toRGBA32(0xff);
    _alpha_slider->setColors(start, mid, end);
    ColorScales::setScaled(_alpha_adjustment, _color.alpha());

    _updating = false;
}

Gtk::Widget *ColorWheelHSLuvSelectorFactory::createWidget(
        Inkscape::UI::SelectedColor &color) const
{
    Gtk::Widget *w = Gtk::manage(new ColorWheelHSLuvSelector(color));
    return w;
}

Glib::ustring ColorWheelHSLuvSelectorFactory::modeName() const
{
    return gettext(ColorWheelHSLuvSelector::MODE_NAME);
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=80 :
