// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011 Authors:
 *   Nicolas Dufour <nicoduf@yahoo.fr>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "2geom/transforms.h"
#include "extension/effect.h"
#include "extension/system.h"

#include "crop.h"
#include "selection-chemistry.h"
#include "object/sp-item.h"
#include "object/sp-rect.h"
#include "object/sp-image.h"
#include "object/sp-clippath.h"
#include "object/sp-item-transform.h"
#include <Magick++.h>

namespace Inkscape {
namespace Extension {
namespace Internal {
namespace Bitmap {

void
Crop::applyEffect(Magick::Image *image) {
    int width = image->baseColumns() - (_left + _right);
    int height = image->baseRows() - (_top + _bottom);
    if (width > 0 and height > 0) {
        image->crop(Magick::Geometry(width, height, _left, _top, false, false));
        image->page("+0+0");
    } else {
        g_warning("Refusing to crop to %d,%d (w:%d, h:%d)", _left, _top, width, height);
    }
}

void
Crop::preEffect(Magick::Image *image, SPItem *item) {
    _auto = false;
    if (_left || _top || _right || _bottom) {
        return;
    }
    if (auto sp_clip = item->getClipObject()) {
        Geom::OptRect item_bbox(item->documentGeometricBounds());
        Geom::OptRect clip_bbox(sp_clip->geometricBounds(item->i2doc_affine()));

        if (item_bbox && clip_bbox) {
            _auto = true;
            _left = clip_bbox->left() - item_bbox->left();
            _top = clip_bbox->top() - item_bbox->top();
            _right = item_bbox->width() - clip_bbox->width() - _left;
            _bottom = item_bbox->height() - clip_bbox->height() - _top;
        }
    }
}

void
Crop::postEffect(Magick::Image *image, SPItem *item)
{
    // Scale bbox
    auto scale = Geom::Scale(image->columns() / (double) image->baseColumns());

    // OK something is very wrong here, when applying both scales, the height ends up
    // squashed, but if we apply the width scale only, it somehow works. We have no idea why.
    // image->rows() / (double) image->baseRows());
    item->scale_rel(scale);

    // Translate proportionaly to the image/bbox ratio
    Geom::OptRect bbox(item->desktopGeometricBounds());

    Geom::Translate translate (0,0);
    translate = Geom::Translate(((_left - _right) / 2.0) * (bbox->dimensions()[Geom::X] / (double) image->columns()),
                                ((_bottom - _top) / 2.0) * (bbox->dimensions()[Geom::Y] / (double) image->rows()));
    item->move_rel(translate);

    if (_auto) {
        // Resetting the x,y coords allows the smaller image to keep the crop
        //auto translate = Geom::Translate(_left, _top);
        if (auto sp_clip = item->getClipObject()) {
            item->setAttribute("clip-path", "none");
            //for (auto& child: sp_clip->childList(false)) {
            //    delete child;
            //}
        }
    }
}

void
Crop::refreshParameters(Inkscape::Extension::Effect *module) {
    _auto = false;
    _top = module->get_param_int("top");
    _bottom = module->get_param_int("bottom");
    _left = module->get_param_int("left");
    _right = module->get_param_int("right");
}

#include "../clear-n_.h"

void
Crop::init()
{
    // clang-format off
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
          "<name>" N_("Crop") "</name>\n"
          "<id>org.inkscape.effect.bitmap.crop</id>\n"
          "<param name=\"top\" gui-text=\"" N_("Top (px):") "\" type=\"int\" min=\"0\" max=\"100000\">0</param>\n"
          "<param name=\"bottom\" gui-text=\"" N_("Bottom (px):") "\" type=\"int\" min=\"0\" max=\"100000\">0</param>\n"
          "<param name=\"left\" gui-text=\"" N_("Left (px):") "\" type=\"int\" min=\"0\" max=\"100000\">0</param>\n"
          "<param name=\"right\" gui-text=\"" N_("Right (px):") "\" type=\"int\" min=\"0\" max=\"100000\">0</param>\n"
          "<effect>\n"
            "<object-type>all</object-type>\n"
            "<effects-menu>\n"
              "<submenu name=\"" N_("Raster") "\" />\n"
            "</effects-menu>\n"
            "<menu-tip>" N_("Crop selected bitmap(s)") "</menu-tip>\n"
          "</effect>\n"
        "</inkscape-extension>\n", new Crop());
    // clang-format on
}

}; /* namespace Bitmap */
}; /* namespace Internal */
}; /* namespace Extension */
}; /* namespace Inkscape */
