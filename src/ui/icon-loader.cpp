// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Icon Loader
 *
 * Icon Loader management code
 *
 * Authors:
 *  Jabiertxo Arraiza <jabier.arraiza@marker.es>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include "icon-loader.h"
#include "inkscape.h"
#include "io/resource.h"
#include "svg/svg-color.h"
#include "widgets/toolbox.h"
#include <fstream>
#include <gdkmm/display.h>
#include <gdkmm/screen.h>
#include <gtkmm/iconinfo.h>
#include <gtkmm/icontheme.h>
#include <glib-object.h>

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, gint size)
{
    Gtk::Image *icon = new Gtk::Image();
    icon->set_from_icon_name(icon_name, Gtk::IconSize(Gtk::ICON_SIZE_BUTTON));
    icon->set_pixel_size(size);
    return icon;
}

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, Gtk::IconSize icon_size)
{
    Gtk::Image *icon = new Gtk::Image();
    icon->set_from_icon_name(icon_name, icon_size);
    return icon;
}

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, Gtk::BuiltinIconSize icon_size)
{
    Gtk::Image *icon = new Gtk::Image();
    icon->set_from_icon_name(icon_name, icon_size);
    return icon;
}

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, gchar const *prefs_size)
{
    Gtk::IconSize icon_size = Inkscape::UI::ToolboxFactory::prefToSize_mm(prefs_size);
    return sp_get_icon_image(icon_name, icon_size);
}

GtkWidget *sp_get_icon_image(Glib::ustring icon_name, GtkIconSize icon_size)
{
    return gtk_image_new_from_icon_name(icon_name.c_str(), icon_size);
}

// Gtk::Image* sp_get_sized_icon_image(const Glib::ustring& icon_name, Inkscape::UI::InkIconSize size) {
// 	if (size.isIconSize()) {
	// GTK_IMAGE
// 		return Glib::wrap(G_TYPE_CHECK_INSTANCE_CAST(gtk_image_new_from_icon_name(icon_name.c_str(), size.getIconSize()), GTK_TYPE_IMAGE, GtkImage));
// 	}
// 	else if (size.isPixelSize()) {
// 		return sp_get_icon_image(icon_name, size.getPixelSize());
// 	}
// 	else {
// 		return nullptr;
// 	}
// }

const double MIN_RESOLUTION = 1.0;
const double MAX_RESOLUTION = 1000.0;

// get monitor dpi function lifted from GnuIMP with modifications
std::pair<double, double> get_monitor_resolution(GdkMonitor* monitor) {
  GdkRectangle size_pixels;
  double      x = 0.0;
  double      y = 0.0;
#ifdef PLATFORM_OSX
  CGSize       size;
#endif

  if (!GDK_IS_MONITOR(monitor)) return std::make_pair(96.0, 96.0);

#ifndef PLATFORM_OSX
  gdk_monitor_get_geometry(monitor, &size_pixels);

  int width_mm  = gdk_monitor_get_width_mm(monitor);
  int height_mm = gdk_monitor_get_height_mm(monitor);
#else
  int width_mm  = 0;
  int height_mm = 0;
  size = CGDisplayScreenSize(kCGDirectMainDisplay);
  if (!CGSizeEqualToSize(size, CGSizeZero)) {
      width_mm  = size.width;
      height_mm = size.height;
  }
  size_pixels.width  = CGDisplayPixelsWide(kCGDirectMainDisplay);
  size_pixels.height = CGDisplayPixelsHigh(kCGDirectMainDisplay);
#endif

  if (width_mm > 0 && height_mm > 0) {
      x = size_pixels.width  * 25.4 / width_mm;
      y = size_pixels.height * 25.4 / height_mm;
  }

  if (x < MIN_RESOLUTION || x > MAX_RESOLUTION ||
      y < MIN_RESOLUTION || y > MAX_RESOLUTION) {
      g_printerr("get_monitor_resolution(): GDK returned bogus values for the monitor resolution, using 96 dpi instead.\n");
      x = 96.0;
      y = 96.0;
  }

  return std::make_pair(x, y);
}


Gtk::Widget* sp_get_sized_icon_image(const Glib::ustring& icon_name, Inkscape::UI::InkIconSize size) {
	if (size.isIconSize()) {
		return Glib::wrap(gtk_image_new_from_icon_name(icon_name.c_str(), size.getIconSize()));
	}
	else if (size.isPixelSize()) {
		// grab dpi of primary screen and keep it static
		static auto dpi = get_monitor_resolution(gdk_display_get_monitor(gdk_display_get_default(), 0));
		// transform logical pixels to physical pixels
		int pixelSize = static_cast<int>(std::round((size.getPixelSize() * (dpi.first + dpi.second) / 2.0) / 96.0));
		return sp_get_icon_image(icon_name, pixelSize);
	}
	else {
      g_printerr("sp_get_sized_icon_image(): Invalid size, neither in pixels nor enum. Defaulting to small toolbar icons.\n");
		return sp_get_icon_image(icon_name, Gtk::ICON_SIZE_SMALL_TOOLBAR);
	}
}

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, gint size)
{
    Glib::RefPtr<Gdk::Display> display = Gdk::Display::get_default();
    Glib::RefPtr<Gdk::Screen>  screen = display->get_default_screen();
    Glib::RefPtr<Gtk::IconTheme> icon_theme = Gtk::IconTheme::get_for_screen(screen);
    Glib::RefPtr<Gdk::Pixbuf> _icon_pixbuf;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/theme/symbolicIcons", false)) {
        Gtk::IconInfo iconinfo = icon_theme->lookup_icon(icon_name + Glib::ustring("-symbolic"), size, Gtk::ICON_LOOKUP_FORCE_SIZE);
        if (iconinfo && SP_ACTIVE_DESKTOP->getToplevel()) {
            bool was_symbolic = false;
            Glib::ustring css_str = "";
            Glib::ustring themeiconname = prefs->getString("/theme/iconTheme");
            guint32 colorsetbase = prefs->getInt("/theme/" + themeiconname + "/symbolicBaseColor", 0x2E3436ff);
            guint32 colorsetsuccess = prefs->getInt("/theme/" + themeiconname + "/symbolicSuccessColor", 0x4AD589ff);
            guint32 colorsetwarning = prefs->getInt("/theme/" + themeiconname + "/symbolicWarningColor", 0xF57900ff);
            guint32 colorseterror = prefs->getInt("/theme/" + themeiconname + "/symbolicErrorColor", 0xcc0000ff);
            gchar colornamed[64];
            gchar colornamedsuccess[64];
            gchar colornamedwarning[64];
            gchar colornamederror[64];
            sp_svg_write_color(colornamed, sizeof(colornamed), colorsetbase);
            sp_svg_write_color(colornamedsuccess, sizeof(colornamedsuccess), colorsetsuccess);
            sp_svg_write_color(colornamedwarning, sizeof(colornamedwarning), colorsetwarning);
            sp_svg_write_color(colornamederror, sizeof(colornamederror), colorseterror);
            _icon_pixbuf =
                iconinfo.load_symbolic(Gdk::RGBA(colornamed), Gdk::RGBA(colornamedsuccess),
                                       Gdk::RGBA(colornamedwarning), Gdk::RGBA(colornamederror), was_symbolic);
        } else {
            Gtk::IconInfo iconinfo = icon_theme->lookup_icon(icon_name, size, Gtk::ICON_LOOKUP_FORCE_SIZE);
            _icon_pixbuf = iconinfo.load_icon();
        }
    } else {
        Gtk::IconInfo iconinfo = icon_theme->lookup_icon(icon_name, size, Gtk::ICON_LOOKUP_FORCE_SIZE);
        _icon_pixbuf = iconinfo.load_icon();
    }
    return _icon_pixbuf;
}

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, Gtk::IconSize icon_size)
{
    int width, height;
    Gtk::IconSize::lookup(icon_size, width, height);
    return sp_get_icon_pixbuf(icon_name, width);
}

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, Gtk::BuiltinIconSize icon_size)
{
    int width, height;
    Gtk::IconSize::lookup(Gtk::IconSize(icon_size), width, height);
    return sp_get_icon_pixbuf(icon_name, width);
}

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, GtkIconSize icon_size)
{
    gint width, height;
    gtk_icon_size_lookup(icon_size, &width, &height);
    return sp_get_icon_pixbuf(icon_name, width);
}

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, gchar const *prefs_size)
{
    // Load icon based in preference size defined allowed values are:
    //"/toolbox/tools/small" Toolbox icon size
    //"/toolbox/small" Control bar icon size
    //"/toolbox/secondary" Secondary toolbar icon size
    GtkIconSize icon_size = Inkscape::UI::ToolboxFactory::prefToSize(prefs_size);
    return sp_get_icon_pixbuf(icon_name, icon_size);
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
