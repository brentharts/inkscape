// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SEEN_INK_ICON_SIZE
#define SEEN_INK_ICON_SIZE

#include <glibmm/ustring.h>
#include <gtk/gtk.h>

namespace Inkscape {
namespace UI {

// Icon size read from preferences; either one of GtkIconSize values or size in logical pixels
// Note: named InkIconSize to avoid confusion with Gtk::IconSize
//
class InkIconSize {
public:
	InkIconSize(const Glib::ustring& prefPath);
	InkIconSize(GtkIconSize size);
	InkIconSize(const InkIconSize& src);
	InkIconSize();

	// true if size specified in pixels
	bool isPixelSize() const;
	// true if size specified as enum
	bool isIconSize() const;

	GtkIconSize getIconSize() const;
	int getPixelSize() const;

	int getRawValue() const;
	static InkIconSize fromRawValue(int size);

	static int minValue();
	static int maxValue();
	static int minPixelValue();
	static int maxPixelValue();
	static int defaultCustomSize();

private:
	int _size;
	int _size_px;
};


}
}

#endif
