// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_GRADIENT_EDITOR_H
#define SEEN_GRADIENT_EDITOR_H

#include <gtkmm/box.h>
#include <gtkmm/popover.h>
#include <gtkmm/image.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treemodelcolumn.h>

#include "object/sp-gradient.h"
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
	~GradientEditor() override;

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

	GradientSelector* _selector;
	Inkscape::UI::SelectedColor _selected_color;
	SpinScale _stepOffset;
	Gtk::Popover* _popover = nullptr;
	Gtk::Image* _repeatIcon = nullptr;
	GradientImage _gradientImage = GradientImage(nullptr);
	GradientImage _gradientStops = GradientImage(nullptr);
	Glib::RefPtr<Gtk::ListStore> _stopListStore;
	Gtk::TreeModelColumnRecord _stopColumns;
	Gtk::TreeModelColumn<SPGradientStop> _stopObj;
	Gtk::TreeModelColumn<Glib::ustring> _stopID;
	Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> _stopColor;
	Gtk::TreeView* _stopTree = nullptr;
	Gtk::Button* _addStop;
	Gtk::Button* _deleteStop;
	Gtk::Button* _duplicateStop;
};


} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif