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

#include "pages-tool.h"

#include <gdk/gdkkeysyms.h>
#include <glibmm/i18n.h>

#include "desktop.h"
#include "display/control/canvas-item-curve.h"
#include "display/control/canvas-item-bpath.h"
#include "display/control/canvas-item-group.h"
#include "display/control/canvas-item-rect.h"
#include "display/control/snap-indicator.h"
#include "document-undo.h"
#include "include/macros.h"
#include "object/sp-page.h"
#include "path/path-outline.h"
#include "pure-transform.h"
#include "rubberband.h"
#include "selection.h"
#include "selection-chemistry.h"
#include "snap.h"
#include "snap-preferences.h"
#include "ui/knot/knot.h"

namespace Inkscape {
namespace UI {
namespace Tools {

const std::string &PagesTool::getPrefsPath()
{
    return PagesTool::prefsPath;
}

const std::string PagesTool::prefsPath = "/tools/pages";

PagesTool::PagesTool()
    : ToolBase("select.svg")
{}

PagesTool::~PagesTool() {}

void PagesTool::finish()
{
    _selector_changed_connection.disconnect();
    selectionChanged(nullptr);

    ungrabCanvasEvents();

    desktop->selection->restoreBackup();

    ToolBase::finish();

    if (visual_box) {
        delete visual_box;
        visual_box = nullptr;
    }

    if (resize_knot) {
        delete resize_knot;
        resize_knot = nullptr;
    }

    if (drag_group) {
        delete drag_group;
        drag_group = nullptr;
        drag_shapes.clear(); // Already deleted by group
    }
}

void PagesTool::setup()
{
    ToolBase::setup();

    // Stash the regular object selection so we don't modify them in base-tools root handler.
    desktop->selection->setBackup();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    drag_tolerance = prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);

    if (!resize_knot) {
        resize_knot = new SPKnot(desktop, _("Resize page"), Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "PageTool:Resize");
        resize_knot->setShape(Inkscape::CANVAS_ITEM_CTRL_SHAPE_SQUARE);
        resize_knot->setFill(0xffffff00, 0x0000ff00, 0x000000ff, 0x000000ff);
        resize_knot->setSize(9);
        resize_knot->setAnchor(SP_ANCHOR_CENTER);
        resize_knot->updateCtrl();
        resize_knot->hide();
        resize_knot->moved_signal.connect(sigc::mem_fun(*this, &PagesTool::resizeKnotMoved));
        resize_knot->ungrabbed_signal.connect(sigc::mem_fun(*this, &PagesTool::resizeKnotFinished));
    }

    if (!visual_box) {
        visual_box = new Inkscape::CanvasItemRect(desktop->getCanvasControls());
        visual_box->set_stroke(0x0000ff7f);
        visual_box->hide();
    }
    if (!drag_group) {
        drag_group = new Inkscape::CanvasItemGroup(desktop->getCanvasTemp());
        drag_group->set_name("CanvasItemGroup:PagesDragShapes");
    }
    if (auto page_manager = getPageManager()) {
        _selector_changed_connection =
            page_manager->connectPageSelected(sigc::mem_fun(*this, &PagesTool::selectionChanged));
        if (auto page = page_manager->getSelected()) {
            selectionChanged(page);
        }
    }
}

void PagesTool::resizeKnotMoved(SPKnot *knot, Geom::Point const &ppointer, guint state)
{
    Geom::Point point = knot->position();
    if (auto page_manager = getPageManager()) {
        if (auto page = page_manager->getSelected()) {
            auto rect = page->getDesktopRect();
            if (point != rect.corner(2)) {
                rect.setMax(point);
                visual_box->show();
                visual_box->set_rect(rect);
                if (on_screen_rect) {
                    delete on_screen_rect;
                }
                on_screen_rect = new Geom::Rect(rect);
                mouse_is_pressed = true;
            }
        }
    }
}


void PagesTool::resizeKnotFinished(SPKnot *knot, guint state)
{
    if (auto page_manager = getPageManager()) {
        if (on_screen_rect) {
            page_manager->resizePage(on_screen_rect->width(), on_screen_rect->height());
            Inkscape::DocumentUndo::done(desktop->getDocument(), SP_VERB_NONE, "Resize page");
            delete on_screen_rect;
            on_screen_rect = nullptr;
        }
    }
    visual_box->hide();
    mouse_is_pressed = false;
}

bool PagesTool::root_handler(GdkEvent *event)
{
    bool ret = false;
    auto page_manager = getPageManager();
    if (!page_manager)
        return false;

    switch (event->type) {
        case GDK_BUTTON_PRESS: {
            if (event->button.button == 1) {
                mouse_is_pressed = true;
                drag_origin_w = Geom::Point(event->button.x, event->button.y);
                drag_origin_dt = desktop->w2d(drag_origin_w);
                ret = true;
                if (auto page = pageUnder(drag_origin_dt)) {
                    // Select the clicked on page. Manager ignores the same-page.
                    page_manager->selectPage(page);
                }
            }
            break;
        }
        case GDK_MOTION_NOTIFY: {
            if (mouse_is_pressed && event->motion.state & GDK_BUTTON1_MASK) {
                auto point_w = Geom::Point(event->motion.x, event->motion.y);
                auto point_dt = desktop->w2d(point_w);

                // do not drag if we're within tolerance from origin
                if (Geom::distance(drag_origin_w, point_w) < drag_tolerance) {
                    break;
                }

                if (dragging_item) {
                    // Continue to drag item.
                    Geom::Affine tr = moveTo(point_dt);
                    // XXX Moving the existing shapes would be much better, but it has
                    //  weird bug which stops it from working well.
                    //drag_group->update(tr * drag_group->get_parent()->get_affine());
                    addDragShapes(dragging_item, tr);
                } else if (on_screen_rect) {
                    // Continue to drag new box
                    delete on_screen_rect;
                    on_screen_rect = new Geom::Rect(drag_origin_dt, point_dt);
                } else if (auto page = pageUnder(point_dt)) {
                    // Starting to drag page around the screen.
                    dragging_item = page;
                    page_manager->selectPage(page);
                    addDragShapes(page, Geom::Affine());
                    grabPage(page);
                } else {
                    // Start making a new page.
                    setupResizeSnap(point_dt);
                    dragging_item = nullptr;
                    on_screen_rect = new Geom::Rect(point_dt, point_dt);
                }
            } else {
                mouse_is_pressed = false;
            }
            break;
        }
        case GDK_BUTTON_RELEASE: {
            auto point_w = Geom::Point(event->button.x, event->button.y);
            auto point_dt = desktop->w2d(point_w);

            if (dragging_item) {
                if (dragging_item->isViewportPage()) {
                    // Move the document's viewport first
                    auto rect = dragging_item->document->preferredBounds();
                    auto affine = moveTo(point_dt);
                    dragging_item->document->fitToRect(*rect * affine, false);
                    // Now move the page back to where we expect it.
                    dragging_item->movePage(affine, page_manager->move_objects());
                    dragging_item->setDesktopRect(*rect);
                } else {
                    // Move the page object on the canvas.
                    dragging_item->movePage(moveTo(point_dt), page_manager->move_objects());
                }
                Inkscape::DocumentUndo::done(desktop->getDocument(), SP_VERB_NONE, "Move page position");
            } else if (on_screen_rect) {
                // conclude box here (make new page)
                page_manager->newDesktopPage(*on_screen_rect);
                Inkscape::DocumentUndo::done(desktop->getDocument(), SP_VERB_NONE, "Create new drawn page");
            }
            mouse_is_pressed = false;
            ret = true;
            break;
        }
        case GDK_KEY_RELEASE: {
            if (event->key.keyval == GDK_KEY_Escape) {
                mouse_is_pressed = false;
                ret = true;
            }
            if (event->key.keyval == GDK_KEY_Delete) {
                page_manager->deletePage(page_manager->move_objects());
                Inkscape::DocumentUndo::done(desktop->getDocument(), SP_VERB_NONE, "Delete Page");
                ret = true;
            }
        }
        default:
            break;
    }

    // Clean up any finished dragging, doesn't matter how it ends
    if (!mouse_is_pressed && (dragging_item || on_screen_rect)) {
        dragging_item = nullptr;
        on_screen_rect = nullptr;
        clearDragShapes();
        visual_box->hide();
        unsetupSnap();
        ret = true;
    } else if (on_screen_rect) {
        visual_box->show();
        visual_box->set_rect(*on_screen_rect);
        ret = true;
    }

    return ret ? true : ToolBase::root_handler(event);
}

/**
 * Creates the right snapping setup for dragging items around.
 */
void PagesTool::grabPage(SPPage *target)
{
    unsetupSnap();
    snap_manager = &(desktop->namedview->snap_manager);
    snap_manager->setup(desktop, true, target);

    _bbox_points.clear();
    getBBoxPoints(target->getDesktopRect(), &_bbox_points, false,
        SNAPSOURCE_PAGE_CORNER, SNAPTARGET_UNDEFINED,
        SNAPSOURCE_UNDEFINED, SNAPTARGET_UNDEFINED,
        SNAPSOURCE_PAGE_CENTER, SNAPTARGET_UNDEFINED);
}

/*
 * Generate the movement affine as the page is dragged around (including snapping)
 */
Geom::Affine PagesTool::moveTo(Geom::Point xy)
{
    Geom::Point dxy = xy - drag_origin_dt;

    if (snap_manager) {
        snap_manager->snapprefs.clearTargetMask(0); // Disable all snapping targets
        snap_manager->snapprefs.setTargetMask(SNAPTARGET_ALIGNMENT_CATEGORY, -1);
        snap_manager->snapprefs.setTargetMask(SNAPTARGET_ALIGNMENT_PAGE_CORNER, -1);
        snap_manager->snapprefs.setTargetMask(SNAPTARGET_ALIGNMENT_PAGE_CENTER, -1);
        snap_manager->snapprefs.setTargetMask(SNAPTARGET_PAGE_CORNER, -1);
        snap_manager->snapprefs.setTargetMask(SNAPTARGET_PAGE_CENTER, -1);

        Inkscape::PureTranslate *bb = new Inkscape::PureTranslate(dxy);
        snap_manager->snapTransformed(_bbox_points, drag_origin_dt, (*bb));

        if (bb->best_snapped_point.getSnapped()) {
            dxy = bb->getTranslationSnapped();
            desktop->snapindicator->set_new_snaptarget(bb->best_snapped_point);
        }

        snap_manager->snapprefs.clearTargetMask(-1); // Reset preferences
    }

    return Geom::Translate(dxy);
}

/**
 * Create the snapping for the resize boxes.
 */
void PagesTool::setupResizeSnap(Geom::Point start)
{
    unsetupSnap();
    // XXX Add all the resize points
}

void PagesTool::unsetupSnap()
{
    if (snap_manager) {
      snap_manager->unSetup();
      snap_manager = nullptr;
    }
}


/**
 * Add all the shapes needed to see it being dragged.
 */
void PagesTool::addDragShapes(SPPage *page, Geom::Affine tr)
{
    clearDragShapes();
    addDragShape(page, tr);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/tools/pages/move_objects", true)) {
        for (auto &item : page->getOverlappingItems()) {
            addDragShape(item, tr);
        }
    }
}

/**
 * Add a page the the things being dragged.
 */
void PagesTool::addDragShape(SPPage *page, Geom::Affine tr)
{
    addDragShape(Geom::PathVector(Geom::Path(Geom::Rect(page->getDesktopRect()))), tr);
}

/**
 * Add an SPItem to the things being dragged.
 */
void PagesTool::addDragShape(SPItem *item, Geom::Affine tr)
{
    if (auto shape = item_to_outline(item)) {
        addDragShape(*shape * item->i2dt_affine(), tr);
    }
}

/**
 * Add a shape to the set of dragging shapes, these are deleted when dragging stops.
 */
void PagesTool::addDragShape(Geom::PathVector pth, Geom::Affine tr)
{
    auto shape = new CanvasItemBpath(drag_group, pth * tr, false);
    shape->set_stroke(0x00ff007f);
    shape->set_fill(0x00000000, SP_WIND_RULE_EVENODD);
    drag_shapes.push_back(shape);
}

/**
 * Remove all drag shapes from the canvas.
 */
void PagesTool::clearDragShapes()
{
    for (auto &shape : drag_shapes) {
        delete shape;
    }
    drag_shapes.clear();
}

/**
 * Find a page under the cursor point.
 */
SPPage *PagesTool::pageUnder(Geom::Point pt)
{
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

Inkscape::PageManager *PagesTool::getPageManager()
{
    if (auto desktop = getDesktop()) {
        if (auto document = desktop->getDocument()) {
            return document->getNamedView()->getPageManager();
        }
    }
    return nullptr;
}

void PagesTool::selectionChanged(SPPage *page)
{
    if (_page_modified_connection) {
        _page_modified_connection.disconnect();
        resize_knot->hide();
    }

    // Loop existing pages because highlight_item is unsafe.
    if (auto page_manager = getPageManager()) {
        for (auto &possible : page_manager->getPages()) {
            if (highlight_item == possible) {
                highlight_item->setSelected(false);
            }
        }
        highlight_item = page;
        if (page) {
            _page_modified_connection = page->connectModified(sigc::mem_fun(*this, &PagesTool::pageModified));
            page->setSelected(true);
            pageModified(page, 0);
        }
    }
}

void PagesTool::pageModified(SPObject *object, guint /*flags*/)
{
    if (auto page = dynamic_cast<SPPage *>(object)) {
        if (resize_knot) {
            resize_knot->setPosition(page->getDesktopRect().corner(2), 0);
            resize_knot->show();
        }
    }
}

} // namespace Tools
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
