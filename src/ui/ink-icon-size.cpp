#include "ink-icon-size.h"
#include "preferences.h"
#include <algorithm>

using Inkscape::UI::InkIconSize;
const int CUSTOM_SIZE = 4;
const int DEFAULT_SIZE_PX = 20;

InkIconSize::InkIconSize(const Glib::ustring& prefPath) {
	InkIconSize def;
	// size values: 0 .. 3, 4
	// 0 .. 3 are predefined GTK icon sizes,
	// 4 signifies a custom size
	_size = Inkscape::Preferences::get()->getIntLimited(prefPath, def._size, minValue(), maxValue());

	// pixel size value: 5 .. 100 (logical pixels)
	_size_px = Inkscape::Preferences::get()->getIntLimited(prefPath + "_px", def._size_px, minPixelValue(), maxPixelValue());
}

InkIconSize::InkIconSize(const InkIconSize& src) {
	_size = src._size;
	_size_px = src._size_px;
}

InkIconSize::InkIconSize(GtkIconSize size) {
	_size_px = DEFAULT_SIZE_PX;
	switch (size) {
		case GTK_ICON_SIZE_SMALL_TOOLBAR:
			_size = 1;
			break;
		case GTK_ICON_SIZE_MENU:
			_size = 2;
			break;
		case GTK_ICON_SIZE_DIALOG:
			_size = 3;
			break;
		default:
			_size = 0;
			break;
	}
}

GtkIconSize InkIconSize::getIconSize() const {
	switch (_size) {
		case 0: return GTK_ICON_SIZE_LARGE_TOOLBAR;
		case 1: return GTK_ICON_SIZE_SMALL_TOOLBAR;
		case 2: return GTK_ICON_SIZE_MENU;
		case 3: return GTK_ICON_SIZE_DIALOG;
		default:
			throw std::domain_error("Icon size is not specified as enum.");
	}
}

InkIconSize::InkIconSize() : _size(1), _size_px(DEFAULT_SIZE_PX) {
}

bool InkIconSize::isPixelSize() const {
	return _size == CUSTOM_SIZE;
}

bool InkIconSize::isIconSize() const {
	return _size < CUSTOM_SIZE;
}

int InkIconSize::getPixelSize() const {
	if (isPixelSize()) {
		return _size_px;
	}
	else {
		throw std::domain_error("Icon size is not specified in pixels.");
	}
}

// packing sizes into single "raw int value" so it plays nice with GTK widget properties
int InkIconSize::getRawValue() const {
	return isPixelSize() ? -_size_px : _size;
}

// reconstruct icon size from "raw int value"
InkIconSize InkIconSize::fromRawValue(int size) {
	InkIconSize i;
	if (size < 0) { // negative raw value - pixel size?
		i._size_px = std::min(std::max(-size, InkIconSize::minPixelValue()), InkIconSize::maxPixelValue());
		i._size = CUSTOM_SIZE;
	}
	else {
		i._size_px = DEFAULT_SIZE_PX;
		i._size = std::min(std::max(size, InkIconSize::minValue()), InkIconSize::maxValue());
	}
	return i;
}

int InkIconSize::minValue() {
	return 0;
}

int InkIconSize::maxValue() {
	return CUSTOM_SIZE;
}

// arbitrarily selected range of valid icon sizes in pixels

int InkIconSize::minPixelValue() {
	return 5;
}

int InkIconSize::maxPixelValue() {
	return 100;
}

int InkIconSize::defaultCustomSize() {
	return DEFAULT_SIZE_PX;
}
