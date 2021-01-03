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
#include "gradient-chemistry.h"
#include "document-undo.h"
#include "verbs.h"

namespace Inkscape {
namespace UI {
namespace Widget {

using namespace Inkscape::IO;
using Inkscape::UI::Widget::ColorNotebook;

void set_icon(Gtk::Button& btn, gchar const* pixmap) {
	if (Gtk::Image* img = sp_get_icon_image(pixmap, Gtk::ICON_SIZE_BUTTON)) {
		btn.set_image(*img);
	}
	// btn.signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &PaintSelector::style_button_toggled), b));
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

// get widget from builder or throw
template<class W> W& get_widget(Glib::RefPtr<Gtk::Builder>& builder, const char* id) {
	W* widget;
	builder->get_widget(id, widget);
	if (!widget) {
		throw std::runtime_error("Missing widget in a glade resource file");
	}
	return *widget;
}

Glib::RefPtr<Gtk::Builder> create_builder() {
	auto glade = Resource::get_filename(Resource::UIS, "gradient-edit.glade");
	Glib::RefPtr<Gtk::Builder> builder;
	try {
		return Gtk::Builder::create_from_file(glade);
	}
	catch (Glib::Error& ex) {
		g_error(("Cannot load glade file for gradient editor. " + ex.what()).c_str());
		throw;
	}
}

GradientEditor::GradientEditor() :
	_builder(create_builder()),
	_selector(Gtk::manage(new GradientSelector())),
	// _stepOffset("Offset", 0.0, 0.0, 1.0, 0.01, 0.1, 2),
	_repeatIcon(get_widget<Gtk::Image>(_builder, "repeatIco")),
	_stopBox(get_widget<Gtk::Box>(_builder, "stopBox")),
	_popover(get_widget<Gtk::Popover>(_builder, "libraryPopover")),
	_stopTree(get_widget<Gtk::TreeView>(_builder, "stopList")),
	_offsetBtn(get_widget<Gtk::SpinButton>(_builder, "offsetSpin")),
	_showStopsList(get_widget<Gtk::Button>(_builder, "stopsBtn")),
	_addStop(get_widget<Gtk::Button>(_builder, "stopAdd")),
	_deleteStop(get_widget<Gtk::Button>(_builder, "stopDelete")),
	_stopsGallery(get_widget<Gtk::Box>(_builder, "stopsGallery")),
	_colorsBox(get_widget<Gtk::Box>(_builder, "colorsBox")),
	_mainGrid(get_widget<Gtk::Grid>(_builder, "mainGrid"))
{

	auto& linear = get_widget<Gtk::ToggleButton>(_builder, "linearBtn");
	set_icon(linear, INKSCAPE_ICON("paint-gradient-linear"));

	auto& radial = get_widget<Gtk::ToggleButton>(_builder, "radialBtn");
	set_icon(radial, INKSCAPE_ICON("paint-gradient-radial"));

	auto& reverse = get_widget<Gtk::Button>(_builder, "reverseBtn");
	set_icon(reverse, INKSCAPE_ICON("object-flip-horizontal"));
	reverse.signal_clicked().connect([=](){ reverse_gradient(); });

	auto pix = sp_get_icon_pixbuf("paint-none", GTK_ICON_SIZE_BUTTON);
	_repeatIcon.set(pix);

	// if (auto repeat = get_widget<Gtk::MenuButton>(builder, "repeatMode")) {
		//
		// set_button(repeat, INKSCAPE_ICON("object-flip-horizontal"), _("Reverse gradient's direction"));
	// }

	auto& gradBox = get_widget<Gtk::Box>(_builder, "gradientBox");
	const int dot_size = 8;
	_gradientImage.show();
	_gradientImage.set_margin_left(dot_size / 2);
	_gradientImage.set_margin_right(dot_size / 2);
	_gradientStops.draw_stops_only(true, dot_size);
	_gradientStops.set_margin_top(1);
	_gradientStops.set_size_request(-1, dot_size);
	_gradientStops.show();
	gradBox.pack_start(_gradientImage, true, true, 0);
	gradBox.pack_start(_gradientStops, true, true, 0);

	// add color selector
	Gtk::Widget* color_selector = Gtk::manage(new ColorNotebook(_selected_color));
	color_selector->show();
	_colorsBox.pack_start(*color_selector, true, true, 0);

	// _stepOffset.show();
	// _stopBox.pack_start(_stepOffset, true, true, 0);

	// gradient library in a popup
	_popover.add(*_selector);
	_selector->show();

	// construct store for a list of stops
	_stopColumns.add(_stopObj);
	_stopColumns.add(_stopID);
	_stopColumns.add(_stopColor);
	_stopListStore = Gtk::ListStore::create(_stopColumns);
	_stopTree.set_model(_stopListStore);
	_stopTree.append_column("n", _stopID); // 1-based step index
	_stopTree.append_column("c", _stopColor); // and its color

	auto selection = _stopTree.get_selection();
	selection->signal_changed().connect(sigc::mem_fun(*this, &GradientEditor::stop_selected));

	_showStopsList.signal_clicked().connect(sigc::mem_fun(this, &GradientEditor::toggle_stops));
	update_stops_layout();

	set_icon(_addStop, "list-add");
	_addStop.signal_clicked().connect(sigc::mem_fun(this, &GradientEditor::add_stop));

	set_icon(_deleteStop, "list-remove");
	_deleteStop.signal_clicked().connect(sigc::mem_fun(this, &GradientEditor::delete_stop));

	// connect gradient repeat modes menu
	std::tuple<const char*, SPGradientSpread> repeats[3] = {
		{"repeatNone", SP_GRADIENT_SPREAD_PAD},
		{"repeatDirect", SP_GRADIENT_SPREAD_REPEAT},
		{"repeatReflected", SP_GRADIENT_SPREAD_REFLECT}
	};
	for (auto& el : repeats) {
		auto& item = get_widget<Gtk::MenuItem>(_builder, std::get<0>(el));
		auto mode = std::get<1>(el);
		item.signal_select().connect([=](){ set_repeat_mode(mode); });
	}

	// auto& repeatNone = get_widget<Gtk::MenuItem>(_builder, "repeatNone");
	// repeatNone.signal_select().connect([this](){ setRepeatMode(SP_GRADIENT_SPREAD_PAD); });
	
	// _offsetBtn.set_range(0.0, 1.0);

	pack_start(_mainGrid);
}

GradientEditor::~GradientEditor() {
}

GradientSelector* GradientEditor::get_selector() {
	return _selector;
}

void GradientEditor::stop_selected() {
	auto sel = _stopTree.get_selection();
	if (!sel) return;

	auto it = sel->get_selected();
	if (it) {
		auto row = *it;
		const SPGradientStop& stop = row[_stopObj];
		_selected_color.setColor(stop.color);
		_selected_color.setAlpha(stop.opacity);

		// _stepOffset.set_value(stop.offset);
		_offsetBtn.set_value(stop.offset);
	}
}

void GradientEditor::add_stop() {
	//
}

void GradientEditor::delete_stop() {
	//
}

void GradientEditor::toggle_stops() {
	_stopsListVisible = !_stopsListVisible;
	update_stops_layout();
}

void GradientEditor::update_stops_layout() {
	const int top = 3;

	if (_stopsListVisible) {
		// shrink color box
		_mainGrid.remove(_colorsBox);
		_mainGrid.attach(_colorsBox, 1, top);
		set_icon(_showStopsList, "go-previous");
		_stopsGallery.show();
	}
	else {
		set_icon(_showStopsList, "go-next");
		_stopsGallery.hide();
		// expand color box
		_mainGrid.remove(_colorsBox);
		_mainGrid.attach(_colorsBox, 0, top, 2);
	}
}

void GradientEditor::reverse_gradient() {
	if (_document && _gradient) {
		sp_gradient_reverse_vector(_gradient);
		DocumentUndo::done(_document, SP_VERB_CONTEXT_GRADIENT, _("Reverse gradient"));
	}
}

void GradientEditor::set_repeat_mode(SPGradientSpread mode) {
	// g_warning("selected.");
	//
}

void GradientEditor::set_repeat_icon(SPGradientSpread mode) {
	auto pix = sp_get_icon_pixbuf("paint-none", GTK_ICON_SIZE_BUTTON);
	_repeatIcon.set(pix);
}


SPGradient* GradientEditor::getVector() {
	return _selector->getVector();
}

void GradientEditor::setVector(SPDocument* doc, SPGradient* vector) {
	_document = doc;
	_gradient = vector;
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
	const int size = 30;
	for (const SPGradientStop& stop : stops) {
		auto it = _stopListStore->append();
		it->set_value(_stopObj, stop);
		it->set_value(_stopID, Glib::ustring::compose("%1.", index++));
		it->set_value(_stopColor, drawCircle(size, stop.color.toRGBA32(stop.opacity)));
	}

	_gradientImage.set_gradient(gradient);
	_gradientStops.set_gradient(gradient);
}


} // namespace Widget
} // namespace UI
} // namespace Inkscape