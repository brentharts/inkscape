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
    for(auto item : canvas_items) {
        delete item;
    }
    this->canvas_items.clear();

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
 *
 * This ignores layers so items in the same layer which are shared
 * between pages are not moved around or exported into pages they
 * shouldn't be.
 */
std::vector<SPItem*> SPPage::getExclusiveItems() const
{
    // There's no logical reason why the desktop is needed here
    // we should have a getItemsInBox that doesn't use the desktop
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    return document->getItemsInBox(desktop->dkey, getDesktopRect(), true, true, true, false);
}

/**
 * Like ExcludiveItems above but get all the items which are inside or overlapping.
 */
std::vector<SPItem*> SPPage::getOverlappingItems() const
{
    // There's no logical reason why the desktop is needed here
    // we should have a getItemsPartiallyInBox that doesn't use the desktop
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    return document->getItemsPartiallyInBox(desktop->dkey, getDesktopRect(), true, true, true, false);
}

/**
 * Return true if this item is contained within the page boundry.
 */
bool SPPage::itemOnPage(SPItem *item, bool contains) const
{
    if (auto box = item->desktopGeometricBounds()) {
        if (contains) {
            return getDesktopRect().contains(*box);
        }
        return getDesktopRect().intersects(*box);
    }
    return false;
}

/**
 * Shows the page in the given canvas item group.
 */
void SPPage::showPage(SPDesktop *desktop, Inkscape::CanvasItemGroup *background_group, Inkscape::CanvasItemGroup *border_group)
{
    // Foreground 'border'
    if (auto item = new Inkscape::CanvasItemRect(border_group, getDesktopRect())) {
        item->set_name("foreground");
        canvas_items.push_back(item);
    }

    // Background rectangle 'fill'
    if (auto item = new Inkscape::CanvasItemRect(background_group, getDesktopRect())) {
        item->set_name("background");
        item->set_dashed(false);
        item->set_inverted(false);
        item->set_stroke(0x00000000);
        canvas_items.push_back(item);
    }

    if (auto label = new Inkscape::CanvasItemText(border_group, Geom::Point(0, 0), "{Page Label}")) {
        label->set_fontsize(10.0);
        label->set_fill(0xffffffff);
        label->set_background(0x00000099);
        label->set_bg_radius(1.0);
        label->set_anchor(Geom::Point(-1.0, -1.5));
        label->set_adjust(Geom::Point(-3, 0));
        canvas_items.push_back(label);
    }

    // The final steps are completed in an update cycle
    this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

/**
 * Hide the page in the given canvas widget.
 */
void SPPage::hidePage(Inkscape::UI::Widget::Canvas *canvas)
{
    g_assert(canvas != nullptr);
    for (auto it = canvas_items.begin(); it != canvas_items.end(); it) {
        if (canvas == (*it)->get_canvas()) {
            delete (*it);
            it = canvas_items.erase(it);
        } else {
            ++it;
        }
    }
}

void SPPage::showPage()
{
    for (auto item : canvas_items) {
        item->show();
    }
}

void SPPage::hidePage()
{
    for(auto item : canvas_items) {
        item->hide();
    }
}

/** 
 * Sets the default attributes from the namedview.
 */
bool SPPage::setDefaultAttributes(bool on_top, guint32 border, guint32 bg, int shadow)
{
    if (on_top != border_on_top || border != border_color || bg != background_color || shadow != shadow_size) {
        this->border_on_top = on_top;
        this->border_color = border;
        this->background_color = bg;
        this->shadow_size = shadow;
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        return true;
    }
    return false;
}

/**
 * Set the selected high-light for this page.
 */
void SPPage::setSelected(bool sel)
{
    this->is_selected = sel;
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

        // This is needed to update the xml
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
    for (auto item : canvas_items) {
        if (auto rect = dynamic_cast<Inkscape::CanvasItemRect *>(item)) {
            rect->set_rect(getDesktopRect());

            bool is_foreground = (rect->get_name() == "foreground");
            // This will put the border on the background OR foreground layer as needed.
            if (is_foreground == border_on_top) {
                rect->show();
                rect->set_shadow(shadow_color, shadow_size);
                rect->set_stroke(is_selected ? select_color : border_color);
            } else {
                rect->hide();
                rect->set_shadow(0x0, 0);
                rect->set_stroke(0x0);
            }
            // This undoes the hide for the background rect, but that's ok
            if (!is_foreground) {
                rect->show();
                rect->set_background(background_color);
            }
        }
        if (auto label = dynamic_cast<Inkscape::CanvasItemText *>(item)) {
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
