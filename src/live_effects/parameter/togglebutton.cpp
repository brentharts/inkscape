// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 * Copyright (C) Jabiertxo Arraiza Cenoz 2014 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/widget/registered-widget.h"
#include <glibmm/i18n.h>

#include <utility>

#include "helper-fns.h"
#include "inkscape.h"
#include "live_effects/effect.h"
#include "live_effects/parameter/togglebutton.h"
#include "selection.h"
#include "svg/stringstream.h"
#include "svg/svg.h"
#include "ui/icon-loader.h"
#include "verbs.h"

namespace Inkscape {

namespace LivePathEffect {

ToggleButtonParam::ToggleButtonParam(const Glib::ustring& label, const Glib::ustring& tip,
                      const Glib::ustring& key, Inkscape::UI::Widget::Registry* wr,
                      Effect* effect, bool default_value, Glib::ustring  inactive_label,
                      char const * _icon_active, char const * _icon_inactive, 
                      GtkIconSize _icon_size)
    : Parameter(label, tip, key, wr, effect), value(default_value), defvalue(default_value),
      inactive_label(std::move(inactive_label)), _icon_active(_icon_active), _icon_inactive(_icon_inactive), _icon_size(_icon_size)
{
    checkwdg = nullptr;
}

ToggleButtonParam::~ToggleButtonParam()
{
    if (_toggled_connection.connected()) {
        _toggled_connection.disconnect();
    }
}

void
ToggleButtonParam::param_set_default()
{
    param_setValue(defvalue);
}

bool
ToggleButtonParam::param_readSVGValue(const gchar * strvalue)
{
    param_setValue(helperfns_read_bool(strvalue, defvalue));
    return true; // not correct: if value is unacceptable, should return false!
}

gchar *
ToggleButtonParam::param_getSVGValue() const
{
    return g_strdup(value ? "true" : "false");
}

gchar *
ToggleButtonParam::param_getDefaultSVGValue() const
{
    return g_strdup(defvalue ? "true" : "false");
}

void 
ToggleButtonParam::param_update_default(bool default_value)
{
    defvalue = default_value;
}

void 
ToggleButtonParam::param_update_default(const gchar * default_value)
{
    param_update_default(helperfns_read_bool(default_value, defvalue));
}

Gtk::Widget *
ToggleButtonParam::param_newWidget()
{
    if (_toggled_connection.connected()) {
        _toggled_connection.disconnect();
    }

   checkwdg = Gtk::manage(
        new Inkscape::UI::Widget::RegisteredToggleButton(param_label,
                                                         param_tooltip,
                                                         param_key,
                                                         *param_wr,
                                                         false,
                                                         param_effect->getRepr(),
                                                         param_effect->getSPDoc()));
    auto box_button = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(box_button), false);
    GtkWidget * label_button = gtk_label_new ("");
    if (!param_label.empty()) {
        if(value || inactive_label.empty()){
            gtk_label_set_text(GTK_LABEL(label_button), param_label.c_str());
        }else{
            gtk_label_set_text(GTK_LABEL(label_button), inactive_label.c_str());
        }
    }
    gtk_widget_show(label_button);
    if (_icon_active) {
        if(!_icon_inactive){
            _icon_inactive = _icon_active;
        }
        gtk_widget_show(box_button);
        GtkWidget *icon_button = nullptr;
        if (!value) {
            icon_button = sp_get_icon_image(_icon_inactive, _icon_size);
        } else {
            icon_button = sp_get_icon_image(_icon_active, _icon_size);
        }
        gtk_widget_show(icon_button);
        gtk_box_pack_start (GTK_BOX(box_button), icon_button, false, false, 1);
        if (!param_label.empty()) {
            gtk_box_pack_start (GTK_BOX(box_button), label_button, false, false, 1);
        }
    }else{
        gtk_box_pack_start (GTK_BOX(box_button), label_button, false, false, 1);
    }

    checkwdg->add(*Gtk::manage(Glib::wrap(box_button)));
    checkwdg->setActive(value);
    checkwdg->setProgrammatically = false;
    checkwdg->set_undo_parameters(SP_VERB_DIALOG_LIVE_PATH_EFFECT, _("Change togglebutton parameter"));

    _toggled_connection = checkwdg->signal_toggled().connect(sigc::mem_fun(*this, &ToggleButtonParam::toggled));
    return checkwdg;
}

void
ToggleButtonParam::refresh_button()
{
    if (!_toggled_connection.connected()) {
        return;
    }

    if(!checkwdg){
        return;
    }
    Gtk::Widget * box_button = checkwdg->get_child();
    if(!box_button){
        return;
    }
    std::vector<Gtk::Widget*> children = Glib::wrap(GTK_CONTAINER(box_button))->get_children();
    if (!param_label.empty()) {
        Gtk::Label *lab = dynamic_cast<Gtk::Label*>(children[children.size()-1]);
        if (!lab) return;
        if(value || inactive_label.empty()){
            lab->set_text(param_label.c_str());
        }else{
            lab->set_text(inactive_label.c_str());
        }
    }
    if (_icon_active) {
        GdkPixbuf * icon_pixbuf = nullptr;
        Gtk::Widget *im = dynamic_cast<Gtk::Image *>(children[0]);
        if (!im) return;
        if (!value) {
            im = Glib::wrap(sp_get_icon_image(_icon_inactive, _icon_size));
        } else {
            im = Glib::wrap(sp_get_icon_image(_icon_active, _icon_size));
        }
    }
}

void
ToggleButtonParam::param_setValue(bool newvalue)
{
    if (value != newvalue) {
        param_effect->upd_params = true;
    }
    value = newvalue;
    refresh_button();
}

void
ToggleButtonParam::toggled() {
    if (SP_ACTIVE_DESKTOP) {
        Inkscape::Selection *selection = SP_ACTIVE_DESKTOP->getSelection();
        selection->emitModified();
    }
    _signal_toggled.emit();
}

} /* namespace LivePathEffect */

} /* namespace Inkscape */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
