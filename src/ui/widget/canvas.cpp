// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Author:
 *   Tavmjong Bah
 *
 * Rewrite of code originally in sp-canvas.cpp.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <iostream>
#include <algorithm>

#include <glibmm/i18n.h>

#include <2geom/rect.h>

#include "canvas.h"
#include "canvas-grid.h"

#include "color.h"          // Background color
#include "cms-system.h"     // Color correction
#include "desktop.h"
#include "preferences.h"

#include "display/cairo-utils.h"     // Checkerboard background.
#include "display/drawing.h"
#include "display/control/canvas-item-group.h"
#include "display/control/snap-indicator.h"

#include "ui/tools/tool-base.h"      // Default cursor

/*
 *   The canvas is responsible for rendering the SVG drawing with various "control"
 *   items below and on top of the drawing. Rendering is triggered by a call to one of:
 *
 *
 *   * redraw_all()     Redraws the entire canvas by calling redraw_area() with the canvas area.
 *
 *   * redraw_area()    Redraws the indicated area. Use when there is a change that doesn't effect
 *                      a CanvasItem's geometry or size.
 *
 *   * request_update() Redraws after recalculating bounds for changed CanvasItem's. Use if a
 *                      CanvasItem's geometry or size has changed.
 *
 *   * redraw_now()     Redraw immediately, skipping the "idle" stage.
 *
 *   The first three functions add a request to the Gtk's "idle" list via
 *
 *   * add_idle()       Which causes Gtk to call when resources are available:
 *
 *   * on_idle()        Which calls:
 *
 *   * do_update()      Which makes a few checks and then calls:
 *
 *   * paint()          Which calls for each area of the canvas that has been marked unclean:
 *
 *   * paint_rect()     Which determines the maximum area to draw at once and where the cursor is, then calls:
 *
 *   * paint_rect_internal()  Which recursively divides the area into smaller pieces until a piece is small
 *                            enough to render. It renders the pieces closest to the cursor first. The pieces
 *                            are rendered onto a Cairo surface "backing_store". After a piece is rendered
 *                            there is a call to:
 *
 *   * queue_draw_area() A Gtk function for drawing into a widget which when the time is right calls:
 *
 *   * on_draw()        Which blits the Cairo surface to the screen.
 *
 *   One thing to note is that on_draw() must be called twice to render anything to the screen, as the
 *   first time through it sets up the backing store which must then be drawn to. The second call then
 *   blits the backing store to the screen. It might be better to setup the backing store on a call
 *   to on_allocate() but it is what works now.
 *
 *   The other responsibility of the canvas is to determine where to send GUI events. It does this
 *   by determining which CanvasItem is "picked" and then forwards the events to that item. Not all
 *   items can be picked. As a last resort, the "CatchAll" CanvasItem will be picked as it is the
 *   lowest CanvasItem in the stack (except for the "root" CanvasItem). With a small be of work, it
 *   should be possible to make the "root" CanvasItem a "CatchAll" eliminating the need for a
 *   dedicated "CatchAll" CanvasItem. There probably could be efficiency improvements as some
 *   items that are not pickable probably should be which would save having to effectively pick
 *   them "externally" (e.g. gradient CanvasItemCurves).
 */

struct PaintRectSetup {
    gint64 start_time;
    Geom::IntRect canvas_rect;
    int max_pixels;
    Geom::Point mouse_loc;
    bool disable_timeouts;
};

namespace Inkscape {
namespace UI {
namespace Widget {


Canvas::Canvas()
    : _size_observer(this, "/options/grabsize/value")
{
    set_name("InkscapeCanvas");

    // Events
    add_events(Gdk::BUTTON_PRESS_MASK   |
               Gdk::BUTTON_RELEASE_MASK |
               Gdk::ENTER_NOTIFY_MASK   |
               Gdk::LEAVE_NOTIFY_MASK   |
               Gdk::FOCUS_CHANGE_MASK   |
               Gdk::KEY_PRESS_MASK      |
               Gdk::KEY_RELEASE_MASK    |
               Gdk::POINTER_MOTION_MASK |
               Gdk::SCROLL_MASK         |
               Gdk::SMOOTH_SCROLL_MASK  );

    // Give _pick_event an initial definition.
    _pick_event.type = GDK_LEAVE_NOTIFY;
    _pick_event.crossing.x = 0;
    _pick_event.crossing.y = 0;

    // Drawing
    _clean_region = Cairo::Region::create();

    _background = Cairo::SolidPattern::create_rgb(1.0, 1.0, 1.0);

    _canvas_item_root = new Inkscape::CanvasItemGroup(nullptr);
    _canvas_item_root->set_name("CanvasItemGroup:Root");
    _canvas_item_root->set_canvas(this);

    srand(g_get_monotonic_time());
}

Canvas::~Canvas()
{
    assert(!_desktop);

    _drawing = nullptr;
    _in_destruction = true;

    // Remove entire CanvasItem tree.
    delete _canvas_item_root;
}

/**
 * Is world point inside canvas area?
 */
bool
Canvas::world_point_inside_canvas(Geom::Point const &world)
{
    Gtk::Allocation allocation = get_allocation();
    return ( _x0 <= world.x() && world.x() < _x0 + allocation.get_width() &&
             _y0 <= world.y() && world.y() < _y0 + allocation.get_height() );
}

/**
 * Translate point in canvas to world coordinates.
 */
Geom::Point
Canvas::canvas_to_world(Geom::Point const &point)
{
    return Geom::Point(point[Geom::X]+ _x0, point[Geom::Y] + _y0);
}

/**
 * Return the area shown in the canvas in world coordinates.
 */
Geom::IntRect
Canvas::get_area_world()
{
    Gtk::Allocation allocation = get_allocation();
    return Geom::IntRect::from_xywh(_x0, _y0, allocation.get_width(), allocation.get_height());
}

/**
 * Set the affine for the canvas and flag need for geometry update.
 */
void
Canvas::set_affine(Geom::Affine const &affine)
{
    if (_affine != affine) {
        _affine = affine;
        _need_update = true;
    }
}

/**
 * Invalidate drawing and redraw during idle.
 */
void
Canvas::redraw_all()
{
    if (_in_destruction) {
        // CanvasItems redraw their area when being deleted... which happens when the Canvas is destroyed.
        // We need to ignore their requests!
        return;
    }
    _in_full_redraw = true;
    _clean_region = Cairo::Region::create(); // Empty region (i.e. everything is dirty).
    add_idle();
}

/**
 * Redraw the given area during idle.
 */
void
Canvas::redraw_area(int x0, int y0, int x1, int y1)
{
    // std::cout << "Canvas::redraw_area: "
    //           << " x0: " << x0
    //           << " y0: " << y0
    //           << " x1: " << x1
    //           << " y1: " << y1 << std::endl;
    if (_in_destruction) {
        // CanvasItems redraw their area when being deleted... which happens when the Canvas is destroyed.
        // We need to ignore their requests!
        return;
    }

    if (x0 >= x1 || y0 >= y1) {
        return;
    }

    // Clamp area to Cairo's technically supported max size (-2^30..+2^30-1).
    // This ensures that the rectangle dimensions don't overflow and wrap around.

    constexpr int min_coord = std::numeric_limits<int>::min() / 2;
    constexpr int max_coord = std::numeric_limits<int>::max() / 2;

    x0 = std::clamp(x0, min_coord, max_coord);
    y0 = std::clamp(y0, min_coord, max_coord);
    x1 = std::clamp(x1, min_coord, max_coord);
    y1 = std::clamp(y1, min_coord, max_coord);

    Cairo::RectangleInt crect = { x0, y0, x1-x0, y1-y0 };
    _clean_region->subtract(crect);
    add_idle();
}

void
Canvas::redraw_area(Geom::Coord x0, Geom::Coord y0, Geom::Coord x1, Geom::Coord y1)
{
    // Handle overflow during conversion gracefully.
    // Round outward to make sure integral coordinates cover the entire area.

    constexpr Geom::Coord min_int = static_cast<Geom::Coord>(std::numeric_limits<int>::min());
    constexpr Geom::Coord max_int = static_cast<Geom::Coord>(std::numeric_limits<int>::max());

    redraw_area(
        static_cast<int>(std::floor(std::clamp(x0, min_int, max_int))),
        static_cast<int>(std::floor(std::clamp(y0, min_int, max_int))),
        static_cast<int>(std::ceil (std::clamp(x1, min_int, max_int))),
        static_cast<int>(std::ceil (std::clamp(y1, min_int, max_int)))
    );
}

void
Canvas::redraw_area(Geom::Rect& area)
{
    redraw_area(area.left(), area.top(), area.right(), area.bottom());
}

/**
 * Redraw after changing canvas item geometry.
 */
void
Canvas::request_update()
{
    _need_update = true;
    add_idle(); // Geometry changed, need to redraw.
}

/**
 * This is the first function called (after constructor) for Inkscape (not Inkview).
 * Scroll window so drawing point 'c' is at upper left corner of canvas.
 */
void
Canvas::scroll_to(Geom::Point const &c)
{
    int x = (int) std::round(c[Geom::X]);
    int y = (int) std::round(c[Geom::Y]);

    if (x == _x0 && y == _y0) {
        return;
    }

    _x0 = x;
    _y0 = y;

    add_idle();
    queue_draw();

    if (auto grid = dynamic_cast<Inkscape::UI::Widget::CanvasGrid*>(get_parent())) {
        grid->UpdateRulers();
    }
}

/**
 * Set canvas background color (display only).
 */
void
Canvas::set_background_color(guint32 rgba)
{
    double r = SP_RGBA32_R_F(rgba);
    double g = SP_RGBA32_G_F(rgba);
    double b = SP_RGBA32_B_F(rgba);

    _background = Cairo::SolidPattern::create_rgb(r, g, b);

    redraw_all();
}

/**
 * Set canvas background to a checkerboard pattern.
 */
void
Canvas::set_background_checkerboard(guint32 rgba)
{
    auto pattern = ink_cairo_pattern_create_checkerboard(rgba);
    _background = Cairo::RefPtr<Cairo::Pattern>(new Cairo::Pattern(pattern));
    redraw_all();
}

void
Canvas::set_render_mode(Inkscape::RenderMode mode)
{
    if (_render_mode != mode) {
        _render_mode = mode;
        redraw_all();
    }
    if (_desktop) {
        _desktop->setWindowTitle(); // Mode is listed in title.
    }
}

void
Canvas::set_color_mode(Inkscape::ColorMode mode)
{
    if (_color_mode != mode) {
        _color_mode = mode;
        redraw_all();
    }
    if (_desktop) {
        _desktop->setWindowTitle(); // Mode is listed in title.
    }
}

void
Canvas::set_split_mode(Inkscape::SplitMode mode)
{
    if (_split_mode != mode) {
        _split_mode = mode;
        redraw_all();
    }
}

void
Canvas::set_split_direction(Inkscape::SplitDirection dir)
{
    if (_split_direction != dir) {
        _split_direction = dir;
        redraw_all();
    }
}

void
Canvas::forced_redraws_start(int count, bool reset)
{
    _forced_redraw_limit = count;
    if (reset) {
        _forced_redraw_count = 0;
    }
}

/**
 * Clear current and grabbed items.
 */
void
Canvas::canvas_item_clear(Inkscape::CanvasItem* item)
{
    if (item == _current_canvas_item) {
        _current_canvas_item = nullptr;
        _need_repick = true;
    }

    if (item == _current_canvas_item_new) {
        _current_canvas_item_new = nullptr;
        _need_repick = true;
    }

    if (item == _grabbed_canvas_item) {
        _grabbed_canvas_item = nullptr;
        auto const display = Gdk::Display::get_default();
        auto const seat    = display->get_default_seat();
        seat->ungrab();
    }
}

// ============== Protected Functions ==============

void
Canvas::get_preferred_width_vfunc(int &minimum_width,  int &natural_width) const
{
    minimum_width = natural_width = 256;
} 	

void
Canvas::get_preferred_height_vfunc(int &minimum_height, int &natural_height) const
{
    minimum_height = natural_height = 256;
}

// ******* Event handlers ******
bool
Canvas::on_scroll_event(GdkEventScroll *scroll_event)
{
    // Scroll canvas and in Select Tool, cycle selection through objects under cursor.
    return emit_event(reinterpret_cast<GdkEvent*>(scroll_event));
}

// Our own function that combines press and release.
bool
Canvas::on_button_event(GdkEventButton *button_event)
{
    // Dispatch normally regardless of the event's window if an item
    // has a pointer grab in effect.
    auto window = get_window();
    if (!_grabbed_canvas_item && window->gobj() != button_event->window) {
        return false;
    }

    int mask = 0;
    switch (button_event->button) {
        case 1:  mask = GDK_BUTTON1_MASK; break;
        case 2:  mask = GDK_BUTTON1_MASK; break;
        case 3:  mask = GDK_BUTTON1_MASK; break;
        case 4:  mask = GDK_BUTTON1_MASK; break;
        case 5:  mask = GDK_BUTTON1_MASK; break;
        default: mask = 0;  // Buttons can range at least to 9 but mask defined only to 5.
    }

    bool retval = false;
    switch (button_event->type) {
        case GDK_BUTTON_PRESS:

            if (_hover_direction != Inkscape::SplitDirection::NONE) {
                // We're hovering over Split controller.
                _split_dragging = true;
                _split_drag_start = Geom::Point(button_event->x, button_event->y);
                break;
            }
            // Fallthrough

        case GDK_2BUTTON_PRESS:

            if (_hover_direction != Inkscape::SplitDirection::NONE) {
                _split_direction = _hover_direction;
                _split_dragging = false;
                queue_draw();
                break;
            }
            // Fallthrough

        case GDK_3BUTTON_PRESS:
            // Pick the current item as if the button were not pressed and then process event.

            _state = button_event->state;
            pick_current_item(reinterpret_cast<GdkEvent *>(button_event));
            _state ^= mask;
            retval = emit_event(reinterpret_cast<GdkEvent *>(button_event));
            break;

        case GDK_BUTTON_RELEASE:
            // Process the event as if the button were pressed, then repick after the button has
            // been released.
            _split_dragging = false;

            _state = button_event->state;
            retval = emit_event(reinterpret_cast<GdkEvent *>(button_event));
            button_event->state ^= mask;
            _state = button_event->state;
            pick_current_item(reinterpret_cast<GdkEvent *>(button_event));
            button_event->state ^= mask;
            break;

        default:
            std::cerr << "Canvas::on_button_event: illegal event type!" << std::endl;
    }

    return retval;
}

bool
Canvas::on_button_press_event(GdkEventButton *button_event)
{
    return on_button_event(button_event);
}

bool
Canvas::on_button_release_event(GdkEventButton *button_event)
{
    return on_button_event(button_event);
}

bool
Canvas::on_enter_notify_event(GdkEventCrossing *crossing_event)
{
    auto window = get_window();
    if (window->gobj() != crossing_event->window) {
        std::cout << "  WHOOPS... this does really happen" << std::endl;
        return false;
    }
    _state = crossing_event->state;
    return pick_current_item(reinterpret_cast<GdkEvent *>(crossing_event));
}

bool
Canvas::on_leave_notify_event(GdkEventCrossing *crossing_event)
{
    auto window = get_window();
    if (window->gobj() != crossing_event->window) {
        std::cout << "  WHOOPS... this does really happen" << std::endl;
        return false;
    }
    _state = crossing_event->state;
    // this is needed to remove alignment or distribution snap indicators
    if (_desktop)
        _desktop->snapindicator->remove_snaptarget();
    return pick_current_item(reinterpret_cast<GdkEvent *>(crossing_event));
}

bool
Canvas::on_focus_in_event(GdkEventFocus *focus_event)
{
    grab_focus();
    return false;
}

// TODO See if we still need this (canvas->focused_item removed between 0.48 and 0.91).
bool
Canvas::on_focus_out_event(GdkEventFocus *focus_event)
{
    return false;
}

// Actually, key events never reach here.
bool
Canvas::on_key_press_event(GdkEventKey *key_event)
{
    return emit_event(reinterpret_cast<GdkEvent *>(key_event));
}

// Actually, key events never reach here.
bool
Canvas::on_key_release_event(GdkEventKey *key_event)
{
    return emit_event(reinterpret_cast<GdkEvent *>(key_event));
}

bool
Canvas::on_motion_notify_event(GdkEventMotion *motion_event)
{
    Geom::IntPoint cursor_position = Geom::IntPoint(motion_event->x, motion_event->y);

    if (_desktop) {
    // Check if we are near the edge. If so, revert to normal mode.
    if (_split_mode == Inkscape::SplitMode::SPLIT && _split_dragging) {
        if (cursor_position.x() < 5                                  ||
            cursor_position.y() < 5                                  ||
            cursor_position.x() - get_allocation().get_width()  > -5 ||
            cursor_position.y() - get_allocation().get_height() > -5 ) {

            // Reset everything.
            _split_mode = Inkscape::SplitMode::NORMAL;
            _split_position = Geom::Point(-1, -1);
            _hover_direction = Inkscape::SplitDirection::NONE;
            set_cursor();
            queue_draw();

            // Update action (turn into utility function?).
            auto window = dynamic_cast<Gtk::ApplicationWindow *>(get_toplevel());
            if (!window) {
                std::cerr << "Canvas::on_motion_notify_event: window missing!" << std::endl;
                return true;
            }

            auto action = window->lookup_action("canvas-split-mode");
            if (!action) {
                std::cerr << "Canvas::on_motion_notify_event: action 'canvas-split-mode' missing!" << std::endl;
                return true;
            }

            auto saction = Glib::RefPtr<Gio::SimpleAction>::cast_dynamic(action);
            if (!saction) {
                std::cerr << "Canvas::on_motion_notify_event: action 'canvas-split-mode' not SimpleAction!" << std::endl;
                return true;
            }

            saction->change_state((int)Inkscape::SplitMode::NORMAL);

            return true;
        }
    }

    if (_split_mode == Inkscape::SplitMode::XRAY) {
        _split_position = cursor_position;
        queue_draw(); // Re-blit
    }

    if (_split_mode == Inkscape::SplitMode::SPLIT) {

        Inkscape::SplitDirection hover_direction = Inkscape::SplitDirection::NONE;
        Geom::Point difference(cursor_position - _split_position);

        // Move controller
        if (_split_dragging) {
            Geom::Point delta = cursor_position - _split_drag_start; // We don't use _split_position
            if (_hover_direction == Inkscape::SplitDirection::HORIZONTAL) {
                _split_position += Geom::Point(0, delta.y());
            } else if (_hover_direction == Inkscape::SplitDirection::VERTICAL) {
                _split_position += Geom::Point(delta.x(), 0);
            } else {
                _split_position += delta;
            }
            _split_drag_start = cursor_position;
            queue_draw();
            return true;
        }

        if (Geom::distance(cursor_position, _split_position) < 20 * _device_scale) {

            // We're hovering over circle, figure out which direction we are in.
            if (difference.y() - difference.x() > 0) {
                if (difference.y() + difference.x() > 0) {
                    hover_direction = Inkscape::SplitDirection::SOUTH;
                } else {
                    hover_direction = Inkscape::SplitDirection::WEST;
                }
            } else {
                if (difference.y() + difference.x() > 0) {
                    hover_direction = Inkscape::SplitDirection::EAST;
                } else {
                    hover_direction = Inkscape::SplitDirection::NORTH;
                }
            }
        } else if (_split_direction == Inkscape::SplitDirection::NORTH ||
                   _split_direction == Inkscape::SplitDirection::SOUTH) {
            if (std::abs(difference.y()) < 3 * _device_scale) {
                // We're hovering over horizontal line
                hover_direction = Inkscape::SplitDirection::HORIZONTAL;
            }
        } else {
            if (std::abs(difference.x()) < 3 * _device_scale) {
               // We're hovering over vertical line
                hover_direction = Inkscape::SplitDirection::VERTICAL;
            }
        }

        if (_hover_direction != hover_direction) {
            _hover_direction = hover_direction;
            set_cursor();
            queue_draw();
        }

        if (_hover_direction != Inkscape::SplitDirection::NONE) {
            // We're hovering, don't pick or emit event.
            return true;
        }
    }
    } // End if(desktop)

    _state = motion_event->state;
    pick_current_item(reinterpret_cast<GdkEvent*>(motion_event));
    bool status = emit_event(reinterpret_cast<GdkEvent*>(motion_event));
    return status;
}

/*
 * The on_draw() function is called whenever Gtk wants to update the window. This function:
 *
 * 1. Sets up the backing and outline stores (images). These stores are drawn to elsewhere during idles.
 *    The backing store is always uses, rendering in which ever "render mode" the user has selected.
 *    The outline store is only used when the "split mode" is set to 'split' or 'x-ray'.
 *    (Changing either the render mode or split mode results in a complete redrawing the store(s).)
 *
 * 2. Calls shift_content() if the drawing area has changed.
 *
 * 3. Blits the store(s) onto the canvas, clipping the outline store as required.
 *
 * 4. Draws the "controller" in the 'split' split mode.
 *
 * 5. Calls add_idle() to update the drawing if necessary.
 */
bool
Canvas::on_draw(const::Cairo::RefPtr<::Cairo::Context> &cr)
{
    // sp_canvas_item_recursive_print_tree(0, _root);
    // canvas_item_print_tree(_canvas_item_root);

    std::cout << "on_draw\n";

    assert(_backing_store/* && _outline_store*/);
    assert(_drawing);

    // todo: can minutely optimise this by only running the first part of on_idle()
    on_idle();

    // Blit background (e.g. checkerboard).
    cr->save();
    cr->set_operator(Cairo::OPERATOR_SOURCE);
    cr->set_source(_background);
    cr->paint();
    cr->restore();
    // todo: remember the solid-colour optimisation to go into this

    // Blit from the backing store, without regard for the clean region.
    cr->set_source(_backing_store, _store_rect.left() - _x0, _store_rect.top() - _y0);
    cr->paint();

    // Paint unclean regions in red
    auto reg = Cairo::Region::create( Cairo::RectangleInt{ _x0, _y0, get_allocation().get_width(), get_allocation().get_height() } );
    reg->subtract(_clean_region);

    cr->set_source_rgba(1, 0, 0, 0.07);
    for (int i = 0; i < reg->get_num_rectangles(); i++)
    {
        auto rect = reg->get_rectangle(i);
        cr->rectangle(rect.x - _x0, rect.y - _y0, rect.width, rect.height);
        cr->fill();
    }

/*
    // Draw overlay if required.
    if (_drawing->outlineOverlay()) {

        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        double outline_overlay_opacity = 1 - (prefs->getIntLimited("/options/rendering/outline-overlay-opacity", 50, 1, 100) / 100.0);

        // Partially obscure drawing by painting semi-transparent white.
        cr->set_source_rgb(255,255,255);
        cr->paint_with_alpha(outline_overlay_opacity);

        // Overlay outline
        cr->set_source(_outline_store, 0, 0);
        cr->paint();
    }

    // Draw split if required.
    if (_split_mode != Inkscape::SplitMode::NORMAL) {

        // Move split position to center if not in canvas.
        auto const rect = Geom::Rect(0, 0, _width, _height);
        if (!rect.contains(_split_position)) {
            _split_position = rect.midpoint();
        }

        // Add clipping path and blit background.
        cr->save();
        cr->set_operator(Cairo::OPERATOR_SOURCE);
        cr->set_source(_background);
        add_clippath(cr);
        cr->paint();
        cr->restore();

        // Add clipping path and blit outline store.
        cr->save();
        cr->set_source(_outline_store, 0, 0);
        add_clippath(cr);
        cr->paint();
        cr->restore();
    }

    if (_split_mode == Inkscape::SplitMode::SPLIT) {

        // Add dividing line.
        cr->save();
        cr->set_source_rgb(0, 0, 0);
        cr->set_line_width(1);
        if (_split_direction == Inkscape::SplitDirection::EAST ||
            _split_direction == Inkscape::SplitDirection::WEST) {
            cr->move_to((int)_split_position.x() + 0.5,                        0);
            cr->line_to((int)_split_position.x() + 0.5, _allocation.get_height());
            cr->stroke();
        } else {
            cr->move_to(                      0, (int)_split_position.y() + 0.5);
            cr->line_to(_allocation.get_width(), (int)_split_position.y() + 0.5);
            cr->stroke();
        }
        cr->restore();

        // Add controller image.
        double a = _hover_direction == Inkscape::SplitDirection::NONE ? 0.5 : 1.0;
        cr->save();
        cr->set_source_rgba(0.2, 0.2, 0.2, a);
        cr->arc(_split_position.x(), _split_position.y(), 20 * _device_scale, 0, 2 * M_PI);
        cr->fill();
        cr->restore();

        cr->save();
        for (int i = 0; i < 4; ++i) {
            // The four direction triangles.
            cr->save();

            // Position triangle.
            cr->translate(_split_position.x(), _split_position.y());
            cr->rotate((i+2)*M_PI/2.0);

            // Draw triangle.
            cr->move_to(-5 * _device_scale,  8 * _device_scale);
            cr->line_to( 0,                 18 * _device_scale);
            cr->line_to( 5 * _device_scale,  8 * _device_scale);
            cr->close_path();

            double b = (int)_hover_direction == (i+1) ? 0.9 : 0.7;
            cr->set_source_rgba(b, b, b, a);
            cr->fill();

            cr->restore();
        }
        cr->restore();
    }*/

    // static int i = 0;
    // ++i;
    // std::string file = "on_draw_" + std::to_string(i) + ".png";
    // _backing_store->write_to_png(file);
/*
    // This whole section is just to determine if we call add_idle!
    auto dirty_region = Cairo::Region::create();

    std::vector<Cairo::Rectangle> clip_rectangles;
    cr->copy_clip_rectangle_list(clip_rectangles);
    for (auto & rectangle : clip_rectangles) {
        Geom::Rect dr = Geom::Rect::from_xywh(rectangle.x + _x0,
                                              rectangle.y + _y0,
                                              rectangle.width,
                                              rectangle.height);
        // "rectangle" is floating point, we must convert to integer. We round outward as it's
        // better to have a larger dirty region to avoid artifacts.
        Geom::IntRect ir = dr.roundOutwards();
        Cairo::RectangleInt irect = { ir.left(), ir.top(), ir.width(), ir.height() };
        dirty_region->do_union(irect);
    }

    dirty_region->subtract(_clean_region);
    
    if (!dirty_region->empty()) {
        add_idle();
    }*/

    return true;
}

void
Canvas::update_canvas_item_ctrl_sizes(int size_index)
{
    _canvas_item_root->update_canvas_item_ctrl_sizes(size_index);
}

void
Canvas::add_idle()
{
    if (_in_destruction) {
        std::cerr << "Canvas::add_idle: Called after canvas destroyed!" << std::endl;
        return;
    }

    if (!_idle_connection.connected()) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        guint redrawPriority = prefs->getIntLimited("/options/redrawpriority/value", G_PRIORITY_HIGH_IDLE, G_PRIORITY_HIGH_IDLE, G_PRIORITY_DEFAULT_IDLE);
        if (_in_full_redraw) {
            _in_full_redraw = false;
            redrawPriority = G_PRIORITY_DEFAULT_IDLE;
        }
        // G_PRIORITY_HIGH_IDLE = 100, G_PRIORITY_DEFAULT_IDLE = 200: Higher number => lower priority.

        // TEMP HACK: the high priority idle screws with my redraw implementation, so ignoring it for now
        _idle_connection = Glib::signal_idle().connect(sigc::mem_fun(*this, &Canvas::on_idle), G_PRIORITY_DEFAULT_IDLE);
    }
}

auto
geom_to_cairo(Geom::IntRect rect)
{
    return Cairo::RectangleInt { rect.left(), rect.top(), rect.width(), rect.height() };
}

auto
cairo_to_geom(Cairo::RectangleInt rect)
{
    return Geom::IntRect::from_xywh(rect.x, rect.y, rect.width, rect.height);
}

auto
distSq(const Geom::IntPoint pt, const Geom::IntRect &rect)
{
    auto v = rect.clamp(pt) - pt;
    return v.x() * v.x() + v.y() * v.y();
}

bool
Canvas::on_idle()
{
    if (_in_destruction) {
        std::cerr << "Canvas::on_idle: Called after canvas destroyed!" << std::endl;
    }

    // Quit idle process if not supposed to be drawing
    if (!_drawing || _drawing_disabled) {
        return false;
    }

    // Ensure geometry is up to date
    assert(_canvas_item_root);
    if (_need_update) {
        _canvas_item_root->update(_affine);
        _need_update = false;
    }

    // Get canvas rectangle in world coordinates
    const auto canvas_rect = Geom::IntRect::from_xywh( _x0, _y0, get_allocation().get_width(), get_allocation().get_height() );

    // Assert that _clean_region is a subregion of _store_rect
    auto tmp = _clean_region->copy();
    tmp->subtract(geom_to_cairo(_store_rect));
    assert(tmp->empty());

    // Ensure store contains canvas_rect
    const auto pad = Geom::IntPoint(200, 200);
    const auto device_scale = get_scale_factor();

    if (!_backing_store || _device_scale != device_scale || !_store_rect.intersects(canvas_rect))
    {
        // Recreate the store, using the same memory if possible
        _store_rect = Geom::IntRect::from_xywh( _x0, _y0, canvas_rect.width(), canvas_rect.height() );
        _store_rect.expandBy(pad);
        _device_scale = device_scale;
        if (!_backing_store || _backing_store->get_width() != _store_rect.width() * _device_scale || _backing_store->get_height() != _store_rect.height() * _device_scale)
            _backing_store = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, _store_rect.width() * _device_scale, _store_rect.height() * _device_scale);
       _clean_region = Cairo::Region::create();

       std::cout << "Recreated store" << std::endl;
    }
    else if (!_store_rect.contains(canvas_rect))
    {
        // Create new store, copy usable content across, set as new store
        auto store_rect = Geom::IntRect::from_xywh( _x0, _y0, canvas_rect.width(), canvas_rect.height() );
        store_rect.expandBy(pad);
        auto backing_store = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, store_rect.width() * _device_scale, store_rect.height() * _device_scale);

        auto shift = store_rect.min() - _store_rect.min();
        auto reuse_rect = store_rect & _store_rect;
        auto cr = Cairo::Context::create(backing_store);

        // copy contents of store
        assert(reuse_rect);
        cr->save();
        cr->rectangle(reuse_rect->left() - store_rect.left(), reuse_rect->top() - store_rect.top(), reuse_rect->width(), reuse_rect->height());
        cr->clip();
        cr->set_source(_backing_store, -shift.x(), -shift.y());
        cr->paint();
        cr->restore();

        _store_rect = store_rect;
        _backing_store = std::move(backing_store);
        _clean_region->intersect(geom_to_cairo(_store_rect));

        std::cout << "Partially recreated store" << std::endl;
    }

    assert(_store_rect.contains(canvas_rect));

    // Get region that requires painting
    auto region = Cairo::Region::create(geom_to_cairo(canvas_rect));
    region->subtract(_clean_region);

    // Get mouse position in canvas space
    Geom::IntPoint mouse_loc;
    if (auto window = get_window()) {
        int x;
        int y;
        Gdk::ModifierType mask;
        window->get_device_position(Gdk::Display::get_default()->get_default_seat()->get_pointer(), x, y, mask);
        mouse_loc = Geom::IntPoint(_x0 + x, _y0 + y);
    }
    else {
         mouse_loc = canvas_rect.midpoint();
    }

    // Obtain rectangles list sorted by distance from mouse
    std::vector<Geom::IntRect> rects;
    rects.reserve(region->get_num_rectangles());
    for (int i = 0; i < region->get_num_rectangles(); i++) {
        rects.emplace_back(cairo_to_geom(region->get_rectangle(i)));
    }
    std::sort(rects.begin(), rects.end(), [&] (const Geom::IntRect &a, const Geom::IntRect &b) {return distSq(mouse_loc, a) < distSq(mouse_loc, b);});

    // Set up painting info to pass down
    PaintRectSetup setup;
    setup.canvas_rect = canvas_rect;
    setup.mouse_loc = mouse_loc;

    auto prefs = Inkscape::Preferences::get();
    auto tile_multiplier = prefs->getIntLimited("/options/rendering/tile-multiplier", 16, 1, 512);
    if (_render_mode != Inkscape::RenderMode::OUTLINE) {
        // Can't be too small or large gradient will be rerendered too many times!
        setup.max_pixels = 65536 * tile_multiplier;
    } else {
        // Paths only. 1M is catched buffer and we need four channels.
        setup.max_pixels = 262144;
    }

    // Begin painting
    setup.start_time = g_get_monotonic_time();
    setup.disable_timeouts = _forced_redraw_limit != -1 && _forced_redraw_count >= _forced_redraw_limit;
    setup.disable_timeouts = false; // TEMP HACK: I WANT TO INVESTIGATE BEHAVIOUR WITHOUT THIS INTERFERING

    for (const auto &rect : rects) {
        auto area = rect & canvas_rect;

        if (!area || area->hasZeroArea()) {
            continue;
        }

        if (!paint_rect_internal(setup, rect)) {
            // Timed out. Temporarily return to idle loop, and come back here if still idle.
            std::cout << "timed out: " << g_get_monotonic_time() - setup.start_time << " us \n";
            _forced_redraw_count++;
            return true;
        }
    }

    // Check if suppressed a time out, and adjust counter if so
    if (setup.disable_timeouts)
    {
        auto now = g_get_monotonic_time();
        auto elapsed = now - setup.start_time;
        if (elapsed > 1000) {
            // Timed out
            std::cout << "ignore timeout: " << g_get_monotonic_time() - setup.start_time << " us \n";
            _forced_redraw_count = 0;
        }
    }

    // todo: check clean region is what it should be

    std::cout << "finished drawing\n";
    return false;
}

/*
 * Returns false to bail out in the event of a timeout.
 * Queues Gtk redraw of widget.
 */
bool
Canvas::paint_rect_internal(PaintRectSetup const &setup, Geom::IntRect const &this_rect)
{
    // Find optimal buffer dimension
    int bw = this_rect.width();
    int bh = this_rect.height();

    if (bw < 1 || bh < 1) {
        // Nothing to draw!
        return true;
    }

    /*
    // Uncomment for artificial redraw slowness fun - you won't regret it!
    if (bw > bh || bw > 10) {
        int mid = this_rect[Geom::X].middle();

        Geom::IntRect lo, hi;
        lo = Geom::IntRect(this_rect.left(), this_rect.top(), mid,               this_rect.bottom());
        hi = Geom::IntRect(mid,              this_rect.top(), this_rect.right(), this_rect.bottom());

        if (setup.mouse_loc[Geom::X] < mid) {
            // Always paint towards the mouse first
            return paint_rect_internal(setup, lo)
                && paint_rect_internal(setup, hi);
        } else {
            return paint_rect_internal(setup, hi)
                && paint_rect_internal(setup, lo);
        }
    } else if (bh > bw && bh > 10) {
        int mid = this_rect[Geom::Y].middle();

        Geom::IntRect lo, hi;
        lo = Geom::IntRect(this_rect.left(), this_rect.top(), this_rect.right(), mid                );
        hi = Geom::IntRect(this_rect.left(), mid,             this_rect.right(), this_rect.bottom());

        if (setup.mouse_loc[Geom::Y] < mid) {
            // Always paint towards the mouse first
            return paint_rect_internal(setup, lo)
                && paint_rect_internal(setup, hi);
        } else {
            return paint_rect_internal(setup, hi)
                && paint_rect_internal(setup, lo);
        }
    }
    */

    if (bw * bh < setup.max_pixels) {
        // We are small enough!

        if (!setup.disable_timeouts) {
            auto now = g_get_monotonic_time();
            auto elapsed = now - setup.start_time;
            if (elapsed > 1000) {
                return false;
            }
        }

        _drawing->setRenderMode(_render_mode);
        _drawing->setColorMode(_color_mode);

        paint_single_buffer(this_rect, setup.canvas_rect, _backing_store);
        /*bool outline_overlay = _drawing->outlineOverlay();
        if (_split_mode != Inkscape::SplitMode::NORMAL || outline_overlay) {
            _drawing->setRenderMode(Inkscape::RenderMode::OUTLINE);
            paint_single_buffer(this_rect, setup.canvas_rect, _outline_store);
            if (outline_overlay) {
                _drawing->setRenderMode(Inkscape::RenderMode::OUTLINE_OVERLAY);
            }
        }*/

        Cairo::RectangleInt crect = { this_rect.left(), this_rect.top(), this_rect.width(), this_rect.height() };
        _clean_region->do_union( crect );

        queue_draw_area(this_rect.left() - _x0, this_rect.top() - _y0, this_rect.width(), this_rect.height());

        return true;
    }

    /*
     * Determine redraw strategy:
     *
     * bw < bh (strips mode): Draw horizontal strips starting from cursor position.
     *                        Seems to be faster for drawing many smaller objects zoomed out.
     *
     * bw > hb (chunks mode): Splits across the larger dimension of the rectangle, painting
     *                        in almost square chunks (from the cursor.
     *                        Seems to be faster for drawing a few blurred objects across the entire screen.
     *                        Seems to be somewhat psycologically faster.
     *
     * Default is for strips mode.
     */

    static int TILE_SIZE = 16;
    Geom::IntRect lo, hi;

    if (bw < bh || bh < 2 * TILE_SIZE) {
        int mid = this_rect[Geom::X].middle();

        lo = Geom::IntRect(this_rect.left(), this_rect.top(), mid,               this_rect.bottom());
        hi = Geom::IntRect(mid,              this_rect.top(), this_rect.right(), this_rect.bottom());

        if (setup.mouse_loc[Geom::X] < mid) {
            // Always paint towards the mouse first
            return paint_rect_internal(setup, lo)
                && paint_rect_internal(setup, hi);
        } else {
            return paint_rect_internal(setup, hi)
                && paint_rect_internal(setup, lo);
        }
    } else {
        int mid = this_rect[Geom::Y].middle();

        lo = Geom::IntRect(this_rect.left(), this_rect.top(), this_rect.right(), mid                );
        hi = Geom::IntRect(this_rect.left(), mid,             this_rect.right(), this_rect.bottom());

        if (setup.mouse_loc[Geom::Y] < mid) {
            // Always paint towards the mouse first
            return paint_rect_internal(setup, lo)
                && paint_rect_internal(setup, hi);
        } else {
            return paint_rect_internal(setup, hi)
                && paint_rect_internal(setup, lo);
        }
    }
}

/*
 * Paint a single buffer.
 * paint_rect: buffer rectangle.
 * canvas_rect: canvas rectangle.
 * store: Cairo surface to draw on.
 */
void
Canvas::paint_single_buffer(Geom::IntRect const &paint_rect, Geom::IntRect const &canvas_rect,
                            Cairo::RefPtr<Cairo::ImageSurface> &store)
{
    if (!store) {
        std::cerr << "Canvas::paint_single_buffer: store not created!" << std::endl;
        return;
        // Maybe store not created! - todo: check if this can actually happen
    }

    // Make sure the following code does not go outside of store's data
    assert(store->get_format() == Cairo::FORMAT_ARGB32);
    assert(_store_rect.contains(paint_rect));

    /*auto cr = Cairo::Context::create(store);
    cr->set_source_rgba((rand() % 255) / 255.0, (rand() % 255) / 255.0, (rand() % 255) / 255.0, 0.2);
    cr->rectangle(paint_rect.left() - _store_rect.left(), paint_rect.top() - _store_rect.top(), paint_rect.width(), paint_rect.height());
    cr->fill();*/

    Inkscape::CanvasItemBuffer buf(paint_rect, _store_rect, _device_scale);

    // Create temporary surface that draws directly to store.
    store->flush();

    // std::cout << "  Writing store to png" << std::endl;
    // static int i = 0;
    // ++i;
    // if (i < 5) {
    //     std::string file = "paint_single_buffer0_" + std::to_string(i) + ".png";
    //     store->write_to_png(file);
    // }

    // Create temporary surface that draws directly to store.
    unsigned char *data = store->get_data();
    int stride = store->get_stride();

    // Check we are using the correct device scale.
    double x_scale = 1.0;
    double y_scale = 1.0;
    cairo_surface_get_device_scale(store->cobj(), &x_scale, &y_scale); // No C++ API!
    assert (_device_scale == (int) x_scale);
    assert (_device_scale == (int) y_scale);


    // Move to the correct row.
    data += stride * (paint_rect.top() - _store_rect.top()) * (int)y_scale;
    // Move to the correct column.
    data += 4 * (paint_rect.left() - _store_rect.left()) * (int)x_scale;
    auto imgs = Cairo::ImageSurface::create(data, Cairo::FORMAT_ARGB32,
                                            paint_rect.width()  * _device_scale,
                                            paint_rect.height() * _device_scale,
                                            stride);

    cairo_surface_set_device_scale(imgs->cobj(), _device_scale, _device_scale); // No C++ API!

    auto cr = Cairo::Context::create(imgs);

    // Clear background
    cr->save();
    cr->set_operator(Cairo::OPERATOR_SOURCE);
    cr->set_source_rgba(0,0,0,0);
    cr->paint();
    cr->restore();

    buf.cr = cr;

    // Render drawing on top of background.
    if (_canvas_item_root->is_visible()) {
        _canvas_item_root->render(&buf);
    }

    if (_cms_active) {
        cmsHTRANSFORM transf = nullptr;
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        bool fromDisplay = prefs->getBool( "/options/displayprofile/from_display");
        if ( fromDisplay ) {
            transf = Inkscape::CMSSystem::getDisplayPer(_cms_key);
        } else {
            transf = Inkscape::CMSSystem::getDisplayTransform();
        }

        if (transf) {
            imgs->flush();
            unsigned char *px = imgs->get_data();
            int stride = imgs->get_stride();
            for (int i=0; i<paint_rect.height(); ++i) {
                unsigned char *row = px + i*stride;
                Inkscape::CMSSystem::doTransform(transf, row, row, paint_rect.width());
            }
            imgs->mark_dirty();
        }
    }

    store->mark_dirty();

    // if (i < 5) {
    //     std::cout << "  Writing store to png" << std::endl;
    //     std::string file = "paint_single_buffer1_" + std::to_string(i) + ".png";
    //     store->write_to_png(file);
    // }

    // Uncomment to see how Inkscape paints to rectangles on canvas.
    // cr->save();
    // cr->move_to (0.5,                    0.5);
    // cr->line_to (0.5,                    paint_rect.height()-0.5);
    // cr->line_to (paint_rect.width()-0.5, paint_rect.height()-0.5);
    // cr->line_to (paint_rect.width()-0.5, 0.5);
    // cr->close_path();
    // cr->set_source_rgba(0.0, 0.0, 0.5, 1.0);
    // cr->stroke();
    // cr->restore();
}

// Sets clip path for Split and X-Ray modes.
void
Canvas::add_clippath(const Cairo::RefPtr<Cairo::Context>& cr) {

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double radius = prefs->getIntLimited("/options/rendering/xray-radius", 100, 1, 1500);

    double width  = get_allocation().get_width();
    double height = get_allocation().get_height();
    double sx     = _split_position.x();
    double sy     = _split_position.y();

    if (_split_mode == Inkscape::SplitMode::SPLIT) {
        // We're clipping the outline region... so it's backwards.
        switch (_split_direction) {
            case Inkscape::SplitDirection::SOUTH:
                cr->rectangle(0,   0, width,               sy);
                break;
            case Inkscape::SplitDirection::NORTH:
                cr->rectangle(0,  sy, width,      height - sy);
                break;
            case Inkscape::SplitDirection::EAST:
                cr->rectangle(0,   0,         sx, height     );
                break;
            case Inkscape::SplitDirection::WEST:
                cr->rectangle(sx,  0, width - sx, height     );
                break;
            default:
                // no clipping (for NONE, HORIZONTAL, VERTICAL)
                break;
        }
    } else {
        cr->arc(sx, sy, radius, 0, 2 * M_PI);
    }

    cr->clip();
}

// Change cursor
void
Canvas::set_cursor() {

    if (!_desktop) {
        return;
    }

    auto display = Gdk::Display::get_default();

    switch (_hover_direction) {

        case Inkscape::SplitDirection::NONE:
            _desktop->event_context->use_tool_cursor();
            break;

        case Inkscape::SplitDirection::NORTH:
        case Inkscape::SplitDirection::EAST:
        case Inkscape::SplitDirection::SOUTH:
        case Inkscape::SplitDirection::WEST:
        {
            auto cursor = Gdk::Cursor::create(display, "pointer");
            get_window()->set_cursor(cursor);
            break;
        }

        case Inkscape::SplitDirection::HORIZONTAL:
        {
            auto cursor = Gdk::Cursor::create(display, "ns-resize");
            get_window()->set_cursor(cursor);
            break;
        }

        case Inkscape::SplitDirection::VERTICAL:
        {
            auto cursor = Gdk::Cursor::create(display, "ew-resize");
            get_window()->set_cursor(cursor);
            break;
        }

        default:
            // Shouldn't reach.
            std::cerr << "Canvas::set_cursor: Unknown hover direction!" << std::endl;
    }
}


// This routine reacts to events from the canvas. It's main purpose is to find the canvas item
// closest to the cursor where the event occurred and then send the event (sometimes modified) to
// that item. The event then bubbles up the canvas item tree until an object handles it. If the
// widget is redrawn, this routine may be called again for the same event.
//
// Canvas items register their interest by connecting to the "event" signal.
// Example in desktop.cpp:
//   canvas_catchall->connect_event(sigc::bind(sigc::ptr_fun(sp_desktop_root_handler), this));
bool
Canvas::pick_current_item(GdkEvent *event)
{
    // Ensure geometry is correct.
    if (_need_update) {
        _canvas_item_root->update(_affine);
        _need_update = false;
    }

    int button_down = 0;
    if (_all_enter_events == false) {
        // Only set true in connector-tool.cpp.

        // If a button is down, we'll perform enter and leave events on the
        // current item, but not enter on any other item.  This is more or
        // less like X pointer grabbing for canvas items.
        button_down = _state & (GDK_BUTTON1_MASK |
                                GDK_BUTTON2_MASK |
                                GDK_BUTTON3_MASK |
                                GDK_BUTTON4_MASK |
                                GDK_BUTTON5_MASK);
        if (!button_down) _left_grabbed_item = false;
    }
        
    // Save the event in the canvas.  This is used to synthesize enter and
    // leave events in case the current item changes.  It is also used to
    // re-pick the current item if the current one gets deleted.  Also,
    // synthesize an enter event.
    if (event != &_pick_event) {
        if (event->type == GDK_MOTION_NOTIFY || event->type == GDK_BUTTON_RELEASE) {
            // Convert to GDK_ENTER_NOTIFY

            // These fields have the same offsets in both types of events.
            _pick_event.crossing.type       = GDK_ENTER_NOTIFY;
            _pick_event.crossing.window     = event->motion.window;
            _pick_event.crossing.send_event = event->motion.send_event;
            _pick_event.crossing.subwindow  = nullptr;
            _pick_event.crossing.x          = event->motion.x;
            _pick_event.crossing.y          = event->motion.y;
            _pick_event.crossing.mode       = GDK_CROSSING_NORMAL;
            _pick_event.crossing.detail     = GDK_NOTIFY_NONLINEAR;
            _pick_event.crossing.focus      = false;
            _pick_event.crossing.state      = event->motion.state;

            // These fields don't have the same offsets in both types of events.
            if (event->type == GDK_MOTION_NOTIFY) {
                _pick_event.crossing.x_root = event->motion.x_root;
                _pick_event.crossing.y_root = event->motion.y_root;
            } else {
                _pick_event.crossing.x_root = event->button.x_root;
                _pick_event.crossing.y_root = event->button.y_root;
            }
        } else {
            _pick_event = *event;
        }
    }

    if (_in_repick) {
        // Don't do anything else if this is a recursive call.
        return false;
    }

    // Find new item
    _current_canvas_item_new = nullptr;

    if (_pick_event.type != GDK_LEAVE_NOTIFY && _canvas_item_root->is_visible()) {
        // Leave notify means there is no current item.
        // Find closest item.
        double x = 0.0;
        double y = 0.0;

        if (_pick_event.type == GDK_ENTER_NOTIFY) {
            x = _pick_event.crossing.x;
            y = _pick_event.crossing.y;
        } else {
            x = _pick_event.motion.x;
            y = _pick_event.motion.y;
        }

        // If in split mode, look at where cursor is to see if one should pick with outline mode.
        _drawing->setRenderMode(_render_mode);
        if (_split_mode == Inkscape::SplitMode::SPLIT && !_drawing->outlineOverlay()) {
            if ((_split_direction == Inkscape::SplitDirection::NORTH && y > _split_position.y()) ||
                (_split_direction == Inkscape::SplitDirection::SOUTH && y < _split_position.y()) ||
                (_split_direction == Inkscape::SplitDirection::WEST  && x > _split_position.x()) ||
                (_split_direction == Inkscape::SplitDirection::EAST  && x < _split_position.x()) ) {
                _drawing->setRenderMode(Inkscape::RenderMode::OUTLINE);
            }
        }

        // Convert to world coordinates.
        x += _x0;
        y += _y0;
        Geom::Point p(x, y);

        _current_canvas_item_new = _canvas_item_root->pick_item(p);
        // if (_current_canvas_item_new) {
        //     std::cout << "  PICKING: FOUND ITEM: " << _current_canvas_item_new->get_name() << std::endl;
        // } else {
        //     std::cout << "  PICKING: DID NOT FIND ITEM" << std::endl;
        // }
    }

    if (_current_canvas_item_new == _current_canvas_item &&
        !_left_grabbed_item                               ) {
        // Current item did not change!
        return false;
    }

    // Synthesize events for old and new current items.
    bool retval = false;
    if (_current_canvas_item_new != _current_canvas_item &&
        _current_canvas_item != nullptr                  &&
        !_left_grabbed_item                               ) {

        GdkEvent new_event;
        new_event = _pick_event;
        new_event.type = GDK_LEAVE_NOTIFY;
        new_event.crossing.detail = GDK_NOTIFY_ANCESTOR;
        new_event.crossing.subwindow = nullptr;
        _in_repick = true;
        retval = emit_event(&new_event);
        _in_repick = false;
    }

    if (_all_enter_events == false) {
        // new_current_item may have been set to nullptr during the call to emitEvent() above.
        if (_current_canvas_item_new != _current_canvas_item &&
            button_down                                       ) {
            _left_grabbed_item = true;
            return retval;
        }
    }

    // Handle the rest of cases
    _left_grabbed_item = false;
    _current_canvas_item = _current_canvas_item_new;

    if (_current_canvas_item != nullptr ) {
        GdkEvent new_event;
        new_event = _pick_event;
        new_event.type = GDK_ENTER_NOTIFY;
        new_event.crossing.detail = GDK_NOTIFY_ANCESTOR;
        new_event.crossing.subwindow = nullptr;
        retval = emit_event(&new_event);
    }

    return retval;
}

bool
Canvas::emit_event(GdkEvent *event)
{
    Gdk::EventMask mask = (Gdk::EventMask)0;
    if (_grabbed_canvas_item) {
        switch (event->type) {
        case GDK_ENTER_NOTIFY:
            mask = Gdk::ENTER_NOTIFY_MASK;
            break;
        case GDK_LEAVE_NOTIFY:
            mask = Gdk::LEAVE_NOTIFY_MASK;
            break;
        case GDK_MOTION_NOTIFY:
            mask = Gdk::POINTER_MOTION_MASK;
            break;
        case GDK_BUTTON_PRESS:
        case GDK_2BUTTON_PRESS:
        case GDK_3BUTTON_PRESS:
            mask = Gdk::BUTTON_PRESS_MASK;
            break;
        case GDK_BUTTON_RELEASE:
            mask = Gdk::BUTTON_RELEASE_MASK;
            break;
        case GDK_KEY_PRESS:
            mask = Gdk::KEY_PRESS_MASK;
            break;
        case GDK_KEY_RELEASE:
            mask = Gdk::KEY_RELEASE_MASK;
            break;
        case GDK_SCROLL:
            mask = Gdk::SCROLL_MASK;
            mask |= Gdk::SMOOTH_SCROLL_MASK;
            break;
        default:
            break;
        }

        if (!(mask & _grabbed_event_mask)) {
            return false;
        }
    }

    // Convert to world coordinates. We have two different cases due to
    // different event structures.
    GdkEvent *event_copy = gdk_event_copy(event);
    switch (event_copy->type) {
        case GDK_ENTER_NOTIFY:
        case GDK_LEAVE_NOTIFY:
            event_copy->crossing.x += _x0;
            event_copy->crossing.y += _y0;
            break;
        case GDK_MOTION_NOTIFY:
        case GDK_BUTTON_PRESS:
        case GDK_2BUTTON_PRESS:
        case GDK_3BUTTON_PRESS:
        case GDK_BUTTON_RELEASE:
            event_copy->motion.x += _x0;
            event_copy->motion.y += _y0;
            break;
        default:
            break;
    }

    // Block undo/redo while anything is dragged.
    if (event->type == GDK_BUTTON_PRESS && event->button.button == 1) {
        _is_dragging = true;
    } else if (event->type == GDK_BUTTON_RELEASE) {
        _is_dragging = false;
    }

    gint finished = false; // Can't be bool or "stack smashing detected"!
    
    if (_current_canvas_item) {
        // Choose where to send event;
        CanvasItem *item = _current_canvas_item;

        if (_grabbed_canvas_item && !_current_canvas_item->is_descendant_of(_grabbed_canvas_item)) {
            item = _grabbed_canvas_item;
        }

        // Propagate the event up the canvas item hierarchy until handled.
        while (item) {
            finished = item->handle_event(event_copy);
            if (finished) break;
            item = item->get_parent();
        }
    }

    gdk_event_free(event_copy);

    return finished;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
