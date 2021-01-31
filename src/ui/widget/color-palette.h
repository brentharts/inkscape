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

#include <gtkmm/bin.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/flowbox.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/menu.h>

namespace Inkscape {
namespace UI {
namespace Widget {

class ColorPalette : public Gtk::Bin {
public:
	ColorPalette();

	// 
	void set_colors(const std::vector<Gtk::Widget*>& swatches);
	// list of palette names to present in the menu
	void set_palettes(const std::vector<Glib::ustring>& palettes);

	void set_tile_size(int size_px);
	void set_tile_border(int border_px);
	void set_rows(int rows);

	sigc::signal<void, Glib::ustring>& get_palette_selected_signal();

private:
	void resize();
	void set_up_scrolling();
	void free();
	void scroll(int dx, int dy);

	Glib::RefPtr<Gtk::Builder> _builder;
	Gtk::FlowBox& _flowbox;
	Gtk::ScrolledWindow& _scroll;
	Gtk::FlowBox& _scroll_btn;
	Gtk::Button& _scroll_left;
	Gtk::Button& _scroll_right;
	Gtk::Menu& _menu;
	int _size = 10;
	int _border = 0;
	int _rows = 1;
	int _count = 1;
	sigc::signal<void, Glib::ustring> _signal_palette_selected;
};

}}} // namespace

#endif // SEEN_COLOR_PALETTE_H
