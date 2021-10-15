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

#include "sp-page.h"

#include "attributes.h"
#include "desktop.h"
#include "inkscape.h"
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
    for (auto item : canvas_items) {
        delete item;
    }
    this->canvas_items.clear();

    if (this->document) {
        // Unregister ourselves
        this->document->removeResource("page", this);
    }

    SPObject::release();
}

void SPPage::set(SPAttr key, const gchar *value)
{
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
    return Geom::Rect(this->x.computed, this->y.computed, this->x.computed + this->width.computed,
                      this->y.computed + this->height.computed);
}

/**
 * Get the rectangle of the page, scaled to the document.
 */
Geom::Rect SPPage::getDesktopRect() const
{
    return getRect() * document->getDocumentScale();
}

/**
 * Set the page rectangle in it's native units.
 */
void SPPage::setRect(Geom::Rect rect)
{
    this->x = rect.left();
    this->y = rect.top();
    this->width = rect.width();
    this->height = rect.height();

    // This is needed to update the xml
    this->updateRepr();

    // This eventually calls the ::update below while idle
    this->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

/**
 * Set the page rectangle is desktop coordinates.
 */
void SPPage::setDesktopRect(Geom::Rect rect)
{
    setRect(rect * document->getDocumentScale().inverse());
}

/**
 * Set just the height and width from a predefined size.
 */
void SPPage::setDesktopSize(double width, double height)
{
    auto rect = getDesktopRect();
    rect.setMax(rect.corner(0) + Geom::Point(width, height));
    setDesktopRect(rect);
}

/**
 * Get the items which are ONLY on this page and don't overlap.
 *
 * This ignores layers so items in the same layer which are shared
 * between pages are not moved around or exported into pages they
 * shouldn't be.
 */
std::vector<SPItem *> SPPage::getExclusiveItems() const
{
    // There's no logical reason why the desktop is needed here
    // we should have a getItemsInBox that doesn't use the desktop
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    return document->getItemsInBox(desktop->dkey, getDesktopRect(), true, true, true, false);
}

/**
 * Like ExcludiveItems above but get all the items which are inside or overlapping.
 */
std::vector<SPItem *> SPPage::getOverlappingItems() const
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
void SPPage::showPage(SPDesktop *desktop, Inkscape::CanvasItemGroup *background_group,
                      Inkscape::CanvasItemGroup *border_group)
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
    for (auto item : canvas_items) {
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
int SPPage::getPageIndex()
{
    if (_manager) {
        return _manager->getPageIndex(this);
    }
    return -5;
}

/**
 * Set this page to a new order in the page stack.
 *
 * @param index - Placement of page in the stack, starting at '0'
 * @param swap_page - Swap the rectangle position
 *
 * @returns true if page has been moved.
 */
bool SPPage::setPageIndex(int index, bool swap_page)
{
    int current = getPageIndex();

    if (_manager && current != index) {
        // The page we're going to be shifting to
        auto sibling = _manager->getPage(index);

        // Insertions are done to the right of the sibling
        if (index < current) {
            index -= 1;
        }
        auto insert_after = _manager->getPage(index);

        // We may have selected an index off the end, so attach it after the last page.
        if (!insert_after && index > 0) {
            insert_after = _manager->getLastPage();
            sibling = nullptr; // disable swap
        }

        if (insert_after) {
            if (this == insert_after) {
                g_warning("Page is already at this index. Not moving.");
                return false;
            }
            // Attach after the given page
            getRepr()->parent()->changeOrder(getRepr(), insert_after->getRepr());
        } else {
            // Attach to before any existing page
            sibling = _manager->getFirstPage();
            getRepr()->parent()->changeOrder(getRepr(), nullptr);
        }
        if (sibling && swap_page) {
            swapPage(sibling, true);
        }
        return true;
    }
    return false;
}

/**
 * Returns the sibling page next to this one in the stack order.
 */
SPPage *SPPage::getNextPage()
{
    if (_manager) {
        _manager->getPage(getPageIndex() + 1);
    }
    return nullptr;
}

/**
 * Returns the sibling page previous to this one in the stack order.
 */
SPPage *SPPage::getPreviousPage()
{
    if (_manager) {
        _manager->getPage(getPageIndex() - 1);
    }
    return nullptr;
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
        if (with_objects) {
            // Move each item that is overlapping this page too
            auto scale = document->getDocumentScale();
            for (auto &item : getOverlappingItems()) {
                auto move = (item->transform * scale) * translate * scale.inverse();
                item->doWriteTransform(move, &move, false);
            }
        }
        setDesktopRect(getDesktopRect() * translate);
    }
}

/**
 * Swap the locations of this page with another page (see movePage)
 *
 * @param other - The other page to swap with
 * @param with_objects - Should the page objects move too.
 */
void SPPage::swapPage(SPPage *other, bool with_objects)
{
    auto this_affine = Geom::Translate(getDesktopRect().corner(0));
    auto other_affine = Geom::Translate(other->getDesktopRect().corner(0));
    movePage(this_affine.inverse() * other_affine, with_objects);
    other->movePage(other_affine.inverse() * this_affine, with_objects);
}

/**
 * Update the visual representation on screen, this is manual because
 * this is not an SPItem, but it's own visual identity.
 */
void SPPage::update(SPCtx * /*ctx*/, unsigned int /*flags*/)
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
