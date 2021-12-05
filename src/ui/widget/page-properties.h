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

protected:
    sigc::signal<void (unsigned int, Color)> _signal_color_changed;
    unsigned int _background_color = 0xffffff00;
};

} } } // namespace Inkscape/Widget/UI

#endif // INKSCAPE_UI_WIDGET_PAGE_PROPERTIES_H
