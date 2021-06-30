// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *
 * Copyright (C) 1999-2007 Authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SP_EXPORT_SINGLE_H
#define SP_EXPORT_SINGLE_H

#include <gtkmm.h>

#include "export-helper.h"
#include "extension/output.h"
#include "ui/dialog/dialog-base.h"
#include "ui/widget/scrollprotected.h"
#include "ui/widget/unit-menu.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

enum sb_type
{
    SPIN_X0 = 0,
    SPIN_X1,
    SPIN_Y0,
    SPIN_Y1,
    SPIN_WIDTH,
    SPIN_HEIGHT,
    SPIN_BMWIDTH,
    SPIN_BMHEIGHT,
    SPIN_DPI
};

enum selection_mode
{
    SELECTION_PAGE = 0, // Default is alaways placed first
    SELECTION_SELECTION,
    SELECTION_DRAWING,
    SELECTION_CUSTOM,
};

class SingleExport : public Gtk::Box
{
public:
    SingleExport();
    ~SingleExport() override;

public:
    void initialise(Glib::RefPtr<Gtk::Builder> *builder);

private:
    // Single Image Objects Start Here
    Gtk::Box *single_image = nullptr;

    std::map<sb_type, SpinButton *> spin_buttons;
    Gtk::CheckButton *show_export_area = nullptr;
    Inkscape::UI::Widget::UnitMenu *units = nullptr;

    Gtk::CheckButton *si_hide_all = nullptr;
    Gtk::Box *si_preview_box = nullptr;
    Gtk::CheckButton *si_show_preview = nullptr;

    Gtk::ComboBoxText *si_extension_cb = nullptr;
    Gtk::Entry *si_filename_entry = nullptr;
    Gtk::Button *si_export = nullptr;
};
} // namespace Dialog
} // namespace UI
} // namespace Inkscape
#endif

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