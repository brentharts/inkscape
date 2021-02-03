#include <gtkmm/box.h>
#include <gtkmm/popover.h>
#include <gtkmm/scale.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/button.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/menu.h>

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
	_scroll_left(get_widget<Gtk::Button>(_builder, "btn-left")),
	_scroll_right(get_widget<Gtk::Button>(_builder, "btn-right")),
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
		set_tile_size(static_cast<int>(size.get_value())); return true; });

	auto& border = get_widget<Gtk::Scale>(_builder, "border-slider");
	border.signal_change_value().connect([=,&border](Gtk::ScrollType, double val) {
		set_tile_border(static_cast<int>(border.get_value())); return true; });

	auto& rows = get_widget<Gtk::Scale>(_builder, "row-slider");
	rows.signal_change_value().connect([=,&rows](Gtk::ScrollType, double val) {
		set_rows(static_cast<int>(rows.get_value())); return true; });

	_scroll.set_min_content_height(1);

	auto css_provider = Gtk::CssProvider::create();
	css_provider->load_from_data(
	"flowboxchild, flowbox, scrolledwindow {"
	" padding: 0;"
	" border: 0;"
	" margin: 0;"
	" min-width: 1px;"
	" min-height: 1px;"
	"}\n"
	// "flowbox { background: #ff0000; } "
	// "scrolledwindow { background: #0000ff; } "
	// "bin { background: #ffff00; } "
	// "box.main-box { background: #ff4ff0; border: 1px solid #ff0000; } "
	".small {"
	" padding: 1px;"
	" margin: 0;"
	"}"
	);
	// this->get_style_context()->add_provider(css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	get_style_context()->add_provider_for_screen(this->get_screen(), css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	get_widget<Gtk::Button>(_builder, "btn-down").signal_clicked().connect([=](){ scroll(0, _size + _border); });
	get_widget<Gtk::Button>(_builder, "btn-up").signal_clicked().connect([=](){ scroll(0, -(_size + _border)); });

	// free();
	set_up_scrolling();
	// auto vert = _scroll.get_vscrollbar();
	// g_warning("vert: %p", vert);
	// vert->s

	set_valign(Gtk::ALIGN_START);
	set_vexpand(false);
	set_vexpand_set(true);
}

void ColorPalette::scroll(int dx, int dy) {
	if (auto vert = _scroll.get_vscrollbar()) {
		vert->set_value(vert->get_value() + dy);
	}
}

void ColorPalette::set_tile_border(int border) {
	if (border == _size) return;

	if (border < 0 || border > 100) {
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

void ColorPalette::set_compact(bool compact) {
    if (_compact != compact) {
        _compact = compact;
        set_up_scrolling();
    }
}

void ColorPalette::set_up_scrolling() {
    if (_compact) {
        // in compact mode scrollbars are hidden; they take up too much space

        if (_rows == 1) {
            // horizontal scrolling with single row
            _flowbox.set_max_children_per_line(_count);
            _flowbox.set_min_children_per_line(_count);

            _scroll_btn.hide();
            _scroll_left.show();
            _scroll_right.show();

            _scroll.set_policy(Gtk::POLICY_EXTERNAL, Gtk::POLICY_NEVER);
        }
        else {
            // vertical scrolling with multiple rows
            // 'external' allows scrollbar to shrink vertically
            _scroll.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_EXTERNAL);
            _flowbox.set_min_children_per_line(1);
            _flowbox.set_max_children_per_line(_count);
            _scroll_left.hide();
            _scroll_right.hide();
            _scroll_btn.show();
        }
    }
    else {
        // in normal mode use regular full-size scrollbars
        _scroll_left.hide();
        _scroll_right.hide();
        _scroll_btn.hide();

        _flowbox.set_min_children_per_line(1);
        _flowbox.set_max_children_per_line(_count);

        // 'always' allocates space for scrollbar
        _scroll.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
    }

	resize();
}

void ColorPalette::resize() {
	if (_rows == 1) {
		// auto size for single row to allocate space for scrollbar
		_scroll.set_size_request(-1, -1);
	}
	else {
		// exact size for multiple rows
		int height = (_size + _border) * _rows - _border;
		height += _flowbox.get_margin_top() + _flowbox.get_margin_bottom();
		_scroll.set_size_request(1, height);
	}

	_flowbox.set_column_spacing(_border);
	_flowbox.set_row_spacing(_border);

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
	_flowbox.freeze_notify();
	_flowbox.freeze_child_notify();

	free();

	int count = 0;
	for (auto widget : swatches) {
		if (widget) {
			_flowbox.add(*widget);
			++count;
		}
	}	

	_flowbox.show_all();
	_count = std::max(1, count);
	// if (_rows == 1) {
		// _flowbox.set_min_children_per_line(std::max(1, count));
	// }
	_flowbox.set_max_children_per_line(_count);

	// resize();
	set_up_scrolling();

	_flowbox.thaw_child_notify();
	_flowbox.thaw_notify();
}

void ColorPalette::set_palettes(const std::vector<Glib::ustring>& palettes) {
	auto& menu = get_widget<Gtk::Menu>(_builder, "menu");
	auto items = menu.get_children();
	auto count = items.size();

	int index = 0;
	while (count > 2) {
		if (auto item = items[index++]) {
			menu.remove(*item);
			delete item;
		}
		count--;
	}
// for (auto& name : palettes) menu.append(*Gtk::manage(new Gtk::MenuItem(name)));

	for (auto it = palettes.rbegin(); it != palettes.rend(); ++it) {
		auto& name = *it;
		auto item = Gtk::manage(new Gtk::MenuItem(name));
		item->signal_activate().connect([=](){ _signal_palette_selected.emit(name); });
		item->show();
		menu.prepend(*item);
	}
}

sigc::signal<void, Glib::ustring>& ColorPalette::get_palette_selected_signal() {
	return _signal_palette_selected;
}

}}} // namespace
