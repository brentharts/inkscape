#include <string>

#include "gradient-with-stops.h"
#include "object/sp-gradient.h"
#include "display/cairo-utils.h"
#include "io/resource.h"
#include "ui/cursor-utils.h"

// widget's height; it should take stop template's height into account
const int GRADIENT_WIDGET_HEIGHT = 32;
// gradient's image height (multiple of checkerboard tiles, they are 6x6)
const int GRADIENT_IMAGE_HEIGHT = 3 * 6;

namespace Inkscape {
namespace UI {
namespace Widget {

using namespace Inkscape::IO;

std::string get_stop_template_path() {
	// "stop handle" template file path
	return Resource::get_filename(Resource::UIS, "gradient-stop.svg");
}

GradientWithStops::GradientWithStops() : _template(get_stop_template_path().c_str()) {
	// default color, it will be updated
	_background_color.set_grey(0.5);
	// for theming, but not used
	set_name("GradientEdit");
	// we need some events
	add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::BUTTON_MOTION_MASK |
		Gdk::POINTER_MOTION_MASK | Gdk::KEY_PRESS_MASK);
	set_can_focus();
}

void GradientWithStops::set_gradient(SPGradient* gradient) {
	_gradient = gradient;

	// listen to release & changes
	_release  = gradient ? gradient->connectRelease([=](SPObject*){ set_gradient(nullptr); }) : sigc::connection();
	_modified = gradient ? gradient->connectModified([=](SPObject*, guint){ update(); }) : sigc::connection();

	update();
}

void GradientWithStops::size_request(GtkRequisition* requisition) const {
	requisition->width = 60;
	requisition->height = GRADIENT_WIDGET_HEIGHT;
}

void GradientWithStops::get_preferred_width_vfunc(int& minimal_width, int& natural_width) const {
	GtkRequisition requisition;
	size_request(&requisition);
	minimal_width = natural_width = requisition.width;
}

void GradientWithStops::get_preferred_height_vfunc(int& minimal_height, int& natural_height) const {
	GtkRequisition requisition;
	size_request(&requisition);
	minimal_height = natural_height = requisition.height;
}

void GradientWithStops::update() {
	if (get_is_drawable()) {
		queue_draw();
	}
}

// capture background color when styles change
void GradientWithStops::on_style_updated() {
	if (auto wnd = dynamic_cast<Gtk::Window*>(this->get_toplevel())) {
		_background_color = wnd->get_style_context()->get_background_color();
	}
	// load and cache cursors
	auto wnd = get_window();
	if (wnd && !_cursor_mouseover) {
		_cursor_mouseover = load_svg_cursor(get_display(), wnd, "select-mouseover.svg");
		_cursor_dragging  = load_svg_cursor(get_display(), wnd, "select-dragging.svg");
		wnd->set_cursor();
	}
}

void draw_gradient(const Cairo::RefPtr<Cairo::Context>& cr, SPGradient* gradient, int x, int width) {
	cairo_pattern_t* check = ink_cairo_pattern_create_checkerboard();

	cairo_set_source(cr->cobj(), check);
	cr->fill_preserve();
	cairo_pattern_destroy(check);

	if (gradient) {
		auto p = gradient->create_preview_pattern(width);
		cairo_matrix_t m;
		cairo_matrix_init_translate(&m, -x, 0);
		cairo_pattern_set_matrix(p, &m);
		cairo_set_source(cr->cobj(), p);
		cr->fill();
		cairo_pattern_destroy(p);
	}
}

// return on-screen positionn of the UI stop corresponding to gradient's color stop at 'index'
GradientWithStops::stop_pos_t GradientWithStops::get_stop_position(size_t index, const layout_t& layout) const {
	if (!_gradient || index >= _gradient->vector.stops.size()) {
		return stop_pos_t {};
	}

	// half of the stop template width; round it to avoid half-pixel coordinates
	const auto dx = round((_template.get_width_px() + 1) / 2);

	auto pos = [&](double offset) { return round(layout.x + layout.width * CLAMP(offset, 0, 1)); };
	const auto& v = _gradient->vector.stops;

	auto offset = pos(v[index].offset);
	auto left = offset - dx;
	if (index > 0) {
		// check previous stop; it may overlap
		auto prev = pos(v[index - 1].offset) + dx;
		if (prev > left) {
			// overlap
			left = round((left + prev) / 2);
		}
	}

	auto right = offset + dx;
	if (index + 1 < v.size()) {
		// check next stop for overlap
		auto next = pos(v[index + 1].offset) - dx;
		if (right > next) {
			// overlap
			right = round((right + next) / 2);
		}
	}

	return stop_pos_t {
		.left = left,
		.tip = offset,
		.right = right,
		.top = layout.height - _template.get_height_px(),
		.bottom = layout.height
	};
}

// widget's layout; mainly location of the gradient's image and stop handles
GradientWithStops::layout_t GradientWithStops::get_layout() const {
	auto allocation = get_allocation();

	const auto stop_width = _template.get_width_px();
	const auto half_stop = round((stop_width + 1) / 2);
	const auto x = half_stop;
	const double width = allocation.get_width() - stop_width;
	const double height = allocation.get_height();

	return layout_t {
		.x = x,
		.y = 0,
		.width = width,
		.height = height
	};
}

// check if stop handle is under (x, y) location, return its index or -1 is not hit
int GradientWithStops::find_stop_at(double x, double y) const {
	if (!_gradient) return -1;

	_gradient->ensureVector();
	const auto& v = _gradient->vector.stops;

	const auto& layout = get_layout();

	// binary search would be nice... find stop handle at (x, y) position
	for (size_t i = 0; i < v.size(); ++i) {
		auto pos = get_stop_position(i, layout);
		if (x >= pos.left && x <= pos.right && y >= pos.top && y <= pos.bottom) {
			return static_cast<int>(i);
		}
	}

	return -1;
}

// this is range of offset adjustment for a given stop
GradientWithStops::limits_t GradientWithStops::get_stop_limits(size_t index) const {
	if (!_gradient) return limits_t {};

	_gradient->ensureVector();
	const auto& v = _gradient->vector.stops;

	if (index < v.size()) {
		double min = 0;
		double max = 1;
		// special cases:
		if (index == 0) {
			min = max = 0; // cannot move first stop
		}
		else if (index + 1 == v.size()) {
			min = max = 1; // cannot move last stop
		}
		else {
			// stops "inside" gradient
			if (index > 0) {
				min = v[index - 1].offset;
			}
			if (index + 1 < v.size()) {
				max = v[index + 1].offset;
			}
		}
		return limits_t { .min_offset = min, .max_offset = max };
	}
	else {
		return limits_t {};
	}
}

bool GradientWithStops::on_focus(Gtk::DirectionType direction) {
	return true;
}

bool GradientWithStops::on_button_press_event(GdkEventButton* event) {
	// single button press selects stop and can start dragging it
	constexpr auto LEFT_BTN = 1;
	if (event->button == LEFT_BTN && _gradient && event->type == GDK_BUTTON_PRESS) {
		auto index = find_stop_at(event->x, event->y);

		if (index >= 0) {
			_signal_stop_selected.emit(index);

			_selected_stop = index;
			auto limits = get_stop_limits(index);

			if (limits.min_offset < limits.max_offset) {
				_dragging = true;
				_pointer_x = event->x;
				_stop_offset = _gradient->vector.stops.at(index).offset;

				auto display = get_display();
				if (_cursor_dragging) {
					gdk_window_set_cursor(event->window, _cursor_dragging->gobj());
				}
			}
		}
	}

	return false;
}

bool GradientWithStops::on_button_release_event(GdkEventButton* event) {
	if (_dragging) {
		gdk_window_set_cursor(event->window, nullptr);
	}
	_dragging = false;
	return false;
}

bool GradientWithStops::on_motion_notify_event(GdkEventMotion* event) {
	if (_dragging && _gradient) {
		// move stop to a new position (adjust offset)
		auto dx = event->x - _pointer_x;
		auto layout = get_layout();
		if (layout.width > 0) {
			auto delta = dx / layout.width;
			auto limits = get_stop_limits(_selected_stop);
			if (limits.min_offset < limits.max_offset) {
				auto new_offset = CLAMP(_stop_offset + delta, limits.min_offset, limits.max_offset);
				_signal_stop_offset_changed.emit(_selected_stop, new_offset);
			}
		}
	}
	else if (!_dragging && _gradient) {
		GdkCursor* cursor = nullptr;
		// check if mouse if over stop handle that we can adjust
		auto index = find_stop_at(event->x, event->y);
	// g_warning("idx: %d\n", int(index));
		if (index >= 0) {
			auto limits = get_stop_limits(index);
	// g_warning("lim: %f %f\n", limits.min_offset, limits.max_offset);
			if (limits.min_offset < limits.max_offset) {
				cursor = _cursor_mouseover.get()->gobj();
			}
		}
		gdk_window_set_cursor(event->window, cursor);
	}

	return false;
}

bool GradientWithStops::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
	auto allocation = get_allocation();
	auto context = get_style_context();
	const double scale = get_scale_factor();
	const auto layout = get_layout();

	if (layout.width <= 0) return true;

	context->render_background(cr, 0, 0, allocation.get_width(), allocation.get_height());
// context->render_frame(cr, 0, 0, width, height);

	// empty gradient checkboard or gradient itself
	cr->rectangle(layout.x, layout.y, layout.width, GRADIENT_IMAGE_HEIGHT);
	draw_gradient(cr, _gradient, layout.x, layout.width);

	if (!_gradient) return true;

	// draw stop handles

	cr->begin_new_path();

	Gdk::RGBA fg = context->get_color(get_state_flags());
	Gdk::RGBA bg = _background_color;

	_gradient->ensureVector();

	const auto& stops = _gradient->vector.stops;

	// stop handle outlines use theme colors:
	_template.set_style("path.outer", "fill", rgba_to_css_color(fg));
	_template.set_style("path.inner", "stroke", rgba_to_css_color(bg));

	for (size_t i = 0; i < stops.size(); ++i) {
		const auto& stop = stops[i];

		// stop handle shows stop color and opacity:
		_template.set_style("path.color", "fill", rgba_to_css_color(stop.color));
		_template.set_style("path.opacity", "opacity", double_to_css_value(stop.opacity));

		// render stop handle
		auto pix = _template.render(scale);

		if (!pix) {
			g_warning("Rendering gradient stop failed.");
			break;
		}

		// surface from pixbuf *without* scaling (scale = 1)
		auto surface = Gdk::Cairo::create_surface_from_pixbuf(pix, 1);
		if (!surface) continue;

		// calc space available for stop marker
		auto pos = get_stop_position(i, layout);
		cr->save();
		cr->rectangle(pos.left, layout.y, pos.right - pos.left, layout.height);
		cr->clip();
		// scale back to physical pixels
		cr->scale(1 / scale, 1 / scale);
		// paint bitmap
		cr->set_source(surface, round(pos.tip * scale - pix->get_width() / 2), pos.top * scale);
		cr->paint();
		cr->restore();
		cr->reset_clip();
	}

	return true;
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape