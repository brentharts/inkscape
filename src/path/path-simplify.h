// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Simplify paths (reduce node count).
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef PATH_SIMPLIFY_H
#define PATH_SIMPLIFY_H

namespace Inkscape {
  class Selection;
} // Inkscape

void sp_selected_path_simplify (Inkscape::Selection *selection);

#endif // PATH_SIMPLIFY_H

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
