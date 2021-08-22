// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::PageManager - Manage a list of pages in a document.
 *
 * Copyright 2021 Martin Ownes
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <set>

#include <sigc++/functors/mem_fun.h>
#include <sigc++/adaptors/hide.h>


#include "desktop.h"
#include "document.h"
#include "gc-finalized.h"
#include "page-manager.h"
#include "selection.h"

#include "inkgc/gc-managed.h"

#include "display/control/canvas-item-rect.h"

#include "object/sp-item-group.h"
#include "object/sp-clippath.h"

#include "xml/node-observer.h"

namespace Inkscape {


using Inkscape::XML::Node;

class PageManager::PageWatcher : public Inkscape::XML::NodeObserver {
public:
    PageWatcher(PageManager* mgr, SPObject* obj)
        : _mgr(mgr)
        , _obj(obj)
        , _lockedAttr(g_quark_from_string("sodipodi:insensitive"))
        , _labelAttr(g_quark_from_string("inkscape:label"))
        , _transformAttr(g_quark_from_string("transform"))
    {
        _connection = _obj->connectModified(sigc::mem_fun(*mgr, &PageManager::_objectModified));
        _obj->getRepr()->addObserver(*this);
    }

    ~PageWatcher() override
    {
        _connection.disconnect();

        if (_obj) {
            Node *node = _obj->getRepr();
            if (node) {
                node->removeObserver(*this);
            }
        }
    }

    void notifyChildAdded( Node &/*node*/, Node &/*child*/, Node */*prev*/ ) override {}
    void notifyChildRemoved( Node &/*node*/, Node &/*child*/, Node */*prev*/ ) override {}
    void notifyChildOrderChanged( Node &/*node*/, Node &/*child*/, Node */*old_prev*/, Node */*new_prev*/ ) override {}
    void notifyContentChanged( Node &/*node*/, Util::ptr_shared /*old_content*/, Util::ptr_shared /*new_content*/ ) override {}
    void notifyAttributeChanged( Node &/*node*/, GQuark name, Util::ptr_shared /*old_value*/, Util::ptr_shared /*new_value*/ ) override {
        if (_mgr && _obj) {
            if (name == _lockedAttr || name == _labelAttr) {
                _mgr->_objectModified(_obj, 0);
            }
            if (name == _transformAttr) {
                _mgr->_rebuild();
            }
        }
    }

    PageManager* _mgr;
    SPObject* _obj;
    sigc::connection _connection;
    GQuark _lockedAttr;
    GQuark _labelAttr;
    GQuark _transformAttr;
};

PageManager::PageManager(SPDesktop *desktop)
    : _desktop(desktop)
{
    //_page_connection = desktop->connectCurrentPageChanged( sigc::mem_fun(*this, &PageManager::_selectedPageChanged) );
    _resource_connection = desktop->getDocument()->connectResourcesChanged("page", sigc::mem_fun(*this, &PageManager::_rebuild));
    _rebuild();
}

PageManager::~PageManager()
{
    _page_connection.disconnect();
    _resource_connection.disconnect();
    _desktop = nullptr;
}

void PageManager::setCurrentPage( SPObject* obj )
{
    // XXX Redo, this data should be stored locally
    //g_return_if_fail( _desktop->currentRoot() );
    /*if ( _desktop->currentRoot() ) {
        _desktop->setCurrentPage( obj );

        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        if (prefs->getBool("/options/selection/pagedeselect", true)) {
            _desktop->getSelection()->clear();
        }
    }*/
}



void PageManager::_objectModified(SPObject* obj, guint /*flags*/)
{
    _details_changed_signal.emit(obj);
}

void PageManager::_rebuild()
{
    for(auto view : _views) {
        delete view;
    }
    _views.clear();
    _watchers.clear();
    _clear();

    for (auto page : _desktop->getDocument()->getResourceList("page")) {
        addOne(dynamic_cast<SPGroup *>(page));
    }
}

void PageManager::addOne(SPGroup *page)
{
    if (auto clip = page->getClipObject()) {
        if (auto bbox = clip->geometricBounds(page->transform)) {
            _watchers.emplace_back(new PageWatcher(this, page));

            auto rect = (*bbox) * _desktop->getDocument()->getDocumentScale();;
            auto item = new Inkscape::CanvasItemRect(_desktop->getCanvasPages(), rect);
            item->set_fill(0xffffffff);
            item->set_stroke(0x000000cc);
            item->set_shadow(0x00000088, 2);
            item->set_dashed(false);
            item->set_inverted(false); 
            _views.push_back(item);
        }
    }
}

// Connected to the desktop's CurrentPageChanged signal
void PageManager::_selectedPageChanged(SPObject *page)
{
    // notify anyone who's listening to this instead of directly to the desktop
    _page_changed_signal.emit(page);
}

}

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
