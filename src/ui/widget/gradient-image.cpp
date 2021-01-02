// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * A simple gradient preview
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <sigc++/sigc++.h>

#include <glibmm/refptr.h>
#include <gdkmm/pixbuf.h>

#include <cairomm/surface.h>

#include "gradient-image.h"

#include "display/cairo-utils.h"

#include "object/sp-gradient.h"
#include "object/sp-stop.h"

namespace Inkscape {
namespace UI {
namespace Widget {
GradientImage::GradientImage(SPGradient *gradient)
    : _gradient(nullptr)
{
    set_has_window(false);
    set_gradient(gradient);
}

GradientImage::~GradientImage()
{
    if (_gradient) {
        _release_connection.disconnect();
        _modified_connection.disconnect();
        _gradient = nullptr;
    }
}

void
GradientImage::size_request(GtkRequisition *requisition) const
{
    requisition->width = 54;
    requisition->height = _stop_size ? _stop_size : 12;
}

void
GradientImage::get_preferred_width_vfunc(int &minimal_width, int &natural_width) const
{
    GtkRequisition requisition;
    size_request(&requisition);
    minimal_width = natural_width = requisition.width;
}

void
GradientImage::get_preferred_height_vfunc(int &minimal_height, int &natural_height) const
{
    GtkRequisition requisition;
    size_request(&requisition);
    minimal_height = natural_height = requisition.height;
}

bool
GradientImage::on_draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    auto allocation = get_allocation();

    auto ct = cr->cobj();

    if (_stops_only) {
        auto context = get_style_context();
        Gdk::RGBA fg;
        double alpha = 0.2;
        if (context->lookup_color("theme_fg_color", fg)) {
            cairo_set_source_rgba(ct, fg.get_red(), fg.get_green(), fg.get_blue(), alpha);
        }
        else {
            cairo_set_source_rgba(ct, 0.5, 0.5, 0.5, alpha);
        }

        int w = allocation.get_width();
        int h = allocation.get_height();

        if (_gradient && _stop_size > 0 && w > _stop_size && h >= _stop_size) {
            // draw stop positions
            _gradient->ensureVector();
            const auto& stops = _gradient->vector.stops;
            double x = _stop_size / 2.0;
            double y = _stop_size / 2.0;
            int width = w - _stop_size;
            double radius = _stop_size / 2.0;
            auto pos = [=](double offset) { return round(x + width * offset); };

            cairo_save(ct);
            cairo_new_sub_path(ct);
            for (const auto& stop : stops) {
                int position = pos(stop.offset);
                cairo_arc(ct, position, y, radius, 0, 2 * M_PI);
            }
            cairo_close_path(ct);
            cairo_fill(ct);
            cairo_restore(ct);

            radius--;

            cairo_save(ct);
            // draw stops from the last to the first, that ensures right overlap, if there's any
            for (auto it = rbegin(stops); it != rend(stops); ++it) {
                int position = pos(it->offset);
                cairo_new_sub_path(ct);
                cairo_arc(ct, position, y, radius, 0, 2 * M_PI);
                cairo_close_path(ct);
                ink_cairo_set_source_color(ct, it->color, 1.0);
                cairo_fill(ct);
            }
            cairo_restore(ct);

            cairo_pattern_t* checkers = ink_cairo_pattern_create_checkerboard();
            // now draw half-circles, this time in order
            for (auto it = begin(stops); it != end(stops); ++it) {
                int position = pos(it->offset);
                cairo_new_sub_path(ct);
                double deg90 = M_PI / 2;
                cairo_arc(ct, position, y, radius, -deg90, deg90);
                cairo_line_to(ct, position, y);
                cairo_close_path(ct);
                if (it->opacity < 1.0) {
                    cairo_set_source(ct, checkers);
                    cairo_fill_preserve(ct);
                }
                ink_cairo_set_source_rgba32(ct, it->color.toRGBA32(it->opacity));
                cairo_fill(ct);
            }
            cairo_pattern_destroy(checkers);
        }
    } else {
        cairo_pattern_t *check = ink_cairo_pattern_create_checkerboard();

        cairo_set_source(ct, check);
        cairo_paint(ct);
        cairo_pattern_destroy(check);

        if (_gradient) {
            auto p = _gradient->create_preview_pattern(allocation.get_width());
            cairo_set_source(ct, p);
            cairo_paint(ct);
            cairo_pattern_destroy(p);
        }
    }
    return true;
}

void GradientImage::draw_stops_only(bool enable, int size) {
    _stops_only = enable;
    _stop_size = size;
    update();
}

void
GradientImage::set_gradient(SPGradient *gradient)
{
    if (_gradient) {
        _release_connection.disconnect();
        _modified_connection.disconnect();
    }

    _gradient = gradient;

    if (gradient) {
        _release_connection = gradient->connectRelease(sigc::mem_fun(this, &GradientImage::gradient_release));
        _modified_connection = gradient->connectModified(sigc::mem_fun(this, &GradientImage::gradient_modified));
    }

    update();
}

void
GradientImage::gradient_release(SPObject *)
{
    if (_gradient) {
        _release_connection.disconnect();
        _modified_connection.disconnect();
    }

    _gradient = nullptr;

    update();
}

void
GradientImage::gradient_modified(SPObject *, guint /*flags*/)
{
    update();
}

void
GradientImage::update()
{
    if (get_is_drawable()) {
        queue_draw();
    }
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

GdkPixbuf*
sp_gradient_to_pixbuf (SPGradient *gr, int width, int height)
{
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *ct = cairo_create(s);

    cairo_pattern_t *check = ink_cairo_pattern_create_checkerboard();
    cairo_set_source(ct, check);
    cairo_paint(ct);
    cairo_pattern_destroy(check);

    if (gr) {
        cairo_pattern_t *p = gr->create_preview_pattern(width);
        cairo_set_source(ct, p);
        cairo_paint(ct);
        cairo_pattern_destroy(p);
    }

    cairo_destroy(ct);
    cairo_surface_flush(s);

    // no need to free s - the call below takes ownership
    GdkPixbuf *pixbuf = ink_pixbuf_create_from_cairo_surface(s);
    return pixbuf;
}


Glib::RefPtr<Gdk::Pixbuf>
sp_gradient_to_pixbuf_ref (SPGradient *gr, int width, int height)
{
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *ct = cairo_create(s);

    cairo_pattern_t *check = ink_cairo_pattern_create_checkerboard();
    cairo_set_source(ct, check);
    cairo_paint(ct);
    cairo_pattern_destroy(check);

    if (gr) {
        cairo_pattern_t *p = gr->create_preview_pattern(width);
        cairo_set_source(ct, p);
        cairo_paint(ct);
        cairo_pattern_destroy(p);
    }

    cairo_destroy(ct);
    cairo_surface_flush(s);

    Cairo::RefPtr<Cairo::Surface> sref = Cairo::RefPtr<Cairo::Surface>(new Cairo::Surface(s));
    Glib::RefPtr<Gdk::Pixbuf> pixbuf =
        Gdk::Pixbuf::create(sref, 0, 0, width, height);

    cairo_surface_destroy(s);

    return pixbuf;
}


Glib::RefPtr<Gdk::Pixbuf>
sp_gradstop_to_pixbuf_ref (SPStop *stop, int width, int height)
{
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *ct = cairo_create(s);

    /* Checkerboard background */
    cairo_pattern_t *check = ink_cairo_pattern_create_checkerboard();
    cairo_rectangle(ct, 0, 0, width, height);
    cairo_set_source(ct, check);
    cairo_fill_preserve(ct);
    cairo_pattern_destroy(check);

    if (stop) {
        /* Alpha area */
        cairo_rectangle(ct, 0, 0, width/2, height);
        ink_cairo_set_source_rgba32(ct, stop->get_rgba32());
        cairo_fill(ct);

        /* Solid area */
        cairo_rectangle(ct, width/2, 0, width, height);
        ink_cairo_set_source_rgba32(ct, stop->get_rgba32() | 0xff);
        cairo_fill(ct);
    }

    cairo_destroy(ct);
    cairo_surface_flush(s);

    Cairo::RefPtr<Cairo::Surface> sref = Cairo::RefPtr<Cairo::Surface>(new Cairo::Surface(s));
    Glib::RefPtr<Gdk::Pixbuf> pixbuf =
        Gdk::Pixbuf::create(sref, 0, 0, width, height);

    cairo_surface_destroy(s);

    return pixbuf;
}



/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
