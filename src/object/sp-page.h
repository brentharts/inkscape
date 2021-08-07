// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * SPPage -- a guideline
 *//*
 * Authors:
 *   Martin Owens 2021
 * 
 * Copyright (C) 2021 Martin Owens
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_PAGE_H
#define SEEN_SP_PAGE_H

#include <2geom/point.h>
#include <vector>

#include "sp-object.h"

class SPDesktop;

class SPPage : public SPObject {
public:
    SPPage();
    ~SPPage() override = default;

protected:

private:
};

#endif // SEEN_SP_PAGE_H

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
