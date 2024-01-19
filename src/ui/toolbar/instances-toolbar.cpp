// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Toolbar for Instanceping options.
 */
/* Authors:
 *   Michael Kowalski
 *   Tavmjong Bah
 *
 * Copyright (C) 2023 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "instances-toolbar.h"

#include <iostream>
#include <glibmm/i18n.h>
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include "ui/widget/canvas.h"
#include "ui/builder-utils.h"
#include "ui/widget/desktop-widget.h"
#include "ui/pack.h"
#include "ui/util.h"
#include "inkscape.h"
#include "inkscape-window.h"
#include "inkscape-application.h"
#include "document.h"
#include "desktop.h"
#include "enums.h"
#include "ui/widget/custom-tooltip.h"

namespace Inkscape::UI::Toolbar {

InstancesToolbar::InstancesToolbar()
    : Gtk::Box()
    , builder(UI::create_builder("toolbar-instances.ui"))
    , _instances_box(UI::get_widget<Gtk::Box>(builder, "instances-box"))
{
    set_name("InstancesToolbar");
    UI::pack_start(*this, _instances_box, true, true);
}

SPDesktop * 
sp_desktop_from_dkey(unsigned int dkey) {
    
    if (auto desktops = INKSCAPE.get_desktops()) {
        for (auto & desktop : *desktops) {
            if (desktop->dkey == dkey) {
                return desktop;
            }
        }
    }
    return nullptr;
}

void InstancesToolbar::activate_instance(SPDesktop * desktop) {
    static unsigned int dkey = 0;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    GtkWindow *currentw = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(_instances_box.gobj())));
    static bool fullscreen = prefs->getBool("/desktop/geometry/fullscreen");
    static bool maximized = prefs->getBool("/desktop/geometry/maximized");
    if(prefs->getBool("/window/instances/state", true)) {;
        if (dkey) {
            int x, y, w, h = 0;
            if (auto _desktop = sp_desktop_from_dkey(dkey)) {
                _desktop->getWindowGeometry(x, y, w, h);
                maximized = _desktop->is_maximized();
                fullscreen = _desktop->is_fullscreen();
                if (GTK_IS_WINDOW(currentw)) {
                    if (desktop->is_maximized() != maximized) {
                        if (maximized) {
                            gtk_window_maximize(currentw);
                        } else {
                            gtk_window_unmaximize(currentw);
                        }
                    }
                    if (desktop->is_fullscreen() != fullscreen) {
                        if (fullscreen) {
                            gtk_window_fullscreen(currentw);
                        } else {
                            gtk_window_unfullscreen(currentw);
                        }
                    }
                }
                desktop->setWindowPosition (Geom::Point(x,y));
                desktop->setWindowSize (w,h);
                set_buttons(); //pass leaved dkey
            }
        }
    }
    dkey = desktop->dkey;
}

Cairo::RefPtr<Cairo::Surface>
sp_instance_preview(SPDesktop *desktop) {
    assert(desktop != nullptr);
    cairo_surface_t *surface;
    cairo_t *cr;
    if (auto canvas = desktop->getCanvas()) {
        Gtk::Allocation alloc = canvas->get_allocation();
        auto device_scale = canvas->get_scale_factor();
        surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, (alloc.get_width() * device_scale) / 4.0, (alloc.get_height() * device_scale) / 4.0);
        cairo_surface_set_device_scale(surface, device_scale, device_scale);
        cr = cairo_create (surface);
        cairo_scale(cr, 0.25, 0.25);
        cairo_push_group(cr);
        gtk_widget_draw (canvas->Gtk::Widget::gobj(), cr);
        cairo_pop_group_to_source(cr);
        cairo_paint_with_alpha(cr, 1);
        Cairo::RefPtr<Cairo::Surface> ret = Cairo::RefPtr<Cairo::Surface>(new Cairo::Surface(surface, false));
        //char *fn = g_strdup_printf("/home/jabiertxof/dump.png");
        //cairo_surface_write_to_png(surface, fn);
        cairo_surface_destroy(surface);
        return ret;
    }
    return Cairo::RefPtr<Cairo::Surface>();
}

void InstancesToolbar::set_buttons() {
    if (auto desktops = INKSCAPE.get_desktops()) {
        _instances_box.set_sensitive(true);
        std::map<unsigned int,SPDesktop *> dkey_desktops;
        for (auto & desktop : *desktops) {
            if (desktop->attached) {
                dkey_desktops.emplace(desktop->dkey, desktop);
            }
        }
        auto buttons = UI::get_children(_instances_box);
        bool add_dattach = true;
        for(auto &widg : buttons) {
            bool found = false;
            for (auto & desktop : *desktops) {
                if (desktop->attached) {
                    Glib::ustring name = Glib::ustring("instancebuttom_") + desktop->dkey;
                    if (widg->get_name() == name) {
                        found = true;
                        dynamic_cast<Gtk::Button *>(widg)->set_label(desktop->getDocument()->getDocumentName());
                        break;
                    }
                }
            }
            if (widg->get_name() == "instance_deatach") {
                add_dattach = false;
            }
            if (!found && widg->get_style_context()->has_class("instance_button")) {
                delete widg;
            }
        }
        buttons = UI::get_children(_instances_box);
        if (add_dattach) {
            auto instance = Gtk::make_managed<Gtk::Button>(_("Deattach current"));
            instance->set_name("instance_deatach");
            instance->set_tooltip_text(_("Deattach current document from list"));
            UI::pack_end(_instances_box, *instance, true, false);
            instance->show_all();
            instance->signal_clicked().connect([this,instance] { 
                if (auto desktops = INKSCAPE.get_desktops()) {
                    SPDesktop * cdesktop = *desktops->begin();
                    if (cdesktop) {
                        cdesktop->attached = false;
                        int x, y, w, h = 0;
                        for (auto & desktop : *desktops) {
                            if (desktop->attached) {
                                auto parent = desktop->getInkscapeWindow(); 
                                g_assert(parent != nullptr);
                                parent->on_is_active_changed();
                                 desktop->getWindowGeometry(x, y, w, h);
                                if (!desktop->is_maximized() && !desktop->is_fullscreen()) {
                                    desktop->setWindowPosition (Geom::Point(x+30, y+30));
                                }
                                break;
                            }
                        }
                        raise_instance(cdesktop);
                        instance->get_parent()->get_parent()->set_visible(false); 
                    }
                }
            });
        }
        size_t index = 0;
        static Cairo::RefPtr<Cairo::Surface> preview = Cairo::RefPtr<Cairo::Surface>();
        for (auto dkey_desktop : dkey_desktops) {
            index++;
            auto desktop = dkey_desktop.second;
            Glib::ustring name = Glib::ustring("instancebuttom_") + Glib::ustring::format(desktop->dkey);
            bool found = false;
            for(auto &widg : buttons) {
                if (widg->get_name() == name) {
                    found = true;
                    std::cout << desktop->getDocument()->getDocumentName() << std::endl;
                    dynamic_cast<Gtk::Button *>(widg)->set_label(desktop->getDocument()->getDocumentName());
                    if (desktop != SP_ACTIVE_DESKTOP) {
                        widg->get_style_context()->remove_class("instance_active");
                    } else {
                        widg->get_style_context()->add_class("instance_active");
                    }
                }
            }
            if (found) {
                continue;
            }
            auto instance = Gtk::make_managed<Gtk::Button>();
            auto box = Gtk::make_managed<Gtk::Box>();
            auto const close = Gtk::make_managed<Gtk::Button>();
            close->set_image_from_icon_name("window-close");
            close->set_tooltip_text(_("Close Document"));
            close->get_style_context()->add_class("close-button");
            auto label = Gtk::make_managed<Gtk::Label>(desktop->getDocument()->getDocumentName(),false);
            label->set_max_width_chars(20);
            label->set_ellipsize(Pango::ELLIPSIZE_END);
            label->set_margin_end(6);
            UI::pack_start(*box, *label);
            UI::pack_end(*box, *close);
            instance->add(*box);
            instance->property_has_tooltip() = true;
            instance->set_name(name);
            instance->get_style_context()->add_class("instance_button");
            instance->set_relief(Gtk::RELIEF_NONE);
            if (desktop != SP_ACTIVE_DESKTOP) {
                instance->get_style_context()->remove_class("instance_active");
            } else {
                instance->get_style_context()->add_class("instance_active");
            }
            unsigned int dkey = desktop->dkey;
            instance->signal_enter_notify_event().connect([this, dkey] (GdkEventCrossing* crossing_event) { 
                if (auto desktop = sp_desktop_from_dkey(dkey)) {
                    preview = sp_instance_preview(desktop);
                }
                return true;
            });
            
            instance->signal_query_tooltip().connect([this, dkey](int x, int y, bool kbd, const Glib::RefPtr<Gtk::Tooltip>& tooltipw){
                if (auto desktop = sp_desktop_from_dkey(dkey)) {
                    return sp_query_custom_tooltip(x, y, kbd, tooltipw, dkey, desktop->getDocument()->getDocumentName(), "", Gtk::ICON_SIZE_DIALOG, preview, Gtk::ORIENTATION_VERTICAL, 200);
                }
                return false;
            });
            instance->set_visible(true);
            instance->signal_clicked().connect([this, desktop] { this->raise_instance(desktop); });
            instance->set_sensitive(true);
            close->signal_clicked().connect([this, dkey, instance, close] { 
                if (auto desktop = sp_desktop_from_dkey(dkey)) {
                    _instances_box.set_sensitive(false);
                    auto app = InkscapeApplication::instance();
                    auto parent = desktop->getInkscapeWindow(); 
                    g_assert(parent != nullptr);
                    if (auto desktops = INKSCAPE.get_desktops()) {
                        for (auto & desk : *desktops) {
                            if (desk->attached && desk != desktop) {
                                auto parentrise = desk->getInkscapeWindow(); 
                                g_assert(parentrise != nullptr);
                                parentrise->on_is_active_changed();
                                app->destroy_window(parent, false, false);
                                break;
                            }
                        }
                    }
                    
                }
            });
            UI::pack_start(_instances_box, *instance, false, false);
            _instances_box.reorder_child(*instance, index - 1);
            instance->show_all();
        }
    }
}

void InstancesToolbar::raise_instance(SPDesktop * desktop) {
    InkscapeWindow *parent = desktop->getInkscapeWindow();
    g_assert(parent != nullptr);
    auto w = GTK_WIDGET(parent->gobj());
    if (w) {
        auto const gdkw = gtk_widget_get_window(w);
        if (gdkw) {
            gdk_window_focus(gdkw, GDK_CURRENT_TIME);
        }
    }
} 
} // namespace Inkscape::UI::Toolbar

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
