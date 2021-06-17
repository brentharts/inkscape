/*
	Marker Editing Context
	Released under GNU GPL v2+, read the file 'COPYING' for more information.
*/

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
#include "object/sp-rect.h"
#include "object/sp-namedview.h"
#include "ui/shape-editor.h"
#include "xml/node-event-vector.h"

#include "object/sp-marker.h"
#include "ui/tools/rect-tool.h"
#include "ui/tools/marker-tool.h"
#include "object/sp-shape.h"
#include "object/object-set.h"
#include "document.h"
#include "ui/tool/shape-record.h"


using Inkscape::DocumentUndo;
namespace Inkscape {
namespace UI {
namespace Tools {

const std::string& MarkerTool::getPrefsPath() {
	return MarkerTool::prefsPath;
}

const std::string MarkerTool::prefsPath = "/tools/marker";

MarkerTool::MarkerTool()
    : ToolBase("select.svg")
{}

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


void MarkerTool::selection_changed(Inkscape::Selection* selection) {
    using namespace Inkscape::UI;

    std::set<ShapeRecord> shapes;

    auto items = this->desktop->getSelection()->items();

    for(auto i=items.begin();i!=items.end();++i){
        SPItem *item = *i;
        if (item) {
            SPShape* shape = dynamic_cast<SPShape*>(item);

            if(shape && shape->hasMarkers()) {
                Inkscape::XML::Node* repr = shape->_marker[SP_MARKER_LOC_START]->getRepr()->firstChild();
                if(repr) {
                    item = dynamic_cast<SPItem *>(desktop->getDocument()->getObjectByRepr(repr));
                    if (SP_IS_ITEM(item)) {
                        ShapeRecord r;
                        r.object = item;
                        // TODO - add support for objectBoundingBox
                        // TODO - fix the edit_transform
                        r.edit_transform = Geom::identity();
                        r.role = SHAPE_ROLE_NORMAL;
                        shapes.insert(r);
                    }
                }
            }
        }
    }

    for (auto i = this->_shape_editors.begin(); i != this->_shape_editors.end();) {
        ShapeRecord s;
        s.object = dynamic_cast<SPObject *>(i->first);

        if (shapes.find(s) == shapes.end()) {
            this->_shape_editors.erase(i++);
        } else {
            ++i;
        }
    }

    for (const auto & r : shapes) {
        if (this->_shape_editors.find(SP_ITEM(r.object)) == this->_shape_editors.end()) {
            auto si = std::make_unique<ShapeEditor>(this->desktop, r.edit_transform);
            SPItem *item = SP_ITEM(r.object);
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

/* handles selection of items */
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