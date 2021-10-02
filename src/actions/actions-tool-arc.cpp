// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for use with the Arc tool (or SPGenericEllipse).
 *
 * Copyright (C) 2021 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 * The Arc tool works with the <circle>, <ellipse>, and <path> elements. These elements are handled
 * by the SPGenericEllipse class that will automatically convert between the elements as the "arc"
 * shape is changed.
 *
 * See actions-element-circle.cpp and action-element-ellipse.cpp for addtional actions.
 *
 */

#include "actions-tool-arc.h"

#include <iostream>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include "document-undo.h"
#include "inkscape-application.h"

#include "selection.h"            // Selection
#include "object/sp-ellipse.h"
#include "ui/icon-names.h"        // Tag Inkscape icons.

/* Actions needed:
 *
 * Radius x, radius y
 * Arc start/stop
 * Convert between arc types (stateful)
 * Make Whole
 */

void
set_arc_type(Glib::ustring type, InkscapeApplication* app)
{
    if ( !(type == "slice" || type == "arc" || type == "chord")) {
        std::cerr << "set_arc_type: invalid type: " << type << std::endl;
        return;
    }

    bool open = (type != "slice");

    auto selection = app->get_active_selection();
    bool modmade = false;

    for (auto item : selection->items()) {
        auto ellipse = dynamic_cast<SPGenericEllipse *>(item);
        if (ellipse && ellipse->type == SP_GENERIC_ELLIPSE_ARC) {
            ellipse->setAttribute("sodipodi:open", (open ? "true" : nullptr));
            ellipse->setAttribute("sodipodi:arc-type", type);
            modmade = true;
        }
    }

    if (modmade) {
        Inkscape::DocumentUndo::done(app->get_active_document(), _("Changed Arc type"), INKSCAPE_ICON("draw-ellipse"));
    }

    // Update action state!
    auto *gapp = app->gio_app();

    auto action = gapp->lookup_action("tool-arc-arc-type");
    if (!action) {
        std::cerr << "set_arc_type: action 'tool-arc-arc-type' missing!" << std::endl;
        return;
    }

    auto saction = Glib::RefPtr<Gio::SimpleAction>::cast_dynamic(action);
    if (!saction) {
        std::cerr << "set_arc_type: action 'tool-arc-arc-type' not SimpleAction!" << std::endl;
        return;
    }

    // Update button states.
    saction->set_enabled(false);
    saction->change_state(type);
    saction->set_enabled(true);
}

// // Set attribute. If visible is "true", take into account the current transform (value in document units).
// void
// set_attribute_ellipse(const Glib::VariantBase& value, InkscapeApplication *app, bool visible, Glib::ustring& attribute)
// {
//     Glib::Variant<double> d = Glib::VariantBase::cast_dynamic<Glib::Variant<double> >(value);
//     double dval = d.get();
//     auto selection = app->get_active_selection();
//     bool modmade = false;

//     for (auto item : selection->items()) {
//         auto ellipse = dynamic_cast<SPGenericEllipse *>(item);
//         if (ellipse && ellipse->type == SP_GENERIC_ELLIPSE_ELLIPSE) {
//             if (dval != 0.0) {
//                 // Find "stretch"
//                 if (visible) {
//                     auto affine = ellipse->i2doc_affine();
//                     if (!affine.isSingular()) {
//                         if (attribute == "cx"|| attribute == "rx") {
//                             dval /= affine.expansionX();
//                         } else {
//                             dval /= affine.expansionY();
//                         }
//                     }
//                 }
//                 ellipse->setAttribute(attribute.c_str(), std::to_string(dval));
//             } else {
//                 ellipse->removeAttribute(attribute.c_str());
//             }
//             modmade = true;
//         }
//     }

//     if (modmade) {
//         Inkscape::DocumentUndo::done(app->get_active_document(), _("Change ellipse"), INKSCAPE_ICON("draw-ellipse"));
//     }
// }

// Should really be ObjectSet level actions.
std::vector<std::vector<Glib::ustring>> raw_data_tool_arc =
{
//     // clang-format off
//     {"app.element-ellipse-cx",          N_("Cx"),          "Ellipse",  N_("Set ellipse center horizontal position")                  },
//     {"app.element-ellipse-cy",          N_("Cy"),          "Ellipse",  N_("Set ellipse center vertical position")                    },
//     {"app.element-ellipse-rx",          N_("Rx"),          "Ellipse",  N_("Set ellipse horizontal radius")                           },
//     {"app.element-ellipse-ry",          N_("Ry"),          "Ellipse",  N_("Set ellipse vertical radius")                             },
//     {"app.element-ellipse-visible-cx",  N_("Visible Cx"),  "Ellipse",  N_("Set ellipse center horizontal position in document units")},
//     {"app.element-ellipse-visible-cy",  N_("Visible Cy"),  "Ellipse",  N_("Set ellipse center vertical position in document units")  },
//     {"app.element-ellipse-visible-rx",  N_("Visible Rx"),  "Ellipse",  N_("Set ellipse horizontal radius in document units")         },
//     {"app.element-ellipse-visible-ry",  N_("Visible Ry"),  "Ellipse",  N_("Set ellipse vertical radius in document units")           }
    // clang-format on
};

void
add_actions_tool_arc(InkscapeApplication* app)
{
    Glib::VariantType Bool(  Glib::VARIANT_TYPE_BOOL);
    Glib::VariantType Int(   Glib::VARIANT_TYPE_INT32);
    Glib::VariantType Double(Glib::VARIANT_TYPE_DOUBLE);
    Glib::VariantType String(Glib::VARIANT_TYPE_STRING);

    auto *gapp = app->gio_app();

    // clang-format off
     gapp->add_action_radio_string( "tool-arc-arc-type",   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&set_arc_type),      app),   "slice");
    // gapp->add_action_with_parameter( "element-ellipse-cx",          Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_ellipse), app, false, "cx" ));
    // clang-format on

    app->get_action_extra_data().add_data(raw_data_tool_arc);
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
