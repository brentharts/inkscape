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

#include "page-toolbar.h"

#include <glibmm/i18n.h>
#include <gtkmm.h>

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "io/resource.h"
#include "object/sp-namedview.h"
#include "object/sp-page.h"
#include "ui/tools/pages-tool.h"
#include "util/paper.h"

using Inkscape::IO::Resource::UIS;

namespace Inkscape {
namespace UI {
namespace Toolbar {

PageToolbar::PageToolbar(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &builder, SPDesktop *desktop)
    : Gtk::Toolbar(cobject)
    , _desktop(desktop)
    , combo_page_sizes(nullptr)
    , text_page_label(nullptr)
{
    builder->get_widget("page_sizes", combo_page_sizes);
    builder->get_widget("page_label", text_page_label);

    if (text_page_label) {
        text_page_label->signal_changed().connect(sigc::mem_fun(*this, &PageToolbar::labelEdited));
    }

    if (combo_page_sizes) {
        combo_page_sizes->signal_changed().connect(sigc::mem_fun(*this, &PageToolbar::sizeChoose));
        entry_page_sizes = dynamic_cast<Gtk::Entry *>(combo_page_sizes->get_child());
        if (entry_page_sizes) {
            entry_page_sizes->signal_activate().connect(sigc::mem_fun(*this, &PageToolbar::sizeChanged));
        }
        page_sizes = Inkscape::PaperSize::getPageSizes();
        for (int i = 0; i < page_sizes.size(); i++) {
            combo_page_sizes->append(std::to_string(i), page_sizes[i]->getDescription());
        }
    }

    // Watch for when the tool changes
    _ec_connection = _desktop->connectEventContextChanged(sigc::mem_fun(*this, &PageToolbar::toolChanged));
}

PageToolbar::~PageToolbar()
{
    _ec_connection.disconnect();
}

void PageToolbar::toolChanged(SPDesktop *desktop, Inkscape::UI::Tools::ToolBase *ec)
{
    // Disconnect previous page changed signal
    if (_page_connection) {
        _page_connection.disconnect();
        _document = nullptr;
        _page_manager = nullptr;
    }
    if (dynamic_cast<Inkscape::UI::Tools::PagesTool *>(ec)) {
        // Save the document and page_manager for future use.
        if (_document = desktop->getDocument()) {
            if (_page_manager = _document->getNamedView()->getPageManager()) {
                // Connect the page changed signal and indicate changed
                _page_connection =
                    _page_manager->connectPageSelected(sigc::mem_fun(*this, &PageToolbar::selectionChanged));
                selectionChanged(_page_manager->getSelected());
            }
        }
    }
}

void PageToolbar::labelEdited()
{
    auto text = text_page_label->get_text();
    if (_page_manager) {
        if (auto page = _page_manager->getSelected()) {
            page->setLabel(text.empty() ? nullptr : text.c_str());
        }
    }
}

void PageToolbar::sizeChoose()
{
    try {
        auto page_id = std::stoi(combo_page_sizes->get_active_id());
        if (page_id >= 0 && page_id < page_sizes.size()) {
            auto ps = page_sizes[page_id];
            auto smaller = ps->unit->convert(ps->smaller, "px");
            auto larger = ps->unit->convert(ps->larger, "px");
            _page_manager->resizePage(smaller, larger);
        }
    } catch (std::invalid_argument const &e) {
        // Ignore because user is typing into Entry
    }
}

void PageToolbar::sizeChanged()
{
    auto size = combo_page_sizes->get_active_text();
    entry_page_sizes->set_text("Not Implemented!");
}

void PageToolbar::selectionChanged(SPPage *page)
{
    // Set label widget content with page label.
    if (page) {
        text_page_label->set_sensitive(true);

        gchar *format = g_strdup_printf(_("Page %d"), page->getPageNumber());
        text_page_label->set_placeholder_text(format);
        g_free(format);

        if (auto label = page->label()) {
            text_page_label->set_text(label);
        } else {
            text_page_label->set_text("");
        }
    } else {
        text_page_label->set_text("");
        text_page_label->set_sensitive(false);
        text_page_label->set_placeholder_text(_("No Page Selected"));
    }
    // Set size widget with page size
}

GtkWidget *PageToolbar::create(SPDesktop *desktop)
{
    Glib::ustring page_toolbar_builder_file = get_filename(UIS, "toolbar-page.ui");
    auto builder = Gtk::Builder::create();
    try {
        builder->add_from_file(page_toolbar_builder_file);
    } catch (const Glib::Error &ex) {
        std::cerr << "PageToolbar: " << page_toolbar_builder_file << " file not read! " << ex.what() << std::endl;
    }

    PageToolbar *toolbar = nullptr;
    builder->get_widget_derived("page-toolbar", toolbar, desktop);
    if (!toolbar) {
        std::cerr << "InkscapeWindow: Failed to load page toolbar!" << std::endl;
        return nullptr;
    }

    toolbar->reference(); // Or it will be deleted when builder is destroyed since we haven't added
                          // it to a container yet. This probably causes a memory leak but we'll
                          // fix it when all toolbars are converted to use Gio::Actions.

    return GTK_WIDGET(toolbar->gobj());
}
} // namespace Toolbar
} // namespace UI
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
