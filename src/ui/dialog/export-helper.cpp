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

#include "export-helper.h"

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

AdvanceOptions::AdvanceOptions()
    : row(0)
{
    this->set_label(_("Advance"));
    Gtk::Grid *grid = Gtk::manage(new Gtk::Grid());
    this->add(*grid);
    std::vector<Glib::ustring> labels;
    {
        interlacing.set_label("Use Interlacing");
        grid->attach(interlacing, 0, row, 2, 1);
        row++;
    }
    {
        labels.insert(labels.end(), {"Gray_1", "Gray_2", "Gray_4", "Gray_8", "Gray_16", "RGB_8", "RGB_16",
                                     "GrayAlpha_8", "GrayAlpha_16", "RGBA_8", "RGBA_16"});
        bit_depth_list.clear();
        bit_depth_list.insert(bit_depth_list.end(), {1, 2, 4, 8, 16, 8, 16, 8, 16, 8, 16});
        color_list.clear();
        color_list.insert(color_list.end(), {0, 0, 0, 0, 0, 2, 2, 4, 4, 6, 6});

        for (auto label : labels) {
            bit_depth_cb.append(label);
        }
        labels.clear();

        bit_depth_cb.set_active_text("RGBA_8");
        bit_depth_cb.set_hexpand();
        Gtk::Label *bit_depth_label = Gtk::manage(new Gtk::Label(_("Bit Depth"), Gtk::ALIGN_START));
        grid->attach(*bit_depth_label, 0, row, 1, 1);
        grid->attach(bit_depth_cb, 1, row, 1, 1);
        row++;
    }
    {
        labels.insert(labels.end(), {"Z_NO_COMPRESSION", "Z_BEST_SPEED", "2", "3", "4", "5", "Z_DEFAULT_COMPRESSION",
                                     "7", "8", "Z_BEST_COMPRESSION"});
        compression_list.clear();
        compression_list.insert(compression_list.end(), {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
        for (auto label : labels) {
            compression_cb.append(label);
        }
        labels.clear();

        compression_cb.set_active_text("Z_DEFAULT_COMPRESSION");
        Gtk::Label *compression_label = Gtk::manage(new Gtk::Label(_("Compression"), Gtk::ALIGN_START));
        grid->attach(*compression_label, 0, row, 1, 1);
        grid->attach(compression_cb, 1, row, 1, 1);
        row++;
    }

    {
        auto pHYs_adj = Gtk::Adjustment::create(0, 0, 100000, 0.1, 1.0, 0);
        pHYs_sb.set_adjustment(pHYs_adj);
        pHYs_sb.set_width_chars(7);
        pHYs_sb.set_digits(2);
        Gtk::Label *phys_dpi_label = Gtk::manage(new Gtk::Label(_("pHYs DPI"), Gtk::ALIGN_START));
        grid->attach(*phys_dpi_label, 0, row, 1, 1);
        grid->attach(pHYs_sb, 1, row, 1, 1);
        row++;
    }
    {
        labels.insert(labels.end(), {"CAIRO_ANTIALIAS_NONE", "CAIRO_ANTIALIAS_FAST", "CAIRO_ANTIALIAS_GOOD (default)",
                                     "CAIRO_ANTIALIAS_BEST"});
        anti_aliasing_list.clear();
        anti_aliasing_list.insert(anti_aliasing_list.end(), {0, 1, 2, 3});

        for (auto label : labels) {
            anti_aliasing_cb.append(label);
        }
        labels.clear();

        anti_aliasing_cb.set_active_text("CAIRO_ANTIALIAS_GOOD (default)");
        Gtk::Label *anti_aliasing_label = Gtk::manage(new Gtk::Label(_("Anti Aliasing"), Gtk::ALIGN_START));
        grid->attach(*anti_aliasing_label, 0, row, 1, 1);
        grid->attach(anti_aliasing_cb, 1, row, 1, 1);
        row++;
    }
}

AdvanceOptions::~AdvanceOptions()
{
    ;
}

bool ExtensionList::list_created{false};
std::map<Glib::ustring, Inkscape::Extension::Output *> ExtensionList::valid_extensions{};
std::map<Glib::ustring, Inkscape::Extension::Output *> ExtensionList::all_extensions{};

void ExtensionList::setup()
{
    this->remove_all();
    createList();

    for (auto [key, omod] : valid_extensions) {
        this->append(key);
    }
    this->set_active(0);
}
void ExtensionList::createList()
{
    if (list_created) {
        return;
    }
    Inkscape::Extension::DB::OutputList extensions;
    Inkscape::Extension::db.get_output_list(extensions);
    Glib::ustring extension;
    for (auto omod : extensions) {
        all_extensions[omod->get_extension()] = omod;

        // FIXME: would be nice to grey them out instead of not listing them
        if (omod->deactivated() || (omod->is_raster() != true))
            continue;

        extension = omod->get_extension();
        valid_extensions[extension] = omod;
    }

    // add extentions manually
    Inkscape::Extension::Output *manual_omod;
    manual_omod = dynamic_cast<Inkscape::Extension::Output *>(Inkscape::Extension::db.get(SP_MODULE_KEY_OUTPUT_SVG));
    extension = manual_omod->get_extension();
    valid_extensions[extension] = manual_omod;

    list_created = true;
}

ExtensionList::~ExtensionList()
{
    ;
}
void ExtensionList::setExtensionFromFilename(Glib::ustring const &filename)
{
    Glib::ustring extension = get_ext_from_filename(filename);
    if (valid_extensions[extension]) {
        this->set_active_text(extension);
        return;
    }
}
void ExtensionList::appendExtensionToFilename(Glib::ustring &filename)
{
    Glib::ustring filename_extension = get_ext_from_filename(filename);
    Glib::ustring active_extension = this->get_active_text();
    if (active_extension == filename_extension) {
        return;
    }
    if (valid_extensions[filename_extension]) {
        auto extension_point = filename.rfind(filename_extension);
        filename.erase(extension_point);
    }
    filename = filename + active_extension;
    return;
}
void ExtensionList::appendExtensionToFilename(Glib::ustring &filename, Glib::ustring &extension)
{
    createList();
    Glib::ustring filename_extension = get_ext_from_filename(filename);
    Glib::ustring active_extension = extension;
    if (all_extensions[filename_extension]) {
        auto extension_point = filename.rfind(filename_extension);
        filename.erase(extension_point);
    }
    if (valid_extensions[filename_extension]) {
        active_extension = filename_extension;
    }
    if (!valid_extensions[active_extension]) {
        active_extension = ".png";
    }
    filename = filename + active_extension;
    return;
}

float getValuePx(float value, Unit const *unit)
{
    return Inkscape::Util::Quantity::convert(value, unit, "px");
}

void setValuePx(Glib::RefPtr<Gtk::Adjustment> &adj, double val, Unit const *unit)
{
    auto value = Inkscape::Util::Quantity::convert(val, "px", unit);
    adj->set_value(value);
    return;
}

// We Create filename by removing already present extension in document name and replacing it with extension passed as
// parameter if exxtension is not valid. If document doesn't have a name we use bitmap as defalt name.
Glib::ustring get_default_filename(Glib::ustring &filename_entry_text, Glib::ustring &extension)
{
    Glib::ustring filename;
    if (SP_ACTIVE_DOCUMENT && SP_ACTIVE_DOCUMENT->getDocumentFilename()) {
        SPDocument *doc = SP_ACTIVE_DOCUMENT;
        filename = doc->getDocumentFilename();

        ExtensionList::appendExtensionToFilename(filename, extension);

    } else if (SP_ACTIVE_DOCUMENT) {
        filename = create_filepath_from_id(_("bitmap"), filename_entry_text);
        filename = filename + extension;
    }
    return filename;
}

std::string create_filepath_from_id(Glib::ustring id, const Glib::ustring &file_entry_text)
{
    if (id.empty()) { /* This should never happen */
        id = "bitmap";
    }

    std::string directory;

    if (!file_entry_text.empty()) {
        directory = Glib::path_get_dirname(Glib::filename_from_utf8(file_entry_text));
    }

    if (directory.empty()) {
        /* Grab document directory */
        const gchar *docFilename = SP_ACTIVE_DOCUMENT->getDocumentFilename();
        if (docFilename) {
            directory = Glib::path_get_dirname(docFilename);
        }
    }

    if (directory.empty()) {
        directory = Inkscape::IO::Resource::homedir_path(nullptr);
    }

    return Glib::build_filename(directory, Glib::filename_from_utf8(id));
}

Glib::ustring get_ext_from_filename(Glib::ustring const &filename)
{
    Glib::ustring extension = "";
    if (!filename.empty()) {
        auto extension_point = filename.rfind('.');
        if (extension_point != Glib::ustring::npos) {
            extension = filename.substr(extension_point);
        }
    }
    return extension;
}

std::string absolutize_path_from_document_location(SPDocument *doc, const std::string &filename)
{
    std::string path;
    // Make relative paths go from the document location, if possible:
    if (!Glib::path_is_absolute(filename) && doc->getDocumentFilename()) {
        auto dirname = Glib::path_get_dirname(doc->getDocumentFilename());
        if (!dirname.empty()) {
            path = Glib::build_filename(dirname, filename);
        }
    }
    if (path.empty()) {
        path = filename;
    }
    return path;
}

bool _export_raster(Geom::Rect const &area, unsigned long int const &width, unsigned long int const &height,
                    float const &dpi, Glib::ustring const &filename, bool overwrite,
                    unsigned (*callback)(float, void *), ExportProgressDialog *prog_dialog,
                    Inkscape::Extension::Output *extension, std::vector<SPItem *> *items, AdvanceOptions *adv)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (!desktop)
        return false;
    SPNamedView *nv = desktop->getNamedView();
    SPDocument *doc = desktop->getDocument();

    if (area.hasZeroArea() || width == 0 || height == 0) {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("The chosen area to be exported is invalid."));
        sp_ui_error_dialog(_("The chosen area to be exported is invalid"));
        return false;
    }
    if (filename.empty()) {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("You have to enter a filename."));
        sp_ui_error_dialog(_("You have to enter a filename"));
        return false;
    }

    if (!extension || !extension->is_raster()) {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("Raster Export Error"));
        sp_ui_error_dialog(_("Raster export Method is used for NON RASTER EXTENSION"));
        return false;
    }

    // Advance Parameters default value. We will change them later if adv dialog is provided.
    bool use_interlacing = false; // Maybe use prefs here?
    float pHYs = dpi;             // default is dpi.
    int bit_depth = 8;            // corresponds to RGBA 8
    int color_type = 6;           // corresponds to RGBA 8
    int zlib = 6;                 // Z_DEFAULT_COMPRESSION
    int antialiasing = 2;         // Cairo anti aliasing

    if (adv) {
        use_interlacing = adv->get_interlacing();
        if (adv->get_pHYs() > 0.01) {
            pHYs = adv->get_pHYs();
        }
        bit_depth = adv->get_bit_depth();
        color_type = adv->get_color();
        zlib = adv->get_compression();
        antialiasing = adv->get_anti_aliasing();
    }

    std::string path = absolutize_path_from_document_location(doc, Glib::filename_from_utf8(filename));
    Glib::ustring dirname = Glib::path_get_dirname(path);

    if (dirname.empty() ||
        !Inkscape::IO::file_test(dirname.c_str(), (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))) {
        gchar *safeDir = Inkscape::IO::sanitizeString(dirname.c_str());
        gchar *error = g_strdup_printf(_("Directory %s does not exist or is not a directory.\n"), safeDir);

        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, error);
        sp_ui_error_dialog(error);

        g_free(safeDir);
        g_free(error);
        return false;
    }

    // Do the over-write protection now, since the png is just a temp file.
    if (!overwrite && !sp_ui_overwrite_file(path.c_str())) {
        return false;
    }

    auto fn = Glib::path_get_basename(path);
    auto png_filename = path;
    {
        // Select the extension and set the filename to a temporary file
        int tempfd_out = Glib::file_open_tmp(png_filename, "ink_ext_");
        close(tempfd_out);
    }

    // Export Start Here
    std::vector<SPItem *> selected;
    if (items) {
        selected = *items;
    }

    ExportResult result = sp_export_png_file(desktop->getDocument(), png_filename.c_str(), area, width, height, pHYs,
                                             pHYs, // previously xdpi, ydpi.
                                             nv->pagecolor, callback, (void *)prog_dialog, true, selected,
                                             use_interlacing, color_type, bit_depth, zlib, antialiasing);

    bool failed = result == EXPORT_ERROR || prog_dialog->get_stopped();
    delete prog_dialog;
    if (failed){
        gchar *safeFile = Inkscape::IO::sanitizeString(path.c_str());
        gchar *error = g_strdup_printf(_("Could not export to filename %s.\n"), safeFile);

        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, error);
        sp_ui_error_dialog(error);

        g_free(safeFile);
        g_free(error);
        return false;
    } else if (result == EXPORT_OK) {
        if (extension->prefs()) {
            try {
                extension->export_raster(doc, png_filename, path.c_str(), false);
            } catch (Inkscape::Extension::Output::save_failed &e) {
                return false;
            }
        } else {
            return false;
        }

    } else {
        // Extensions have their own error popup, so this only tracks failures in the png step
        desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Export aborted."));
        return false;
    }

    auto recentmanager = Gtk::RecentManager::get_default();
    if (recentmanager && Glib::path_is_absolute(path)) {
        Glib::ustring uri = Glib::filename_to_uri(path);
        recentmanager->add_item(uri);
    }

    gchar *safeFile = Inkscape::IO::sanitizeString(path.c_str());
    desktop->messageStack()->flashF(Inkscape::INFORMATION_MESSAGE, _("Drawing exported to <b>%s</b>."), safeFile);
    g_free(safeFile);
    

    unlink(png_filename.c_str());
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