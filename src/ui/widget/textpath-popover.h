// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * The popover menu which opens on clicking the textpath handles.
 * This popover will facilitate on canvas editing of textpath attributes.
 */
/*
 * Authors:
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_UI_WIDGET_TEXTPATH_POPOVER_H
#define SEEN_UI_WIDGET_TEXTPATH_POPOVER_H

#include <gtkmm/popover.h>
#include <vector>

#include "object/sp-text.h"
#include "object/sp-textpath.h"

namespace Glib {
class ustring;
} // namespace Glib

namespace Gtk {
class Builder;
class SpinButton;
class ToggleButton;
} // namespace Gtk

namespace Inkscape::UI::Widget {

class TextpathPopover final : public Gtk::Popover
{
public:
    TextpathPopover(SPText *text, SPTextPath *textpath, double offset_val = 0);
    ~TextpathPopover() override = default;

    const Glib::ustring side_left = "left";
    const Glib::ustring side_right = "right";

    void side_btn_clicked(Glib::ustring side);

private:
    SPText *_text;
    SPTextPath *_textpath;

    // ************* Widgets ************* //
    Glib::RefPtr<Gtk::Builder> _builder;

    Gtk::SpinButton &_start_offset_sb;
    // Gtk::SpinButton &_text_length_sb;

    Gtk::ToggleButton &_side_left_btn;
    Gtk::ToggleButton &_side_right_btn;
};

} // namespace Inkscape::UI::Widget

#endif // SEEN_UI_WIDGET_TEXTPATH_POPOVER_H

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
