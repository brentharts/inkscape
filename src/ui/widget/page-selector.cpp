// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::Widgets::PageSelector - select and move to pages
 *
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2021 Martin Owens
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstring>
#include <string>

#include <glibmm/i18n.h>

#include "page-selector.h"

#include "ui/icon-names.h"
#include "ui/icon-loader.h"
#include "object/sp-page.h"
#include "object/sp-namedview.h"
#include "page-manager.h"
#include "desktop.h"
#include "document.h"

namespace Inkscape {
namespace UI {
namespace Widget {

PageSelector::PageSelector(SPDesktop *desktop)
: Gtk::Box(Gtk::ORIENTATION_HORIZONTAL)
    , _desktop(desktop)
{
    set_name("PageSelector");

    _prev_button.add(*Gtk::manage(sp_get_icon_image(INKSCAPE_ICON("pan-start"), Gtk::ICON_SIZE_MENU)));
    _prev_button.set_relief(Gtk::RELIEF_NONE);
    _prev_button.set_tooltip_text(_("Move to previous page"));
    _prev_page_connection = _prev_button.signal_clicked().connect(sigc::mem_fun(*this, &PageSelector::prevPage));

    _next_button.add(*Gtk::manage(sp_get_icon_image(INKSCAPE_ICON("pan-end"), Gtk::ICON_SIZE_MENU)));
    _next_button.set_relief(Gtk::RELIEF_NONE);
    _next_button.set_tooltip_text(_("Move to next page"));
    _next_page_connection = _next_button.signal_clicked().connect(sigc::mem_fun(*this, &PageSelector::nextPage));

    _selector.set_tooltip_text(_("Current page"));

    _page_model = Gtk::ListStore::create(_model_columns);
    _selector.set_model(_page_model);
    _selector.pack_start(_label_renderer);
    _selector.set_cell_data_func(_label_renderer, sigc::mem_fun(*this, &PageSelector::renderPageLabel));

    pack_start(_prev_button, Gtk::PACK_EXPAND_PADDING);
    pack_start(_selector, Gtk::PACK_EXPAND_WIDGET);
    pack_start(_next_button, Gtk::PACK_EXPAND_PADDING);

    _doc_replaced = _desktop->connectDocumentReplaced(sigc::hide<0>(sigc::mem_fun(*this, &PageSelector::setDocument)));

    setDocument(desktop->getDocument());
}

PageSelector::~PageSelector() {
    _doc_replaced.disconnect();
    setDocument(nullptr);
}

void PageSelector::setDocument(SPDocument *document) {
    if (_page_manager) {
        _page_manager = nullptr;
        _changed_connection.disconnect();
        //_selection_connection.disconnect();
    }
    if (document) {
        _page_manager = document->getNamedView()->getPageManager();
        _changed_connection = _page_manager->connectPagesChanged(sigc::mem_fun(*this, &PageSelector::pagesChanged));
        //_selection_connection = _page_manager->connectSelectionChanged(...);
        pagesChanged();
    }
}

void PageSelector::pagesChanged()
{
    // Destroy all existing pages in the model.
    while (!_page_model->children().empty()) {
        Gtk::ListStore::iterator row(_page_model->children().begin());
        // Put cleanup here if any
        _page_model->erase(row);
    }

    // Add in pages, do not use getResourcelist("page") because the items
    // are not guarenteed to be in node order, they are in first-seen order.
    for (auto &page : _page_manager->getPages()) {
        //_connections.emplace_back(page->connectModified(sigc::mem_fun(*this, &PageSelector::pageModified)));

        Gtk::ListStore::iterator row(_page_model->append());
        row->set_value(_model_columns.object, page);
    }
}

/**
 * Render the page icon into a suitable label.
 */
void PageSelector::renderPageLabel(Gtk::TreeModel::const_iterator const &row)
{
    SPPage *page = (*row)[_model_columns.object];
    bool label_defaulted(false);

    if (page && page->getRepr()) {
        label_defaulted = !(page->label());
        auto label = page->defaultLabel();
        int page_num = page->getPageNumber();

        gchar *format = g_strdup_printf(
              "<span size=\"smaller\"><tt>%d.</tt>%s</span>",
              page_num, label);

        _label_renderer.property_markup() = format;
        g_free(format);
    } else {
        _label_renderer.property_markup() = "⚠️";
    }

    _label_renderer.property_ypad() = 1;
    _label_renderer.property_style() = (label_defaulted ? Pango::STYLE_ITALIC : Pango::STYLE_NORMAL);
}

void PageSelector::nextPage() {
    _page_manager->selectNextPage();
}

void PageSelector::prevPage() {
    _page_manager->selectNextPage();
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99


void PageSelector::nextPage() {
    g_warning("Next Page");
}

void PageSelector::prevPage() {
    g_warning("Previous Page");
}

} // namespace Widget
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
