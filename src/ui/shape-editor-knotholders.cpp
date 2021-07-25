// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Node editing extension to objects
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Mitsuru Oka
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

// Declared in shape-editor.cpp.

#include <glibmm/i18n.h>

#include "preferences.h"
#include "desktop.h"
#include "style.h"

#include "live_effects/effect.h"

#include "object/box3d.h"
#include "object/sp-marker.h"
#include "object/sp-ellipse.h"
#include "object/sp-flowtext.h"
#include "object/sp-item.h"
#include "object/sp-namedview.h"
#include "object/sp-offset.h"
#include "object/sp-pattern.h"
#include "object/sp-rect.h"
#include "object/sp-spiral.h"
#include "object/sp-star.h"
#include "object/sp-text.h"
#include "object/sp-textpath.h"
#include "object/sp-tspan.h"

#include "ui/knot/knot-holder.h"
#include "ui/knot/knot-holder-entity.h"

class RectKnotHolder : public KnotHolder {
public:
    RectKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler);
    ~RectKnotHolder() override = default;;
};

class Box3DKnotHolder : public KnotHolder {
public:
    Box3DKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler);
    ~Box3DKnotHolder() override = default;;
};

class MarkerKnotHolder : public KnotHolder {
public:
    MarkerKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler);
    ~MarkerKnotHolder() override = default;;
};

class ArcKnotHolder : public KnotHolder {
public:
    ArcKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler);
    ~ArcKnotHolder() override = default;;
};

class StarKnotHolder : public KnotHolder {
public:
    StarKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler);
    ~StarKnotHolder() override = default;;
};

class SpiralKnotHolder : public KnotHolder {
public:
    SpiralKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler);
    ~SpiralKnotHolder() override = default;;
};

class OffsetKnotHolder : public KnotHolder {
public:
    OffsetKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler);
    ~OffsetKnotHolder() override = default;;
};

class TextKnotHolder : public KnotHolder {
public:
    TextKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler);
    ~TextKnotHolder() override = default;;
};

class FlowtextKnotHolder : public KnotHolder {
public:
    FlowtextKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler);
    ~FlowtextKnotHolder() override = default;;
};

class MiscKnotHolder : public KnotHolder {
public:
    MiscKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler);
    ~MiscKnotHolder() override = default;;
};

namespace {

static bool _edit_marker_mode = false;

static KnotHolder *sp_lpe_knot_holder(SPLPEItem *item, SPDesktop *desktop)
{
    KnotHolder *knot_holder = new KnotHolder(desktop, item, nullptr);

    Inkscape::LivePathEffect::Effect *effect = item->getCurrentLPE();
    effect->addHandles(knot_holder, item);

    return knot_holder;
}

/* wrapper around item->requestDisplayUpdate to handle items which are being edited in edit_marker_mode.
All the objects that reference the marker also need to update their displays */
static void requestDisplayUpdate(SPItem *item, unsigned int flags = SP_OBJECT_MODIFIED_FLAG) {
    if(_edit_marker_mode) {
        /* todo - handle logic when marker is not immediate parent, its under a group etc. */
        SPObject *parent = SP_IS_MARKER(item)? item: item->parent;

        for(auto i: parent->hrefList) {
            item->requestDisplayUpdate(flags);
            i->requestDisplayUpdate(flags);
        }
    } else {
        item->requestDisplayUpdate(flags);
    }
}

} // namespace

namespace Inkscape {
namespace UI {

KnotHolder *createKnotHolder(SPItem *item, SPDesktop *desktop, bool edit_marker_mode)
{
    KnotHolder *knotholder = nullptr;
    _edit_marker_mode = edit_marker_mode;

    if (dynamic_cast<SPRect *>(item)) {
        knotholder = new RectKnotHolder(desktop, item, nullptr);
    } else if (dynamic_cast<SPBox3D *>(item)) {
        knotholder = new Box3DKnotHolder(desktop, item, nullptr);
    } else if (dynamic_cast<SPMarker *>(item)) {
        knotholder = new MarkerKnotHolder(desktop, item, nullptr);
    } else if (dynamic_cast<SPGenericEllipse *>(item)) {
        knotholder = new ArcKnotHolder(desktop, item, nullptr);
    } else if (dynamic_cast<SPStar *>(item)) {
        knotholder = new StarKnotHolder(desktop, item, nullptr);
    } else if (dynamic_cast<SPSpiral *>(item)) {
        knotholder = new SpiralKnotHolder(desktop, item, nullptr);
    } else if (dynamic_cast<SPOffset *>(item)) {
        knotholder = new OffsetKnotHolder(desktop, item, nullptr);
    } else if (dynamic_cast<SPText *>(item)) {
        SPText *text = dynamic_cast<SPText *>(item);

        // Do not allow conversion to 'inline-size' wrapped text if on path!
        // <textPath> might not be first child if <title> or <desc> is present.
        bool is_on_path = false;
        for (auto child : text->childList(false)) {
            if (dynamic_cast<SPTextPath *>(child)) is_on_path = true;
        }
        if (!is_on_path) {
            knotholder = new TextKnotHolder(desktop, item, nullptr);
        }
    } else {
        SPFlowtext *flowtext = dynamic_cast<SPFlowtext *>(item);
        if (flowtext && flowtext->has_internal_frame()) {
            knotholder = new FlowtextKnotHolder(desktop, flowtext->get_frame(nullptr), nullptr);
        } else if ((item->style->fill.isPaintserver() && dynamic_cast<SPPattern *>(item->style->getFillPaintServer())) ||
                   (item->style->stroke.isPaintserver() && dynamic_cast<SPPattern *>(item->style->getStrokePaintServer()))) {
            knotholder = new KnotHolder(desktop, item, nullptr);
            knotholder->add_pattern_knotholder();
        }
    }
    if (!knotholder) knotholder = new KnotHolder(desktop, item, nullptr);
    knotholder->add_filter_knotholder();

    return knotholder;
}

KnotHolder *createLPEKnotHolder(SPItem *item, SPDesktop *desktop)
{
    KnotHolder *knotholder = nullptr;

    SPLPEItem *lpe = dynamic_cast<SPLPEItem *>(item);
    if (lpe &&
        lpe->getCurrentLPE() &&
        lpe->getCurrentLPE()->isVisible() &&
        lpe->getCurrentLPE()->providesKnotholder()) {
        knotholder = sp_lpe_knot_holder(lpe, desktop);
    }
    return knotholder;
}

}
} // namespace Inkscape

/* SPRect */

/* handle for horizontal rounding radius */
class RectKnotHolderEntityRX : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_click(unsigned int state) override;
};

/* handle for vertical rounding radius */
class RectKnotHolderEntityRY : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_click(unsigned int state) override;
};

/* handle for width/height adjustment */
class RectKnotHolderEntityWH : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;

protected:
    void set_internal(Geom::Point const &p, Geom::Point const &origin, unsigned int state);
};

/* handle for x/y adjustment */
class RectKnotHolderEntityXY : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

/* handle for position */
class RectKnotHolderEntityCenter : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

Geom::Point
RectKnotHolderEntityRX::knot_get() const
{
    SPRect *rect = dynamic_cast<SPRect *>(item);
    g_assert(rect != nullptr);

    return Geom::Point(rect->x.computed + rect->width.computed - rect->rx.computed, rect->y.computed);
}

void
RectKnotHolderEntityRX::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    SPRect *rect = dynamic_cast<SPRect *>(item);
    g_assert(rect != nullptr);

    //In general we cannot just snap this radius to an arbitrary point, as we have only a single
    //degree of freedom. For snapping to an arbitrary point we need two DOF. If we're going to snap
    //the radius then we should have a constrained snap. snap_knot_position() is unconstrained
    Geom::Point const s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(Geom::Point(rect->x.computed + rect->width.computed, rect->y.computed), Geom::Point(-1, 0)), state);

    if (state & GDK_CONTROL_MASK) {
        gdouble temp = MIN(rect->height.computed, rect->width.computed) / 2.0;
        rect->rx = rect->ry = CLAMP(rect->x.computed + rect->width.computed - s[Geom::X], 0.0, temp);
    } else {
        rect->rx = CLAMP(rect->x.computed + rect->width.computed - s[Geom::X], 0.0, rect->width.computed / 2.0);
    }

    update_knot();

    requestDisplayUpdate(rect);
}

void
RectKnotHolderEntityRX::knot_click(unsigned int state)
{
    SPRect *rect = dynamic_cast<SPRect *>(item);
    g_assert(rect != nullptr);

    if (state & GDK_SHIFT_MASK) {
        /* remove rounding from rectangle */
        rect->getRepr()->removeAttribute("rx");
        rect->getRepr()->removeAttribute("ry");
    } else if (state & GDK_CONTROL_MASK) {
        /* Ctrl-click sets the vertical rounding to be the same as the horizontal */
        rect->getRepr()->setAttribute("ry", rect->getRepr()->attribute("rx"));
    }

}

Geom::Point
RectKnotHolderEntityRY::knot_get() const
{
    SPRect *rect = dynamic_cast<SPRect *>(item);
    g_assert(rect != nullptr);

    return Geom::Point(rect->x.computed + rect->width.computed, rect->y.computed + rect->ry.computed);
}

void
RectKnotHolderEntityRY::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    SPRect *rect = dynamic_cast<SPRect *>(item);
    g_assert(rect != nullptr);

    //In general we cannot just snap this radius to an arbitrary point, as we have only a single
    //degree of freedom. For snapping to an arbitrary point we need two DOF. If we're going to snap
    //the radius then we should have a constrained snap. snap_knot_position() is unconstrained
    Geom::Point const s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(Geom::Point(rect->x.computed + rect->width.computed, rect->y.computed), Geom::Point(0, 1)), state);

    if (state & GDK_CONTROL_MASK) { // When holding control then rx will be kept equal to ry,
                                    // resulting in a perfect circle (and not an ellipse)
        gdouble temp = MIN(rect->height.computed, rect->width.computed) / 2.0;
        rect->rx = rect->ry = CLAMP(s[Geom::Y] - rect->y.computed, 0.0, temp);
    } else {
        if (!rect->rx._set || rect->rx.computed == 0) {
            rect->ry = CLAMP(s[Geom::Y] - rect->y.computed,
                             0.0,
                             MIN(rect->height.computed / 2.0, rect->width.computed / 2.0));
        } else {
            rect->ry = CLAMP(s[Geom::Y] - rect->y.computed,
                             0.0,
                             rect->height.computed / 2.0);
        }
    }

    update_knot();

    requestDisplayUpdate(rect);
}

void
RectKnotHolderEntityRY::knot_click(unsigned int state)
{
    SPRect *rect = dynamic_cast<SPRect *>(item);
    g_assert(rect != nullptr);

    if (state & GDK_SHIFT_MASK) {
        /* remove rounding */
        rect->getRepr()->removeAttribute("rx");
        rect->getRepr()->removeAttribute("ry");
    } else if (state & GDK_CONTROL_MASK) {
        /* Ctrl-click sets the vertical rounding to be the same as the horizontal */
        rect->getRepr()->setAttribute("rx", rect->getRepr()->attribute("ry"));
    }
}

#define SGN(x) ((x)>0?1:((x)<0?-1:0))

static void sp_rect_clamp_radii(SPRect *rect)
{
    // clamp rounding radii so that they do not exceed width/height
    if (2 * rect->rx.computed > rect->width.computed) {
        rect->rx = 0.5 * rect->width.computed;
    }
    if (2 * rect->ry.computed > rect->height.computed) {
        rect->ry = 0.5 * rect->height.computed;
    }
}

Geom::Point
RectKnotHolderEntityWH::knot_get() const
{
    SPRect *rect = dynamic_cast<SPRect *>(item);
    g_assert(rect != nullptr);

    return Geom::Point(rect->x.computed + rect->width.computed, rect->y.computed + rect->height.computed);
}

void
RectKnotHolderEntityWH::set_internal(Geom::Point const &p, Geom::Point const &origin, unsigned int state)
{
    SPRect *rect = dynamic_cast<SPRect *>(item);
    g_assert(rect != nullptr);

    Geom::Point s = p;

    if (state & GDK_CONTROL_MASK) {
        // original width/height when drag started
        gdouble const w_orig = (origin[Geom::X] - rect->x.computed);
        gdouble const h_orig = (origin[Geom::Y] - rect->y.computed);

        //original ratio
        gdouble ratio = (w_orig / h_orig);

        // mouse displacement since drag started
        gdouble minx = p[Geom::X] - origin[Geom::X];
        gdouble miny = p[Geom::Y] - origin[Geom::Y];

        Geom::Point p_handle(rect->x.computed + rect->width.computed, rect->y.computed + rect->height.computed);

        if (fabs(minx) > fabs(miny)) {
            // snap to horizontal or diagonal
            if (minx != 0 && fabs(miny/minx) > 0.5 * 1/ratio && (SGN(minx) == SGN(miny))) {
                // closer to the diagonal and in same-sign quarters, change both using ratio
                s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(p_handle, Geom::Point(-ratio, -1)), state);
                minx = s[Geom::X] - origin[Geom::X];
                // Dead assignment: Value stored to 'miny' is never read
                //miny = s[Geom::Y] - origin[Geom::Y];
                rect->height = MAX(h_orig + minx / ratio, 0);
            } else {
                // closer to the horizontal, change only width, height is h_orig
                s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(p_handle, Geom::Point(-1, 0)), state);
                minx = s[Geom::X] - origin[Geom::X];
                // Dead assignment: Value stored to 'miny' is never read
                //miny = s[Geom::Y] - origin[Geom::Y];
                rect->height = MAX(h_orig, 0);
            }
            rect->width = MAX(w_orig + minx, 0);

        } else {
            // snap to vertical or diagonal
            if (miny != 0 && fabs(minx/miny) > 0.5 * ratio && (SGN(minx) == SGN(miny))) {
                // closer to the diagonal and in same-sign quarters, change both using ratio
                s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(p_handle, Geom::Point(-ratio, -1)), state);
                // Dead assignment: Value stored to 'minx' is never read
                //minx = s[Geom::X] - origin[Geom::X];
                miny = s[Geom::Y] - origin[Geom::Y];
                rect->width = MAX(w_orig + miny * ratio, 0);
            } else {
                // closer to the vertical, change only height, width is w_orig
                s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(p_handle, Geom::Point(0, -1)), state);
                // Dead assignment: Value stored to 'minx' is never read
                //minx = s[Geom::X] - origin[Geom::X];
                miny = s[Geom::Y] - origin[Geom::Y];
                rect->width = MAX(w_orig, 0);
            }
            rect->height = MAX(h_orig + miny, 0);

        }

    } else {
        // move freely
        s = snap_knot_position(p, state);
        rect->width = MAX(s[Geom::X] - rect->x.computed, 0);
        rect->height = MAX(s[Geom::Y] - rect->y.computed, 0);
    }

    sp_rect_clamp_radii(rect);

    requestDisplayUpdate(rect);
}

void
RectKnotHolderEntityWH::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state)
{
    set_internal(p, origin, state);
    update_knot();
}

Geom::Point
RectKnotHolderEntityXY::knot_get() const
{
    SPRect *rect = dynamic_cast<SPRect *>(item);
    g_assert(rect != nullptr);

    return Geom::Point(rect->x.computed, rect->y.computed);
}

void
RectKnotHolderEntityXY::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state)
{
    SPRect *rect = dynamic_cast<SPRect *>(item);
    g_assert(rect != nullptr);

    // opposite corner (unmoved)
    gdouble opposite_x = (rect->x.computed + rect->width.computed);
    gdouble opposite_y = (rect->y.computed + rect->height.computed);

    // original width/height when drag started
    gdouble w_orig = opposite_x - origin[Geom::X];
    gdouble h_orig = opposite_y - origin[Geom::Y];

    Geom::Point s = p;
    Geom::Point p_handle(rect->x.computed, rect->y.computed);

    // mouse displacement since drag started
    gdouble minx = p[Geom::X] - origin[Geom::X];
    gdouble miny = p[Geom::Y] - origin[Geom::Y];

    if (state & GDK_CONTROL_MASK) {
        //original ratio
        gdouble ratio = (w_orig / h_orig);

        if (fabs(minx) > fabs(miny)) {
            // snap to horizontal or diagonal
            if (minx != 0 && fabs(miny/minx) > 0.5 * 1/ratio && (SGN(minx) == SGN(miny))) {
                // closer to the diagonal and in same-sign quarters, change both using ratio
                s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(p_handle, Geom::Point(-ratio, -1)), state);
                minx = s[Geom::X] - origin[Geom::X];
                // Dead assignment: Value stored to 'miny' is never read
                //miny = s[Geom::Y] - origin[Geom::Y];
                rect->y = MIN(origin[Geom::Y] + minx / ratio, opposite_y);
                rect->height = MAX(h_orig - minx / ratio, 0);
            } else {
                // closer to the horizontal, change only width, height is h_orig
                s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(p_handle, Geom::Point(-1, 0)), state);
                minx = s[Geom::X] - origin[Geom::X];
                // Dead assignment: Value stored to 'miny' is never read
                //miny = s[Geom::Y] - origin[Geom::Y];
                rect->y = MIN(origin[Geom::Y], opposite_y);
                rect->height = MAX(h_orig, 0);
            }
            rect->x = MIN(s[Geom::X], opposite_x);
            rect->width = MAX(w_orig - minx, 0);
        } else {
            // snap to vertical or diagonal
            if (miny != 0 && fabs(minx/miny) > 0.5 *ratio && (SGN(minx) == SGN(miny))) {
                // closer to the diagonal and in same-sign quarters, change both using ratio
                s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(p_handle, Geom::Point(-ratio, -1)), state);
                // Dead assignment: Value stored to 'minx' is never read
                //minx = s[Geom::X] - origin[Geom::X];
                miny = s[Geom::Y] - origin[Geom::Y];
                rect->x = MIN(origin[Geom::X] + miny * ratio, opposite_x);
                rect->width = MAX(w_orig - miny * ratio, 0);
            } else {
                // closer to the vertical, change only height, width is w_orig
                s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(p_handle, Geom::Point(0, -1)), state);
                // Dead assignment: Value stored to 'minx' is never read
                //minx = s[Geom::X] - origin[Geom::X];
                miny = s[Geom::Y] - origin[Geom::Y];
                rect->x = MIN(origin[Geom::X], opposite_x);
                rect->width = MAX(w_orig, 0);
            }
            rect->y = MIN(s[Geom::Y], opposite_y);
            rect->height = MAX(h_orig - miny, 0);
        }

    } else {
        // move freely
        s = snap_knot_position(p, state);
        minx = s[Geom::X] - origin[Geom::X];
        miny = s[Geom::Y] - origin[Geom::Y];

        rect->x = MIN(s[Geom::X], opposite_x);
        rect->y = MIN(s[Geom::Y], opposite_y);
        rect->width = MAX(w_orig - minx, 0);
        rect->height = MAX(h_orig - miny, 0);
    }

    sp_rect_clamp_radii(rect);

    update_knot();

    requestDisplayUpdate(rect);
}

Geom::Point
RectKnotHolderEntityCenter::knot_get() const
{
    SPRect *rect = dynamic_cast<SPRect *>(item);
    g_assert(rect != nullptr);

    return Geom::Point(rect->x.computed + (rect->width.computed / 2.), rect->y.computed + (rect->height.computed / 2.));
}

void
RectKnotHolderEntityCenter::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    SPRect *rect = dynamic_cast<SPRect *>(item);
    g_assert(rect != nullptr);

    Geom::Point const s = snap_knot_position(p, state);

    rect->x = s[Geom::X] - (rect->width.computed / 2.);
    rect->y = s[Geom::Y] - (rect->height.computed / 2.);

    // No need to call sp_rect_clamp_radii(): width and height haven't changed.
    // No need to call update_knot(): the knot is set directly by the user.

    requestDisplayUpdate(rect);
}

RectKnotHolder::RectKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler) :
    KnotHolder(desktop, item, relhandler)
{
    RectKnotHolderEntityRX *entity_rx = new RectKnotHolderEntityRX();
    RectKnotHolderEntityRY *entity_ry = new RectKnotHolderEntityRY();
    RectKnotHolderEntityWH *entity_wh = new RectKnotHolderEntityWH();
    RectKnotHolderEntityXY *entity_xy = new RectKnotHolderEntityXY();
    RectKnotHolderEntityCenter *entity_center = new RectKnotHolderEntityCenter();

    entity_rx->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_ROTATE, "Rect:rx",
                      _("Adjust the <b>horizontal rounding</b> radius; with <b>Ctrl</b> "
                        "to make the vertical radius the same"));

    entity_ry->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_ROTATE, "Rect:ry",
                      _("Adjust the <b>vertical rounding</b> radius; with <b>Ctrl</b> "
                        "to make the horizontal radius the same"));

    entity_wh->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Rect:wh",
                      _("Adjust the <b>width and height</b> of the rectangle; with <b>Ctrl</b> "
                        "to lock ratio or stretch in one dimension only"));

    entity_xy->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Rect:xy",
                      _("Adjust the <b>width and height</b> of the rectangle; with <b>Ctrl</b> "
                        "to lock ratio or stretch in one dimension only"));

    entity_center->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_POINT, "Rect:center",
                          _("Drag to move the rectangle"));

    entity.push_back(entity_rx);
    entity.push_back(entity_ry);
    entity.push_back(entity_wh);
    entity.push_back(entity_xy);
    entity.push_back(entity_center);

    add_pattern_knotholder();
    add_hatch_knotholder();
}

/* Box3D (= the new 3D box structure) */

class Box3DKnotHolderEntity : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override = 0;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override = 0;

    Geom::Point knot_get_generic(SPItem *item, unsigned int knot_id) const;
    void knot_set_generic(SPItem *item, unsigned int knot_id, Geom::Point const &p, unsigned int state);
};

Geom::Point
Box3DKnotHolderEntity::knot_get_generic(SPItem *item, unsigned int knot_id) const
{
    SPBox3D *box = dynamic_cast<SPBox3D *>(item);
    if (box) {
        return box->get_corner_screen(knot_id);
    } else {
        return Geom::Point(); // TODO investigate proper fallback
    }
}

void
Box3DKnotHolderEntity::knot_set_generic(SPItem *item, unsigned int knot_id, Geom::Point const &new_pos, unsigned int state)
{
    Geom::Point const s = snap_knot_position(new_pos, state);

    g_assert(item != nullptr);
    SPBox3D *box = dynamic_cast<SPBox3D *>(item);
    g_assert(box != nullptr);
    Geom::Affine const i2dt (item->i2dt_affine ());

    Box3D::Axis movement;
    if ((knot_id < 4) != (state & GDK_SHIFT_MASK)) {
        movement = Box3D::XY;
    } else {
        movement = Box3D::Z;
    }

    box->set_corner (knot_id, s * i2dt, movement, (state & GDK_CONTROL_MASK));
    box->set_z_orders();
    box->position_set();
}

class Box3DKnotHolderEntity0 : public Box3DKnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

class Box3DKnotHolderEntity1 : public Box3DKnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

class Box3DKnotHolderEntity2 : public Box3DKnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

class Box3DKnotHolderEntity3 : public Box3DKnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

class Box3DKnotHolderEntity4 : public Box3DKnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

class Box3DKnotHolderEntity5 : public Box3DKnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

class Box3DKnotHolderEntity6 : public Box3DKnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

class Box3DKnotHolderEntity7 : public Box3DKnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

class Box3DKnotHolderEntityCenter : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

Geom::Point
Box3DKnotHolderEntity0::knot_get() const
{
    return knot_get_generic(item, 0);
}

Geom::Point
Box3DKnotHolderEntity1::knot_get() const
{
    return knot_get_generic(item, 1);
}

Geom::Point
Box3DKnotHolderEntity2::knot_get() const
{
    return knot_get_generic(item, 2);
}

Geom::Point
Box3DKnotHolderEntity3::knot_get() const
{
    return knot_get_generic(item, 3);
}

Geom::Point
Box3DKnotHolderEntity4::knot_get() const
{
    return knot_get_generic(item, 4);
}

Geom::Point
Box3DKnotHolderEntity5::knot_get() const
{
    return knot_get_generic(item, 5);
}

Geom::Point
Box3DKnotHolderEntity6::knot_get() const
{
    return knot_get_generic(item, 6);
}

Geom::Point
Box3DKnotHolderEntity7::knot_get() const
{
    return knot_get_generic(item, 7);
}

Geom::Point
Box3DKnotHolderEntityCenter::knot_get() const
{
    SPBox3D *box = dynamic_cast<SPBox3D *>(item);
    if (box) {
        return box->get_center_screen();
    } else {
        return Geom::Point(); // TODO investigate proper fallback
    }
}

void
Box3DKnotHolderEntity0::knot_set(Geom::Point const &new_pos, Geom::Point const &/*origin*/, unsigned int state)
{
    knot_set_generic(item, 0, new_pos, state);
}

void
Box3DKnotHolderEntity1::knot_set(Geom::Point const &new_pos, Geom::Point const &/*origin*/, unsigned int state)
{
    knot_set_generic(item, 1, new_pos, state);
}

void
Box3DKnotHolderEntity2::knot_set(Geom::Point const &new_pos, Geom::Point const &/*origin*/, unsigned int state)
{
    knot_set_generic(item, 2, new_pos, state);
}

void
Box3DKnotHolderEntity3::knot_set(Geom::Point const &new_pos, Geom::Point const &/*origin*/, unsigned int state)
{
    knot_set_generic(item, 3, new_pos, state);
}

void
Box3DKnotHolderEntity4::knot_set(Geom::Point const &new_pos, Geom::Point const &/*origin*/, unsigned int state)
{
    knot_set_generic(item, 4, new_pos, state);
}

void
Box3DKnotHolderEntity5::knot_set(Geom::Point const &new_pos, Geom::Point const &/*origin*/, unsigned int state)
{
    knot_set_generic(item, 5, new_pos, state);
}

void
Box3DKnotHolderEntity6::knot_set(Geom::Point const &new_pos, Geom::Point const &/*origin*/, unsigned int state)
{
    knot_set_generic(item, 6, new_pos, state);
}

void
Box3DKnotHolderEntity7::knot_set(Geom::Point const &new_pos, Geom::Point const &/*origin*/, unsigned int state)
{
    knot_set_generic(item, 7, new_pos, state);
}

void
Box3DKnotHolderEntityCenter::knot_set(Geom::Point const &new_pos, Geom::Point const &origin, unsigned int state)
{
    Geom::Point const s = snap_knot_position(new_pos, state);

    SPBox3D *box = dynamic_cast<SPBox3D *>(item);
    g_assert(box != nullptr);
    Geom::Affine const i2dt (item->i2dt_affine ());

    box->set_center(s * i2dt, origin * i2dt, !(state & GDK_SHIFT_MASK) ? Box3D::XY : Box3D::Z,
                    state & GDK_CONTROL_MASK);

    box->set_z_orders();
    box->position_set();
}

Box3DKnotHolder::Box3DKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler) :
    KnotHolder(desktop, item, relhandler)
{
    Box3DKnotHolderEntity0 *entity_corner0 = new Box3DKnotHolderEntity0();
    Box3DKnotHolderEntity1 *entity_corner1 = new Box3DKnotHolderEntity1();
    Box3DKnotHolderEntity2 *entity_corner2 = new Box3DKnotHolderEntity2();
    Box3DKnotHolderEntity3 *entity_corner3 = new Box3DKnotHolderEntity3();
    Box3DKnotHolderEntity4 *entity_corner4 = new Box3DKnotHolderEntity4();
    Box3DKnotHolderEntity5 *entity_corner5 = new Box3DKnotHolderEntity5();
    Box3DKnotHolderEntity6 *entity_corner6 = new Box3DKnotHolderEntity6();
    Box3DKnotHolderEntity7 *entity_corner7 = new Box3DKnotHolderEntity7();
    Box3DKnotHolderEntityCenter *entity_center = new Box3DKnotHolderEntityCenter();

    entity_corner0->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Box3D:corner0",
                           _("Resize box in X/Y direction; with <b>Shift</b> along the Z axis; "
                             "with <b>Ctrl</b> to constrain to the directions of edges or diagonals"));

    entity_corner1->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Box3D:corner1",
                           _("Resize box in X/Y direction; with <b>Shift</b> along the Z axis; "
                             "with <b>Ctrl</b> to constrain to the directions of edges or diagonals"));

    entity_corner2->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Box3D:corner2",
                           _("Resize box in X/Y direction; with <b>Shift</b> along the Z axis; "
                             "with <b>Ctrl</b> to constrain to the directions of edges or diagonals"));

    entity_corner3->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Box3D:corner3",
                           _("Resize box in X/Y direction; with <b>Shift</b> along the Z axis; "
                             "with <b>Ctrl</b> to constrain to the directions of edges or diagonals"));

    entity_corner4->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Box3D:corner4",
                     _("Resize box along the Z axis; with <b>Shift</b> in X/Y direction; "
                       "with <b>Ctrl</b> to constrain to the directions of edges or diagonals"));

    entity_corner5->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Box3D:corner5",
                     _("Resize box along the Z axis; with <b>Shift</b> in X/Y direction; "
                       "with <b>Ctrl</b> to constrain to the directions of edges or diagonals"));

    entity_corner6->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Box3D:corner6",
                     _("Resize box along the Z axis; with <b>Shift</b> in X/Y direction; "
                       "with <b>Ctrl</b> to constrain to the directions of edges or diagonals"));

    entity_corner7->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Box3D:corner7",
                     _("Resize box along the Z axis; with <b>Shift</b> in X/Y direction; "
                       "with <b>Ctrl</b> to constrain to the directions of edges or diagonals"));

    entity_center->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_POINT, "Box3D:center",
                          _("Move the box in perspective"));

    entity.push_back(entity_corner0);
    entity.push_back(entity_corner1);
    entity.push_back(entity_corner2);
    entity.push_back(entity_corner3);
    entity.push_back(entity_corner4);
    entity.push_back(entity_corner5);
    entity.push_back(entity_corner6);
    entity.push_back(entity_corner7);
    entity.push_back(entity_center);

    add_pattern_knotholder();
    add_hatch_knotholder();
}

/* SPMarker Section */

/* MarkerKnotHolderEntity helper functions */
class MarkerKnotHolderWrapper {
public:
    static SPItem* item;
    static double original_scale; //helps with set_internal for scaling, need to keep track of the original_scale there

    static void validateMarker(SPItem* i);
    static double getScale();
};

SPItem* MarkerKnotHolderWrapper::item = nullptr;
double MarkerKnotHolderWrapper::original_scale = 1;

/* gets calculated markerScale, xScale and yScale should be the same for now */
double MarkerKnotHolderWrapper::getScale(){
    SPMarker *sp_marker = dynamic_cast<SPMarker *>(item);
    g_assert(sp_marker != nullptr);

    return (sp_marker->markerWidth.computed/sp_marker->viewBox.width());
}

/* sets any missing properties */
void MarkerKnotHolderWrapper::validateMarker(SPItem* i) {
    item = i;
    original_scale = 1;

    SPMarker *sp_marker = dynamic_cast<SPMarker *>(item);
    g_assert(sp_marker != nullptr);

    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    SPDocument *doc = desktop->getDocument();
    doc->ensureUpToDate();

    /* calculate marker bounds */
    std::vector<SPObject*> items = const_cast<SPMarker*>(sp_marker)->childList(false, SPObject::ActionBBox);

    Geom::OptRect r;
    for (auto *i : items) {
        SPItem *item = dynamic_cast<SPItem*>(i);
        r.unionWith(item->desktopVisualBounds());
    }

    Geom::Rect bounds(r->min() * doc->dt2doc(), r->max() * doc->dt2doc());
    Geom::Point const center = bounds.dimensions() * 0.5;

    /* check if refX/refY properties are set. If not, set them to default values. */
    if(!sp_marker->refX._set) {
        sp_marker->refX = center[Geom::X];
    }

    if(!sp_marker->refY._set) {
        sp_marker->refY = center[Geom::Y];
    }

    /* if there is no markerWidth or markerHeight, calculate and set it */
    if(!sp_marker->markerWidth._set || !sp_marker->markerHeight._set) {
        sp_marker->markerWidth = bounds.dimensions()[Geom::X];
        sp_marker->markerHeight = bounds.dimensions()[Geom::Y];
        sp_marker->viewBox = Geom::Rect::from_xywh(0, 0, sp_marker->markerWidth.computed / original_scale, sp_marker->markerHeight.computed / original_scale);
        sp_marker->viewBox_set = true;
    } else {
        /* check if markerWidth/markerHeight was correctly calculated */
        if((sp_marker->markerWidth.computed != bounds.dimensions()[Geom::X]) || (sp_marker->markerHeight.computed != bounds.dimensions()[Geom::Y])) {
            /* xScale and yScale should be the same for now, check if some scaling exists already and save it */
            if(sp_marker->viewBox_set && sp_marker->viewBox.width() > 0 && sp_marker->viewBox.height() > 0) {
                    double xScale = sp_marker->markerWidth.computed/sp_marker->viewBox.width();
                    double yScale = sp_marker->markerHeight.computed/sp_marker->viewBox.height();
                    if(xScale == yScale) {
                        original_scale = xScale;
                    } else if(xScale != 1) {
                        original_scale = xScale;
                    } else if(yScale != 1) {
                        original_scale = yScale;
                    }
            }
            
            sp_marker->markerWidth = bounds.dimensions()[Geom::X];
            sp_marker->markerHeight = bounds.dimensions()[Geom::Y];
            sp_marker->viewBox = Geom::Rect::from_xywh(0, 0, sp_marker->markerWidth.computed / original_scale, 
                                    sp_marker->markerHeight.computed / original_scale);
            sp_marker->viewBox_set = true;
        }
    }
}

/* handles for marker scaling */
class MarkerKnotHolderEntityScale : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
protected:
    void set_internal(Geom::Point const &p, Geom::Point const &origin, unsigned int state);
};

Geom::Point 
MarkerKnotHolderEntityScale::knot_get() const
{
    SPMarker *sp_marker = dynamic_cast<SPMarker *>(item);
    g_assert(sp_marker != nullptr);

    return Geom::Point((-sp_marker->refX.computed + sp_marker->markerWidth.computed) * MarkerKnotHolderWrapper::getScale(), 
        (-sp_marker->refY.computed + sp_marker->markerHeight.computed) * MarkerKnotHolderWrapper::getScale());
}

void 
MarkerKnotHolderEntityScale::knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) {
    SPMarker *sp_marker = dynamic_cast<SPMarker *>(item);
    g_assert(sp_marker != nullptr);
    
    MarkerKnotHolderWrapper::original_scale = MarkerKnotHolderWrapper::getScale();
}

void
MarkerKnotHolderEntityScale::set_internal(Geom::Point const &p, Geom::Point const &origin, unsigned int state)
{
    SPMarker *sp_marker = dynamic_cast<SPMarker *>(item);
    g_assert(sp_marker != nullptr);
    Geom::Point s = p;

    // ratio for the uniform scaling
    gdouble ratio = sp_marker->markerWidth.computed/sp_marker->markerHeight.computed;
    
    // mouse displacement since drag started
    gdouble minx = p[Geom::X] - origin[Geom::X];
    gdouble miny = p[Geom::Y] - origin[Geom::Y];
    
    Geom::Point p_handle((-sp_marker->refX.computed + sp_marker->markerWidth.computed) * MarkerKnotHolderWrapper::getScale(), 
        (-sp_marker->refY.computed + sp_marker->markerHeight.computed) * MarkerKnotHolderWrapper::getScale());

    // always snap to diagonal for now, to keep scaling uniform
    if (fabs(minx) > fabs(miny)) {
        s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(p_handle, Geom::Point(-ratio, -1)), state);
        
        double tmp_scale = MarkerKnotHolderWrapper::original_scale * (s[Geom::X]/sp_marker->markerWidth.computed);
        sp_marker->viewBox = Geom::Rect::from_xywh(0, 0, sp_marker->markerWidth.computed / tmp_scale, 
                        sp_marker->markerHeight.computed / tmp_scale);
        sp_marker->viewBox_set = true;
    } else {
        s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(p_handle, Geom::Point(-ratio, -1)), state);

        double tmp_scale = MarkerKnotHolderWrapper::original_scale * (s[Geom::Y]/sp_marker->markerHeight.computed);
        sp_marker->viewBox = Geom::Rect::from_xywh(0, 0, sp_marker->markerWidth.computed / tmp_scale, 
                        sp_marker->markerHeight.computed / tmp_scale);
        sp_marker->viewBox_set = true;
    }

    requestDisplayUpdate(sp_marker, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
}

void MarkerKnotHolderEntityScale::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state)
{
    set_internal(p, origin, state);
    update_knot();
}

/* Handle for marker orientation - setting up here, come back to math l8r g8r */
class MarkerKnotHolderEntityOrient : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

Geom::Point
MarkerKnotHolderEntityOrient::knot_get() const
{
    SPMarker *sp_marker = dynamic_cast<SPMarker *>(item);
    g_assert(sp_marker != nullptr);

    return Geom::Point((-sp_marker->refX.computed + sp_marker->markerWidth.computed) * MarkerKnotHolderWrapper::getScale(), 
        -sp_marker->refY.computed * MarkerKnotHolderWrapper::getScale());
}

void
MarkerKnotHolderEntityOrient::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state)
{
    SPMarker *sp_marker = dynamic_cast<SPMarker *>(item);
    g_assert(sp_marker != nullptr);

    gdouble   dx = p[Geom::X] - sp_marker->refX.computed;
    gdouble   dy = p[Geom::Y] - sp_marker->refY.computed;

    gdouble   moved_y = p[Geom::Y] - origin[Geom::Y];

    double exp_delta = moved_y;
    sp_marker->orient = exp_delta;

    sp_marker->orient_mode = MARKER_ORIENT_ANGLE;
    sp_marker->orient_set = TRUE;
    requestDisplayUpdate(sp_marker);
}

/* handles the marker refX/refY attributes */
class MarkerKnotHolderEntityCenter : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

Geom::Point
MarkerKnotHolderEntityCenter::knot_get() const
{
    SPMarker *sp_marker = dynamic_cast<SPMarker *>(item);
    g_assert(sp_marker != nullptr);

    return Geom::Point(-sp_marker->refX.computed * MarkerKnotHolderWrapper::getScale(), -sp_marker->refY.computed * MarkerKnotHolderWrapper::getScale());
}

void
MarkerKnotHolderEntityCenter::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    SPMarker *sp_marker = dynamic_cast<SPMarker *>(item);
    g_assert(sp_marker != nullptr);

    Geom::Point const s = -snap_knot_position(p, state) / MarkerKnotHolderWrapper::getScale();
    sp_marker->refX = s[Geom::X];
    sp_marker->refY = s[Geom::Y];

    requestDisplayUpdate(sp_marker);
}


MarkerKnotHolder::MarkerKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler) :
    KnotHolder(desktop, item, relhandler)
{
    /* This needs to be called before proceeding, to make sure all the marker attributes have been set correctly */
    MarkerKnotHolderWrapper::validateMarker(item);

    MarkerKnotHolderEntityCenter *entity_center = new MarkerKnotHolderEntityCenter();
    MarkerKnotHolderEntityScale *entity_scale = new MarkerKnotHolderEntityScale();
    MarkerKnotHolderEntityOrient *entity_orient = new MarkerKnotHolderEntityOrient();

    entity_center->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_POINT, "Marker:center",
                          _("Drag to adjust the refX/refY position of the marker"));

    entity_scale->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Marker:scale",
                      _("Adjust the <b>size</b> of the marker"));

    entity_orient->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Marker:orient",
                        _("Adjust marker orientation through rotation"));

    entity.push_back(entity_center);
    entity.push_back(entity_scale);
    entity.push_back(entity_orient);

    add_pattern_knotholder();
    add_hatch_knotholder();
}

/* SPArc */

class ArcKnotHolderEntityStart : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_click(unsigned int state) override;
};

class ArcKnotHolderEntityEnd : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_click(unsigned int state) override;
};

class ArcKnotHolderEntityRX : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_click(unsigned int state) override;
};

class ArcKnotHolderEntityRY : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_click(unsigned int state) override;
};

class ArcKnotHolderEntityCenter : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

/*
 * return values:
 *   1  : inside
 *   0  : on the curves
 *   -1 : outside
 */
static gint
sp_genericellipse_side(SPGenericEllipse *ellipse, Geom::Point const &p)
{
    gdouble dx = (p[Geom::X] - ellipse->cx.computed) / ellipse->rx.computed;
    gdouble dy = (p[Geom::Y] - ellipse->cy.computed) / ellipse->ry.computed;

    gdouble s = dx * dx + dy * dy;
    // We add a bit of a buffer, so there's a decent chance the user will
    // be able to adjust the arc without the closed status flipping between
    // open and closed during micro mouse movements.
    if (s < 0.75) return 1;
    if (s > 1.25) return -1;
    return 0;
}

void
ArcKnotHolderEntityStart::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    int snaps = Inkscape::Preferences::get()->getInt("/options/rotationsnapsperpi/value", 12);

    SPGenericEllipse *arc = dynamic_cast<SPGenericEllipse *>(item);
    g_assert(arc != nullptr);

    gint side = sp_genericellipse_side(arc, p);
    if(side != 0) { arc->setArcType( (side == -1) ?
                                     SP_GENERIC_ELLIPSE_ARC_TYPE_SLICE :
                                     SP_GENERIC_ELLIPSE_ARC_TYPE_ARC); }

    Geom::Point delta = p - Geom::Point(arc->cx.computed, arc->cy.computed);
    Geom::Scale sc(arc->rx.computed, arc->ry.computed);

    double offset = arc->start - atan2(delta * sc.inverse());
    arc->start -= offset;

    if ((state & GDK_CONTROL_MASK) && snaps) {
        double snaps_radian = M_PI/snaps;
        arc->start = std::round(arc->start/snaps_radian) * snaps_radian;
    }
    if (state & GDK_SHIFT_MASK) {
        arc->end -= offset;
    }

    arc->normalize();
    requestDisplayUpdate(arc);
}

Geom::Point
ArcKnotHolderEntityStart::knot_get() const
{
    SPGenericEllipse const *ge = dynamic_cast<SPGenericEllipse const *>(item);
    g_assert(ge != nullptr);

    return ge->getPointAtAngle(ge->start);
}

void
ArcKnotHolderEntityStart::knot_click(unsigned int state)
{
    SPGenericEllipse *ge = dynamic_cast<SPGenericEllipse *>(item);
    g_assert(ge != nullptr);

    if (state & GDK_SHIFT_MASK) {
        ge->end = ge->start = 0;
        ge->updateRepr();
    }
}

void
ArcKnotHolderEntityEnd::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    int snaps = Inkscape::Preferences::get()->getInt("/options/rotationsnapsperpi/value", 12);

    SPGenericEllipse *arc = dynamic_cast<SPGenericEllipse *>(item);
    g_assert(arc != nullptr);

    gint side = sp_genericellipse_side(arc, p);
    if(side != 0) { arc->setArcType( (side == -1) ?
                                     SP_GENERIC_ELLIPSE_ARC_TYPE_SLICE :
                                     SP_GENERIC_ELLIPSE_ARC_TYPE_ARC); }

    Geom::Point delta = p - Geom::Point(arc->cx.computed, arc->cy.computed);
    Geom::Scale sc(arc->rx.computed, arc->ry.computed);

    double offset = arc->end - atan2(delta * sc.inverse());
    arc->end -= offset;

    if ((state & GDK_CONTROL_MASK) && snaps) {
        double snaps_radian = M_PI/snaps;
        arc->end = std::round(arc->end/snaps_radian) * snaps_radian;
    }
    if (state & GDK_SHIFT_MASK) {
        arc->start -= offset;
    }

    arc->normalize();
    requestDisplayUpdate(arc);
}

Geom::Point
ArcKnotHolderEntityEnd::knot_get() const
{
    SPGenericEllipse const *ge = dynamic_cast<SPGenericEllipse const *>(item);
    g_assert(ge != nullptr);

    return ge->getPointAtAngle(ge->end);
}


void
ArcKnotHolderEntityEnd::knot_click(unsigned int state)
{
    SPGenericEllipse *ge = dynamic_cast<SPGenericEllipse *>(item);
    g_assert(ge != nullptr);

    if (state & GDK_SHIFT_MASK) {
        ge->end = ge->start = 0;
        ge->updateRepr();
    }
}


void
ArcKnotHolderEntityRX::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    SPGenericEllipse *ge = dynamic_cast<SPGenericEllipse *>(item);
    g_assert(ge != nullptr);

    Geom::Point const s = snap_knot_position(p, state);

    ge->rx = fabs( ge->cx.computed - s[Geom::X] );

    if ( state & GDK_CONTROL_MASK ) {
        ge->ry = ge->rx.computed;
    }

    requestDisplayUpdate(item);
}

Geom::Point
ArcKnotHolderEntityRX::knot_get() const
{
    SPGenericEllipse const *ge = dynamic_cast<SPGenericEllipse const *>(item);
    g_assert(ge != nullptr);

    return (Geom::Point(ge->cx.computed, ge->cy.computed) -  Geom::Point(ge->rx.computed, 0));
}

void
ArcKnotHolderEntityRX::knot_click(unsigned int state)
{
    SPGenericEllipse *ge = dynamic_cast<SPGenericEllipse *>(item);
    g_assert(ge != nullptr);

    if (state & GDK_CONTROL_MASK) {
        ge->ry = ge->rx.computed;
        ge->updateRepr();
    }
}

void
ArcKnotHolderEntityRY::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    SPGenericEllipse *ge = dynamic_cast<SPGenericEllipse *>(item);
    g_assert(ge != nullptr);

    Geom::Point const s = snap_knot_position(p, state);

    ge->ry = fabs( ge->cy.computed - s[Geom::Y] );

    if ( state & GDK_CONTROL_MASK ) {
        ge->rx = ge->ry.computed;
    }

    requestDisplayUpdate(item);
}

Geom::Point
ArcKnotHolderEntityRY::knot_get() const
{
    SPGenericEllipse const *ge = dynamic_cast<SPGenericEllipse *>(item);
    g_assert(ge != nullptr);

    return (Geom::Point(ge->cx.computed, ge->cy.computed) -  Geom::Point(0, ge->ry.computed));
}

void
ArcKnotHolderEntityRY::knot_click(unsigned int state)
{
    SPGenericEllipse *ge = dynamic_cast<SPGenericEllipse *>(item);
    g_assert(ge != nullptr);

    if (state & GDK_CONTROL_MASK) {
        ge->rx = ge->ry.computed;
        ge->updateRepr();
    }
}

void
ArcKnotHolderEntityCenter::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    SPGenericEllipse *ge = dynamic_cast<SPGenericEllipse *>(item);
    g_assert(ge != nullptr);

    Geom::Point const s = snap_knot_position(p, state);

    ge->cx = s[Geom::X];
    ge->cy = s[Geom::Y];

    requestDisplayUpdate(item);
}

Geom::Point
ArcKnotHolderEntityCenter::knot_get() const
{
    SPGenericEllipse const *ge = dynamic_cast<SPGenericEllipse *>(item);
    g_assert(ge != nullptr);

    return Geom::Point(ge->cx.computed, ge->cy.computed);
}


ArcKnotHolder::ArcKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler) :
    KnotHolder(desktop, item, relhandler)
{
    ArcKnotHolderEntityRX *entity_rx = new ArcKnotHolderEntityRX();
    ArcKnotHolderEntityRY *entity_ry = new ArcKnotHolderEntityRY();
    ArcKnotHolderEntityStart *entity_start = new ArcKnotHolderEntityStart();
    ArcKnotHolderEntityEnd *entity_end = new ArcKnotHolderEntityEnd();
    ArcKnotHolderEntityCenter *entity_center = new ArcKnotHolderEntityCenter();

    entity_rx->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Arc:rx",
                      _("Adjust ellipse <b>width</b>, with <b>Ctrl</b> to make circle"));

    entity_ry->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Arc:ry",
                      _("Adjust ellipse <b>height</b>, with <b>Ctrl</b> to make circle"));

    entity_start->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_ROTATE, "Arc:start",
                         _("Position the <b>start point</b> of the arc or segment; with <b>Shift</b> to move "
                           "with <b>end point</b>; with <b>Ctrl</b> to snap angle; drag <b>inside</b> the "
                           "ellipse for arc, <b>outside</b> for segment"));

    entity_end->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_ROTATE, "Arc:end",
                       _("Position the <b>end point</b> of the arc or segment; with <b>Shift</b> to move "
                         "with <b>start point</b>; with <b>Ctrl</b> to snap angle; drag <b>inside</b> the "
                         "ellipse for arc, <b>outside</b> for segment"));

    entity_center->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_POINT, "Arc:center",
                          _("Drag to move the ellipse"));

    entity.push_back(entity_rx);
    entity.push_back(entity_ry);
    entity.push_back(entity_start);
    entity.push_back(entity_end);
    entity.push_back(entity_center);

    add_pattern_knotholder();
    add_hatch_knotholder();
}

/* SPStar */

class StarKnotHolderEntity1 : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_click(unsigned int state) override;
};

class StarKnotHolderEntity2 : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_click(unsigned int state) override;
};

class StarKnotHolderEntityCenter : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

void
StarKnotHolderEntity1::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    SPStar *star = dynamic_cast<SPStar *>(item);
    g_assert(star != nullptr);

    Geom::Point const s = snap_knot_position(p, state);

    Geom::Point d = s - star->center;

    double arg1 = atan2(d);
    double darg1 = arg1 - star->arg[0];

    if (state & GDK_MOD1_MASK) {
        star->randomized = darg1/(star->arg[0] - star->arg[1]);
    } else if (state & GDK_SHIFT_MASK) {
        star->rounded = darg1/(star->arg[0] - star->arg[1]);
    } else if (state & GDK_CONTROL_MASK) {
        star->r[0]    = L2(d);
    } else {
        star->r[0]    = L2(d);
        star->arg[0]  = arg1;
        star->arg[1] += darg1;
    }
    requestDisplayUpdate(star);
}

void
StarKnotHolderEntity2::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    SPStar *star = dynamic_cast<SPStar *>(item);
    g_assert(star != nullptr);

    Geom::Point const s = snap_knot_position(p, state);

    if (star->flatsided == false) {
        Geom::Point d = s - star->center;

        double arg1 = atan2(d);
        double darg1 = arg1 - star->arg[1];

        if (state & GDK_MOD1_MASK) {
            star->randomized = darg1/(star->arg[0] - star->arg[1]);
        } else if (state & GDK_SHIFT_MASK) {
            star->rounded = fabs(darg1/(star->arg[0] - star->arg[1]));
        } else if (state & GDK_CONTROL_MASK) {
            star->r[1]   = L2(d);
            star->arg[1] = star->arg[0] + M_PI / star->sides;
        }
        else {
            star->r[1]   = L2(d);
            star->arg[1] = atan2(d);
        }
        requestDisplayUpdate(star);
    }
}

void
StarKnotHolderEntityCenter::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    SPStar *star = dynamic_cast<SPStar *>(item);
    g_assert(star != nullptr);

    star->center = snap_knot_position(p, state);

    requestDisplayUpdate(item);
}

Geom::Point
StarKnotHolderEntity1::knot_get() const
{
    g_assert(item != nullptr);

    SPStar const *star = dynamic_cast<SPStar const *>(item);
    g_assert(star != nullptr);

    return sp_star_get_xy(star, SP_STAR_POINT_KNOT1, 0);

}

Geom::Point
StarKnotHolderEntity2::knot_get() const
{
    g_assert(item != nullptr);

    SPStar const *star = dynamic_cast<SPStar const *>(item);
    g_assert(star != nullptr);

    return sp_star_get_xy(star, SP_STAR_POINT_KNOT2, 0);
}

Geom::Point
StarKnotHolderEntityCenter::knot_get() const
{
    g_assert(item != nullptr);

    SPStar const *star = dynamic_cast<SPStar const *>(item);
    g_assert(star != nullptr);

    return star->center;
}

static void
sp_star_knot_click(SPItem *item, unsigned int state)
{
    SPStar *star = dynamic_cast<SPStar *>(item);
    g_assert(star != nullptr);

    if (state & GDK_MOD1_MASK) {
        star->randomized = 0;
        star->updateRepr();
    } else if (state & GDK_SHIFT_MASK) {
        star->rounded = 0;
        star->updateRepr();
    } else if (state & GDK_CONTROL_MASK) {
        star->arg[1] = star->arg[0] + M_PI / star->sides;
        star->updateRepr();
    }
}

void
StarKnotHolderEntity1::knot_click(unsigned int state)
{
    sp_star_knot_click(item, state);
}

void
StarKnotHolderEntity2::knot_click(unsigned int state)
{
    sp_star_knot_click(item, state);
}

StarKnotHolder::StarKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler) :
    KnotHolder(desktop, item, relhandler)
{
    SPStar *star = dynamic_cast<SPStar *>(item);
    g_assert(item != nullptr);

    StarKnotHolderEntity1 *entity1 = new StarKnotHolderEntity1();
    entity1->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Star:entity1",
                    _("Adjust the <b>tip radius</b> of the star or polygon; "
                      "with <b>Shift</b> to round; with <b>Alt</b> to randomize"));

    entity.push_back(entity1);

    if (star->flatsided == false) {
        StarKnotHolderEntity2 *entity2 = new StarKnotHolderEntity2();
        entity2->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Star:entity2",
                        _("Adjust the <b>base radius</b> of the star; with <b>Ctrl</b> to keep star rays "
                          "radial (no skew); with <b>Shift</b> to round; with <b>Alt</b> to randomize"));
        entity.push_back(entity2);
    }

    StarKnotHolderEntityCenter *entity_center = new StarKnotHolderEntityCenter();
    entity_center->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_POINT, "Star:center",
                          _("Drag to move the star"));
    entity.push_back(entity_center);

    add_pattern_knotholder();
    add_hatch_knotholder();
}

/* SPSpiral */

class SpiralKnotHolderEntityInner : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_click(unsigned int state) override;
};

class SpiralKnotHolderEntityOuter : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

class SpiralKnotHolderEntityCenter : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};


/*
 * set attributes via inner (t=t0) knot point:
 *   [default] increase/decrease inner point
 *   [shift]   increase/decrease inner and outer arg synchronizely
 *   [control] constrain inner arg to round per PI/4
 */
void
SpiralKnotHolderEntityInner::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int snaps = prefs->getInt("/options/rotationsnapsperpi/value", 12);

    SPSpiral *spiral = dynamic_cast<SPSpiral *>(item);
    g_assert(spiral != nullptr);

    gdouble   dx = p[Geom::X] - spiral->cx;
    gdouble   dy = p[Geom::Y] - spiral->cy;

    gdouble   moved_y = p[Geom::Y] - origin[Geom::Y];

    if (state & GDK_MOD1_MASK) {
        // adjust divergence by vertical drag, relative to rad
        if (spiral->rad > 0) {
            double exp_delta = 0.1*moved_y/(spiral->rad); // arbitrary multiplier to slow it down
            spiral->exp += exp_delta;
            if (spiral->exp < 1e-3)
                spiral->exp = 1e-3;
        }
    } else {
        // roll/unroll from inside
        gdouble   arg_t0;
        spiral->getPolar(spiral->t0, nullptr, &arg_t0);

        gdouble   arg_tmp = atan2(dy, dx) - arg_t0;
        gdouble   arg_t0_new = arg_tmp - floor((arg_tmp+M_PI)/(2.0*M_PI))*2.0*M_PI + arg_t0;
        spiral->t0 = (arg_t0_new - spiral->arg) / (2.0*M_PI*spiral->revo);

        /* round inner arg per PI/snaps, if CTRL is pressed */
        if ( ( state & GDK_CONTROL_MASK )
             && ( fabs(spiral->revo) > SP_EPSILON_2 )
             && ( snaps != 0 ) ) {
            gdouble arg = 2.0*M_PI*spiral->revo*spiral->t0 + spiral->arg;
            double snaps_radian = M_PI/snaps;
            spiral->t0 = (std::round(arg/snaps_radian)*snaps_radian - spiral->arg)/(2.0*M_PI*spiral->revo);
        }

        spiral->t0 = CLAMP(spiral->t0, 0.0, 0.999);
    }

    requestDisplayUpdate(spiral);
}

/*
 * set attributes via outer (t=1) knot point:
 *   [default] increase/decrease revolution factor
 *   [control] constrain inner arg to round per PI/4
 */
void
SpiralKnotHolderEntityOuter::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int snaps = prefs->getInt("/options/rotationsnapsperpi/value", 12);

    SPSpiral *spiral = dynamic_cast<SPSpiral *>(item);
    g_assert(spiral != nullptr);

    gdouble  dx = p[Geom::X] - spiral->cx;
    gdouble  dy = p[Geom::Y] - spiral->cy;

    if (state & GDK_SHIFT_MASK) { // rotate without roll/unroll
        spiral->arg = atan2(dy, dx) - 2.0*M_PI*spiral->revo;
        if (!(state & GDK_MOD1_MASK)) {
            // if alt not pressed, change also rad; otherwise it is locked
            spiral->rad = MAX(hypot(dx, dy), 0.001);
        }
        if ( ( state & GDK_CONTROL_MASK ) && snaps ) {
            double snaps_radian = M_PI/snaps;
            spiral->arg = std::round(spiral->arg/snaps_radian) * snaps_radian;
        }
    } else { // roll/unroll
        // arg of the spiral outer end
        double arg_1;
        spiral->getPolar(1, nullptr, &arg_1);

        // its fractional part after the whole turns are subtracted
        static double _2PI = 2.0 * M_PI;
        double arg_r = arg_1 - std::round(arg_1/_2PI) * _2PI;

        // arg of the mouse point relative to spiral center
        double mouse_angle = atan2(dy, dx);
        if (mouse_angle < 0)
            mouse_angle += _2PI;

        // snap if ctrl
        if ( ( state & GDK_CONTROL_MASK ) && snaps ) {
            double snaps_radian = M_PI/snaps;
            mouse_angle = std::round(mouse_angle/snaps_radian) * snaps_radian;
        }

        // by how much we want to rotate the outer point
        double diff = mouse_angle - arg_r;
        if (diff > M_PI)
            diff -= _2PI;
        else if (diff < -M_PI)
            diff += _2PI;

        // calculate the new rad;
        // the value of t corresponding to the angle arg_1 + diff:
        double t_temp = ((arg_1 + diff) - spiral->arg)/(_2PI*spiral->revo);
        // the rad at that t:
        double rad_new = 0;
        if (t_temp > spiral->t0)
            spiral->getPolar(t_temp, &rad_new, nullptr);

        // change the revo (converting diff from radians to the number of turns)
        spiral->revo += diff/(2*M_PI);
        if (spiral->revo < 1e-3)
            spiral->revo = 1e-3;

        // if alt not pressed and the values are sane, change the rad
        if (!(state & GDK_MOD1_MASK) && rad_new > 1e-3 && rad_new/spiral->rad < 2) {
            // adjust t0 too so that the inner point stays unmoved
            double r0;
            spiral->getPolar(spiral->t0, &r0, nullptr);
            spiral->rad = rad_new;
            spiral->t0 = pow(r0 / spiral->rad, 1.0/spiral->exp);
        }
        if (!std::isfinite(spiral->t0)) spiral->t0 = 0.0;
        spiral->t0 = CLAMP(spiral->t0, 0.0, 0.999);
    }

    requestDisplayUpdate(spiral);
}

void
SpiralKnotHolderEntityCenter::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    SPSpiral *spiral = dynamic_cast<SPSpiral *>(item);
    g_assert(spiral != nullptr);

    Geom::Point const s = snap_knot_position(p, state);

    spiral->cx = s[Geom::X];
    spiral->cy = s[Geom::Y];

    requestDisplayUpdate(spiral);
}

Geom::Point
SpiralKnotHolderEntityInner::knot_get() const
{
    SPSpiral const *spiral = dynamic_cast<SPSpiral const *>(item);
    g_assert(spiral != nullptr);

    return spiral->getXY(spiral->t0);
}

Geom::Point
SpiralKnotHolderEntityOuter::knot_get() const
{
    SPSpiral const *spiral = dynamic_cast<SPSpiral const *>(item);
    g_assert(spiral != nullptr);

    return spiral->getXY(1.0);
}

Geom::Point
SpiralKnotHolderEntityCenter::knot_get() const
{
    SPSpiral const *spiral = dynamic_cast<SPSpiral const *>(item);
    g_assert(spiral != nullptr);

    return Geom::Point(spiral->cx, spiral->cy);
}

void
SpiralKnotHolderEntityInner::knot_click(unsigned int state)
{
    SPSpiral *spiral = dynamic_cast<SPSpiral *>(item);
    g_assert(spiral != nullptr);

    if (state & GDK_MOD1_MASK) {
        spiral->exp = 1;
        spiral->updateRepr();
    } else if (state & GDK_SHIFT_MASK) {
        spiral->t0 = 0;
        spiral->updateRepr();
    }
}

SpiralKnotHolder::SpiralKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler) :
    KnotHolder(desktop, item, relhandler)
{
    SpiralKnotHolderEntityCenter *entity_center = new SpiralKnotHolderEntityCenter();
    SpiralKnotHolderEntityInner *entity_inner = new SpiralKnotHolderEntityInner();
    SpiralKnotHolderEntityOuter *entity_outer = new SpiralKnotHolderEntityOuter();

    // NOTE: entity_central and entity_inner can overlap.
    //
    // In that case it would be a problem if the center control point was ON
    // TOP because it would steal the mouse focus and the user would loose the
    // ability to access the inner control point using only the mouse.
    //
    // However if the inner control point is ON TOP, taking focus, the
    // situation is a lot better: the user can still move the inner control
    // point with the mouse to regain access to the center control point.
    //
    // So, create entity_inner AFTER entity_center; this ensures that
    // entity_inner gets rendered ON TOP.
    entity_center->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_POINT, "Spiral:center",
                          _("Drag to move the spiral"));

    entity_inner->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Spiral:inner",
                         _("Roll/unroll the spiral from <b>inside</b>; with <b>Ctrl</b> to snap angle; "
                           "with <b>Alt</b> to converge/diverge"));

    entity_outer->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Spiral:outer",
                         _("Roll/unroll the spiral from <b>outside</b>; with <b>Ctrl</b> to snap angle; "
                           "with <b>Shift</b> to scale/rotate; with <b>Alt</b> to lock radius"));

    entity.push_back(entity_center);
    entity.push_back(entity_inner);
    entity.push_back(entity_outer);

    add_pattern_knotholder();
    add_hatch_knotholder();
}

/* SPOffset */

class OffsetKnotHolderEntity : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

void
OffsetKnotHolderEntity::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    SPOffset *offset = dynamic_cast<SPOffset *>(item);
    g_assert(offset != nullptr);

    Geom::Point const p_snapped = snap_knot_position(p, state);

    offset->rad = sp_offset_distance_to_original(offset, p_snapped);
    offset->knot = p_snapped;
    offset->knotSet = true;

    requestDisplayUpdate(offset);
}


Geom::Point
OffsetKnotHolderEntity::knot_get() const
{
    SPOffset const *offset = dynamic_cast<SPOffset const *>(item);
    g_assert(offset != nullptr);

    Geom::Point np;
    sp_offset_top_point(offset,&np);
    return np;
}

OffsetKnotHolder::OffsetKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler) :
    KnotHolder(desktop, item, relhandler)
{
    OffsetKnotHolderEntity *entity_offset = new OffsetKnotHolderEntity();
    entity_offset->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Offset:entity",
                          _("Adjust the <b>offset distance</b>"));
    entity.push_back(entity_offset);

    add_pattern_knotholder();
    add_hatch_knotholder();
}


/* SPText */
class TextKnotHolderEntityInlineSize : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_click(unsigned int state) override;
};

Geom::Point
TextKnotHolderEntityInlineSize::knot_get() const
{
    SPText *text = dynamic_cast<SPText *>(item);
    g_assert(text != nullptr);

    SPStyle* style = text->style;
    double inline_size = style->inline_size.computed;
    unsigned mode      = style->writing_mode.computed;
    unsigned anchor    = style->text_anchor.computed;
    unsigned direction = style->direction.computed;

    Geom::Point p(text->attributes.firstXY());

    if (text->has_inline_size()) {
        // SVG 2 'inline-size'

        // Keep handle at end of text line.
        if (mode == SP_CSS_WRITING_MODE_LR_TB ||
            mode == SP_CSS_WRITING_MODE_RL_TB) {
            // horizontal
            if ( (direction == SP_CSS_DIRECTION_LTR && anchor == SP_CSS_TEXT_ANCHOR_START  ) ||
                 (direction == SP_CSS_DIRECTION_RTL && anchor == SP_CSS_TEXT_ANCHOR_END) ) {
                p *= Geom::Translate (inline_size, 0);
            } else if ( direction == SP_CSS_DIRECTION_LTR && anchor == SP_CSS_TEXT_ANCHOR_MIDDLE) {
                p *= Geom::Translate (inline_size/2.0, 0 );
            } else if ( direction == SP_CSS_DIRECTION_RTL && anchor == SP_CSS_TEXT_ANCHOR_MIDDLE) {
                p *= Geom::Translate (-inline_size/2.0, 0 );
            } else if ( (direction == SP_CSS_DIRECTION_LTR && anchor == SP_CSS_TEXT_ANCHOR_END  ) ||
                        (direction == SP_CSS_DIRECTION_RTL && anchor == SP_CSS_TEXT_ANCHOR_START) ) {
                p *= Geom::Translate (-inline_size, 0);
            }
        } else {
            // vertical
            if (anchor == SP_CSS_TEXT_ANCHOR_START) {
                p *= Geom::Translate (0, inline_size);
            } else if (anchor == SP_CSS_TEXT_ANCHOR_MIDDLE) {
                p *= Geom::Translate (0, inline_size/2.0);
            } else if (anchor == SP_CSS_TEXT_ANCHOR_END) {
                p *= Geom::Translate (0, -inline_size);
            }
        }
    } else {
        // Normal single line text.
        Geom::OptRect bbox = text->geometricBounds(); // Check if this is best.
        if (bbox) {
            if (mode == SP_CSS_WRITING_MODE_LR_TB ||
                mode == SP_CSS_WRITING_MODE_RL_TB) {
                // horizontal
                if ( (direction == SP_CSS_DIRECTION_LTR && anchor == SP_CSS_TEXT_ANCHOR_START  ) ||
                     (direction == SP_CSS_DIRECTION_RTL && anchor == SP_CSS_TEXT_ANCHOR_END) ) {
                    p *= Geom::Translate ((*bbox).width(), 0);
                } else if ( direction == SP_CSS_DIRECTION_LTR && anchor == SP_CSS_TEXT_ANCHOR_MIDDLE) {
                    p *= Geom::Translate ((*bbox).width()/2, 0);
                } else if ( direction == SP_CSS_DIRECTION_RTL && anchor == SP_CSS_TEXT_ANCHOR_MIDDLE) {
                    p *= Geom::Translate (-(*bbox).width()/2, 0);
                } else if ( (direction == SP_CSS_DIRECTION_LTR && anchor == SP_CSS_TEXT_ANCHOR_END  ) ||
                            (direction == SP_CSS_DIRECTION_RTL && anchor == SP_CSS_TEXT_ANCHOR_START) ) {
                    p *= Geom::Translate (-(*bbox).width(), 0);
                }
            } else {
                // vertical
                if (anchor == SP_CSS_TEXT_ANCHOR_START) {
                    p *= Geom::Translate (0, (*bbox).height());
                } else if (anchor == SP_CSS_TEXT_ANCHOR_MIDDLE) {
                    p *= Geom::Translate (0, (*bbox).height()/2);
                } else if (anchor == SP_CSS_TEXT_ANCHOR_END) {
                    p *= Geom::Translate (0, -(*bbox).height());
                }
                p += Geom::Point((*bbox).width(), 0); // Keep on right side
            }
        }
    }

    return p;
}

// Conversion from Inkscape SVG 1.1 to SVG 2 'inline-size'.
void
TextKnotHolderEntityInlineSize::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    SPText *text = dynamic_cast<SPText *>(item);
    g_assert(text != nullptr);

    SPStyle* style = text->style;
    unsigned mode      = style->writing_mode.computed;
    unsigned anchor    = style->text_anchor.computed;
    unsigned direction = style->direction.computed;

    Geom::Point const s = snap_knot_position(p, state);
    Geom::Point delta = s - text->attributes.firstXY();
    double size = 0.0;
    if (mode == SP_CSS_WRITING_MODE_LR_TB ||
        mode == SP_CSS_WRITING_MODE_RL_TB) {
        // horizontal

        size = delta[Geom::X];
        if ( (direction == SP_CSS_DIRECTION_LTR && anchor == SP_CSS_TEXT_ANCHOR_START  ) ||
             (direction == SP_CSS_DIRECTION_RTL && anchor == SP_CSS_TEXT_ANCHOR_END) ) {
            // Do nothing
        } else if ( (direction == SP_CSS_DIRECTION_LTR && anchor == SP_CSS_TEXT_ANCHOR_END  ) ||
                    (direction == SP_CSS_DIRECTION_RTL && anchor == SP_CSS_TEXT_ANCHOR_START) ) {
            size = -size;
        } else if ( anchor == SP_CSS_TEXT_ANCHOR_MIDDLE) {
            size = 2.0 * abs(size);
        } else {
            std::cerr << "TextKnotHolderEntityInlinSize: Should not be reached!" << std::endl;
        }

    } else {
        // vertical

        size = delta[Geom::Y];
        if (anchor == SP_CSS_TEXT_ANCHOR_START) {
            // Do nothing
        } else if (anchor == SP_CSS_TEXT_ANCHOR_END) {
            size = -size;
        } else if (anchor == SP_CSS_TEXT_ANCHOR_MIDDLE) {
            size = 2.0 * abs(size);
        }
    }

    // Size should never be negative
    if (size < 0.0) {
        size = 0.0;
    }

    // Set 'inline-size'.
    text->style->inline_size.setDouble(size);
    text->style->inline_size.set = true;

    // Ensure we respect new lines.
    text->style->white_space.read("pre");
    text->style->white_space.set = true;

    // Convert sodipodi:role="line" to '\n'.
    text->sodipodi_to_newline();

    requestDisplayUpdate(text);
    text->updateRepr();
}

// Conversion from SVG 2 'inline-size' to Inkscape's SVG 1.1.
void
TextKnotHolderEntityInlineSize::knot_click(unsigned int state)
{
    SPText *text = dynamic_cast<SPText *>(item);
    g_assert(text != nullptr);

    if (state & GDK_CONTROL_MASK) {

        text->style->inline_size.clear();
        text->remove_svg11_fallback();   // Else 'x' and 'y' will be interpreted as absolute positions.
        text->newline_to_sodipodi();     // Convert '\n' to tspans with sodipodi:role="line".

        requestDisplayUpdate(text);
        text->updateRepr();
    }
}

/**
 * Shape padding editor knot positioned top right corner of first object
 */
class TextKnotHolderEntityShapePadding : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

Geom::Point
TextKnotHolderEntityShapePadding::knot_get() const
{
    SPText *text = dynamic_cast<SPText *>(item);
    g_assert(text != nullptr);
    Geom::Point corner;
    if (text->has_shape_inside()) {
        auto shape = text->get_first_shape_dependency();
        Geom::OptRect bounds = shape->geometricBounds();
        if (bounds) {
            corner = (*bounds).corner(1);
            if (text->style->shape_padding.set) {
                auto padding = text->style->shape_padding.computed;
                corner *= Geom::Affine(Geom::Translate(-padding, padding));
            }
            corner *= shape->transform;
        }
    }
    return corner;
}

void
TextKnotHolderEntityShapePadding::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    // Text in a shape: rectangle
    SPText *text = dynamic_cast<SPText *>(item);

    if (text->has_shape_inside()) {
        auto shape = text->get_first_shape_dependency();
        Geom::OptRect bounds = shape->geometricBounds();
        if (bounds) {
            Geom::Point const point_a = snap_knot_position(p, state);
            Geom::Point point_b = point_a * shape->transform.inverse();
            auto padding = (*bounds).corner(1)[Geom::X] - point_b[Geom::X];
            gchar *pad = g_strdup_printf("%f", padding);
            text->style->shape_padding.read(pad);
            g_free(pad);

            requestDisplayUpdate(text);
            text->updateRepr();
        }
    }
}


/**
 * Shape margin editor knot positioned top right corner of each object
 */
class TextKnotHolderEntityShapeMargin : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void set_shape(SPShape *shape) { linked_shape = shape; }
    SPShape *linked_shape;
};

Geom::Point
TextKnotHolderEntityShapeMargin::knot_get() const
{
    Geom::Point corner;
    if (linked_shape == nullptr) return corner;
    
    Geom::OptRect bounds = linked_shape->geometricBounds();
    if (bounds) {
        corner = (*bounds).corner(1);
        if (linked_shape->style->shape_margin.set) {
            auto margin = linked_shape->style->shape_margin.computed;
            corner *= Geom::Affine(Geom::Translate(margin, -margin));
        }
        corner *= linked_shape->transform;
    }
    return corner;
}

void
TextKnotHolderEntityShapeMargin::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    g_assert(linked_shape != nullptr);

    Geom::OptRect bounds = linked_shape->geometricBounds();
    if (bounds) {
        Geom::Point const point_a = snap_knot_position(p, state);
        Geom::Point point_b = point_a * linked_shape->transform.inverse();
        auto margin = (*bounds).corner(1)[Geom::X] - point_b[Geom::X];
        gchar *pad = g_strdup_printf("%f", -margin);
        linked_shape->style->shape_margin.read(pad);
        g_free(pad);

        requestDisplayUpdate(linked_shape);
        linked_shape->updateRepr();
    }
}




class TextKnotHolderEntityShapeInside : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

Geom::Point
TextKnotHolderEntityShapeInside::knot_get() const
{
    // SVG 2 'shape-inside'. We only get here if there is a rectangle shape.
    SPText *text = dynamic_cast<SPText *>(item);
    g_assert(text != nullptr);
    // we have a crash on undo cration so remove assert
    // g_assert(text->style->shape_inside.set);
    Geom::Point p;
    if (text->has_shape_inside()) {
        Geom::OptRect frame = text->get_frame();
        if (frame) {
            p = (*frame).corner(2);
        } else {
            std::cerr << "TextKnotHolderEntityShapeInside::knot_get(): no frame!" << std::endl;
        }
    }
    return p;
}

void
TextKnotHolderEntityShapeInside::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    // Text in a shape: rectangle
    SPText *text = dynamic_cast<SPText *>(item);
    g_assert(text != nullptr);
    g_assert(text->style->shape_inside.set);

    Geom::Point const s = snap_knot_position(p, state);

    Inkscape::XML::Node* rectangle = text->get_first_rectangle();
    double x = rectangle->getAttributeDouble("x", 0.0);;
    double y = rectangle->getAttributeDouble("y", 0.0);
    double width  = s[Geom::X] - x;
    double height = s[Geom::Y] - y;
    rectangle->setAttributeSvgDouble("width", width);
    rectangle->setAttributeSvgDouble("height", height);
    requestDisplayUpdate(text);
    text->updateRepr();
}

TextKnotHolder::TextKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler) :
    KnotHolder(desktop, item, relhandler)
{
    SPText *text = dynamic_cast<SPText *>(item);
    g_assert(text != nullptr);

    if (text->style->shape_inside.set) {
        // 'shape-inside'

        if (text->get_first_rectangle()) {
            auto entity_shapeinside = new TextKnotHolderEntityShapeInside();
            entity_shapeinside->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Text:shapeinside",
                                       _("Adjust the <b>rectangular</b> region of the text."));
            entity.push_back(entity_shapeinside);
        }

        auto entity_shapepadding = new TextKnotHolderEntityShapePadding();
        entity_shapepadding->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Text:shapepadding",
                                    _("Adjust the text <b>shape padding</b>."));
        entity.push_back(entity_shapepadding);

        // Add knots for shape subtraction margins
        if (text->style->shape_subtract.set) {
            for (auto *href : text->style->shape_subtract.hrefs) {
                auto shape = href->getObject();
                if (dynamic_cast<SPShape *>(shape)) {
                    auto entity_shapemargin = new TextKnotHolderEntityShapeMargin();
                    entity_shapemargin->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Text:shapemargin",
                                                _("Adjust the shape's <b>text margin</b>."));
                    entity_shapemargin->set_shape(shape);
                    entity_shapemargin->update_knot();
                    entity.push_back(entity_shapemargin);
                }
            }
        }

    } else {
        // 'inline-size' or normal text
        TextKnotHolderEntityInlineSize *entity_inlinesize = new TextKnotHolderEntityInlineSize();

        entity_inlinesize->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Text:inlinesize",
                                  _("Adjust the <b>inline size</b> (line length) of the text."));
        entity.push_back(entity_inlinesize);
    }

    add_pattern_knotholder();
    add_hatch_knotholder();
}


// TODO: this is derived from RectKnotHolderEntityWH because it used the same static function
// set_internal as the latter before KnotHolderEntity was C++ified. Check whether this also makes
// sense logically.
class FlowtextKnotHolderEntity : public RectKnotHolderEntityWH {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

Geom::Point
FlowtextKnotHolderEntity::knot_get() const
{
    SPRect const *rect = dynamic_cast<SPRect const *>(item);
    g_assert(rect != nullptr);

    return Geom::Point(rect->x.computed + rect->width.computed, rect->y.computed + rect->height.computed);
}

void
FlowtextKnotHolderEntity::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state)
{
    set_internal(p, origin, state);
}

FlowtextKnotHolder::FlowtextKnotHolder(SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler) :
    KnotHolder(desktop, item, relhandler)
{
    g_assert(item != nullptr);

    FlowtextKnotHolderEntity *entity_flowtext = new FlowtextKnotHolderEntity();
    entity_flowtext->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "FlowText:entity",
                            _("Drag to resize the <b>flowed text frame</b>"));
    entity.push_back(entity_flowtext);
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
