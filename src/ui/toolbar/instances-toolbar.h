// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Tavmjong Bah
 *
 * Copyright (C) 2023 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_TOOLBAR_INSTANCES_H
#define SEEN_TOOLBAR_INSTANCES_H

#include <gtkmm/box.h>
#include <gtkmm/builder.h>

#include "preferences.h"

namespace Gtk {
class Box;
} // namespace Gtk

class InkscapeWindow;
class SPDesktop;
class SPDesktopWidget;

namespace Inkscape::UI::Toolbar {

class InstancesToolbar final : public Gtk::Box
{
public:
    InstancesToolbar();
protected:
    friend SPDesktopWidget;
    void activate_instance(SPDesktop * desktop);
private:
    void raise_instance(SPDesktop * desktop);
    void set_buttons();
    Inkscape::PrefObserver _observer;
    Glib::RefPtr<Gtk::Builder> builder;
    Gtk::Box &_instances_box;
};

} // namespace Inkscape::UI::Toolbar

#endif /* SEEN_TOOLBAR_INSTANCES_H */

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
