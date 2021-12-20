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

#include "ink-color-wheel-hsluv.h"

#include <cstring>
#include <algorithm>

#include "hsluv.h"

// Sizes in pixels
static const int SIZE = 400;
static const int OUTER_CIRCLE_RADIUS = 190;

static const double MAX_HUE = 360.0;
static const double MAX_SATURATION = 100.0;
static const double MAX_LIGHTNESS = 100.0;
static const double MIN_HUE = 0.0;
static const double MIN_SATURATION = 0.0;
static const double MIN_LIGHTNESS = 0.0;
static const double OUTER_CIRCLE_DASH_SIZE = 10.0;

// A point with a color value.
class color_point {
public:
    color_point() : x(0), y(0), r(0), g(0), b(0) {};
    color_point(double x, double y, double r, double g, double b) : x(x), y(y), r(r), g(g), b(b) {};
    color_point(double x, double y, guint color) : x(x), y(y),
                                                   r(((color&0xff0000)>>16)/255.0),
                                                   g(((color&0x00ff00)>> 8)/255.0),
                                                   b(((color&0x0000ff)    )/255.0)
    {};
    guint32 get_color() { return (int(r*255) << 16 | int(g*255) << 8 | int(b*255)); };
    void set_color(double red, double green, double blue) { r = red, g = green; b = blue; };
    double x;
    double y;
    double r;
    double g;
    double b;
};

class Point {
public:
    Point () : x(0), y(0) {}
    Point (double x, double y) : x(x), y(y) {}
    double x;
    double y;
};
using Hsluv::Line;

class Intersection {
public:
    Intersection () : line1(0), line2(0) {}
    Intersection (int line1, int line2, Point intersectionPoint,
            double intersectionPointAngle, double relativeAngle)
        : line1(line1)
        , line2(line2)
        , intersectionPoint(intersectionPoint)
        , intersectionPointAngle(intersectionPointAngle)
        , relativeAngle(relativeAngle) {}
    int line1;
    int line2;
    Point intersectionPoint;
    double intersectionPointAngle;
    double relativeAngle;
};

namespace Geometry {

static Point intersectLineLine(Line a, Line b)
{
    double x = (a.intercept - b.intercept) / (b.slope - a.slope);
    double y = a.slope * x + a.intercept;
    return {x, y};
}

static double distanceFromOrigin(Point point)
{
    return std::sqrt(std::pow(point.x, 2) + std::pow(point.y, 2));
}

static double distanceLineFromOrigin(Line line)
{
    // https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
    return std::abs(line.intercept) / std::sqrt(std::pow(line.slope, 2) + 1);
}

static double angleFromOrigin(Point point)
{
    return std::atan2(point.y, point.x);
}

static double normalizeAngle(double angle)
{
    double m = 2 * M_PI;
    return std::fmod(std::fmod(angle, m) + m, m);
}

} // namespace Geometry

/**
 * Convert the vertice of the in gamut color polygon (Luv) to pixel coordinates.
 *
 * @param point The point in Luv coordinates.
 * @param scale Zoom amount to fit polygon to outer circle.
 * @param resize Zoom amount to fit wheel in widget.
 */
static Point toPixelCoordinate(const Point& point, double scale, double resize)
{
    return Point(
        point.x * scale * resize + (SIZE * resize / 2.0),
        (SIZE * resize / 2.0) - point.y * scale * resize
    );
}

/**
 * Convert a point in pixels on the widget to Luv coordinates.
 *
 * @param point The point in pixel coordinates.
 * @param scale Zoom amount to fit polygon to outer circle.
 * @param resize Zoom amount to fit wheel in widget.
 */
static Point fromPixelCoordinate(const Point& point, double scale, double resize)
{
    return Point(
        (point.x - (SIZE * resize / 2.0)) / (scale * resize),
        ((SIZE * resize / 2.0) - point.y) / (scale * resize)
    );
}

/**
 * @overload
 * @param point A vector of points in Luv coordinates.
 * @param scale Zoom amount to fit polygon to outer circle.
 * @param resize Zoom amount to fit wheel in widget.
 */
static std::vector<Point> toPixelCoordinate(
        const std::vector<Point>& points, double scale, double resize)
{
    std::vector<Point> result;

    for (const Point& p : points) {
        result.emplace_back(toPixelCoordinate(p, scale, resize));
    }

    return result;
}

namespace Inkscape {
namespace UI {
namespace Widget {

/**
 * Used to represent the in RGB gamut colors polygon of the color wheel.
 *
 * @struct
 */
struct PickerGeometry {
    std::vector<Line> lines;
    /** Ordered such that 1st vertex is intersection between first and second
     *  line, 2nd vertex between second and third line etc. */
    std::vector<Point> vertices;
    /** Angles from origin to corresponding vertex, in radians */
    std::vector<double> angles;
    /** Smallest circle with center at origin such that polygon fits inside */
    double outerCircleRadius;
    /** Largest circle with center at origin such that it fits inside polygon */
    double innerCircleRadius;
};

/**
 * Update the passed in PickerGeometry structure to the given lightness value.
 *
 * @param[out] pickerGeometry The PickerGeometry instance to update.
 * @param lightness The lightness value.
 */
static void getPickerGeometry(PickerGeometry *pickerGeometry, double lightness)
{
    // Add a lambda to avoid overlapping intersections
    lightness = std::clamp(lightness + 0.01, 0.1, 99.9);

    // Array of lines
    const std::array<Line, 6> lines = Hsluv::getBounds(lightness);
    int numLines = lines.size();
    double outerCircleRadius = 0.0;

    // Find the line closest to origin
    int closestIndex2 = -1;
    double closestLineDistance = -1;

    for (int i = 0; i < numLines; i++) {
        double d = Geometry::distanceLineFromOrigin(lines[i]);
        if (closestLineDistance < 0 || d < closestLineDistance) {
            closestLineDistance = d;
            closestIndex2 = i;
        }
    }

    Line closestLine = lines[closestIndex2];
    Line perpendicularLine (0 - (1 / closestLine.slope), 0.0);

    Point intersectionPoint = Geometry::intersectLineLine(closestLine,
            perpendicularLine);
    double startingAngle = Geometry::angleFromOrigin(intersectionPoint);

    std::vector<Intersection> intersections;
    double intersectionPointAngle;
    double relativeAngle;

    for (int i = 0; i < numLines - 1; i++) {
        for (int j = i + 1; j < numLines; j++) {
            intersectionPoint = Geometry::intersectLineLine(lines[i], lines[j]);
            intersectionPointAngle = Geometry::angleFromOrigin(intersectionPoint);
            relativeAngle = Geometry::normalizeAngle(
                    intersectionPointAngle - startingAngle);
            intersections.emplace_back(i, j, intersectionPoint,
                    intersectionPointAngle, relativeAngle);
        }
    }

    std::sort(intersections.begin(), intersections.end(),
            [] (const Intersection &lhs, const Intersection &rhs)
    {
        return lhs.relativeAngle >= rhs.relativeAngle;
    });

    std::vector<Line> orderedLines;
    std::vector<Point> orderedVertices;
    std::vector<double> orderedAngles;

    int nextIndex;
    double intersectionPointDistance;
    int currentIndex = closestIndex2;

    for (Intersection intersection : intersections) {
        nextIndex = -1;

        if (intersection.line1 == currentIndex) {
            nextIndex = intersection.line2;
        }
        else if (intersection.line2 == currentIndex) {
            nextIndex = intersection.line1;
        }

        if (nextIndex > -1) {
            currentIndex = nextIndex;

            orderedLines.emplace_back(lines[nextIndex]);
            orderedVertices.emplace_back(intersection.intersectionPoint);
            orderedAngles.emplace_back(intersection.intersectionPointAngle);

            intersectionPointDistance = Geometry::distanceFromOrigin(intersection.intersectionPoint);
            if (intersectionPointDistance > outerCircleRadius) {
                outerCircleRadius = intersectionPointDistance;
            }
        }
    }

    pickerGeometry->lines = orderedLines;
    pickerGeometry->vertices = orderedVertices;
    pickerGeometry->angles = orderedAngles;
    pickerGeometry->outerCircleRadius = outerCircleRadius;
    pickerGeometry->innerCircleRadius = closestLineDistance;
}

ColorWheelHSLuv::ColorWheelHSLuv ()
    : _adjusting(false)
    , _scale(1.0)
    , _cache_width(0)
    , _cache_height(0)
    , _square_size(1)
{
    set_name("ColorWheelHSLuv");
    add_events(Gdk::BUTTON_PRESS_MASK	|
	       Gdk::BUTTON_RELEASE_MASK	|
	       Gdk::BUTTON_MOTION_MASK	|
	       Gdk::KEY_PRESS_MASK	);
    set_can_focus();

    _picker_geometry = new PickerGeometry;

    set_hsluv(0.0, 100.0, 50.0);
}

ColorWheelHSLuv::~ColorWheelHSLuv ()
{
    delete _picker_geometry;
}

void ColorWheelHSLuv::set_rgb(double r, double g, double b)
{
    double h, s ,l;
    Hsluv::rgb_to_hsluv(r, g, b, &h, &s, &l);

    set_hue(h);
    set_saturation(s);
    set_lightness(l);
}

void ColorWheelHSLuv::get_rgb(double *r, double *g, double *b) const
{
    Hsluv::hsluv_to_rgb(_hue, _saturation, _lightness, r, g, b);
}

guint32 ColorWheelHSLuv::get_rgb() const
{
    double r, g, b;
    Hsluv::hsluv_to_rgb(_hue, _saturation, _lightness, &r, &g, &b);

    return (
        (static_cast<guint32>(r * 255.0) << 16) |
        (static_cast<guint32>(g * 255.0) <<  8) |
        (static_cast<guint32>(b * 255.0)      )
    );
}

void ColorWheelHSLuv::set_hsluv(double h, double s, double l)
{
    set_hue(h);
    set_saturation(s);
    set_lightness(l);
}

void ColorWheelHSLuv::set_hue(double h)
{
    _hue = std::clamp(h, MIN_HUE, MAX_HUE);
}

void ColorWheelHSLuv::set_saturation(double s)
{
    _saturation = std::clamp(s, MIN_SATURATION, MAX_SATURATION);
}

void ColorWheelHSLuv::set_lightness(double l)
{
    _lightness = std::clamp(l, MIN_LIGHTNESS, MAX_LIGHTNESS);

    // Update polygon
    getPickerGeometry(_picker_geometry, _lightness);
    _scale = OUTER_CIRCLE_RADIUS / _picker_geometry->outerCircleRadius;
    update_polygon();

    queue_draw();
}

void ColorWheelHSLuv::get_hsluv(double *h, double *s, double *l) const
{
    if (h) *h = _hue;
    if (s) *s = _saturation;
    if (l) *l = _lightness;
}

bool ColorWheelHSLuv::on_draw(const::Cairo::RefPtr<::Cairo::Context>& cr)
{
    if (_lightness < 1e-4 || _lightness > 99.9999) {
        return true;
    }

    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();
    const double resize = std::min(width, height) / static_cast<double>(SIZE);

    const int cx = width/2;
    const int cy = height/2;

    const int marginX = std::max(0.0, (width - height) / 2.0);
    const int marginY = std::max(0.0, (height - width) / 2.0);

    std::vector<Point> shapePointsPixel =
        toPixelCoordinate(_picker_geometry->vertices, _scale, resize);
    for (Point &point : shapePointsPixel) {
        point.x += marginX;
        point.y += marginY;
    }

    cr->set_antialias(Cairo::ANTIALIAS_SUBPIXEL);

    if (width > _square_size && height > _square_size) {
        if (_cache_width != width || _cache_height != height) {
            update_polygon();
        }

        // Paint with surface, clipping to polygon
        cr->save();
        cr->set_source(_surface_polygon, 0, 0);
        cr->move_to(shapePointsPixel[0].x, shapePointsPixel[0].y);
        for (int i = 1; i < shapePointsPixel.size(); i++) {
            const Point &point = shapePointsPixel[i];
            cr->line_to(point.x, point.y);
        }
        cr->close_path();
        cr->fill();
        cr->restore();
    }

    // Draw foreground

    // Outer circle
    std::vector<double> dashes{OUTER_CIRCLE_DASH_SIZE};
    cr->set_line_width(1);
    // White dashes
    cr->set_source_rgb(1.0, 1.0, 1.0);
    cr->set_dash(dashes, 0.0);
    cr->begin_new_path();
    cr->arc(cx, cy, _scale * resize * _picker_geometry->outerCircleRadius, 0, 2 * M_PI);
    cr->stroke();
    // Black dashes
    cr->set_source_rgb(0.0, 0.0, 0.0);
    cr->set_dash(dashes, OUTER_CIRCLE_DASH_SIZE);
    cr->begin_new_path();
    cr->arc(cx, cy, _scale * resize * _picker_geometry->outerCircleRadius, 0, 2 * M_PI);
    cr->stroke();
    cr->unset_dash();

    // Contrast
    double a = (_lightness > 70.0) ? 0.0 : 1.0;
    cr->set_source_rgb(a, a, a);

    // Pastel circle
    cr->set_line_width(2);
    cr->begin_new_path();
    cr->arc(cx, cy, _scale * resize * _picker_geometry->innerCircleRadius, 0, 2 * M_PI);
    cr->stroke();

    // Center
    cr->begin_new_path();
    cr->arc(cx, cy, 2, 0, 2 * M_PI);
    cr->fill();

    // Draw marker
    double l, u, v;
    Hsluv::hsluv_to_luv(_hue, _saturation, _lightness, &l, &u, &v);
    Point mp = toPixelCoordinate(Point(u, v), _scale, resize);
    mp.x += marginX;
    mp.y += marginY;

    cr->set_line_width(2);
    cr->begin_new_path();
    cr->arc(mp.x, mp.y, 4, 0, 2 * M_PI);
    cr->stroke();

    // Focus
    if (has_focus()) {
        Glib::RefPtr<Gtk::StyleContext> style_context = get_style_context();
        style_context->render_focus(cr, mp.x-4, mp.y-4, 8, 8);

        cr->set_line_width(0.5);
        cr->set_source_rgb(1-a, 1-a, 1-a);
        cr->begin_new_path();
        cr->arc(mp.x, mp.y, 7, 0, 2 * M_PI);
        cr->stroke();
    }

    return true;
}

void ColorWheelHSLuv::set_from_xy(const double x, const double y)
{
    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();

    const double resize = std::min(width, height) / static_cast<double>(SIZE);

    const int margin_x = std::max(0.0, (width - height) / 2.0);
    const int margin_y = std::max(0.0, (height - width) / 2.0);

    const Point p = fromPixelCoordinate(Point(
        x - margin_x,
        y - margin_y
    ), _scale, resize);

    double h, s, l;
    Hsluv::luv_to_hsluv(_lightness, p.x, p.y, &h, &s, &l);

    set_hue(h);
    set_saturation(s);

    _signal_color_changed.emit();
    queue_draw();
}

void ColorWheelHSLuv::update_polygon()
{
    if (_lightness < 1e-4 || _lightness > 99.9999) {
        return;
    }

    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();
    const int size = std::min(width, height);

    // Update square size
    _square_size = std::max(1, static_cast<int>(size/50));

    if (width < _square_size || height < _square_size) {
        return;
    }

    _cache_width = width;
    _cache_height = height;

    const double resize = size / static_cast<double>(SIZE);

    const int marginX = std::max(0.0, (width - height) / 2.0);
    const int marginY = std::max(0.0, (height - width) / 2.0);

    std::vector<Point> shapePointsPixel =
        toPixelCoordinate(_picker_geometry->vertices, _scale, resize);

    for (Point &point : shapePointsPixel) {
        point.x += marginX;
        point.y += marginY;
    }

    std::vector<double> xs;
    std::vector<double> ys;

    for (const Point &point : shapePointsPixel) {
        xs.emplace_back(point.x);
        ys.emplace_back(point.y);
    }

    const int xmin =
        std::floor(*std::min_element(xs.begin(), xs.end()) / _square_size);
    const int ymin =
        std::floor(*std::min_element(ys.begin(), ys.end()) / _square_size);
    const int xmax =
        std::ceil(*std::max_element(xs.begin(), xs.end()) / _square_size);
    const int ymax =
        std::ceil(*std::max_element(ys.begin(), ys.end()) / _square_size);

    const int stride =
        Cairo::ImageSurface::format_stride_for_width(Cairo::FORMAT_RGB24, width);

    _buffer_polygon.resize(height * stride / 4);
    std::vector<guint32> buffer_line;
    buffer_line.resize(stride / 4);

    color_point clr;

    // Set the color of each pixel/square
    for (int y = ymin; y < ymax; y++) {
        for (int x = xmin; x < xmax; x++) {
            double px = x * _square_size;
            double py = y * _square_size;
            Point point = fromPixelCoordinate(Point(
                px + (_square_size / 2) - marginX,
                py + (_square_size / 2) - marginY
            ), _scale, resize);

            double r, g ,b;
            Hsluv::luv_to_rgb(_lightness, point.x, point.y, &r, &g, &b);

            r = std::clamp(r, 0.0, 1.0);
            g = std::clamp(g, 0.0, 1.0);
            b = std::clamp(b, 0.0, 1.0);

            clr.set_color(r, g, b);

            guint32 *p = buffer_line.data() + (x * _square_size);
            for (int i = 0; i < _square_size; i++) {
                p[i] = clr.get_color();
            }
        }

        // Copy the line buffer to the surface buffer
        int Y = y * _square_size;
        for (int i = 0; i < _square_size; i++) {
            guint32 *t = _buffer_polygon.data() + (Y + i) * (stride / 4);
            std::memcpy(t, buffer_line.data(), stride);
        }
    }

    _surface_polygon = ::Cairo::ImageSurface::create(
        reinterpret_cast<unsigned char *>(_buffer_polygon.data()),
        Cairo::FORMAT_RGB24, width, height, stride
    );
}

bool ColorWheelHSLuv::on_button_press_event(GdkEventButton* event)
{
    const double x = event->x;
    const double y = event->y;

    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();

    const int margin_x = std::max(0.0, (width - height) / 2.0);
    const int margin_y = std::max(0.0, (height - width) / 2.0);
    const int size = std::min(width, height);

    if (x > margin_x && x < (margin_x+size) && y > margin_y && y < (margin_y+size)) {
        _adjusting = true;
        grab_focus();
        set_from_xy(x, y);
        return true;
    }

    return false;
}

bool ColorWheelHSLuv::on_button_release_event(GdkEventButton */*event*/)
{
    _adjusting = false;
    return true;
}

bool ColorWheelHSLuv::on_motion_notify_event(GdkEventMotion* event)
{
    if (!_adjusting) {
        return false;
    }

    double x = event->x;
    double y = event->y;

    set_from_xy(x, y);

    return true;
}

bool ColorWheelHSLuv::on_key_press_event(GdkEventKey* key_event)
{
    bool consumed = false;

    unsigned int key = 0;
    gdk_keymap_translate_keyboard_state( Gdk::Display::get_default()->get_keymap(),
                                         key_event->hardware_keycode,
                                         (GdkModifierType)key_event->state,
                                         0, &key, nullptr, nullptr, nullptr );

    // Get current point
    double l, u, v;
    Hsluv::hsluv_to_luv(_hue, _saturation, _lightness, &l, &u, &v);

    const double marker_move = 1.0 / _scale;

    switch (key) {
        case GDK_KEY_Up:
        case GDK_KEY_KP_Up:
            v += marker_move;
            consumed = true;
            break;

        case GDK_KEY_Down:
        case GDK_KEY_KP_Down:
            v -= marker_move;
            consumed = true;
            break;

        case GDK_KEY_Left:
        case GDK_KEY_KP_Left:
            u -= marker_move;
            consumed = true;
            break;

        case GDK_KEY_Right:
        case GDK_KEY_KP_Right:
            u += marker_move;
            consumed = true;
            break;
    }

    if (consumed) {
        double h, s, l;
        Hsluv::luv_to_hsluv(_lightness, u, v, &h, &s, &l);

        set_hue(h);
        set_saturation(s);

        _adjusting = true;
        _signal_color_changed.emit();
        queue_draw();
    }

    return consumed;
}

bool ColorWheelHSLuv::on_key_release_event(GdkEventKey* key_event)
{
    unsigned int key = 0;
    gdk_keymap_translate_keyboard_state( Gdk::Display::get_default()->get_keymap(),
                                         key_event->hardware_keycode,
                                         (GdkModifierType)key_event->state,
                                         0, &key, nullptr, nullptr, nullptr );

    switch (key) {
        case GDK_KEY_Up:
        case GDK_KEY_KP_Up:
        case GDK_KEY_Down:
        case GDK_KEY_KP_Down:
        case GDK_KEY_Left:
        case GDK_KEY_KP_Left:
        case GDK_KEY_Right:
        case GDK_KEY_KP_Right:
            _adjusting = false;
            return true;
    }

    return false;
}

sigc::signal<void> ColorWheelHSLuv::signal_color_changed()
{
    return _signal_color_changed;
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape
