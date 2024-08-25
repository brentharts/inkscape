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

#include "textpath-popover.h"

// #include <glibmm/main.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/togglebutton.h>

#include "object/sp-path.h"
#include "ui/builder-utils.h"

namespace Inkscape::UI::Widget {

TextpathPopover::TextpathPopover(SPText *text, SPTextPath *textpath, double offset_val)
    : Gtk::Popover{}
    , _text(text)
    , _textpath(textpath)
    , _builder(create_builder("textpath-popover-box.ui"))
    , _start_offset_sb(get_widget<Gtk::SpinButton>(_builder, "start-offset-sb"))
    // , _text_length_sb(get_widget<Gtk::SpinButton>(_builder, "text-length-sb"))
    , _side_left_btn(get_widget<Gtk::ToggleButton>(_builder, "side-left-btn"))
    , _side_right_btn(get_widget<Gtk::ToggleButton>(_builder, "side-right-btn"))
{
    g_assert(text != nullptr);
    g_assert(textpath != nullptr);

    // Populate the popup.
    set_child(get_widget<Gtk::Box>(_builder, "popover-box"));

    auto start_adj = _start_offset_sb.get_adjustment();
    start_adj->set_value(offset_val);
    start_adj->signal_value_changed().connect([this, textpath, start_adj] {
        // Update the `startOffset` attribute of the textpath.
        auto offset_str = std::to_string(start_adj->get_value()) + "%";

        if (textpath) {
            textpath->setAttribute("startOffset", offset_str);
        }
    });

    /*
    auto length_adj = _text_length_sb.get_adjustment();
    length_adj->signal_value_changed().connect([this, text, length_adj] {
        // Update the `textLength` attribute of the textpath.
        auto length_str = std::to_string(length_adj->get_value()) + "%";

        if (text) {
            text->setAttribute("textLength", length_str);
            text->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            text->updateRepr();
        }
    });
    */

    // Setup the flip buttons.
    _side_left_btn.signal_clicked().connect(
        sigc::bind(sigc::mem_fun(*this, &TextpathPopover::side_btn_clicked), side_left));
    _side_right_btn.signal_clicked().connect(
        sigc::bind(sigc::mem_fun(*this, &TextpathPopover::side_btn_clicked), side_right));

    if (auto side = textpath->getAttribute("side")) {
        Glib::ustring side_str(side);
        _side_right_btn.set_active(side_str == side_right);
    }
}

void TextpathPopover::side_btn_clicked(Glib::ustring side)
{
    bool same_side = false;
    bool unset_and_not_left = false;

    if (auto current_side = _textpath->getAttribute("side")) {
        Glib::ustring side_str(current_side);

        if (side == current_side) {
            same_side = true;
        }
    } else if (side != side_left) {
        unset_and_not_left = true;
    }

    if (unset_and_not_left || !same_side) {
        _textpath->setAttribute("side", side);
        auto old_val = _start_offset_sb.get_value();

        if (auto path_item = static_cast<SPPath *>(sp_textpath_get_path_item(_textpath))) {
            if (auto curve = path_item->curve()) {
                auto const pwd2 = Geom::paths_to_pw(curve->get_pathvector());
                auto total_len = Geom::length(pwd2);

                auto &layout = _text->layout;
                auto iter = layout.begin();
                Geom::Point start_pt = layout.characterAnchorPoint(iter);

                iter = layout.end();
                Geom::Point end_pt = layout.characterAnchorPoint(iter);

                // To keep the text on the same position on the path
                // after side change.
                double tmp_offset = Geom::distance(start_pt, end_pt) * 100.0 / total_len;
                old_val += tmp_offset;
            }
        }

        // Changing the side attribute reverses the direction
        // from which the offset is calculated.
        _start_offset_sb.set_value(100 - old_val);
    }
}

} // namespace Inkscape::UI::Widget

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
