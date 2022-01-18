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
#include <gtkmm/grid.h>
#include "ui/dialog-events.h"
#include "ui/widget/color-scales.h"
#include "ui/widget/color-slider.h"
#include "ui/widget/ink-color-wheel-hsluv.h"
#include "ui/widget/scrollprotected.h"
#include "hsluv.h"
#include "preferences.h"

static const int XPAD = 2;
static const int YPAD = 2;
enum COMPONENTS {
    HUE = 0,
    SATURATION,
    LIGHTNESS,
    ALPHA
};

/*
 * Add a slider to the passed in Gtk Grid.
 *
 * @param widget The grid.
 * @param row The row to add the slider to.
 * @param label The label.
 * @param tooltip The tooltip.
 * @param adjMax The maximum value of the Gtk adjustment.
 * @param adjustment The Gtk adjustment.
 * @param slider A pointer to set to the newly created slider.
 */
static void add_slider(Gtk::Grid *widget, int row, const char *label,
        const char *tooltip, double adjMax,
        Glib::RefPtr<Gtk::Adjustment> &adjustment,
        Inkscape::UI::Widget::ColorSlider *&slider)
{
    // Label
    Gtk::Label *l = Gtk::manage(new Gtk::Label(_(label), true));
    l->set_halign(Gtk::ALIGN_END);
    l->set_valign(Gtk::ALIGN_CENTER);

    l->set_margin_start(2 * XPAD);
    l->set_margin_end(XPAD);
    l->set_margin_top(YPAD);
    l->set_margin_bottom(YPAD);
    l->set_halign(Gtk::ALIGN_START);
    l->set_valign(Gtk::ALIGN_FILL);
    widget->attach(*l, 0, row, 1, 1);

    // Adjustment
    adjustment = Gtk::Adjustment::create(0.0, 0.0, adjMax, 1.0, 10.0, 10.0);

    // Slider
    slider = Gtk::manage(new Inkscape::UI::Widget::ColorSlider(adjustment));
    slider->set_tooltip_text(_(tooltip));

    slider->set_margin_start(XPAD);
    slider->set_margin_end(XPAD);
    slider->set_margin_top(YPAD);
    slider->set_margin_bottom(YPAD);
    slider->set_hexpand(true);
    slider->set_valign(Gtk::ALIGN_FILL);
    slider->set_halign(Gtk::ALIGN_FILL);
    widget->attach(*slider, 1, row, 1, 1);

    // Spinbutton
    auto spin_button = Gtk::manage(new Inkscape::UI::Widget::ScrollProtected
            <Gtk::SpinButton>(adjustment, 1.0, 0));
    spin_button->set_tooltip_text(_(tooltip));
    sp_dialog_defocus_on_enter(GTK_WIDGET(spin_button->gobj()));
    l->set_mnemonic_widget(*spin_button);

    spin_button->set_margin_start(XPAD);
    spin_button->set_margin_end(XPAD);
    spin_button->set_margin_top(YPAD);
    spin_button->set_margin_bottom(YPAD);
    spin_button->set_halign(Gtk::ALIGN_CENTER);
    spin_button->set_valign(Gtk::ALIGN_CENTER);
    widget->attach(*spin_button, 2, row, 1, 1);
}

namespace Inkscape {
namespace UI {
namespace Widget {

const gchar *ColorWheelHSLuvSelector::MODE_NAME = N_("HSLuv");

ColorWheelHSLuvSelector::ColorWheelHSLuvSelector(SelectedColor &color)
    : Gtk::Box()
    , _color(color)
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
    set_orientation(Gtk::ORIENTATION_VERTICAL);

    // Wheel
    _wheel = Gtk::manage(new Inkscape::UI::Widget::ColorWheelHSLuv());
    _wheel->set_halign(Gtk::ALIGN_FILL);
    _wheel->set_valign(Gtk::ALIGN_FILL);
    _wheel->set_hexpand(true);
    _wheel->set_vexpand(true);

    // Signal
    _wheel->signal_color_changed().connect(sigc::mem_fun(*this,
        &ColorWheelHSLuvSelector::_wheelChanged));

    // Expander
    auto wheel_frame = Gtk::make_managed<Gtk::Expander>(_("Color Wheel"));
    wheel_frame->set_margin_bottom(3);
    wheel_frame->set_halign(Gtk::ALIGN_FILL);
    wheel_frame->set_valign(Gtk::ALIGN_FILL);
    wheel_frame->set_hexpand(true);
    wheel_frame->set_vexpand(false);
    wheel_frame->set_expanded(true);
    wheel_frame->set_resize_toplevel(true);

    // Signal
    wheel_frame->property_expanded().signal_changed().connect(
        [=](){ _show_wheel(wheel_frame->get_expanded()); }
    );

    // Add Expander before the color wheel
    add(*wheel_frame);
    add(*_wheel);

    // Create sliders
    Gtk::Grid *grid = Gtk::manage(new Gtk::Grid());
    add(*grid);
    gint row = 0;

    add_slider(grid, row++, _("H*:"), _("Hue"), 360, _adjustments[HUE],
            _sliders[HUE]);
    add_slider(grid, row++, _("S*:"), _("Saturation"), 100, _adjustments[SATURATION],
            _sliders[SATURATION]);
    add_slider(grid, row++, _("L*:"), _("Lightness"), 100, _adjustments[LIGHTNESS],
            _sliders[LIGHTNESS]);
    add_slider(grid, row++, _("A:"), _("Alpha (opacity)"), 100, _adjustments[ALPHA],
            _sliders[ALPHA]);

    // Setup maps
    double h, s, l;
    _wheel->get_hsluv(&h, &s, &l);
    _sliders[HUE]->setMap(ColorScales::hsluvHueMap(s/100, l/100,
                &_slider_maps[HUE]));
    _sliders[SATURATION]->setMap(ColorScales::hsluvSaturationMap(h/360, l/100,
                &_slider_maps[SATURATION]));
    _sliders[LIGHTNESS]->setMap(ColorScales::hsluvLightnessMap(h/360, s/100,
                &_slider_maps[LIGHTNESS]));
    _sliders[ALPHA]->setColors(0xffffff00, 0xffffff7f, 0xffffffff);

    // Signals
    _adjustments[HUE]->signal_value_changed().connect(sigc::mem_fun(this,
        &ColorWheelHSLuvSelector::_adjustmentHueChanged));
    _adjustments[SATURATION]->signal_value_changed().connect(sigc::mem_fun(this,
        &ColorWheelHSLuvSelector::_adjustmentSaturationChanged));
    _adjustments[LIGHTNESS]->signal_value_changed().connect(sigc::mem_fun(this,
        &ColorWheelHSLuvSelector::_adjustmentLightnessChanged));
    _adjustments[ALPHA]->signal_value_changed().connect(sigc::mem_fun(this,
        &ColorWheelHSLuvSelector::_adjustmentAlphaChanged));
    for (int i = 0; i < 4; i++) {
        _sliders[i]->signal_grabbed.connect(sigc::mem_fun(*this,
            &ColorWheelHSLuvSelector::_sliderGrabbed));
        _sliders[i]->signal_released.connect(sigc::mem_fun(*this,
            &ColorWheelHSLuvSelector::_sliderReleased));
    }

    show_all();

    // Restore the visibility of the wheel
    _wheel_visible = Inkscape::Preferences::get()->getBool(_prefs + "/wheel", true);
    wheel_frame->set_expanded(_wheel_visible);
    _update_wheel_layout();
}

void ColorWheelHSLuvSelector::on_show()
{
    Gtk::Box::on_show();
    _updateDisplay();
}

void ColorWheelHSLuvSelector::_show_wheel(bool visible)
{
    _wheel_visible = visible;
    _update_wheel_layout();
    Inkscape::Preferences::get()->setBool(_prefs + "/wheel", _wheel_visible);
}

void ColorWheelHSLuvSelector::_update_wheel_layout()
{
    if (_wheel_visible) {
        _wheel->show();
    }
    else {
        _wheel->hide();
    }
}

void ColorWheelHSLuvSelector::_colorChanged()
{
    _updateDisplay();
}

void ColorWheelHSLuvSelector::_sliderGrabbed()
{
    _color.preserveICC();
    _color.setHeld(true);
}

void ColorWheelHSLuvSelector::_sliderReleased()
{
    _color.preserveICC();
    _color.setHeld(false);
}

void ColorWheelHSLuvSelector::_adjustmentHueChanged()
{
    if (_updating) return;

    _color.preserveICC();

    double h = _adjustments[HUE]->get_value(), s, l;
    _wheel->get_hsluv(nullptr, &s, &l);
    double r, g, b;
    Hsluv::hsluv_to_rgb(h, s, l, &r, &g, &b);
    SPColor color(r, g, b);
    _color.setColor(color);
}

void ColorWheelHSLuvSelector::_adjustmentSaturationChanged()
{
    if (_updating) return;

    _color.preserveICC();

    double h, s = _adjustments[SATURATION]->get_value(), l;
    _wheel->get_hsluv(&h, nullptr, &l);
    double r, g, b;
    Hsluv::hsluv_to_rgb(h, s, l, &r, &g, &b);
    SPColor color(r, g, b);
    _color.setColor(color);
}

void ColorWheelHSLuvSelector::_adjustmentLightnessChanged()
{
    if (_updating) return;

    _color.preserveICC();

    double h, s, l = _adjustments[LIGHTNESS]->get_value();
    _wheel->get_hsluv(&h, &s, nullptr);
    double r, g, b;
    Hsluv::hsluv_to_rgb(h, s, l, &r, &g, &b);
    SPColor color(r, g, b);
    _color.setColor(color);
}

void ColorWheelHSLuvSelector::_adjustmentAlphaChanged()
{
    if (_updating) return;

    _color.preserveICC();
    _color.setAlpha(ColorScales::getScaled(_adjustments[ALPHA]));
}

void ColorWheelHSLuvSelector::_wheelChanged()
{
    if (_updating) return;

    _updating = true;

    double rgb[3];
    _wheel->get_rgb(&rgb[0], &rgb[1], &rgb[2]);
    SPColor color (rgb[0], rgb[1], rgb[2]);

    double h, s, l;
    _wheel->get_hsluv(&h, &s, &l);

    // Sliders
    _sliders[HUE]->setMap(ColorScales::hsluvHueMap(s/100, l/100,
                &_slider_maps[HUE]));
    _sliders[SATURATION]->setMap(ColorScales::hsluvSaturationMap(h/360, l/100,
                &_slider_maps[SATURATION]));
    _sliders[LIGHTNESS]->setMap(ColorScales::hsluvLightnessMap(h/360, s/100,
                &_slider_maps[LIGHTNESS]));

    // Alpha slider
    guint32 start = color.toRGBA32(0x00);
    guint32 mid = color.toRGBA32(0x7f);
    guint32 end = color.toRGBA32(0xff);
    _sliders[ALPHA]->setColors(start, mid, end);

    // Adjustments
    ColorScales::setScaled(_adjustments[HUE], h / 360.0);
    ColorScales::setScaled(_adjustments[SATURATION], s / 100.0);

    // Color
    _color.preserveICC();
    _color.setHeld(_wheel->is_adjusting());

    _color.setColor(color);

    _updating = false;
}

void ColorWheelHSLuvSelector::_updateDisplay()
{
    if (_updating) return;

#ifdef DUMP_CHANGE_INFO
    g_message("ColorWheelHSLuvSelector::_colorChanged( this=%p, %f, %f, %f,   %f)",
        this, _color.color().v.c[0], _color.color().v.c[1], _color.color().v.c[2],
        alpha);
#endif

    _updating = true;

    _wheel->set_rgb(_color.color().v.c[0], _color.color().v.c[1],
            _color.color().v.c[2]);

    double h, s, l;
    _wheel->get_hsluv(&h, &s, &l);

    // Sliders
    _sliders[HUE]->setMap(ColorScales::hsluvHueMap(s/100, l/100,
                &_slider_maps[HUE]));
    _sliders[SATURATION]->setMap(ColorScales::hsluvSaturationMap(h/360, l/100,
                &_slider_maps[SATURATION]));
    _sliders[LIGHTNESS]->setMap(ColorScales::hsluvLightnessMap(h/360, s/100,
                &_slider_maps[LIGHTNESS]));

    // Alpha slider
    guint32 start = _color.color().toRGBA32(0x00);
    guint32 mid = _color.color().toRGBA32(0x7f);
    guint32 end = _color.color().toRGBA32(0xff);
    _sliders[ALPHA]->setColors(start, mid, end);

    // Adjustments
    ColorScales::setScaled(_adjustments[HUE], h / 360.0);
    ColorScales::setScaled(_adjustments[SATURATION], s / 100.0);
    ColorScales::setScaled(_adjustments[LIGHTNESS], l / 100.0);
    ColorScales::setScaled(_adjustments[ALPHA], _color.alpha());

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
