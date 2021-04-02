// SPDX-License-Identifier: GPL-2.0-or-later

/** @file
 * @brief A class containing all the actions that have been created for a certain
 *              verb. Actions are referenced by the view that they are created for
 *              or if exists, by an associated document.
 *
 * Authors: Osama Ahmad
 *
 * Copyright (c) 2021 Osama Ahmad, Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_ACTIONTABLE_H
#define INKSCAPE_ACTIONTABLE_H

#include <map>
#include "src/helper/action.h"

class ActionTable
{
    std::map<Inkscape::UI::View::View *, SPAction *> actionsByView;
    std::map<SPDocument *, SPAction *> actionsByDocument;

public:

    bool empty() const;

    // Could've just overloaded them. But I think that's clearer.
    SPAction *findByView(Inkscape::UI::View::View * const) const;
    SPAction *findByDocument(SPDocument * const) const;

    // Can return an iterator, but it's intended not to reveal the internals.
    void insert(Inkscape::UI::View::View * const, SPAction * const);
    SPAction *erase(Inkscape::UI::View::View * const);

    const std::map<Inkscape::UI::View::View *, SPAction *>& getActions();
};

#endif // INKSCAPE_ACTIONTABLE_H
