// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_GRADIENT_WITH_STOPS_H
#define SEEN_GRADIENT_WITH_STOPS_H

#include <gtkmm/widget.h>
#include "ui/svg-renderer.h"

class SPGradient;

namespace Inkscape {
namespace UI {
namespace Widget {

class GradientWithStops : public Gtk::Widget {
public:
	GradientWithStops();

	// gradient to draw or nullptr
	void set_gradient(SPGradient* gradient);

protected:
	void get_preferred_width_vfunc(int& minimum_width, int& natural_width) const override;
	void get_preferred_height_vfunc(int& minimum_height, int& natural_height) const override;
	bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

private:
	void size_request(GtkRequisition* requisition) const;
	void update();

	SPGradient* _gradient = nullptr;
	svg_renderer _template;
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif
