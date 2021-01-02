// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Gradient editor widget for "Fill and Stroke" dialog
 *
 * Author:
 *   Michael Kowalski
 *
 * Copyright (C) 2020-2021 Michael Kowalski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm/builder.h>
#include <gtkmm/grid.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/button.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treemodelcolumn.h>
#include <glibmm/i18n.h>
#include <cairo.h>

#include "display/cairo-utils.h"
#include "gradient-editor.h"
#include "gradient-selector.h"
#include "io/resource.h"
#include "color-notebook.h"
#include "ui/icon-names.h"
#include "ui/icon-loader.h"
#include "color-preview.h"

namespace Inkscape {
namespace UI {
namespace Widget {

using namespace Inkscape::IO;
using Inkscape::UI::Widget::ColorNotebook;

void set_button(Gtk::Button* btn, gchar const* pixmap, gchar const* tip) {
	if (tip) {
		btn->set_tooltip_text(tip);
	}

	if (Gtk::Image* img = sp_get_icon_image(pixmap, Gtk::ICON_SIZE_BUTTON)) {
		btn->set_image(*img);
	}

	// btn->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &PaintSelector::style_button_toggled), b));
}

// draw solid color circle with black outline; right side is to show checkerboard if color's alpha is > 0
Glib::RefPtr<Gdk::Pixbuf> drawCircle(int size, guint32 rgba) {
	int width = size;
	int height = size;
	gint w2 = width / 2;

	cairo_surface_t* s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cairo_t* cr = cairo_create(s);

	int x = 0, y = 0;
	double radius = size / 2;
	double degrees = M_PI / 180.0;
	cairo_new_sub_path(cr);
	cairo_arc(cr, x + radius, y + radius, radius, 0, 2 * M_PI);
	cairo_close_path(cr);
	// semi-transparent black outline
	cairo_set_source_rgba(cr, 0, 0, 0, 0.3);
	cairo_fill(cr);

	radius--;

	cairo_new_sub_path(cr);
	cairo_line_to(cr, x + w2, 0);
	cairo_line_to(cr, x + w2, height);
	cairo_arc(cr, x + w2, y + w2, radius, 90 * degrees, 270 * degrees);
	cairo_close_path(cr);

	// solid part
	ink_cairo_set_source_rgba32(cr, rgba | 0xff);
	cairo_fill(cr);

	x = w2;

	cairo_new_sub_path(cr);
	cairo_arc(cr, x, y + w2, radius, -90 * degrees, 90 * degrees);
	cairo_line_to(cr, x, y);
	cairo_close_path(cr);

	// (semi)transparent part
	if ((rgba & 0xff) != 0xff) {
		cairo_pattern_t* checkers = ink_cairo_pattern_create_checkerboard();
		cairo_set_source(cr, checkers);
		cairo_fill_preserve(cr);
		cairo_pattern_destroy(checkers);
	}
	ink_cairo_set_source_rgba32(cr, rgba);
	cairo_fill(cr);
	
	cairo_destroy(cr);
	cairo_surface_flush(s);

	GdkPixbuf* pixbuf = ink_pixbuf_create_from_cairo_surface(s);
	return Glib::wrap(pixbuf);
}

template<class W> W* get_widget(Glib::RefPtr<Gtk::Builder>& builder, const char* id) {
	W* widget;
	builder->get_widget(id, widget);
	return widget;
}

GradientEditor::GradientEditor() :
	_stepOffset("Offset", 0.0, 0.0, 1.0, 0.01, 0.1, 2)
{
	_selector = Gtk::manage(new GradientSelector());

	auto glade = Resource::get_filename(Resource::UIS, "gradient-edit.glade");
	Glib::RefPtr<Gtk::Builder> builder;
	try {
		builder = Gtk::Builder::create_from_file(glade);
	}
	catch (Glib::Error& ex) {
		g_error(("Cannot load glade file for gradient editor. " + ex.what()).c_str());
		return;
	}

	if (auto linear = get_widget<Gtk::ToggleButton>(builder, "linearBtn")) {
		set_button(linear, INKSCAPE_ICON("paint-gradient-linear"), _("Linear gradient"));
	}

	if (auto radial = get_widget<Gtk::ToggleButton>(builder, "radialBtn")) {
		set_button(radial, INKSCAPE_ICON("paint-gradient-radial"), _("Radial gradient"));
	}

	if (auto reverse = get_widget<Gtk::Button>(builder, "reverseBtn")) {
		set_button(reverse, INKSCAPE_ICON("object-flip-horizontal"), _("Reverse gradient's direction"));
	}

	if (auto repeatIco = get_widget<Gtk::Image>(builder, "repeatIco")) {
		_repeatIcon = repeatIco;
		auto pix = sp_get_icon_pixbuf("paint-none", GTK_ICON_SIZE_BUTTON);
		repeatIco->set(pix);
	}

	// if (auto repeat = get_widget<Gtk::MenuButton>(builder, "repeatMode")) {
		//
		// set_button(repeat, INKSCAPE_ICON("object-flip-horizontal"), _("Reverse gradient's direction"));
	// }

	if (auto box = get_widget<Gtk::Box>(builder, "gradientBox")) {
		int dot_size = 8;
		_gradientImage.show();
		_gradientImage.set_margin_left(dot_size / 2);
		_gradientImage.set_margin_right(dot_size / 2);
		_gradientStops.draw_stops_only(true, dot_size);
		_gradientStops.set_margin_top(1);
		_gradientStops.set_size_request(-1, dot_size);
		_gradientStops.show();
		box->pack_start(_gradientImage, true, true, 0);
		box->pack_start(_gradientStops, true, true, 0);
	}

	if (auto colors = get_widget<Gtk::Box>(builder, "colorsBox")) {
		// add color selector
		Gtk::Widget* color_selector = Gtk::manage(new ColorNotebook(_selected_color));
		color_selector->show();
		colors->pack_start(*color_selector, true, true, 0);
	}

	if (auto stopBox = get_widget<Gtk::Box>(builder, "stopBox")) {
		_stepOffset.show();
		stopBox->pack_start(_stepOffset, true, true, 0);
	}

	if (auto popover = get_widget<Gtk::Popover>(builder, "libraryPopover")) {
		_popover = popover;
		_popover->add(*_selector);
		_selector->show();
	}
	
	if (auto stops = get_widget<Gtk::TreeView>(builder, "stopList")) {
		_stopTree = stops;
		_stopColumns.add(_stopObj);
		_stopColumns.add(_stopID);
		_stopColumns.add(_stopColor);
		_stopListStore = Gtk::ListStore::create(_stopColumns);
		stops->set_model(_stopListStore);
		stops->append_column("n", _stopID); // 1-based step index
		stops->append_column("c", _stopColor); // and its color

		auto sel = stops->get_selection();
		sel->signal_changed().connect(sigc::mem_fun(*this, &GradientEditor::stop_selected));

		// auto it = _stopListStore->append();
		// it->set_value(_stopID, Glib::ustring("1."));
		// it->set_value(_stopColor, drawCircle(28, 0xff000080));
		// it = _stopListStore->append();
		// it->set_value(_stopID, Glib::ustring("2."));
		// it->set_value(_stopColor, drawCircle(28, 0x00ff0000));
		// it = _stopListStore->append();
		// it->set_value(_stopID, Glib::ustring("13."));
		// it->set_value(_stopColor, drawCircle(28, 0xffff00ff));
	}

	if (_addStop = get_widget<Gtk::Button>(builder, "stopAdd")) {
		set_button(_addStop, "list-add", nullptr);
		_addStop->signal_clicked().connect(sigc::mem_fun(this, &GradientEditor::add_stop));
	}
	if (_deleteStop = get_widget<Gtk::Button>(builder, "stopDelete")) {
		set_button(_deleteStop, "list-remove", nullptr);
		_deleteStop->signal_clicked().connect(sigc::mem_fun(this, &GradientEditor::delete_stop));
	}
	if (_duplicateStop = get_widget<Gtk::Button>(builder, "stopDup")) {
		set_button(_duplicateStop, "edit-copy", nullptr);
		_duplicateStop->signal_clicked().connect(sigc::mem_fun(this, &GradientEditor::duplicate_stop));
	}

	// if (auto store = get_widget<Gtk::ListStore>(builder, "stopListStore")) {
		//
	// }

	if (auto grid = get_widget<Gtk::Grid>(builder, "mainGrid")) {
		pack_start(*grid);
	}
}

GradientEditor::~GradientEditor() {
}

GradientSelector* GradientEditor::get_selector() {
	return _selector;
}

void GradientEditor::stop_selected() {
	auto sel = _stopTree->get_selection();
	if (!sel) return;

	auto it = sel->get_selected();
	if (it) {
		auto row = *it;
		const SPGradientStop& stop = row[_stopObj];
		_selected_color.setColor(stop.color);
		_selected_color.setAlpha(stop.opacity);

		_stepOffset.set_value(stop.offset);
	}
}

void GradientEditor::add_stop() {
	//
}

void GradientEditor::delete_stop() {
	//
}

void GradientEditor::duplicate_stop() {
	//
}

SPGradient* GradientEditor::getVector() {
	return _selector->getVector();
}

void GradientEditor::setVector(SPDocument* doc, SPGradient* vector) {
	_selector->setVector(doc, vector);
	set_gradient(vector);
}

void GradientEditor::setMode(SelectorMode mode) {
	_selector->setMode(mode);
}

void GradientEditor::setUnits(SPGradientUnits units) {
	_selector->setUnits(units);
}

SPGradientUnits GradientEditor::getUnits() {
	return _selector->getUnits();
}

void GradientEditor::setSpread(SPGradientSpread spread) {
	_selector->setSpread(spread);
}

SPGradientSpread GradientEditor::getSpread() {
	return _selector->getSpread();
}

void GradientEditor::set_gradient(SPGradient* gradient) {
	_stopListStore->clear();

	if (!gradient || !gradient->hasStops()) return;

	gradient->ensureVector();
	const auto& stops = gradient->vector.stops;

	int index = 1;
	for (const SPGradientStop& stop : stops) {
		auto it = _stopListStore->append();
		it->set_value(_stopObj, stop);
		it->set_value(_stopID, Glib::ustring::compose("%1.", index++));
		it->set_value(_stopColor, drawCircle(30, stop.color.toRGBA32(stop.opacity)));
	}

	_gradientImage.set_gradient(gradient);
	_gradientStops.set_gradient(gradient);
}


} // namespace Widget
} // namespace UI
} // namespace Inkscape