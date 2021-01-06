#include "svg-renderer.h"
#include "io/file.h"
#include "xml/repr.h"
#include "object/sp-root.h"
#include "display/cairo-utils.h"
#include "helper/pixbuf-ops.h"

namespace Inkscape {

const char* rgba_to_css_color(Gdk::RGBA color, char* buffer) {
	sprintf(buffer, "#%02x%02x%02x",
		static_cast<int>(color.get_red() * 0xff),
		static_cast<int>(color.get_green() * 0xff),
		static_cast<int>(color.get_blue() * 0xff)
	);
	return buffer;
}


svg_renderer::svg_renderer(const char* svg_file_path) {

	auto file = Gio::File::create_for_path(svg_file_path);

	_document.reset(ink_file_open(file, nullptr));

	if (_document) {
		_root = _document->getRoot();
	}

	if (!_root) throw std::runtime_error("Cannot find root element in svg document");
}

size_t svg_renderer::set_style(const Glib::ustring& selector, const char* name, const char* value) {
	auto objects = _document->getObjectsBySelector(selector);
	for (auto el : objects) {
		SPCSSAttr* css = sp_repr_css_attr(el->getRepr(), "style");
		sp_repr_css_set_property(css, name, value);
		el->changeCSS(css, "style");
		sp_repr_css_attr_unref(css);
	}
	return objects.size();
}

Glib::RefPtr<Gdk::Pixbuf> svg_renderer::render(double scale) {
	auto w = _document->getWidth().value("px");
	auto h = _document->getHeight().value("px");
	int sw = w * scale;
	int sh = h * scale;
	auto dpix = 96 * scale;
	auto dpiy = 96 * scale;

	Inkscape::Pixbuf* pixbuf = sp_generate_internal_bitmap(_document.get(), nullptr, 0, 0, w, h, sw, sh, dpix, dpiy, 0, nullptr);

	return Glib::wrap(pixbuf ? pixbuf->getPixbufRaw() : nullptr);
}

} // namespace
