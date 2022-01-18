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

#include <iostream> // Logging
#include <algorithm> // Sort
#include <set> // Coarsener

#include <glibmm/i18n.h>

#include <2geom/rect.h>

#include "canvas.h"
#include "canvas-grid.h"

#include "color.h"          // Background color
#include "cms-system.h"     // Color correction
#include "desktop.h"
#include "preferences.h"

#include "display/cairo-utils.h"     // Checkerboard background
#include "display/drawing.h"
#include "display/control/canvas-item-group.h"
#include "display/control/snap-indicator.h"

#include "ui/tools/tool-base.h"      // Default cursor

#include "framecheck.h"    // For frame profiling
#define framecheck_whole_function(D) auto framecheckobj = D->prefs.debug_framecheck ? FrameCheck::Event(__func__) : FrameCheck::Event();

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

namespace Inkscape {
namespace UI {
namespace Widget {

struct PaintRectSetup {
    Geom::IntPoint mouse_loc;
    int max_pixels;
    gint64 start_time;
    bool disable_timeouts;
};

// Preferences system

template<typename T>
struct PrefBase
{
    T t;
    std::unique_ptr<Preferences::PreferencesObserver> obs;
    operator T() const {return t;}
};

template<typename T>
struct Pref {};

template<>
struct Pref<bool> : PrefBase<bool>
{
    std::function<void()> action;
    Pref(const char *path)
    {
        auto prefs = Inkscape::Preferences::get();
        t = prefs->getBool(path);
        obs = prefs->createObserver(path, [=] (const Preferences::Entry &e) {t = e.getBool(); if (action) action();});
    }
};

template<>
struct Pref<int> : PrefBase<int>
{
    std::function<void(int)> action;
    Pref(const char *path, int def, int min, int max)
    {
        auto prefs = Inkscape::Preferences::get();
        t = prefs->getIntLimited(path, def, min, max);
        obs = prefs->createObserver(path, [=] (const Preferences::Entry &e) {t = e.getIntLimited(def, min, max); if (action) action(t);});
    }
};

template<>
struct Pref<double> : PrefBase<double>
{
    Pref(const char *path, double def, double min, double max)
    {
        auto prefs = Inkscape::Preferences::get();
        t = prefs->getDoubleLimited(path, def, min, max);
        obs = prefs->createObserver(path, [=] (const Preferences::Entry &e) {t = e.getDoubleLimited(def, min, max);});
    }
};

struct Prefs
{
    // Original parameters
    Pref<int>    tile_size                = Pref<int>   ("/options/rendering/tile-size", 16, 1, 10000);
    Pref<int>    tile_multiplier          = Pref<int>   ("/options/rendering/tile-multiplier", 16, 1, 512);
    Pref<int>    x_ray_radius             = Pref<int>   ("/options/rendering/xray-radius", 100, 1, 1500);
    Pref<bool>   from_display             = Pref<bool>  ("/options/displayprofile/from_display");
    Pref<int>    grabsize                 = Pref<int>   ("/options/grabsize/value", 3, 1, 15);

    // New parameters
    Pref<int>    render_time_limit        = Pref<int>   ("/options/rendering/render_time_limit", 1000, 100, 1000000);
    Pref<double> max_affine_diff          = Pref<double>("/options/rendering/max_affine_diff", 1.8, 0.0, 100.0);
    Pref<int>    pad                      = Pref<int>   ("/options/rendering/pad", 200, 0, 1000);
    Pref<int>    coarsener_min_size       = Pref<int>   ("/options/rendering/coarsener_min_size", 200, 0, 1000);
    Pref<int>    coarsener_glue_size      = Pref<int>   ("/options/rendering/coarsener_glue_size", 80, 0, 1000);

    // Debug switches
    Pref<bool>   debug_framecheck         = Pref<bool>  ("/options/rendering/debug/framecheck");
    Pref<bool>   debug_logging            = Pref<bool>  ("/options/rendering/debug/logging");
    Pref<bool>   debug_overbisection      = Pref<bool>  ("/options/rendering/debug/overbisection");
    Pref<int>    debug_overbisection_size = Pref<int>   ("/options/rendering/debug/overbisection_size", 30, 1, 10000);
    Pref<bool>   debug_slow_redraw        = Pref<bool>  ("/options/rendering/debug/slow_redraw");
    Pref<int>    debug_slow_redraw_time   = Pref<int>   ("/options/rendering/debug/slow_redraw_time", 50, 0, 1000000);
    Pref<bool>   debug_show_redraw        = Pref<bool>  ("/options/rendering/debug/show_redraw");
    Pref<bool>   debug_show_unclean       = Pref<bool>  ("/options/rendering/debug/show_unclean");
    Pref<bool>   debug_show_snapshot      = Pref<bool>  ("/options/rendering/debug/show_snapshot");
    Pref<bool>   debug_show_clean         = Pref<bool>  ("/options/rendering/debug/show_clean");
    Pref<bool>   debug_disable_redraw     = Pref<bool>  ("/options/rendering/debug/disable_redraw");
    Pref<bool>   debug_sticky_decoupled   = Pref<bool>  ("/options/rendering/debug/sticky_decoupled");
};

class CanvasPrivate
{
public:

    friend class Canvas;
    Canvas *q;
    CanvasPrivate(Canvas *q) : q(q) {}

    void add_idle();
    bool on_idle();
    bool paint_rect_internal(PaintRectSetup const &setup, Geom::IntRect const &this_rect);
    void paint_single_buffer(Geom::IntRect const &paint_rect, Cairo::RefPtr<Cairo::ImageSurface> &store);

    // Important global properties of all the stores. If these change, all the stores must be recreated.
    int _device_scale = 1;
    bool _store_solid_background;

    Geom::IntRect _store_rect;                         ///< Rectangle of the store in world space.
    Geom::Affine _store_affine;
    Cairo::RefPtr<Cairo::ImageSurface> _backing_store; ///< Canvas content.
    Cairo::RefPtr<Cairo::Region> _clean_region; ///< Subregion of backing store with up-to-date content.

    Geom::IntRect _snapshot_rect;
    Geom::Affine _snapshot_affine;
    Cairo::RefPtr<Cairo::ImageSurface> _snapshot_store;
    Cairo::RefPtr<Cairo::Region> _snapshot_clean_region;

    bool decoupled_mode = false;

    bool solid_background;

    sigc::connection hipri_idle;
    sigc::connection lopri_idle;

    struct GdkEventFreer {void operator()(GdkEvent *ev) const {gdk_event_free(ev);}};
    std::vector<std::unique_ptr<GdkEvent, GdkEventFreer>> bucket;
    sigc::connection bucket_emptier;

    bool pending_bucket_emptier = false;
    bool idle_running = false;

    void schedule_bucket_emptier();
    void empty_bucket();

    GdkEvent *ignore = nullptr;

    Geom::Affine geom_affine;

    void queue_draw_area(Geom::IntRect &rect);

    Prefs prefs;

    bool on_hipri_idle();
    bool on_lopri_idle();

    void update_ctrl_sizes(int size) {q->_canvas_item_root->update_canvas_item_ctrl_sizes(size);}
};

void CanvasPrivate::schedule_bucket_emptier()
{
    if (bucket_emptier.connected()) return;

    bucket_emptier = Glib::signal_idle().connect([this] {
        bucket_emptier.disconnect();
        empty_bucket();
        return false;
    }, G_PRIORITY_DEFAULT_IDLE - 5); // before lowpri_idle
}

void CanvasPrivate::empty_bucket()
{
    framecheck_whole_function(this)

    pending_bucket_emptier = false;

    auto bucket2 = std::move(bucket);

    for (auto &event : bucket2) {
        // Block undo/redo while anything is dragged.
        if (event->type == GDK_BUTTON_PRESS && event->button.button == 1) {
            q->_is_dragging = true;
        } else if (event->type == GDK_BUTTON_RELEASE) {
            q->_is_dragging = false;
        }

        bool finished = false;

        if (q->_current_canvas_item) {
            // Choose where to send event.
            CanvasItem *item = q->_current_canvas_item;

            if (q->_grabbed_canvas_item && !q->_current_canvas_item->is_descendant_of(q->_grabbed_canvas_item)) {
                item = q->_grabbed_canvas_item;
            }

            // Propagate the event up the canvas item hierarchy until handled.
            while (item) {
                finished = item->handle_event(event.get());
                if (finished) break;
                item = item->get_parent();
            }
        }

        if (!finished) {
            // Re-fire the event at the window, and ignore it when it comes back here again.
            ignore = event.get();
            q->get_toplevel()->event(event.get());
            ignore = nullptr;
        }
    }
}

void CanvasPrivate::queue_draw_area(Geom::IntRect &rect)
{
    q->queue_draw_area(rect.left(), rect.top(), rect.width(), rect.height());
}

Canvas::Canvas()
    : d(std::make_unique<CanvasPrivate>(this))
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

    // Preferences
    d->prefs.grabsize.action = [=] (int size) {_canvas_item_root->update_canvas_item_ctrl_sizes(size);};
    d->prefs.debug_show_unclean.action = [=] {queue_draw();};
    d->prefs.debug_show_clean.action = [=] {queue_draw();};
    d->prefs.debug_disable_redraw.action = [=] {queue_draw();};
    d->prefs.debug_sticky_decoupled.action = [=] {d->add_idle();};

    // Give _pick_event an initial definition.
    _pick_event.type = GDK_LEAVE_NOTIFY;
    _pick_event.crossing.x = 0;
    _pick_event.crossing.y = 0;

    // Drawing
    d->_clean_region = Cairo::Region::create();

    _background = Cairo::SolidPattern::create_rgb(1.0, 1.0, 1.0);
    d->solid_background = true;

    _canvas_item_root = new Inkscape::CanvasItemGroup(nullptr);
    _canvas_item_root->set_name("CanvasItemGroup:Root");
    _canvas_item_root->set_canvas(this);

    if (d->prefs.debug_show_redraw) srand(g_get_monotonic_time()); // Initialise seed for random colours.
}

Canvas::~Canvas()
{
    assert(!_desktop);

    _drawing = nullptr;
    _in_destruction = true;

    // Disconnect signals. Otherwise called after destructor and crashes.
    d->hipri_idle.disconnect();
    d->lopri_idle.disconnect();

    // Remove entire CanvasItem tree.
    delete _canvas_item_root;
}

/**
 * Is world point inside canvas area?
 */
bool
Canvas::world_point_inside_canvas(Geom::Point const &world) const
{
    Gtk::Allocation allocation = get_allocation();
    return ( _x0 <= world.x() && world.x() < _x0 + allocation.get_width() &&
             _y0 <= world.y() && world.y() < _y0 + allocation.get_height() );
}

/**
 * Translate point in canvas to world coordinates.
 */
Geom::Point
Canvas::canvas_to_world(Geom::Point const &point) const
{
    return Geom::Point(point[Geom::X]+ _x0, point[Geom::Y] + _y0);
}

/**
 * Return the area shown in the canvas in world coordinates.
 */
Geom::IntRect
Canvas::get_area_world() const
{
    Gtk::Allocation allocation = get_allocation();
    return Geom::IntRect::from_xywh(_x0, _y0, allocation.get_width(), allocation.get_height());
}

/**
 * Set the affine for the canvas.
 */
void
Canvas::set_affine(Geom::Affine const &affine)
{
    if (_affine == affine) {
        return;
    }

    _affine = affine;

    d->add_idle();
    queue_draw();
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
    d->_clean_region = Cairo::Region::create(); // Empty region (i.e. everything is dirty).
    d->add_idle();
    if (d->prefs.debug_show_unclean) queue_draw();
}

/**
 * Redraw the given area during idle.
 */
void
Canvas::redraw_area(int x0, int y0, int x1, int y1)
{
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
    d->_clean_region->subtract(crect);
    d->add_idle();
    if (d->prefs.debug_show_unclean) queue_draw();
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
    // Flag geometry as needing update.
    _need_update = true;

    // Trigger the idle process to perform the update.
    d->add_idle();
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

    d->add_idle();
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
    d->solid_background = true;

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
    d->solid_background = false;
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

Cairo::RefPtr<Cairo::ImageSurface> Canvas::get_backing_store() const
{
     return d->_backing_store;
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
            pick_current_item(reinterpret_cast<GdkEvent*>(button_event));
            _state ^= mask;
            retval = emit_event(reinterpret_cast<GdkEvent*>(button_event));
            break;

        case GDK_BUTTON_RELEASE:
            // Process the event as if the button were pressed, then repick after the button has
            // been released.
            _split_dragging = false;

            _state = button_event->state;
            retval = emit_event(reinterpret_cast<GdkEvent*>(button_event));
            button_event->state ^= mask;
            _state = button_event->state;
            pick_current_item(reinterpret_cast<GdkEvent*>(button_event));
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

        if (Geom::distance(cursor_position, _split_position) < 20 * d->_device_scale) {

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
            if (std::abs(difference.y()) < 3 * d->_device_scale) {
                // We're hovering over horizontal line
                hover_direction = Inkscape::SplitDirection::HORIZONTAL;
            }
        } else {
            if (std::abs(difference.x()) < 3 * d->_device_scale) {
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

/**
 * Resize handler
 */
void Canvas::on_size_allocate(Gtk::Allocation &allocation)
{
    parent_type::on_size_allocate(allocation);
    assert(allocation == get_allocation());
    d->add_idle(); // Trigger the size update to be applied to the stores before the next call to on_draw.
}

void Canvas::on_realize()
{
    parent_type::on_realize();
    d->add_idle();
}

auto
geom_to_cairo(Geom::IntRect rect)
{
    return Cairo::RectangleInt{rect.left(), rect.top(), rect.width(), rect.height()};
}

auto
cairo_to_geom(Cairo::RectangleInt rect)
{
    return Geom::IntRect::from_xywh(rect.x, rect.y, rect.width, rect.height);
}

auto geom_to_cairo(Geom::Affine affine)
{
    return Cairo::Matrix(affine[0], affine[1], affine[2], affine[3], affine[4], affine[5]);
}

auto geom_act(Geom::Affine a, Geom::IntPoint p)
{
    Geom::Point p2 = p;
    p2 *= a;
    return Geom::IntPoint(std::round(p2.x()), std::round(p2.y()));
}

void
region_to_path(const Cairo::RefPtr<Cairo::Context> &cr, const Cairo::RefPtr<Cairo::Region> &reg)
{
    for (int i = 0; i < reg->get_num_rectangles(); i++) {
        auto rect = reg->get_rectangle(i);
        cr->rectangle(rect.x, rect.y, rect.width, rect.height);
    }
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
    auto f = FrameCheck::Event();

    // sp_canvas_item_recursive_print_tree(0, _root);
    // canvas_item_print_tree(_canvas_item_root);

    assert(_drawing);

    // Although hipri_idle is scheduled at a priority higher than draw, and should therefore always be called first if asked, there are times when GTK simply decides to call on_draw anyway.
    // Here we ensure that that call has taken place. This is problematic because if hipri_idle does rendering, enlarging the damage rect, then our drawing will still be clipped to the old
    // damage rect. It was precisely this problem that lead to the introduction of hipri_idle. Fortunately, the following failsafe only seems to execute once during initialisation, and
    // once on further resize events. Both these events seem to trigger a full damage, hence we are ok.
    if (d->hipri_idle.connected()) {
        d->hipri_idle.disconnect();
        d->on_hipri_idle();
    }

    // Blit background if not solid. (If solid, it is baked into the stores.)
    if (!d->solid_background) {
        if (d->prefs.debug_framecheck) f = FrameCheck::Event("background");
        cr->save();
        cr->set_operator(Cairo::OPERATOR_SOURCE);
        cr->set_source(_background);
        cr->paint();
        cr->restore();
    }

    if (!d->decoupled_mode) {
        // Blit backing store to screen.
        if (d->prefs.debug_framecheck) f = FrameCheck::Event("draw");
        cr->save();
        cr->set_operator(d->solid_background ? Cairo::OPERATOR_SOURCE : Cairo::OPERATOR_OVER);
        cr->set_source(d->_backing_store, d->_store_rect.left() - _x0, d->_store_rect.top() - _y0);
        cr->paint();
        cr->restore();
    } else {
        // Turn off anti-aliasing for huge performance gains. Only applies to this compositing step.
        cr->set_antialias(Cairo::ANTIALIAS_NONE);

        // Blit background to complement of both clean regions, if solid (and therefore not already drawn).
        if (d->solid_background) {
            if (d->prefs.debug_framecheck) f = FrameCheck::Event("composite", 2);
            cr->save();
            cr->set_fill_rule(Cairo::FILL_RULE_EVEN_ODD);
            cr->rectangle(0, 0, get_allocation().get_width(), get_allocation().get_height());
            cr->translate(-_x0, -_y0);
            cr->transform(geom_to_cairo(_affine * d->_store_affine.inverse()));
            region_to_path(cr, d->_clean_region);
            cr->transform(geom_to_cairo(d->_store_affine * d->_snapshot_affine.inverse()));
            region_to_path(cr, d->_snapshot_clean_region);
            cr->clip();
            cr->set_source(_background);
            cr->set_operator(Cairo::OPERATOR_SOURCE);
            Cairo::SurfacePattern(cr->get_source()->cobj()).set_filter(Cairo::FILTER_FAST);
            cr->paint();
            cr->restore();
        }

        // Draw transformed snapshot, clipped to its clean region and the complement of the backing store's clean region.
        if (d->prefs.debug_framecheck) f = FrameCheck::Event("composite", 1);
        cr->save();
        cr->set_fill_rule(Cairo::FILL_RULE_EVEN_ODD);
        cr->rectangle(0, 0, get_allocation().get_width(), get_allocation().get_height());
        cr->translate(-_x0, -_y0);
        cr->transform(geom_to_cairo(_affine * d->_store_affine.inverse()));
        region_to_path(cr, d->_clean_region);
        cr->clip();
        cr->transform(geom_to_cairo(d->_store_affine * d->_snapshot_affine.inverse()));
        region_to_path(cr, d->_snapshot_clean_region);
        cr->clip();
        cr->set_source(d->_snapshot_store, d->_snapshot_rect.left(), d->_snapshot_rect.top());
        cr->set_operator(d->solid_background ? Cairo::OPERATOR_SOURCE : Cairo::OPERATOR_OVER);
        Cairo::SurfacePattern(cr->get_source()->cobj()).set_filter(Cairo::FILTER_FAST);
        cr->paint();
        if (d->prefs.debug_show_snapshot) {
            cr->set_source_rgba(0, 0, 1, 0.2);
            cr->set_operator(Cairo::OPERATOR_OVER);
            cr->paint();
        }
        cr->restore();

        // Draw transformed store, clipped to clean region.
        if (d->prefs.debug_framecheck) f = FrameCheck::Event("composite", 0);
        cr->save();
        cr->translate(-_x0, -_y0);
        cr->transform(geom_to_cairo(_affine * d->_store_affine.inverse()));
        region_to_path(cr, d->_clean_region);
        cr->clip();
        cr->set_source(d->_backing_store, d->_store_rect.left(), d->_store_rect.top());
        cr->set_operator(d->solid_background ? Cairo::OPERATOR_SOURCE : Cairo::OPERATOR_OVER);
        Cairo::SurfacePattern(cr->get_source()->cobj()).set_filter(Cairo::FILTER_FAST);
        cr->paint();
        cr->restore();
    }

    // Paint unclean regions in red.
    if (d->prefs.debug_show_unclean) {
        if (d->prefs.debug_framecheck) f = FrameCheck::Event("paint_unclean");
        auto reg = Cairo::Region::create(geom_to_cairo(d->_store_rect));
        reg->subtract(d->_clean_region);
        cr->save();
        cr->translate(-_x0, -_y0);
        if (d->decoupled_mode) {
            cr->transform(geom_to_cairo(_affine * d->_store_affine.inverse()));
        }
        cr->set_source_rgba(1, 0, 0, 0.2);
        region_to_path(cr, reg);
        cr->fill();
        cr->restore();
    }

    // Paint internal edges of clean region in green.
    if (d->prefs.debug_show_clean) {
        if (d->prefs.debug_framecheck) f = FrameCheck::Event("paint_clean");
        cr->save();
        cr->translate(-_x0, -_y0);
        if (d->decoupled_mode) {
            cr->transform(geom_to_cairo(_affine * d->_store_affine.inverse()));
        }
        cr->set_source_rgba(0, 0.7, 0, 0.4);
        region_to_path(cr, d->_clean_region);
        cr->stroke();
        cr->restore();
    }

    // Process bucketed events as soon as possible after draw.
    if (d->pending_bucket_emptier) {
        if (!d->bucket.empty()) {
            d->schedule_bucket_emptier();
        } else {
            d->pending_bucket_emptier = false;
        }
    }

    // Todo: Add back X-ray view.

    return true;
}

void
CanvasPrivate::add_idle()
{
    framecheck_whole_function(this)

    if (q->_in_destruction) {
        std::cerr << "Canvas::add_idle: Called after canvas destroyed!" << std::endl;
        return;
    }

    if (!q->get_realized()) {
        // We can safely discard events until realized, because we will run add_idle in the on_realize handler later in initialisation.
        return;
    }

    if (!hipri_idle.connected()) {
        hipri_idle = Glib::signal_idle().connect(sigc::mem_fun(this, &CanvasPrivate::on_hipri_idle), G_PRIORITY_HIGH_IDLE + 15); // after resize, before draw
    }

    if (!lopri_idle.connected()) {
        lopri_idle = Glib::signal_idle().connect(sigc::mem_fun(this, &CanvasPrivate::on_lopri_idle), G_PRIORITY_DEFAULT_IDLE);
    }

    idle_running = true;
}

auto
distSq(const Geom::IntPoint pt, const Geom::IntRect &rect)
{
    auto v = rect.clamp(pt) - pt;
    return v.x() * v.x() + v.y() * v.y();
}

auto
calc_affine_diff(const Geom::Affine &a, const Geom::Affine &b) {
    auto c = a.inverse() * b;
    return std::abs(c[0] - 1) + std::abs(c[1]) + std::abs(c[2]) + std::abs(c[3] - 1);
}

// Replace a region with a larger region consisting of fewer, larger rectangles. (Allowed to slightly overlap.)
auto
coarsen(const Cairo::RefPtr<Cairo::Region> &region, int min_size, int glue_size)
{
    // Sort the rects by minExtent
    struct Compare
    {
        bool operator()(const Geom::IntRect &a, const Geom::IntRect &b) const {
            return a.minExtent() < b.minExtent();
        }
    };
    std::multiset<Geom::IntRect, Compare> rects;
    int nrects = region->get_num_rectangles();
    for (int i = 0; i < nrects; i++) {
        rects.emplace(cairo_to_geom(region->get_rectangle(i)));
    }

    // List of processed rectangles.
    std::vector<Geom::IntRect> processed;
    processed.reserve(nrects);

    // Repeatedly expand small rectangles by absorbing their nearby small rectangles.
    while (!rects.empty() && rects.begin()->minExtent() < min_size) {
        // Extract the smallest unprocessed rectangle.
        auto rect = *rects.begin();
        rects.erase(rects.begin());

        while (true) {
            // Find the glue zone.
            auto glue_zone = rect;
            glue_zone.expandBy(glue_size);

            // Absorb rectangles in the glue zone. We could do better algorithmically speaking, but in real life it's already plenty fast.
            auto orig = rect;
            for (auto it = rects.begin(); it != rects.end(); ) {
                if (glue_zone.contains(*it)) {
                    rect.unionWith(*it);
                    it = rects.erase(it);
                } else {
                    ++it;
                }
            }
            for (auto it = processed.begin(); it != processed.end(); ) {
                if (glue_zone.contains(*it)) {
                    rect.unionWith(*it);
                    *it = processed.back();
                    processed.pop_back();
                } else {
                    ++it;
                }
            }

            // Stop growing if not changed or now big enough.
            bool finished = rect == orig || rect.minExtent() >= min_size;
            if (finished) {
                break;
            }
        }

        // Put the finished rectangle in processed.
        processed.emplace_back(rect);
    }

    // Put any remaining rectangles in processed.
    for (auto &rect : rects) {
        processed.emplace_back(rect);
    }

    return processed;
}

bool
CanvasPrivate::on_hipri_idle()
{
    if (idle_running) {
        idle_running = on_idle();
    }
    return false;
}

bool
CanvasPrivate::on_lopri_idle()
{
    if (idle_running) {
        idle_running = on_idle();
    }
    return idle_running;
}

bool
CanvasPrivate::on_idle()
{
    framecheck_whole_function(this)

    assert(q->_canvas_item_root);

    if (q->_in_destruction) {
        std::cerr << "Canvas::on_idle: Called after canvas destroyed!" << std::endl;
        return false;
    }

    // Quit idle process if not supposed to be drawing.
    if (!q->_drawing || q->_drawing_disabled) {
        return false;
    }

    const Geom::IntPoint pad(prefs.pad, prefs.pad);
    auto recreate_store = [&, this] {
        // Recreate the store at the current affine so that it covers the visible region.
        _store_rect = Geom::IntRect::from_xywh( q->_x0, q->_y0, q->get_allocation().get_width(), q->get_allocation().get_height() );
        _store_rect.expandBy(pad);
        _store_affine = q->_affine;
        int desired_width  = _store_rect.width()  * _device_scale;
        int desired_height = _store_rect.height() * _device_scale;
        if (!_backing_store || _backing_store->get_width() != desired_width || _backing_store->get_height() != desired_height) {
            // Todo: Stop cairo unnecessarily pre-filling the store with transparency below if solid_background is true.
            _backing_store = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, desired_width, desired_height);
            cairo_surface_set_device_scale(_backing_store->cobj(), _device_scale, _device_scale); // No C++ API!
        }
        auto cr = Cairo::Context::create(_backing_store);
        if (solid_background) {
            cr->set_operator(Cairo::OPERATOR_SOURCE);
            cr->set_source(q->_background);
        } else {
            cr->set_operator(Cairo::OPERATOR_CLEAR);
        }
        cr->paint();
        _clean_region = Cairo::Region::create();
        if (prefs.debug_show_unclean) q->queue_draw();
    };

    // Determine the rendering parameters have changed, and reset if so.
    if (!_backing_store || _device_scale != q->get_scale_factor() || _store_solid_background != solid_background) {
        _device_scale = q->get_scale_factor();
        _store_solid_background = solid_background;
        recreate_store();
        decoupled_mode = false;
        if (prefs.debug_logging) std::cout << "Full reset" << std::endl;
    }

    // Todo: Consider incrementally and pre-emptively performing this operation across several frames to avoid lag spikes.
    // Todo: Screw that. Just do it on the GPU.
    auto shift_store = [&, this] {
        // Recreate the store, but keep re-usable content from the old store.
        auto store_rect = Geom::IntRect::from_xywh( q->_x0, q->_y0, q->get_allocation().get_width(), q->get_allocation().get_height() );
        store_rect.expandBy(pad);
        // Todo: Stop cairo unnecessarily pre-filling the store with transparency below if solid_background is true.
        auto backing_store = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, store_rect.width() * _device_scale, store_rect.height() * _device_scale);
        cairo_surface_set_device_scale(backing_store->cobj(), _device_scale, _device_scale); // No C++ API!

        // Determine the geometry of the shift.
        auto shift = store_rect.min() - _store_rect.min();
        auto reuse_rect = store_rect & _store_rect;
        assert(reuse_rect); // Should not be called if there is no overlap.
        auto cr = Cairo::Context::create(backing_store);

        // Paint background into region not covered by next operation.
        if (solid_background) {
            auto reg = Cairo::Region::create(geom_to_cairo(store_rect));
            reg->subtract(geom_to_cairo(*reuse_rect));
            reg->translate(-store_rect.left(), -store_rect.top());
            cr->save();
            if (solid_background) {
                cr->set_operator(Cairo::OPERATOR_SOURCE);
                cr->set_source(q->_background);
            } else {
                cr->set_operator(Cairo::OPERATOR_CLEAR);
            }
            region_to_path(cr, reg);
            cr->fill();
            cr->restore();
        }

        // Copy re-usuable contents of old store into new store, shifted.
        cr->save();
        cr->rectangle(reuse_rect->left() - store_rect.left(), reuse_rect->top() - store_rect.top(), reuse_rect->width(), reuse_rect->height());
        cr->clip();
        cr->set_source(_backing_store, -shift.x(), -shift.y());
        cr->set_operator(Cairo::OPERATOR_SOURCE);
        cr->paint();
        cr->restore();

        // Set the result as the new backing store.
        _store_rect = store_rect;
        assert(_store_affine == q->_affine); // Should not be called if the affine has changed.
        _backing_store = std::move(backing_store);
        _clean_region->intersect(geom_to_cairo(_store_rect));
        if (prefs.debug_show_unclean) q->queue_draw();
    };

    auto take_snapshot = [&, this] {
        // Copy the backing store to the snapshot, leaving us temporarily in an invalid state.
        std::swap(_snapshot_store, _backing_store); // This will re-use the old snapshot store later if possible.
        _snapshot_rect = _store_rect;
        _snapshot_affine = _store_affine;
        _snapshot_clean_region = _clean_region->copy();

        // Recreate the backing store, making the state valid again.
        recreate_store();
    };

    // Handle transitions and actions in response to viewport changes.
    if (!decoupled_mode) {
        // Enter decoupled mode if the affine has changed from what the backing store was drawn at.
        if (q->_affine != _store_affine) {
            // Snapshot and reset the backing store.
            take_snapshot();

            // Enter decoupled mode.
            if (prefs.debug_logging) std::cout << "Entering decoupled mode" << std::endl;
            decoupled_mode = true;

            // Note: If redrawing is fast enough to finish during the frame, then going into decoupled mode, drawing, and leaving
            // it again performs exactly the same rendering operations as if we had not gone into it at all. Also, no extra copies
            // or blits are performed, and the drawing operations done on the screen are the same. Hence this feature comes at zero cost.
        } else {
            // Get visible rectangle in canvas coordinates.
            const auto visible = Geom::IntRect::from_xywh( q->_x0, q->_y0, q->get_allocation().get_width(), q->get_allocation().get_height() );
            if (!_store_rect.intersects(visible)) {
                // If the store has gone completely off-screen, recreate it.
                recreate_store();
                if (prefs.debug_logging) std::cout << "Recreated store" << std::endl;
            } else if (!_store_rect.contains(visible))
            {
                // If the store has gone partially off-screen, shift it.
                shift_store();
                if (prefs.debug_logging) std::cout << "Shifted store" << std::endl;
            }
            // After these operations, the store should now be fully on-screen.
            assert(_store_rect.contains(visible));
        }
    } else { // if (decoupled_mode)
        // Completely cancel the previous redraw and start again if the viewing parameters have changed too much.
        if (!prefs.debug_sticky_decoupled) {
            auto pl = Geom::Parallelogram(q->get_area_world());
            pl *= _store_affine * q->_affine.inverse();
            if (!pl.intersects(_store_rect)) {
                // Store has gone off the screen.
                recreate_store();
                if (prefs.debug_logging) std::cout << "Restarting redraw (store off-screen)" << std::endl;
            } else {
                auto diff = calc_affine_diff(q->_affine, _store_affine);
                if (diff > prefs.max_affine_diff) {
                    // Affine has changed too much.
                    recreate_store();
                    if (prefs.debug_logging) std::cout << "Restarting redraw (affine changed too much)" << std::endl;
                }
            }
        }
    }

    // Assert that _clean_region is a subregion of _store_rect.
    #ifndef NDEBUG
    auto tmp = _clean_region->copy();
    tmp->subtract(geom_to_cairo(_store_rect));
    assert(tmp->empty());
    #endif

    // Ensure the geometry is up-to-date and in the right place.
    auto affine = decoupled_mode ? _store_affine : q->_affine;
    if (q->_need_update || geom_affine != affine) {
        q->_canvas_item_root->update(affine);
        geom_affine = affine;
        q->_need_update = false;
    }

    // Todo: if redrawing is measured to be extremely heavy, we could consider deferring the start of redrawing until the viewing parameters have changed sufficiently much.
    // There are only two main obstructions to implementing this feature. First, having to write timing code to measure whether redraw is heavy. Second, and more importantly,
    // it can't be implemented without the Canvas being hooked up to a "transforming finished" event, say mouse release. As these are annoying and the benefit is minor, I'm
    // not considering implementing this feature. But it would go here.

    // Get the rectangle of store that is visible.
    Geom::OptIntRect visible_rect;
    if (!decoupled_mode) {
        // By a previous assertion, this always lies within the store.
        visible_rect = Geom::IntRect::from_xywh( q->_x0, q->_y0, q->get_allocation().get_width(), q->get_allocation().get_height() );
    } else {
        // Get the window rectangle transformed into canvas space.
        auto pl = Geom::Parallelogram(Geom::IntRect::from_xywh( q->_x0, q->_y0, q->get_allocation().get_width(), q->get_allocation().get_height() ));
        pl *= _store_affine * q->_affine.inverse();

        // Get its bounding box, rounded outwards.
        auto b = pl.bounds();
        auto bi = Geom::IntRect(b.min().floor(), b.max().ceil());

        // The visible rect is the intersection of this with the store
        visible_rect = bi & _store_rect;

        // Note: We could have used a smaller region containing pl consisting of many rectangles, rather than a single bounding box.
        // However, while this uses less pixels it also uses more rectangles, so is NOT necessarily an optimisation and could backfire.
        // For this reason, and for simplicity, we use the bounding rect. (Note: Slightly invalidated by addition of coarsener below.)
    }
    // The visible rectangle must be a subrectangle of store.
    assert(_store_rect.contains(visible_rect));

    // Get the region to paint, which is the visible rectangle minus the clean region (both subregions of store).
    Cairo::RefPtr<Cairo::Region> paint_region;
    if (visible_rect) {
        paint_region = Cairo::Region::create(geom_to_cairo(*visible_rect));
        paint_region->subtract(_clean_region);
    } else {
        paint_region = Cairo::Region::create();
    }

    // Get the list of rectangles to paint, coarsened to avoid fragmentation.
    auto paint_rects = coarsen(paint_region, prefs.coarsener_min_size, prefs.coarsener_glue_size);

    // Clip the coarsened rectangles back to the visible rect, in case coarsening made them go outside it.
    // Todo: That should be impossible, but somehow it occasionally happens. Understand!
    for (auto it = paint_rects.begin(); it != paint_rects.end(); ) {
        auto opt = *it & *visible_rect;
        if (opt) {
            *it = *opt;
            ++it;
        } else {
            *it = paint_rects.back();
            paint_rects.pop_back();
        }
    }

    // Get the mouse position in screen space.
    Geom::IntPoint mouse_loc;
    if (auto window = q->get_window()) {
        int x;
        int y;
        Gdk::ModifierType mask;
        window->get_device_position(Gdk::Display::get_default()->get_default_seat()->get_pointer(), x, y, mask);
        mouse_loc = Geom::IntPoint(x, y);
    } else {
        mouse_loc = Geom::IntPoint(0, 0); // Doesn't particularly matter, just as long as it's initialised.
    }

    // Map the mouse to canvas space.
    mouse_loc += Geom::IntPoint(q->_x0, q->_y0);
    if (decoupled_mode) {
        mouse_loc = geom_act(_store_affine * q->_affine.inverse(), mouse_loc);
    }

    // Todo: Consider further subdividing the region if the mouse lies on the edge of a big rectangle.
    // Otherwise, the whole of the big rectangle will be rendered first, at the expense of points in other
    // rectangles very near to the mouse.

    // Sort the rectangles to paint by distance from mouse.
    std::sort(paint_rects.begin(), paint_rects.end(), [&] (const Geom::IntRect &a, const Geom::IntRect &b) {
        return distSq(mouse_loc, a) < distSq(mouse_loc, b);
    });

    // Set up painting info to pass down the stack.
    PaintRectSetup setup;
    setup.mouse_loc = mouse_loc;
    // Todo: Logging reveals that Inkscape often underestimates render times, and can sometimes time out quite badly,
    // leading to missed frames. Consider fixing the time estimator below, possibly based on run-time feedback. May
    // even wish to consider maintaining a coarse heatmap of the drawing to judge rendering time/order. In the meantime,
    // this can be worked around by enabling overbisection with a tile size of about 400.
    if (q->_render_mode != Inkscape::RenderMode::OUTLINE) {
        // Can't be too small or large gradient will be rerendered too many times!
        setup.max_pixels = 65536 * prefs.tile_multiplier;
    } else {
        // Paths only. 1M is catched buffer and we need four channels.
        setup.max_pixels = 262144;
    }
    setup.start_time = g_get_monotonic_time();
    //setup.disable_timeouts = _forced_redraw_limit != -1 && _forced_redraw_count >= _forced_redraw_limit; // Enable a forced redraw if asked.
    // Todo: Forced redraws are temporarily disabled. They are no longer helpful in all the situations they used to be.
    // Need to go through all places where forced redraw is requested, and check whether it is helpful or harmful.
    setup.disable_timeouts = false;

    // If asked to, don't paint anything and instead halt the idle process.
    if (prefs.debug_disable_redraw) {
        return false;
    }

    // Paint the rectangles.
    for (const auto &rect : paint_rects) {
        if (rect.hasZeroArea()) {
            // Todo: I'm not sure if these can get through or not. If not, remove this check. This is a question about cairo.
            continue;
        }
        if (!paint_rect_internal(setup, rect)) {
            // Timed out. Temporarily return to GTK main loop, and come back here when next idle.
            if (prefs.debug_logging) std::cout << "Timed out: " << g_get_monotonic_time() - setup.start_time << " us" << std::endl;
            framecheckobj.subtype = 1;
            q->_forced_redraw_count++;
            return true;
        }
    }

    // Check if suppressed a timeout.
    if (setup.disable_timeouts) {
        auto now = g_get_monotonic_time();
        auto elapsed = now - setup.start_time;
        if (elapsed > prefs.render_time_limit) {
            // Timed out. Reset counter. It will count up again on future timeouts, eventually triggering another forced redraw.
            q->_forced_redraw_count = 0;
            if (prefs.debug_logging) std::cout << "Ignored timeout: " << g_get_monotonic_time() - setup.start_time << " us" << std::endl;
            framecheckobj.subtype = 2;
            return false;
        }
    }

    // Implement the sticky decoupled mode debug switch.
    if (decoupled_mode && prefs.debug_sticky_decoupled) return false;

    // Finished drawing. Handle transitions out of decoupled mode, by checking if we need to do a final redraw at the correct affine.
    if (decoupled_mode) {
        if (_store_affine == q->_affine) {
            // Content is rendered at the correct affine - exit decoupled mode and quit idle process.
            if (prefs.debug_logging) std::cout << "Finished drawing - exiting decoupled mode" << std::endl;
            // Exit decoupled mode.
            decoupled_mode = false;
            // Quit idle process.
            return false;
        } else {
            // Content is rendered at the wrong affine - take a new snapshot and continue idle process to continue rendering at the new affine.
            if (prefs.debug_logging) std::cout << "Scheduling final redraw" << std::endl;
            // Snapshot and reset the backing store.
            take_snapshot();
            // Continue idle process.
            return true;
        }
    } else {
        // All done, quit the idle process.
        framecheckobj.subtype = 3;
        return false;
    }
}

/*
 * Returns false to bail out in the event of a timeout.
 * Queues Gtk redraw of widget.
 */
bool
CanvasPrivate::paint_rect_internal(PaintRectSetup const &setup, Geom::IntRect const &this_rect)
{
    int bw = this_rect.width();
    int bh = this_rect.height();

    if (bw < 1 || bh < 1) {
        // Nothing to draw!
        return true;
    }

    // Due to coarsening, it's possible for bisected rectangles to lie entirely inside the clean region. These can be discarded, and in fact must be,
    // because otherwise we risk rendering only clean rectangles for the whole frame, which would lead to a render stall.
    if (_clean_region->contains_rectangle(geom_to_cairo(this_rect)) == Cairo::REGION_OVERLAP_IN) {
        // Rectangle is already clean.
        return true;
    }

    // Bisect rectangles that are too big.
    if (!prefs.debug_overbisection) {
        // Use original bisector.
        // Todo: I still don't understand the rationale behind the following strategy. But I'm keeping it default for now.

        /*
         * Determine redraw strategy:
         *
         * bw < bh (strips mode): Draw horizontal strips starting from cursor position.
         *                        Seems to be faster for drawing many smaller objects zoomed out.
         *
         * bw > hb (chunks mode): Splits across the larger dimension of the rectangle, painting
         *                        in almost square chunks (from the cursor.
         *                        Seems to be faster for drawing a few blurred objects across the entire screen.
         *                        Seems to be somewhat psychologically faster.
         *
         * Default is for strips mode.
         */

        if (bw * bh > setup.max_pixels) {
            if (bw < bh || bh < 2 * prefs.tile_size) {
                int mid = this_rect[Geom::X].middle();

                auto lo = Geom::IntRect(this_rect.left(), this_rect.top(), mid,               this_rect.bottom());
                auto hi = Geom::IntRect(mid,              this_rect.top(), this_rect.right(), this_rect.bottom());

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

                auto lo = Geom::IntRect(this_rect.left(), this_rect.top(), this_rect.right(), mid               );
                auto hi = Geom::IntRect(this_rect.left(), mid,             this_rect.right(), this_rect.bottom());

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
    } else {
        // Use the new bisector, which just chops in half along the larger dimension.
        if (bw > bh) {
            if (bw > prefs.debug_overbisection_size) {
                int mid = this_rect[Geom::X].middle();

                auto lo = Geom::IntRect(this_rect.left(), this_rect.top(), mid,               this_rect.bottom());
                auto hi = Geom::IntRect(mid,              this_rect.top(), this_rect.right(), this_rect.bottom());

                if (setup.mouse_loc[Geom::X] < mid) {
                    return paint_rect_internal(setup, lo)
                        && paint_rect_internal(setup, hi);
                } else {
                    return paint_rect_internal(setup, hi)
                        && paint_rect_internal(setup, lo);
                }
            }
        } else {
            if (bh > prefs.debug_overbisection_size) {
                int mid = this_rect[Geom::Y].middle();

                auto lo = Geom::IntRect(this_rect.left(), this_rect.top(), this_rect.right(), mid               );
                auto hi = Geom::IntRect(this_rect.left(), mid,             this_rect.right(), this_rect.bottom());

                if (setup.mouse_loc[Geom::Y] < mid) {
                    return paint_rect_internal(setup, lo)
                        && paint_rect_internal(setup, hi);
                } else {
                    return paint_rect_internal(setup, hi)
                        && paint_rect_internal(setup, lo);
                }
            }
        }
    }

    // Paint the rectangle.
    q->_drawing->setRenderMode(q->_render_mode);
    q->_drawing->setColorMode(q->_color_mode);
    paint_single_buffer(this_rect, _backing_store);

    // Introduce an artificial delay for each rectangle.
    if (prefs.debug_slow_redraw) g_usleep(prefs.debug_slow_redraw_time);

    // Mark the rectangle as clean.
    _clean_region->do_union(geom_to_cairo(this_rect));

    // Mark the screen dirty.
    if (!decoupled_mode) {
        // Get rectangle needing repaint
        auto repaint_rect = this_rect - Geom::IntPoint(q->_x0, q->_y0);

        // Assert that a repaint actually occurs (guaranteed because we are only asked to paint fully on-screen rectangles)
        auto screen_rect = Geom::IntRect(0, 0, q->get_allocation().get_width(), q->get_allocation().get_height());
        assert(repaint_rect & screen_rect);

        // Schedule repaint
        queue_draw_area(repaint_rect); // Guarantees on_draw will be called in the future.
        pending_bucket_emptier = true; // On the next call to on_draw, schedules the bucket emptier.
    } else {
        // Get rectangle needing repaint (transform into screen space, take bounding box, round outwards)
        auto pl = Geom::Parallelogram(this_rect);
        pl *= q->_affine * _store_affine.inverse();
        pl *= Geom::Translate(-q->_x0, -q->_y0);
        auto b = pl.bounds();
        auto repaint_rect = Geom::IntRect(b.min().floor(), b.max().ceil());

        // Check if repaint is necessary - some rectangles could be entirely off-screen.
        auto screen_rect = Geom::IntRect(0, 0, q->get_allocation().get_width(), q->get_allocation().get_height());
        if (repaint_rect & screen_rect) {
            // Schedule repaint
            queue_draw_area(repaint_rect);
            pending_bucket_emptier = true;
        }
    }

    // Exit if timed out.
    if (!setup.disable_timeouts) {
        auto now = g_get_monotonic_time();
        auto elapsed = now - setup.start_time;
        if (elapsed > prefs.render_time_limit) {
            return false;
        }
    }

    // Continue rendering.
    return true;
}

/*
 * Paint a single buffer.
 * paint_rect: buffer rectangle.
 * canvas_rect: canvas rectangle.
 * store: Cairo surface to draw on.
 */
void
CanvasPrivate::paint_single_buffer(Geom::IntRect const &paint_rect, Cairo::RefPtr<Cairo::ImageSurface> &store)
{
    // Make sure the following code does not go outside of store's data
    assert(store);
    assert(store->get_format() == Cairo::FORMAT_ARGB32);
    assert(_store_rect.contains(paint_rect));

    // Create temporary surface that draws directly to store.
    store->flush();
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
    if (solid_background) {
        cr->set_source(q->_background);
        cr->set_operator(Cairo::OPERATOR_SOURCE);
    } else {
        cr->set_operator(Cairo::OPERATOR_CLEAR);
    }
    cr->paint();
    cr->restore();

    // Render drawing on top of background.
    if (q->_canvas_item_root->is_visible()) {
        auto buf = Inkscape::CanvasItemBuffer{ paint_rect, _device_scale, cr };
        q->_canvas_item_root->render(&buf);
    }

    // Paint over newly drawn content with a translucent random colour
    if (prefs.debug_show_redraw) {
        cr->set_source_rgba((rand() % 255) / 255.0, (rand() % 255) / 255.0, (rand() % 255) / 255.0, 0.2);
        cr->set_operator(Cairo::OPERATOR_OVER);
        cr->rectangle(0, 0, imgs->get_width(), imgs->get_height());
        cr->fill();
    }

    if (q->_cms_active) {
        auto transf = prefs.from_display
                    ? Inkscape::CMSSystem::getDisplayPer(q->_cms_key)
                    : Inkscape::CMSSystem::getDisplayTransform();

        if (transf) {
            imgs->flush();
            auto px = imgs->get_data();
            int stride = imgs->get_stride();
            for (int i = 0; i < paint_rect.height(); i++) {
                auto row = px + i * stride;
                Inkscape::CMSSystem::doTransform(transf, row, row, paint_rect.width());
            }
            imgs->mark_dirty();
        }
    }

    store->mark_dirty();
}

// Sets clip path for Split and X-Ray modes.
void
Canvas::add_clippath(const Cairo::RefPtr<Cairo::Context>& cr)
{
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
        cr->arc(sx, sy, d->prefs.x_ray_radius, 0, 2 * M_PI);
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
    auto affine = d->decoupled_mode ? d->_store_affine : _affine;
    if (_need_update || d->geom_affine != affine) {
        _canvas_item_root->update(affine);
        d->geom_affine = affine;
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
    framecheck_whole_function(d)

    if (event == d->ignore) {
        return false;
    }

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

    d->bucket.emplace_back(event_copy);
    if (!d->pending_bucket_emptier) {
        add_tick_callback([this] (const Glib::RefPtr<Gdk::FrameClock>&) {
            d->schedule_bucket_emptier();
            d->pending_bucket_emptier = true;
            return false;
        });
    }

    return true;
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
