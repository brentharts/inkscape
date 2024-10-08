// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Johan Engelen <j.b.c.engelen@utwente.nl>
 *   Carl Hetherington <inkscape@carlh.net>
 *   Derek P. Moore <derekm@hackunix.org>
 *   Bryce Harrington <bryce@bryceharrington.org>
 *
 * Copyright (C) 2007 Authors
 * Copyright (C) 2004 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_POINT_H
#define INKSCAPE_UI_WIDGET_POINT_H

#include <2geom/point.h>

#include "ui/widget/labelled.h"
#include "ui/widget/scalar.h"

namespace Gtk {
class Adjustment;
} // namespace Gtk

namespace Inkscape::UI::Widget {

/**
 * A labelled text box, with spin buttons and optional icon, for
 * entering arbitrary coordinate values.
 */
class Point : public Labelled
{
public:
    /**
     * Construct a Point Widget.
     *
     * @param label     Label, as per the Labelled base class.
     * @param tooltip   Tooltip, as per the Labelled base class.
     * @param icon      Icon name, placed before the label (defaults to empty).
     * @param mnemonic  Mnemonic toggle; if true, an underscore (_) in the label
     *                  indicates the next character should be used for the
     *                  mnemonic accelerator key (defaults to false).
     */
    Point(Glib::ustring const &label,
          Glib::ustring const &tooltip,
          Glib::ustring const &icon = {},
          bool mnemonic = true);

    /**
     * Construct a Point Widget.
     *
     * @param label     Label, as per the Labelled base class.
     * @param tooltip   Tooltip, as per the Labelled base class.
     * @param digits    Number of decimal digits to display.
     * @param icon      Icon name, placed before the label (defaults to empty).
     * @param mnemonic  Mnemonic toggle; if true, an underscore (_) in the label
     *                  indicates the next character should be used for the
     *                  mnemonic accelerator key (defaults to false).
     */
    Point(Glib::ustring const &label,
          Glib::ustring const &tooltip,
          unsigned digits,
          Glib::ustring const &icon = {},
          bool mnemonic = true);

    /**
     * Construct a Point Widget.
     *
     * @param label     Label, as per the Labelled base class.
     * @param tooltip   Tooltip, as per the Labelled base class.
     * @param adjust    Adjustment to use for the SpinButton.
     * @param digits    Number of decimal digits to display (defaults to 0).
     * @param icon      Icon name, placed before the label (defaults to empty).
     * @param mnemonic  Mnemonic toggle; if true, an underscore (_) in the label
     *                  indicates the next character should be used for the
     *                  mnemonic accelerator key (defaults to true).
     */
    Point(Glib::ustring const &label,
          Glib::ustring const &tooltip,
	  Glib::RefPtr<Gtk::Adjustment> const &adjust,
          unsigned digits = 0,
          Glib::ustring const &icon = {},
          bool mnemonic = true);

    /**
     * Fetches the precision of the spin button.
     */
    unsigned getDigits() const;

    /**
     * Gets the current step increment used by the spin button.
     */
    double  getStep() const;

    /**
     * Gets the current page increment used by the spin button.
     */
    double  getPage() const;

    /**
     * Gets the minimum range value allowed for the spin button.
     */
    double  getRangeMin() const;

    /**
     * Gets the maximum range value allowed for the spin button.
     */
    double  getRangeMax() const;

    bool    getSnapToTicks() const;

    /**
     * Get the value in the spin_button.
     */
    double  getXValue() const;

    double  getYValue() const;

    Geom::Point getValue() const;

    /**
     * Get the value spin_button represented as an integer.
     */
    int     getXValueAsInt() const;

    int     getYValueAsInt() const;

    /**
     * Sets the precision to be displayed by the spin button.
     */
    void    setDigits(unsigned digits);

    /**
     * Sets the step and page increments for the spin button.
     */
    void    setIncrements(double step, double page);

    /**
     * Sets the minimum and maximum range allowed for the spin button.
     */
    void    setRange(double min, double max);

    /**
     * Sets the value of the spin button.
     */
    void    setValue(Geom::Point const & p);

    /**
     * Manually forces an update of the spin button.
     */
    void    update();

    /**
     * Signal raised when the spin button's value changes.
     */
    Glib::SignalProxy<void()> signal_x_value_changed();
    Glib::SignalProxy<void()> signal_y_value_changed();

    /**
     * Check 'setProgrammatically' of both scalar widgets.   False if value is changed by user by clicking the widget.
     * true if the value was set by setValue, not changed by the user;
     * if a callback checks it, it must reset it back to false.
     */
    bool setProgrammatically();
    void clearProgrammatically();

protected:
    Scalar xwidget;
    Scalar ywidget;

private:
    void construct_impl();
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_POINT_H

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
