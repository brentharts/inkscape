// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_GRADIENT_EDITOR_H
#define SEEN_GRADIENT_EDITOR_H

#include <gtkmm/box.h>
#include <gtkmm/grid.h>
#include <gtkmm/button.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/popover.h>
#include <gtkmm/image.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treemodelcolumn.h>
#include <gtkmm/builder.h>
#include <optional>

#include "object/sp-gradient.h"
#include "object/sp-stop.h"
#include "ui/selected-color.h"
#include "spin-scale.h"
#include "gradient-image.h"
#include "gradient-selector-interface.h"

namespace Inkscape {
namespace UI {
namespace Widget {

class GradientSelector;

class GradientEditor : public Gtk::Box, public GradientSelectorInterface {
public:
	GradientEditor();
	~GradientEditor() noexcept override;

	GradientSelector* get_selector();

private:
	sigc::signal<void> _signal_grabbed;
	sigc::signal<void> _signal_dragged;
	sigc::signal<void> _signal_released;
	sigc::signal<void, SPGradient*> _signal_changed;

public:
	decltype(_signal_changed) signal_changed() const { return _signal_changed; }
	decltype(_signal_grabbed) signal_grabbed() const { return _signal_grabbed; }
	decltype(_signal_dragged) signal_dragged() const { return _signal_dragged; }
	decltype(_signal_released) signal_released() const { return _signal_released; }

	void setGradient(SPGradient* gradient) override;
	SPGradient* getVector() override;
	void setVector(SPDocument* doc, SPGradient* vector) override;
	void setMode(SelectorMode mode) override;
	void setUnits(SPGradientUnits units) override;
	SPGradientUnits getUnits() override;
	void setSpread(SPGradientSpread spread) override;
	SPGradientSpread getSpread() override;

private:
	void set_gradient(SPGradient* gradient);
	void stop_selected(); //const Gtk::TreeModel::const_iterator& iter);
	void add_stop();
	void duplicate_stop();
	void delete_stop();
	void toggle_stops();
	void update_stops_layout();
	void set_repeat_mode(SPGradientSpread mode);
	void set_repeat_icon(SPGradientSpread mode);
	void reverse_gradient();
	void set_step_color(SPColor color, float opacity);
	std::optional<Gtk::TreeRow> current_stop();

	Glib::RefPtr<Gtk::Builder> _builder;
	GradientSelector* _selector;
	Inkscape::UI::SelectedColor _selected_color;
	// SpinScale _stepOffset;
	Gtk::Popover& _popover;
	Gtk::Image& _repeatIcon;
	GradientImage _gradientImage{nullptr};
	GradientImage _gradientStops{nullptr};
	Glib::RefPtr<Gtk::ListStore> _stopListStore;
	Gtk::TreeModelColumnRecord _stopColumns;
	Gtk::TreeModelColumn<SPStop*> _stopObj;
	Gtk::TreeModelColumn<Glib::ustring> _stopID;
	Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> _stopColor;
	Gtk::TreeView& _stopTree;
	Gtk::SpinButton& _offsetBtn;
	Gtk::Button& _addStop;
	Gtk::Button& _deleteStop;
	Gtk::Button& _showStopsList;
	bool _stopsListVisible = true;
	Gtk::Box& _stopsGallery;
	Gtk::Box& _stopBox;
	Gtk::Box& _colorsBox;
	Gtk::Grid& _mainGrid;
	SPGradient* _gradient = nullptr;
	SPDocument* _document = nullptr;
};


} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif