// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for use with <circle>.
 *
 * Copyright (C) 2021 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "actions-element-circle.h"

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
set_attribute_circle(const Glib::VariantBase& value, InkscapeApplication *app, bool visible, Glib::ustring& attribute)
{
    Glib::Variant<double> d = Glib::VariantBase::cast_dynamic<Glib::Variant<double> >(value);
    double dval = d.get();
    auto selection = app->get_active_selection();
    bool modmade = false;

    for (auto item : selection->items()) {
        auto ellipse = dynamic_cast<SPGenericEllipse *>(item);
        if (ellipse && ellipse->type == SP_GENERIC_ELLIPSE_CIRCLE) {
            if (dval != 0.0) {
                // Find "stretch"
                if (visible) {
                    auto affine = ellipse->i2doc_affine();
                    if (!affine.isSingular()) {
                        if (attribute == "cx") {
                            dval /= affine.expansionX();
                        } else if (attribute == "cy") {
                            dval /= affine.expansionY();
                        } else { // radius
                            dval /= affine.expansion().length();
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
        Inkscape::DocumentUndo::done(app->get_active_document(), _("Change circle"), INKSCAPE_ICON("draw-ellipse"));
    }
}

// Should really be ObjectSet level actions.
std::vector<std::vector<Glib::ustring>> raw_data_element_circle =
{
    // clang-format off
    {"app.element-circle-cx",             N_("Cx"),          "Circle",  N_("Set circle center x position")                  },
    {"app.element-circle-cy",             N_("Cy"),          "Circle",  N_("Set circle center y position")                  },
    {"app.element-circle-radius",         N_("R"),           "Circle",  N_("Set circle radius")                             },
    {"app.element-circle-visible-cx",     N_("Visible Cx"),  "Circle",  N_("Set circle center x position in document units")},
    {"app.element-circle-visible-cy",     N_("Visible Cy"),  "Circle",  N_("Set circle center y position in document units")},
    {"app.element-circle-visible-radius", N_("Visible R"),   "Circle",  N_("Set circle radius in document units")           }
    // clang-format on
};

void
add_actions_element_circle(InkscapeApplication* app)
{
    Glib::VariantType Bool(  Glib::VARIANT_TYPE_BOOL);
    Glib::VariantType Int(   Glib::VARIANT_TYPE_INT32);
    Glib::VariantType Double(Glib::VARIANT_TYPE_DOUBLE);
    Glib::VariantType String(Glib::VARIANT_TYPE_STRING);

    auto *gapp = app->gio_app();

    // clang-format off
    gapp->add_action_with_parameter( "element-circle-cx",         Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_circle), app, false, "cx" ));
    gapp->add_action_with_parameter( "element-circle-cy",         Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_circle), app, false, "cy" ));
    gapp->add_action_with_parameter( "element-circle-r",          Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_circle), app, false, "r"  ));
    gapp->add_action_with_parameter( "element-circle-visible-cx", Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_circle), app, true,  "cx" ));
    gapp->add_action_with_parameter( "element-circle-visible-cy", Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_circle), app, true,  "cy" ));
    gapp->add_action_with_parameter( "element-circle-visible-r",  Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_circle), app, true,  "r"  ));
    // clang-format on

    app->get_action_extra_data().add_data(raw_data_element_circle);
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
