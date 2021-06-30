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

static std::string create_filepath_from_id(Glib::ustring id, const Glib::ustring &file_entry_text);
static void append_ext_to_filename(Glib::ustring &filename, Glib::ustring &extension);
static Glib::ustring get_default_filename(Glib::ustring &filename_entry_text, Glib::ustring &extension);
static Glib::ustring get_ext_from_filename(Glib::ustring &filename);
static std::string absolutize_path_from_document_location(SPDocument *doc, const std::string &filename);

Export::Export()
    : DialogBase("/dialogs/export/", "Export")
    , filename_modified(false)
    , original_name()
    , doc_export_name()
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
    // initalise all variables from gtk::builder at same place. Makes it easy to sync with glade
    initialise_all();

    // add top level container and show all children.
    add(*container);
    show_all_children();

    prefs = Inkscape::Preferences::get();

    // setting up units beforehand is important as we will use it to initialise other values
    setupUnits();
    setupSpinButtons();
    setupExtensionList();
    setupAdvance();

    // Callback when container is dinally mapped on window. All intialisation like set active is done inside it.
    container->signal_map().connect(sigc::mem_fun(*this, &Export::onContainerVisible));
    units->signal_changed().connect(sigc::mem_fun(*this, &Export::onUnitChanged));
    si_extension_cb->signal_changed().connect(sigc::mem_fun(*this, &Export::onExtensionChanged));
    for (auto [key, button] : selection_buttons) {
        button->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &Export::onAreaTypeToggle), key));
    }
    filenameChangedConn =
        si_filename_entry->signal_changed().connect(sigc::mem_fun(*this, &Export::onFilenameModified));
    siExportConn = si_export->signal_clicked().connect(sigc::mem_fun(*this, &Export::onExport));
}

Export::~Export()
{
    selectModifiedConn.disconnect();
    subselChangedConn.disconnect();
    selectChangedConn.disconnect();

    delete advance_grid;
}

void Export::update()
{
    if (!_app) {
        std::cerr << "Export::update(): _app is null" << std::endl;
        return;
    }
    onSelectionChanged();
    onSelectionModified(0);
#if 0
    setDesktop(getDesktop());
#endif
}

void Export::onSelectionChanged()
{
    Inkscape::Selection *selection = SP_ACTIVE_DESKTOP->getSelection();
    if (selection->isEmpty()) {
        selection_buttons[SELECTION_SELECTION]->set_sensitive(false);
        if (current_key == SELECTION_SELECTION) {
            selection_buttons[(selection_mode)0]->set_active(true); // This causes refresh area
            // return otherwise refreshArea will be called again
            // even though we are at default key, selection is the one which was original key.
            prefs->setString("/dialogs/export/exportarea/value", selection_names[SELECTION_SELECTION]);
            return;
        }
    } else {
        selection_buttons[SELECTION_SELECTION]->set_sensitive(true);
        Glib::ustring pref_key_name = prefs->getString("/dialogs/export/exportarea/value");
        if (selection_names[SELECTION_SELECTION] == pref_key_name && current_key != SELECTION_SELECTION) {
            selection_buttons[SELECTION_SELECTION]->set_active();
            return;
        }
    }
    refreshArea();
}

void Export::onSelectionModified(guint /*flags*/)
{
    if (SP_ACTIVE_DESKTOP) {
        Geom::OptRect bbox;

        switch (current_key) {
            case SELECTION_DRAWING:
                SPDocument *doc;
                doc = SP_ACTIVE_DESKTOP->getDocument();
                bbox = doc->getRoot()->desktopVisualBounds();
                if (bbox) {
                    setArea(bbox->left(), bbox->top(), bbox->right(), bbox->bottom());
                }
                break;
            case SELECTION_SELECTION:
                Inkscape::Selection *Sel;
                Sel = SP_ACTIVE_DESKTOP->getSelection();
                if (Sel->isEmpty() == false) {
                    bbox = Sel->visualBounds();
                    if (bbox) {
                        setArea(bbox->left(), bbox->top(), bbox->right(), bbox->bottom());
                    }
                }
                break;
            default:
                /* Do nothing for page or for custom */
                break;
        }
    }
    refreshExportHints();

    return;
}

void Export::initialise_all()
{
    if (builder) {
        // Top container
        builder->get_widget("Export Dialog Box", container);
        builder->get_widget("Export Notebook", export_notebook);

        builder->get_widget("Single Image", single_image);

        builder->get_widget("si_s_document", selection_buttons[SELECTION_DRAWING]);
        selection_names[SELECTION_DRAWING] = "drawing";
        builder->get_widget("si_s_page", selection_buttons[SELECTION_PAGE]);
        selection_names[SELECTION_PAGE] = "page";
        builder->get_widget("si_s_selection", selection_buttons[SELECTION_SELECTION]);
        selection_names[SELECTION_SELECTION] = "selection";
        builder->get_widget("si_s_custom", selection_buttons[SELECTION_CUSTOM]);
        selection_names[SELECTION_CUSTOM] = "custom";

        builder->get_widget_derived("si_left_sb", spin_buttons[SPIN_X0]);
        builder->get_widget_derived("si_right_sb", spin_buttons[SPIN_X1]);
        builder->get_widget_derived("si_top_sb", spin_buttons[SPIN_Y0]);
        builder->get_widget_derived("si_bottom_sb", spin_buttons[SPIN_Y1]);
        builder->get_widget_derived("si_height_sb", spin_buttons[SPIN_HEIGHT]);
        builder->get_widget_derived("si_width_sb", spin_buttons[SPIN_WIDTH]);

        builder->get_widget_derived("si_img_height_sb", spin_buttons[SPIN_BMHEIGHT]);
        builder->get_widget_derived("si_img_width_sb", spin_buttons[SPIN_BMWIDTH]);
        builder->get_widget_derived("si_dpi_sb", spin_buttons[SPIN_DPI]);

        builder->get_widget("si_show_export_area", show_export_area);
        builder->get_widget_derived("si_units", units);

        builder->get_widget("si_hide_all", si_hide_all);
        builder->get_widget("si_preview_box", si_preview_box);
        builder->get_widget("si_show_preview", si_show_preview);

        builder->get_widget("si_extention", si_extension_cb);
        builder->get_widget("si_filename", si_filename_entry);
        builder->get_widget("si_export", si_export);

        builder->get_widget("Batch Export", batch_export);
    }
    return;
}

void Export::setupUnits()
{
    if (units) {
        units->setUnitType(Inkscape::Util::UNIT_TYPE_LINEAR);
        SPDesktop *desktop = SP_ACTIVE_DESKTOP;
        if (desktop) {
            units->setUnit(desktop->getNamedView()->display_units->abbr);
        }
    }
}

void Export::setupExtensionList()
{
    Inkscape::Extension::DB::OutputList extensions;
    Inkscape::Extension::db.get_output_list(extensions);
    Glib::ustring extension;

    for (auto omod : extensions) {
        // FIXME: would be nice to grey them out instead of not listing them
        if (omod->deactivated() || (omod->is_raster() != true))
            continue;

        extension = omod->get_extension();
        si_extension_cb->append(extension);
        extension_list[extension] = omod;
    }

    // add extentions manually
    Inkscape::Extension::Output *manual_omod;
    manual_omod = dynamic_cast<Inkscape::Extension::Output *>(Inkscape::Extension::db.get(SP_MODULE_KEY_OUTPUT_SVG));
    extension = manual_omod->get_extension();
    si_extension_cb->append(extension);
    extension_list[extension] = manual_omod;
}

void Export::setupAdvance()
{
    advance_grid = new Gtk::Grid;
    int row = 0;
    std::vector<Glib::ustring> labels;
    {
        interlacing.set_label("Use Interlacing");
        advance_grid->attach(interlacing, 0, row, 2, 1);
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
        advance_grid->attach(*bit_depth_label, 0, row, 1, 1);
        advance_grid->attach(bit_depth_cb, 1, row, 1, 1);
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
        advance_grid->attach(*compression_label, 0, row, 1, 1);
        advance_grid->attach(compression_cb, 1, row, 1, 1);
        row++;
    }

    {
        auto pHYs_adj = Gtk::Adjustment::create(0, 0, 100000, 0.1, 1.0, 0);
        phys_dpi_sb.set_adjustment(pHYs_adj);
        phys_dpi_sb.set_width_chars(7);
        phys_dpi_sb.set_digits(2);
        Gtk::Label *phys_dpi_label = Gtk::manage(new Gtk::Label(_("pHYs DPI"), Gtk::ALIGN_START));
        advance_grid->attach(*phys_dpi_label, 0, row, 1, 1);
        advance_grid->attach(phys_dpi_sb, 1, row, 1, 1);
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
        advance_grid->attach(*anti_aliasing_label, 0, row, 1, 1);
        advance_grid->attach(anti_aliasing_cb, 1, row, 1, 1);
        row++;
    }
}

void Export::setupSpinButtons()
{
    setupSpinButton<sb_type>(spin_buttons[SPIN_X0], 0.0, -1000000.0, 1000000.0, 0.1, 1.0, EXPORT_COORD_PRECISION, true,
                             &Export::onAreaXChange, SPIN_X0);
    setupSpinButton<sb_type>(spin_buttons[SPIN_X1], 0.0, -1000000.0, 1000000.0, 0.1, 1.0, EXPORT_COORD_PRECISION, true,
                             &Export::onAreaXChange, SPIN_X1);
    setupSpinButton<sb_type>(spin_buttons[SPIN_Y0], 0.0, -1000000.0, 1000000.0, 0.1, 1.0, EXPORT_COORD_PRECISION, true,
                             &Export::onAreaYChange, SPIN_Y0);
    setupSpinButton<sb_type>(spin_buttons[SPIN_Y1], 0.0, -1000000.0, 1000000.0, 0.1, 1.0, EXPORT_COORD_PRECISION, true,
                             &Export::onAreaYChange, SPIN_Y1);

    setupSpinButton<sb_type>(spin_buttons[SPIN_HEIGHT], 0.0, 0.0, PNG_UINT_31_MAX, 0.1, 1.0, EXPORT_COORD_PRECISION,
                             true, &Export::onAreaYChange, SPIN_HEIGHT);
    setupSpinButton<sb_type>(spin_buttons[SPIN_WIDTH], 0.0, 0.0, PNG_UINT_31_MAX, 0.1, 1.0, EXPORT_COORD_PRECISION,
                             true, &Export::onAreaXChange, SPIN_WIDTH);

    setupSpinButton<sb_type>(spin_buttons[SPIN_BMHEIGHT], 1.0, 1.0, 1000000.0, 1.0, 10.0, 3, true, &Export::onDpiChange,
                             SPIN_BMHEIGHT);
    setupSpinButton<sb_type>(spin_buttons[SPIN_BMWIDTH], 1.0, 1.0, 1000000.0, 1.0, 10.0, 3, true, &Export::onDpiChange,
                             SPIN_BMWIDTH);
    setupSpinButton<sb_type>(spin_buttons[SPIN_DPI], prefs->getDouble("/dialogs/export/defaultxdpi/value", DPI_BASE),
                             0.01, 100000.0, 0.1, 1.0, 2, true, &Export::onDpiChange, SPIN_DPI);
}

template <typename T>
void Export::setupSpinButton(Gtk::SpinButton *sb, double val, double min, double max, double step, double page,
                             int digits, bool sensitive, void (Export::*cb)(T), T param)
{
    if (sb) {
        sb->set_digits(digits);
        sb->set_increments(step, page);
        sb->set_range(min, max);
        sb->set_value(val);
        sb->set_sensitive(sensitive);
        sb->set_width_chars(7);
        if (cb) {
            auto signal = sb->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, cb), param));
            // add signals to list to block all easily
            spinButtonConns.push_back(signal);
        }
    }
}

/**
 * UTILS FUNCTIONS
 */
float Export::getValuePx(float value)
{
    Unit const *unit = units->getUnit();
    return Inkscape::Util::Quantity::convert(value, unit, "px");
}

void Export::setValuePx(Glib::RefPtr<Gtk::Adjustment> &adj, double val)
{
    Unit const *unit = units->getUnit();
    auto value = Inkscape::Util::Quantity::convert(val, "px", unit);
    adj->set_value(value);
    return;
}

void Export::areaXChange(sb_type type)
{
    auto x0_adj = spin_buttons[SPIN_X0]->get_adjustment();
    auto x1_adj = spin_buttons[SPIN_X1]->get_adjustment();
    auto width_adj = spin_buttons[SPIN_WIDTH]->get_adjustment();

    float x0, x1, dpi, width, bmwidth;

    // Get all values in px
    x0 = getValuePx(x0_adj->get_value());
    x1 = getValuePx(x1_adj->get_value());
    width = getValuePx(width_adj->get_value());
    bmwidth = spin_buttons[SPIN_BMWIDTH]->get_value();
    dpi = spin_buttons[SPIN_DPI]->get_value();

    switch (type) {
        case SPIN_X0:
            bmwidth = (x1 - x0) * dpi / DPI_BASE;
            if (bmwidth < SP_EXPORT_MIN_SIZE) {
                x0 = x1 - (SP_EXPORT_MIN_SIZE * DPI_BASE) / dpi;
            }
            break;
        case SPIN_X1:
            bmwidth = (x1 - x0) * dpi / DPI_BASE;
            if (bmwidth < SP_EXPORT_MIN_SIZE) {
                x1 = x0 + (SP_EXPORT_MIN_SIZE * DPI_BASE) / dpi;
            }
            break;
        case SPIN_WIDTH:
            bmwidth = width * dpi / DPI_BASE;
            if (bmwidth < SP_EXPORT_MIN_SIZE) {
                width = (SP_EXPORT_MIN_SIZE * DPI_BASE) / dpi;
            }
            x1 = x0 + width;
            break;
        default:
            break;
    }

    width = x1 - x0;
    bmwidth = floor(width * dpi / DPI_BASE + 0.5);

    setValuePx(x0_adj, x0);
    setValuePx(x1_adj, x1);
    setValuePx(width_adj, width);
    spin_buttons[SPIN_BMWIDTH]->set_value(bmwidth);
}

void Export::areaYChange(sb_type type)
{
    auto y0_adj = spin_buttons[SPIN_Y0]->get_adjustment();
    auto y1_adj = spin_buttons[SPIN_Y1]->get_adjustment();
    auto height_adj = spin_buttons[SPIN_HEIGHT]->get_adjustment();

    float y0, y1, dpi, height, bmheight;

    // Get all values in px
    y0 = getValuePx(y0_adj->get_value());
    y1 = getValuePx(y1_adj->get_value());
    height = getValuePx(height_adj->get_value());
    bmheight = spin_buttons[SPIN_BMHEIGHT]->get_value();
    dpi = spin_buttons[SPIN_DPI]->get_value();

    switch (type) {
        case SPIN_Y0:
            bmheight = (y1 - y0) * dpi / DPI_BASE;
            if (bmheight < SP_EXPORT_MIN_SIZE) {
                y0 = y1 - (SP_EXPORT_MIN_SIZE * DPI_BASE) / dpi;
            }
            break;
        case SPIN_Y1:
            bmheight = (y1 - y0) * dpi / DPI_BASE;
            if (bmheight < SP_EXPORT_MIN_SIZE) {
                y1 = y0 + (SP_EXPORT_MIN_SIZE * DPI_BASE) / dpi;
            }
            break;
        case SPIN_HEIGHT:
            bmheight = height * dpi / DPI_BASE;
            if (bmheight < SP_EXPORT_MIN_SIZE) {
                height = (SP_EXPORT_MIN_SIZE * DPI_BASE) / dpi;
            }
            y1 = y0 + height;
            break;
        default:
            break;
    }

    height = y1 - y0;
    bmheight = floor(height * dpi / DPI_BASE + 0.5);

    setValuePx(y0_adj, y0);
    setValuePx(y1_adj, y1);
    setValuePx(height_adj, height);
    spin_buttons[SPIN_BMHEIGHT]->set_value(bmheight);
}

void Export::dpiChange(sb_type type)
{
    float dpi, height, width, bmheight, bmwidth;

    // Get all values in px
    height = getValuePx(spin_buttons[SPIN_HEIGHT]->get_value());
    width = getValuePx(spin_buttons[SPIN_WIDTH]->get_value());
    bmheight = spin_buttons[SPIN_BMHEIGHT]->get_value();
    bmwidth = spin_buttons[SPIN_BMWIDTH]->get_value();
    dpi = spin_buttons[SPIN_DPI]->get_value();

    switch (type) {
        case SPIN_BMHEIGHT:
            if (bmheight < SP_EXPORT_MIN_SIZE) {
                bmheight = SP_EXPORT_MIN_SIZE;
            }
            dpi = bmheight * DPI_BASE / height;
            break;
        case SPIN_BMWIDTH:
            if (bmwidth < SP_EXPORT_MIN_SIZE) {
                bmwidth = SP_EXPORT_MIN_SIZE;
            }
            dpi = bmwidth * DPI_BASE / width;
            break;
        case SPIN_DPI:
            prefs->setDouble("/dialogs/export/defaultdpi/value", dpi);
            break;
        default:
            break;
    }

    bmwidth = floor(width * dpi / DPI_BASE + 0.5);
    bmheight = floor(height * dpi / DPI_BASE + 0.5);

    spin_buttons[SPIN_BMHEIGHT]->set_value(bmheight);
    spin_buttons[SPIN_BMWIDTH]->set_value(bmwidth);
    spin_buttons[SPIN_DPI]->set_value(dpi);
}

void Export::blockSpinConns(bool status = true)
{
    for (auto signal : spinButtonConns) {
        if (status) {
            signal.block();
        } else {
            signal.unblock();
        }
    }
}

void Export::setDefaultNotebookPage()
{
    // if (export_notebook && batch_export) {
    //     auto page_num = export_notebook->page_num(*batch_export);
    // export_notebook->set_current_page(page_num);
    // }
}

void Export::setDefaultSelectionMode()
{
    if (SP_ACTIVE_DESKTOP) {
        { // Single Image Selection Mode

            current_key = (selection_mode)0; // default key
            bool found = false;
            Glib::ustring pref_key_name = prefs->getString("/dialogs/export/exportarea/value");
            for (auto [key, name] : selection_names) {
                if (pref_key_name == name) {
                    current_key = key;
                    found = true;
                    break;
                }
            }
            if (!found) {
                pref_key_name = selection_names[current_key];
            }

            if (current_key == SELECTION_SELECTION && (SP_ACTIVE_DESKTOP->getSelection())->isEmpty()) {
                current_key = (selection_mode)0;
            }
            if ((SP_ACTIVE_DESKTOP->getSelection())->isEmpty()) {
                selection_buttons[SELECTION_SELECTION]->set_sensitive(false);
            }
            selection_buttons[current_key]->set_active(true);
            prefs->setString("/dialogs/export/exportarea/value", pref_key_name);
        }

        { // Batch Export Selection Mode
        }
    }
}

void Export::setDefaultFilename()
{
    Glib::ustring filename;
    float xdpi = 0.0, ydpi = 0.0;
    SPDocument *doc = SP_ACTIVE_DOCUMENT;
    sp_document_get_export_hints(doc, filename, &xdpi, &ydpi);
    if (filename.empty()) {
        Glib::ustring filename_entry_text = si_filename_entry->get_text();
        Glib::ustring extention_entry_text = ".png";
        filename = get_default_filename(filename_entry_text, extention_entry_text);
    }
    doc_export_name = filename;
    original_name = filename;
    si_filename_entry->set_text(filename);
    si_filename_entry->set_position(filename.length());
    // We only need to check xdpi
    if (xdpi != 0.0) {
        spin_buttons[SPIN_DPI]->set_value(xdpi);
    }
    refreshExportHints();
}

void Export::refreshArea()
{
    if (SP_ACTIVE_DESKTOP) {
        SPDocument *doc;
        Geom::OptRect bbox;
        doc = SP_ACTIVE_DESKTOP->getDocument();
        doc->ensureUpToDate();

        switch (current_key) {
            case SELECTION_SELECTION:
                if ((SP_ACTIVE_DESKTOP->getSelection())->isEmpty() == false) {
                    bbox = SP_ACTIVE_DESKTOP->getSelection()->visualBounds();
                    break;
                }
            case SELECTION_DRAWING:
                bbox = doc->getRoot()->desktopVisualBounds();
                if (bbox) {
                    break;
                }
            case SELECTION_PAGE:
                bbox = Geom::Rect(Geom::Point(0.0, 0.0),
                                  Geom::Point(doc->getWidth().value("px"), doc->getHeight().value("px")));
                break;
            case SELECTION_CUSTOM:
                break;
            default:
                break;
        }
        if (current_key != SELECTION_CUSTOM && bbox) {
            setArea(bbox->min()[Geom::X], bbox->min()[Geom::Y], bbox->max()[Geom::X], bbox->max()[Geom::Y]);
        }
    }
}

void Export::refreshExportHints()
{
    if (SP_ACTIVE_DESKTOP && !filename_modified) {
        SPDocument *doc = SP_ACTIVE_DOCUMENT;
        Glib::ustring filename;
        float xdpi = 0.0, ydpi = 0.0;
        switch (current_key) {
            case SELECTION_CUSTOM:
            case SELECTION_PAGE:
            case SELECTION_DRAWING:
                sp_document_get_export_hints(doc, filename, &xdpi, &ydpi);
                if (filename.empty()) {
                    Glib::ustring filename_entry_text = si_filename_entry->get_text();
                    Glib::ustring extension_entry_text = si_extension_cb->get_active_text();
                    filename = get_default_filename(filename_entry_text, extension_entry_text);
                }
                doc_export_name = filename;
                break;
            case SELECTION_SELECTION:
                if ((SP_ACTIVE_DESKTOP->getSelection())->isEmpty()) {
                    break;
                }
                SP_ACTIVE_DESKTOP->getSelection()->getExportHints(filename, &xdpi, &ydpi);

                /* If we still don't have a filename -- let's build
                   one that's nice */
                if (filename.empty()) {
                    const gchar *id = "object";
                    auto reprlst = SP_ACTIVE_DESKTOP->getSelection()->xmlNodes();
                    for (auto i = reprlst.begin(); reprlst.end() != i; ++i) {
                        Inkscape::XML::Node *repr = *i;
                        if (repr->attribute("id")) {
                            id = repr->attribute("id");
                            break;
                        }
                    }
                    filename = create_filepath_from_id(id, si_filename_entry->get_text());
                    filename = filename + si_extension_cb->get_active_text();
                }
                break;
            default:
                break;
        }
        if (!filename.empty()) {
            original_name = filename;
            si_filename_entry->set_text(filename);
            si_filename_entry->set_position(filename.length());
        }

        if (xdpi != 0.0) {
            spin_buttons[SPIN_DPI]->set_value(xdpi);
        }
    }
}

void Export::setArea(double x0, double y0, double x1, double y1)
{
    blockSpinConns(true);

    auto x0_adj = spin_buttons[SPIN_X0]->get_adjustment();
    auto x1_adj = spin_buttons[SPIN_X1]->get_adjustment();
    auto y0_adj = spin_buttons[SPIN_Y0]->get_adjustment();
    auto y1_adj = spin_buttons[SPIN_Y1]->get_adjustment();

    setValuePx(x1_adj, x1);
    setValuePx(y1_adj, y1);
    setValuePx(x0_adj, x0);
    setValuePx(y0_adj, y0);

    areaXChange(SPIN_X1);
    areaYChange(SPIN_Y1);

    blockSpinConns(false);
}

Glib::ustring Export::getValidExtension(Glib::ustring &extension, Glib::ustring &original_extension)
{
    if (extension_list[extension]) {
        return extension;
    }
    if (!original_extension.empty()) {
        return original_extension;
    }
    return ".png";
}

/**
 * SIGNALS
 */

// Set current page based on preference/last visited page

void Export::onContainerVisible()
{
    std::cout << "Container Visible" << std::endl;
    setDefaultNotebookPage();
    setDefaultSelectionMode();
    setDefaultFilename();
    si_extension_cb->set_active(0);
}

void Export::onAreaXChange(sb_type type)
{
    blockSpinConns(true);
    areaXChange(type);
    blockSpinConns(false);
}
void Export::onAreaYChange(sb_type type)
{
    blockSpinConns(true);
    areaYChange(type);
    blockSpinConns(false);
}
void Export::onDpiChange(sb_type type)
{
    blockSpinConns(true);
    dpiChange(type);
    blockSpinConns(false);
}

void Export::onAreaTypeToggle(selection_mode key)
{
    // Prevent executing function twice
    if (!selection_buttons[key]->get_active()) {
        return;
    }
    // If you have reached here means the current key is active one ( not sure if multiple transitions happen but
    // last call will change values)
    current_key = key;
    prefs->setString("/dialogs/export/exportarea/value", selection_names[current_key]);
    refreshArea();
    refreshExportHints();
}

void Export::onUnitChanged()
{
    refreshArea();
}

void Export::onFilenameModified()
{
    filenameChangedConn.block();
    Glib::ustring filename = si_filename_entry->get_text();
    Glib::ustring filename_extension = get_ext_from_filename(filename);
    Glib::ustring active_extension = si_extension_cb->get_active_text();
    Glib::ustring filtered_extension = getValidExtension(filename_extension, active_extension);

    if (original_name == filename) {
        filename_modified = false;
    } else {
        filename_modified = true;
    }
    std::cout << "Original Name: " << original_name << std::endl;
    std::cout << "Filename Name: " << filename << std::endl;

    si_extension_cb->set_active_text(filtered_extension);

    filenameChangedConn.unblock();
}

void Export::onExtensionChanged()
{
    filenameChangedConn.block();
    Glib::ustring filename = si_filename_entry->get_text();
    Glib::ustring filename_extension = get_ext_from_filename(filename);
    Glib::ustring active_extension = si_extension_cb->get_active_text();
    if (filename_extension == active_extension) {
        return;
    }
    if (extension_list[filename_extension]) {
        auto extension_point = filename.rfind(filename_extension);
        filename.erase(extension_point);
    }
    filename = filename + active_extension;
    si_filename_entry->set_text(filename);
    si_filename_entry->set_position(filename.length());

    filenameChangedConn.unblock();
}

// Single Export Callback.
// Do not add batch export widgets check here
void Export::onExport()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (!desktop)
        return;
    si_export->set_sensitive(false);
    bool exportSuccessful = false;
    auto extension = si_extension_cb->get_active_text();
    if (!extension_list[extension]) {
        si_export->set_sensitive(true);
        return;
    }
    if (extension_list[extension]->is_raster()) {
        double x0 = getValuePx(spin_buttons[SPIN_X0]->get_value());
        double x1 = getValuePx(spin_buttons[SPIN_X1]->get_value());
        double y0 = getValuePx(spin_buttons[SPIN_Y0]->get_value());
        double y1 = getValuePx(spin_buttons[SPIN_Y1]->get_value());
        auto area = Geom::Rect(Geom::Point(x0, y0), Geom::Point(x1, y1)) * desktop->dt2doc();

        int width = int(spin_buttons[SPIN_BMWIDTH]->get_value() + 0.5);
        int height = int(spin_buttons[SPIN_BMHEIGHT]->get_value() + 0.5);

        double dpi = spin_buttons[SPIN_DPI]->get_value();

        Glib::ustring filename = si_filename_entry->get_text();

        exportSuccessful = _export_raster(area, width, height, dpi, filename, extension_list[extension]);

    } else {
        exportSuccessful = _export_vector(extension_list[extension]);
    }
    si_export->set_sensitive(true);
}

bool Export::_export_raster(Geom::Rect const &area, unsigned long int const &width, unsigned long int const &height,
                            float const &dpi, Glib::ustring const &filename, bool overwrite,
                            Inkscape::Extension::Output *extension)
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

    // Advance Parameters
    const bool use_interlacing = interlacing.get_active();
    const float pHYs = (phys_dpi_sb.get_value() > 0.01) ? phys_dpi_sb.get_value() : dpi;
    const int color_type = color_list[bit_depth_cb.get_active_row_number()];
    const int bit_depth = bit_depth_list[bit_depth_cb.get_active_row_number()];
    const int antialiasing = anti_aliasing_list[anti_aliasing_cb.get_active_row_number()];
    const int zlib = compression_list[compression_cb.get_active_row_number()];

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
    if (!overwrite && !sp_ui_overwrite_file(filename.c_str())) {
        return false;
    }

    auto fn = Glib::path_get_basename(path);
    auto png_filename = std::string(path.c_str());
    // Select the extension and set the filename to a temporary file
    int tempfd_out = Glib::file_open_tmp(png_filename, "ink_ext_");
    close(tempfd_out);
    

    return true;
}

bool Export::_export_vector(Inkscape::Extension::Output *extension, std::vector<SPItem *> *items)
{
    ;
    return true;
}

/**
 * UTILS FUNCTIONS
 */

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

Glib::ustring get_ext_from_filename(Glib::ustring &filename)
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

void append_ext_to_filename(Glib::ustring &filename, Glib::ustring &extention)
{
    if (Glib::str_has_suffix(filename, extention)) {
        return;
    }
    filename = filename + extention;
    return;
}

Glib::ustring get_default_filename(Glib::ustring &filename_entry_text, Glib::ustring &extension)
{
    Glib::ustring filename;
    if (SP_ACTIVE_DOCUMENT && SP_ACTIVE_DOCUMENT->getDocumentFilename()) {
        SPDocument *doc = SP_ACTIVE_DOCUMENT;
        filename = doc->getDocumentFilename();
        auto &&text_extension = get_file_save_extension(Inkscape::Extension::FILE_SAVE_METHOD_SAVE_AS);
        Inkscape::Extension::Output *oextension = nullptr;
        if (!text_extension.empty()) {
            oextension =
                dynamic_cast<Inkscape::Extension::Output *>(Inkscape::Extension::db.get(text_extension.c_str()));
        }
        if (oextension != nullptr) {
            Glib::ustring old_extension = oextension->get_extension();
            if (Glib::str_has_suffix(filename, old_extension)) {
                auto extension_point = filename.rfind(old_extension);
                filename.erase(extension_point);
            }
        }
        filename = filename + extension;

    } else if (SP_ACTIVE_DOCUMENT) {
        filename = create_filepath_from_id(_("bitmap"), filename_entry_text);
        filename = filename + extension;
    }
    return filename;
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