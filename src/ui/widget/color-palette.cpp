#include <gtkmm/box.h>
#include <gtkmm/popover.h>
#include <gtkmm/scale.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/button.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/menu.h>
#include <gtkmm/radiomenuitem.h>
#include <gtkmm/menubutton.h>

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
    _scroll_up(get_widget<Gtk::Button>(_builder, "btn-up")),
    _scroll_down(get_widget<Gtk::Button>(_builder, "btn-down")),
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
        _set_tile_size(static_cast<int>(size.get_value()));
        _signal_settings_changed.emit();
        return true;
    });

    auto& border = get_widget<Gtk::Scale>(_builder, "border-slider");
    border.signal_change_value().connect([=,&border](Gtk::ScrollType, double val) {
        _set_tile_border(static_cast<int>(border.get_value()));
        _signal_settings_changed.emit();
        return true;
    });

    auto& rows = get_widget<Gtk::Scale>(_builder, "row-slider");
    rows.signal_change_value().connect([=,&rows](Gtk::ScrollType, double val) {
        _set_rows(static_cast<int>(rows.get_value()));
        _signal_settings_changed.emit();
        return true;
    });

    _scroll.set_min_content_height(1);

    // set style for small buttons; we need them reasonably small, since they impact min height of color palette strip
    {
        auto css_provider = Gtk::CssProvider::create();
        css_provider->load_from_data(
        ".small {"
        " padding: 1px;"
        " margin: 0;"
        "}"
        );

        auto& btn_menu = get_widget<Gtk::MenuButton>(_builder, "btn-menu");
        Gtk::Widget* small_buttons[5] = {&_scroll_up, &_scroll_down, &_scroll_left, &_scroll_right, &btn_menu};
        for (auto button : small_buttons) {
            button->get_style_context()->add_provider(css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        }
    }

    _scroll_down.signal_clicked().connect([=](){ scroll(0, _size + _border); });
    _scroll_up.signal_clicked().connect([=](){ scroll(0, -(_size + _border)); });
    _scroll_left.signal_clicked().connect([=](){ scroll(-10 * (_size + _border), 0); });
    _scroll_right.signal_clicked().connect([=](){ scroll(10 * (_size + _border), 0); });

    {
        auto css_provider = Gtk::CssProvider::create();
        css_provider->load_from_data(
        "flowbox, scrolledwindow {"
        " padding: 0;"
        " border: 0;"
        " margin: 0;"
        " min-width: 1px;"
        " min-height: 1px;"
        "}");
        _scroll.get_style_context()->add_provider(css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        _flowbox.get_style_context()->add_provider(css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    // remove padding/margins from FlowBoxChild widgets, so previews can be adjacent to each other
    {
        auto css_provider = Gtk::CssProvider::create();
        css_provider->load_from_data(
        ".color-palette-main-box flowboxchild {"
        " padding: 0;"
        " border: 0;"
        " margin: 0;"
        " min-width: 1px;"
        " min-height: 1px;"
        "}");
        get_style_context()->add_provider_for_screen(this->get_screen(), css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    set_vexpand_set(true);
    set_up_scrolling();
}

void ColorPalette::scroll(int dx, int dy) {
    if (auto vert = _scroll.get_vscrollbar()) {
        vert->set_value(vert->get_value() + dy);
    }
    if (auto horz = _scroll.get_hscrollbar()) {
        horz->set_value(horz->get_value() + dx);
    }
}

int ColorPalette::get_tile_size() const {
    return _size;
}

int ColorPalette::get_tile_border() const {
    return _border;
}

int ColorPalette::get_rows() const {
    return _rows;
}

void ColorPalette::set_tile_border(int border) {
    _set_tile_border(border);
    auto& slider = get_widget<Gtk::Scale>(_builder, "border-slider");
    slider.set_value(border);
}

void ColorPalette::_set_tile_border(int border) {
    if (border == _border) return;

    if (border < 0 || border > 100) {
        g_warning("Unexpected tile border size of color palette: %d", border);
        return;
    }

    _border = border;
    resize();
}

void ColorPalette::set_tile_size(int size) {
    _set_tile_size(size);
    auto& slider = get_widget<Gtk::Scale>(_builder, "size-slider");
    slider.set_value(size);
}

void ColorPalette::_set_tile_size(int size) {
    if (size == _size) return;

    if (size < 1 || size > 1000) {
        g_warning("Unexpected tile size for color palette: %d", size);
        return;
    }

    _size = size;
    resize();
}

void ColorPalette::set_rows(int rows) {
    _set_rows(rows);
    auto& slider = get_widget<Gtk::Scale>(_builder, "row-slider");
    slider.set_value(rows);
}

void ColorPalette::_set_rows(int rows) {
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

        get_widget<Gtk::Scale>(_builder, "row-slider").set_visible(compact);
        get_widget<Gtk::Label>(_builder, "row-label").set_visible(compact);
    }
}

void ColorPalette::set_up_scrolling() {
    if (_compact) {
        // in compact mode scrollbars are hidden; they take up too much space
        set_valign(Gtk::ALIGN_START);
        set_vexpand(false);

        _scroll.set_valign(Gtk::ALIGN_END);
        _flowbox.set_valign(Gtk::ALIGN_END);

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
        set_valign(Gtk::ALIGN_FILL);
        set_vexpand(true);

        _scroll_left.hide();
        _scroll_right.hide();
        _scroll_btn.hide();

        _flowbox.set_valign(Gtk::ALIGN_START);
        _flowbox.set_min_children_per_line(1);
        _flowbox.set_max_children_per_line(_count);

        _scroll.set_valign(Gtk::ALIGN_FILL);
        // 'always' allocates space for scrollbar
        _scroll.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
    }

    resize();
}

void ColorPalette::resize() {
    if (_rows == 1 || !_compact) {
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

class CustomMenuItem : public Gtk::RadioMenuItem {
public:
    CustomMenuItem(Gtk::RadioMenuItem::Group& group, const Glib::ustring& label, std::vector<ColorPalette::rgb_t> colors):
        Gtk::RadioMenuItem(group, label), _colors(colors) {

        set_margin_bottom(2);
    }
private:
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;
    std::vector<ColorPalette::rgb_t> _colors;
};

bool CustomMenuItem::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
    RadioMenuItem::on_draw(cr);
    if (_colors.empty()) return false;

    auto allocation = get_allocation();
    auto x = 0;
    auto y = 0;
    auto width = allocation.get_width();
    auto height = allocation.get_height();
    auto left = x + height;
    auto right = x + width - height;
    auto dx = 1;
    auto dy = 2;
    auto px = left;
    auto py = y + height - dy;
    auto w = right - left;
    if (w <= 0) return false;

    for (int i = 0; i < w; ++i) {
        if (px >= right) break;

        int index = i * _colors.size() / w;
        auto& color = _colors.at(index);

        cr->set_source_rgb(color.r, color.g, color.b);
        cr->rectangle(px, py, dx, dy);
        cr->fill();

        px += dx;
    }

    return false;
}

void ColorPalette::set_palettes(const std::vector<ColorPalette::palette_t>& palettes) {
    auto items = _menu.get_children();
    auto count = items.size();

    int index = 0;
    while (count > 2) {
        if (auto item = items[index++]) {
            _menu.remove(*item);
            delete item;
        }
        count--;
    }

    Gtk::RadioMenuItem::Group group;
    for (auto it = palettes.rbegin(); it != palettes.rend(); ++it) {
        auto& name = it->name;
        auto item = Gtk::manage(new CustomMenuItem(group, name, it->colors));
        item->signal_activate().connect([=](){
            if (!_in_update) {
                _in_update = true;
                _signal_palette_selected.emit(name);
                _in_update = false;
            }
        });
        item->show();
        _menu.prepend(*item);
    }
}

sigc::signal<void, Glib::ustring>& ColorPalette::get_palette_selected_signal() {
    return _signal_palette_selected;
}

sigc::signal<void>& ColorPalette::get_settings_changed_signal() {
    return _signal_settings_changed;
}

void ColorPalette::set_selected(const Glib::ustring& name) {
    auto items = _menu.get_children();
    _in_update = true;
    for (auto item : items) {
        if (auto radio = dynamic_cast<Gtk::RadioMenuItem*>(item)) {
            radio->set_active(radio->get_label() == name);
        }
    }
    _in_update = false;
}

}}} // namespace
