/***************
-   SPDX-License-Identifier: GPL-2.0-or-later
-   Released under GNU GPL v2+, read the file 'COPYING' for more information.

-   Marker Editing Context
****************/

#ifndef __SP_MARKER_CONTEXT_H__
#define __SP_MARKER_CONTEXT_H__

#include <cstddef>
#include <sigc++/sigc++.h>
#include <2geom/point.h>
#include "ui/tools/tool-base.h"

namespace Inkscape {
    namespace UI {
        class MultiPathManipulator;
        class ControlPointSelection;
        class ControlPoint;

        struct PathSharedData;
    }
}


namespace Inkscape {
class Selection;
namespace UI {
namespace Tools {

class MarkerTool : public ToolBase {
	public:
		MarkerTool();
		~MarkerTool() override;

		void setup() override;
		void finish() override;
		bool root_handler(GdkEvent* event) override;
		const std::string& getPrefsPath() override;

		static const std::string prefsPath;
		std::map<SPItem *, std::unique_ptr<ShapeEditor>> _shape_editors;

		Inkscape::UI::ControlPointSelection* _selected_nodes = nullptr;
		Inkscape::UI::MultiPathManipulator* _multipath = nullptr;
		
	private:
		sigc::connection sel_changed_connection;
		void selection_changed(Inkscape::Selection* selection);

		Inkscape::UI::PathSharedData* _path_data = nullptr;
		Inkscape::CanvasItemGroup *_transform_handle_group = nullptr;
};

}}}

#endif
