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
#include <gtkmm/flowbox.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/menu.h>

namespace Inkscape {
namespace UI {
namespace Widget {

class ColorPalette : public Gtk::Bin {
public:
	ColorPalette();

	void set_tile_size(int size_px);
	void set_tile_border(int border_px);
	void set_rows(int rows);
	void set_colors(const std::vector<Gtk::Widget*>& swatches);

private:
	void resize();
	void set_up_scrolling();
	void free();

	Glib::RefPtr<Gtk::Builder> _builder;
	Gtk::FlowBox& _flowbox;
	Gtk::ScrolledWindow& _scroll;
	Gtk::FlowBox& _scroll_btn;
	Gtk::Menu& _menu;
	int _size = 10;
	int _border = 0;
	int _rows = 1;
};

}}} // namespace

#endif // SEEN_COLOR_PALETTE_H
