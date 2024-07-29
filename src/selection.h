// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   MenTaLguY <mental@rydia.net>
 *   bulia byak <buliabyak@users.sf.net>
 *   Adrian Boguszewski
 *
 * Copyright (C) 2016 Adrian Boguszewski
 * Copyright (C) 2004-2005 MenTaLguY
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_SELECTION_H
#define SEEN_INKSCAPE_SELECTION_H

#include <cstddef>
#include <list>
#include <unordered_map>
#include <utility>
#include <vector>
#include <sigc++/signal.h>
#include <sigc++/slot.h>

#include "helper/auto-connection.h"
#include "object/object-set.h"

namespace Inkscape {

namespace XML {
class Node;
} // namespace XML

/**
 * The set of selected SPObjects for a given document and layer model.
 *
 * This class represents the set of selected SPItems for a given
 * document.
 *
 * An SPObject and its parent cannot be simultaneously selected;
 * selecting an SPObjects has the side-effect of unselecting any of
 * its children which might have been selected.
 *
 * This is a per-desktop object that keeps the list of selected objects
 * at the given desktop. Both SPItem and SPRepr lists can be retrieved
 * from the selection. Many actions operate on the selection, so it is
 * widely used throughout the code.
 * It also implements its own asynchronous notification signals that
 * UI elements can listen to.
 */
class Selection : public ObjectSet
{
public:
    /**
     * Constructs an selection object, bound to a particular
     * layer model
     *
     * @param layers the layer model (for the SPDesktop, if GUI)
     * @param desktop the desktop associated with the layer model, or NULL if in console mode
     */
    Selection(SPDesktop *desktop);
    Selection(SPDocument *document);
    ~Selection() override;

    /**
     * Returns active layer for selection (currentLayer or its parent).
     *
     * @return layer item the selection is bound to
     */
    SPObject *activeContext();

    using ObjectSet::add;

    /**
     * Add an XML node's SPObject to the set of selected objects.
     *
     * @param the xml node of the item to add
     */
    void add(XML::Node *repr) {
        add(_objectForXMLNode(repr));
    }

     using ObjectSet::set;

    /**
     * Set the selection to an XML node's SPObject.
     *
     * @param repr the xml node of the item to select
     */
    void set(XML::Node *repr) {
        set(_objectForXMLNode(repr));
    }

    using ObjectSet::remove;

    /**
     * Removes an item from the set of selected objects.
     *
     * It is ok to call this method for an unselected item.
     *
     * @param repr the xml node of the item to remove
     */
    void remove(XML::Node *repr) {
        remove(_objectForXMLNode(repr));
    }

    using ObjectSet::includes;

    /**
     * Returns true if the given item is selected.
     */
    bool includes(XML::Node *repr, bool anyAncestor = false) {
        return includes(_objectForXMLNode(repr), anyAncestor);
    }

    using ObjectSet::includesAncestor;
    
    /**
     * Returns ancestor if the given object has ancestor selected.
     */
    SPObject * includesAncestor(XML::Node *repr) {
        return includesAncestor(_objectForXMLNode(repr));
    }

    /** Returns the number of layers in which there are selected objects. */
    size_t numberOfLayers();

    /** Returns the number of parents to which the selected objects belong. */
    size_t numberOfParents();

    /**
     * Compute the list of points in the selection that are to be considered for snapping from.
     *
     * @return Selection's snap points
     */
    std::vector<Inkscape::SnapCandidatePoint> getSnapPoints(SnapPreferences const *snapprefs) const;

    // Fixme: Hack should not exist, but used by live_effects.
    void emitModified() { _emitModified(_flags); };

    /**
     * Connects a slot to be notified of selection changes.
     *
     * This method connects the given slot such that it will
     * be called upon any change in the set of selected objects.
     *
     * @param slot the slot to connect
     *
     * @return the resulting connection
     */
    sigc::connection connectChanged     (sigc::slot<void (Selection *)> slot);
    sigc::connection connectChangedFirst(sigc::slot<void (Selection *)> slot);

    /**
     * Set the anchor point of the selection, used for telling it how transforms
     * should be anchored against.
     * @param x, y - Coordinates for the anchor between 0..1 of the bounding box
     * @param set - If set to false, causes the anchor to become unset (default)
     */
    void setAnchor(double x, double y, bool set = true);
    // Allow the selection to specify a facus anchor (helps with transforming against this point)
    bool has_anchor = false;
    Geom::Point anchor;

    /**
     * Connects a slot to be notified of selected object modifications.
     *
     * This method connects the given slot such that it will
     * receive notifications whenever any selected item is
     * modified.
     *
     * @param slot the slot to connect
     *
     * @return the resulting connection
     *
     */
    sigc::connection connectModified     (sigc::slot<void (Selection *, unsigned)> slot);
    sigc::connection connectModifiedFirst(sigc::slot<void (Selection *, unsigned)> slot);

    /**
     * Set a backup of current selection and store it also to be command line readable by extension system
     */
    void setBackup();
    /**
     * Clear backup of current selection
     */
    void emptyBackup();
    /**
     * Restore a selection from a existing backup
     */
    void restoreBackup();
    /**
     * Here store a paramlist when set backup
     */
    std::list<std::string> params;

    /**
     * Decide if the selection changing should change the layer and page selection too
     */
    void setChangeLayer(bool option) { _change_layer = option; }
    void setChangePage(bool option) { _change_page = option; }

protected:
    void _connectSignals(SPObject* object) override;
    void _releaseSignals(SPObject* object) override;

private:
    /** Issues modification notification signals. */
    static int _emit_modified(Selection *selection);
    /** Schedules an item modification signal to be sent. */
    void _schedule_modified(SPObject *obj, unsigned int flags);

    /** Issues modified selection signal. */
    void _emitModified(unsigned int flags);
    /** Issues changed selection signal. */
    void _emitChanged(bool persist_selection_context = false) override;
    /** returns the SPObject corresponding to an xml node (if any). */
    SPObject *_objectForXMLNode(XML::Node *repr) const;
    /** Releases an active layer object that is being removed. */
    void _releaseContext(SPObject *obj);

    SPObject* _selection_context = nullptr;
    unsigned _flags = 0;
    unsigned _idle = 0;
    bool _change_layer = true;
    bool _change_page = true;
    std::vector<std::pair<std::string, std::pair<int, int>>> _seldata;
    std::vector<std::string> _selected_ids;
    std::unordered_map<SPObject *, auto_connection> _modified_connections;
    auto_connection _context_release_connection;

    std::list<sigc::signal<void (Selection *)>> _changed_signals;
    std::list<sigc::signal<void (Selection *, unsigned int)>> _modified_signals;
};

} // namespace Inkscape

#endif // SEEN_INKSCAPE_SELECTION_H

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
