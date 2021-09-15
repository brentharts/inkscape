// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __UI_TOOLS_PAGES_CONTEXT_H__
#define __UI_TOOLS_PAGES_CONTEXT_H__

/*
 * Page editing tool
 *
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/tools/tool-base.h"

#define SP_PAGES_CONTEXT(obj) (dynamic_cast<Inkscape::UI::Tools::PagesTool*>((Inkscape::UI::Tools::ToolBase*)obj))
#define SP_IS_PAGES_CONTEXT(obj) (dynamic_cast<const Inkscape::UI::Tools::PagesTool*>((const Inkscape::UI::Tools::ToolBase*)obj) != NULL)

class SPObject;
class SPPage;

namespace Inkscape {
    class CanvasItemRect;
    class PageManager;

namespace UI {
namespace Tools {

class PagesTool : public ToolBase {
    public:
	PagesTool();
	~PagesTool() override;

	static const std::string prefsPath;

	void setup() override;
	void finish() override;
	bool root_handler(GdkEvent* event) override;

	const std::string& getPrefsPath() override;

    private:
        SPPage *page_under(Geom::Point pt);
        Inkscape::PageManager *getPageManager();

        bool mouse_is_pressed = false;
        Geom::Point drag_origin_w;
        Geom::Point drag_origin_dt;
        int drag_tolerance = 5;

        SPPage *dragging_item = nullptr;
        Geom::Rect *dragging_box = nullptr;
        Inkscape::CanvasItemRect *visual_box = nullptr;
};

}
}
}

#endif
