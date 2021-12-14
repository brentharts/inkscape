// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * HSLuv color wheel widget, based on the web implementation at
 * https://www.hsluv.org
 *
 * Authors:
 *   Massinissa Derriche <massinissa.derriche@gmail.com>
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INK_COLORWHEEL_HSLUV_H
#define INK_COLORWHEEL_HSLUV_H

#include <gtkmm.h>

namespace Inkscape {
namespace UI {
namespace Widget {

class PickerGeometry;
class ColorWheelHSLuv : public Gtk::DrawingArea
{
public:
    ColorWheelHSLuv();
    ~ColorWheelHSLuv();
    void set_rgb(double r, double g, double b);
    void get_rgb(double *r, double *g, double *b) const;
    guint32 get_rgb() const;
    void set_hsluv(double h, double s, double l);
    void set_hue(double h);
    void set_saturation(double s);
    void set_lightness(double l);
    void get_hsluv(double *h, double *s, double *l) const;
    bool is_adjusting() const { return _dragging; }

protected:
    bool on_draw(const::Cairo::RefPtr<::Cairo::Context>& cr) override;

private:
    void set_from_xy(const double x, const double y);
    void update_polygon();

    double _hue;            // Range [0,360]
    double _saturation;     // Range [0,100]
    double _lightness;      // Range [0,100]
    bool _dragging;
    double _scale;
    PickerGeometry *_picker_geometry;
    std::vector<guint32> _buffer_polygon;
    Cairo::RefPtr<::Cairo::ImageSurface> _surface_polygon;
    int _cache_width, _cache_height;
    bool _lock;
    int _square_size;

    // Callbacks
    bool on_button_press_event(GdkEventButton* event) override;
    bool on_button_release_event(GdkEventButton* event) override;
    bool on_motion_notify_event(GdkEventMotion* event) override;
    bool on_key_press_event(GdkEventKey* key_event) override;

    // Signals
public:
    sigc::signal<void> signal_color_changed();

protected:
    sigc::signal<void> _signal_color_changed;
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif // INK_COLORWHEEL_HSLUV_H

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
