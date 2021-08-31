// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * On-canvas alignment and distribution tool
 *
 * Authors:
 *   Kavya Jaiswal
 *
 * Copyright (C) 2021 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#pragma once

#include "ui/tools/tool-base.h"

#define SP_SELECT_CONTEXT(obj) (dynamic_cast<Inkscape::UI::Tools::DistributeTool*>((Inkscape::UI::Tools::ToolBase*)obj))
#define SP_IS_SELECT_CONTEXT(obj) (dynamic_cast<const Inkscape::UI::Tools::DistributeTool*>((const Inkscape::UI::Tools::ToolBase*)obj) != NULL)

namespace Inkscape {
class SelTrans;
class SelectionDescriber;
}

namespace Inkscape {
namespace UI {
namespace Tools {

class DistributeTool : public ToolBase {
public:
    DistributeTool();
    ~DistributeTool() override;

    bool dragging;
    bool moved;
    guint button_press_state;

    std::vector<SPItem *> cycling_items;
    std::vector<SPItem *> cycling_items_cmp;
    SPItem *cycling_cur_item;
    bool cycling_wrap;

    SPItem *item;
    Inkscape::CanvasItem *grabbed = nullptr;
    Inkscape::SelTrans *_seltrans;
    Inkscape::SelectionDescriber *_describer;
    gchar *no_selection_msg = nullptr;

    static const std::string prefsPath;

    void setup() override;
    void set(const Inkscape::Preferences::Entry& val) override;
    bool root_handler(GdkEvent* event) override;
    bool item_handler(SPItem* item, GdkEvent* event) override;

    const std::string& getPrefsPath() override;

private:
    bool sp_select_context_abort();
    void sp_select_context_cycle_through_items(Inkscape::Selection *selection, GdkEventScroll *scroll_event);
    void sp_select_context_reset_opacities();

    bool _alt_on;
    bool _force_dragging;

    Glib::RefPtr<Gdk::Cursor> _default_cursor;
    Glib::RefPtr<Gdk::Cursor> _cursor_mouseover;
    Glib::RefPtr<Gdk::Cursor> _cursor_dragging;
};

}
}
}
