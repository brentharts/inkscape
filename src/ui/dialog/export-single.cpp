// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *
 * Copyright (C) 1999-2007 Authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "export-single.h"

#include <gtkmm.h>

#include "export-helper.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

enum sb_type
{
    SPIN_X0 = 0,
    SPIN_X1,
    SPIN_Y0,
    SPIN_Y1,
    SPIN_WIDTH,
    SPIN_HEIGHT,
    SPIN_BMWIDTH,
    SPIN_BMHEIGHT,
    SPIN_DPI
};

enum selection_mode
{
    SELECTION_PAGE = 0, // Default is alaways placed first
    SELECTION_SELECTION,
    SELECTION_DRAWING,
    SELECTION_CUSTOM,
};

void SingleExport::initialise(Glib::RefPtr<Gtk::Builder> *builder)
{
    ;
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

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