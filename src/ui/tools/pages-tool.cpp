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
#include "document-undo.h"
#include "object/sp-page.h"
#include "path/path-outline.h"

#include "display/control/canvas-item-group.h"
#include "display/control/canvas-item-rect.h"
#include "display/control/canvas-item-bpath.h"

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
    _selector_changed_connection.disconnect();
    selectionChanged(nullptr);

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
    if (auto page_manager = getPageManager()) {
        _selector_changed_connection = page_manager->connectPageSelected(sigc::mem_fun(*this, &PagesTool::selectionChanged));
        if (auto page = page_manager->getSelected()) {
            selectionChanged(page);
        }
    }
}

bool PagesTool::root_handler(GdkEvent* event)
{
    bool ret = false;
    auto page_manager = getPageManager();
    if (!page_manager) return false;

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
                    Geom::Affine tr = Geom::Translate(drag_origin_w).inverse() * Geom::Translate(point_w);
                    for (auto &shape : drag_shapes) {
                        shape->update(shape->get_parent()->get_affine() * tr);
                    }
                } else if (creating_box) {
                    // Continue to drag new box
                    delete creating_box;
                    creating_box = new Geom::Rect(drag_origin_dt, point_dt);
                } else if (auto page = pageUnder(point_dt)) {
                    // Starting to drag page around the screen.
                    dragging_item = page;
                    addDragShape(page);
                    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
                    if (prefs->getBool("/tools/pages/move_objects", true)) {
                        for (auto &item : page->getOverlappingItems()) {
                            addDragShape(item);
                        }
                    }
                    page_manager->selectPage(page);
                } else {
                    // Start making a new page.
                    dragging_item = nullptr;
                    creating_box = new Geom::Rect(point_dt, point_dt);
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

            if (dragging_item) {
                // conclude item here (move item to new location)
                auto affine = Geom::Translate(drag_origin_dt).inverse() * Geom::Translate(point_dt);
                Inkscape::Preferences *prefs = Inkscape::Preferences::get();
                dragging_item->movePage(affine, prefs->getBool("/tools/pages/move_objects", true));
                Inkscape::DocumentUndo::done(desktop->getDocument(), SP_VERB_NONE, "Move page position");
            } else if (creating_box) {
                // conclude box here (make new page)
                page_manager->newDesktopPage(*creating_box);
                Inkscape::DocumentUndo::done(desktop->getDocument(), SP_VERB_NONE, "Create new drawn page");
            } else if (auto page = pageUnder(point_dt)) {
                // Select the clicked on page. Manager ignores the same-page.
                page_manager->selectPage(page);
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
    if (!mouse_is_pressed && (dragging_item || creating_box)) {
        dragging_item = nullptr;
        creating_box = nullptr;
        for (auto &shape : drag_shapes) {
            delete shape;
        }
        drag_shapes.clear();
        visual_box->hide();
        ret = true;
    } else if (creating_box) {
        visual_box->show();
        visual_box->set_rect(*creating_box);
        ret = true;
    }

    return ret ? true : ToolBase::root_handler(event);
}

/**
 * Add a page the the things being dragged.
 */
void PagesTool::addDragShape(SPPage *page)
{
    addDragShape(Geom::PathVector(Geom::Path(Geom::Rect(page->getDesktopRect()))));
}

/**
 * Add an SPItem to the things being dragged.
 */
void PagesTool::addDragShape(SPItem *item)
{
    auto shape = *item_to_outline(item);
    addDragShape(shape * item->i2dt_affine());
}

/**
 * Add a shape to the set of dragging shapes, these are deleted when dragging stops.
 */
void PagesTool::addDragShape(Geom::PathVector pth)
{
    auto shape = new CanvasItemBpath(desktop->getCanvasTemp(), pth, false);
    shape->set_stroke(0x00ff007f);
    shape->set_fill(0x00000000, SP_WIND_RULE_EVENODD);
    drag_shapes.push_back(shape);
}

/**
 * Find a page under the cursor point.
 */
SPPage *PagesTool::pageUnder(Geom::Point pt) {
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

void PagesTool::selectionChanged(SPPage *page)
{
    // Loop existing pages because highlight_item is unsafe.
    if (auto page_manager = getPageManager()) {
        for (auto &possible : page_manager->getPages()) {
            if (highlight_item == possible) {
                highlight_item->setSelected(false);
            }
        }
        highlight_item = page;
        if (page) {
            page->setSelected(true);
        }
    }
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
