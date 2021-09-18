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

    for(auto label : labels) {
        delete label;
    }
    this->labels.clear();

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
    // There's no logical reason why the desktop is needed here
    // we should have a getItemsInBox that doesn't use the desktop
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    return document->getItemsInBox(desktop->dkey, getDesktopRect(), true, true, true, false);
}

/**
 * Get all the items which are inside or overlapping.
 */
std::vector<SPItem*> SPPage::getOverlappingItems() const
{
    // There's no logical reason why the desktop is needed here
    // we should have a getItemsInBox that doesn't use the desktop
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    return document->getItemsPartiallyInBox(desktop->dkey, getDesktopRect(), true, true, true, false);
}

/**
 * Shows the page in the given canvas item group.
 */
void SPPage::showPage(SPDesktop *desktop, Inkscape::CanvasItemGroup *group)
{
    auto item = new Inkscape::CanvasItemRect(group, getDesktopRect());
    item->set_dashed(false);
    item->set_inverted(false);
    views.push_back(item);

    auto label = new Inkscape::CanvasItemText(group, Geom::Point(0, 0), "{Page Label}");
    label->set_fontsize(10.0);
    label->set_fill(0xffffffff);
    label->set_background(0x00000099);
    label->set_bg_radius(1.0);
    label->set_anchor(Geom::Point(-1.0, -1.5));
    label->set_adjust(Geom::Point(-3, 0));
    labels.push_back(label);

    // The final steps are completed in an update cycle
    this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

/**
 * Hide the page in the given canvas widget.
 */
void SPPage::hidePage(Inkscape::UI::Widget::Canvas *canvas)
{
    g_assert(canvas != nullptr);
    int done = 0;
    for (auto it = views.begin(); it != views.end(); ++it) {
        if (canvas == (*it)->get_canvas()) {
            delete (*it);
            views.erase(it);
            done |= 1;
        }
    }
    for (auto it = labels.begin(); it != labels.end(); ++it) {
        if (canvas == (*it)->get_canvas()) {
            delete (*it);
            labels.erase(it);
            done |= 2;
        }
    }
    assert(done < 3);
}

void SPPage::showPage()
{
    for (auto view : views) {
        view->show();
    }
    for(auto label : labels) {
        label->show();
    }
}

void SPPage::hidePage()
{
    for(auto view : views) {
        view->hide();
    }
    for(auto label : labels) {
        label->hide();
    }
}

void SPPage::setPageColor(guint32 color)
{
    this->fill_color = color;
    this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void SPPage::setPageBorder(guint32 color)
{
    this->stroke_color = color;
    this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

/**
 * Set the selected high-light for this page.
 */
void SPPage::setSelected(bool sel)
{
    this->is_selected = sel;
    this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void SPPage::setPageShadow(bool show)
{
    this->has_shadow = show;
    this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
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
 *
 * @param translate - The positional translation to apply.
 * @param with_objects - Flag to request that connected objects also move.
 */
void SPPage::movePage(Geom::Affine translate, bool with_objects)
{
    if (translate.isTranslation()) {
        auto scale = document->getDocumentScale();

        if (with_objects) {
            // Move each item that is overlapping this page too
            for (auto &item : getOverlappingItems()) {
                auto move = (item->transform * scale) * translate * scale.inverse();
                item->doWriteTransform(move, &move, false);
            }
        }
        auto current_rect = getDesktopRect() * translate;
        current_rect *= scale.inverse();

        this->x = current_rect.left();
        this->y = current_rect.top();
        this->width = current_rect.width();
        this->height = current_rect.height();

        // This is needed to update the xml, although perhaps it
        // should be moved to the ::update below.
        this->updateRepr();

        // This eventually calls the ::update below while idle
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    }
}

/**
 * Update the visual representation on screen, this is manual because
 * this is not an SPItem, but it's own visual identity.
 */
void SPPage::update(SPCtx* /*ctx*/, unsigned int /*flags*/)
{
    // Put these in the preferences?
    guint32 shadow_color = 0x00000088;
    guint32 select_color = 0xff0000cc;
    for (auto view : views) {
        view->set_rect(getDesktopRect());
        view->set_shadow(shadow_color, has_shadow ? 2 : 0);
        view->set_stroke(is_selected ? select_color : stroke_color);
        view->set_fill(fill_color);
    }
    for (auto label : labels) {
        if (auto txt = this->label()) {
            auto corner = getDesktopRect().corner(0);
            label->set_coord(corner);
            label->set_text(txt);
            label->show();
        } else {
            label->set_text("");
            label->hide();
        }
    }
}

/**
 * Write out the page's data into it's xml structure.
 */
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
