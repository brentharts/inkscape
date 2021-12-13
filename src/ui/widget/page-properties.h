// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Michael Kowalski
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_PAGE_PROPERTIES_H
#define INKSCAPE_UI_WIDGET_PAGE_PROPERTIES_H

#include <gtkmm/box.h>

namespace Inkscape {    
namespace UI {
namespace Widget {

class PageProperties : public Gtk::Box {
public:
    static PageProperties* create();

    ~PageProperties() override = default;

    enum class Color { Background, Desk, Border };
    virtual void set_color(Color element, unsigned int rgba) = 0;

    sigc::signal<void (unsigned int, Color)>& signal_color_changed() { return _signal_color_changed; }

    enum class Check { Checkerboard, Border, Shadow, BorderOnTop, AntiAlias };
    virtual void set_check(Check element, bool checked) = 0;

    sigc::signal<void (bool, Check)>& signal_check_toggled() { return _signal_check_toggled; }

    enum class Dimension { PageSize, ViewboxSize, ViewboxPosition, Scale };
    virtual void set_dimension(Dimension dim, double x, double y) = 0;

    sigc::signal<void (double, double, Dimension)>& signal_dimmension_changed() { return _signal_dimmension_changed; }

    enum class Units { Display, Document };
    virtual void set_unit(Units unit, const Glib::ustring& abbr) = 0;

    sigc::signal<void (const Glib::ustring&, Units)> signal_unit_changed() { return _signal_unit_changed; }

protected:
    sigc::signal<void (unsigned int, Color)> _signal_color_changed;
    sigc::signal<void (bool, Check)> _signal_check_toggled;
    sigc::signal<void (double, double, Dimension)> _signal_dimmension_changed;
    sigc::signal<void (const Glib::ustring&, Units)> _signal_unit_changed;
};

} } } // namespace Inkscape/Widget/UI

#endif // INKSCAPE_UI_WIDGET_PAGE_PROPERTIES_H
