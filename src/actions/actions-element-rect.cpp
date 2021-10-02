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

// Set attribute. If visible is "true", take into account the current transform (value in document units).
void
set_attribute_rect(const Glib::VariantBase& value, InkscapeApplication *app, bool visible, Glib::ustring& attribute)
{
    Glib::Variant<double> d = Glib::VariantBase::cast_dynamic<Glib::Variant<double> >(value);
    double dval = d.get();
    auto selection = app->get_active_selection();
    bool modmade = false;

    for (auto item : selection->items()) {
        auto rect = dynamic_cast<SPRect *>(item);
        if (rect) {
            if (dval != 0.0) {
                auto d = dval;
                // Find "stretch"
                if (visible) {
                    auto affine = rect->i2doc_affine();
                    if (!affine.isSingular()) {
                        if (attribute == "x" || attribute == "width" || attribute == "rx") {
                            d /= affine.expansionX();
                        } else {
                            d /= affine.expansionY();
                        }
                    }
                }
                rect->setAttribute(attribute.c_str(), std::to_string(d));
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

// Should really be ObjectSet level actions.
std::vector<std::vector<Glib::ustring>> raw_data_element_rect =
{
    // clang-format off
    {"app.element-rect-x",              N_("X"),              "Rectangle",  N_("Set rectangle x position")                                 },
    {"app.element-rect-y",              N_("Y"),              "Rectangle",  N_("Set rectangle y position")                                 },
    {"app.element-rect-width",          N_("Width"),          "Rectangle",  N_("Set rectangle width")                                      },
    {"app.element-rect-height",         N_("Height"),         "Rectangle",  N_("Set rectangle height")                                     },
    {"app.element-rect-rx",             N_("Rx"),             "Rectangle",  N_("Set rectangle horizontal corner radius")                   },
    {"app.element-rect-ry",             N_("Ry"),             "Rectangle",  N_("Set rectangle vertical corner radius")                     },
    {"app.element-rect-visible-x",      N_("Visible X"),      "Rectangle",  N_("Set rectangle x position in document units")               },
    {"app.element-rect-visible-y",      N_("Visible Y"),      "Rectangle",  N_("Set rectangle y position in document units")               },
    {"app.element-rect-visible-width",  N_("Visible Width"),  "Rectangle",  N_("Set rectangle width in document units")                    },
    {"app.element-rect-visible-height", N_("Visible Height"), "Rectangle",  N_("Set rectangle height in document units")                   },
    {"app.element-rect-visible-rx",     N_("Visible Rx"),     "Rectangle",  N_("Set rectangle horizontal corner radius in document units") },
    {"app.element-rect-visible-ry",     N_("Visible Ry"),     "Rectangle",  N_("Set rectangle vertical corner radius in document units")   },
    {"app.element-rect-reset-corners",  N_("Reset Corners"),  "Rectangle",  N_("Remove rounded corners")                                   }
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
    gapp->add_action_with_parameter( "element-rect-x",              Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_rect), app, false, "x"     ));
    gapp->add_action_with_parameter( "element-rect-y",              Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_rect), app, false, "y"     ));
    gapp->add_action_with_parameter( "element-rect-width",          Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_rect), app, false, "width" ));
    gapp->add_action_with_parameter( "element-rect-height",         Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_rect), app, false, "height"));
    gapp->add_action_with_parameter( "element-rect-rx",             Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_rect), app, false, "rx"    ));
    gapp->add_action_with_parameter( "element-rect-ry",             Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_rect), app, false, "ry"    ));
    gapp->add_action_with_parameter( "element-rect-visible-x",      Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_rect), app, true,  "x"     ));
    gapp->add_action_with_parameter( "element-rect-visible-y",      Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_rect), app, true,  "y"     ));
    gapp->add_action_with_parameter( "element-rect-visible-width",  Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_rect), app, true,  "width" ));
    gapp->add_action_with_parameter( "element-rect-visible-height", Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_rect), app, true,  "height"));
    gapp->add_action_with_parameter( "element-rect-visible-rx",     Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_rect), app, true,  "rx"    ));
    gapp->add_action_with_parameter( "element-rect-visible-ry",     Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_rect), app, true,  "ry"    ));
    gapp->add_action(                "element-rect-reset-corners",          sigc::bind<InkscapeApplication*                     >(sigc::ptr_fun(&reset_corners),      app                 ));
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
