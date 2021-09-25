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

class SPPage : public SPObject
{
public:
    SPPage();
    ~SPPage() override = default;

    void movePage(Geom::Affine translate, bool with_objects);
    void showPage(SPDesktop *desktop, Inkscape::CanvasItemGroup *, Inkscape::CanvasItemGroup *);
    void hidePage(Inkscape::UI::Widget::Canvas *canvas);
    void showPage();
    void hidePage();

    void setManager(Inkscape::PageManager *manager);

    void setSelected(bool selected);
    bool setDefaultAttributes(bool on_top, guint32 border, guint32 bg, int shadow);
    int getPageNumber();

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
    bool is_selected = false;

    bool border_on_top = true;
    guint32 background_color = 0xffffff00;
    guint32 border_color = 0x000000cc;
    int shadow_size = 0;

    Inkscape::PageManager *_manager = nullptr;
    std::vector<Inkscape::CanvasItem *> canvas_items;

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
