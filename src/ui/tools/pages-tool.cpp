// SPDX-License-Identifier: GPL-2.0-or-later
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


#include <gdk/gdkkeysyms.h>

#include "pages-tool.h"

#include "include/macros.h"

#include "desktop.h"
#include "rubberband.h"
#include "selection-chemistry.h"
#include "object/sp-page.h"

#include "display/control/canvas-item-rect.h"

namespace Inkscape {
namespace UI {
namespace Tools {

const std::string& PagesTool::getPrefsPath() {
	return PagesTool::prefsPath;
}

const std::string PagesTool::prefsPath = "/tools/pages";

PagesTool::PagesTool()
    : ToolBase("select.svg")
{
}

PagesTool::~PagesTool() {
}

void PagesTool::finish() {
    this->enableGrDrag(false);

    ungrabCanvasEvents();

    ToolBase::finish();

    if (visual_box) {
        delete visual_box;
        visual_box = nullptr;
    }
}

void PagesTool::setup() {
    ToolBase::setup();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    drag_tolerance = prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);

    if (!visual_box) {
        visual_box = new Inkscape::CanvasItemRect(desktop->getCanvasControls());
        visual_box->set_stroke(0x0000ff7f);
        visual_box->hide();
    }
}

bool PagesTool::root_handler(GdkEvent* event)
{
    bool ret = false;

    switch (event->type) {
        case GDK_BUTTON_PRESS:
        {
            if (event->button.button == 1) {
                mouse_is_pressed = true;
                drag_origin_w = Geom::Point(event->button.x, event->button.y);
                drag_origin_dt = desktop->w2d(drag_origin_w);
                ret = true;
            }
            break;
        }
        case GDK_MOTION_NOTIFY:
        {
            if (mouse_is_pressed && event->motion.state & GDK_BUTTON1_MASK) {
                auto point_w = Geom::Point(event->motion.x, event->motion.y);
                auto point_dt = desktop->w2d(point_w);

                // do not drag if we're within tolerance from origin
                if (Geom::distance(drag_origin_w, point_w) < drag_tolerance) {
                    break; 
                }
                if (dragging_item) {
                    // Continue to drag item.
                    delete dragging_box;
                    dragging_box = new Geom::Rect(dragging_item->getDesktopRect());
                    *dragging_box *= Geom::Translate(drag_origin_dt).inverse() * Geom::Translate(point_dt);
                } else if (dragging_box) {
                    // Continue to drag new box
                    delete dragging_box;
                    dragging_box = new Geom::Rect(drag_origin_dt, point_dt);
                } else if (auto page = page_under(point_dt)) {
                    // Starting to drag page around the screen.
                    dragging_item = page;
                    dragging_box = new Geom::Rect(page->getDesktopRect());
                } else {
                    // Start making a new page.
                    dragging_item = nullptr;
                    dragging_box = new Geom::Rect(point_dt, point_dt);
                }
            } else {
                mouse_is_pressed = false;
            }
            break;
        }
        case GDK_BUTTON_RELEASE:
        {
            auto point_w = Geom::Point(event->button.x, event->button.y);
            auto point_dt = desktop->w2d(point_w);

            if (auto page_manager = getPageManager()) {
                if (dragging_item) {
                    // conclude item here (move item to new location)
                    g_warning("conclude moving the page around.");
                    auto affine = Geom::Translate(drag_origin_dt).inverse() * Geom::Translate(point_dt);
                    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
                    dragging_item->movePage(affine, prefs->getBool("/tools/pages/move_objects"));
                } else if (dragging_box) {
                    // conclude box here (make new page)
                    page_manager->newDesktopPage(*dragging_box);
                } else if (auto page = page_under(point_dt)) {
                    // Select the clicked on page. Manager ignores the same-page.
                    page_manager->selectPage(page);
                }
            }
            mouse_is_pressed = false;
            ret = true;
            break;
        }
        case GDK_KEY_RELEASE:
        {
            if (event->key.keyval == GDK_KEY_Escape) {
                mouse_is_pressed = false;
            }
        }
        default:
            break;
    }

    // Clean up any finished dragging, doesn't matter how it ends
    if (mouse_is_pressed == false) {
        dragging_item = nullptr;
        dragging_box = nullptr;
        visual_box->hide();
        ret = true;
    } else if (dragging_box) {
        visual_box->show();
        visual_box->set_rect(*dragging_box);
        ret = true;
    }

    if (!ret) {
        ret = ToolBase::root_handler(event);
    }

    return ret;
}

/**
 * Find a page under the cursor point.
 */
SPPage *PagesTool::page_under(Geom::Point pt) {
    if (auto page_manager = getPageManager()) {
        // If the point is still on the selected, favour that one.
        if (auto selected = page_manager->getSelected()) {
            if (selected->getDesktopRect().contains(pt)) {
                return selected;
            }
        }
        // If multiple pages are at the same point; this currently only gives
        // you the bottom-most page (the first in the stack).
        for (auto &page : page_manager->getPages()) {
            if (page->getDesktopRect().contains(pt)) {
                return page;
            }
        }
    }
    return nullptr;
}

Inkscape::PageManager *PagesTool::getPageManager() {
    if (auto desktop = getDesktop()) {
        if (auto document = desktop->getDocument()) {
            return document->getNamedView()->getPageManager();
        }
    }
    return nullptr;
}

}
}
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
