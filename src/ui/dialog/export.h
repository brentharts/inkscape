// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *
 * Copyright (C) 1999-2007 Authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SP_EXPORT_H
#define SP_EXPORT_H

#include <gtkmm.h>

#include "export-helper.h"
#include "export-single.h"
#include "extension/output.h"
#include "ui/dialog/dialog-base.h"
#include "ui/widget/scrollprotected.h"
#include "ui/widget/unit-menu.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

enum notebook_page
{
    SINGLE_IMAGE = 0,
    BATCH_EXPORT
};

class Export : public DialogBase
{
public:
    Export();
    ~Export() override;

    static Export &getInstance() { return *new Export(); }

private:
    Glib::RefPtr<Gtk::Builder> builder;
    // Main Container
    Gtk::Box *container = nullptr;
    // Notebook containing pages for diiferent export types
    Gtk::Notebook *export_notebook = nullptr;

private:
    SingleExport *single_image = nullptr;
    Gtk::Box *batch_export = nullptr;


private:
    // Utils Variables
    Inkscape::Preferences *prefs = nullptr;

    // setup default values of widgets
    void setDefaultNotebookPage();

    // Export Functions
    bool _export_raster(Geom::Rect const &area, unsigned long int const &width, unsigned long int const &height,
                        float const &dpi, Glib::ustring const &filename, bool overwrite,
                        Inkscape::Extension::Output *extension);
    bool _export_vector(Inkscape::Extension::Output *extension = nullptr, std::vector<SPItem *> *items = nullptr);

    // signals callback
    void onRealize();
    void onExport();
    void onBatchExport();

    /**
     * Inkscape selection change callback
     */
    void onSelectionChanged();
    void onSelectionModified(guint flags);

    // signals
    sigc::connection selectChangedConn;
    sigc::connection subselChangedConn;
    sigc::connection selectModifiedConn;
};
} // namespace Dialog
} // namespace UI
} // namespace Inkscape
#endif

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