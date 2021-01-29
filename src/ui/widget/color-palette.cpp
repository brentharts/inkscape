#include <gtkmm/box.h>
#include <gtkmm/popover.h>
#include <gtkmm/scale.h>
#include <gtkmm/cssprovider.h>

#include "color-palette.h"
#include "ui/builder-utils.h"

namespace Inkscape {
namespace UI {
namespace Widget {

ColorPalette::ColorPalette():
	_builder(create_builder("color-palette.glade")),
	_flowbox(get_widget<Gtk::FlowBox>(_builder, "flow-box")),
	_menu(get_widget<Gtk::Menu>(_builder, "menu")),
	_scroll_btn(get_widget<Gtk::FlowBox>(_builder, "scroll-buttons")),
	_scroll(get_widget<Gtk::ScrolledWindow>(_builder, "scroll-wnd"))
	{

	auto& box = get_widget<Gtk::Box>(_builder, "palette-box");
	this->add(box);

	auto& config = get_widget<Gtk::MenuItem>(_builder, "config");
	auto& dlg = get_widget<Gtk::Popover>(_builder, "config-popup");
	config.signal_activate().connect([=,&dlg](){
		dlg.popup();
	});
	auto& size = get_widget<Gtk::Scale>(_builder, "size-slider");
	size.signal_change_value().connect([=,&size](Gtk::ScrollType, double val) {
		set_tile_size(static_cast<int>(val)); return true; });

	auto& border = get_widget<Gtk::Scale>(_builder, "border-slider");
	border.signal_change_value().connect([=,&border](Gtk::ScrollType, double val) {
		set_tile_border(static_cast<int>(val)); return true; });

	auto& rows = get_widget<Gtk::Scale>(_builder, "row-slider");
	rows.signal_change_value().connect([=,&rows](Gtk::ScrollType, double val) {
		set_rows(static_cast<int>(val)); return true; });

	_scroll.set_min_content_height(1);

	auto css_provider = Gtk::CssProvider::create();
	css_provider->load_from_data(
	"flowboxchild {"
	" padding: 0;"
	" min-width: 0;"
	" min-height: 0;"
	"}"
	"scrolledwindow {"
	" padding: 0;"
	// " min-content-height: 1px;"
	"}"
	"menubutton {"
	" padding: 0;"
	"}"
	);
	this->get_style_context()->add_provider(css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	set_up_scrolling();
}

void ColorPalette::set_tile_border(int border) {
	if (border == _size) return;

	if (border < 1 || border > 100) {
		g_warning("Unexpected tile border size of color palette: %d", border);
		return;
	}

	_border = border;
	resize();
}

void ColorPalette::set_tile_size(int size) {
	if (size == _size) return;

	if (size < 1 || size > 1000) {
		g_warning("Unexpected tile size for color palette: %d", size);
		return;
	}

	_size = size;
	resize();
}

void ColorPalette::set_rows(int rows) {
	if (rows == _rows) return;

	if (rows < 1 || rows > 1000) {
		g_warning("Unexpected number of rows for color palette: %d", rows);
		return;
	}

	_rows = rows;
	set_up_scrolling();
}

void ColorPalette::set_up_scrolling() {
	if (_rows == 1) {
		// horizontal scrolling with single row
		_scroll_btn.hide();
		_scroll.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_NEVER);
		int count = 0;
		_flowbox.foreach([&](Gtk::Widget&){ ++count; });
		_flowbox.set_min_children_per_line(std::max(1, count));
	}
	else {
		// vertical scrolling with multiple rows
		_scroll.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_EXTERNAL);
		_flowbox.set_min_children_per_line(1);
		_scroll_btn.show();
	}

	resize();
}

void ColorPalette::resize() {
	int height = (_size + _border) * _rows;
	_scroll.set_size_request(1, height);

	_flowbox.foreach([=](Gtk::Widget& w){
		w.set_size_request(_size, _size);
	});
}

void ColorPalette::free() {
	for (auto widget : _flowbox.get_children()) {
		if (widget) {
			_flowbox.remove(*widget);
			delete widget;
		}
	}
}

void ColorPalette::set_colors(const std::vector<Gtk::Widget*>& swatches) {
	free();

	int count = 0;
	for (auto widget : swatches) {
		if (widget) {
			_flowbox.add(*widget);
			++count;
		}
	}	

	if (_rows == 1) {
		_flowbox.set_min_children_per_line(std::max(1, count));
	}

	resize();
}

}}} // namespace
