// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Peter Bostrom
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 1999-2007, 2012 Authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "export.h"

#include <gtkmm.h>

#include "io/resource.h"

#ifdef _WIN32

#endif

namespace Inkscape {
namespace UI {
namespace Dialog {

Export::Export()
    : DialogBase("/dialogs/export/", "Export")
    , container(Gtk::ORIENTATION_VERTICAL, 0)
    , single_image(Gtk::ORIENTATION_VERTICAL, 0)

{
    createSingleImage();
    createBatch();
    attachLabels();
    createRadioGroups();
    setupSpinButtons();
    container.pack_start(notebook, true, true);
    add(container);
    show_all_children();

    select_page.set_active();
}

Export::~Export() {}

void Export::createSingleImage()
{
    notebook.append_page(single_image, "Single Image");

    { // Setup Scrolled Window
        single_image.pack_start(si_scrolled, true, true);
        si_scrolled.set_propagate_natural_width();
        si_scrolled.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
        si_scrolled.set_vexpand();
        si_scrolled.add(si_scrolled_grid);
        { // Setup Scrolled Grid

            int row = 0;

            si_scrolled_grid.set_column_spacing(10);
            si_scrolled_grid.set_row_spacing(5);

            { // Setup selection box
                si_scrolled_grid.attach(si_selection_box, 0, row, 8, 1);
                si_selection_box.pack_start(select_document, true, true);
                si_selection_box.pack_start(select_page, true, true);
                si_selection_box.pack_start(select_selection, true, true);
                si_selection_box.pack_start(select_custom, true, true);
                si_selection_box.set_homogeneous();
                si_selection_box.set_hexpand();
                si_selection_box.set_layout(Gtk::BUTTONBOX_EXPAND);
                row = row + 1;
            }
            {// Setup Corrdinates Box
             {auto x0_label = Gtk::make_managed<Gtk::Label>("Left", Gtk::ALIGN_START);
            si_scrolled_grid.attach(*x0_label, 1, row, 1, 1);
            si_scrolled_grid.attach(x0_sb, 2, row, 1, 1);

            auto x1_label = Gtk::make_managed<Gtk::Label>("Right", Gtk::ALIGN_START);
            si_scrolled_grid.attach(*x1_label, 4, row, 1, 1);
            si_scrolled_grid.attach(x1_sb, 5, row, 1, 1);

            row = row + 1;
        }
        {
            auto y0_label = Gtk::make_managed<Gtk::Label>("Top", Gtk::ALIGN_START);
            si_scrolled_grid.attach(*y0_label, 1, row, 1, 1);
            si_scrolled_grid.attach(y0_sb, 2, row, 1, 1);

            auto y1_label = Gtk::make_managed<Gtk::Label>("Bottom", Gtk::ALIGN_START);
            si_scrolled_grid.attach(*y1_label, 4, row, 1, 1);
            si_scrolled_grid.attach(y1_sb, 5, row, 1, 1);

            row = row + 1;
        }
        {
            auto width_label = Gtk::make_managed<Gtk::Label>("Width", Gtk::ALIGN_START);
            si_scrolled_grid.attach(*width_label, 1, row, 1, 1);
            si_scrolled_grid.attach(width_sb, 2, row, 1, 1);

            auto height_label = Gtk::make_managed<Gtk::Label>("Height", Gtk::ALIGN_START);
            si_scrolled_grid.attach(*height_label, 4, row, 1, 1);
            si_scrolled_grid.attach(height_sb, 5, row, 1, 1);

            row = row + 1;
        }
    }
    { // Setup Units Box
        auto units_box = Gtk::make_managed<Gtk::Box>();
        si_scrolled_grid.attach(*units_box, 1, row, 5, 1);

        units_box->pack_start(show_export_area, true, true, 0);

        unit_selector.setUnitType(Inkscape::Util::UNIT_TYPE_LINEAR);
        units_box->pack_end(unit_selector, false, false, 0);

        auto unit_label = Gtk::make_managed<Gtk::Label>("Unit", Gtk::ALIGN_START);
        units_box->pack_end(*unit_label, false, false, 0);

        units_box->set_spacing(5);
        units_box->set_hexpand();
        row = row + 1;
    }
    { // Setup Image Box
        {
            auto img_label = Gtk::make_managed<Gtk::Label>("Image Size", Gtk::ALIGN_START);
            si_scrolled_grid.attach(*img_label, 1, row, 2, 1);
            row = row + 1;
        }
        {
            auto bmwidth_label = Gtk::make_managed<Gtk::Label>("Width", Gtk::ALIGN_START);
            si_scrolled_grid.attach(*bmwidth_label, 1, row, 1, 1);
            si_scrolled_grid.attach(bmwidth_sb, 2, row, 1, 1);

            auto bmheight_label = Gtk::make_managed<Gtk::Label>("Height", Gtk::ALIGN_START);
            si_scrolled_grid.attach(*bmheight_label, 4, row, 1, 1);
            si_scrolled_grid.attach(bmheight_sb, 5, row, 1, 1);

            row = row + 1;
        }
        {
            auto dpi_label = Gtk::make_managed<Gtk::Label>("DPI", Gtk::ALIGN_START);
            si_scrolled_grid.attach(*dpi_label, 1, row, 1, 1);
            si_scrolled_grid.attach(dpi_sb, 2, row, 1, 1);

            row = row + 1;
        }
        { // Options Box
            auto options_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL);
            si_scrolled_grid.attach(*options_box, 1, row, 2, 3);
            options_box->pack_start(hide_all, true, true, 0);
            options_box->pack_start(preview, true, true, 0);
            options_box->set_spacing(5);
            row = row + 3;
        }
    }
}
} // namespace Dialog

{ // Setup Bottom Grid
    single_image.pack_end(si_bottom_grid, false, true);
    si_bottom_grid.attach(si_filename, 0, 0, 1, 1);
    si_bottom_grid.attach(si_extention, 1, 0, 1, 1);
    si_bottom_grid.attach(si_export, 2, 0, 1, 1);
    si_filename.set_hexpand();
}
} // namespace UI

void Export::createBatch()
{
    notebook.append_page(batch, "Batch Export");
}

void Export::attachLabels()
{
    select_document.set_label("Document");
    select_page.set_label("Page");
    select_selection.set_label("Selection");
    select_custom.set_label("Custom");
    si_export.set_label("Export");

    show_export_area.set_label("Show Export Area on Canvas");

    hide_all.set_label("Hide all except selected");
    preview.set_label("Preview");
}

void Export::createRadioGroups()
{
    { // single image selection group
        auto group = select_document.get_group();
        select_page.set_group(group);
        select_selection.set_group(group);
        select_custom.set_group(group);
    }
    removeIndicators();
}

void Export::removeIndicators()
{
    {
        select_document.set_mode(false);
        select_page.set_mode(false);
        select_selection.set_mode(false);
        select_custom.set_mode(false);
    }
}

void Export::setupSpinButtons()
{
    { // Coordinate sp
        auto adj = Gtk::Adjustment::create(0, 0.0, 1000.0, 0.1, 1.0, 0);
        x0_sb.configure(adj,1.0,3);
        x0_sb.set_width_chars(7);
        x0_sb.set_sensitive (true);

        x1_sb.configure(adj,1.0,3);
        x1_sb.set_width_chars(7);
        x1_sb.set_sensitive (true);
    }
}

} // namespace Inkscape
} // namespace UI
} // namespace Inkscape

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :