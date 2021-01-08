#include <string>

#include "gradient-with-stops.h"
#include "object/sp-gradient.h"
#include "display/cairo-utils.h"
#include "io/resource.h"

// need access to the main window to read its style
#include "desktop.h"
#include "inkscape.h"

const int GRADIENT_WIDGET_HEIGHT = 32;
const int GRADIENT_IMAGE_HEIGHT = 19;

namespace Inkscape {
namespace UI {
namespace Widget {

using namespace Inkscape::IO;

std::string get_stop_template_path() {
	return Resource::get_filename(Resource::UIS, "gradient-stop.svg");
}

GradientWithStops::GradientWithStops():
	_template(get_stop_template_path().c_str()) {
	set_has_window(false);
}

void GradientWithStops::set_gradient(SPGradient* gradient) {
	_gradient = gradient;

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

Gdk::RGBA get_background_color() {
	Gdk::RGBA bg;
	bg.set_grey(0.5);

	auto desktop = SP_ACTIVE_DESKTOP;
	if (desktop) {
		Gtk::Window* window = desktop->getToplevel();
		if (window) {
			bg = window->get_style_context()->get_background_color();
		}
	}

	return bg;
}

bool GradientWithStops::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
	auto allocation = get_allocation();
	auto context = get_style_context();

	const double scale = get_scale_factor();
	const auto stop_width = _template.get_width_px();
	const auto half_stop = round((stop_width + 1) / 2);
	const auto x = half_stop;
	const int width = allocation.get_width() - stop_width;
	const int height = allocation.get_height();
	if (width <= 0) return true;

	context->render_background(cr, 0, 0, width, height);
// context->render_frame(cr, 0, 0, width, height);

	// empty gradient checkboard or gradient itself
	cr->rectangle(x, 0, width, GRADIENT_IMAGE_HEIGHT);
	draw_gradient(cr, _gradient, x, width);

	if (!_gradient) return true;

	// draw stops

	cr->begin_new_path();

	Gdk::RGBA fg = context->get_color(get_state_flags());
	Gdk::RGBA bg = get_background_color();

	_gradient->ensureVector();

	const auto& stops = _gradient->vector.stops;
	// stop position
	auto pos = [=](double offset) { return round(x + width * CLAMP(offset, 0, 1)); };

	_template.set_style("path.outer", "fill", rgba_to_css_color(fg));
	_template.set_style("path.inner", "stroke", rgba_to_css_color(bg));

	double left = x - half_stop;
	for (size_t i = 0; i < stops.size(); ++i) {
		const auto& stop = stops[i];

		_template.set_style("path.color", "fill", rgba_to_css_color(stop.color));
		_template.set_style("path.opacity", "opacity", double_to_css_value(stop.opacity));

		auto pix = _template.render(scale);
		const auto y = (GRADIENT_WIDGET_HEIGHT - _template.get_height_px()) * scale;

		if (!pix) {
			g_warning("Rendering gradient stop failed.");
			break;
		}

		// surface from pixbuf *without* scaling (scale = 1)
		auto surface = Gdk::Cairo::create_surface_from_pixbuf(pix, 1);
		if (!surface) continue;

		// calc space available for stop marker
		cr->save();
		auto right = pos(stop.offset) + half_stop;
		auto next = i + 1 < stops.size() ? pos(stops[i + 1].offset) - half_stop : allocation.get_width();
		if (next < right) {
			// stops overlap, limit extent
			right = (next + right) / 2;
		}
		cr->rectangle(left, 0, right - left, height);
		cr->clip();
		// scale back to physical pixels
		cr->scale(1 / scale, 1 / scale);
		// paint bitmap
		cr->set_source(surface, pos(stop.offset) * scale - pix->get_width() / 2, y);
		cr->paint();
		cr->restore();

		cr->reset_clip();
		left = right;
	}

	return true;
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape