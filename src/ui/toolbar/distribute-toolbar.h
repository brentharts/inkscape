#pragma once

// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Toolbar for the On-canvas alignment and distribution tool.
 * Authors:
 *   Kavya Jaiswal
 *
 * Copyright (C) 2021 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm.h>

#include <utility>

#include "toolbar.h"

class SPDesktop;

namespace Inkscape {
class Selection;

namespace UI {

namespace Widget {
class UnitTracker;
}

namespace Toolbar {

class DistributeToolbar;

// A method that belongs to DistributeToolbar that returns void and accepts noting.
typedef void (DistributeToolbar::*DistributeToolbarVoidMethod)();

struct ButtonDescriptor
{
    std::string label;
    std::string tooltip_text;
    std::string icon_name;
    DistributeToolbarVoidMethod handler;
};

enum DistributeToolMode {
    JUST_SELECT = 0,
    ON_CANVAS_ALIGN = 1,
    ON_CANVAS_DISTRIBUTE = 2,
};

class DistributeToolbar : public Toolbar {
    using parent_type = Toolbar;

private:
    std::vector<Gtk::RadioToolButton *> _mode_buttons;
    std::vector<DistributeToolbarVoidMethod> _mode_handlers;
    DistributeToolMode _current_mode;

//  Mode related methods {
    void mode_buttons_init();
    void mode_buttons_init_create_buttons(const std::vector<ButtonDescriptor>& descriptors);
    void mode_buttons_init_set_active_button();
    void mode_buttons_init_add_buttons();

    void mode_changed(int mode);

    // handlers that gets called when mode is changed:
    void set_mode_just_select();
    void set_mode_align();
    void set_mode_distribute();
//  }

    void oncanvas_buttons_init();
    void oncanvas_buttons_init_actions();
    void oncanvas_buttons_init_actions_add_buttons(const std::vector<ButtonDescriptor>& descriptors);
    void oncanvas_buttons_init_verbs();
    void perform_horizontal_distribution();
    void perform_vertical_distribution();

    void add_separator();

protected:
    DistributeToolbar(SPDesktop *desktop);

public:
    static GtkWidget * create(SPDesktop *desktop);
    DistributeToolMode get_mode() const;
};

}
}
}
