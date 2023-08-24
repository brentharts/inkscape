// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for selection tied to the application and without GUI.
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 * TODO: REMOVE THIS FILE It's really not necessary.
 */

#include "inkscape-application.h"
#include "inkscape.h"
#include "selection.h"
#include <cstdio>
#include "xml/simple-document.h"
#include "xml/node.h"
#include "xml/node-iterators.h"

using namespace Inkscape::IO;

static bool log_actions= false;
static Inkscape::XML::Document *log_data = nullptr; 

// this function is called when in command line we call with parameter --active-window | -q
// is called by a auto add new start and end action that fire first this action
// and keep on till last inserted action is done
void
log_start_helper() {
    //cleanup (we can need to use withot redirect_output to pass file multiple times in a command)
    std::string tmpfile = Glib::build_filename(Glib::get_tmp_dir(), "log_commands.xml");
    if (Glib::file_test(tmpfile, Glib::FILE_TEST_EXISTS)) {
        unlink(tmpfile.c_str());
    } 
    log_actions = true;
    log_data = sp_repr_document_new("logfiledata");
}

// this is the end of previous function. Finish the wrap of actions to active desktop
// it also save a file to allow print in the caller terminal the output to be redeable by
// external programs like extensions.
void
log_end_helper() {
    std::string tmpfile = Glib::build_filename(Glib::get_tmp_dir(), "log_commands.xml");
    Glib::ustring utf8name = Glib::filename_to_utf8(Glib::build_filename(Glib::get_tmp_dir(), "log_commands_prev.xml"));
    sp_repr_save_file(log_data, utf8name.c_str());
    std::rename(utf8name.c_str(), tmpfile.c_str());
    log_actions = false;
    Inkscape::GC::release(log_data);
    log_data = nullptr;
}

void 
show_output(Glib::ustring data, bool is_cerr = true) {
    if (is_cerr) {
        std::cerr << data << std::endl;
    } else {
        std::cout << data << std::endl;
    }
    if (log_actions) {
        if (auto root = log_data->root()) {
            Inkscape::XML::Node * node = nullptr;
            if (is_cerr) {
                node = log_data->createElement("cerr");
            } else {
                node = log_data->createElement("cout");
            }
            root->appendChild(node);
            Inkscape::GC::release(node);
            auto txtnode = log_data->createTextNode("", true);
            node->appendChild(txtnode);
            Inkscape::GC::release(txtnode);
            txtnode->setContent(data.c_str());
        }
    }
}

// Helper function: returns true if both document and selection found. Maybe this should
// work on current view. Or better, application could return the selection of the current view.
bool
get_document_and_selection(InkscapeApplication* app, SPDocument** document, Inkscape::Selection** selection)
{
    *document = app->get_active_document();
    if (!(*document)) {
        show_output("get_document_and_selection: No document!");
        return false;
    }

    *selection = app->get_active_selection();
    if (!*selection) {
        show_output("get_document_and_selection: No selection!");
        return false;
    }

    return true;
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
