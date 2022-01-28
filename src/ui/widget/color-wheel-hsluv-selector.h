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

#include <gtkmm/box.h>
#include <gtkmm/expander.h>

#include "ui/selected-color.h"

namespace Inkscape {
namespace UI {
namespace Widget {

class ColorSlider;
class ColorWheelHSLuv;
class ColorWheelHSLuvSelector
    : public Gtk::Box
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

    void _show_wheel(bool visible);
    void _update_wheel_layout();

    // Signals
    void _colorChanged();

    void _sliderGrabbed();
    void _sliderReleased();

    void _adjustmentHueChanged();
    void _adjustmentSaturationChanged();
    void _adjustmentLightnessChanged();
    void _adjustmentAlphaChanged();

    void _wheelChanged();

    void _updateDisplay();

    // Members
    SelectedColor &_color;
    bool _updating = false;
    Inkscape::UI::Widget::ColorWheelHSLuv *_wheel = nullptr;
    Glib::RefPtr<Gtk::Adjustment> _adjustments[4];
    Inkscape::UI::Widget::ColorSlider *_sliders[4]{};
    std::array<guchar, 4 * 1024> _slider_maps[4];

private:
    sigc::connection _color_changed_connection;
    sigc::connection _color_dragged_connection;
    bool _wheel_visible = true;
    const Glib::ustring _prefs = "/hsluv-selector";
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
