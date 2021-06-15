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

#include <gtkmm.h>
#include <png.h>

#include "document-undo.h"
#include "document.h"
#include "file.h"
#include "inkscape-window.h"
#include "inkscape.h"
#include "preferences.h"
#include "selection-chemistry.h"

// required to set status message after export
#include "desktop.h"
#include "extension/db.h"
#include "helper/png-write.h"
#include "io/resource.h"
#include "io/sys.h"
#include "message-stack.h"
#include "object/sp-namedview.h"
#include "object/sp-root.h"
#include "ui/dialog-events.h"
#include "ui/dialog/dialog-notebook.h"
#include "ui/dialog/filedialog.h"
#include "ui/interface.h"
#include "ui/widget/scrollprotected.h"
#include "ui/widget/unit-menu.h"

#ifdef _WIN32

#endif

#define EXPORT_COORD_PRECISION 3
#define SP_EXPORT_MIN_SIZE 1.0
#define DPI_BASE Inkscape::Util::Quantity::convert(1, "in", "px")

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

    // Callback when container is dinally mapped on window. All intialisation like set active is done inside it.
    container->signal_map().connect(sigc::mem_fun(*this, &Export::onContainerVisible));
    units->signal_changed().connect(sigc::mem_fun(*this, &Export::onUnitChanged));
    for (auto [key, button] : selection_buttons) {
        button->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &Export::onAreaTypeToggle), key));
    }
}

Export::~Export()
{
    selectModifiedConn.disconnect();
    subselChangedConn.disconnect();
    selectChangedConn.disconnect();
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
            return;
        }
    } else {
        selection_buttons[SELECTION_SELECTION]->set_sensitive(true);
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

    return;
}

void Export::initialise_all()
{
    if (builder) {
        // Top container
        builder->get_widget("Export Dialog Box", container);

        // Notebook Start
        {
            builder->get_widget("Export Notebook", export_notebook);
            // Single Image Start
            {
                builder->get_widget("Single Image", single_image);

                builder->get_widget("si_s_document", selection_buttons[SELECTION_DRAWING]);
                selection_names[SELECTION_DRAWING] = "drawing";
                builder->get_widget("si_s_page", selection_buttons[SELECTION_PAGE]);
                selection_names[SELECTION_PAGE] = "page";
                builder->get_widget("si_s_selection", selection_buttons[SELECTION_SELECTION]);
                selection_names[SELECTION_SELECTION] = "selection";
                builder->get_widget("si_s_custom", selection_buttons[SELECTION_CUSTOM]);
                selection_names[SELECTION_CUSTOM] = "custom";

                builder->get_widget_derived("si_left_sb", x0_sb);
                builder->get_widget_derived("si_right_sb", x1_sb);
                builder->get_widget_derived("si_top_sb", y0_sb);
                builder->get_widget_derived("si_bottom_sb", y1_sb);
                builder->get_widget_derived("si_height_sb", height_sb);
                builder->get_widget_derived("si_width_sb", width_sb);

                builder->get_widget_derived("si_img_height_sb", bmheight_sb);
                builder->get_widget_derived("si_img_width_sb", bmwidth_sb);
                builder->get_widget_derived("si_dpi_sb", dpi_sb);

                builder->get_widget("si_show_export_area", show_export_area);
                builder->get_widget_derived("si_units", units);

                builder->get_widget("si_hide_all", hide_all);
                builder->get_widget("si_preview_box", si_preview_box);
                builder->get_widget("si_show_preview", si_show_preview);

                builder->get_widget("si_extention", extension_cb);
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
        extension_cb->append(extension);
        extension_list[extension] = omod;
    }

    // add extentions manually
    Inkscape::Extension::Output *manual_omod;
    manual_omod = dynamic_cast<Inkscape::Extension::Output *>(Inkscape::Extension::db.get(SP_MODULE_KEY_OUTPUT_SVG));
    extension = manual_omod->get_extension();
    extension_cb->append(extension);
    extension_list[extension] = manual_omod;

    extension_cb->set_active(0);
}

void Export::setupSpinButtons()
{
    setupSpinButton<sb_type>(x0_sb, 0.0, -1000000.0, 1000000.0, 0.1, 1.0, EXPORT_COORD_PRECISION, true,
                             &Export::onAreaXChange, SPIN_X0);
    setupSpinButton<sb_type>(x1_sb, 0.0, -1000000.0, 1000000.0, 0.1, 1.0, EXPORT_COORD_PRECISION, true,
                             &Export::onAreaXChange, SPIN_X1);
    setupSpinButton<sb_type>(y0_sb, 0.0, -1000000.0, 1000000.0, 0.1, 1.0, EXPORT_COORD_PRECISION, true,
                             &Export::onAreaYChange, SPIN_Y0);
    setupSpinButton<sb_type>(y1_sb, 0.0, -1000000.0, 1000000.0, 0.1, 1.0, EXPORT_COORD_PRECISION, true,
                             &Export::onAreaYChange, SPIN_Y1);

    setupSpinButton<sb_type>(height_sb, 0.0, 0.0, PNG_UINT_31_MAX, 0.1, 1.0, EXPORT_COORD_PRECISION, true,
                             &Export::onAreaYChange, SPIN_HEIGHT);
    setupSpinButton<sb_type>(width_sb, 0.0, 0.0, PNG_UINT_31_MAX, 0.1, 1.0, EXPORT_COORD_PRECISION, true,
                             &Export::onAreaXChange, SPIN_WIDTH);

    setupSpinButton<sb_type>(bmheight_sb, 1.0, 1.0, 1000000.0, 1.0, 10.0, 3, true, &Export::onDpiChange, SPIN_BMHEIGHT);
    setupSpinButton<sb_type>(bmwidth_sb, 1.0, 1.0, 1000000.0, 1.0, 10.0, 3, true, &Export::onDpiChange, SPIN_BMWIDTH);
    setupSpinButton<sb_type>(dpi_sb, 92.0, 0.01, 100000.0, 0.1, 1.0, 2, true, &Export::onDpiChange, SPIN_DPI);
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
    auto x0_adj = x0_sb->get_adjustment();
    auto x1_adj = x1_sb->get_adjustment();
    auto width_adj = width_sb->get_adjustment();

    float x0, x1, dpi, width, bmwidth;

    // Get all values in px
    x0 = getValuePx(x0_adj->get_value());
    x1 = getValuePx(x1_adj->get_value());
    width = getValuePx(width_adj->get_value());
    bmwidth = bmwidth_sb->get_value();
    dpi = dpi_sb->get_value();

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
    bmwidth_sb->set_value(bmwidth);
}

void Export::areaYChange(sb_type type)
{
    auto y0_adj = y0_sb->get_adjustment();
    auto y1_adj = y1_sb->get_adjustment();
    auto height_adj = height_sb->get_adjustment();

    float y0, y1, dpi, height, bmheight;

    // Get all values in px
    y0 = getValuePx(y0_adj->get_value());
    y1 = getValuePx(y1_adj->get_value());
    height = getValuePx(height_adj->get_value());
    bmheight = bmheight_sb->get_value();
    dpi = dpi_sb->get_value();

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
    bmheight_sb->set_value(bmheight);
}

void Export::dpiChange(sb_type type)
{
    float dpi, height, width, bmheight, bmwidth;

    // Get all values in px
    height = getValuePx(height_sb->get_value());
    width = getValuePx(width_sb->get_value());
    bmheight = bmheight_sb->get_value();
    bmwidth = bmwidth_sb->get_value();
    dpi = dpi_sb->get_value();

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
            break;
        default:
            break;
    }

    bmwidth = floor(width * dpi / DPI_BASE + 0.5);
    bmheight = floor(height * dpi / DPI_BASE + 0.5);

    bmheight_sb->set_value(bmheight);
    bmwidth_sb->set_value(bmwidth);
    dpi_sb->set_value(dpi);
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
    //     export_notebook->set_current_page(page_num);
    // }
}

void Export::setDefaultSelectionMode()
{
    if (SP_ACTIVE_DESKTOP) {
        { // Single Image Selection Mode

            current_key = (selection_mode)0; // default key
            Glib::ustring pref_key_name = prefs->getString("/dialogs/export/exportarea/value");
            for (auto [key, name] : selection_names) {
                if (pref_key_name == name) {
                    current_key = key;
                    break;
                }
            }
            if (current_key == SELECTION_SELECTION && (SP_ACTIVE_DESKTOP->getSelection())->isEmpty()) {
                current_key = (selection_mode)0;
            }
            if ((SP_ACTIVE_DESKTOP->getSelection())->isEmpty()) {
                selection_buttons[SELECTION_SELECTION]->set_sensitive(false);
            }
            selection_buttons[current_key]->set_active(true);
        }

        { // Batch Export Selection Mode
        }
    }
}

void Export::refreshArea()
{
    if (SP_ACTIVE_DESKTOP) {
        SPDocument *doc;
        Geom::OptRect bbox;
        doc = SP_ACTIVE_DESKTOP->getDocument();

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

    // TODO: FilenameModified
}

void Export::setArea(double x0, double y0, double x1, double y1)
{
    blockSpinConns(true);

    auto x0_adj = x0_sb->get_adjustment();
    auto x1_adj = x1_sb->get_adjustment();
    auto y0_adj = y0_sb->get_adjustment();
    auto y1_adj = y1_sb->get_adjustment();

    setValuePx(x1_adj, x1);
    setValuePx(y1_adj, y1);
    setValuePx(x0_adj, x0);
    setValuePx(y0_adj, y0);

    areaXChange(SPIN_X1);
    areaYChange(SPIN_Y1);

    blockSpinConns(false);
}

/**
 * SIGNALS
 */

// Set current page based on preference/last visited page

void Export::onContainerVisible()
{
    setDefaultNotebookPage();
    setDefaultSelectionMode();
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
    // If you have reached here means the current key is active one ( not sure if multiple transitions happen but last
    // call will change values)

    current_key = key;
    prefs->setString("/dialogs/export/exportarea/value", selection_names[current_key]);
    refreshArea();
}

void Export::onUnitChanged()
{
    refreshArea();
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