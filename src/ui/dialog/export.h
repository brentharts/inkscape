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

#ifndef SP_EXPORT_H
#define SP_EXPORT_H

#include <gtkmm.h>

#include "ui/widget/scrollprotected.h"
#include "ui/widget/unit-menu.h"

#include "ui/dialog/dialog-base.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

class ExportProgressDialog;

/**
 * A dialog widget to export to various image formats such as bitmap and png.
 *
 * Creates a dialog window for exporting an image to a bitmap if one doesn't already exist and
 * shows it to the user. If the dialog has already been created, it simply shows the window.
 *
 */
class Export : public DialogBase
{
public:
    Export();
    ~Export() override;

    static Export &getInstance() { return *new Export(); }

private:
    Gtk::Box container;
    Gtk::Notebook notebook;
    Gtk::Box single_image;
    Gtk::Box batch;

    
    Gtk::ScrolledWindow si_scrolled;
    Gtk::Grid si_scrolled_grid;
    
    Gtk::ButtonBox si_selection_box;
    Gtk::RadioButton select_document;
    Gtk::RadioButton select_page;
    Gtk::RadioButton select_selection;
    Gtk::RadioButton select_custom;

    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> x0_sb;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> x1_sb;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> y0_sb;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> y1_sb;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> width_sb;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> height_sb;

    Gtk::CheckButton show_export_area;
    Inkscape::UI::Widget::UnitMenu unit_selector;

    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> bmwidth_sb;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> bmheight_sb;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> dpi_sb;

    Gtk::CheckButton hide_all;
    Gtk::CheckButton preview;

    Gtk::Grid si_bottom_grid;
    Gtk::Entry si_filename;
    Gtk::ComboBoxText si_extention;
    Gtk::Button si_export;


    void createSingleImage();
    void createBatch();
    void attachLabels();
    void createRadioGroups();
    void removeIndicators();
    void setupSpinButtons();
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
