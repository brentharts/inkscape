// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape pages implmentation
 *
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2021 Martin Owens
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "attributes.h"

#include "desktop.h"
#include "sp-page.h"
#include "sp-namedview.h"
#include "sp-root.h"

using Inkscape::DocumentUndo;

SPPage::SPPage()
    : SPObject()
{}

void SPPage::build(SPDocument *document, Inkscape::XML::Node *repr)
{
    SPObject::build(document, repr);

    this->readAttr(SPAttr::INKSCAPE_LABEL);
    this->readAttr(SPAttr::X);
    this->readAttr(SPAttr::Y);
    this->readAttr(SPAttr::WIDTH);
    this->readAttr(SPAttr::HEIGHT);

    /* Register */
    document->addResource("page", this);
}

void SPPage::release()
{
    for(auto view : views) {
        delete view;
    }
    this->views.clear();

    if (this->document) {
        // Unregister ourselves
        this->document->removeResource("page", this);
    }

    SPObject::release();
}

void SPPage::set(SPAttr key, const gchar *value) {
    switch (key) {
        case SPAttr::INKSCAPE_LABEL:
            if (value) {
                this->label = std::string(value);
            } else {
                this->label = "";
            }
            break;
        case SPAttr::X:
            this->x.readOrUnset(value);
            break;
        case SPAttr::Y:
            this->y.readOrUnset(value);
            break;
        case SPAttr::WIDTH:
            this->width.readOrUnset(value);
            break;
        case SPAttr::HEIGHT:
            this->height.readOrUnset(value);
            break;
        default:
            SPObject::set(key, value);
            break;
    }
    this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

/**
 * Gets the rectangle in document units
 */
Geom::Rect SPPage::get_rect()
{
    return Geom::Rect(
        this->x.computed, this->y.computed,
        this->x.computed + this->width.computed,
        this->y.computed + this->height.computed
    );
}

void SPPage::showPage(SPDesktop *desktop, Inkscape::CanvasItemGroup *group)
{
    auto rect_b = get_rect() * desktop->d2w();
    auto rect_c = get_rect() * document->doc2dt();
    auto rect = get_rect() * document->getDocumentScale();

    std::cout << "RECT A" << get_rect() << "\n"
                 "RECT B" << rect_b << "\n"
                 "RECT C" << rect_c << "\n";

    auto item = new Inkscape::CanvasItemRect(group, rect);
    item->set_fill(0xffffffff);
    item->set_stroke(0x000000cc);
    item->set_shadow(0x00000088, 2);
    item->set_dashed(false);
    item->set_inverted(false);
    views.push_back(item);
}
  
void SPPage::hidePage(Inkscape::UI::Widget::Canvas *canvas)
{
    g_assert(canvas != nullptr);
    for (auto it = views.begin(); it != views.end(); ++it) {
        if (canvas == (*it)->get_canvas()) {
            delete (*it);
            views.erase(it);
            return;
        }
    }
    assert(false);
}

void SPPage::showPage()
{
    for (auto view : views) {
        view->show();
    }   
}

void SPPage::hidePage()
{
    for(auto view : views) {
        view->hide();
    }
}

void SPPage::setPageColor(guint32 color) {
    // TODO: There may be more related to defaults here.
    for (auto view : views) {
        view->set_fill(color);
    }
}
void SPPage::setPageBorder(guint32 color) {
    for (auto view : views) {
        view->set_stroke(color);
    }
}

void SPPage::setPageShadow(bool show) {
    for (auto view : views) {
        view->set_shadow(0x00000088, show ? 2 : 0);
    }
}

void SPPage::setLabel(const char* label, bool const commit)
{
    // TODO: Add set_label to rectangle canvas control
    if (!views.empty()) {
        //views[0]->set_label(label);
    }
    if (commit) {
        setAttribute("inkscape:label", label);
    }
}

// TODO We need to watch the parent namedview for pagecolor changes.
// or maybe not, as the namedview will actually update us now...

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
