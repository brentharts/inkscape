// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::UI::Widget::PageSelector - page selector widget
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2004 MenTaLguY
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_WIDGETS_PAGE_SELECTOR
#define SEEN_INKSCAPE_WIDGETS_PAGE_SELECTOR

#include <gtkmm/box.h>
#include <gtkmm/combobox.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/cellrenderertext.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/liststore.h>
#include <sigc++/slot.h>

#include "object/sp-page.h"

class SPDesktop;
class SPPage;

namespace Inkscape {
    class PageManager;
namespace UI {
namespace Widget {

//class DocumentTreeModel;

class PageSelector : public Gtk::Box {
    public:
        PageSelector(SPDesktop *desktop = nullptr);
        ~PageSelector() override;
    private:
        class PageModelColumns : public Gtk::TreeModel::ColumnRecord {
            public:
                Gtk::TreeModelColumn<SPPage*> object;

                PageModelColumns() {
                    add(object);
                }
        };

        SPDesktop *_desktop;
        Inkscape::PageManager *_page_manager;

        std::vector<sigc::connection> _connections;

        Gtk::ComboBox _selector;
        Gtk::Button _prev_button;
        Gtk::Button _next_button;

        PageModelColumns _model_columns;
        Gtk::CellRendererText _label_renderer;
        Glib::RefPtr<Gtk::ListStore> _page_model;

        sigc::connection _changed_connection;
        sigc::connection _resource_connection;
        sigc::connection _doc_replaced;
        sigc::connection _prev_page_connection;
        sigc::connection _next_page_connection;

        void setDocument(SPDocument *document);
        void pagesChanged();

        void nextPage();
        void prevPage();

        void renderPageLabel(Gtk::TreeModel::const_iterator const &row);
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif
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
