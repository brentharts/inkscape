// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for pages, mostly for the toolbar.
 *
 * Copyright (C) 2021 Martin Owens
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <iostream>

#include <giomm.h>
#include <glibmm/i18n.h>

#include "actions-pages.h"
#include "inkscape-application.h"

#include "page-manager.h"
#include "object/sp-page.h"

void page_new(SPDocument *document)
{
    if (auto manager = document->getNamedView()->getPageManager()) {
        manager->newPage();
    }
}

void page_delete(SPDocument *document)
{
    if (auto manager = document->getNamedView()->getPageManager()) {
        manager->deletePage();
    }
}

std::vector<std::vector<Glib::ustring>> raw_data_actions =
{
    // clang-format off
    {"doc.page-new",               N_("New Page"),                "Page",     N_("Create a new page")                                  },
    {"doc.page-delete",            N_("Delete Page"),             "Page",     N_("Delete the selected page")                           },
    // clang-format on
};

void add_actions_pages(SPDocument* doc)
{
    auto group = doc->getActionGroup();
    group->add_action("page-new", sigc::bind<SPDocument*>(sigc::ptr_fun(&page_new), doc));
    group->add_action("page-delete", sigc::bind<SPDocument*>(sigc::ptr_fun(&page_delete), doc));
    //doc->get_action_extra_data().add_data(raw_data_actions);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
