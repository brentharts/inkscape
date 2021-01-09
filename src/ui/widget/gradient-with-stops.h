// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_GRADIENT_WITH_STOPS_H
#define SEEN_GRADIENT_WITH_STOPS_H

#include <gtkmm/widget.h>
#include <gdkmm/color.h>
#include "ui/svg-renderer.h"
#include "helper/auto-connection.h"

class SPGradient;

namespace Inkscape {
namespace UI {
namespace Widget {

class GradientWithStops : public Gtk::DrawingArea {
public:
	GradientWithStops();

	// gradient to draw or nullptr
	void set_gradient(SPGradient* gradient);

	// stop has been selected
	sigc::signal<void (size_t)>& signal_stop_selected() {
		return _signal_stop_selected;
	}

	// request to change stop's offset
	sigc::signal<void (size_t, double)>& signal_stop_offset_changed() {
		return _signal_stop_offset_changed;
	}

private:
	void get_preferred_width_vfunc(int& minimum_width, int& natural_width) const override;
	void get_preferred_height_vfunc(int& minimum_height, int& natural_height) const override;
	bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;
	void on_style_updated() override;
	bool on_button_press_event(GdkEventButton* event) override;
	bool on_button_release_event(GdkEventButton* event) override;
	bool on_motion_notify_event(GdkEventMotion* event) override;
	bool on_focus(Gtk::DirectionType direction) override;
	void size_request(GtkRequisition* requisition) const;
	// repaint widget
	void update();
	// index of gradient stop handle under (x, y) or -1
	int find_stop_at(double x, double y) const;

	// layout of gradient image/editor
	struct layout_t {
		double x, y, width, height;
	};
	layout_t get_layout() const;

	// position of single gradient stop handle
	struct stop_pos_t {
		double left, tip, right, top, bottom;
	};
	stop_pos_t get_stop_position(size_t index, const layout_t& layout) const;

	struct limits_t {
		double min_offset, max_offset;
	};
	limits_t get_stop_limits(size_t index) const;

	SPGradient* _gradient = nullptr;
	svg_renderer _template;
	auto_connection _release;
	auto_connection _modified;
	Gdk::RGBA _background_color;
	sigc::signal<void (size_t)> _signal_stop_selected;
	sigc::signal<void (size_t, double)> _signal_stop_offset_changed;
	bool _dragging = false;
	size_t _selected_stop = 0;
	double _pointer_x = 0;
	double _stop_offset = 0;
	Glib::RefPtr<Gdk::Cursor> _cursor_mouseover;
	Glib::RefPtr<Gdk::Cursor> _cursor_dragging;
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif
