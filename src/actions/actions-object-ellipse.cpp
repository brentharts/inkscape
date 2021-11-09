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
#include "inkscape-window.h"

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

// From command line.
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
            ellipse->setAttribute("sodipodi:open", (open ? "true" : nullptr)); // Deprecated.
            ellipse->setAttribute("sodipodi:arc-type", type);
            modmade = true;
        }
    }

    if (modmade) {
        Inkscape::DocumentUndo::done(app->get_active_document(), _("Changed Arc type"), INKSCAPE_ICON("draw-ellipse"));
    }

    // No need to update action state as we don't use it in GUI.
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

// From toolbar.
void
set_arc_type_win(Glib::ustring type, InkscapeWindow* win)
{
    if ( !(type == "slice" || type == "arc" || type == "chord")) {
        std::cerr << "set_arc_type_win: invalid type: " << type << std::endl;
        return;
    }

    bool open = (type != "slice");

    auto selection = win->get_desktop()->selection;
    if (!selection) {
        std::cerr << "set_arc_type_win: no selection!" << std::endl;
        return;
    }

    bool modmade = false;

    for (auto item : selection->items()) {
        auto ellipse = dynamic_cast<SPGenericEllipse *>(item);
        if (ellipse && ellipse->type == SP_GENERIC_ELLIPSE_ARC) {
            ellipse->setAttribute("sodipodi:open", (open ? "true" : nullptr)); // Deprecated.
            ellipse->setAttribute("sodipodi:arc-type", type);
            modmade = true;
        }
    }

    if (modmade) {
        Inkscape::DocumentUndo::done(win->get_document(), _("Changed Arc type"), INKSCAPE_ICON("draw-ellipse"));
    }

    // Update action state!
    auto action = win->lookup_action("object-ellipse-arc-type");
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

// Should really be ObjectSet level actions.
std::vector<std::vector<Glib::ustring>> raw_data_object_ellipse =
{
    // clang-format off
    {"app.object-ellipse-cx",                 N_("Cx"),          "Arc",  N_("Set arc center horizontal position.")                   },
    {"app.object-ellipse-cy",                 N_("Cy"),          "Arc",  N_("Set arc center horizontal position.")                   },
    {"app.object-ellipse-rx",                 N_("Rx"),          "Arc",  N_("Set arc horizontal radius.")                            },
    {"app.object-ellipse-ry",                 N_("Ry"),          "Arc",  N_("Set arc vertical radius.")                              },
    {"app.object-ellipse-visible-cx",         N_("Visible Cx"),  "Arc",  N_("Set arc center horizontal position in document units.") },
    {"app.object-ellipse-visible-cy",         N_("Visible Cy"),  "Arc",  N_("Set arc center horizontal position in document units.") },
    {"app.object-ellipse-visible-rx",         N_("Visible Rx"),  "Arc",  N_("Set arc horizontal radius in document units.")          },
    {"app.object-ellipse-visible-ry",         N_("Visible Ry"),  "Arc",  N_("Set arc vertical radius in document units.")            },
    {"app.object-ellipse-start",              N_("Arc start"),   "Arc",  N_("Set arc start angle (degrees).")                        },
    {"app.object-ellipse-end",                N_("Arc end"),     "Arc",  N_("Set arc end angle (degress).")                          },
    {"app.object-ellipse-arc-type('slice')",  N_("Slice"),       "Arc",  N_("Set arc type to 'Slice'")                               },
    {"app.object-ellipse-arc-type('arc')",    N_("Arc"),         "Arc",  N_("Set arc type to 'Arc'")                                 },
    {"app.object-ellipse-arc-type('chord')",  N_("Chord"),       "Arc",  N_("Set arc type to 'Chord'")                               },
    {"app.object-ellipse-make-whole",         N_("Make whole"),  "Arc",  N_("Convert to full ellipse or circle.")                    },
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
    // clang-format on

    app->get_action_extra_data().add_data(raw_data_object_ellipse);
}

// Stateful actions for toolbar. (App action needed for command line.)
void
add_actions_object_ellipse(InkscapeWindow *win)
{

    // clang-format off
    win->add_action_radio_string(    "object-ellipse-arc-type",           sigc::bind<InkscapeWindow*>(                          sigc::ptr_fun(&set_arc_type_win),  win),       "slice"          );
    // clang-format on

    auto app = InkscapeApplication::instance();
    if (!app) {
        std::cerr << "add_actions_canvas_transform: no app!" << std::endl;
        return;
    }
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
