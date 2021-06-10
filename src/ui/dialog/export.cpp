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
{
    std::string gladefile = get_filename_string(Inkscape::IO::Resource::UIS, "dialog-export.glade");

    try {
        builder = Gtk::Builder::create_from_file(gladefile);
    } catch (const Glib::Error &ex) {
        g_error("Glade file loading failed for export screen");
        return;
    }
    builder->get_widget("Export Dialog Box", container);
    add(*container);
    show_all_children();
    initialise_all();
    removeScrollEvents();
    export_notebook->signal_map().connect(sigc::mem_fun(*this, &Export::onNotebookVisible));
}

Export::~Export() {}

void Export::initialise_all()
{
    if (builder) {
        // Notebook Start
        {
            builder->get_widget("Export Notebook", export_notebook);
            // Single Image Start
            {
                builder->get_widget("Single Image", single_image);

                builder->get_widget("si_s_document", select_document);
                builder->get_widget("si_s_page", select_page);
                builder->get_widget("si_s_selection", select_selection);
                builder->get_widget("si_s_custom", select_custom);

                builder->get_widget("si_left_sb", left_sb);
                builder->get_widget("si_right_sb", right_sb);
                builder->get_widget("si_top_sb", top_sb);
                builder->get_widget("si_bottom_sb", bottom_sb);
                builder->get_widget("si_height_sb", height_sb);
                builder->get_widget("si_width_sb", width_sb);

                builder->get_widget("si_img_height_sb", img_height_sb);
                builder->get_widget("si_img_width_sb", img_width_sb);
                builder->get_widget("si_dpi_sb", dpi_sb);

                builder->get_widget("si_show_export_area", show_export_area);
                builder->get_widget("si_units", units);

                builder->get_widget("si_hide_all", hide_all);
                builder->get_widget("si_preview_box", si_preview_box);
                builder->get_widget("si_show_preview", si_show_preview);

                builder->get_widget("si_extention", extension);
                builder->get_widget("si_filename", filename);
                builder->get_widget("si_export", si_export);
            } // Single Image End

            // Batch Export Start
            {
                builder->get_widget("Batch Export", batch_export);
            } // Batch Export End

        } // Notebook End
    }
    return;
}

/**
 * Set current page based on preference/last visited page
 */

void Export::onNotebookVisible()
{
    if (export_notebook && batch_export) {
        auto page_num = export_notebook->page_num(*batch_export);
        export_notebook->set_current_page(page_num);
    }
    return;
}

void Export::removeScrollEvent(Gtk::Widget* widget){
    if(widget){
        Gdk::EventMask event_masks = widget->get_events();
        Gdk::EventMask new_masks = event_masks & ~(Gdk::EventMask::SCROLL_MASK | ~Gdk::EventMask::SMOOTH_SCROLL_MASK);
        widget->set_events(new_masks);
        std::cout << "Removed\n";
    }else{
        std::cout << "Not Removed\n";
    }
    std::cout.flush();
    return;
}

void Export::removeScrollEvents(){
    removeScrollEvent(dynamic_cast<Gtk::Widget *>(left_sb));
    removeScrollEvent(dynamic_cast<Gtk::Widget *>(right_sb));
    removeScrollEvent(dynamic_cast<Gtk::Widget *>(top_sb));
    removeScrollEvent(dynamic_cast<Gtk::Widget *>(bottom_sb));
    removeScrollEvent(dynamic_cast<Gtk::Widget *>(height_sb));
    removeScrollEvent(dynamic_cast<Gtk::Widget *>(width_sb));
    removeScrollEvent(dynamic_cast<Gtk::Widget *>(units));
    removeScrollEvent(dynamic_cast<Gtk::Widget *>(extension));
    removeScrollEvent(dynamic_cast<Gtk::Widget *>(img_height_sb));
    removeScrollEvent(dynamic_cast<Gtk::Widget *>(img_width_sb));

    return;
}

} // namespace Dialog
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