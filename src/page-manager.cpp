// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::PageManager - Multi-Page management.
 *
 * Copyright 2021 Martin Owens <doctormo@geek-2.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "desktop.h"
#include "document.h"
#include "page-manager.h"

#include "object/sp-namedview.h"
#include "object/sp-page.h"

namespace Inkscape {

PageManager::PageManager(SPDocument *document)
{
    _document = document;
}

PageManager::~PageManager()
{
    pages.clear();
    _selected_page = nullptr;
}

/**
 * Add a page to this manager, called from namedview parent.
 */
void PageManager::addPage(SPPage *page)
{
    // TODO: Pages may not always be added at the end.
    pages.push_back(page);
    page->setManager(this);
    pagesChanged();
}

/**
 * Remove a page from this manager, called from namedview parent.
 */
void PageManager::removePage(Inkscape::XML::Node *child)
{
    for(auto it = pages.begin(); it != pages.end(); ++it) {
        if ((*it)->getRepr() == child) {
            pages.erase(it);
            pagesChanged();
            break;
        }
    }
}

/**
 * Reorder page within the internal list to keep it up to date.
 */
void PageManager::reorderPage(Inkscape::XML::Node *child)
{
    auto nv = _document->getNamedView();
    pages.clear();
    // Reverse order from children order, we want the top-down order.
    for (auto rit = nv->children.rbegin(); rit != nv->children.rend(); ++rit) {
        if (auto page = dynamic_cast<SPPage *>(&*rit)) {
            pages.push_back(page);
        }
    }
    pagesChanged();
}

/**
 * Enables multi page support by turning the document viewBox into
 * the first page.
 */
void PageManager::enablePages()
{
    if (!hasPages()) {
        newPage();
    }
}

/**
 * Add a new page of the default size, this will be either
 * the size of the viewBox if no pages exist, or the size
 * of the selected page.
 */
SPPage *PageManager::newPage()
{
    auto unit = _document->getDisplayUnit();
    if (_selected_page) {
        auto rect = _selected_page->getRect();
        return newPage(rect.width(), rect.height());
    }
    return newPage(
        _document->getWidth().value(unit),
        _document->getHeight().value(unit));
}

/**
 * Add a new page of the given width and height.
 */
SPPage *PageManager::newPage(double width, double height)
{
    // Get a new location for the page.
    // XXX This is just silly simple.
    double top = 0.0;
    double left = 0.0;
    for (auto &page : pages) {
        auto rect = page->getRect();
        if (rect.right() > left) {
            left = rect.right() + 10;
        }
    }
    return newPage(Geom::Rect(left, top, left + width, top + height));
}


/**
 * Add a new page with the given rectangle.
 */
SPPage *PageManager::newPage(Geom::Rect rect)
{
    auto xml_doc = _document->getReprDoc();
    auto repr = xml_doc->createElement("inkscape:page");
    repr->setAttributeSvgDouble("x", rect.left());
    repr->setAttributeSvgDouble("y", rect.top());
    repr->setAttributeSvgDouble("width", rect.width());
    repr->setAttributeSvgDouble("height", rect.height());
    if (auto nv = _document->getNamedView()) {
        if (auto page = dynamic_cast<SPPage *>(nv->appendChildRepr(repr))) {
            return page;
        }
    }
    return nullptr;
}

/**
 * Create a new page, resizing the rectangle from desktop coordinates.
 */
SPPage *PageManager::newDesktopPage(Geom::Rect rect)
{
    return newPage(rect * _document->getDocumentScale().inverse());
}

/**
 * Delete the given page.
 */
void PageManager::deletePage(SPPage *page)
{
    if (page) {
        // Removal from pages is done automatically via signals.
        page->deleteObject();
    }
}

/**
 * Delete the selected page.
 */
void PageManager::deletePage()
{
    deletePage(_selected_page);
}

/**
 * Disables multi page supply by removing all the page objects.
 */
void PageManager::disablePages()
{
    if (hasPages()) {
        for (auto &page : pages) {
            page->deleteObject();
        }
    }
}

/**
 * Get page index, returns -1 if the page is not found in this document.
 */
int PageManager::getPageIndex(SPPage *page) const
{
    if (page) {
        auto it = std::find(pages.begin(), pages.end(), page);
        if (it != pages.end()) {
            return it - pages.begin();
        }
        g_warning("Can't get page index for %s", page->getId());
    }
    return -1;
}

/**
 * Return the index of the page in the index
 */
int PageManager::getSelectedPageIndex() const
{
    return getPageIndex(_selected_page);
}

/**
 * Called when the pages vector is updated, either page
 * deleted or page created (but not if the page is modified)
 */
void PageManager::pagesChanged()
{
    if (pages.empty() || getSelectedPageIndex() == -1) {
        _selected_page = nullptr;
    }
    _pages_changed_signal.emit();
    if (!_selected_page) {
        for (auto &page : pages) {
            selectPage(page);
            break;
        }
    }
}

/**
 * Set the given page as the selected page.
 *
 * @param page - The page to set as the selected page.
 */
bool PageManager::selectPage(SPPage *page)
{
    if (getPageIndex(page) >= 0) {
        if (_selected_page != page) {
            if (_selected_page) {
                _selected_page->setSelected(false);
            }
            _selected_page = page;
            _selected_page->setSelected(true);
            _page_selected_signal.emit(_selected_page);
            return true;
        }
    }
    return false;
}

/**
 * Set the given page as the selected page.
 *
 * @param page_index - The page index (from 0) of the page to select.
 */
bool PageManager::selectPage(int page_index)
{
    if (page_index >= 0 && page_index < pages.size()) {
        return selectPage(pages[page_index]);
    }
    return false;
}

/**
 * Center/zoom on the given page.
 */
void PageManager::zoomToPage(SPDesktop *desktop, SPPage *page)
{
    if (!page) return;

    auto d = page->getDesktopRect();
    if (d.minExtent() < 1.0)
          return;

    desktop->set_display_area(d, 10);
}


};

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
