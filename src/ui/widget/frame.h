// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Murray C
 *
 * Copyright (C) 2012 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_FRAME_H
#define INKSCAPE_UI_WIDGET_FRAME_H

#include <gtkmm/frame.h>
#include <gtkmm/label.h>

namespace Inkscape::UI::Widget {

/**
 * Creates a Gnome HIG style indented frame with bold label
 * See http://developer.gnome.org/hig-book/stable/controls-frames.html.en
 */
class Frame : public Gtk::Frame
{
public:

    /**
     * Construct a Frame Widget.
     *
     * @param label     The frame text.
     */
    Frame(Glib::ustring const &label = {}, bool label_bold = true);

    /**
     * Return the label widget
     */
    Gtk::Label const *get_label_widget() const;

    /**
     * Add a widget to this frame
     */
    void add(Widget& widget);

    /**
     * Set the frame label text and if bold or not
     */
    void set_label(Glib::ustring const &label, bool label_bold = true);

    /**
     * Set the frame padding
     */
    void set_padding(unsigned padding_top, unsigned padding_bottom,
                     unsigned padding_left, unsigned padding_right);

protected:
    Gtk::Label _label;
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_FRAME_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
