// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_UI_WIDGET_FONT_LIST_H
#define INKSCAPE_UI_WIDGET_FONT_LIST_H

#include <unordered_map>
#include <vector>
#include <gtkmm/builder.h>
#include <gtkmm/grid.h>
#include <gtkmm/box.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scale.h>
#include "util/font-discovery.h"
#include "ui/operation-blocker.h"

namespace Inkscape {
namespace UI {
namespace Widget {

class FontList : public Gtk::Box {
public:
    FontList();

    // get font selected in this FontList, if any
    Glib::ustring get_fontspec() const;
    double get_fontsize() const;

    // show requested font in a FontList
    void set_current_font(const Glib::ustring& family, const Glib::ustring& face);
    // 
    void set_current_size(double size);

    sigc::signal<void ()>& signal_changed() { return _signal_changed; }
    sigc::signal<void ()>& signal_apply() { return _signal_apply; }

private:
    void sort_fonts(Inkscape::FontOrder order);
    void filter();
    struct Show {
        bool monospaced;
        bool oblique;
        bool others;
    };
    void filter(Glib::ustring text, const Show& params);
    void add_font(const Glib::ustring& fontspec, bool select);
    bool select_font(const Glib::ustring& fontspec);

    sigc::signal<void ()> _signal_changed;
    sigc::signal<void ()> _signal_apply;
    Glib::RefPtr<Gtk::Builder> _builder;
    Gtk::Grid& _main_grid;
    Gtk::TreeView& _font_list;
    Gtk::TreeViewColumn _text_column;
    Gtk::TreeViewColumn _icon_column;
    Glib::RefPtr<Gtk::ListStore> _font_list_store;
    // std::unordered_map<std::string, const Gtk::TreeRow*> _fspec_to_row;
    std::vector<FontInfo> _fonts;
    Inkscape::FontOrder _order = Inkscape::FontOrder::by_name;
    Glib::ustring _filter;
    Gtk::ComboBoxText& _font_size;
    Gtk::Scale& _font_size_scale;
    std::unique_ptr<Gtk::CellRendererText> _cell_renderer;
    std::unique_ptr<Gtk::CellRenderer> _cell_icon_renderer;
    Glib::ustring _current_fspec;
    double _current_fsize = 0.0;
    OperationBlocker _update;
};

}}} // namespaces

#endif // INKSCAPE_UI_WIDGET_FONT_LIST_H