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
#include "sp-namedview.h"
#include "sp-root.h"

using Inkscape::DocumentUndo;

SPPage::SPPage()
    : SPObject()
    , label(nullptr)
    , color(0x0000ff7f)
    , hicolor(0xff00007f)
{}

void SPPage::setColor(guint32 color)
{
    this->color = color;
    for (auto view : views) {
        view->set_stroke(color);
    }
}

void SPPage::build(SPDocument *document, Inkscape::XML::Node *repr)
{
    SPObject::build(document, repr);

    this->readAttr(SPAttr::INKSCAPE_LABEL);
    this->readAttr(SPAttr::POSITION);
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
    case SPAttr::INKSCAPE_COLOR:
        if (value) {
            this->setColor(sp_svg_read_color(value, 0x0000ff00) | 0x7f);
        }
        break;
    case SPAttr::INKSCAPE_LABEL:
        // this->label already freed in sp_guideline_set_label (src/display/guideline.cpp)
        // see bug #1498444, bug #1469514
        if (value) {
            this->label = g_strdup(value);
        } else {
            this->label = nullptr;
        }

        this->set_label(this->label, false);
        break;
    case SPAttr::INKSCAPE_LOCKED:
        if (value) {
            this->set_locked(helperfns_read_bool(value, false), false);
        }
        break;
    case SPAttr::ORIENTATION:
    {
        if (value && !strcmp(value, "horizontal")) {
            /* Visual representation of a horizontal line, constrain vertically (y coordinate). */
            this->normal_to_line = Geom::Point(0., 1.);
        } else if (value && !strcmp(value, "vertical")) {
            this->normal_to_line = Geom::Point(1., 0.);
        } else if (value) {
            gchar ** strarray = g_strsplit(value, ",", 2);
            double newx, newy;
            unsigned int success = sp_svg_number_read_d(strarray[0], &newx);
            success += sp_svg_number_read_d(strarray[1], &newy);
            g_strfreev (strarray);
            if (success == 2 && (fabs(newx) > 1e-6 || fabs(newy) > 1e-6)) {
                Geom::Point direction(newx, newy);

                // <sodipodi:guide> stores inverted y-axis coordinates
                if (document->is_yaxisdown()) {
                    direction[Geom::X] *= -1.0;
                }

                direction.normalize();
                this->normal_to_line = direction;
            } else {
                // default to vertical line for bad arguments
                this->normal_to_line = Geom::Point(1., 0.);
            }
        } else {
            // default to vertical line for bad arguments
            this->normal_to_line = Geom::Point(1., 0.);
        }
        this->set_normal(this->normal_to_line, false);
    }
    break;
    case SPAttr::POSITION:
    {
        if (value) {
            gchar ** strarray = g_strsplit(value, ",", 2);
            double newx, newy;
            unsigned int success = sp_svg_number_read_d(strarray[0], &newx);
            success += sp_svg_number_read_d(strarray[1], &newy);
            g_strfreev (strarray);
            if (success == 2) {
                // If root viewBox set, interpret guides in terms of viewBox (90/96)
                SPRoot *root = document->getRoot();
                if( root->viewBox_set ) {
                    if(Geom::are_near((root->width.computed * root->viewBox.height()) / (root->viewBox.width() * root->height.computed), 1.0, Geom::EPSILON)) {
                        // for uniform scaling, try to reduce numerical error
                        double vbunit2px = (root->width.computed / root->viewBox.width() + root->height.computed / root->viewBox.height())/2.0;
                        newx = newx * vbunit2px;
                        newy = newy * vbunit2px;
                    } else {
                        newx = newx * root->width.computed  / root->viewBox.width();
                        newy = newy * root->height.computed / root->viewBox.height();
                    }
                }
                this->point_on_line = Geom::Point(newx, newy);
            } else if (success == 1) {
                // before 0.46 style guideline definition.
                const gchar *attr = this->getRepr()->attribute("orientation");
                if (attr && !strcmp(attr, "horizontal")) {
                    this->point_on_line = Geom::Point(0, newx);
                } else {
                    this->point_on_line = Geom::Point(newx, 0);
                }
            }

            // <sodipodi:guide> stores inverted y-axis coordinates
            if (document->is_yaxisdown()) {
                this->point_on_line[Geom::Y] = document->getHeight().value("px") - this->point_on_line[Geom::Y];
            }
        } else {
            // default to (0,0) for bad arguments
            this->point_on_line = Geom::Point(0,0);
        }
        // update position in non-committing way
        // fixme: perhaps we need to add an update method instead, and request_update here
        this->moveto(this->point_on_line, false);
    }
    break;
    default:
    	SPObject::set(key, value);
        break;
    }
}

void SPPage::setColor(const unsigned r, const unsigned g, const unsigned b, bool const commit)
{
    if (! views.empty()) {
        views[0]->set_stroke(color);
    }
    if (commit) {
        setAttribute("inkscape:color", os.str());
    }
}

void SPPage::setLabel(const char* label, bool const commit)
{
    if (!views.empty()) {
        views[0]->set_label(label);
    }
    if (commit) {
        setAttribute("inkscape:label", label);
    }
}

char* SPPage::description(bool const verbose) const
{
    return "Page";
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
