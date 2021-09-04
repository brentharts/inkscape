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
    pages.empty();
    _selected_page = nullptr;
}

/**
 * Add a page to this manager, called from namedview parent.
 */
void PageManager::addPage(SPPage *page)
{
    pages.push_back(page);
}

/**
 * Remove a page from this manager, called from namedview parent.
 */
void PageManager::removePage(Inkscape::XML::Node *child) {
    g_warning("removePage...");
    for(auto it = pages.begin(); it != pages.end(); ++it) {
        g_warning("removePage Iter... %d", (int)pages.size());
        if ((*it)->getRepr() == child) {
            pages.erase(it);
            pagesChanged();
            break;
        }
    }
}

/**
 * Enables multi page support by turning the document viewBox into
 * the first page.
 */
void PageManager::enablePages()
{
    if (!hasPages()) {
        auto unit = _document->getDisplayUnit();
        auto width = _document->getWidth().value(unit);
        auto height = _document->getHeight().value(unit);
        auto xml_doc = _document->getReprDoc();
        auto page = xml_doc->createElement("inkscape:page");
        page->setAttributeSvgDouble("x", 0.0);
        page->setAttributeSvgDouble("y", 0.0);
        page->setAttributeSvgDouble("width", width);
        page->setAttributeSvgDouble("height", height);
        if (auto nv = _document->getNamedView()) {
            nv->getRepr()->appendChild(page);
        }
    }
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
    auto it = std::find(pages.begin(), pages.end(), page);
    if (it != pages.end()) {
        return it - pages.begin();
    }
    return -1;
}

/**
 * Return the index of the page in the index
 */
int PageManager::getCurrentPageIndex() const
{
    return getPageIndex(_selected_page);
}

/**
 * Called when the pages vector is updated, either page
 * deleted or page created (but not if the page is modified)
 */
void PageManager::pagesChanged()
{
    if (pages.empty() || getCurrentPageIndex() == -1) {
        _selected_page = nullptr;
    }
    for (auto &page : pages) {
        if (!_selected_page) {
            _selected_page = page;
            break;
        }
    }
}

/**
 * Set the given page as the current page.
 */
void PageManager::selectPage(SPPage *page)
{
    if (getPageIndex(page) != -1) {
        _selected_page = page;
    }
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
