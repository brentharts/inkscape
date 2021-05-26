// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for switching tools.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <iostream>
#include <map>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include "config.h"  // #ifdef WITH_GSPELL

#include "actions-dialogs.h"

#include "inkscape-application.h"
#include "inkscape-window.h"

#include "verbs.h"  // TEMPORARY!!!

#include "ui/dialog/dialog-container.h"
#include "ui/icon-names.h"

// TEMPORARY UNTIL VERBS ARE REPLACED IN DialogContainer.
static std::map<Glib::ustring, unsigned int> dialog_to_verb =
{
    {"AlignDistribute",    SP_VERB_DIALOG_ALIGN_DISTRIBUTE },
    {"ObjectAttributes",   SP_VERB_DIALOG_ATTR             },
    {"AttrDialog",         SP_VERB_DIALOG_ATTR_XML         },
    {"Clonetiler",         SP_VERB_DIALOG_CLONETILER       },
    {"Debug",              SP_VERB_DIALOG_DEBUG            },
    {"DocumentProperties", SP_VERB_DIALOG_DOCPROPERTIES    },
    {"Export",             SP_VERB_DIALOG_EXPORT           },
    {"FillStroke",         SP_VERB_DIALOG_FILL_STROKE      },
    {"FilterEffects",      SP_VERB_DIALOG_FILTER_EFFECTS   },
    {"Find",               SP_VERB_DIALOG_FIND             },
    {"Glyphs",             SP_VERB_DIALOG_GLYPHS           },
    {"Input",              SP_VERB_DIALOG_INPUT            },
    {"ObjectProperties",   SP_VERB_DIALOG_ITEM             },
    {"Layers",             SP_VERB_DIALOG_LAYERS           },
    {"LivePathEffect",     SP_VERB_DIALOG_LIVE_PATH_EFFECT },
    {"Objects",            SP_VERB_DIALOG_OBJECTS          },
    {"PaintServers",       SP_VERB_DIALOG_PAINT            },
    {"Preferences",        SP_VERB_DIALOG_PREFERENCES      },
    {"Selectors",          SP_VERB_DIALOG_SELECTORS        },
    {"Style",              SP_VERB_DIALOG_STYLE            },
    {"SVGFonts",           SP_VERB_DIALOG_SVG_FONTS        },
    {"Swatches",           SP_VERB_DIALOG_SWATCHES         },
    {"Symbols",            SP_VERB_DIALOG_SYMBOLS          },
    {"Text",               SP_VERB_DIALOG_TEXT             },
    {"sToggle",            SP_VERB_DIALOG_TOGGLE           },
    {"Transform",          SP_VERB_DIALOG_TRANSFORM        },
    {"UndoHistory",        SP_VERB_DIALOG_UNDO_HISTORY     },
    {"XMLEditor",          SP_VERB_DIALOG_XML_EDITOR       },
#if WITH_GSPELL
    {"Spellcheck",         SP_VERB_DIALOG_SPELLCHECK       },
#endif
#if DEBUG
    {"Prototype",          SP_VERB_DIALOG_PROTOTYPE        },
#endif
};

class DialogData {
public:
    int dialog = DIALOG_INVALID; // TODO: Switch to named enum
    Glib::ustring label;
    Glib::ustring tip;
    Glib::ustring icon_name;
};

// TODO: The strings should be in .ui files. The icons in the header file.
static std::map<Glib::ustring, DialogData> dialog_data =
{
    // clang-format off
    {"AlignDistribute",    {DIALOG_ALIGN_DISTRIBUTE,  N_("_Align and Distribute..."),INKSCAPE_ICON("dialog-align-and-distribute")}},
    {"ObjectAttributes",   {DIALOG_ATTR,              N_("_Object attributes..."),   INKSCAPE_ICON("dialog-object-properties")}},
    {"AttrDialog",         {DIALOG_ATTR_XML,          N_("_Object attributes..."),   INKSCAPE_ICON("dialog-object-properties")}},
    {"Clonetiler",         {DIALOG_CLONETILER,        N_("Create Tiled Clones..."),  INKSCAPE_ICON("dialog-tile-clones")}},
    {"Debug",              {DIALOG_DEBUG,             N_("_Messages..."),            INKSCAPE_ICON("dialog-messages")}},
    {"DocumentProperties", {DIALOG_DOCPROPERTIES,     N_("_Document Properties..."), INKSCAPE_ICON("document-properties")}},
    {"Export",             {DIALOG_EXPORT,            N_("_Export PNG Image..."),    INKSCAPE_ICON("document-export")}},
    {"FillStroke",         {DIALOG_FILL_STROKE,       N_("_Fill and Stroke..."),     INKSCAPE_ICON("dialog-fill-and-stroke")}},
    {"FilterEffects",      {DIALOG_FILTER_EFFECTS,    N_("Filter _Editor..."),       INKSCAPE_ICON("dialog-filters")}},
    {"Find",               {DIALOG_FIND,              N_("_Find/Replace..."),        INKSCAPE_ICON("edit-find")}},
    {"Glyphs",             {DIALOG_GLYPHS,            N_("_Unicode Characters..."),  INKSCAPE_ICON("accessories-character-map")}},
    {"Input",              {DIALOG_INPUT,             N_("_Input Devices..."),       INKSCAPE_ICON("dialog-input-devices")}},
    {"ObjectProperties",   {DIALOG_ITEM,              N_("_Object Properties..."),   INKSCAPE_ICON("dialog-object-properties")}},
    {"Layers",             {DIALOG_LAYERS,            N_("Layer_s..."),              INKSCAPE_ICON("dialog-layers")}},
    {"LivePathEffect",     {DIALOG_LIVE_PATH_EFFECT,  N_("Path E_ffects..."),        INKSCAPE_ICON("dialog-path-effects")}},
    {"Objects",            {DIALOG_OBJECTS,           N_("Object_s..."),             INKSCAPE_ICON("dialog-objects")}},
    {"PaintServers",       {DIALOG_PAINT,             N_("_Paint Servers..."),       INKSCAPE_ICON("symbols")}},
    {"Preferences",        {DIALOG_PREFERENCES,       N_("P_references"),            INKSCAPE_ICON("preferences-system")}},
    {"Selectors",          {DIALOG_SELECTORS,         N_("_Selectors and CSS..."),   INKSCAPE_ICON("dialog-selectors")}},
    {"Style",              {DIALOG_STYLE,             N_("Style Dialog..."),         ""}},
    {"SVGFonts",           {DIALOG_SVG_FONTS,         N_("SVG Font Editor..."),      ""}},
    {"Swatches",           {DIALOG_SWATCHES,          N_("S_watches..."),            INKSCAPE_ICON("swatches")}},
    {"Symbols",            {DIALOG_SYMBOLS,           N_("S_ymbols..."),             INKSCAPE_ICON("symbols")}},
    {"Text",               {DIALOG_TEXT,              N_("_Text and Font..."),       INKSCAPE_ICON("dialog-text-and-font")}},
    {"sToggle",            {DIALOG_TOGGLE,            N_("Show/Hide D_ialogs"),      INKSCAPE_ICON("show-dialogs")}},
    {"Transform",          {DIALOG_TRANSFORM,         N_("Transfor_m..."),           INKSCAPE_ICON("dialog-transform")}},
    {"UndoHistory",        {DIALOG_UNDO_HISTORY,      N_("Undo _History..."),        INKSCAPE_ICON("edit-undo-history")}},
    {"XMLEditor",          {DIALOG_XML_EDITOR,        N_("_XML Editor..."),          INKSCAPE_ICON("dialog-xml-editor")}},
#if WITH_GSPELL
    {"Spellcheck",         {DIALOG_SPELLCHECK,        N_("Check Spellin_g..."),      INKSCAPE_ICON("tools-check-spelling")}},
#endif
#if DEBUG
    {"Prototype",          {DIALOG_PROTOTYPE,         N_("Prototype..."),            INKSCAPE_ICON("document-properties")}},
#endif
    // clang-format on
};

std::vector<std::vector<Glib::ustring>> raw_data_dialogs =
{
    // clang-format off
    {"win.dialog-open('AlignDistribute')",    N_("Open AlignDistribute"),      "Dialog",  N_("Align and distribute objects")                                                           },
    {"win.dialog-open('ObjectAttributes')",   N_("Open ObjectAttributes"),     "Dialog",  N_("Edit the object attributes...")                                                          },
    {"win.dialog-open('AttrDialog')",         N_("Open AttrDialog"),           "Dialog",  N_("Edit the object attributes...")                                                          },
    {"win.dialog-open('Clonetiler')",         N_("Open Clonetiler"),           "Dialog",  N_("Create multiple clones of selected object, arranging them into a pattern or scattering") },
    {"win.dialog-open('Debug')",              N_("Open Debug"),                "Dialog",  N_("View debug messages")                                                                    },
    {"win.dialog-open('DocumentProperties')", N_("Open DocumentProperties"),   "Dialog",  N_("Edit properties of this document (to be saved with the document)")                       },
    {"win.dialog-open('Export')",             N_("Open Export"),               "Dialog",  N_("Export this document or a selection as a PNG image")                                     },
    {"win.dialog-open('FillStroke')",         N_("Open FillStroke"),           "Dialog",  N_("Edit objects' colors, gradients, arrowheads, and other fill and stroke properties...")   },
    {"win.dialog-open('FilterEffects')",      N_("Open FilterEffects"),        "Dialog",  N_("Manage, edit, and apply SVG filters")                                                    },
    {"win.dialog-open('Find')",               N_("Open Find"),                 "Dialog",  N_("Find objects in document")                                                               },
    {"win.dialog-open('Glyphs')",             N_("Open Glyphs"),               "Dialog",  N_("Select Unicode characters from a palette")                                               },
    {"win.dialog-open('Input')",              N_("Open Input"),                "Dialog",  N_("Configure extended input devices, such as a graphics tablet")                            },
    {"win.dialog-open('ObjectProperties')",   N_("Open ObjectProperties"),     "Dialog",  N_("Edit the ID, locked and visible status, and other object properties")                    },
    {"win.dialog-open('Layers')",             N_("Open Layers"),               "Dialog",  N_("View Layers")                                                                            },
    {"win.dialog-open('LivePathEffect')",     N_("Open LivePathEffect"),       "Dialog",  N_("Manage, edit, and apply path effects")                                                   },
    {"win.dialog-open('Objects')",            N_("Open Objects"),              "Dialog",  N_("View Objects")                                                                           },
    {"win.dialog-open('PaintServers')",       N_("Open PaintServers"),         "Dialog",  N_("Select paint server from a collection")                                                  },
    {"win.dialog-open('Preferences')",        N_("Open Preferences"),          "Dialog",  N_("Edit global Inkscape preferences")                                                       },
    {"win.dialog-open('Selectors')",          N_("Open Selectors"),            "Dialog",  N_("View and edit CSS selectors and styles")                                                 },
    {"win.dialog-open('Style')",              N_("Open Style"),                "Dialog",  N_("View Style Dialog")                                                                      },
    {"win.dialog-open('SVGFonts')",           N_("Open SVGFonts"),             "Dialog",  N_("Edit SVG fonts")                                                                         },
    {"win.dialog-open('Swatches')",           N_("Open Swatches"),             "Dialog",  N_("Select colors from a swatches palette") /* TRANSLATORS: "Swatches" -> color samples */   },
    {"win.dialog-open('Symbols')",            N_("Open Symbols"),              "Dialog",  N_("Select symbol from a symbols palette")                                                   },
    {"win.dialog-open('Text')",               N_("Open Text"),                 "Dialog",  N_("View and select font family, font size and other text properties")                       },
    {"win.dialog-open('Transform')",          N_("Open Transform"),            "Dialog",  N_("Precisely control objects' transformations")                                             },
    {"win.dialog-open('UndoHistory')",        N_("Open UndoHistory"),          "Dialog",  N_("Undo History")                                                                           },
    {"win.dialog-open('XMLEditor')",          N_("Open XMLEditor"),            "Dialog",  N_("View and edit the XML tree of the document")                                             },
#if WITH_GSPELL
    {"win.dialog-open('Spellcheck')",         N_("Open Spellcheck"),           "Dialog",  N_("Check spelling of text in document")                                                     },
#endif
#if DEBUG
    {"win.dialog-open('Prototype')",          N_("Open Prototype"),            "Dialog",  N_("Prototype Dialog")                                                                       },
#endif

    {"win.dialog-toggle",                     N_("Toggle all dialogs"),        "Dialog",  N_("Show or hide all dialogs")                                                               },
    // clang-format on
};

/**
 * Open dialog.
 */
void
dialog_open(Glib::ustring const &dialog, InkscapeWindow *win)
{
    auto dialog_it = dialog_data.find(dialog);
    if (dialog_it == dialog_data.end()) {
        std::cerr << "dialog_open: invalid dialog name: " << dialog << std::endl;
        return;
    }

    SPDesktop* dt = win->get_desktop();
    if (!dt) {
        std::cerr << "dialog_toggle: no desktop!" << std::endl;
        return;
    }

    unsigned int code = dialog_to_verb[dialog];
    Inkscape::UI::Dialog::DialogContainer *container = dt->getContainer();
    container->new_dialog(code);

}

/**
 * Toggle between showing and hiding dialogs.
 */
void
dialog_toggle(InkscapeWindow *win)
{
    SPDesktop* dt = win->get_desktop();
    if (!dt) {
        std::cerr << "dialog_toggle: no desktop!" << std::endl;
        return;
    }

    // Keep track of state?
    // auto action = win->lookup_action("dialog-toggle");
    // if (!action) {
    //     std::cerr << "dialog_toggle: action 'dialog-toggle' missing!" << std::endl;
    //     return;
    // }

    // auto saction = Glib::RefPtr<Gio::SimpleAction>::cast_dynamic(action);
    // if (!saction) {
    //     std::cerr << "dialog_toogle: action 'dialog_switch' not SimpleAction!" << std::endl;
    //     return;
    // }

    // saction->get_state();

    Inkscape::UI::Dialog::DialogContainer *container = dt->getContainer();
    container->toggle_dialogs();
}

void
add_actions_dialogs(InkscapeWindow* win)
{
    // clang-format off
    win->add_action_radio_string ( "dialog-open",        sigc::bind<InkscapeWindow*>(sigc::ptr_fun(&dialog_open),   win), "Find");
    win->add_action(               "dialog-toggle",      sigc::bind<InkscapeWindow*>(sigc::ptr_fun(&dialog_toggle), win)        );
    // clang-format on

    auto app = InkscapeApplication::instance();
    if (!app) {
        std::cerr << "add_actions_dialog: no app!" << std::endl;
        return;
    }

    app->get_action_extra_data().add_data(raw_data_dialogs);
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
