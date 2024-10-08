// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_GRADIENT_VECTOR_H
#define SEEN_SP_GRADIENT_VECTOR_H

#include <vector>
#include "colors/color.h"

/**
 * Differs from SPStop in that SPStop mirrors the \<stop\> element in the document, whereas
 * SPGradientStop shows more the effective stop color.
 *
 * For example, SPGradientStop has no currentColor option: currentColor refers to the color
 * property value of the gradient where currentColor appears, so we interpret currentColor before
 * copying from SPStop to SPGradientStop.
 */
struct SPGradientStop {
    std::optional<Inkscape::Colors::Color> color;
    double offset;
};

/**
 * The effective gradient vector, after copying stops from the referenced gradient if necessary.
 */
struct SPGradientVector {
    bool built;
    std::vector<SPGradientStop> stops;
};

#endif /* !SEEN_SP_GRADIENT_VECTOR_H */

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
