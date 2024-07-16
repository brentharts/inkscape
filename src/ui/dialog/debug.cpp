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

#include "debug.h"

namespace Inkscape::UI::Dialog {

Debug::Debug()
    : DialogBase("/dialogs/debug", "Debug"), 
      notebook (new Gtk::Notebook()),
      memory   (new Memory()),
      messages (new Messages())
{
    notebook->append_page(*memory);
    notebook->append_page(*messages);

    this->insert_child_at_start(*notebook);
};

Debug::~Debug() {}

} // namespace Inkscape::UI::Dialog::Debug
