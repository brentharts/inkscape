#ifndef __SP_MARKER_CONTEXT_H__
#define __SP_MARKER_CONTEXT_H__

#include "ui/tools/node-tool.h"
#include "object/sp-marker-loc.h"

namespace Inkscape {
namespace UI {
namespace Tools {

class MarkerTool : public NodeTool {
	public:
		MarkerTool();

	private:
        bool enter_marker_mode = false;

		void selection_changed(Inkscape::Selection *sel) override;
        bool root_handler_extended(GdkEvent* event) override;
		void finish() override;

		Geom::Affine get_marker_transform(SPShape* shape, SPItem *parent_item, SPItem *item, SPMarkerLoc marker_type);
};

}
}
}

#endif

