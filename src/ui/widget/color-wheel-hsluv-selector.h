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

#ifndef SEEN_SP_COLOR_WHEEL_HSLUV_SELECTOR_H
#define SEEN_SP_COLOR_WHEEL_HSLUV_SELECTOR_H

#include <gtkmm/grid.h>

#include "ui/selected-color.h"

namespace Inkscape {
namespace UI {
namespace Widget {

class ColorSlider;
class ColorWheelHSLuv;
class ColorWheelHSLuvSelector
    : public Gtk::Grid
{
public:
    static const gchar *MODE_NAME;

    ColorWheelHSLuvSelector(SelectedColor &color);
    ~ColorWheelHSLuvSelector() override;

    // By default, disallow copy constructor and assignment operator
    ColorWheelHSLuvSelector(const ColorWheelHSLuvSelector &obj) = delete;
    ColorWheelHSLuvSelector &operator=(const ColorWheelHSLuvSelector &obj) = delete;

protected:
    void _initUI();

    void on_show() override;

    // Signals
    void _colorChanged();

    void _adjustmentLightnessChanged();
    void _sliderLightnessGrabbed();
    void _sliderLightnessReleased();

    void _adjustmentAlphaChanged();
    void _sliderAlphaGrabbed();
    void _sliderAlphaReleased();

    void _wheelChanged();

    void _updateDisplay();

    // Members
    SelectedColor &_color;
    bool _updating;
    Glib::RefPtr<Gtk::Adjustment> _lightness_adjustment;
    Glib::RefPtr<Gtk::Adjustment> _alpha_adjustment;
    Inkscape::UI::Widget::ColorWheelHSLuv *_wheel;
    Inkscape::UI::Widget::ColorSlider *_lightness_slider;
    Inkscape::UI::Widget::ColorSlider *_alpha_slider;
    std::array<guchar, 4 * 1024> _lightness_slider_map;

private:
    sigc::connection _color_changed_connection;
    sigc::connection _color_dragged_connection;
};

class ColorWheelHSLuvSelectorFactory : public ColorSelectorFactory {
public:
    Gtk::Widget *createWidget(SelectedColor &color) const override;
    Glib::ustring modeName() const override;
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif // SEEN_SP_COLOR_WHEEL_HSLUV_SELECTOR_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
