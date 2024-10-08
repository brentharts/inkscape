// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2007 Authors:
 *   Christopher Brown <audiere@gmail.com>
 *   Ted Gould <ted@gould.cx>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "extension/effect.h"
#include "extension/system.h"

#include "adaptiveThreshold.h"
#include <Magick++.h>

namespace Inkscape {
namespace Extension {
namespace Internal {
namespace Bitmap {
	
void
AdaptiveThreshold::applyEffect(Magick::Image *image) {
	image->adaptiveThreshold(_width, _height);
}

void
AdaptiveThreshold::refreshParameters(Inkscape::Extension::Effect *module) {	
	_width = module->get_param_int("width");
	_height = module->get_param_int("height");
	_offset = module->get_param_int("offset");
}

#include "../clear-n_.h"

void
AdaptiveThreshold::init()
{
    // clang-format off
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("Adaptive Threshold") "</name>\n"
            "<id>org.inkscape.effect.bitmap.adaptiveThreshold</id>\n"
            "<param name=\"width\" gui-text=\"" N_("Width:") "\" type=\"int\" min=\"-100\" max=\"100\">5</param>\n"
            "<param name=\"height\" gui-text=\"" N_("Height:") "\" type=\"int\" min=\"-100\" max=\"100\">5</param>\n"
            "<param name=\"offset\" gui-text=\"" N_("Offset:") "\" type=\"int\" min=\"0\" max=\"100\">0</param>\n"
            "<effect>\n"
                "<object-type>all</object-type>\n"
                "<effects-menu>\n"
                    "<submenu name=\"" N_("Raster") "\" />\n"
                "</effects-menu>\n"
                "<menu-tip>" N_("Apply adaptive thresholding to selected bitmap(s)") "</menu-tip>\n"
            "</effect>\n"
        "</inkscape-extension>\n", std::make_unique<AdaptiveThreshold>());
    // clang-format on
}

}; /* namespace Bitmap */
}; /* namespace Internal */
}; /* namespace Extension */
}; /* namespace Inkscape */
