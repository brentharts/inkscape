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

class SPDesktop;

class SPPage : public SPObject {
public:
    SPPage();
    ~SPPage() override = default;

    void setLabel(const char* label, bool const commit);
    void showPage(SPDesktop *desktop, Inkscape::CanvasItemGroup *group);
    void hidePage(Inkscape::UI::Widget::Canvas *canvas);
    void showPage();
    void hidePage();

    void setPageColor(guint32 color);
    void setPageBorder(guint32 color);
    void setPageShadow(bool show);

    Geom::Rect getRect() const;
    Geom::Rect getDesktopRect() const;
    std::vector<SPItem*> getExclusiveItems() const;
    std::vector<SPItem*> getOverlappingItems() const;
protected:
    void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
    void release() override;
    void set(SPAttr key, const char* value) override;
private:
    std::string label;

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
