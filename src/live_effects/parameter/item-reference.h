// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_LPE_ITEM_REFERENCE_H
#define SEEN_LPE_ITEM_REFERENCE_H

/*
 * Copyright (C) 2008-2012 Authors
 * Authors: Johan Engelen
 *          Abhishek Sharma
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "object/uri-references.h"
#include "object/sp-object.h"
#include <glib.h>
#include <iostream>

class SPItem;
namespace Inkscape {

namespace XML { class Node; }

namespace LivePathEffect {

/**
 * The reference corresponding to href of LPE ItemParam.
 */
class ItemReference : public Inkscape::URIReference {
public:
    ItemReference(SPObject *owner) : URIReference(owner) {}

    SPItem *getObject() const {
        return (SPItem *)URIReference::getObject();
    }

protected:
    bool _acceptObject(SPObject * const obj) const override;

private:
    ItemReference(const ItemReference&) = delete;
    ItemReference& operator=(const ItemReference&) = delete;
};

class LPEItemRef : public Inkscape::URIReference {
public:
    LPEItemRef(SPObject *owner) : URIReference(owner) {};
    ~LPEItemRef() override {
        linked_delete_connection.disconnect();
        linked_modified_connection.disconnect();
        linked_transformed_connection.disconnect();
        linked_changed_connection.disconnect();
        owner_release_connection.disconnect();
        if (isAttached()) {
            detach();
        }
        if (href) {
            g_free(href);
            href = nullptr;
        }    
    };
    sigc::connection owner_release_connection = getOwner()->connectRelease([this](SPObject *obj) {
        // Fully detach to prevent reconnecting with a modified signal
        owner_release_connection.disconnect();
        if (isAttached()) {
            detach();
        }
    });
    sigc::connection linked_changed_connection;
    sigc::connection linked_delete_connection;
    sigc::connection linked_modified_connection;
    sigc::connection linked_transformed_connection;
    gchar *href;
private:
    LPEItemRef(const LPEItemRef&) = delete;
    LPEItemRef& operator=(const LPEItemRef&) = delete;
};

} // namespace LivePathEffect

} // namespace Inkscape



#endif /* !SEEN_LPE_PATH_REFERENCE_H */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
