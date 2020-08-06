// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Macros dialog - implementation.
 * Macros group of action that can be repeated many times
 */
/* Author:
 *   Abhay Raj Singh <abhayonlyone@gmail.com>
 *
 * Copyright (C) 2020 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "macros.h"

#include <gtkmm/builder.h>
#include <iostream>
#include <sigc++/functors/mem_fun.h>

#include "io/resource.h"
#include "ui/widget/panel.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

Macros::Macros()
    : UI::Widget::Panel()
{
    std::string gladefile = IO::Resource::get_filename_string(Inkscape::IO::Resource::UIS, "dialog-macros.glade");
    {
        Glib::RefPtr<Gtk::Builder> builder;
        try {
            builder = Gtk::Builder::create_from_file(gladefile);
        } catch (const Glib::Error &ex) {
            g_warning("GtkBuilder file loading failed for Macros dialog");
            return;
        }

        // Linking UI
        builder->get_widget("MacroBase", _MacroBase);

        builder->get_widget("MacroCreate", _MacroCreate);
        builder->get_widget("MacroDelete", _MacroDelete);
        builder->get_widget("MacroImport", _MacroImport);
        builder->get_widget("MacroExport", _MacroExport);

        builder->get_widget("MacroRecord", _MacroRecord);
        builder->get_widget("MacroPlay", _MacroPlay);
        builder->get_widget("MacroEdit", _MacroEdit);

        // Adding signals
        _MacroCreate->signal_button_press_event().connect(sigc::mem_fun(*this, &Macros::on_macro_create));
        _MacroDelete->signal_button_press_event().connect(sigc::mem_fun(*this, &Macros::on_macro_delete));
        _MacroImport->signal_button_press_event().connect(sigc::mem_fun(*this, &Macros::on_macro_import));
        _MacroExport->signal_button_press_event().connect(sigc::mem_fun(*this, &Macros::on_macro_export));

        _MacroRecord->signal_button_press_event().connect(sigc::mem_fun(*this, &Macros::on_macro_record));
        _MacroPlay->signal_button_press_event().connect(sigc::mem_fun(*this, &Macros::on_macro_play));
        _MacroEdit->signal_button_press_event().connect(sigc::mem_fun(*this, &Macros::on_macro_edit));

        // TODO: Initialize Marcos tree
    }
}
bool Macros::on_macro_create()
{
    std::cout << "Macro create not implemented" << std::endl;
    return true;
}

bool Macros::on_macro_delete()
{
    std::cout << "Macro delete not implemented" << std::endl;
    return true;
}

bool Macros::on_macro_import()
{
    std::cout << "Macro import not implemented" << std::endl;
    return true;
}

bool Macros::on_macro_export()
{
    std::cout << "Macro export not implemented" << std::endl;
    return true;
}

bool Macros::on_macro_record()
{
    std::cout << "Macro record not implemented" << std::endl;
    return true;
}

bool Macros::on_macro_play()
{
    std::cout << "Macro play not implemented" << std::endl;
    return true;
}

bool Macros::on_macro_edit()
{
    std::cout << "Macro edit not implemented" << std::endl;
    return true;
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape
