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

    std::map<selection_mode, Gtk::RadioButton *> selection_buttons;

    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> *x0_sb = nullptr;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> *x1_sb = nullptr;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> *y0_sb = nullptr;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> *y1_sb = nullptr;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> *height_sb = nullptr;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> *width_sb = nullptr;

    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> *bmheight_sb = nullptr;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> *bmwidth_sb = nullptr;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> *dpi_sb = nullptr;

    Gtk::CheckButton *show_export_area = nullptr;
    Inkscape::UI::Widget::UnitMenu *units = nullptr;

    Gtk::CheckButton *hide_all = nullptr;
    Gtk::Box *si_preview_box = nullptr;
    Gtk::CheckButton *si_show_preview = nullptr;

    Gtk::Grid *advance_grid = nullptr;

    Gtk::ComboBoxText *extension_cb = nullptr;
    Gtk::Entry *filename_entry = nullptr;
    Gtk::Button *si_export = nullptr;

    Gtk::Box *batch_export = nullptr;

    // Utils Variables
    Inkscape::Preferences *prefs = nullptr;
    std::map<selection_mode, Glib::ustring> selection_names;
    selection_mode current_key;
    std::map<Glib::ustring, Inkscape::Extension::Output *> extension_list;

    // Advanced
    Gtk::CheckButton interlacing;
    std::vector<int> bit_depth_list;
    std::vector<int> color_list;
    Inkscape::UI::Widget::ScrollProtected<Gtk::ComboBoxText> bit_depth_cb;
    std::vector<int> compression_list;
    Inkscape::UI::Widget::ScrollProtected<Gtk::ComboBoxText> compression_cb;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> phys_dpi_sb;
    std::vector<int> anti_aliasing_list;
    Inkscape::UI::Widget::ScrollProtected<Gtk::ComboBoxText> anti_aliasing_cb;

    // Once user change filename it is set and prevent automatic changes to filename_entry
    bool filename_modified;
    // original name for export. Changes everytime selection changes or when exported.
    Glib::ustring original_name;
    // initialised only at startup and is used as fallback for original name.
    Glib::ustring doc_export_name;

    // Initialise all objects from builder
    void initialise_all();
    // Add units from db
    void setupUnits();
    void setupExtensionList();
    void setupAdvance();

    // change range and callbacks to spinbuttons
    void setupSpinButtons();
    template <typename T>
    void setupSpinButton(Gtk::SpinButton *sb, double val, double min, double max, double step, double page, int digits,
                         bool sensitive, void (Export::*cb)(T), T param);

    // setup default values of widgets
    void setDefaultNotebookPage();
    void setDefaultSelectionMode();
    void setDefaultFilename();

    // Utils Functions
    void areaXChange(sb_type type);
    void areaYChange(sb_type type);
    void dpiChange(sb_type type);
    float getValuePx(float value);
    void setValuePx(Glib::RefPtr<Gtk::Adjustment> &adj, double val);
    Glib::ustring getValidExtension(Glib::ustring &extension, Glib::ustring &original_extension);

    void blockSpinConns(bool status);

    void refreshArea();
    void refreshExportHints();
    void setArea(double x0, double y0, double x1, double y1);

    // Export Functions
    bool _export_raster(Inkscape::Extension::Output *extension = nullptr, std::vector<SPItem *> *items = nullptr);
    bool _export_vector(Inkscape::Extension::Output *extension = nullptr, std::vector<SPItem *> *items = nullptr);

    // signals callback
    void onContainerVisible();
    void onAreaXChange(sb_type type);
    void onAreaYChange(sb_type type);
    void onDpiChange(sb_type type);
    void onAreaTypeToggle(selection_mode key);
    void onUnitChanged();
    void onExport();
    void onBatchExport();
    void onFilenameModified();
    void onExtensionChanged();

    /**
     * Inkscape selection change callback
     */
    void onSelectionChanged();
    void onSelectionModified(guint flags);

    /**
     * Update active window.
     */
    void update() override;

    // signals
    sigc::connection selectChangedConn;
    sigc::connection subselChangedConn;
    sigc::connection selectModifiedConn;
    std::vector<sigc::connection> spinButtonConns;
    sigc::connection filenameChangedConn;
    sigc::connection siExportConn;
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