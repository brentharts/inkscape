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

class SingleExport : public Gtk::Box
{
public:
    SingleExport(){};
    SingleExport(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade)
        : Gtk::Box(cobject){};
    ~SingleExport() override;

private:
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

private:
    typedef Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> SpinButton;

    std::map<sb_type, SpinButton *> spin_buttons;
    std::map<selection_mode, Gtk::RadioButton *> selection_buttons;
    Gtk::CheckButton *show_export_area = nullptr;
    Inkscape::UI::Widget::UnitMenu *units = nullptr;

    Gtk::CheckButton *si_hide_all = nullptr;
    Gtk::Box *si_preview_box = nullptr;
    Gtk::CheckButton *si_show_preview = nullptr;

    ExtensionList *si_extension_cb = nullptr;
    Gtk::Entry *si_filename_entry = nullptr;
    Gtk::Button *si_export = nullptr;
    Gtk::Box *adv_box = nullptr;

    AdvanceOptions advance_options;

private:
    bool filename_modified;
    Glib::ustring original_name;
    Glib::ustring doc_export_name;

    Inkscape::Preferences *prefs = nullptr;
    std::map<selection_mode, Glib::ustring> selection_names;
    selection_mode current_key;

public:
    // initialise variables from builder
    void initialise(const Glib::RefPtr<Gtk::Builder> &builder);
    void setup();

private:
    void setupUnits();
    void setupExtensionList();

private:
    // change range and callbacks to spinbuttons
    void setupSpinButtons();
    template <typename T>
    void setupSpinButton(Gtk::SpinButton *sb, double val, double min, double max, double step, double page, int digits,
                         bool sensitive, void (SingleExport::*cb)(T), T param);

private:
    // signals callback
    void onRealize();
    void onUnrealize();

    void onAreaXChange(sb_type type);
    void onAreaYChange(sb_type type);
    void onDpiChange(sb_type type);
    void onAreaTypeToggle(selection_mode key);
    void onUnitChanged();
    void onFilenameModified();
    void onExtensionChanged();
    void onExport();
    void on_inkscape_selection_modified(Inkscape::Selection *selection, guint flags);
    void on_inkscape_selection_changed(Inkscape::Selection *selection);

private:
    void refreshArea();
    void refreshExportHints();
    void areaXChange(sb_type type);
    void areaYChange(sb_type type);
    void dpiChange(sb_type type);

    void setArea(double x0, double y0, double x1, double y1);
    void blockSpinConns(bool status);

private:
    void setDefaultSelectionMode();
    void setDefaultFilename();

private:
    // Signals
    std::vector<sigc::connection> spinButtonConns;
    sigc::connection filenameConn;
    sigc::connection extensionConn;
    sigc::connection exportConn;
    sigc::connection selectionModifiedConn;
    sigc::connection selectionChangedConn;

private:
    void on_realize() override;
    void on_unrealize() override;
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