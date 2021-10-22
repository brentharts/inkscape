// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * SPPage -- a guideline
 *//*
 * Authors:
 *   Martin Owens 2021
 * 
 * Copyright (C) 2021 Martin Owens
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_PAGE_H
#define SEEN_SP_PAGE_H

#include <2geom/rect.h>
#include <vector>

#include "display/control/canvas-item-rect.h"
#include "display/control/canvas-item-text.h"
#include "page-manager.h"
#include "sp-object.h"
#include "svg/svg-length.h"

class SPDesktop;
class SPItem;

class PageOnCanvas
{
public:
    PageOnCanvas();
    ~PageOnCanvas();

    void update(Geom::Rect size, const char *txt, bool outline = false);
    void add(Geom::Rect size, Inkscape::CanvasItemGroup *background_group, Inkscape::CanvasItemGroup *border_group);
    void remove(Inkscape::UI::Widget::Canvas *canvas);
    void show();
    void hide();

    bool setAttributes(bool on_top, guint32 border, guint32 bg, int shadow);
    void setOutline(bool outline);

    bool is_selected = false;
private:
    std::vector<Inkscape::CanvasItem *> canvas_items;

    bool _border_on_top = true;
    guint32 _background_color = 0xffffff00;
    guint32 _border_color = 0x000000cc;
    int _shadow_size = 0;
};

class SPPage : public SPObject
{
public:
    SPPage();
    ~SPPage() override;

    void movePage(Geom::Affine translate, bool with_objects);
    void swapPage(SPPage *other, bool with_objects);

    // Canvas visualisation
    void showPage(Inkscape::CanvasItemGroup *fg, Inkscape::CanvasItemGroup *bg);
    void hidePage(Inkscape::UI::Widget::Canvas *canvas) { _canvas_item->remove(canvas); }
    void showPage() { _canvas_item->show(); }
    void hidePage() { _canvas_item->hide(); }

    void setManager(Inkscape::PageManager *manager);

    void setSelected(bool selected);
    bool setDefaultAttributes();
    int getPageIndex();
    int getPagePosition() { return getPageIndex() + 1; }
    bool setPageIndex(int index, bool swap_page);
    bool setPagePosition(int position, bool swap_page) { return setPageIndex(position - 1, swap_page); }

    SPPage *getNextPage();
    SPPage *getPreviousPage();

    Geom::Rect getRect() const;
    Geom::Rect getDesktopRect() const;
    void setRect(Geom::Rect rect);
    void setDesktopRect(Geom::Rect rect);
    void setDesktopSize(double width, double height);
    std::vector<SPItem *> getExclusiveItems() const;
    std::vector<SPItem *> getOverlappingItems() const;
    bool itemOnPage(SPItem *item, bool contains = false) const;

protected:
    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
    void release() override;
    void update(SPCtx *ctx, unsigned int flags) override;
    void set(SPAttr key, const char *value) override;
    Inkscape::XML::Node *write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) override;

private:
    PageOnCanvas *_canvas_item = nullptr;
    Inkscape::PageManager *_manager = nullptr;

    SVGLength x;
    SVGLength y;
    SVGLength width;
    SVGLength height;
};

#endif // SEEN_SP_PAGE_H

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
