#ifndef SEEN_SVG_RENDERER_H
#define SEEN_SVG_RENDERER_H

#include <gtkmm.h>
#include <gdkmm/rgba.h>

#include "document.h"

namespace Inkscape {

const char* rgba_to_css_color(Gdk::RGBA color, char* buffer);


class svg_renderer
{
public:
	svg_renderer(const char* svg_file_path);

	// set inline style on selected elements; return number of elements found
	size_t set_style(const Glib::ustring& selector, const char* name, const char* value);

	// render document
	Glib::RefPtr<Gdk::Pixbuf> render(double scale = 1.0);

private:
	std::unique_ptr<SPDocument> _document;
	SPRoot* _root = nullptr;
};

}

#endif
