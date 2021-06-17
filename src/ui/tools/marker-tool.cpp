/***************
-   SPDX-License-Identifier: GPL-2.0-or-later
-   Released under GNU GPL v2+, read the file 'COPYING' for more information.

-   Marker Editing Context
****************/

#include <cstring>
#include <string>

#include <gdk/gdkkeysyms.h>
#include <glibmm/i18n.h>
#include "context-fns.h"
#include "desktop-style.h"
#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "include/macros.h"
#include "message-context.h"
#include "selection-chemistry.h"
#include "selection.h"
#include "verbs.h"
#include "object/sp-namedview.h"
#include "ui/shape-editor.h"
#include "xml/node-event-vector.h"

#include "ui/tool/shape-record.h"
#include "object/sp-shape.h"
#include "object/object-set.h"
#include "object/sp-marker.h"
#include "ui/tools/marker-tool.h"


using Inkscape::DocumentUndo;
namespace Inkscape {
namespace UI {
namespace Tools {

const std::string MarkerTool::prefsPath = "/tools/marker";

MarkerTool::MarkerTool()
    : ToolBase("select.svg")
{}

const std::string& MarkerTool::getPrefsPath() {
	return MarkerTool::prefsPath;
}

void MarkerTool::finish() {
    ungrabCanvasEvents();
    this->message_context->clear();
    this->sel_changed_connection.disconnect();
    ToolBase::finish();
}

MarkerTool::~MarkerTool() {
    this->enableGrDrag(false);
    this->sel_changed_connection.disconnect();
}

/* 
Recursively collect all the child objects from the top level marker object 
to be editable by the shape editor

~ Todo:
 - add support for objectBoundingBox
 - fix the edit_transform
 - follow up on clipping, editing masks thing
*/
static void gather_marker_items(SPObject *obj, Inkscape::UI::ShapeRole role, std::set<Inkscape::UI::ShapeRecord> &shapes) {
    
    using namespace Inkscape::UI;
    if (!obj) return;

    if (SP_IS_GROUP(obj) || SP_IS_OBJECTGROUP(obj) || SP_IS_MARKER(obj)) {
        for (auto& c: obj->children) {
            gather_marker_items(&c, role, shapes);
        }
    } else if (SP_IS_ITEM(obj)) {
        SPObject *object = obj;
        SPItem *item = dynamic_cast<SPItem *>(obj);

        ShapeRecord sr;
        sr.object = object;
        sr.edit_transform = Geom::identity();
        sr.role = role;
        shapes.insert(sr);
    }
}

/* 
Get the selected objects and passes their marker items, if they have any, into
the shape editor for editing.

~ Todo:
 - allow users to only edit markers on one object at a time?
*/
void MarkerTool::selection_changed(Inkscape::Selection* selection) {
    
    using namespace Inkscape::UI;
    std::set<ShapeRecord> shapes;
    auto selected_items = this->desktop->getSelection()->items();

    for(auto i = selected_items.begin(); i != selected_items.end(); ++i){
        SPItem *item = *i;
        
        if (item) {
            SPShape* shape = dynamic_cast<SPShape*>(item);

            if(shape && shape->hasMarkers()) {
                for(int i = 0; i < SP_MARKER_LOC_QTY; i++) {
                    SPObject *marker_obj = shape->_marker[i];
                    if(marker_obj) {
                        Inkscape::XML::Node *marker_repr = marker_obj->getRepr();
                        SPItem* marker_item = dynamic_cast<SPItem *>(desktop->getDocument()->getObjectByRepr(marker_repr));
                        gather_marker_items(marker_item, SHAPE_ROLE_NORMAL, shapes);
                    }
                }
            }
        }
    }

    for (auto i = this->_shape_editors.begin(); i != this->_shape_editors.end();) {
        ShapeRecord sr;
        sr.object = dynamic_cast<SPObject *>(i->first);

        if (shapes.find(sr) == shapes.end()) {
            this->_shape_editors.erase(i++);
        } else {
            ++i;
        }
    }

    for (const auto & shape : shapes) {
        if (this->_shape_editors.find(SP_ITEM(shape.object)) == this->_shape_editors.end()) {
            auto si = std::make_unique<ShapeEditor>(this->desktop, shape.edit_transform);
            SPItem *item = SP_ITEM(shape.object);
            si->set_item(item);
            this->_shape_editors.insert({item, std::move(si)});
        }
    }
}

void MarkerTool::setup() {

    ToolBase::setup();
    Inkscape::Selection *selection = this->desktop->getSelection();

    this->sel_changed_connection.disconnect();
    this->sel_changed_connection = selection->connectChanged(
    	sigc::mem_fun(this, &MarkerTool::selection_changed)
    );

    this->selection_changed(selection);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/tools/marker/selcue")) this->enableSelectionCue();
    if (prefs->getBool("/tools/marker/gradientdrag")) this->enableGrDrag();
}

/* 
Handles selection/deselection of new items in marker edit mode
*/
bool MarkerTool::root_handler(GdkEvent* event) {
    SPDesktop *desktop = this->desktop;
    Inkscape::Selection *selection = desktop->getSelection();
    gint ret = false;
    
    switch (event->type) {
        case GDK_BUTTON_PRESS:
            if (event->button.button == 1) {
                Geom::Point const button_w(event->button.x, event->button.y);  
                this->item_to_select = sp_event_context_find_item (desktop, button_w, event->button.state & GDK_MOD1_MASK, TRUE);
                grabCanvasEvents();
                ret = true;
            }
            break;
        case GDK_BUTTON_RELEASE:
            if (event->button.button == 1) {
                if (this->item_to_select) {
                    selection->toggle(this->item_to_select);
                } else {
                    selection->clear();
                }
                this->item_to_select = nullptr;
                ungrabCanvasEvents();
                ret = true;
            }
            break;
        default:
            break;
    }

    if (!ret) ret = ToolBase::root_handler(event);
    return ret;
}

}}}