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
#include "sp-object.h"
#include "svg/svg-length.h"
#include "page-manager.h"

class SPDesktop;

class SPPage : public SPObject {
public:
    SPPage();
    ~SPPage() override = default;

    void movePage(Geom::Affine translate, bool with_objects);
    void showPage(SPDesktop *desktop, Inkscape::CanvasItemGroup *group);
    void hidePage(Inkscape::UI::Widget::Canvas *canvas);
    void showPage();
    void hidePage();

    void setManager(Inkscape::PageManager *manager);

    void setPageColor(guint32 color);
    void setPageBorder(guint32 color);
    void setPageShadow(bool show);
    int getPageNumber();

    Geom::Rect getRect() const;
    Geom::Rect getDesktopRect() const;
    std::vector<SPItem*> getExclusiveItems() const;
    std::vector<SPItem*> getOverlappingItems() const;
protected:
    void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
    void release() override;
    void update(SPCtx* ctx, unsigned int flags) override;
    void set(SPAttr key, const char* value) override;
private:

    Inkscape::PageManager *_manager = nullptr;
    std::vector<Inkscape::CanvasItemRect *> views;

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
