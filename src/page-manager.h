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
#include "document.h"
#include "object/sp-namedview.h"

class SPDesktop;
class SPPage;

namespace Inkscape {

class PageManager
{
public:
    PageManager(SPDocument *document);
    ~PageManager();

    std::vector<SPPage *> getPages() const { return pages; }
    void addPage(SPPage *page);
    void removePage(Inkscape::XML::Node *child);
    void reorderPage(Inkscape::XML::Node *child);
    
    // Returns None if no page selected
    SPPage *getSelected() const { return _selected_page; }
    bool hasPages() const { return !pages.empty(); }
    int getPageIndex(SPPage *page) const;
    int getSelectedPageIndex() const;

    void enablePages();
    void disablePages();
    void pagesChanged();
    void selectPage(SPPage *page);
    void selectPage(int page_index);
    void selectNextPage() { selectPage(getSelectedPageIndex() + 1); }
    void selectPrevPage() { selectPage(getSelectedPageIndex() - 1); }
    SPPage *newPage();
    SPPage *newPage(double width, double height);

    static void enablePages(SPDocument *document) {
        document->getNamedView()->getPageManager()->enablePages();
    }
    static void disablePages(SPDocument *document) {
        document->getNamedView()->getPageManager()->disablePages();
    }
    static SPPage *newPage(SPDocument *document) {
        return document->getNamedView()->getPageManager()->newPage();
    }

    sigc::connection connectPageSelected(const sigc::slot<void, SPPage *> & slot) {
        return _page_selected_signal.connect(slot);
    }
    sigc::connection connectPagesChanged(const sigc::slot<void> & slot) {
        return _pages_changed_signal.connect(slot);
    }

private:
    SPDocument *_document;
    SPPage *_selected_page = nullptr;
    std::vector<SPPage *> pages;

    sigc::signal<void, SPPage *> _page_selected_signal;
    sigc::signal<void> _pages_changed_signal;
};

}

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
