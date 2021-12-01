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

    void set_background_color(unsigned int rgba);

    sigc::signal<void (unsigned int)>& signal_background_color() { return _signal_background_color; }

protected:
    sigc::signal<void (unsigned int)> _signal_background_color;
    unsigned int _background_color = 0xffffff00;
};

} } } // namespace Inkscape/Widget/UI

#endif // INKSCAPE_UI_WIDGET_PAGE_PROPERTIES_H
