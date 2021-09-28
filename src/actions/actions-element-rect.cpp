// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for use with <rect>.
 *
 * Copyright (C) 2021 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "actions-element-rect.h"

#include <iostream>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include "document-undo.h"
#include "inkscape-application.h"

#include "selection.h"            // Selection
#include "object/sp-rect.h"
#include "ui/icon-names.h"        // Tag Inkscape icons.

void
set_attribute(const Glib::VariantBase& value, InkscapeApplication *app, Glib::ustring& attribute)
{
    Glib::Variant<double> d = Glib::VariantBase::cast_dynamic<Glib::Variant<double> >(value);
    double dval = d.get();
    auto selection = app->get_active_selection();
    bool modmade = false;

    for (auto item : selection->items()) {
        auto rect = dynamic_cast<SPRect *>(item);
        if (rect) {
            if (dval != 0.0) {
                if (attribute == "width") {
                    rect->setVisibleWidth(dval);
                } else if (attribute == "height") {
                    rect->setVisibleHeight(dval);
                } else if (attribute == "rx") {
                    rect->setVisibleRx(dval);
                } else if (attribute == "ry") {
                    rect->setVisibleRy(dval);
                } else {
                    std::cerr << "Rectangle action set_attribute: invalid attribute" << std::endl;
                }
            } else {
                rect->removeAttribute(attribute.c_str());
            }
            modmade = true;
        }
    }

    if (modmade) {
        Inkscape::DocumentUndo::done(app->get_active_document(), _("Change rectangle"), INKSCAPE_ICON("draw-rectangle"));
    }
}

void
reset_corners(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();
    bool modmade = false;

    for (auto item : selection->items()) {
        auto rect = dynamic_cast<SPRect *>(item);
        if (rect) {
            rect->removeAttribute("rx");
            rect->removeAttribute("ry");
            modmade = true;
        }
    }

    if (modmade) {
        Inkscape::DocumentUndo::done(app->get_active_document(), 0, _("Change rectangle"));
    }
}

// SHOULD REALLY BE DOC LEVEL ACTIONS
std::vector<std::vector<Glib::ustring>> raw_data_element_rect =
{
    // clang-format off
    {"app.element-rect-width",          N_("Width"),    "Rectangle",  N_("Set rectangle width")                    },
    {"app.element-rect-height",         N_("Height"),   "Rectangle",  N_("Set rectangle height")                   },
    {"app.element-rect-rx",             N_("Rx"),       "Rectangle",  N_("Set rectangle horizontal corner radius") },
    {"app.element-rect-ry",             N_("Ry"),       "Rectangle",  N_("Set rectangle vertical corner radius")   },
    {"app.element-rect-reset-corners",  N_("Corner"),   "Rectangle",  N_("Remove rounded corners")                 }
    // clang-format on
};

void
add_actions_element_rect(InkscapeApplication* app)
{
    Glib::VariantType Bool(  Glib::VARIANT_TYPE_BOOL);
    Glib::VariantType Int(   Glib::VARIANT_TYPE_INT32);
    Glib::VariantType Double(Glib::VARIANT_TYPE_DOUBLE);
    Glib::VariantType String(Glib::VARIANT_TYPE_STRING);

    auto *gapp = app->gio_app();

    // clang-format off
    gapp->add_action_with_parameter( "element-rect-width",  Double, sigc::bind<InkscapeApplication*, Glib::ustring>(sigc::ptr_fun(&set_attribute), app, "width" ));
    gapp->add_action_with_parameter( "element-rect-height", Double, sigc::bind<InkscapeApplication*, Glib::ustring>(sigc::ptr_fun(&set_attribute), app, "height"));
    gapp->add_action_with_parameter( "element-rect-rx",     Double, sigc::bind<InkscapeApplication*, Glib::ustring>(sigc::ptr_fun(&set_attribute), app, "rx"    ));
    gapp->add_action_with_parameter( "element-rect-ry",     Double, sigc::bind<InkscapeApplication*, Glib::ustring>(sigc::ptr_fun(&set_attribute), app, "ry"    ));
    gapp->add_action(                "element-rect-reset-corners",  sigc::bind<InkscapeApplication*               >(sigc::ptr_fun(&reset_corners), app          ));
    // clang-format on

    app->get_action_extra_data().add_data(raw_data_element_rect);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
