// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::PageManager - Multi-Page management.
 *
 * Copyright 2021 Martin Owens <doctormo@geek-2.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_PAGE_MANAGER_H
#define SEEN_INKSCAPE_PAGE_MANAGER_H

#include <vector>

#include "color-rgba.h"
#include "document.h"
#include "object/sp-namedview.h"
#include "svg/svg-bool.h"

class SPDesktop;
class SPPage;

namespace Inkscape {
class Selection;
namespace UI {
namespace Dialog {
class DocumentProperties;
}
} // namespace UI

class PageManager
{
public:
    PageManager(SPDocument *document);
    ~PageManager();

    const std::vector<SPPage *> &getPages() const { return pages; }

    void addPage(SPPage *page);
    void removePage(Inkscape::XML::Node *child);
    void reorderPage(Inkscape::XML::Node *child);

    // Returns None if no page selected
    SPPage *getSelected() const { return _selected_page; }
    SPPage *getPageFor(SPItem *item, bool contains) const;
    bool hasPages() const { return !pages.empty(); }
    int getPageIndex(SPPage *page) const;
    int getSelectedPageIndex() const;

    void enablePages();
    void disablePages();
    void pagesChanged();
    bool selectPage(SPPage *page);
    bool selectPage(int page_index);
    bool selectPage(SPItem *item, bool contains) { return selectPage(getPageFor(item, contains)); }
    bool selectNextPage() { return selectPage(getSelectedPageIndex() + 1); }
    bool selectPrevPage() { return selectPage(getSelectedPageIndex() - 1); }
    bool hasNextPage() const { return getSelectedPageIndex() + 1 < pages.size(); }
    bool hasPrevPage() const { return getSelectedPageIndex() - 1 >= 0; }

    ColorRGBA getDefaultBackgroundColor() const { return ColorRGBA(background_color); }

    void zoomToPage(SPDesktop *desktop, SPPage *page);
    void zoomToSelectedPage(SPDesktop *desktop) { zoomToPage(desktop, _selected_page); };

    SPPage *newPage();
    SPPage *newPage(double width, double height);
    SPPage *newPage(Geom::Rect rect);
    SPPage *newDesktopPage(Geom::Rect rect);
    void deletePage(SPPage *page);
    void deletePage();
    void resizePage(double width, double height);

    bool subset(SPAttr key, const gchar *value);
    void modified();

    static void enablePages(SPDocument *document) { document->getNamedView()->getPageManager()->enablePages(); }
    static void disablePages(SPDocument *document) { document->getNamedView()->getPageManager()->disablePages(); }
    static SPPage *newPage(SPDocument *document) { return document->getNamedView()->getPageManager()->newPage(); }

    sigc::connection connectPageSelected(const sigc::slot<void, SPPage *> &slot)
    {
        return _page_selected_signal.connect(slot);
    }
    sigc::connection connectPagesChanged(const sigc::slot<void> &slot) { return _pages_changed_signal.connect(slot); }

    // Access from export.cpp and others for the guint32
    guint32 background_color = 0xffffff00;
protected:
    friend class Inkscape::UI::Dialog::DocumentProperties;

    // Default settings from sp-namedview
    SVGBool border_show;
    SVGBool border_on_top;
    SVGBool shadow_show;

    guint32 border_color = 0x000000cc;

    int shadow_size = 0;

private:
    SPDocument *_document;
    SPPage *_selected_page = nullptr;
    std::vector<SPPage *> pages;

    sigc::signal<void, SPPage *> _page_selected_signal;
    sigc::signal<void> _pages_changed_signal;
};

} // namespace Inkscape

#endif
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
