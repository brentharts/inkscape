// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 *
 * Document properties widget: viewbox, document size, colors
 */
/*
 * Authors:
 *   Mike Kowalski
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/menu.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/togglebutton.h>

#include <type_traits>

#include "page-properties.h"
#include "page-size-preview.h"
#include "util/paper.h"
#include "ui/widget/registry.h"
#include "ui/widget/color-picker.h"
#include "ui/widget/unit-menu.h"
#include "ui/builder-utils.h"
#include "ui/operation-blocker.h"

using Inkscape::UI::create_builder;
using Inkscape::UI::get_widget;

namespace Inkscape {    
namespace UI {
namespace Widget {

#define GET(prop, id) prop(get_widget<std::remove_reference_t<decltype(prop)>>(_builder, id))

class PagePropertiesBox : public PageProperties {
public:
    PagePropertiesBox() :
        _builder(create_builder("page-properties.glade")),
        GET(_main_grid, "main-grid"),
        GET(_page_width, "page-width"),
        GET(_page_height, "page-height"),
        GET(_portrait, "page-portrait"),
        GET(_landscape, "page-landscape"),
        GET(_auto_viewbox, "auto-viewbox"),
        GET(_scale_x, "scale-x"),
        GET(_scale_y, "scale-y"),
        GET(_viewbox_x, "viewbox-x"),
        GET(_viewbox_y, "viewbox-y"),
        GET(_viewbox_width, "viewbox-width"),
        GET(_viewbox_height, "viewbox-height"),
        GET(_page_templates_menu, "page-templates-menu"),
        GET(_template_name, "page-template-name"),
        GET(_preview_box, "preview-box"),
        GET(_checkerboard, "checkerboard"),
        GET(_antialias, "use-antialias"),
        GET(_border, "border"),
        GET(_border_on_top, "border-top"),
        GET(_shadow, "shadow"),
        GET(_link_width_height, "link-width-height")
    {
#undef GET

        _backgnd_color_picker = std::make_unique<ColorPicker>(
            _("Background color"), _("Page background color used during editing and exporting"), 0xffffff00, true,
            &get_widget<Gtk::Button>(_builder, "background-color"));

        _border_color_picker = std::make_unique<ColorPicker>(
            _("Border and shadow color"), _("Page border and shadow color"), 0x0000001f, true,
            &get_widget<Gtk::Button>(_builder, "border-color"));

        _desk_color_picker = std::make_unique<ColorPicker>(
            _("Desk color"), _("Desk color"), 0xd0d0d0ff, true,
            &get_widget<Gtk::Button>(_builder, "desk-color"));

        for (auto element : {Color::Background, Color::Border, Color::Desk}) {
            get_color_picker(element).connectChanged([=](guint rgba) {
                update_preview_color(element, rgba);
                if (_update.pending()) return;
                _signal_color_changed.emit(rgba, Color::Background);
            });
        }

        // _backgnd_color_picker->connectChanged([=](guint rgba) { _preview->set_page_color(rgba); });
        // _border_color_picker->connectChanged([=](guint rgba) { _preview->set_border_color(rgba); });
        // _desk_color_picker->connectChanged([=](guint rgba) { _preview->set_desk_color(rgba); });

        _display_units = std::make_unique<UnitMenu>(&get_widget<Gtk::ComboBoxText>(_builder, "display-units"));
        _display_units->setUnitType(UNIT_TYPE_LINEAR);

        auto& page_units = get_widget<Gtk::ComboBoxText>(_builder, "page-units");
        _page_units = std::make_unique<UnitMenu>(&page_units);
        _page_units->setUnitType(UNIT_TYPE_LINEAR);
        _current_page_unit = _page_units->getUnit();
        page_units.signal_changed().connect([=](){ set_page_unit(); });

        for (auto&& page : PaperSize::getPageSizes()) {
            auto item = new Gtk::MenuItem(page.getDescription());
            item->show();
            _page_templates_menu.append(*item);
            item->signal_activate().connect([=](){ set_page_template(page); });
        }

        _preview->set_hexpand();
        _preview->set_vexpand();
        _preview_box.add(*_preview);

        for (auto check : {Check::Border, Check::Shadow, Check::Checkerboard, Check::BorderOnTop, Check::Antialias}) {
            auto checkbutton = &get_checkbutton(check);
            checkbutton->signal_toggled().connect([=](){ fire_checkbox_toggled(*checkbutton, check); });
        }
        _border.signal_toggled().connect([=](){
            _preview->draw_border(_border.get_active());
        });
        _shadow.signal_toggled().connect([=](){
            //
            _preview->enable_drop_shadow(_shadow.get_active());
        });
        _checkerboard.signal_toggled().connect([=](){
            _preview->enable_checkerboard(_checkerboard.get_active());
        });
        _auto_viewbox.signal_toggled().connect([=](){
            // 
        });

        const char* linked = "entries-linked-symbolic";
        const char* unlinked = "entries-unlinked-symbolic";
        _link_width_height.signal_clicked().connect([=](){
            // toggle size link
            if (_size_ratio > 0) {
                _size_ratio = 0;
            }
            else {
                auto width = _page_width.get_value();
                auto height = _page_height.get_value();
                if (width > 0 && height > 0) {
                    _size_ratio = width / height;
                }
            }
            // set image
            _link_width_height.set_image_from_icon_name(_size_ratio > 0 ? linked : unlinked, Gtk::ICON_SIZE_LARGE_TOOLBAR);
        });
        _link_width_height.set_image_from_icon_name(unlinked, Gtk::ICON_SIZE_LARGE_TOOLBAR);

        _page_width .signal_value_changed().connect([=](){ set_page_size_linked(true); });
        _page_height.signal_value_changed().connect([=](){ set_page_size_linked(false); });
        _landscape.signal_toggled().connect([=](){ if (_landscape.get_active()) swap_width_height(); });
        _portrait .signal_toggled().connect([=](){ if (_portrait .get_active()) swap_width_height(); });

        for (auto dim : {Dimension::Scale, Dimension::ViewboxPosition, Dimension::ViewboxSize}) {
            auto pair = get_dimension(dim);
            auto b1 = &pair.first;
            auto b2 = &pair.first;
            b1->signal_value_changed().connect([=](){ fire_value_changed(*b1, *b2, dim); });
            b2->signal_value_changed().connect([=](){ fire_value_changed(*b1, *b2, dim); });
        }

        add(_main_grid);
        show();
    }

private:
    void update_preview_color(Color element, guint rgba) {
        switch (element) {
            case Color::Desk: _preview->set_desk_color(rgba); break;
            case Color::Border: _preview->set_border_color(rgba); break;
            case Color::Background: _preview->set_page_color(rgba); break;
        }
    }

    void set_page_template(const PaperSize& page) {
        if (_update.pending()) return;

        {
            auto scoped(_update.block());
            auto width = page.larger;
            auto height = page.smaller;
            if (_landscape.get_active() != (width > height)) {
                std::swap(width, height);
            }
            _page_width.set_value(width);
            _page_height.set_value(height);
            _page_units->setUnit(page.unit->abbr);
            _current_page_unit = _page_units->getUnit();
            if (width > 0 && height > 0 && _size_ratio > 0) {
                _size_ratio = width / height;
            }
        }
        set_page_size();
    }

    void set_page_size_linked(bool width_changing) {
        if (_size_ratio > 0) {
            auto scoped(_update.block());
            if (width_changing) {
                auto width = _page_width.get_value();
                _page_height.set_value(width / _size_ratio);
            }
            else {
                auto height = _page_height.get_value();
                _page_width.set_value(height * _size_ratio);
            }
        }
        set_page_size();
    }

    void set_page_size() {
        auto pending = _update.pending();

        auto scoped(_update.block());

        auto unit = _page_units->getUnit();
        auto width = _page_width.get_value();
        auto height = _page_height.get_value();
        _preview->set_page_size(width, height);
        if (width != height) {
            (width > height ? _landscape : _portrait).set_active();
            _portrait.set_sensitive();
            _landscape.set_sensitive();
        }
        else {
            _portrait.set_sensitive(false);
            _landscape.set_sensitive(false);
        }

        auto templ = find_page_template(width, height, *unit);
        _template_name.set_label(templ ? templ->name : _("Custom"));

        if (!pending) {
            _signal_dimmension_changed.emit(width, height, Dimension::PageSize);
        }
    }

    void swap_width_height() {
        if (_update.pending()) return;

        {
            auto scoped(_update.block());
            auto width = _page_width.get_value();
            auto height = _page_height.get_value();
            _page_width.set_value(height);
            _page_height.set_value(width);
        }
        set_page_size();
    };

    void set_page_unit() {
        if (_update.pending()) return;

        const auto old_unit = _current_page_unit;
        _current_page_unit = _page_units->getUnit();
        const auto new_unit = _current_page_unit;

        if (new_unit == old_unit) return;

        {
            auto width = _page_width.get_value();
            auto height = _page_height.get_value();
            Quantity w(width, old_unit->abbr);
            Quantity h(height, old_unit->abbr);
            auto scoped(_update.block());
            _page_width.set_value(w.value(new_unit));
            _page_height.set_value(h.value(new_unit));
        }
        set_page_size();
    }

    void set_color(Color element, unsigned int color) override {
        auto scoped(_update.block());

        get_color_picker(element).setRgba32(color);
        update_preview_color(element, color);
    }

    void set_check(Check element, bool checked) override {
        auto scoped(_update.block());

        get_checkbutton(element).set_active(checked);

        // todo: special cases
        if (element == Check::Checkerboard) _preview->enable_checkerboard(checked);
        if (element == Check::Shadow) _preview->enable_drop_shadow(checked);
        if (element == Check::Border) _preview->draw_border(checked);
    }

    void set_dimension(Dimension dimension, double x, double y) override {
        auto scoped(_update.block());

        auto dim = get_dimension(dimension);
        dim.first.set_value(x);
        dim.second.set_value(y);

        set_page_size();
    }

    void set_unit(Units unit, const Glib::ustring& abbr) override {
        auto scoped(_update.block());

        if (unit == Units::Display) {
            _display_units->setUnit(abbr);
        }
        else if (unit == Units::Document) {
            _page_units->setUnit(abbr);
            set_page_size();
        }
    }

    ColorPicker& get_color_picker(Color element) {
        switch (element) {
            case Color::Background: return *_backgnd_color_picker;
            case Color::Desk: return *_desk_color_picker;
            case Color::Border: return *_border_color_picker;

            default:
                throw std::runtime_error("missing case in get_color_picker");
        }
    }

    void fire_value_changed(Gtk::SpinButton& b1, Gtk::SpinButton& b2, Dimension dim) {
        if (!_update.pending()) {
            _signal_dimmension_changed.emit(b1.get_value(), b2.get_value(), dim);
        }
    }

    void fire_checkbox_toggled(Gtk::CheckButton& checkbox, Check check) {
        if (!_update.pending()) {
            _signal_check_toggled.emit(checkbox.get_active(), check);
        }
    }

    const PaperSize* find_page_template(double width, double height, const Unit& unit) {
        Quantity w(std::min(width, height), &unit);
        Quantity h(std::max(width, height), &unit);

        const double eps = 1e-6;
        for (auto&& page : PaperSize::getPageSizes()) {
            Quantity pw(std::min(page.larger, page.smaller), page.unit);
            Quantity ph(std::max(page.larger, page.smaller), page.unit);

            if (are_near(w, pw, eps) && are_near(h, ph, eps)) {
                return &page;
            }
        }

        return nullptr;
    }

    Gtk::CheckButton& get_checkbutton(Check check) {
        switch (check) {
            case Check::Antialias: return _antialias;
            case Check::Border: return _border;
            case Check::Shadow: return _shadow;
            case Check::BorderOnTop: return _border_on_top;
            case Check::Checkerboard: return _checkerboard;

            default:
                throw std::runtime_error("missing case in get_checkbutton");
        }
    }

    typedef std::pair<Gtk::SpinButton&, Gtk::SpinButton&> spin_pair;
    spin_pair get_dimension(Dimension dimension) {
        switch (dimension) {
            case Dimension::PageSize: return spin_pair(_page_width, _page_height);
            case Dimension::Scale: return spin_pair(_scale_x, _scale_y);
            case Dimension::ViewboxPosition: return spin_pair(_viewbox_x, _viewbox_y);
            case Dimension::ViewboxSize: return spin_pair(_viewbox_width, _viewbox_height);

            default:
                throw std::runtime_error("missing case in get_dimension");
        }
    }

    Glib::RefPtr<Gtk::Builder> _builder;
    Gtk::Grid& _main_grid;
    Gtk::SpinButton& _page_width;
    Gtk::SpinButton& _page_height;
    Gtk::RadioButton& _portrait;
    Gtk::RadioButton& _landscape;
    Gtk::CheckButton& _auto_viewbox;
    Gtk::SpinButton& _scale_x;
    Gtk::SpinButton& _scale_y;
    Gtk::SpinButton& _viewbox_x;
    Gtk::SpinButton& _viewbox_y;
    Gtk::SpinButton& _viewbox_width;
    Gtk::SpinButton& _viewbox_height;
    std::unique_ptr<ColorPicker> _backgnd_color_picker;
    std::unique_ptr<ColorPicker> _border_color_picker;
    std::unique_ptr<ColorPicker> _desk_color_picker;
    Gtk::Menu& _page_templates_menu;
    Gtk::Label& _template_name;
    Gtk::Box& _preview_box;
    std::unique_ptr<PageSizePreview> _preview = std::make_unique<PageSizePreview>();
    Gtk::CheckButton& _border;
    Gtk::CheckButton& _border_on_top;
    Gtk::CheckButton& _shadow;
    Gtk::CheckButton& _checkerboard;
    Gtk::CheckButton& _antialias;
    Gtk::Button& _link_width_height;
    std::unique_ptr<UnitMenu> _display_units;
    std::unique_ptr<UnitMenu> _page_units;
    const Unit* _current_page_unit = nullptr;
    OperationBlocker _update;
    double _size_ratio = 0; // width to height ratio
};

PageProperties* PageProperties::create() {
    return new PagePropertiesBox();
}


} } } // namespace Inkscape/Widget/UI
