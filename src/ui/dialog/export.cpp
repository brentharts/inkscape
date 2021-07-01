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

// This has to be included prior to anything that includes setjmp.h, it croaks otherwise
#include "export.h"

#include <glibmm/convert.h>
#include <glibmm/i18n.h>
#include <glibmm/miscutils.h>
#include <gtkmm.h>
#include <png.h>

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "extension/db.h"
#include "file.h"
#include "helper/png-write.h"
#include "inkscape-window.h"
#include "inkscape.h"
#include "io/resource.h"
#include "io/sys.h"
#include "message-stack.h"
#include "object/object-set.h"
#include "object/sp-namedview.h"
#include "object/sp-root.h"
#include "preferences.h"
#include "selection-chemistry.h"
#include "ui/dialog-events.h"
#include "ui/dialog/dialog-notebook.h"
#include "ui/dialog/filedialog.h"
#include "ui/interface.h"
#include "ui/widget/scrollprotected.h"
#include "ui/widget/unit-menu.h"

#ifdef _WIN32

#endif

using Inkscape::Util::unit_table;
namespace Inkscape {
namespace UI {
namespace Dialog {

Export::Export()
    : DialogBase("/dialogs/export/", "Export")
    , selectChangedConn()
    , subselChangedConn()
    , selectModifiedConn()
{
    std::string gladefile = get_filename_string(Inkscape::IO::Resource::UIS, "dialog-export.glade");

    try {
        builder = Gtk::Builder::create_from_file(gladefile);
    } catch (const Glib::Error &ex) {
        g_error("Glade file loading failed for export screen");
        return;
    }

    prefs = Inkscape::Preferences::get();

    builder->get_widget("Export Dialog Box", container);
    add(*container);
    show_all_children();
    builder->get_widget("Export Notebook", export_notebook);
    builder->get_widget_derived("Single Image", single_image);

    single_image->initialise(builder);

    builder->get_widget("Batch Export", batch_export);
    // Callback when container is dinally mapped on window. All intialisation like set active is done inside it.
    container->signal_realize().connect(sigc::mem_fun(*this, &Export::onRealize));
}

Export::~Export()
{
    selectModifiedConn.disconnect();
    subselChangedConn.disconnect();
    selectChangedConn.disconnect();
}


void Export::setDefaultNotebookPage()
{
    // if (export_notebook && batch_export) {
    //     auto page_num = export_notebook->page_num(*batch_export);
    // export_notebook->set_current_page(page_num);
    // }
}


/**
 * SIGNALS
 */

// Set current page based on preference/last visited page

void Export::onRealize()
{
    single_image->setup();
    // setDefaultNotebookPage();
}

// Single Export Callback.
// Do not add batch export widgets check here
void Export::onExport()
{
    // SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    // if (!desktop)
    //     return;
    // si_export->set_sensitive(false);
    // bool exportSuccessful = false;
    // auto extension = si_extension_cb->get_active_text();
    // if (!extension_list[extension]) {
    //     si_export->set_sensitive(true);
    //     return;
    // }
    // if (extension_list[extension]->is_raster()) {
    //     double x0 = getValuePx(spin_buttons[SPIN_X0]->get_value());
    //     double x1 = getValuePx(spin_buttons[SPIN_X1]->get_value());
    //     double y0 = getValuePx(spin_buttons[SPIN_Y0]->get_value());
    //     double y1 = getValuePx(spin_buttons[SPIN_Y1]->get_value());
    //     auto area = Geom::Rect(Geom::Point(x0, y0), Geom::Point(x1, y1)) * desktop->dt2doc();

    //     int width = int(spin_buttons[SPIN_BMWIDTH]->get_value() + 0.5);
    //     int height = int(spin_buttons[SPIN_BMHEIGHT]->get_value() + 0.5);

    //     double dpi = spin_buttons[SPIN_DPI]->get_value();

    //     Glib::ustring filename = si_filename_entry->get_text();

    //     exportSuccessful = _export_raster(area, width, height, dpi, filename, extension_list[extension]);

    // } else {
    //     exportSuccessful = _export_vector(extension_list[extension]);
    // }
    // si_export->set_sensitive(true);
}

bool Export::_export_raster(Geom::Rect const &area, unsigned long int const &width, unsigned long int const &height,
                            float const &dpi, Glib::ustring const &filename, bool overwrite,
                            Inkscape::Extension::Output *extension)
{
    // SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    // if (!desktop)
    //     return false;
    // SPNamedView *nv = desktop->getNamedView();
    // SPDocument *doc = desktop->getDocument();

    // if (area.hasZeroArea() || width == 0 || height == 0) {
    //     desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("The chosen area to be exported is invalid."));
    //     sp_ui_error_dialog(_("The chosen area to be exported is invalid"));
    //     return false;
    // }
    // if (filename.empty()) {
    //     desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("You have to enter a filename."));
    //     sp_ui_error_dialog(_("You have to enter a filename"));
    //     return false;
    // }

    // if (!extension || !extension->is_raster()) {
    //     desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("Raster Export Error"));
    //     sp_ui_error_dialog(_("Raster export Method is used for NON RASTER EXTENSION"));
    //     return false;
    // }

    // // Advance Parameters
    // const bool use_interlacing = interlacing.get_active();
    // const float pHYs = (phys_dpi_sb.get_value() > 0.01) ? phys_dpi_sb.get_value() : dpi;
    // const int color_type = color_list[bit_depth_cb.get_active_row_number()];
    // const int bit_depth = bit_depth_list[bit_depth_cb.get_active_row_number()];
    // const int antialiasing = anti_aliasing_list[anti_aliasing_cb.get_active_row_number()];
    // const int zlib = compression_list[compression_cb.get_active_row_number()];

    // std::string path = absolutize_path_from_document_location(doc, Glib::filename_from_utf8(filename));
    // Glib::ustring dirname = Glib::path_get_dirname(path);

    // if (dirname.empty() ||
    //     !Inkscape::IO::file_test(dirname.c_str(), (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))) {
    //     gchar *safeDir = Inkscape::IO::sanitizeString(dirname.c_str());
    //     gchar *error = g_strdup_printf(_("Directory %s does not exist or is not a directory.\n"), safeDir);

    //     desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, error);
    //     sp_ui_error_dialog(error);

    //     g_free(safeDir);
    //     g_free(error);
    //     return false;
    // }

    // // Do the over-write protection now, since the png is just a temp file.
    // if (!overwrite && !sp_ui_overwrite_file(filename.c_str())) {
    //     return false;
    // }

    // auto fn = Glib::path_get_basename(path);
    // auto png_filename = std::string(path.c_str());
    // // Select the extension and set the filename to a temporary file
    // int tempfd_out = Glib::file_open_tmp(png_filename, "ink_ext_");
    // close(tempfd_out);

    return true;
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