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

#include "action-table.h"
#include "ui/view/view.h"
#include "document.h"

template <typename paramType>
SPAction *_find(const std::map<paramType *, SPAction *> &container, paramType * const param) {
    auto iterator = container.find(param);
    if (iterator == container.end()) {
        return nullptr;
    }
    SPAction *action = iterator->second;
    return action;
}

bool ActionTable::empty() const {
    return actionsByView.empty();
}

SPAction *ActionTable::findByView(Inkscape::UI::View::View * const view) const {
    return _find(actionsByView, view);
}

SPAction *ActionTable::findByDocument(SPDocument * const document) const {
    return _find(actionsByDocument, document);
}

void ActionTable::insert(Inkscape::UI::View::View * const view, SPAction * const action) {
    actionsByView.insert({view, action});
    if (view && view->doc() != nullptr) {
        actionsByDocument.insert({view->doc(), action});
    }
}

SPAction *ActionTable::erase(Inkscape::UI::View::View * const view) {
    auto iterator = actionsByView.find(view);
    if (iterator == actionsByView.end()) {
        return nullptr;
    }

    if (view) {
        SPDocument *document = view->doc();
        if (document != nullptr) {
            actionsByDocument.erase(document);
        }
    }

    SPAction *action = iterator->second;
    actionsByView.erase(iterator);
    return action;
}

const std::map<Inkscape::UI::View::View *, SPAction *>& ActionTable::getActions() {
    return actionsByView;
}
