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
 * A SpinButton that can optionally trigger Gio::Actions.
 *
 * Actions are set by the "action-name" property. Actions can be either an integer or double type.
 * 
 * Also can be tied to a Units Menu via the "unit-menu-widget" property, in which case the
 * Gtk::Adjustment value is in terms of the Units Menu unit. Actions always use "user units"
 * (i.e. "px").
 *
 * Custom value for a context menu can be given via the "custom-values" property. The value of the
 * property should be a semi-colon list of entries with each entry consisting of a number or a
 * number:label pair.
 *
 * A "defocus" widget (typically the canvas) can be specified by the "defocus-widget" property. Focus is
 * returned to this widget when the ESC or return key is pressed.
 *
 * This is a rewrite of Inkscape::UI::Widget::SpinButton and Inkscape::UI::Widget::SpinButtonToolitem
 */

#ifndef SPINBUTTON_ACTION_H
#define SPINBUTTON_ACTION_H

#include <giomm/application.h>
#include <gtkmm.h>

// Prevents the widget from stealing events when the containing widget is being scrolled.
#include "scrollprotected.h"

// class SpinButtonAction : public Gtk::SpinButton, public Gtk::Actionable doesn't work
// as builder gets confused as to what is the BaseObjectType

namespace Inkscape::Util {
class Unit;
}

namespace Inkscape {
namespace UI {
namespace Widget {

class ComboBoxUnit;

/** A spinbutton with an action.
 *
 * Action can be either an integer or double type.
 * Set action name via "action-name" property.
 * Can be used without action by using signal handlers.
 * Provides menu for GTK3 Inkscape::UI::Widget::ToolItemMenu class.
 *
 * Supports:
 *   * Mathamatical functions in input.
 *   * Key values: ESC, Return, CTRL-Z.
 *   * Returning focus to canvas (or other specified widget).
 *   * Custom values with labels in context menu.
 *   * Display units via link to Inkscape::UI::Widget::ComboBoxUnit class.
 *
 * GTK4 support is sketched in, may not be fully functional.
 */
class SpinButtonAction : public ScrollProtected<Gtk::SpinButton>
{
    using parent_type = ScrollProtected<Gtk::SpinButton>;

public:

    SpinButtonAction(); // Dummy for declaring type.
    SpinButtonAction(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refUI);
    ~SpinButtonAction() override = default;

    /**  Blocks calling action so only GUI is updated. */
    void set_value_gui(double value);

#if !GTK_CHECK_VERSION(4, 0, 0)
    Gtk::MenuItem* get_menu(); // Used by GTK3 Inkscape::UI::Widget::ToolItemMenu class.
#endif

    Glib::PropertyProxy<Glib::ustring> property_action_name()      { return prop_action_name.get_proxy(); }
    Glib::PropertyProxy<Glib::ustring> property_custom_values()    { return prop_custom_values.get_proxy(); }
    Glib::PropertyProxy<Glib::ustring> property_defocus_widget()   { return prop_defocus_widget.get_proxy(); }
    Glib::PropertyProxy<Glib::ustring> property_unit_menu_widget() { return prop_unit_menu_widget.get_proxy(); }

private:

    void on_realize() override;

    Glib::RefPtr<Gio::Menu> create_menu(); // Popup menu with custom values.
    void defocus();                        // Set focus to specified widget ('defocus' property).

#if GTK_CHECK_VERSION(4, 0, 0)
    int on_input(double& new_value);
    void on_value_changed(); // Calls action if action exists.
    bool on_output();
    void on_button_pressed_event(int n_press, double x, double y, Gtk::Text* text);
    bool on_key_pressed_event(guint keyval, guint keycode, Gdk::ModifierType state); // "pressed" vs GTK3 "press"
    void on_enter();
    void on_changed() override;
#else
    /**
     * This callback function should try to convert the entered text to a number and write it to
     * newvalue. It calls a method to evaluate the (potential) mathematical expression.
     *
     * @retval false No conversion done, continue with default handler.
     * @retval true  Conversion successful, don't call default handler. 
     */
    int on_input(double* new_value) override;
    void on_value_changed() override; // Calls action if action exists.
    void on_populate_popup(Gtk::Menu* menu) override;
    bool on_focus_in_event(GdkEventFocus* event) override;
    bool on_key_press_event(GdkEventKey* event) override;
#endif

    bool frozen = false;
    bool internal = false; // We call action only if this spin button caused change.

    void on_action_value_changed(const Glib::VariantBase& parameter);
    void on_unit_changed();

    std::string type_string;    // Type of action ('d' double, 'i' integer, or empty if no action).
    // Glib::VariantType type;
    double saved_value = 0;

    Glib::RefPtr<Gio::Action> action;
    Glib::RefPtr<Gio::Application> application;
    Gtk::ApplicationWindow* window = nullptr;
    Gtk::Widget* defocus_widget = nullptr;   ///< Widget that should grab focus when the spinbutton defocuses
    ComboBoxUnit* unit_menu_widget = nullptr; ///< Widget with units menu.

    const Inkscape::Util::Unit* unit = nullptr;

    // Properties
    Glib::Property<Glib::ustring> prop_action_name;
#if !GTK_CHECK_VERSION(4, 0, 0)
    Glib::Property<Glib::ustring> prop_menu_label; // Used for proxy menu.
#endif
    Glib::Property<Glib::ustring> prop_custom_values;
    Glib::Property<Glib::ustring> prop_defocus_widget;
    Glib::Property<Glib::ustring> prop_unit_menu_widget;
};


} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif // SPINBUTTON_ACTION_H

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
