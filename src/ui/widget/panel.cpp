// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Bryce Harrington <bryce@bryceharrington.org>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Gustav Broberg <broberg@kth.se>
 *
 * Copyright (C) 2004 Bryce Harrington
 * Copyright (C) 2005 Jon A. Cruz
 * Copyright (C) 2007 Gustav Broberg
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm/dialog.h> // for Gtk::RESPONSE_*

#include <glibmm/i18n.h>

#include "panel.h"
#include "desktop.h"

#include "inkscape.h"
#include "inkscape-window.h"
#include "preview.h"

namespace Inkscape {
namespace UI {
namespace Widget {

void Panel::prep() {
    GtkIconSize sizes[] = {
        GTK_ICON_SIZE_MENU,
        GTK_ICON_SIZE_MENU,
        GTK_ICON_SIZE_SMALL_TOOLBAR,
        GTK_ICON_SIZE_BUTTON,
        GTK_ICON_SIZE_DND, // Not used by options, but included to make the last size larger
        GTK_ICON_SIZE_DIALOG
    };
    Preview::set_size_mappings( G_N_ELEMENTS(sizes), sizes );
}

Panel::Panel(gchar const *prefs_path, int verb_num) :
    _prefs_path(prefs_path),
    _desktop(nullptr),
    _verb_num(verb_num),
    _action_area(nullptr)
{
    set_name("InkscapePanel");
    set_orientation(Gtk::ORIENTATION_VERTICAL);

    signalResponse().connect(sigc::mem_fun(*this, &Panel::_handleResponse));
    signalActivateDesktop().connect(sigc::mem_fun(*this, &Panel::on_activate_desktop));

    signal_map().connect([this]() {
        auto *inkwin = dynamic_cast<InkscapeWindow *>(this->get_toplevel());
        if (inkwin) {
            this->setDesktop(inkwin->get_desktop());
        } else {
            this->setDesktop(SP_ACTIVE_DESKTOP);
        }
    });

    signal_unmap().connect([this]() { this->setDesktop(nullptr); });

    _contents = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    pack_start(*_contents, true, true);

    show_all_children();
}

void Panel::_setContents(Gtk::Box *contents) {
    if (_contents) {
        // unlinke `Gtk::Container::remove`, this will delete `_contents` if it is `Gtk::manage`d
        gtk_container_remove(Container::gobj(), _contents->Gtk::Widget::gobj());
    }
    _contents = contents;
    pack_start(*_contents, true, true);
}

void Panel::present()
{
    _signal_present.emit();
}

sigc::signal<void, int> &Panel::signalResponse()
{
    return _signal_response;
}

sigc::signal<void> &Panel::signalPresent()
{
    return _signal_present;
}

gchar const *Panel::getPrefsPath() const
{
    return _prefs_path.data();
}

int const &Panel::getVerb() const
{
    return _verb_num;
}

void Panel::on_activate_desktop(SPDesktop *desktop)
{
    if (get_mapped() && desktop != _desktop) {
        setDesktop(desktop);
    }
}

void Panel::setDesktop(SPDesktop *desktop)
{
    _desktop = desktop;
}

void Panel::_apply()
{
    g_warning("Apply button clicked for panel [Panel::_apply()]");
}

Gtk::Button *Panel::addResponseButton(const Glib::ustring &button_text, int response_id, bool pack_start)
{
    // Create a button box for the response buttons if it's the first button to be added
    if (!_action_area) {
        _action_area = new Gtk::ButtonBox();
        _action_area->set_layout(Gtk::BUTTONBOX_END);
        _action_area->set_spacing(6);
        _action_area->set_border_width(4);
        pack_end(*_action_area, Gtk::PACK_SHRINK, 0);
    }

    Gtk::Button *button = new Gtk::Button(button_text, true);

    _action_area->pack_end(*button);

    if (pack_start) {
        _action_area->set_child_secondary(*button , true);
    }

    if (response_id != 0) {
        // Re-emit clicked signals as response signals
        button->signal_clicked().connect(sigc::bind(_signal_response.make_slot(), response_id));
        _response_map[response_id] = button;
    }

    return button;
}

void Panel::setResponseSensitive(int response_id, bool setting)
{
    if (_response_map[response_id])
        _response_map[response_id]->set_sensitive(setting);
}

sigc::signal<void, SPDesktop *, SPDocument *> &
Panel::signalDocumentReplaced()
{
    return _signal_document_replaced;
}

sigc::signal<void, SPDesktop *> &
Panel::signalActivateDesktop()
{
    return _signal_activate_desktop;
}

sigc::signal<void, SPDesktop *> &
Panel::signalDeactiveDesktop()
{
    return _signal_deactive_desktop;
}

void Panel::_handleResponse(int response_id)
{
    switch (response_id) {
        case Gtk::RESPONSE_APPLY: {
            _apply();
            break;
        }
    }
}

Inkscape::Selection *Panel::_getSelection()
{
    return _desktop->getSelection();
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

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
