// SPDX-License-Identifier: GPL-2.0-or-later
// Released under GNU GPL v2+, read the file 'COPYING' for more information.

#ifndef SEEN_MARKER_TOOLBAR_H
#define SEEN_MARKER_TOOLBAR_H

#include "toolbar.h"

namespace Inkscape {
namespace UI {
namespace Toolbar {

class MarkerToolbar : public Toolbar {
protected:
    MarkerToolbar(SPDesktop *desktop);
public:
    static GtkWidget * create(SPDesktop *desktop);
};

}}}
#endif