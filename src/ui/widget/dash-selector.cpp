// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Combobox for selecting dash patterns - implementation.
 */
/* Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "dash-selector.h"

#include <cstring>

#include <glibmm/i18n.h>

#include <2geom/coord.h>

#include "preferences.h"

#include "display/cairo-utils.h"

#include "style.h"

#include "ui/dialog-events.h"
#include "ui/widget/spinbutton.h"

namespace Inkscape {
namespace UI {
namespace Widget {

gchar const *const DashSelector::_prefs_path = "/palette/dashes";

static double dash_0[] = {-1.0};
static double dash_1_1[] = {1.0, 1.0, -1.0};
static double dash_2_1[] = {2.0, 1.0, -1.0};
static double dash_4_1[] = {4.0, 1.0, -1.0};
static double dash_1_2[] = {1.0, 2.0, -1.0};
static double dash_1_4[] = {1.0, 4.0, -1.0};

static size_t BD_LEN = 7;  // must correspond to the number of entries in the next line
static double *builtin_dashes[] = {dash_0, dash_1_1, dash_2_1, dash_4_1, dash_1_2, dash_1_4, nullptr};

static double **dashes = nullptr;

DashSelector::DashSelector()
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4),
      preview_width(80),
      preview_height(16),
      preview_lineheight(2)
{
    // TODO: find something more sensible here!!
    init_dashes();

    dash_store = Gtk::ListStore::create(dash_columns);
    dash_combo.set_model(dash_store);
    dash_combo.pack_start(image_renderer);
    dash_combo.set_cell_data_func(image_renderer, sigc::mem_fun(*this, &DashSelector::prepareImageRenderer));
    dash_combo.set_tooltip_text(_("Dash pattern"));
    // dash_combo.get_style_context()->add_class("combobright");
    dash_combo.show();
    dash_combo.signal_changed().connect( sigc::mem_fun(*this, &DashSelector::on_selection) );
    // show dashes in two columns to eliminate or minimize scrolling
    dash_combo.set_wrap_width(2);

    this->pack_start(dash_combo, true, true, 0);

    offset = Gtk::Adjustment::create(0.0, 0.0, 10.0, 0.1, 1.0, 0.0);
    offset->signal_value_changed().connect(sigc::mem_fun(*this, &DashSelector::offset_value_changed));
    auto sb = new Inkscape::UI::Widget::SpinButton(offset, 0.1, 2);
    sb->set_tooltip_text(_("Pattern offset"));
    sp_dialog_defocus_on_enter_cpp(sb);
    sb->show();

    this->pack_start(*sb, false, false, 0);

    int np=0;
    while (dashes[np]){ np++;}
    for (int i = 0; i<np-1; i++) {
        if (i == 1) {
            // add the custom one as a second option; it'll show up at the top of second column
            Gtk::TreeModel::Row row = *(dash_store->append());
            row[dash_columns.dash] = dashes[np-1];
            row[dash_columns.surface] = sp_text_to_pixbuf((char *)"Custom");
        }
        // Add the dashes to the combobox
        Gtk::TreeModel::Row row = *(dash_store->append());
        row[dash_columns.dash] = dashes[i];
        row[dash_columns.surface] = sp_dash_to_pixbuf(dashes[i]);
    }

    _pattern = dashes[0];
}

DashSelector::~DashSelector() {
    // FIXME: for some reason this doesn't get called; does the call to manage() in
    // sp_stroke_style_line_widget_new() not processed correctly?
}

void DashSelector::prepareImageRenderer( Gtk::TreeModel::const_iterator const &row ) {
    Cairo::RefPtr<Cairo::Surface> surface = (*row)[dash_columns.surface];
    image_renderer.property_surface() = surface;
}

void DashSelector::init_dashes() {

    if (!dashes) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        std::vector<Glib::ustring> dash_prefs = prefs->getAllDirs(_prefs_path);
        
        int pos = 0;
        if (!dash_prefs.empty()) {
            SPStyle style;
            dashes = g_new (double *, dash_prefs.size() + 2); // +1 for custom slot, +1 for terminator slot
            
            for (auto & dash_pref : dash_prefs) {
                style.readFromPrefs( dash_pref );
                
                if (!style.stroke_dasharray.values.empty()) {
                    dashes[pos] = g_new (double, style.stroke_dasharray.values.size() + 1);
                    double *d = dashes[pos];
                    unsigned i = 0;
                    for (; i < style.stroke_dasharray.values.size(); i++) {
                        d[i] = style.stroke_dasharray.values[i].value;
                    }
                    d[i] = -1;
                } else {
                    dashes[pos] = dash_0;
                }
                pos += 1;
            }
        } else {  //  This code may never execute - a new preferences.xml is created for a new user.  Maybe if the user deletes dashes from preferences.xml?
            dashes = g_new (double *, BD_LEN + 2); // +1 for custom slot, +1 for terminator slot
            unsigned i;
            for(i=0;i<BD_LEN;i++) {
               dashes[i] = builtin_dashes[i];
            }
            pos = BD_LEN;
        }
        // make a place to hold the custom dashes, up to 15 positions long (+ terminator)
        dashes[pos] = g_new (double, 16);
        double *d = dashes[pos];
        int i=0;
        for(i=0;i<15;i++){ d[i]=i; } // have to put something in there, this is a pattern hopefully nobody would choose
        d[15]=-1.0;
        // final terminator
        dashes[++pos]   = nullptr;
    }
}

void DashSelector::set_dash (int ndash, double *dash, double o)
{
    int pos = -1;    // Allows custom patterns to remain unscathed by this.
    int count = 0;   // will hold the NULL terminator at the end of the dashes list 
    if (ndash > 0) {
        double delta = 0.0;
        for (int i = 0; i < ndash; i++)
            delta += dash[i];
        delta /= 1000.0;

        for (int i = 0; dashes[i]; i++,count++) {
            double *pattern = dashes[i];
            int np = 0;
            while (pattern[np] >= 0.0)
                np += 1;
            if (np == ndash) {
                int j;
                for (j = 0; j < ndash; j++) {
                    if (!Geom::are_near(dash[j], pattern[j], delta)) {
                        break;
                    }
                }
                if (j == ndash) {
                    pos = i;
                    break;
                }
            }
        }
    }
    else  if(ndash==0) {
       pos = 0;
    }

    if(pos>=0){
       _pattern = dashes[pos];
       this->dash_combo.set_active(pos > 0 ? pos + 1 : 0); // skip "custom"
       this->offset->set_value(o);
       if(pos == 10) {
           this->offset->set_value(10.0);
       }
    }
    else { // Hit a custom pattern in the SVG, write it into the combobox.
       count = 1;  // the one slot for custom patterns
       double *d = dashes[count];
       int i=0;
       for(i=0;i< (ndash > 15 ? 15 : ndash) ;i++) {
          d[i]=dash[i];
       } // store the custom pattern
       d[ndash]=-1.0;  //terminate it
       _pattern = dashes[count];
       this->dash_combo.set_active(count);
       this->offset->set_value(o);  // what does this do????
    }
}

void DashSelector::get_dash(int *ndash, double **dash, double *off)
{
    int nd = 0;
    while (_pattern[nd] >= 0.0)
        nd += 1;

    if (nd > 0) {
        if (ndash)
            *ndash = nd;
        if (dash) {
            *dash = g_new (double, nd);
            memcpy (*dash, _pattern, nd * sizeof (double));
        }
        if (off)
            *off = offset->get_value();
    } else {
        if (ndash)
            *ndash = 0;
        if (dash)
            *dash = nullptr;
        if (off)
            *off = 0.0;
    }
}

/**
 * Fill a pixbuf with the dash pattern using standard cairo drawing
 */
Cairo::RefPtr<Cairo::Surface> DashSelector::sp_dash_to_pixbuf(double *pattern) {
    auto device_scale = get_scale_factor();
    int n_dashes;
    for (n_dashes = 0; pattern[n_dashes] >= 0.0; n_dashes ++) ;

    auto height = preview_height * device_scale;
    auto width = preview_width * device_scale;
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *ct = cairo_create(s);

    auto context = get_style_context();
    Gdk::RGBA fg = context->get_color(get_state_flags());

    cairo_set_line_width (ct, preview_lineheight * device_scale);
    cairo_scale (ct, preview_lineheight * device_scale, 1);
    cairo_move_to (ct, 0, height/2);
    cairo_line_to (ct, width, height/2);
    cairo_set_dash(ct, pattern, n_dashes, 0);
    cairo_set_source_rgb(ct, fg.get_red(), fg.get_green(), fg.get_blue());
    cairo_stroke (ct);

    cairo_destroy(ct);
    cairo_surface_flush(s);

    cairo_surface_set_device_scale(s, device_scale, device_scale);
    return Cairo::RefPtr<Cairo::Surface>(new Cairo::Surface(s));
}

/**
 * Fill a pixbuf with a text label using standard cairo drawing
 */
Cairo::RefPtr<Cairo::Surface> DashSelector::sp_text_to_pixbuf(char *text)
{
    auto device_scale = get_scale_factor();
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, preview_width * device_scale, preview_height * device_scale);
    cairo_t *ct = cairo_create(s);

    cairo_select_font_face (ct, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (ct, 12.0 * device_scale);
    auto context = get_style_context();
    Gdk::RGBA fg = context->get_color(get_state_flags());
    cairo_set_source_rgb(ct, fg.get_red(), fg.get_green(), fg.get_blue());
    cairo_move_to (ct, 16.0 * device_scale, 13.0 * device_scale);
    cairo_show_text (ct, text);

    cairo_stroke (ct);

    cairo_destroy(ct);
    cairo_surface_flush(s);

    cairo_surface_set_device_scale(s, device_scale, device_scale);
    return Cairo::RefPtr<Cairo::Surface>(new Cairo::Surface(s));
}

void DashSelector::on_selection ()
{
    double *pattern = dash_combo.get_active()->get_value(dash_columns.dash);
    _pattern = pattern;

    changed_signal.emit();
}

void DashSelector::offset_value_changed()
{
    changed_signal.emit();
}
} // namespace Widget
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
