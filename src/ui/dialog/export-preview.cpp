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

#include <glibmm/i18n.h>
#include <glibmm/main.h>
#include <glibmm/timer.h>
#include <gtkmm.h>

#include "display/cairo-utils.h"
#include "object/sp-namedview.h"
#include "object/sp-root.h"
#include "preview-util.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

ExportPreview::ExportPreview()
    : drawing(nullptr)
    , visionkey(0)
    , timer(nullptr)
    , renderTimer(nullptr)
    , pending(false)
    , minDelay(0.1)
    , size(128)
{
    pixMem = nullptr;
    image = nullptr;
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, size);
    pixMem = new guchar[size * stride];
    memset(pixMem, 0x00, size * stride);

    auto pb = Gdk::Pixbuf::create_from_data(pixMem, Gdk::COLORSPACE_RGB, true, 8, size, size, stride);
    image = Gtk::make_managed<Gtk::Image>(pb);
    image->show();
    // add this image to box here
    this->pack_start(*image, true, true, 0);
    show();
    show_all_children();
}

ExportPreview::~ExportPreview()
{
    if (drawing) {
        if (_document) {
            _document->getRoot()->invoke_hide(visionkey);
        }
        delete drawing;
        drawing = nullptr;
    }
    if (timer) {
        timer->stop();
        delete timer;
        timer = nullptr;
    }
    if (renderTimer) {
        renderTimer->stop();
        delete renderTimer;
        renderTimer = nullptr;
    }
}

void ExportPreview::setItem(SPItem *item)
{
    _item = item;
}
void ExportPreview::setDbox(double x0, double x1, double y0, double y1)
{
    if (!_document) {
        return;
    }
    _dbox = Geom::Rect(Geom::Point(x0, y0), Geom::Point(x1, y1)) * _document->dt2doc();
}

void ExportPreview::setDocument(SPDocument *document)
{
    if (_document == document) {
        return;
    }
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

void ExportPreview::queueRefresh()
{
    if (!pending) {
        pending = true;
        if (!timer) {
            timer = new Glib::Timer();
        }
        Glib::signal_idle().connect(sigc::mem_fun(this, &ExportPreview::refreshCB), Glib::PRIORITY_DEFAULT_IDLE);
    }
}

bool ExportPreview::refreshCB()
{
    bool callAgain = true;
    if (!timer) {
        timer = new Glib::Timer();
    }
    if (timer->elapsed() > minDelay) {
        callAgain = false;
        refreshPreview();
        pending = false;
    }
    return callAgain;
}

void ExportPreview::refreshPreview()
{
    auto document = _document;
    if (!timer) {
        timer = new Glib::Timer();
    }
    if (timer->elapsed() < minDelay) {
        // Do not refresh too quickly
        queueRefresh();
    } else if (document) {
        renderPreview();
        timer->reset();
    }
}

/*
This is main function which finally render preview. Call this after setting document, item and dbox.
If dbox is given it will use it.
if item is given and not dbox then item is used
If both are not given then simply we do nothing.
*/
void ExportPreview::renderPreview()
{
    if (!renderTimer) {
        renderTimer = new Glib::Timer();
    }
    renderTimer->reset();

    if (_document) {
        unsigned unused;
        int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, size);
        guchar *px = nullptr;

        if (_dbox) {
            px = Inkscape::UI::PREVIEW::sp_icon_doc_icon(_document, *drawing, nullptr, size, unused, &_dbox);
        } else if (_item) {
            gchar const *id = _item->getId();
            px = Inkscape::UI::PREVIEW::sp_icon_doc_icon(_document, *drawing, id, size, unused);
        } else {
            std::cout << "Not me" << std::endl;
        }

        if (px) {
            memcpy(pixMem, px, size * stride);
            g_free(px);
            std::cout << "Image Rendered" << std::endl;
            px = nullptr;
        } else {
            std::cout << "Image Rendered NOT" << std::endl;
            memset(pixMem, 0, size * stride);
        }
        image->set(image->get_pixbuf());
        image->show();
    }

    renderTimer->stop();
    minDelay = std::max(0.1, renderTimer->elapsed() * 3.0);
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