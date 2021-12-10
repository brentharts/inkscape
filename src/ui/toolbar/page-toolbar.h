// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_PAGE_TOOLBAR_H
#define SEEN_PAGE_TOOLBAR_H

/**
 * @file
 * Page toolbar
 */
/* Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2021 Martin Owens
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm.h>

#include "toolbar.h"

class SPDesktop;
class SPDocument;
class SPPage;

namespace Inkscape {
class PageManager;
class PaperSize;
namespace UI {
namespace Tools {
class ToolBase;
}
namespace Toolbar {

class PageToolbar : public Gtk::Toolbar
{
public:
    PageToolbar(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &builder, SPDesktop *desktop);
    ~PageToolbar() override;

    static GtkWidget *create(SPDesktop *desktop);

protected:
    void labelEdited();
    void sizeChoose();
    void sizeChanged();

private:
    SPDesktop *_desktop;
    SPDocument *_document;
    PageManager *_page_manager;

    void toolChanged(SPDesktop *desktop, Inkscape::UI::Tools::ToolBase *ec);
    void selectionChanged(SPPage *page);

    sigc::connection _ec_connection;
    sigc::connection _page_connection;

    Gtk::ComboBoxText *combo_page_sizes;
    Gtk::Entry *entry_page_sizes;
    Gtk::Entry *text_page_label;
    Gtk::Label *label_page_pos;
    Gtk::ToolButton *btn_page_backward;
    Gtk::ToolButton *btn_page_foreward;

    std::vector<PaperSize *> page_sizes;
};

} // namespace Toolbar
} // namespace UI
} // namespace Inkscape

#endif /* !SEEN_PAGE_TOOLBAR_H */

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
