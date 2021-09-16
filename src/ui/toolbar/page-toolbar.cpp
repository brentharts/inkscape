// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Page aux toolbar: Temp until we convert all toolbars to ui files with Gio::Actions.
 */
/* Authors:
 *   Martin Owens <doctormo@geek-2.com>

 * Copyright (C) 2021 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm.h>

#include "page-toolbar.h"

#include "desktop.h"
#include "io/resource.h"

using Inkscape::IO::Resource::UIS;

namespace Inkscape {
namespace UI {
namespace Toolbar {

GtkWidget *
PageToolbar::create(SPDesktop *desktop)
{
    Glib::ustring page_toolbar_builder_file = get_filename(UIS, "toolbar-page.ui");
    auto builder = Gtk::Builder::create();
    try
    {
        builder->add_from_file(page_toolbar_builder_file);
    }
    catch (const Glib::Error& ex)
    {
        std::cerr << "PageToolbar: " << page_toolbar_builder_file << " file not read! " << ex.what() << std::endl;
    }

    Gtk::Toolbar* toolbar = nullptr;
    builder->get_widget("page-toolbar", toolbar);
    if (!toolbar) {
        std::cerr << "InkscapeWindow: Failed to load page toolbar!" << std::endl;
        return nullptr;
    }

    toolbar->reference(); // Or it will be deleted when builder is destroyed since we haven't added
                          // it to a container yet. This probably causes a memory leak but we'll
                          // fix it when all toolbars are converted to use Gio::Actions.

    // XXX Start building the non-action pieces.
    //builder->get_widget("page_sizes", combo_page_sizes)

    return GTK_WIDGET(toolbar->gobj());
}
}
}
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
