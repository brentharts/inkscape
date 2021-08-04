// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2012 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_ID_CLASH_H
#define SEEN_ID_CLASH_H

#include "document.h"

void prevent_id_clashes(SPDocument *imported_doc, SPDocument *current_doc);
void rename_id(SPObject *elem, Glib::ustring const &newname);
void change_def_references(SPObject *replace_obj, SPObject *with_obj);
Glib::ustring generate_unique_id(SPDocument* document, const Glib::ustring& base_name);

#endif /* !SEEN_ID_CLASH_H */

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
