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

#include "inkscape.h"
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

void SPPage::setManager(Inkscape::PageManager *manager)
{
    if (_manager != manager) {
        if (manager && _manager) {
            g_warning("Overwriting page manager for %s!", this->getId());
        }
        _manager = manager;
    }
}

/**
 * Gets the rectangle in document units
 */
Geom::Rect SPPage::getRect() const
{
    return Geom::Rect(
        this->x.computed, this->y.computed,
        this->x.computed + this->width.computed,
        this->y.computed + this->height.computed
    );
}

/**
 * Get the rectangle of the page, scaled to the document.
 */
Geom::Rect SPPage::getDesktopRect() const
{
    return getRect() * document->getDocumentScale();
}

/**
 * Get the items which are ONLY on this page and don't overlap.
 */
std::vector<SPItem*> SPPage::getExclusiveItems() const
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    return document->getItemsInBox(desktop->dkey, getDesktopRect(), true, true, true, false);
}

/**
 * Get all the items which are inside or overlapping.
 */
std::vector<SPItem*> SPPage::getOverlappingItems() const
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    return document->getItemsPartiallyInBox(desktop->dkey, getDesktopRect(), true, true, true, false);
}

/**
 * Shows the page in the given canvas item group.
 */
void SPPage::showPage(SPDesktop *desktop, Inkscape::CanvasItemGroup *group)
{
    auto item = new Inkscape::CanvasItemRect(group, getDesktopRect());
    item->set_fill(0xffffffff);
    item->set_stroke(0x000000cc);
    item->set_shadow(0x00000088, 2);
    item->set_dashed(false);
    item->set_inverted(false);
    views.push_back(item);
}

/**
 * Hide the page in the given canvas widget.
 */
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

void SPPage::setPageColor(guint32 color)
{
    // TODO: There may be more related to defaults here.
    for (auto view : views) {
        view->set_fill(color);
    }
}

void SPPage::setPageBorder(guint32 color)
{
    for (auto view : views) {
        view->set_stroke(color);
    }
}

void SPPage::setPageShadow(bool show)
{
    for (auto view : views) {
        view->set_shadow(0x00000088, show ? 2 : 0);
    }
}

/**
 * Returns the page number (order of pages) starting at 1
 */
int SPPage::getPageNumber()
{
    if (_manager) {
        return _manager->getPageIndex(this) + 1;
    }
    return -5;
}

/**
 * Move the page by the given affine, in desktop units.
 */
void SPPage::movePage(Geom::Affine translate, bool with_objects)
{
    if (translate.isTranslation()) {
        if (with_objects) {
            g_warning("I want to move all the objects...");
        }
        auto current_rect = getDesktopRect() * translate;
        current_rect *= document->getDocumentScale().inverse();

        this->x = current_rect.left();
        this->y = current_rect.top();
        this->width = current_rect.width();
        this->height = current_rect.height();

        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    }
}

void SPPage::update(SPCtx* /*ctx*/, unsigned int /*flags*/)
{
    for (auto view : views) {
        view->set_rect(getDesktopRect());
    }
}

/*
Inkscape::XML::Node *SPPage::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("inkscape:page");
    }

    repr->setAttributeSvgDouble("x", this->x.computed);
    repr->setAttributeSvgDouble("y", this->y.computed);
    repr->setAttributeSvgDouble("width", this->width.computed);
    repr->setAttributeSvgDouble("height", this->height.computed);

    return SPObject::write(xml_doc, repr, flags);
}
*/

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
