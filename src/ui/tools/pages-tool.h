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

#define SP_PAGES_CONTEXT(obj) (dynamic_cast<Inkscape::UI::Tools::PagesTool *>((Inkscape::UI::Tools::ToolBase *)obj))
#define SP_IS_PAGES_CONTEXT(obj) \
    (dynamic_cast<const Inkscape::UI::Tools::PagesTool *>((const Inkscape::UI::Tools::ToolBase *)obj) != NULL)

class SPObject;
class SPPage;
class SPKnot;
class SnapManager;

namespace Inkscape {
class SnapCandidatePoint;
class CanvasItemGroup;
class CanvasItemRect;
class CanvasItemBpath;
class PageManager;

namespace UI {
namespace Tools {

class PagesTool : public ToolBase
{
public:
    PagesTool();
    ~PagesTool() override;

    static const std::string prefsPath;

    void setup() override;
    void finish() override;
    bool root_handler(GdkEvent *event) override;

    const std::string &getPrefsPath() override;

private:
    void selectionChanged(SPPage *page);
    SPPage *pageUnder(Geom::Point pt);
    bool viewboxUnder(Geom::Point pt);
    void addDragShapes(SPPage *page, Geom::Affine tr);
    void addDragShape(SPPage *page, Geom::Affine tr);
    void addDragShape(SPItem *item, Geom::Affine tr);
    void addDragShape(Geom::PathVector pth, Geom::Affine tr);
    void clearDragShapes();
    Inkscape::PageManager *getPageManager();

    Geom::Point getSnappedResizePoint(Geom::Point point, guint state, Geom::Point origin, SPObject *target = nullptr);
    void resizeKnotMoved(SPKnot *knot, Geom::Point const &ppointer, guint state);
    void resizeKnotFinished(SPKnot *knot, guint state);
    void pageModified(SPObject *object, guint flags);

    void grabPage(SPPage *target);
    Geom::Affine moveTo(Geom::Point xy, bool snap);

    sigc::connection _selector_changed_connection;
    sigc::connection _page_modified_connection;

    bool mouse_is_pressed = false;
    Geom::Point drag_origin_w;
    Geom::Point drag_origin_dt;
    int drag_tolerance = 5;

    SPKnot *resize_knot = nullptr;
    SPPage *highlight_item = nullptr;
    SPPage *dragging_item = nullptr;
    Geom::Rect *on_screen_rect = nullptr;
    Inkscape::CanvasItemRect *visual_box = nullptr;
    Inkscape::CanvasItemGroup *drag_group = nullptr;
    std::vector<Inkscape::CanvasItemBpath *> drag_shapes;

    std::vector<Inkscape::SnapCandidatePoint> _bbox_points;
};

} // namespace Tools
} // namespace UI
} // namespace Inkscape

#endif
