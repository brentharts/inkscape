// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_PAGE_TOOLBAR_H
#define SEEN_PAGE_TOOLBAR_H

/**
 * @file
 * Page toolbar
 */
/* Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2021 Martin Owens
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "toolbar.h"

namespace Inkscape {
namespace UI {
namespace Toolbar {

class PageToolbar {
    public:
       static GtkWidget * create(SPDesktop *desktop);
    protected:
        PageToolbar(SPDesktop *desktop) {};
};

}
}
}

#endif /* !SEEN_PAGE_TOOLBAR_H */

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
