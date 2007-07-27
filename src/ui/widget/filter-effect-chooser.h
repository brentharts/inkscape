#ifndef __FILTER_EFFECT_CHOOSER_H__
#define __FILTER_EFFECT_CHOOSER_H__

/*
 * Filter effect selection selection widget
 *
 * Author:
 *   Nicholas Bishop <nicholasbishop@gmail.com>
 *
 * Copyright (C) 2007 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gtkmm/box.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treeview.h>

#include "combo-enums.h"
#include "filter-enums.h"
#include "labelled.h"
#include "spin-slider.h"
#include "sp-filter.h"

namespace Inkscape {
namespace UI {
namespace Widget {

class FilterEffectChooser
{
public:
    virtual ~FilterEffectChooser();

    virtual Glib::SignalProxy0<void> signal_selection_changed() = 0;
    virtual SPFilter* get_selected_filter() = 0;
    virtual void select_filter(const SPFilter*) = 0;
protected:
    FilterEffectChooser();

    class Columns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        Columns()
        {
            add(filter);
            add(id);
            add(sel);
        }

        Gtk::TreeModelColumn<SPFilter*> filter;
        Gtk::TreeModelColumn<Glib::ustring> id;
        Gtk::TreeModelColumn<int> sel;
    };

    virtual void update_filters();

    Glib::RefPtr<Gtk::ListStore> _model;
    Columns _columns;
private:
    static void on_activate_desktop(Inkscape::Application*, SPDesktop*, FilterEffectChooser*);
    void on_document_replaced(SPDesktop*, SPDocument*);

    sigc::connection _doc_replaced;
    sigc::connection _resource_changed;

    Gtk::TreeView::Column _filter_column;
};

/* Allows basic control over feBlend and feGaussianBlur effects,
   with an option to use the full filter effect controls. */
class SimpleFilterModifier : public Gtk::VBox, public FilterEffectChooser
{
public:
    SimpleFilterModifier();

    virtual Glib::SignalProxy0<void> signal_selection_changed();
    virtual SPFilter* get_selected_filter();
    virtual void select_filter(const SPFilter*);

    sigc::signal<void>& signal_blend_blur_changed();

    const Glib::ustring get_blend_mode();
    // Uses blend mode enum values, or -1 for a complex filter
    void set_blend_mode(const int);

    double get_blur_value() const;
    void set_blur_value(const double);
    void set_blur_sensitive(const bool);
protected:
    virtual void update_filters();
private:
    void show_filter_dialog();
    void blend_mode_changed();

    Gtk::HBox _hb_blend;
    Gtk::VBox _vb_blur;
    Gtk::HBox _hb_filter, _hb_filter_sub;
    Gtk::Label _lb_blend, _lb_blur, _lb_filter;
    ComboBoxEnum<NR::FilterBlendMode> _blend;
    SpinSlider _blur;
    Gtk::ComboBox _filter;
    Gtk::Button _edit_filters;

    sigc::signal<void> _signal_blend_blur_changed;
};

}
}
}

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
