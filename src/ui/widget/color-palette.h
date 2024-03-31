// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Color palette widget
 */
/* Authors:
 *   Michael Kowalski
 *
 * Copyright (C) 2021 Michael Kowalski
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLOR_PALETTE_H
#define SEEN_COLOR_PALETTE_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <variant>
#include <glibmm/ustring.h>
#include <glibmm/refptr.h>
#include <gtkmm/box.h>
#include <sigc++/signal.h>
#include <2geom/int-point.h>

#include "ui/widget/palette_t.h"
#include "ui/widget/bin.h"
#include "util/variant-visitor.h"

namespace Gtk {
class Builder;
class Button;
class FlowBox;
class Popover;
class ScrolledWindow;
} // namespace Gtk

class SPGradient;

namespace detail {
using NoColor = std::monostate;
using Rgb8bit = std::array<uint8_t, 3>;
using ColorKey = std::variant<NoColor, Rgb8bit, SPGradient *>;
}  // namespace detail

namespace std {
template <>
struct hash<detail::ColorKey> {
    /// Hash a ColorKey to enable its use in an unordered_map.
    size_t operator()(detail::ColorKey key) const {
        return std::visit(Inkscape::VariantVisitor{[](detail::NoColor) -> size_t { return 0x0ULL; },
                                                   [](detail::Rgb8bit const &rgb) -> size_t {
                                                       return static_cast<size_t>(rgb[0]) << 24 |
                                                              static_cast<size_t>(rgb[1]) << 16 |
                                                              static_cast<size_t>(rgb[2]) << 8 |
                                                              0xCAFEBABE'000000'01; // odd number
                                                   },
                                                   // Note: memory addresses are even due to alignment
                                                   [](SPGradient *ptr) -> size_t { return (size_t)(ptr) & ~0x1ULL; }},
                          key);
    }
};
}  // namespace std

namespace Inkscape::UI {

namespace Dialog {
class ColorItem;
} // namespace Dialog

namespace Widget {

class ColorPaletteMenuItem;
class PopoverMenu;

class ColorPalette : public UI::Widget::Bin
{
public:
    using ColorKey = detail::ColorKey;
    using NoColor = detail::NoColor;
    using Rgb8bit = detail::Rgb8bit;

    ColorPalette();
    ~ColorPalette() override;

    // set colors presented in a palette
    void set_colors(std::vector<Dialog::ColorItem*> const &swatches);

    /// Remove all colors from the palette
    void clear();

    /// Add a ColorItem to the palette and optionally tie it to a key.
    void add_item(std::unique_ptr<Dialog::ColorItem> swatch, std::optional<ColorKey> key);

    /// Reconstruct the widget after colors have been added/removed.
    void rebuild();

    /// Display fill and stroke indicators for the given color keys or clear them by passing empty optionals.
    void set_fill_stroke_indicators(std::optional<ColorKey> fill_key, std::optional<ColorKey> stroke_key);

    // list of palettes to present in the menu
    void set_palettes(const std::vector<palette_t>& palettes);
    // enable compact mode (true) with mini-scroll buttons, or normal mode (false) with regular scrollbars
    void set_compact(bool compact);
    // enlarge color tiles in a pinned panel
    void set_large_pinned_panel(bool large);
    // preferred number of colors in a group (used for color alignment in columns)
    void set_page_size(int page_size);

    void set_tile_size(int size_px);
    void set_tile_border(int border_px);
    void set_rows(int rows);
    void set_aspect(double aspect);
    // show horizontal scrollbar when only 1 row is set
    void enable_scrollbar(bool show);
    // allow tile stretching (horizontally)
    void enable_stretch(bool enable);
    // Show labels in swatches dialog
    void enable_labels(bool labels);
    // Show/hide settings
    void set_settings_visibility(bool show);

    int get_tile_size() const { return _size; }
    int get_tile_border() const { return _border; }
    int get_rows() const { return _rows; }
    double get_aspect() const { return _aspect; }
    bool is_scrollbar_enabled() const { return _force_scrollbar; }
    bool is_stretch_enabled() const { return _stretch_tiles; }
    bool is_pinned_panel_large() const {return _large_pinned_panel; }
    bool are_labels_enabled() const { return _show_labels; }

    void set_selected(const Glib::ustring& id);

    sigc::signal<void (Glib::ustring)> &get_palette_selected_signal() { return _signal_palette_selected; }
    sigc::signal<void ()> &get_settings_changed_signal() { return _signal_settings_changed; }

    Gtk::Popover &get_settings_popover();

    void set_filter(std::function<bool (const Dialog::ColorItem&)> filter);
    void apply_filter();

private:
    void resize();
    void _set_up_scrolling();
    void scroll(int dx, int dy, double snap, bool smooth);
    void do_scroll(int dx, int dy);
    static gboolean scroll_callback(gpointer self);
    void _set_tile_size(int size_px);
    void _set_tile_border(int border_px);
    void _set_rows(int rows);
    void _set_aspect(double aspect);
    void _enable_scrollbar(bool show);
    void _enable_stretch(bool enable);
    void _set_large_pinned_panel(bool large);
    bool _should_show_normal_item(Dialog::ColorItem const *item) const;
    void update_checkbox();
    void update_stretch();
    int _compute_tile_size(bool horz) const;
    int _compute_tile_width() const;
    int _compute_tile_height() const;
    int get_palette_height() const;
    Gtk::Widget &_create_wrapper_widget(Dialog::ColorItem &item) const;
    void _refresh();

    std::vector<std::unique_ptr<Dialog::ColorItem>> _normal_items;
    std::vector<std::unique_ptr<Dialog::ColorItem>> _pinned_items;

    // A map from colors to their respective widgets. Used to quickly find the widgets corresponding
    // to the current fill/stroke color, in order to update their fill/stroke indicators.
    std::unordered_map<ColorKey, std::vector<Dialog::ColorItem*>> _color_to_widgets;

    std::optional<ColorKey> _current_fill_key;
    std::optional<ColorKey> _current_stroke_key;

    Glib::RefPtr<Gtk::Builder> _builder;
    Gtk::FlowBox& _normal_box;
    Gtk::FlowBox& _pinned_box;
    Gtk::ScrolledWindow& _scroll;
    Gtk::FlowBox& _scroll_btn;
    Gtk::Button& _scroll_up;
    Gtk::Button& _scroll_down;
    Gtk::Button& _scroll_left;
    Gtk::Button& _scroll_right;
    std::unique_ptr<PopoverMenu> _menu;
    std::vector<std::unique_ptr<ColorPaletteMenuItem>> _palette_menu_items;
    int _size = 10;
    int _border = 0;
    int _rows = 1;
    double _aspect = 0.0;
    bool _compact = true;
    sigc::signal<void (Glib::ustring)> _signal_palette_selected;
    sigc::signal<void ()> _signal_settings_changed;
    bool _in_update = false;
    guint _active_timeout = 0;
    bool _force_scrollbar = false;
    bool _stretch_tiles = false;
    double _scroll_step = 0.0; // smooth scrolling step
    double _scroll_final = 0.0; // smooth scroll final value
    bool _large_pinned_panel = false;
    bool _show_labels = false;
    int _page_size = 0;
    Geom::IntPoint _allocation;
};

} // namespace Widget
} // namespace Inkscape::UI

#endif // SEEN_COLOR_PALETTE_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:
