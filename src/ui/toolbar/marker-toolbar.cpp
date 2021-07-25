// SPDX-License-Identifier: GPL-2.0-or-later
// Released under GNU GPL v2+, read the file 'COPYING' for more information.

#include <glibmm/i18n.h>
#include "marker-toolbar.h"
#include "document-undo.h"
#include "preferences.h"
#include "desktop.h"
#include "ui/widget/canvas.h"
namespace Inkscape {
namespace UI {
namespace Toolbar {

MarkerToolbar::MarkerToolbar(SPDesktop *desktop)
    : Toolbar(desktop)
{
    add_label(_("Marker Edit Mode"));
    show_all();
}

GtkWidget* MarkerToolbar::create(SPDesktop *desktop)
{
    auto toolbar = Gtk::manage(new MarkerToolbar(desktop));
    return GTK_WIDGET(toolbar->gobj());
}

}}}