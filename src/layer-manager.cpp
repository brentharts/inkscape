// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::LayerManager
 *
 * Owned by the Desktop, this manager tracks layer objects as
 * distinct entities from groups, providing a comprehensive set
 * of utilities, signals and other Layer based organisation.
 *
 * Refactored from LayerManager and layer-fns in 2021.
 *
 * Copyright 2001 Ximian, Inc.
 *           2002 Lauris Kaplinski <lauris@kaplinski.com>
 *           2006 John Bintz <jcoswell@coswellproductions.org>
 *           2006 MenTaLguY <mental@rydia.net>
 *           2007 Jon A. Cruz <jon@joncruz.org>
 *           2008 Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *           2010 Abhishek Sharma
 *           2021 Martin Owens <doctormo@geek-2.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <set>

#include <sigc++/functors/mem_fun.h>
#include <sigc++/adaptors/hide.h>


#include "desktop.h"
#include "document.h"
#include "gc-finalized.h"
#include "layer-manager.h"
#include "selection.h"

#include "inkgc/gc-managed.h"

#include "object-hierarchy.h"
#include "object/sp-defs.h"
#include "object/sp-root.h"
#include "object/sp-item-group.h"

#include "util/find-last-if.h"
#include "xml/node-observer.h"

namespace Inkscape {


using Inkscape::XML::Node;

class LayerManager::LayerWatcher : public Inkscape::XML::NodeObserver {
public:
    LayerWatcher(LayerManager* mgr, SPObject* obj) :
        _mgr(mgr),
        _obj(obj),
        _lockedAttr(g_quark_from_string("sodipodi:insensitive")),
        _labelAttr(g_quark_from_string("inkscape:label"))
    {
        _connection = _obj->connectModified(sigc::mem_fun(*mgr, &LayerManager::_objectModified));
        _obj->getRepr()->addObserver(*this);
    }

    ~LayerWatcher() override
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
        if ( name == _lockedAttr || name == _labelAttr ) {
            if ( _mgr && _obj ) {
                _mgr->_objectModified( _obj, 0 );
            }
        }
    }

    LayerManager* _mgr;
    SPObject* _obj;
    sigc::connection _connection;
    GQuark _lockedAttr;
    GQuark _labelAttr;
};

LayerManager::LayerManager(SPDesktop *desktop)
    : _desktop(desktop)
    , _document(nullptr)
{
    _layer_hierarchy = std::make_unique<Inkscape::ObjectHierarchy>(nullptr);
    _layer_hierarchy->connectAdded(sigc::mem_fun(*this, &LayerManager::_layer_activated));
    _layer_hierarchy->connectRemoved(sigc::mem_fun(*this, &LayerManager::_layer_deactivated));
    _layer_hierarchy->connectChanged(sigc::mem_fun(*this, &LayerManager::_selectedLayerChanged));
    _document_connection = desktop->connectDocumentReplaced(sigc::mem_fun(this, &LayerManager::_setDocument));
    _setDocument(desktop, desktop->doc());
}

LayerManager::~LayerManager()
{
    _layer_connection.disconnect();
    _activate_connection.disconnect();
    _deactivate_connection.disconnect();
    _document_connection.disconnect();
    _resource_connection.disconnect();
    _document = nullptr;
}

void LayerManager::_setDocument(SPDesktop *, SPDocument *document) {
    _layer_hierarchy->clear();
    _resource_connection.disconnect();
    _document = document;
    if (document) {
        _resource_connection = document->connectResourcesChanged("layer", sigc::mem_fun(*this, &LayerManager::_rebuild));
        _layer_hierarchy->setTop(document->getRoot());
    }
    _rebuild();
}

void LayerManager::_objectModified( SPObject* obj, guint /*flags*/ )
{
    _details_changed_signal.emit( obj );
}

void LayerManager::_layer_activated(SPObject *layer)
{
    if (auto group = dynamic_cast<SPGroup *>(layer)) {
        group->setLayerDisplayMode(_desktop->dkey, SPGroup::LAYER);
    }
}

void LayerManager::_layer_deactivated(SPObject *layer)
{
    if (auto group = dynamic_cast<SPGroup *>(layer)) {
        group->setLayerDisplayMode(_desktop->dkey, SPGroup::GROUP);
    }
}

/**
 * Returns current root (=bottom) layer.
 */
SPGroup *LayerManager::currentRoot() const
{
    return dynamic_cast<SPGroup *>(_layer_hierarchy->top());
}

/**
 * Returns current top layer.
 */
SPGroup *LayerManager::currentLayer() const
{
    return dynamic_cast<SPGroup *>(_layer_hierarchy->bottom());
}

/**
 * Resets the bottom layer to the current root
 */
void LayerManager::reset() {
    _layer_hierarchy->setBottom(currentRoot());
}


/*
 * Return a unique layer name similar to param label
 * A unique name is made by substituting or appending the label's number suffix with
 * the next unique larger number suffix not already used for any layer name
 */
Glib::ustring LayerManager::getNextLayerName( SPObject* obj, gchar const *label)
{
    Glib::ustring incoming( label ? label : "Layer 1" );
    Glib::ustring result(incoming);
    Glib::ustring base(incoming);
    Glib::ustring split(" ");
    guint startNum = 1;

    gint pos = base.length()-1;
    while (pos >= 0 && g_ascii_isdigit(base[pos])) {
        pos-- ;
    }

    gchar* numpart = g_strdup(base.substr(pos+1).c_str());
    if ( numpart ) {
        gchar* endPtr = nullptr;
        guint64 val = g_ascii_strtoull( numpart, &endPtr, 10);
        if ( ((val > 0) || (endPtr != numpart)) && (val < 65536) ) {
            base.erase( pos+1);
            result = incoming;
            startNum = static_cast<int>(val);
            split = "";
        }
        g_free(numpart);
    }

    std::set<Glib::ustring> currentNames;
    std::vector<SPObject *> layers = _document->getResourceList("layer");
    if (currentRoot()) {
        for (auto layer : layers) { 
            if (layer != obj)
                currentNames.insert(layer->label() ? Glib::ustring(layer->label()) : Glib::ustring());
        }
    }

    // Not sure if we need to cap it, but we'll just be paranoid for the moment
    // Intentionally unsigned
    guint endNum = startNum + 3000;
    for ( guint i = startNum; (i < endNum) && (currentNames.find(result) != currentNames.end()); i++ ) {
        result = Glib::ustring::format(base, split, i);
    }

    return result;
}

void LayerManager::renameLayer( SPObject* obj, gchar const *label, bool uniquify )
{
    Glib::ustring incoming( label ? label : "" );
    Glib::ustring result(incoming);

    if (uniquify) {
        result = getNextLayerName(obj, label);
    }

    obj->setLabel( result.c_str() );
}

/**
 * Sets the current layer of the desktop.
 *
 * Make \a object the top layer.
 */
void LayerManager::setCurrentLayer(SPObject *object, bool clear) {
    if (currentRoot()) {
        g_return_if_fail(SP_IS_GROUP(object));
        g_return_if_fail( currentRoot() == object || (currentRoot() && currentRoot()->isAncestorOf(object)) );
        _layer_hierarchy->setBottom(object);

        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        if (clear && prefs->getBool("/options/selection/layerdeselect", true)) {
            _desktop->getSelection()->clear();
        }
    }
}

void LayerManager::toggleHideAllLayers(bool hide) {
    for ( SPObject* obj = Inkscape::previous_layer(currentRoot(), currentRoot()); obj; obj = Inkscape::previous_layer(currentRoot(), obj) ) {
        SP_ITEM(obj)->setHidden(hide);
    }
}
void LayerManager::toggleLockAllLayers(bool lock) {
    for ( SPObject* obj = Inkscape::previous_layer(currentRoot(), currentRoot()); obj; obj = Inkscape::previous_layer(currentRoot(), obj) ) {
        SP_ITEM(obj)->setLocked(lock);
    }
}


void LayerManager::_rebuild() {
//     Debug::EventTracker<DebugLayerRebuild> tracker1();

    _watchers.clear();
    _clear();

    if (!_document || !_desktop)
        return;

    std::vector<SPObject *> layers = _document->getResourceList("layer");

    if (auto root = _desktop->layerManager().currentRoot()) {
        _addOne(root);
        std::set<SPGroup *> layersToAdd;

        for (auto &layer : layers) {
            bool needsAdd = false;
            std::set<SPGroup *> additional;

            if (root->isAncestorOf(layer)) {
                needsAdd = true;
                for (SPObject* curr = layer; curr && (curr != root) && needsAdd; curr = curr->parent) {
                    if (auto group = dynamic_cast<SPGroup *>(curr)) {
                        if (group->isLayer()) {
                            // If we have a layer-group as the one or a parent, ensure it is listed as a valid layer.
                            needsAdd &= ( std::find(layers.begin(),layers.end(),curr) != layers.end() );
			    // XML Tree being used here directly while it shouldn't be...
                            if ( (!(group->getRepr())) || (!(group->getRepr()->parent())) ) {
                                needsAdd = false;
                            }
                        } else {
                            // If a non-layer group is a parent of layer groups, then show it also as a layer.
                            // TODO add the magic Inkscape group mode?
                            // XML Tree being used directly while it shouldn't be...
                            if ( group->getRepr() && group->getRepr()->parent() ) {
                                additional.insert(group);
                            } else {
                                needsAdd = false;
                            }
                        }
                    }
                }
            }
            if (needsAdd) {
                if (!includes(layer)) {
                    layersToAdd.insert(SP_GROUP(layer));
                }
                for (auto it : additional) {
                    if (!includes(it)) {
                        layersToAdd.insert(it);
                    }
                }
            }
        }

        for (auto layer : layersToAdd) {
            // Filter out objects in the middle of being deleted

            // Such may have been the cause of bug 1339397.
            // See http://sourceforge.net/tracker/index.php?func=detail&aid=1339397&group_id=93438&atid=604306

            SPObject const *higher = layer;
            while ( higher && (higher->parent != root) ) {
                higher = higher->parent;
            }
            Node const* node = higher ? higher->getRepr() : nullptr;
            if ( node && node->parent() ) {
                _watchers.emplace_back(new LayerWatcher(this, layer));
                _addOne(layer);
            }
        }
    }
}

static bool is_layer(SPObject &object) {
    return SP_IS_GROUP(&object) &&
           SP_GROUP(&object)->layerMode() == SPGroup::LAYER;
}

// Connected to the desktop's CurrentLayerChanged signal
void LayerManager::_selectedLayerChanged(SPObject *top, SPObject *bottom)
{
    // notify anyone who's listening to this instead of directly to the desktop
    _layer_changed_signal.emit(bottom);
}

/** Finds the next sibling layer for a \a layer
 *
 *  @returns NULL if there are no further layers under a parent
 */
static SPObject *next_sibling_layer(SPObject *layer) {
    if (layer->parent == nullptr) {
      return nullptr;
    }
    SPObject::ChildrenList &list = layer->parent->children;
    auto l = std::find_if(++list.iterator_to(*layer), list.end(), &is_layer);
    return l != list.end() ? &*l : nullptr;
}

/** Finds the previous sibling layer for a \a layer
 *
 *  @returns NULL if there are no further layers under a parent
 */
static SPObject *previous_sibling_layer(SPObject *layer) {
    using Inkscape::Algorithms::find_last_if;

    SPObject::ChildrenList &list = layer->parent->children;
    auto l = find_last_if(list.begin(), list.iterator_to(*layer), &is_layer);
    return l != list.iterator_to(*layer) ? &*(l) : nullptr;
}

/** Finds the first child of a \a layer
 *
 *  @returns the layer itself if layer has no sublayers
 */
static SPObject *first_descendant_layer(SPObject *layer) {
    while (true) {
        auto first_descendant = std::find_if(layer->children.begin(), layer->children.end(), &is_layer);
        if (first_descendant == layer->children.end()) {
            break;
        }
        layer = &*first_descendant;
    }

    return layer;
}

/** Finds the last (topmost) child of a \a layer
 *
 *  @returns NULL if layer has no sublayers
 */
static SPObject *last_child_layer(SPObject *layer) {
    using Inkscape::Algorithms::find_last_if;

    auto l = find_last_if(layer->children.begin(), layer->children.end(), &is_layer);
    return l != layer->children.end() ? &*l : nullptr;
}

static SPObject *last_elder_layer(SPObject *root, SPObject *layer) {
    using Inkscape::Algorithms::find_last_if;
    SPObject *result = nullptr;

    while ( layer != root ) {
        SPObject *sibling(previous_sibling_layer(layer));
        if (sibling) {
            result = sibling;
            break;
        }
        layer = layer->parent;
    }

    return result;
}

/** Finds the next layer under \a root, relative to \a layer in
 *  depth-first order.
 *
 *  @returns NULL if there are no further layers under \a root
 */
SPObject *next_layer(SPObject *root, SPObject *layer) {
    g_return_val_if_fail(layer != nullptr, NULL);
    SPObject *result = nullptr;

    SPObject *sibling = next_sibling_layer(layer);
    if (sibling) {
        result = first_descendant_layer(sibling);
    } else if ( layer->parent != root ) {
        result = layer->parent;
    }

    return result;
}


/** Finds the previous layer under \a root, relative to \a layer in
 *  depth-first order.
 *
 *  @returns NULL if there are no prior layers under \a root.
 */
SPObject *previous_layer(SPObject *root, SPObject *layer) {
    using Inkscape::Algorithms::find_last_if;

    g_return_val_if_fail(layer != nullptr, NULL);
    SPObject *result = nullptr;

    SPObject *child = last_child_layer(layer);
    if (child) {
        result = child;
    } else if ( layer != root ) {
        SPObject *sibling = previous_sibling_layer(layer);
        if (sibling) {
            result = sibling;
        } else {
            result = last_elder_layer(root, layer->parent);
        }
    }

    return result;
}

/**
*  Creates a new layer.  Advances to the next layer id indicated
 *  by the string "layerNN", then creates a new group object of
 *  that id with attribute inkscape:groupmode='layer', and finally
 *  appends the new group object to \a root after object \a layer.
 *
 *  \pre \a root should be either \a layer or an ancestor of it
 */
SPObject *create_layer(SPObject *root, SPObject *layer, LayerRelativePosition position) {
    SPDocument *document = root->document;

    static int layer_suffix=1;
    gchar *id=nullptr;
    do {
        g_free(id);
        id = g_strdup_printf("layer%d", layer_suffix++);
    } while (document->getObjectById(id));

    Inkscape::XML::Document *xml_doc = document->getReprDoc();
    Inkscape::XML::Node *repr = xml_doc->createElement("svg:g");
    repr->setAttribute("inkscape:groupmode", "layer");
    repr->setAttribute("id", id);
    g_free(id);

    if ( LPOS_CHILD == position ) {
        root = layer;
        SPObject *child_layer = Inkscape::last_child_layer(layer);
        if ( nullptr != child_layer ) {
            layer = child_layer;
        }
    }

    if ( root == layer ) {
        root->getRepr()->appendChild(repr);
    } else {
        Inkscape::XML::Node *layer_repr = layer->getRepr();
        layer_repr->parent()->addChild(repr, layer_repr);

        if ( LPOS_BELOW == position ) {
            SP_ITEM(document->getObjectByRepr(repr))->lowerOne();
        }
    }

    return document->getObjectByRepr(repr);
}

/**
 * Toggle the sensitivity of every layer except the given layer.
 */
void LayerManager::toggleLockOtherLayers(SPObject *object, bool force_lock) {
    g_return_if_fail(SP_IS_GROUP(object));
    g_return_if_fail( currentRoot() == object || (currentRoot() && currentRoot()->isAncestorOf(object)) );

    bool othersLocked = false;
    std::vector<SPObject*> layers;
    for ( SPObject* obj = Inkscape::next_layer(currentRoot(), object); obj; obj = Inkscape::next_layer(currentRoot(), obj) ) {
        // Don't lock any ancestors, since that would in turn lock the layer as well
        if (!obj->isAncestorOf(object)) {
            layers.push_back(obj);
            othersLocked |= !SP_ITEM(obj)->isLocked();
        }
    }
    for ( SPObject* obj = Inkscape::previous_layer(currentRoot(), object); obj; obj = Inkscape::previous_layer(currentRoot(), obj) ) {
        if (!obj->isAncestorOf(object)) {
            layers.push_back(obj);
            othersLocked |= !SP_ITEM(obj)->isLocked();
        }
    }

    SPItem *item = SP_ITEM(object);
    if ( item->isLocked() ) {
        item->setLocked(false);
    }

    if (force_lock) {
        othersLocked = true;
    }

    for (auto & layer : layers) {
        SP_ITEM(layer)->setLocked(othersLocked);
    }
}

/**
 * Toggle the visibility of every layer except the given layer.
 */
void LayerManager::toggleLayerSolo(SPObject *object, bool force_hide) {
    g_return_if_fail(SP_IS_GROUP(object));
    g_return_if_fail( currentRoot() == object || (currentRoot() && currentRoot()->isAncestorOf(object)) );

    bool othersShowing = false;
    std::vector<SPObject*> layers;
    for ( SPObject* obj = Inkscape::next_layer(currentRoot(), object); obj; obj = Inkscape::next_layer(currentRoot(), obj) ) {
        // Don't hide ancestors, since that would in turn hide the layer as well
        if (!obj->isAncestorOf(object)) {
            layers.push_back(obj);
            othersShowing |= !SP_ITEM(obj)->isHidden();
        }
    }
    for ( SPObject* obj = Inkscape::previous_layer(currentRoot(), object); obj; obj = Inkscape::previous_layer(currentRoot(), obj) ) {
        if (!obj->isAncestorOf(object)) {
            layers.push_back(obj);
            othersShowing |= !SP_ITEM(obj)->isHidden();
        }
    }

    SPItem *item = SP_ITEM(object);
    if ( item->isHidden() ) {
        item->setHidden(false);
    }

    if (force_hide) {
        othersShowing = true;
    }

    for (auto & layer : layers) {
        SP_ITEM(layer)->setHidden(othersShowing);
    }
}

/**
 * Return layer that contains \a object.
 */
SPObject *LayerManager::layerForObject(SPObject *object) {
    g_return_val_if_fail(object != nullptr, NULL);
    if (isLayer(object)) {
        return object;
    }

    SPObject *root=currentRoot();
    object = object->parent;
    while ( object && object != root && !isLayer(object) ) {
        // Objects in defs have no layer and are NOT in the root layer
        if(SP_IS_DEFS(object))
            return nullptr;
        object = object->parent;
    }
    return object;
}

/**
 * True if object is a layer.
 */
bool LayerManager::isLayer(SPObject *object) const
{
    if (auto group = dynamic_cast<SPGroup *>(object)) {
        return group->effectiveLayerMode(_desktop->dkey) == SPGroup::LAYER;
    }
    return false;
}

/**
 * Return the SPGroup if we have a layer object.
 */
SPGroup *LayerManager::asLayer(SPObject *object)
{
    if (auto group = dynamic_cast<SPGroup *>(object)) {
        return group->isLayer() ? group : nullptr;
    }
    return nullptr;
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
