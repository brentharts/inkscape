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

#include "ui/dialog/dialog-base.h"
#include "ui/widget/scrollprotected.h"
#include "ui/widget/unit-menu.h"



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
    // builder and its object ( in order )
    Glib::RefPtr<Gtk::Builder> builder;
    Gtk::Box *container = nullptr;
    Gtk::Notebook *export_notebook = nullptr;

    Gtk::Box *single_image = nullptr;
    Gtk::RadioButton *select_document = nullptr;
    Gtk::RadioButton *select_page = nullptr;
    Gtk::RadioButton *select_selection = nullptr;
    Gtk::RadioButton *select_custom = nullptr;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> *left_sb = nullptr;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> *right_sb = nullptr;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> *top_sb = nullptr;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> *bottom_sb = nullptr;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> *height_sb = nullptr;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> *width_sb = nullptr;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> *img_height_sb = nullptr;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> *img_width_sb = nullptr;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> *dpi_sb = nullptr;
    Gtk::CheckButton *show_export_area = nullptr;
    Inkscape::UI::Widget::UnitMenu *units = nullptr;
    Gtk::CheckButton *hide_all = nullptr;
    Gtk::Box *si_preview_box = nullptr;
    Gtk::CheckButton *si_show_preview = nullptr;
    Gtk::ComboBoxText *extension = nullptr;
    Gtk::Entry *filename = nullptr;
    Gtk::Button *si_export = nullptr;

    Gtk::Box *batch_export = nullptr;

    // Initialise all objects from builder
    void initialise_all();

    //signals callback
    void onNotebookVisible();

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