// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::PageManager - a view of a document's pages
 *
 * Copyright 2021 Martin Owens
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_PAGE_MANAGER_H
#define SEEN_INKSCAPE_PAGE_MANAGER_H

#include <memory>
#include <vector>
#include <glibmm/ustring.h>

#include "document-subset.h"
#include "gc-finalized.h"
#include "inkgc/gc-soft-ptr.h"

class SPDesktop;
class SPDocument;

namespace Inkscape {

class PageManager : public DocumentSubset,
                     public GC::Finalized
{
public:
    PageManager(SPDesktop *desktop);
    ~PageManager() override;

    void setCurrentPage(SPObject* obj);

    sigc::connection connectCurrentPageChanged(const sigc::slot<void, SPObject *> & slot) {
        return _page_changed_signal.connect(slot);
    }
    sigc::connection connectPageDetailsChanged(const sigc::slot<void, SPObject *> & slot) {
        return _details_changed_signal.connect(slot);
    }

private:
    friend class PageWatcher;
    class PageWatcher;

    void _objectModified(SPObject* obj, unsigned int flags);
    void _setDocument(SPDocument *document);
    void _rebuild();
    void _selectedPageChanged(SPObject *page);

    sigc::connection _page_connection;
    sigc::connection _resource_connection;

    GC::soft_ptr<SPDesktop> _desktop;

    std::vector<std::unique_ptr<PageWatcher>> _watchers;

    sigc::signal<void, SPObject *> _page_changed_signal;
    sigc::signal<void, SPObject *> _details_changed_signal;

    void addOne(SPGroup *group);

    std::vector<Inkscape::CanvasItemRect *> _views;
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
