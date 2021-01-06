#include <string>

#include "gradient-with-stops.h"
#include "object/sp-gradient.h"

namespace Inkscape {
namespace UI {
namespace Widget {

std::string get_stop_template_path() {
	return "/home/mike/Documents/gradient-stop.svg";
}

GradientWithStops::GradientWithStops():
	_template(get_stop_template_path().c_str()) {
	set_has_window(false);
}

void GradientWithStops::set_gradient(SPGradient* gradient) {
	_gradient = gradient;
	update();
}

void GradientWithStops::size_request(GtkRequisition* requisition) const {
	requisition->width = 60;
	requisition->height = 30;
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

bool GradientWithStops::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
	if (!_gradient) return true;

	auto allocation = get_allocation();

	auto context = get_style_context();
	Gdk::RGBA fg;
	if (!context->lookup_color("theme_fg_color", fg)) {
		fg.set_rgba(0.5, 0.5, 0.5);
	}

	Gdk::RGBA bg;
	if (!context->lookup_color("theme_bg_color", bg)) {
		bg.set_rgba(0.5, 0.5, 0.5);
	}
	char buf[16];
	// assert()
	_template.set_style("path.outer", "fill", rgba_to_css_color(fg, buf));
	_template.set_style("path.inner", "stroke", rgba_to_css_color(bg, buf));
	_template.set_style("path.color", "fill", "#ff0000");// rgba_to_css_color(bg));

	double scale = get_scale_factor();
	auto pix = _template.render(scale);

	if (pix) {
		auto surface = Gdk::Cairo::create_surface_from_pixbuf(pix, 1);
		cr->save();
		cr->scale(1 / scale, 1 / scale);
		cr->set_source(surface, 0, 0);
		cr->paint();
		cr->restore();
	}

	_template.set_style("path.color", "fill", "#00ff00");// rgba_to_css_color(bg));
	pix = _template.render(scale);

	if (pix) {
		auto surface = Gdk::Cairo::create_surface_from_pixbuf(pix, 1);
		cr->save();
		cr->scale(1 / scale, 1 / scale);
		cr->set_source(surface, 50 * scale, 0);
		cr->paint();
		cr->restore();
	}

	return true;
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape