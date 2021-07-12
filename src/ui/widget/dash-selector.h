// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_DASH_SELECTOR_NEW_H
#define SEEN_SP_DASH_SELECTOR_NEW_H

/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Maximilian Albert <maximilian.albert> (gtkmm-ification)
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm/box.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>

#include <sigc++/signal.h>

#include "scrollprotected.h"

namespace Inkscape {
namespace UI {
namespace Widget {

/**
 * Class that wraps a combobox and spinbutton for selecting dash patterns.
 */
class DashSelector : public Gtk::Box {
public:
    DashSelector();
    ~DashSelector() override;

    /**
     * Get and set methods for dashes
     */
    void set_dash(int ndash, double *dash, double offset);
    void get_dash(int *ndash, double **dash, double *offset);

    sigc::signal<void> changed_signal;

private:

    /**
     * Initialize dashes list from preferences
     */
    static void init_dashes();

    /**
     * Fill a pixbuf with the dash pattern using standard cairo drawing
     */
    Cairo::RefPtr<Cairo::Surface> sp_dash_to_pixbuf(double *pattern);

    /**
     * Fill a pixbuf with text standard cairo drawing
     */
    GdkPixbuf* sp_text_to_pixbuf(char *text);

    /**
     * Callback for combobox image renderer
     */
    void prepareImageRenderer( Gtk::TreeModel::const_iterator const &row );

    /**
     * Callback for offset adjustment changing
     */
    void offset_value_changed();

    /**
     * Callback for combobox selection changing
     */
    void on_selection();

    /**
     * Combobox columns
     */
    class DashColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        Gtk::TreeModelColumn<double *> dash;
        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > pixbuf;
        Gtk::TreeModelColumn<Cairo::RefPtr<Cairo::Surface>> surface;

        DashColumns() {
            add(dash); add(pixbuf); add(surface);
        }
    };
    DashColumns dash_columns;
    Glib::RefPtr<Gtk::ListStore> dash_store;
    ScrollProtected<Gtk::ComboBox> dash_combo;
    Gtk::CellRendererPixbuf image_renderer;
    Glib::RefPtr<Gtk::Adjustment> offset;

    static gchar const *const _prefs_path;
    int preview_width;
    int preview_height;
    int preview_lineheight;

    double *_pattern = nullptr;
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif // SEEN_SP_DASH_SELECTOR_NEW_H

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
