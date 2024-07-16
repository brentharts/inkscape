// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief A dialog which contains memory information and message logs.
 *
 * Authors: see git history
 *   Kaixo Gamorra
 *
 * Copyright (c) 2024 Kaixo Gamorra, Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_WINDOW_H
#define INKSCAPE_UI_DIALOG_WINDOW_H

#include "gtkmm/notebook.h"

#include "ui/dialog/dialog-base.h"
#include "ui/dialog/dialog-container.h"
#include "ui/widget/memory.h"
#include "ui/widget/messages.h"

class InkscapeWindow;

using Inkscape::UI::Widget::Memory;
using Inkscape::UI::Widget::Messages;

namespace Inkscape::UI::Dialog {

class Debug : public DialogBase {
    public:
        Debug();
        ~Debug() override;
    
    private:
      Gtk::Notebook* notebook;
      Memory* memory;
      Messages* messages;

};

} // namespace Inkscape::UI::Dialog

#endif // INKSCAPE_UI_DIALOG_WINDOW_H

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
