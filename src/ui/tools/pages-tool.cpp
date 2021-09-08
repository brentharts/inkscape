// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Page editing tool
 *
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include <gdk/gdkkeysyms.h>

#include "pages-tool.h"

#include "desktop.h"
#include "rubberband.h"
#include "selection-chemistry.h"

#include "include/macros.h"

namespace Inkscape {
namespace UI {
namespace Tools {

const std::string& PagesTool::getPrefsPath() {
	return PagesTool::prefsPath;
}

const std::string PagesTool::prefsPath = "/tools/pages";

PagesTool::PagesTool()
    : ToolBase("select.svg")
    , escaped(false)
{
}

PagesTool::~PagesTool() = default;

void PagesTool::finish() {
    this->enableGrDrag(false);

    ungrabCanvasEvents();

    ToolBase::finish();
}

void PagesTool::setup() {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    ToolBase::setup();
}

bool PagesTool::root_handler(GdkEvent* event) {
    return false;
}

}
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
