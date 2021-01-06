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
#include "object/sp-linear-gradient.h"
#include "object/sp-gradient-vector.h"
#include "svg/css-ostringstream.h"

namespace Inkscape {
namespace UI {
namespace Widget {

using namespace Inkscape::IO;
using Inkscape::UI::Widget::ColorNotebook;

class scope {
public:
	scope(bool& flag): _flag(flag) {
		flag = true;
	}

	~scope() {
		_flag = false;
	}

private:
	bool& _flag;
};

void set_icon(Gtk::Button& btn, gchar const* pixmap) {
	if (Gtk::Image* img = sp_get_icon_image(pixmap, Gtk::ICON_SIZE_BUTTON)) {
		btn.set_image(*img);
	}
	// btn.signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &PaintSelector::style_button_toggled), b));
}

// draw solid color circle with black outline; right side is to show checkerboard if color's alpha is > 0
Glib::RefPtr<Gdk::Pixbuf> draw_circle(int size, guint32 rgba) {
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


Glib::RefPtr<Gdk::Pixbuf> get_stop_pixmap(SPStop* stop) {
	const int size = 30;
	return draw_circle(size, stop->getColor().toRGBA32(stop->getOpacity()));
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

Glib::ustring get_repeat_icon(SPGradientSpread mode) {
	const char* ico = "";
	switch (mode) {
		case SP_GRADIENT_SPREAD_PAD:
			ico = "gradient-spread-pad";
			break;
		case SP_GRADIENT_SPREAD_REPEAT:
			ico = "gradient-spread-repeat";
			break;
		case SP_GRADIENT_SPREAD_REFLECT:
			ico = "gradient-spread-reflect";
			break;
		default:
			g_warning("Missing case in %s\n", __func__);
			break;
	}
	return ico;
}

GradientEditor::GradientEditor() :
	_builder(create_builder()),
	_selector(Gtk::manage(new GradientSelector())),
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

	auto& gradBox = get_widget<Gtk::Box>(_builder, "gradientBox");
	const int dot_size = 8;
	_gradientImage.show();
	_gradientImage.set_margin_left(dot_size / 2);
	_gradientImage.set_margin_right(dot_size / 2);
	// _gradientStops.draw_stops_only(true, dot_size);
	// _gradientStops.set_margin_top(1);
	// _gradientStops.set_size_request(-1, dot_size);
	// _gradientStops.show();
	gradBox.pack_start(_gradientImage, true, true, 0);
	// gradBox.pack_start(_gradientStops, true, true, 0);

	// add color selector
	Gtk::Widget* color_selector = Gtk::manage(new ColorNotebook(_selected_color));
	color_selector->show();
	_colorsBox.pack_start(*color_selector, true, true, 0);

	// gradient library in a popup
	_popover.add(*_selector);
	_selector->show();

	// construct store for a list of stops
	_stopColumns.add(_stopObj);
	_stopColumns.add(_stopIdx);
	_stopColumns.add(_stopID);
	_stopColumns.add(_stopColor);
	_stopListStore = Gtk::ListStore::create(_stopColumns);
	_stopTree.set_model(_stopListStore);
	_stopTree.append_column("n", _stopID); // 1-based stop index
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
		item.signal_activate().connect([=](){ set_repeat_mode(mode); });
		// pack icon and text into MenuItem, since MenuImageItem is deprecated
		auto text = item.get_label();
		auto hbox = Gtk::manage(new Gtk::Box);
		Gtk::Image* img = sp_get_icon_image(get_repeat_icon(mode), Gtk::ICON_SIZE_BUTTON);
		hbox->pack_start(*img, false, true, 8);
		auto label = Gtk::manage(new Gtk::Label);
		label->set_label(text);
		hbox->pack_start(*label, false, true, 0);
		hbox->show_all();
		item.remove();
		item.add(*hbox);
	}

	set_repeat_icon(SP_GRADIENT_SPREAD_PAD);
	
	_selected_color.signal_changed.connect([=]() {
		set_stop_color(_selected_color.color(), _selected_color.alpha());
	});
	_selected_color.signal_dragged.connect([=]() {
		set_stop_color(_selected_color.color(), _selected_color.alpha());
	});

	// _offsetBtn.set_range(0.0, 1.0);

	pack_start(_mainGrid);
}

GradientEditor::~GradientEditor() {
}

GradientSelector* GradientEditor::get_selector() {
	return _selector;
}

void set_gradient_stop_color(SPDocument* document, SPStop* stop, SPColor color, double opacity) {
	sp_repr_set_css_double(stop->getRepr(), "offset", stop->offset);
	Inkscape::CSSOStringStream os;
	os << "stop-color:" << color.toString() << ";stop-opacity:" << opacity <<";";
	stop->setAttribute("style", os.str());

	DocumentUndo::done(document, SP_VERB_CONTEXT_GRADIENT, _("Change gradient stop color"));
}

void GradientEditor::set_stop_color(SPColor color, float opacity) {
	if (_update.pending()) return;

	if (auto row = current_stop()) {
		SPStop* stop = row->get_value(_stopObj);
		if (stop && _document) {
			auto scoped(_update.block());

			set_gradient_stop_color(_document, stop, color, opacity);

			// update list view too
			row->set_value(_stopColor, get_stop_pixmap(stop));
		}
	}
}

std::optional<Gtk::TreeRow> GradientEditor::current_stop() {
	auto sel = _stopTree.get_selection();
	auto it = sel->get_selected();
	if (!it) {
		return std::nullopt;
	}
	else {
		return *it;
	}
}

void GradientEditor::stop_selected() {
	// if (_update) return;

	if (auto row = current_stop()) {
		SPStop* stop = row->get_value(_stopObj);
		if (stop) {
			auto scoped(_update.block());

			_selected_color.setColor(stop->getColor());
			_selected_color.setAlpha(stop->getOpacity());

			_offsetBtn.set_value(stop->offset);
		}
	}
}

void GradientEditor::add_stop() {
	//
}

void GradientEditor::delete_stop() {
	//
}

// collapse/expand list of stops in the UI
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
		// reverse works on a gradient definition, the one with stops:
		SPGradient* vector = _gradient->getVector();

		if (vector) {
			sp_gradient_reverse_vector(vector);
			DocumentUndo::done(_document, SP_VERB_CONTEXT_GRADIENT, _("Reverse gradient"));
		}
	}
}

void GradientEditor::set_repeat_mode(SPGradientSpread mode) {
	if (_update.pending()) return;

	if (_document && _gradient) {
		auto scoped(_update.block());

		// spread is set on a gradient reference, which is _gradient object
		_gradient->setSpread(mode);
		_gradient->updateRepr();

		DocumentUndo::done(_document, SP_VERB_CONTEXT_GRADIENT, _("Set gradient repeat"));

		set_repeat_icon(mode);
	}
}

void GradientEditor::set_repeat_icon(SPGradientSpread mode) {
	auto ico = get_repeat_icon(mode);
	if (!ico.empty()) {
		_repeatIcon.set_from_icon_name(ico, Gtk::ICON_SIZE_BUTTON);
	}
}


void GradientEditor::setGradient(SPGradient* gradient) {
	auto scoped(_update.block());
	_gradient = gradient;
	_document = gradient ? gradient->document : nullptr;
	set_gradient(gradient);
}

SPGradient* GradientEditor::getVector() {
	return _selector->getVector();
}

void GradientEditor::setVector(SPDocument* doc, SPGradient* vector) {
	auto scoped(_update.block());
	_selector->setVector(doc, vector);
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
// g_warning("set grad\n");
	size_t selected_stop_index = 0;
	{
		auto it = _stopTree.get_selection()->get_selected();
		if (it) {
			selected_stop_index = it->get_value(_stopIdx);
		}
	}

	_stopListStore->clear();

	SPGradient* vector = gradient ? gradient->getVector() : nullptr;

	if (vector) {
		vector->ensureVector();
	}

	if (!vector || !vector->hasStops()) return;

	size_t index = 0;
	for (auto& child : vector->children) {
		if (SP_IS_STOP(&child)) {
			auto stop = SP_STOP(&child);
			auto it = _stopListStore->append();
			it->set_value(_stopObj, stop);
			it->set_value(_stopIdx, index);
			it->set_value(_stopID, Glib::ustring::compose("%1.", index + 1));
			it->set_value(_stopColor, get_stop_pixmap(stop));

			++index;
		}
	}

	_gradientImage.set_gradient(vector);
	// _gradientStops.set_gradient(vector);

	auto mode = gradient->isSpreadSet() ? gradient->getSpread() : SP_GRADIENT_SPREAD_PAD;
	set_repeat_icon(mode);

	if (index > 0) {
		auto it = _stopTree.get_model()->children().begin();
		std::advance(it, std::min(selected_stop_index, index - 1));
		_stopTree.get_selection()->select(it);
	}
}


} // namespace Widget
} // namespace UI
} // namespace Inkscape