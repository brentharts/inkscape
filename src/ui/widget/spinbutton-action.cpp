// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Tavmjong Bah
 *   Johan B. C. Engelen (mathematical input)
 *
 * Copyright (C) 2011 Johan B. C. Engelen
 * Copyright (C) 2021 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

/*
 * Rewrite of Inkscape::UI::Widget::SpinButton and Inkscape::UI::Widget::SpinButtonToolitem
 *
 * See .h file for more details.
 */

#include "spinbutton-action.h"

#include <iostream>
#include <iomanip>

#include <giomm/application.h>
#include <gtkmm.h>

#include "unit-menu.h"
#include "unit-tracker.h"
#include "util/expression-evaluator.h"

#include "combobox-unit.h"

namespace Inkscape {
namespace UI {
namespace Widget {

// Dummy constructor to register type.
SpinButtonAction::SpinButtonAction()
    : ScrollProtected<Gtk::SpinButton>()
    , Glib::ObjectBase("SpinButtonAction")
    , prop_action_name(*this, "action-name")
#if !GTK_CHECK_VERSION(4, 0, 0)
    , prop_menu_label(*this, "menu-label")
#endif
    , prop_custom_values(*this, "custom-values")
    , prop_defocus_widget(*this, "defocus-widget")
    , prop_unit_menu_widget(*this, "unit-menu-widget")
{
}

SpinButtonAction::SpinButtonAction(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refUI)
    : ScrollProtected<Gtk::SpinButton>(cobject)
    , Glib::ObjectBase("SpinButtonAction")
    , prop_action_name(*this, "action-name")
#if !GTK_CHECK_VERSION(4, 0, 0)
    , prop_menu_label(*this, "menu-label")
#endif
    , prop_custom_values(*this, "custom-values")
    , prop_defocus_widget(*this, "defocus-widget")
    , prop_unit_menu_widget(*this, "unit-menu-widget")
{
    // std::cout << "SpinButtonAction::SpinButtonAction(): UI constructor: "
    //           << "  action: " << prop_action_name
    //           << ": " << prop_custom_values
    //           << "  defocus_widget: " << prop_defocus_widget
    //           << "  unit_menu_widget: " << prop_unit_menu_widget
    //           << std::endl;
    application = Gtk::Application::get_default(); // Used to access action.

    if (!application) {
        std::cerr << "SpinButtonAction: no application!" << std::endl;
        return;
    }
}

// ----------- Utility -----------

// Find a widget via name.
Gtk::Widget* find_widget_by_name_recurse(Gtk::Widget* widget, const Glib::ustring& name)
{
    if (!widget) {
        std::cerr << "find_widget_by_name_recurse: No widget!!!" << std::endl;
        return nullptr;
    }

    if (widget && widget->get_name() == name) {
        return widget;
    }

#if GTK_CHECK_VERSION(4, 0, 0)

    for (auto child = widget->get_first_child(); child != nullptr; child = child->get_next_sibling()) {
        auto temp = find_widget_by_name_recurse(child, name);
        if (temp) {
            return temp;
        }
    }

#else

    auto widget_bin = dynamic_cast<Gtk::Bin *>(widget);
    if (widget_bin) {
        auto child = widget_bin->get_child();
        if (child) {
            return find_widget_by_name_recurse(child, name);
        }
        return nullptr;
    }

    auto widget_container = dynamic_cast<Gtk::Container *>(widget);
    if (widget_container) {
        auto children = widget_container->get_children();
        for (auto child : children) {
            auto temp = find_widget_by_name_recurse(child, name);
            if (temp) {
                return temp;
            }
        }
    }

#endif

    return nullptr;
}

// Used in creating menu
Glib::ustring round_to_digits(double value, int digits)
{
    std::stringstream stream;
    stream << std::fixed << std::setprecision(digits) << value;
    return stream.str();
}

// Used in creating menu
Glib::ustring round_to_digits(const Glib::ustring& value, int digits)
{
    return round_to_digits(std::stod(value), digits);
}

// ------------ Class functions ------------

void SpinButtonAction::on_realize()
{
    Gtk::SpinButton::on_realize();

    // Find window we're in.
    window = dynamic_cast<Gtk::ApplicationWindow*>(get_ancestor(GTK_TYPE_APPLICATION_WINDOW));
    if (!window) {
        std::cerr << "SpinButtonAction::on_realize: No application window!" << std::endl;
        return;
    }

    // Find the unit menu widget (if there is one).
    auto widget = find_widget_by_name_recurse(window, prop_unit_menu_widget.get_value());
    if (widget) {
        unit_menu_widget = dynamic_cast<ComboBoxUnit*>(widget);
        if (unit_menu_widget) {
            unit = unit_menu_widget->get_unit();
            unit_menu_widget->signal_changed().connect(sigc::mem_fun(*this, &SpinButtonAction::on_unit_changed));
        } else {
            std::cerr << "SpinButtonAction::on_realize(): Wrong type of widget for Unit Menu!" << std::endl;
        }
    }

    // Get a reference to the action (if one).
    Glib::ustring full_action_name = prop_action_name.get_value();
    if (full_action_name != "") {
        std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("\\.", full_action_name);

        if (tokens.size() != 2) {
            std::cerr << "SpinButtonAction:on_realize: invalid full action name: " << full_action_name << std::endl;
            return;
        }

        Glib::ustring map_name = tokens[0];
        Glib::ustring action_name = tokens[1];
        if (map_name == "app") {
            if (!application->has_action(action_name)) {
                std::cerr << "SpinButtonAction:on_realize: no app action with name: " << full_action_name << std::endl;
                return;
            }

            action = application->lookup_action(action_name);

        } else if (map_name == "win") {
            if (!window->has_action(action_name)) {
                std::cerr << "SpinButtonAction:on_realize: no win action with name: " << full_action_name << std::endl;
                return;
            }
 
            action = window->lookup_action(action_name);

        } else {
            std::cerr << "SpinButtonAction:on_realize: invalid map name: " << map_name << std::endl;
        }

        if (!action) {
            // Should never happen!
            std::cerr << "SpinButtonAction:on_realize: no action with name: " << full_action_name << std::endl;
        }

        // Find action type (should be double or integer).
        // Doesn't seem to be a way to test this using the C++ binding without Glib-CRITICAL errors.
        const  GVariantType* gtype = g_action_get_parameter_type(action->gobj());
        if (!gtype) {
            std::cerr << "SpinButtonAction:on_realize: action without type: " << std::endl;
            return;
        }
        type_string = action->get_parameter_type().get_string();
    }

#if GTK_CHECK_VERSION(4, 0, 0)
    // In GTK3 we override functions, in GTK4 default functions don't exist.
    signal_input().connect(        sigc::mem_fun(*this, &SpinButtonAction::on_input), true);  // bool -> after
    signal_output().connect(       sigc::mem_fun(*this, &SpinButtonAction::on_output), true);
    signal_value_changed().connect(sigc::mem_fun(*this, &SpinButtonAction::on_value_changed));

    // GTK: Add callback to created and add custom menu when using right button to click on text part of SpinButton.
    // In GTK4 SpinButton has three children: Gtk::Text + 2 x Gtk::Button
    for (auto child = get_first_child(); child != nullptr; child = child->get_next_sibling()) {

        auto text = dynamic_cast<Gtk::Text*>(child);
        if (text) {

            // Need to use controllers in GTK4.
            auto gesture = Gtk::GestureClick::create();
            gesture->set_name("CustomMenu");
            gesture->set_button(3); // 3: Right button
            gesture->signal_pressed().connect(sigc::bind(sigc::mem_fun(*this, &SpinButtonAction::on_button_pressed_event), text));
            text->add_controller(gesture);

            auto key = Gtk::EventControllerKey::create();
            key->signal_key_pressed().connect(sigc::mem_fun(*this, &SpinButtonAction::on_key_pressed_event), false); // Before default handler.
            text->add_controller(key);

            auto focus = Gtk::EventControllerFocus::create();
            focus->signal_enter().connect(sigc::mem_fun(*this, &SpinButtonAction::on_enter));
            text->add_controller(focus);

            break;
        }
    }
#endif
}

// Blocks calling action so only GUI is updated.
void
SpinButtonAction::set_value_gui(double value)
{
    frozen = true;
    set_value(value);
    frozen = false;
}

#if !GTK_CHECK_VERSION(4, 0, 0)
// Create menu for GTK3 ToolItemMenu.
Gtk::MenuItem*
SpinButtonAction::get_menu()
{
  Glib::ustring label = prop_menu_label.get_value();
  if (label == "") {
    label = prop_action_name.get_value();
  }
  if (label == "") {
    label = "SpinButtonAction";
  }

  auto menu_item = Gtk::manage(new Gtk::MenuItem(label));
  auto menu = Gtk::manage(new Gtk::Menu(create_menu()));
  menu_item->set_submenu(*menu);
  return menu_item;
}
#endif

void
SpinButtonAction::defocus()
{
    // Find the defocus widget at save it for future use. Normally this is the canvas.
    if (!defocus_widget && prop_defocus_widget.get_value() != "") {
        defocus_widget = find_widget_by_name_recurse(window, prop_defocus_widget.get_value());
    }

    if (defocus_widget) {
        defocus_widget->grab_focus();
    }
}

Glib::RefPtr<Gio::Menu>
SpinButtonAction::create_menu()
{
    Glib::ustring full_action_name = prop_action_name.get_value();
    if (full_action_name == "") {
        // Menu works via actions; no action, no menu!
        return Glib::RefPtr<Gio::Menu>();
    }

    Glib::ustring custom_values    = prop_custom_values.get_value();

    // Determine number of digits after decimal place.
    int digits = property_digits();
    if (type_string == "d" && digits < 1) {
        digits = 1; // Must have decimal place for "double" action to work!
    }
    if (type_string == "i") {
        digits = 0; // Must not have decimal places for "int" action to work!
    }

    // Create map of labels and values.
    std::map<double, Glib::ustring> entries;

    // Add values derived from adjustment (are these really needed, one can click on +/- buttons to do these)
    // clang-format off
    auto adjustment = get_adjustment();
    auto value          = adjustment->get_value();
    auto lower_value    = adjustment->get_lower();
    auto upper_value    = adjustment->get_upper();
    auto page_increment = adjustment->get_page_increment();
    auto page_up        = value + page_increment;              
    auto page_down      = value - page_increment;              


    // Labels use "GUI" values but actions requires "px" value. (FIXME if used by non-linear properties)
    if (unit) {
        entries[Quantity::convert(value,         unit, "px")] = round_to_digits (value, digits);
        entries[Quantity::convert(lower_value,   unit, "px")] = round_to_digits (lower_value, digits);
        entries[Quantity::convert(upper_value,   unit, "px")] = round_to_digits (upper_value, digits);
        if (page_up < upper_value) {
            entries[Quantity::convert(page_up,   unit, "px")] = round_to_digits (page_up, digits);
        }
        if (page_down > lower_value) {
            entries[Quantity::convert(page_down, unit, "px")] = round_to_digits (page_down, digits);
        }
    } else {
        entries[value        ] = round_to_digits (value, digits);
        entries[lower_value  ] = round_to_digits (lower_value, digits);
        entries[upper_value  ] = round_to_digits (upper_value, digits);
        if (page_up < upper_value) {
            entries[page_up  ] = round_to_digits (page_up, digits);
        }
        if (page_down > lower_value) {
            entries[page_down] = round_to_digits (page_down, digits);
        }
    }
    // clang-format on

    // Add custom values (last so they can override values from adjustment).
    auto tokens = Glib::Regex::split_simple("\\s*;\\s*", custom_values);
    for (auto token: tokens) {
        Glib::ustring label;
        double value = 0;
        std::vector<Glib::ustring> parts = Glib::Regex::split_simple("\\s*:\\s*", token);
        if (parts.size() > 0) {
            value = std::stod(parts[0]);
            if (parts.size() == 1) {
                label = round_to_digits(parts[0], digits);
            }
            if (parts.size() == 2) {
                label = parts[1];
            }
        } else {
            std::cerr << "SpinButtonAction::create_menu(): no action value given!" << std::endl;
        }
        entries[value] = label;
    }

    // Create actual menu
    auto gmenu = Gio::Menu::create();
    for (auto const& [value, label] : entries) {

        // Construct detailed action name
        Glib::ustring action = full_action_name + "(" + round_to_digits (value, digits) + ")";

        auto gitem = Gio::MenuItem::create(label, action);
        gmenu->append_item(gitem);
    }

    return gmenu;
}

#if GTK_CHECK_VERSION(4, 0, 0)

void SpinButtonAction::on_button_pressed_event(int n_press, double x, double y, Gtk::Text *text)
{
    auto gmenu = Gio::Menu::create();
    gmenu->append_section(create_menu());
    text->set_extra_menu(gmenu);
}

#else

void SpinButtonAction::on_populate_popup(Gtk::Menu* menu)
{
    auto separator = Gtk::manage(new Gtk::SeparatorMenuItem());
    menu->append(*separator);

    auto gmenu = create_menu();
    auto extra_menu = Gtk::manage(new Gtk::Menu(gmenu));

    auto item = Gtk::manage(new Gtk::MenuItem("Values"));
    item->set_submenu(*extra_menu);
    menu->append(*item);

    menu->show_all();
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
bool SpinButtonAction::on_key_pressed_event(guint keyval, guint keycode, Gdk::ModifierType state)
{
    switch (keyval) {
        case GDK_KEY_Escape: // Defocus
            set_value(saved_value);
            defocus();
            return true;
            break;
        case GDK_KEY_Return:
            defocus();
            break;
        case GDK_KEY_z:
        case GDK_KEY_Z:
            if ((state & Gdk::ModifierType::CONTROL_MASK) == Gdk::ModifierType::CONTROL_MASK) {
                set_value(saved_value);
                return true;
            }
            break;
        default:
            break;
    }

    return false;
}

#else

bool SpinButtonAction::on_key_press_event(GdkEventKey* event)
{
    switch (event->keyval) {
        case GDK_KEY_Escape: // Defocus
            set_value(saved_value);
            defocus();
            return true;
            break;
        case GDK_KEY_Return:
            defocus();
            break;
        case GDK_KEY_z:
        case GDK_KEY_Z:
            if (event->state & GDK_CONTROL_MASK) {
                set_value(saved_value);
                return true;
            }
            break;
        default:
            break;
    }

    return parent_type::on_key_press_event(event); // We've overridden parent class's function.
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
void SpinButtonAction::on_enter()
{
    saved_value = get_adjustment()->get_value();
}
#else
// Save current value on focus in so we can restore it on cancelling (Ctrl-Z).
bool SpinButtonAction::on_focus_in_event(GdkEventFocus* event)
{
    saved_value = get_adjustment()->get_value();
    return parent_type::on_focus_in_event(event); // We've overridden parent class's function.
}
#endif


#if GTK_CHECK_VERSION(4, 0, 0)
int SpinButtonAction::on_input(double& new_value)
{
    // FIXME for GTK4
    new_value = std::stod(get_text());
    return true;
}
#else
int SpinButtonAction::on_input(double* new_value)
{
    try {
        Inkscape::Util::EvaluatorQuantity result;
        Inkscape::Util::ExpressionEvaluator eval = Inkscape::Util::ExpressionEvaluator(get_text().c_str(), unit);
        result = eval.evaluate();

        // Check if output dimension corresponds to input unit.
        if (unit && result.dimension != (unit->isAbsolute() ? 1 : 0) ) {
            throw Inkscape::Util::EvaluatorException("Input dimensions do not match with parameter dimensions.","");
        }

        *new_value = result.value;
    }
    catch(Inkscape::Util::EvaluatorException &e) {
        g_message ("%s", e.what());

        return false;
    }
    internal = true;
    return true;
}

void SpinButtonAction::on_value_changed()
{
    if (frozen) {
        return;
    }

    if (!action) {
        // We can use class without actions by hooking up signals.
        return;
    }

    if (!internal) {
        // We don't call action in response to external changes.
        return;
    }

    if (type_string == "d") {
        // Double

        auto value = get_value();
        if (unit) {
            // Convert to SVG User units.
            value = Quantity::convert(value, unit, "px");
        }
        Glib::Variant<double> d = Glib::Variant<double>::create(value);
        frozen = true;
        action->activate(d);
        frozen = false;

    } else if (type_string == "i") {
        // Integer

        Glib::Variant<int> i = Glib::Variant<int>::create(get_value());
        frozen = true;
        action->activate(i);
        frozen = false;

    } else {
        std::cerr << "SpinButtonAction:on_input: unhandled type: " << type_string << std::endl;
    }
}
 #endif


// Update the SpinButton if something other than a SpinButton calls the action (e.g. via a normal Button).
// Note: SpinButtons that share the same adjustment update automatically.
void SpinButtonAction::on_action_value_changed(const Glib::VariantBase& parameter)
{
    if (frozen) {
        return;
    }

    if (type_string == "d") {
        // Double

        Glib::Variant<double> d = Glib::VariantBase::cast_dynamic<Glib::Variant<double> >(parameter);
        auto value = d.get();
        if (unit) {
            // Convert to GUI value
            value = Quantity::convert(value, "px", unit);
        }
        frozen = true;
        set_value(value);
        frozen = false;

    } else if (type_string == "i") {

        // Integer
        Glib::Variant<int> i = Glib::VariantBase::cast_dynamic<Glib::Variant<int> >(parameter);
        frozen = true;
        set_value(i.get());
        frozen = false;

    } else {
        std::cerr << "SpinButtonAction:on_action_value_changed: unhandled type: " << type_string << std::endl;
    }
}

void
SpinButtonAction::on_unit_changed()
{
    if (!unit_menu_widget) {
        std::cerr << "SpinbuttonAction::on_unit_changed() called without unit menu widget!" << std::endl;
        return;
    }

    auto new_unit = unit_menu_widget->get_unit();
    auto adjustment = get_adjustment();
    if (adjustment && new_unit != unit) {
        auto value = Quantity::convert(adjustment->get_value(), unit, new_unit);
        unit = new_unit;
        frozen = true;
        set_value(value);
        frozen = false;
    } else {
        std::cerr << "SpinButtonAction::on_unit_changed(): No adjustment!!!" << std::endl;
    }
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

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
