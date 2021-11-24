// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * An internal raster export which passes the generated PNG output
 * to an external file. In the future this module could host more of
 * the PNG generation code that isn't needed for other raster export options.
 *
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "png-output.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <glibmm.h>
#include <giomm.h>

#include "clear-n_.h"

namespace Inkscape {
namespace Extension {
namespace Internal {

void PngOutput::export_raster(Inkscape::Extension::Output * /*module*/,
        const SPDocument * /*doc*/, std::string const &png_file, gchar const *filename)
{
    // We want to move the png file to the new location
    Glib::RefPtr<Gio::File> input_fn = Gio::File::create_for_path(png_file);
    Glib::RefPtr<Gio::File> output_fn = Gio::File::create_for_path(filename);
    try {
        // NOTE: if the underlying filesystem doesn't support moving
        //       GIO will fall back to a copy and remove operation.
        input_fn->move(output_fn, Gio::FILE_COPY_OVERWRITE);
    }
    catch (const Gio::Error& e) {
        std::cerr << "Moving resource " << png_file
                  << " to "             << filename
                  << " failed: "        << e.what() << std::endl;
    }
}

void PngOutput::init()
{
    // clang-format off
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("Portable Network Graphic") "</name>\n"
            "<id>org.inkscape.output.png.inkscape</id>\n"
            "<output raster=\"true\">\n"
                "<extension>.png</extension>\n"
                "<mimetype>image/png</mimetype>\n"
                "<filetypename>" N_("Portable Network Graphic (*.png)") "</filetypename>\n"
                "<filetypetooltip>" N_("Default raster graphic export") "</filetypetooltip>\n"
            "</output>\n"
        "</inkscape-extension>",
        new PngOutput());
    // clang-format on
}

} // namespace Internal
} // namespace Extension
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
