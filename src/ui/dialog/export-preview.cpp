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

#include "export-preview.h"

#include <gtkmm.h>

namespace Inkscape {
namespace UI {
namespace Dialog {

ExportPreview::ExportPreview()
    , drawing(nullptr)
    , visionkey(0)
    , timer(nullptr)
    , renderTimer(nullptr)
    , pending(false)
    , minDelay(0.1)
    , targetId()
    , size(128)
{
    ;
}

void ExportPreview::setDocument(SPDocumet *document)
{
    _document = document;
    if (drawing) {
        if (_document) {
            _document->getRoot()->invoke_hide(visionkey);
        }
        delete drawing;
        drawing = nullptr;
    }
    if (_document) {
        drawing = new Inkscape::Drawing();
        visionkey = SPItem::display_key_new(1);
        drawing->setRoot(_document->getRoot()->invoke_show(*drawing, visionkey, SP_ITEM_SHOW_DISPLAY));
        queueRefresh();
    }
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