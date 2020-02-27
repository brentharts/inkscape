// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Simplify paths (reduce node count).
 *//*
 * Authors:
 * see git history
 *  Created by fred on Fri Dec 05 2003.
 *  tweaked endlessly by bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
#endif

#include <vector>

#include <glibmm/i18n.h>

#include "path-simplify.h"
#include "path-util.h"

#include "message-stack.h"
#include "path-chemistry.h"     // copy_object_properties()
#include "selection.h"

#include "livarot/Path.h"
#include "livarot/Shape.h"

#include "object/sp-flowtext.h"
#include "object/sp-path.h"
#include "object/sp-text.h"

using Inkscape::DocumentUndo;

static bool
sp_selected_path_simplify_items(Inkscape::Selection *selection, std::vector<SPItem*> &items,
                                float threshold,  bool justCoalesce,
                                bool modifySelection);


//return true if we changed something, else false
static bool
sp_selected_path_simplify_item(Inkscape::Selection *selection, SPItem *item,
                               float threshold,  bool justCoalesce,
                               gdouble size,     bool modifySelection)
{
    SPGroup* group = dynamic_cast<SPGroup *>(item);
    SPShape* shape = dynamic_cast<SPShape *>(item);
    SPText*  text  = dynamic_cast<SPText  *>(item);

    if (!group && !shape && !text) {
        return false;
    }

    //If this is a group, do the children instead
    if (group) {
        std::vector<SPItem*> items = sp_item_group_item_list(group);
        
        return sp_selected_path_simplify_items(selection, items, threshold, justCoalesce, false);
    }

    // get path to simplify (note that the path *before* LPE calculation is needed)
    Path *orig = Path_for_item_before_LPE(item, false);
    if (orig == nullptr) {
        return false;
    }

    // correct virtual size by full transform (bug #166937)
    size /= item->i2doc_affine().descrim();

    // save the transform, to re-apply it after simplification
    Geom::Affine const transform(item->transform);

    /*
       reset the transform, effectively transforming the item by transform.inverse();
       this is necessary so that the item is transformed twice back and forth,
       allowing all compensations to cancel out regardless of the preferences
    */
    item->doWriteTransform(Geom::identity());

    // remember the position of the item
    gint pos = item->getRepr()->position();
    // remember parent
    Inkscape::XML::Node *parent = item->getRepr()->parent();
    // remember path effect
    char const *patheffect = item->getRepr()->attribute("inkscape:path-effect");
    
    //If a group was selected, to not change the selection list
    if (modifySelection) {
        selection->remove(item);
    }

    if ( justCoalesce ) {
        orig->Coalesce(threshold * size);
    } else {
        orig->ConvertEvenLines(threshold * size);
        orig->Simplify(threshold * size);
    }

    SPDocument* document = selection->document();
    Inkscape::XML::Document *xml_doc = document->getReprDoc();
    Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");

    // restore attributes
    Inkscape::copy_object_properties(repr, item->getRepr());

    item->deleteObject(false);

    // restore path effect
    repr->setAttribute("inkscape:path-effect", patheffect);

    // path
    gchar *str = orig->svg_dump_path();
    if (patheffect)
        repr->setAttribute("inkscape:original-d", str);
    else 
        repr->setAttribute("d", str);
    g_free(str);

    // add the new repr to the parent
    // move to the saved position
    parent->addChildAtPos(repr, pos);

    SPItem *newitem = (SPItem *) document->getObjectByRepr(repr);

    // reapply the transform
    newitem->doWriteTransform(transform);

    //If we are not in a selected group
    if (modifySelection)
        selection->add(repr);

    Inkscape::GC::release(repr);

    // clean up
    if (orig) delete orig;

    return true;
}


bool
sp_selected_path_simplify_items(Inkscape::Selection *selection, std::vector<SPItem*> &items,
                                float threshold,  bool justCoalesce,
                                bool modifySelection)
{
    SPDesktop  *desktop  = selection->desktop();

    // There is actually no option in the preferences dialog for this!
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool simplifyIndividualPaths = prefs->getBool("/options/simplifyindividualpaths/value");

    gchar *simplificationType;
    if (simplifyIndividualPaths) {
        simplificationType = _("Simplifying paths (separately):");
    } else {
        simplificationType = _("Simplifying paths:");
    }

    bool didSomething = false;

    Geom::OptRect selectionBbox = selection->visualBounds();
    if (!selectionBbox) {
        return false;
    }
    gdouble selectionSize  = L2(selectionBbox->dimensions());

    gdouble simplifySize  = selectionSize;

    int pathsSimplified = 0;
    int totalPathCount  = items.size();

    // set "busy" cursor
    if (desktop) {
        desktop->setWaitingCursor();
    }

    for (auto item : items) {

        SPGroup* group = dynamic_cast<SPGroup *>(item);
        SPShape* shape = dynamic_cast<SPShape *>(item);
        SPText*  text  = dynamic_cast<SPText  *>(item);

        if (!group && !shape && !text) {
            continue;
        }

        if (simplifyIndividualPaths) {
            Geom::OptRect itemBbox = item->documentVisualBounds();
            if (itemBbox) {
                simplifySize      = L2(itemBbox->dimensions());
            } else {
                simplifySize      = 0;
            }
        }

        pathsSimplified++;

        if (pathsSimplified % 20 == 0) {
            gchar *message = g_strdup_printf(_("%s <b>%d</b> of <b>%d</b> paths simplified..."),
                simplificationType, pathsSimplified, totalPathCount);
            if (desktop) {
                desktop->messageStack()->flash(Inkscape::IMMEDIATE_MESSAGE, message);
            } else {
                std::cerr << message << std::endl;
            }
            g_free(message);
        }

        didSomething |= sp_selected_path_simplify_item(selection, item,
            threshold, justCoalesce, simplifySize, modifySelection);
    }

    if (desktop) {
        desktop->clearWaitingCursor();
    }

    if (desktop && pathsSimplified > 20) {
        desktop->messageStack()->flashF(Inkscape::NORMAL_MESSAGE, _("<b>%d</b> paths simplified."), pathsSimplified);
    }

    return didSomething;
}

void
sp_selected_path_simplify(Inkscape::Selection *selection)
{
    SPDesktop  *desktop  = selection->desktop();
    SPDocument *document = selection->document();

    if (!document) {
        std::cerr << "sp_selected_path_simplify: no document!" << std::endl;
        return;
    }

    if (selection->isEmpty()) {
        if (selection->desktop()) {
            desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE,
                                           _("Select <b>path(s)</b> to simplify."));
        } else {
            std::cerr << "sp_selected_path_simplify: empty selection!" << std::endl;
        }
        return;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gdouble threshold = prefs->getDouble("/options/simplifythreshold/value", 0.003);
    bool justCoalesce = prefs->getBool(  "/options/simplifyjustcoalesce/value", false);

    // Keep track of accelerated simplify
    static gint64 previous_time = 0;
    static gdouble multiply = 1.0;

    // Get the current time
    gint64 current_time = g_get_monotonic_time();

    // Was the previous call to this function recent? (<0.5 sec)
    if (previous_time > 0 && current_time - previous_time < 500000) {

        // add to the threshold 1/2 of its original value
        multiply  += 0.5;
        threshold *= multiply;

    } else {
        // reset to the default
        multiply = 1;
    }

    // Remember time for next call
    previous_time = current_time;

    std::vector<SPItem *> items(selection->items().begin(), selection->items().end());

    bool didSomething = sp_selected_path_simplify_items(selection, items, threshold, justCoalesce, true);

    if (didSomething) {
        DocumentUndo::done(document, SP_VERB_SELECTION_SIMPLIFY,  _("Simplify"));
    } else if (desktop) {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>No paths</b> to simplify in the selection."));
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
