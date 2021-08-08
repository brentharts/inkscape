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

#ifndef SP_EXPORT_PREVIEW_H
#define SP_EXPORT_PREVIEW_H

#include <gtkmm.h>

#include "desktop.h"
#include "document.h"

class SPObject;
namespace Glib {
class Timer;
}

namespace Inkscape {
class Drawing;
namespace UI {
namespace Dialog {

class ExportPreview : public Gtk::Box
{
public:
    ExportPreview(Glib::ustring id);
    ExportPreview(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade)
        : Gtk::Box(cobject){};
    ~ExportPreview() override;

private:
    Glib::ustring targetId;

    SPDocument *_document = nullptr;
    SPDesktop *_desktop = nullptr;

    Drawing *drawing;
    unsigned int visionkey;
    Glib::Timer *timer;
    Glib::Timer *renderTimer;
    bool pending;
    gdouble minDelay;

    int size; // size of preview image

    guchar *pixMem;    // Our Itme pixels
    Gtk::Image *image; // Our Item Image

public:
    void setDocument(SPDocument *document);
    void setDesktop(SPDesktop *desktop);
    void refreshPreview();

private:
    void renderPreview(SPObject *obj);
    void queueRefresh();
    bool refreshCB();
};
} // namespace Dialog
} // namespace UI
} // namespace Inkscape
#endif

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