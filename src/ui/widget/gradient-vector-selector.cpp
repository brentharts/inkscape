// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gradient vector selection widget
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   MenTaLguY <mental@rydia.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 * Copyright (C) 2004 Monash University
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2006 MenTaLguY
 * Copyright (C) 2010 Jon A. Cruz
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 */

#include "ui/widget/gradient-vector-selector.h"

#include <set>

#include <glibmm.h>
#include <glibmm/i18n.h>

#include "gradient-chemistry.h"
#include "inkscape.h"
#include "preferences.h"
#include "desktop.h"
#include "document-undo.h"
#include "layer-manager.h"
#include "include/macros.h"
#include "selection-chemistry.h"
#include "verbs.h"

#include "io/resource.h"

#include "object/sp-defs.h"
#include "object/sp-linear-gradient.h"
#include "object/sp-radial-gradient.h"
#include "object/sp-root.h"
#include "object/sp-stop.h"
#include "style.h"

#include "svg/css-ostringstream.h"

#include "ui/dialog-events.h"
#include "ui/selected-color.h"
#include "ui/widget/color-notebook.h"
#include "ui/widget/color-preview.h"
#include "ui/widget/gradient-image.h"

#include "xml/repr.h"

using Inkscape::DocumentUndo;
using Inkscape::UI::SelectedColor;

static SPStop *get_selected_stop( GtkWidget *vb);
void gr_get_usage_counts(SPDocument *doc, std::map<SPGradient *, gint> *mapUsageCount );
unsigned long sp_gradient_to_hhssll(SPGradient *gr);

// TODO FIXME kill these globals!!!
static Glib::ustring const prefs_path = "/dialogs/gradienteditor/";

namespace Inkscape {
namespace UI {
namespace Widget {

GradientVectorSelector::GradientVectorSelector(SPDocument *doc, SPGradient *gr)
{
    _columns = new GradientSelector::ModelColumns();
    _store = Gtk::ListStore::create(*_columns);
    set_orientation(Gtk::ORIENTATION_VERTICAL);

    if (doc) {
        set_gradient(doc, gr);
    } else {
        rebuild_gui_full();
    }
}

GradientVectorSelector::~GradientVectorSelector()
{
    if (_gr) {
        _gradient_release_connection.disconnect();
        _tree_select_connection.disconnect();
        _gr = nullptr;
    }

    if (_doc) {
        _defs_release_connection.disconnect();
        _defs_modified_connection.disconnect();
        _doc = nullptr;
    }
}

void GradientVectorSelector::set_gradient(SPDocument *doc, SPGradient *gr)
{
//     g_message("sp_gradient_vector_selector_set_gradient(%p, %p, %p) [%s] %d %d", gvs, doc, gr,
//               (gr ? gr->getId():"N/A"),
//               (gr ? gr->isSwatch() : -1),
//               (gr ? gr->isSolid() : -1));
    static gboolean suppress = FALSE;

    g_return_if_fail(!gr || (doc != nullptr));
    g_return_if_fail(!gr || SP_IS_GRADIENT(gr));
    g_return_if_fail(!gr || (gr->document == doc));
    g_return_if_fail(!gr || gr->hasStops());

    if (doc != _doc) {
        /* Disconnect signals */
        if (_gr) {
            _gradient_release_connection.disconnect();
            _gr = nullptr;
        }
        if (_doc) {
            _defs_release_connection.disconnect();
            _defs_modified_connection.disconnect();
            _doc = nullptr;
        }

        // Connect signals
        if (doc) {
            _defs_release_connection = doc->getDefs()->connectRelease(sigc::mem_fun(this, &GradientVectorSelector::defs_release));
            _defs_modified_connection = doc->getDefs()->connectModified(sigc::mem_fun(this, &GradientVectorSelector::defs_modified));
        }
        if (gr) {
            _gradient_release_connection = gr->connectRelease(sigc::mem_fun(this, &GradientVectorSelector::gradient_release));
        }
        _doc = doc;
        _gr = gr;
        rebuild_gui_full();
        if (!suppress) _signal_vector_set.emit(gr);
    } else if (gr != _gr) {
        // Harder case - keep document, rebuild list and stuff
        // fixme: (Lauris)
        suppress = TRUE;
        set_gradient(nullptr, nullptr);
        set_gradient(doc, gr);
        suppress = FALSE;
        _signal_vector_set.emit(gr);
    }
    /* The case of setting NULL -> NULL is not very interesting */
}

void
GradientVectorSelector::gradient_release(SPObject * /*obj*/)
{
    /* Disconnect gradient */
    if (_gr) {
        _gradient_release_connection.disconnect();
        _gr = nullptr;
    }

    /* Rebuild GUI */
    rebuild_gui_full();
}

void
GradientVectorSelector::defs_release(SPObject * /*defs*/)
{
    _doc = nullptr;

    _defs_release_connection.disconnect();
    _defs_modified_connection.disconnect();

    /* Disconnect gradient as well */
    if (_gr) {
        _gradient_release_connection.disconnect();
        _gr = nullptr;
    }

    /* Rebuild GUI */
    rebuild_gui_full();
}

void
GradientVectorSelector::defs_modified(SPObject *defs, guint flags)
{
    /* fixme: We probably have to check some flags here (Lauris) */
    rebuild_gui_full();
}

void
GradientVectorSelector::rebuild_gui_full()
{
    _tree_select_connection.block();

    /* Clear old list, if there is any */
    _store->clear();

    /* Pick up all gradients with vectors */
    std::vector<SPGradient *> gl;
    if (_gr) {
        auto gradients = _gr->document->getResourceList("gradient");
        for (auto gradient : gradients) {
            SPGradient* grad = SP_GRADIENT(gradient);
            if ( grad->hasStops() && (grad->isSwatch() == _swatched) ) {
                gl.push_back(SP_GRADIENT(gradient));
            }
        }
    }

    /* Get usage count of all the gradients */
    std::map<SPGradient *, gint> usageCount;
    gr_get_usage_counts(_doc, &usageCount);

    if (!_doc) {
        Gtk::TreeModel::Row row = *(_store->append());
        row[_columns->name] = _("No document selected");

    } else if (gl.empty()) {
        Gtk::TreeModel::Row row = *(_store->append());
        row[_columns->name] = _("No gradients in document");

    } else if (!_gr) {
        Gtk::TreeModel::Row row = *(_store->append());
        row[_columns->name] =  _("No gradient selected");

    } else {
        for (auto gr:gl) {
            unsigned long hhssll = sp_gradient_to_hhssll(gr);
            GdkPixbuf *pixb = sp_gradient_to_pixbuf (gr, 64, 18);
            Glib::ustring label = gr_prepare_label(gr);

            Gtk::TreeModel::Row row = *(_store->append());
            row[_columns->name] = label.c_str();
            row[_columns->color] = hhssll;
            row[_columns->refcount] = usageCount[gr];
            row[_columns->data] = gr;
            row[_columns->pixbuf] = Glib::wrap(pixb);
        }
    }

    _tree_select_connection.unblock();
}

void
GradientVectorSelector::setSwatched()
{
    _swatched = true;
    rebuild_gui_full();
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

Glib::ustring gr_prepare_label(SPObject *obj)
{
    const gchar *id = obj->label() ? obj->label() : obj->getId();
    if (!id) {
        id = obj->getRepr()->name();
    }

    if (strlen(id) > 14 && (!strncmp (id, "linearGradient", 14) || !strncmp (id, "radialGradient", 14)))
        return gr_ellipsize_text(id+14, 35);
    return gr_ellipsize_text (id, 35);
}

/*
 * Ellipse text if longer than maxlen, "50% start text + ... + ~50% end text"
 * Text should be > length 8 or just return the original text
 */
Glib::ustring gr_ellipsize_text(Glib::ustring const &src, size_t maxlen)
{
    if (src.length() > maxlen && maxlen > 8) {
        size_t p1 = (size_t) maxlen / 2;
        size_t p2 = (size_t) src.length() - (maxlen - p1 - 1);
        return src.substr(0, p1) + "…" + src.substr(p2);
    }
    return src;
}


/*
 *  Return a "HHSSLL" version of the first stop color so we can sort by it
 */
unsigned long sp_gradient_to_hhssll(SPGradient *gr)
{
    SPStop *stop = gr->getFirstStop();
    unsigned long rgba = stop->get_rgba32();
    float hsl[3];
    SPColor::rgb_to_hsl_floatv (hsl, SP_RGBA32_R_F(rgba), SP_RGBA32_G_F(rgba), SP_RGBA32_B_F(rgba));

    return ((int)(hsl[0]*100 * 10000)) + ((int)(hsl[1]*100 * 100)) + ((int)(hsl[2]*100 * 1));
}

static void get_all_doc_items(std::vector<SPItem*> &list, SPObject *from)
{
    for (auto& child: from->children) {
        if (SP_IS_ITEM(&child)) {
            list.push_back(SP_ITEM(&child));
        }
        get_all_doc_items(list, &child);
    }
}

/*
 * Return a SPItem's gradient
 */
static SPGradient * gr_item_get_gradient(SPItem *item, gboolean fillorstroke)
{
    SPIPaint *item_paint = item->style->getFillOrStroke(fillorstroke);
    if (item_paint->isPaintserver()) {

        SPPaintServer *item_server = (fillorstroke) ?
                item->style->getFillPaintServer() : item->style->getStrokePaintServer();

        if (SP_IS_LINEARGRADIENT(item_server) || SP_IS_RADIALGRADIENT(item_server) ||
                (SP_IS_GRADIENT(item_server) && SP_GRADIENT(item_server)->getVector()->isSwatch()))  {

            return SP_GRADIENT(item_server)->getVector();
        }
    }

    return nullptr;
}

/*
 * Map each gradient to its usage count for both fill and stroke styles
 */
void gr_get_usage_counts(SPDocument *doc, std::map<SPGradient *, gint> *mapUsageCount )
{
    if (!doc)
        return;

    std::vector<SPItem *> all_list;
    get_all_doc_items(all_list, doc->getRoot());

    for (auto item:all_list) {
        if (!item->getId())
            continue;
        SPGradient *gr = nullptr;
        gr = gr_item_get_gradient(item, true); // fill
        if (gr) {
            mapUsageCount->count(gr) > 0 ? (*mapUsageCount)[gr] += 1 : (*mapUsageCount)[gr] = 1;
        }
        gr = gr_item_get_gradient(item, false); // stroke
        if (gr) {
            mapUsageCount->count(gr) > 0 ? (*mapUsageCount)[gr] += 1 : (*mapUsageCount)[gr] = 1;
        }
    }
}



/*##################################################################
  ###                 Vector Editing Widget
  ##################################################################*/

#include "widgets/widget-sizes.h"
#include "xml/node-event-vector.h"
#include "svg/svg-color.h"

#define PAD 4

static void sp_gradient_vector_widget_load_gradient(GtkWidget *widget, SPGradient *gradient);
static void sp_gradient_vector_gradient_release(SPObject *obj, GtkWidget *widget);
static void sp_gradient_vector_gradient_modified(SPObject *obj, guint flags, GtkWidget *widget);
static void update_stop_list( GtkWidget *vb, SPGradient *gradient, SPStop *new_stop);

static gboolean blocked = FALSE;

static void select_stop_in_list( GtkWidget *vb, SPGradient *gradient, SPStop *new_stop)
{
    GtkWidget *combo_box = static_cast<GtkWidget *>(g_object_get_data(G_OBJECT(vb), "combo_box"));

    int i = 0;
    for (auto& ochild: gradient->children) {
        if (SP_IS_STOP(&ochild)) {
            if (&ochild == new_stop) {
                gtk_combo_box_set_active (GTK_COMBO_BOX(combo_box) , i);
                break;
            }
            i++;
        }
    }
}

static void update_stop_list( GtkWidget *vb, SPGradient *gradient, SPStop *new_stop)
{

    if (!SP_IS_GRADIENT(gradient)) {
        return;
    }

    blocked = TRUE;

    /* Clear old list, if there is any */
    GtkWidget *combo_box = static_cast<GtkWidget *>(g_object_get_data(G_OBJECT(vb), "combo_box"));
    if (!combo_box) {
        return;
    }
    GtkListStore *store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(combo_box)));
    if (!store) {
        return;
    }
    gtk_list_store_clear(store);
    GtkTreeIter iter;

    /* Populate the combobox store */
    std::vector<SPStop *> sl;
    if ( gradient->hasStops() ) {
        for (auto& ochild: gradient->children) {
            if (SP_IS_STOP(&ochild)) {
                sl.push_back(SP_STOP(&ochild));
            }
        }
    }
    if (sl.empty()) {
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter, 0, NULL, 1, _("No stops in gradient"), 2, NULL, -1);
        gtk_widget_set_sensitive (combo_box, FALSE);

    } else {

        for (auto stop:sl) {
            Inkscape::XML::Node *repr = stop->getRepr();
            Inkscape::UI::Widget::ColorPreview *cpv = Gtk::manage(new Inkscape::UI::Widget::ColorPreview(stop->get_rgba32()));
            GdkPixbuf *pb = cpv->toPixbuf(64, 16);
            gtk_list_store_append (store, &iter);
            gtk_list_store_set (store, &iter, 0, pb, 1, repr->attribute("id"), 2, stop, -1);
            gtk_widget_set_sensitive (combo_box, FALSE);
        }
        gtk_widget_set_sensitive(combo_box, TRUE);
    }

    /* Set history */
    if (new_stop == nullptr) {
        gtk_combo_box_set_active (GTK_COMBO_BOX(combo_box) , 0);
    } else {
        select_stop_in_list(vb, gradient, new_stop);
    }

    blocked = FALSE;
}

static SPStop *get_selected_stop( GtkWidget *vb)
{
    SPStop *stop = nullptr;
    GtkWidget *combo_box = static_cast<GtkWidget *>(g_object_get_data(G_OBJECT(vb), "combo_box"));
    if (combo_box) {
        GtkTreeIter  iter;
        if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX(combo_box), &iter)) {
            GtkListStore *store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(combo_box)));
            gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, 2, &stop, -1);
        }
    }
    return stop;
}

guint32 sp_average_color(guint32 c1, guint32 c2, gdouble p/* = 0.5*/)
{
    guint32 r = (guint32) (SP_RGBA32_R_U(c1) * p + SP_RGBA32_R_U(c2) * (1 - p));
    guint32 g = (guint32) (SP_RGBA32_G_U(c1) * p + SP_RGBA32_G_U(c2) * (1 - p));
    guint32 b = (guint32) (SP_RGBA32_B_U(c1) * p + SP_RGBA32_B_U(c2) * (1 - p));
    guint32 a = (guint32) (SP_RGBA32_A_U(c1) * p + SP_RGBA32_A_U(c2) * (1 - p));

    return SP_RGBA32_U_COMPOSE(r, g, b, a);
}

static void sp_gradient_vector_widget_load_gradient(GtkWidget *widget, SPGradient *gradient)
{
    blocked = TRUE;

    SPGradient *old;

    old = static_cast<SPGradient*>(g_object_get_data(G_OBJECT(widget), "gradient"));

    if (old != gradient) {
        sigc::connection *release_connection;
        sigc::connection *modified_connection;

        release_connection = static_cast<sigc::connection *>(g_object_get_data(G_OBJECT(widget), "gradient_release_connection"));
        modified_connection = static_cast<sigc::connection *>(g_object_get_data(G_OBJECT(widget), "gradient_modified_connection"));

        if (old) {
            g_assert( release_connection != nullptr );
            g_assert( modified_connection != nullptr );
            release_connection->disconnect();
            modified_connection->disconnect();
            sp_signal_disconnect_by_data(old, widget);
        }

        if (gradient) {
            if (!release_connection) {
                release_connection = new sigc::connection();
            }
            if (!modified_connection) {
                modified_connection = new sigc::connection();
            }
            *release_connection = gradient->connectRelease(sigc::bind<1>(sigc::ptr_fun(&sp_gradient_vector_gradient_release), widget));
            *modified_connection = gradient->connectModified(sigc::bind<2>(sigc::ptr_fun(&sp_gradient_vector_gradient_modified), widget));
        } else {
            if (release_connection) {
                delete release_connection;
                release_connection = nullptr;
            }
            if (modified_connection) {
                delete modified_connection;
                modified_connection = nullptr;
            }
        }

        g_object_set_data(G_OBJECT(widget), "gradient_release_connection", release_connection);
        g_object_set_data(G_OBJECT(widget), "gradient_modified_connection", modified_connection);
    }

    g_object_set_data(G_OBJECT(widget), "gradient", gradient);

    if (gradient) {
        gtk_widget_set_sensitive(widget, TRUE);

        gradient->ensureVector();

        SPStop *stop = get_selected_stop(widget);
        if (!stop) {
            return;
        }

        // get the color selector
        SelectedColor *csel =  static_cast<SelectedColor*>(g_object_get_data(G_OBJECT(widget), "cselector"));

        g_object_set_data(G_OBJECT(widget), "updating_color", reinterpret_cast<void*>(1));
        csel->setColorAlpha(stop->getColor(), stop->getOpacity());
        g_object_set_data(G_OBJECT(widget), "updating_color", reinterpret_cast<void*>(0));

        /* Fill preview */
        auto w = static_cast<Inkscape::UI::Widget::GradientImage *>(g_object_get_data(G_OBJECT(widget), "preview"));
        w->set_gradient(gradient);

        update_stop_list(GTK_WIDGET(widget), gradient, nullptr);

        // Once the user edits a gradient, it stops being auto-collectable
        if (gradient->getRepr()->attribute("inkscape:collect")) {
            SPDocument *document = gradient->document;
            DocumentUndo::ScopedInsensitive _no_undo(document);
            gradient->removeAttribute("inkscape:collect");
        }
    } else { // no gradient, disable everything
        gtk_widget_set_sensitive(widget, FALSE);
    }

    blocked = FALSE;
}

static void sp_gradient_vector_gradient_release(SPObject */*object*/, GtkWidget *widget)
{
    sp_gradient_vector_widget_load_gradient(widget, nullptr);
}

static void sp_gradient_vector_gradient_modified(SPObject *object, guint /*flags*/, GtkWidget *widget)
{
    SPGradient *gradient=SP_GRADIENT(object);
    if (!blocked) {
        blocked = TRUE;
        sp_gradient_vector_widget_load_gradient(widget, gradient);
        blocked = FALSE;
    }
}

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
