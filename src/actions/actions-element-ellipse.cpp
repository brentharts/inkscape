// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for use with <ellipse>.
 *
 * Copyright (C) 2021 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "actions-element-ellipse.h"

#include <iostream>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include "document-undo.h"
#include "inkscape-application.h"

#include "selection.h"            // Selection
#include "object/sp-ellipse.h"
#include "ui/icon-names.h"        // Tag Inkscape icons.

// Set attribute. If visible is "true", take into account the current transform (value in document units).
void
set_attribute_ellipse(const Glib::VariantBase& value, InkscapeApplication *app, bool visible, Glib::ustring& attribute)
{
    Glib::Variant<double> d = Glib::VariantBase::cast_dynamic<Glib::Variant<double> >(value);
    double dval = d.get();
    auto selection = app->get_active_selection();
    bool modmade = false;

    for (auto item : selection->items()) {
        auto ellipse = dynamic_cast<SPGenericEllipse *>(item);
        if (ellipse && ellipse->type == SP_GENERIC_ELLIPSE_ELLIPSE) {
            if (dval != 0.0) {
                // Find "stretch"
                if (visible) {
                    auto affine = ellipse->i2doc_affine();
                    if (!affine.isSingular()) {
                        if (attribute == "cx"|| attribute == "rx") {
                            dval /= affine.expansionX();
                        } else {
                            dval /= affine.expansionY();
                        }
                    }
                }
                ellipse->setAttribute(attribute.c_str(), std::to_string(dval));
            } else {
                ellipse->removeAttribute(attribute.c_str());
            }
            modmade = true;
        }
    }

    if (modmade) {
        Inkscape::DocumentUndo::done(app->get_active_document(), _("Change ellipse"), INKSCAPE_ICON("draw-ellipse"));
    }
}

// Should really be ObjectSet level actions.
std::vector<std::vector<Glib::ustring>> raw_data_element_ellipse =
{
    // clang-format off
    {"app.element-ellipse-cx",          N_("Cx"),          "Ellipse",  N_("Set ellipse center horizontal position")                  },
    {"app.element-ellipse-cy",          N_("Cy"),          "Ellipse",  N_("Set ellipse center vertical position")                    },
    {"app.element-ellipse-rx",          N_("Rx"),          "Ellipse",  N_("Set ellipse horizontal radius")                           },
    {"app.element-ellipse-ry",          N_("Ry"),          "Ellipse",  N_("Set ellipse vertical radius")                             },
    {"app.element-ellipse-visible-cx",  N_("Visible Cx"),  "Ellipse",  N_("Set ellipse center horizontal position in document units")},
    {"app.element-ellipse-visible-cy",  N_("Visible Cy"),  "Ellipse",  N_("Set ellipse center vertical position in document units")  },
    {"app.element-ellipse-visible-rx",  N_("Visible Rx"),  "Ellipse",  N_("Set ellipse horizontal radius in document units")         },
    {"app.element-ellipse-visible-ry",  N_("Visible Ry"),  "Ellipse",  N_("Set ellipse vertical radius in document units")           }
    // clang-format on
};

void
add_actions_element_ellipse(InkscapeApplication* app)
{
    Glib::VariantType Bool(  Glib::VARIANT_TYPE_BOOL);
    Glib::VariantType Int(   Glib::VARIANT_TYPE_INT32);
    Glib::VariantType Double(Glib::VARIANT_TYPE_DOUBLE);
    Glib::VariantType String(Glib::VARIANT_TYPE_STRING);

    auto *gapp = app->gio_app();

    // clang-format off
    gapp->add_action_with_parameter( "element-ellipse-cx",          Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_ellipse), app, false, "cx" ));
    gapp->add_action_with_parameter( "element-ellipse-cy",          Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_ellipse), app, false, "cy" ));
    gapp->add_action_with_parameter( "element-ellipse-rx",          Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_ellipse), app, false, "rx" ));
    gapp->add_action_with_parameter( "element-ellipse-ry",          Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_ellipse), app, false, "ry" ));
    gapp->add_action_with_parameter( "element-ellipse-visible-cx",  Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_ellipse), app, true,  "cx" ));
    gapp->add_action_with_parameter( "element-ellipse-visible-cy",  Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_ellipse), app, true,  "cy" ));
    gapp->add_action_with_parameter( "element-ellipse-visible-rx",  Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_ellipse), app, true,  "rx" ));
    gapp->add_action_with_parameter( "element-ellipse-visible-ry",  Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_ellipse), app, true,  "ry" ));
    // clang-format on

    app->get_action_extra_data().add_data(raw_data_element_ellipse);
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
