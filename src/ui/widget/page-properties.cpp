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
#include "page-sizer.h"
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

// TEMP until page sizes are migrated
class PageSizes : PageSizer {
    Registry _r;
public:
    PageSizes() : PageSizer(_r) {}
    ~PageSizes() override = default;

    const std::map<Glib::ustring, PaperSize>& page_sizes() const {
        return _paperSizeTable;
    }

    const std::vector<PaperSize>& get_page_sizes() {
        return _paper_sizes; // paper sizes in original order
    }
};
// END TEMP

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
        GET(_border, "border"),
        GET(_shadow, "shadow"),
        GET(_link_width_height, "link-width-height")
    {
#undef GET

        _backgnd_color_picker = std::make_unique<ColorPicker>(
            _("Background color"), _("Page background color used during editing and exporting"), 0xffffff00, true,
            &get_widget<Gtk::Button>(_builder, "background-color"));
        _backgnd_color_picker->setRgba32(_background_color);

        _border_color_picker = std::make_unique<ColorPicker>(
            _("Border and shadow color"), _("Page border and shadow color"), 0x0000001f, true,
            &get_widget<Gtk::Button>(_builder, "border-color"));

        _desk_color_picker = std::make_unique<ColorPicker>(
            _("Desk color"), _("Desk color"), 0xd0d0d0ff, true,
            &get_widget<Gtk::Button>(_builder, "desk-color"));

        _backgnd_color_picker->connectChanged([=](guint rgba) {
            _preview->set_page_color(rgba);
            if (_update.pending()) return;
            _signal_color_changed.emit(rgba, Color::Background);
        });
        _border_color_picker->connectChanged([=](guint rgba) {
            _preview->set_border_color(rgba);
            if (_update.pending()) return;
            _signal_color_changed.emit(rgba, Color::Border);
        });
        _desk_color_picker->connectChanged([=](guint rgba) {
            _preview->set_desk_color(rgba);
            if (_update.pending()) return;
            _signal_color_changed.emit(rgba, Color::Desk);
        });

        _display_units = std::make_unique<UnitMenu>(&get_widget<Gtk::ComboBoxText>(_builder, "display-units"));
        _display_units->setUnitType(UNIT_TYPE_LINEAR);

        auto& page_units = get_widget<Gtk::ComboBoxText>(_builder, "page-units");
        _page_units = std::make_unique<UnitMenu>(&page_units);
        _page_units->setUnitType(UNIT_TYPE_LINEAR);
        _current_page_unit = _page_units->getUnit();
        page_units.signal_changed().connect([=](){ set_page_unit(); });

        for (auto&& page : _paper->get_page_sizes()) {
            auto item = new Gtk::MenuItem(page.name);
            item->show();
            _page_templates_menu.append(*item);
            item->signal_activate().connect([=](){ set_page_template(page); });
        }

        _preview->set_hexpand();
        _preview->set_vexpand();
        _preview_box.add(*_preview);

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

        add(_main_grid);
        show();
    }

private:
    void set_page_template(const PaperSize& page) {
        if (_update.pending()) return;

        {
            auto scoped(_update.block());
            _page_width.set_value(page.larger);
            _page_height.set_value(page.smaller);
            _page_units->setUnit(page.unit->abbr);
            _current_page_unit = _page_units->getUnit();
            if (page.larger > 0 && page.smaller > 0 && _size_ratio > 0) {
                _size_ratio = page.larger / page.smaller;
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
        if (_update.pending()) return;

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

        switch (element) {
            case Color::Background:
                _backgnd_color_picker->setRgba32(color);
                break;

            case Color::Desk:
                _desk_color_picker->setRgba32(color);
                break;

            case Color::Border:
                _border_color_picker->setRgba32(color);
                break;
        }
    }

    const PaperSize* find_page_template(double width, double height, const Unit& unit) {
        Quantity w(std::min(width, height), &unit);
        Quantity h(std::max(width, height), &unit);

        const double eps = 1e-6;
        for (auto&& page : _paper->get_page_sizes()) {
            Quantity pw(std::min(page.larger, page.smaller), page.unit);
            Quantity ph(std::max(page.larger, page.smaller), page.unit);

            if (are_near(w, pw, eps) && are_near(h, ph, eps)) {
                return &page;
            }
        }

        return nullptr;
    }

    Glib::RefPtr<Gtk::Builder> _builder;
    Gtk::Grid& _main_grid;
    Gtk::SpinButton& _page_width;
    Gtk::SpinButton& _page_height;
    Gtk::RadioButton& _portrait;
    Gtk::RadioButton& _landscape;
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
    Gtk::CheckButton& _shadow;
    Gtk::CheckButton& _checkerboard;
    Gtk::Button& _link_width_height;
    std::unique_ptr<PageSizes> _paper = std::make_unique<PageSizes>();
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
