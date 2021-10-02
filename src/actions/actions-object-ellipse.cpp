// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for use with the SPGenericEllipse.
 *
 * Copyright (C) 2021 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 * SPGenericEllipse, used by the Arc tool, works with the <circle>, <ellipse>, and <path>
 * elements. These elements are automatically from one to another as the "arc" shape is changed.
 *
 * See actions-element-circle.cpp and action-element-ellipse.cpp for additional actions.
 *
 */

#include "actions-object-ellipse.h"

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

// Set attribute. If visible is "true", take into account the current transform (value in document units).
void
set_attribute_arc(const Glib::VariantBase& value, InkscapeApplication *app, bool visible, Glib::ustring& attribute)
{
    Glib::Variant<double> d = Glib::VariantBase::cast_dynamic<Glib::Variant<double> >(value);
    double dval = d.get();
    auto selection = app->get_active_selection();
    bool modmade = false;

    for (auto item : selection->items()) {
        auto ellipse = dynamic_cast<SPGenericEllipse *>(item);
        if (ellipse) {
            if (dval != 0.0) {
                auto d = dval;
                if (visible) {
                    // Find "stretch"
                    auto affine = ellipse->i2doc_affine();
                    if (!affine.isSingular()) {
                        if (attribute == "cx"|| attribute == "rx") {
                            d /= affine.expansionX();
                        } else {
                            d /= affine.expansionY();
                        }
                    }
                }
                if (attribute == "sodipodi:start" || attribute == "sodipodi:end") {
                    d *= (M_PI / 180.0);
                }
                ellipse->setAttribute(attribute.c_str(), std::to_string(d));
            } else {
                ellipse->removeAttribute(attribute.c_str());
            }
            ellipse->updateRepr(); // This writes out <ellipse>, <circle>, or <path> correctly.
            modmade = true;
        }
    }

    if (modmade) {
        Inkscape::DocumentUndo::done(app->get_active_document(), _("Change ellipse"), INKSCAPE_ICON("draw-ellipse"));
    }
}

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

    auto action = gapp->lookup_action("object-ellipse-arc-type");
    if (!action) {
        std::cerr << "set_arc_type: action 'object-ellipse-arc-type' missing!" << std::endl;
        return;
    }

    auto saction = Glib::RefPtr<Gio::SimpleAction>::cast_dynamic(action);
    if (!saction) {
        std::cerr << "set_arc_type: action 'object-ellipse-arc-type' not SimpleAction!" << std::endl;
        return;
    }

    // Update button states.
    saction->set_enabled(false);
    saction->change_state(type);
    saction->set_enabled(true);
}

void
set_arc_whole(InkscapeApplication* app)
{
    auto selection = app->get_active_selection();
    bool modmade = false;

    for (auto item : selection->items()) {
        auto ellipse = dynamic_cast<SPGenericEllipse *>(item);
        if (ellipse && ellipse->type == SP_GENERIC_ELLIPSE_ARC) {
            ellipse->setAttribute("sodipodi:start", "0.0");
            ellipse->setAttribute("sodipodi:end",   "0.0");
            ellipse->updateRepr(); // This writes out <ellipse> or <circle> correctly.
            modmade = true;
        }
    }

    if (modmade) {
        Inkscape::DocumentUndo::done(app->get_active_document(), _("Changed Arc type"), INKSCAPE_ICON("draw-ellipse"));
    }
}

// Should really be ObjectSet level actions.
std::vector<std::vector<Glib::ustring>> raw_data_object_ellipse =
{
     // clang-format off
     {"app.object-ellipse-arc-type",              N_("Type"),        "Arc",  N_("Set arc type: slice, arc, or chord")                      },
     {"app.object-ellipse-make-whole",            N_("Make whole"),  "Arc",  N_("Make arc whole (circle or ellipse)")                      },
    // clang-format on
};

void
add_actions_object_ellipse(InkscapeApplication* app)
{
    Glib::VariantType Bool(  Glib::VARIANT_TYPE_BOOL);
    Glib::VariantType Int(   Glib::VARIANT_TYPE_INT32);
    Glib::VariantType Double(Glib::VARIANT_TYPE_DOUBLE);
    Glib::VariantType String(Glib::VARIANT_TYPE_STRING);

    auto *gapp = app->gio_app();

    // clang-format off
    gapp->add_action_with_parameter( "object-ellipse-cx",         Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_arc), app, false, "cx"            ));
    gapp->add_action_with_parameter( "object-ellipse-cy",         Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_arc), app, false, "cy"            ));
    gapp->add_action_with_parameter( "object-ellipse-rx",         Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_arc), app, false, "rx"            ));
    gapp->add_action_with_parameter( "object-ellipse-ry",         Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_arc), app, false, "ry"            ));
    gapp->add_action_with_parameter( "object-ellipse-visible-cx", Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_arc), app, true,  "cx"            ));
    gapp->add_action_with_parameter( "object-ellipse-visible-cy", Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_arc), app, true,  "cy"            ));
    gapp->add_action_with_parameter( "object-ellipse-visible-rx", Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_arc), app, true,  "rx"            ));
    gapp->add_action_with_parameter( "object-ellipse-visible-ry", Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_arc), app, true,  "ry"            ));
    gapp->add_action_with_parameter( "object-ellipse-start",      Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_arc), app, false, "sodipodi:start"));
    gapp->add_action_with_parameter( "object-ellipse-end",        Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_arc), app, false, "sodipodi:end"  ));
    gapp->add_action_radio_string(   "object-ellipse-arc-type",           sigc::bind<InkscapeApplication*>(                     sigc::ptr_fun(&set_arc_type),      app),       "slice"          );
    gapp->add_action(                "object-ellipse-set-whole",          sigc::bind<InkscapeApplication*>(                     sigc::ptr_fun(&set_arc_whole),     app)                         );

    // gapp->add_action_with_parameter( "element-ellipse-cx",          Double, sigc::bind<InkscapeApplication*, bool, Glib::ustring>(sigc::ptr_fun(&set_attribute_ellipse), app, false, "cx" ));
    // clang-format on

    app->get_action_extra_data().add_data(raw_data_object_ellipse);
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
