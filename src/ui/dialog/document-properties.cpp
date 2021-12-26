// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Document properties dialog, Gtkmm-style.
 */
/* Authors:
 *   bulia byak <buliabyak@users.sf.net>
 *   Bryce W. Harrington <bryce@bryceharrington.org>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon Phillips <jon@rejon.org>
 *   Ralf Stephan <ralf@ark.in-berlin.de> (Gtkmm)
 *   Diederik van Lierop <mail@diedenrezi.nl>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006-2008 Johan Engelen  <johan@shouraizou.nl>
 * Copyright (C) 2000 - 2008 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#include <vector>
#include "style.h"
#include "rdf.h"
#include "verbs.h"

#include "actions/actions-tools.h"
#include "display/control/canvas-grid.h"
#include "document-properties.h"
#include "helper/action.h"
#include "include/gtkmm_version.h"
#include "io/sys.h"
#include "object/sp-root.h"
#include "object/sp-script.h"
#include "object/color-profile.h"
#include "ui/dialog/filedialog.h"
#include "ui/icon-loader.h"
#include "ui/icon-names.h"
#include "ui/shape-editor.h"
#include "ui/widget/entity-entry.h"
#include "ui/widget/notebook-page.h"
#include "xml/node-event-vector.h"

#include "page-manager.h"
#include "svg/svg-color.h"

#include "ui/widget/page-properties.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

#define SPACE_SIZE_X 15
#define SPACE_SIZE_Y 10

//===================================================

static void on_child_added(Inkscape::XML::Node *repr, Inkscape::XML::Node *child, Inkscape::XML::Node *ref, void * data);
static void on_child_removed(Inkscape::XML::Node *repr, Inkscape::XML::Node *child, Inkscape::XML::Node *ref, void * data);
static void on_repr_attr_changed (Inkscape::XML::Node *, gchar const *, gchar const *, gchar const *, bool, gpointer);

static Inkscape::XML::NodeEventVector const _repr_events = {
    on_child_added, // child_added
    on_child_removed, // child_removed
    on_repr_attr_changed,
    nullptr, // content_changed
    nullptr  // order_changed
};

static void docprops_style_button(Gtk::Button& btn, char const* iconName)
{
    GtkWidget *child = sp_get_icon_image(iconName, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_show( child );
    btn.add(*Gtk::manage(Glib::wrap(child)));
    btn.set_relief(Gtk::RELIEF_NONE);
}

DocumentProperties& DocumentProperties::getInstance()
{
    DocumentProperties &instance = *new DocumentProperties();
    instance.init();

    return instance;
}

DocumentProperties::DocumentProperties()
    : DialogBase("/dialogs/documentoptions", "DocumentProperties")
    , _page_page(Gtk::manage(new UI::Widget::NotebookPage(1, 1, false, true)))
    , _page_guides(Gtk::manage(new UI::Widget::NotebookPage(1, 1)))
    , _page_cms(Gtk::manage(new UI::Widget::NotebookPage(1, 1)))
    , _page_scripting(Gtk::manage(new UI::Widget::NotebookPage(1, 1)))
    , _page_external_scripts(Gtk::manage(new UI::Widget::NotebookPage(1, 1)))
    , _page_embedded_scripts(Gtk::manage(new UI::Widget::NotebookPage(1, 1)))
    , _page_metadata1(Gtk::manage(new UI::Widget::NotebookPage(1, 1)))
    , _page_metadata2(Gtk::manage(new UI::Widget::NotebookPage(1, 1)))
    //---------------------------------------------------------------
    //General guide options
    , _rcb_sgui(_("Show _guides"), _("Show or hide guides"), "showguides", _wr)
    , _rcb_lgui(_("Lock all guides"), _("Toggle lock of all guides in the document"), "inkscape:lockguides", _wr)
    , _rcp_gui(_("Guide co_lor:"), _("Guideline color"), _("Color of guidelines"), "guidecolor", "guideopacity", _wr)
    , _rcp_hgui(_("_Highlight color:"), _("Highlighted guideline color"), _("Color of a guideline when it is under mouse"), "guidehicolor", "guidehiopacity", _wr)
    , _create_guides_btn(_("Create guides around the page"))
    , _delete_guides_btn(_("Delete all guides"))
    //---------------------------------------------------------------
    , _grids_label_crea("", Gtk::ALIGN_START)
    , _grids_button_new(C_("Grid", "_New"), _("Create new grid."))
    , _grids_button_remove(C_("Grid", "_Remove"), _("Remove selected grid."))
    , _grids_label_def("", Gtk::ALIGN_START)
    , _grids_vbox(Gtk::ORIENTATION_VERTICAL)
    , _grids_hbox_crea(Gtk::ORIENTATION_HORIZONTAL)
    , _grids_space(Gtk::ORIENTATION_HORIZONTAL)
{
    set_spacing (0);
    pack_start(_notebook, true, true);

    _notebook.append_page(*_page_page,      _("Display"));
    _notebook.append_page(*_page_guides,    _("Guides"));
    _notebook.append_page(_grids_vbox,      _("Grids"));
    _notebook.append_page(*_page_cms,       _("Color"));
    _notebook.append_page(*_page_scripting, _("Scripting"));
    _notebook.append_page(*_page_metadata1, _("Metadata"));
    _notebook.append_page(*_page_metadata2, _("License"));

    _wr.setUpdating (true);
    build_page();
    build_guides();
    build_gridspage();
    build_cms();
    build_scripting();
    build_metadata();
    _wr.setUpdating (false);

    _grids_button_new.signal_clicked().connect(sigc::mem_fun(*this, &DocumentProperties::onNewGrid));
    _grids_button_remove.signal_clicked().connect(sigc::mem_fun(*this, &DocumentProperties::onRemoveGrid));
}

void DocumentProperties::init()
{
    show_all_children();
    _grids_button_remove.hide();
}

DocumentProperties::~DocumentProperties()
{
    for (auto & it : _rdflist)
        delete it;
}

//========================================================================

/**
 * Helper function that sets widgets in a 2 by n table.
 * arr has two entries per table row. Each row is in the following form:
 *     widget, widget -> function adds a widget in each column.
 *     nullptr, widget -> function adds a widget that occupies the row.
 *     label, nullptr -> function adds label that occupies the row.
 *     nullptr, nullptr -> function adds an empty box that occupies the row.
 * This used to be a helper function for a 3 by n table
 */
void attach_all(Gtk::Grid &table, Gtk::Widget *const arr[], unsigned const n)
{
    for (unsigned i = 0, r = 0; i < n; i += 2) {
        if (arr[i] && arr[i+1]) {
            arr[i]->set_hexpand();
            arr[i+1]->set_hexpand();
            arr[i]->set_valign(Gtk::ALIGN_CENTER);
            arr[i+1]->set_valign(Gtk::ALIGN_CENTER);
            table.attach(*arr[i],   0, r, 1, 1);
            table.attach(*arr[i+1], 1, r, 1, 1);
        } else {
            if (arr[i+1]) {
                Gtk::AttachOptions yoptions = (Gtk::AttachOptions)0;
                arr[i+1]->set_hexpand();

                if (yoptions & Gtk::EXPAND)
                    arr[i+1]->set_vexpand();
                else
                    arr[i+1]->set_valign(Gtk::ALIGN_CENTER);

                table.attach(*arr[i+1], 0, r, 2, 1);
            } else if (arr[i]) {
                Gtk::Label& label = reinterpret_cast<Gtk::Label&>(*arr[i]);

                label.set_hexpand();
                label.set_halign(Gtk::ALIGN_START);
                label.set_valign(Gtk::ALIGN_CENTER);
                table.attach(label, 0, r, 2, 1);
            } else {
                auto space = Gtk::manage (new Gtk::Box);
                space->set_size_request (SPACE_SIZE_X, SPACE_SIZE_Y);

                space->set_halign(Gtk::ALIGN_CENTER);
                space->set_valign(Gtk::ALIGN_CENTER);
                table.attach(*space, 0, r, 1, 1);
            }
        }
        ++r;
    }
}

void set_namedview_bool(SPDesktop* desktop, const Glib::ustring& operation, SPAttr key, bool on) {
    if (!desktop || !desktop->getDocument()) return;

    desktop->getNamedView()->change_bool_setting(key, on);

    desktop->getDocument()->setModifiedSinceSave();
    DocumentUndo::done(desktop->getDocument(), operation, "");
}

void set_color(SPDesktop* desktop, Glib::ustring operation, unsigned int rgba, SPAttr color_key, SPAttr opacity_key = SPAttr::INVALID) {
    if (!desktop || !desktop->getDocument()) return;

    desktop->getNamedView()->change_color(rgba, color_key, opacity_key);

    desktop->getDocument()->setModifiedSinceSave();
    DocumentUndo::maybeDone(desktop->getDocument(), ("document-color-" + operation).c_str(), operation, "");
}

void set_document_dimensions(SPDesktop* desktop, double width, double height, const Inkscape::Util::Unit* unit) {
    if (!desktop) return;

    Inkscape::Util::Quantity w = Inkscape::Util::Quantity(width, unit);
    Inkscape::Util::Quantity h = Inkscape::Util::Quantity(height, unit);
    bool changeSize = true;
    SPDocument* doc = desktop->getDocument();
    Inkscape::Util::Quantity const old_height = doc->getHeight();
    doc->setWidthAndHeight(w, h, changeSize);
    // The origin for the user is in the lower left corner; this point should remain stationary when
    // changing the page size. The SVG's origin however is in the upper left corner, so we must compensate for this
    if (changeSize && !doc->is_yaxisdown()) {
        Geom::Translate const vert_offset(Geom::Point(0, (old_height.value("px") - h.value("px"))));
        doc->getRoot()->translateChildItems(vert_offset);
    }
    // units: this is most likely not needed, units are part of document size attributes
    // if (unit) {
        // set_namedview_value(desktop, "", SPAttr::UNITS)
        // write_str_to_xml(desktop, _("Set document unit"), "unit", unit->abbr.c_str());
    // }
    DocumentUndo::done(doc, _("Set page size"), "");
}

void DocumentProperties::set_viewbox_pos(SPDesktop* desktop, double x, double y) {
    if (!desktop) return;

    auto document = desktop->getDocument();
    if (!document) return;

    auto box = document->getViewBox();
    document->setViewBox(Geom::Rect::from_xywh(x, y, box.width(), box.height()));
    DocumentUndo::done(document, _("Set viewbox position"), "");
    update_scale_ui(desktop);
}

void DocumentProperties::set_viewbox_size(SPDesktop* desktop, double width, double height) {
    if (!desktop) return;

    auto document = desktop->getDocument();
    if (!document) return;

    auto box = document->getViewBox();
    document->setViewBox(Geom::Rect::from_xywh(box.min()[Geom::X], box.min()[Geom::Y], width, height));
    DocumentUndo::done(document, _("Set viewbox size"), "");
    update_scale_ui(desktop);
}

void DocumentProperties::set_document_scale(SPDesktop* desktop, double scale_x) {
    if (!desktop) return;

    auto document = desktop->getDocument();
    if (!document) return;

    if (scale_x > 0) {
        // _lock_scale_update
        document->setDocumentScale(scale_x);
        update_viewbox_ui(desktop);
        update_scale_ui(desktop);
        DocumentUndo::done(document, _("Set page scale"), "");
    }
}

void DocumentProperties::update_scale_ui(SPDesktop* desktop) {
    if (!desktop) return;

    auto document = desktop->getDocument();
    if (!document) return;

    using UI::Widget::PageProperties;

    // if (!_lockScaleUpdate) {
        Geom::Scale scale = document->getDocumentScale();
        _page->set_dimension(PageProperties::Dimension::Scale, scale[Geom::X], scale[Geom::Y]);
    // }
}

void DocumentProperties::update_viewbox_ui(SPDesktop* desktop) {
    if (!desktop) return;

    auto document = desktop->getDocument();
    if (!document) return;

    using UI::Widget::PageProperties;
    // if (!_lockViewboxUpdate) {
        Geom::Rect viewBox = document->getViewBox();
        _page->set_dimension(PageProperties::Dimension::ViewboxPosition, viewBox.min()[Geom::X], viewBox.min()[Geom::Y]);
        _page->set_dimension(PageProperties::Dimension::ViewboxSize, viewBox.width(), viewBox.height());
    // }
}

void DocumentProperties::build_page()
{
    using UI::Widget::PageProperties;
    _page = Gtk::manage(PageProperties::create());
    _page_page->table().attach(*_page, 0, 0);
    _page_page->show();

    _page->signal_color_changed().connect([=](unsigned int color, PageProperties::Color element){
        if (_wr.isUpdating() || !_wr.desktop()) return;

        _wr.setUpdating(true);
        switch (element) {
            case PageProperties::Color::Desk:
                set_color(_wr.desktop(), _("Desk color"), color, SPAttr::INKSCAPE_DESK_COLOR);
                break;
            case PageProperties::Color::Background:
                set_color(_wr.desktop(), _("Background color"), color, SPAttr::PAGECOLOR, SPAttr::INKSCAPE_PAGEOPACITY);
                break;
            case PageProperties::Color::Border:
                set_color(_wr.desktop(), _("Border color"), color, SPAttr::BORDERCOLOR, SPAttr::BORDEROPACITY);
                break;
        }
        _wr.setUpdating(false);
    });

    _page->signal_dimmension_changed().connect([=](double x, double y, const Inkscape::Util::Unit* unit, PageProperties::Dimension element){
        if (_wr.isUpdating() || !_wr.desktop()) return;

        _wr.setUpdating(true);
        switch (element) {
            case PageProperties::Dimension::PageSize:
                set_document_dimensions(_wr.desktop(), x, y, unit);
                // update viewbox and scale
                update_viewbox(_wr.desktop());
                break;

            case PageProperties::Dimension::ViewboxSize:
                set_viewbox_size(_wr.desktop(), x, y);
                break;

            case PageProperties::Dimension::ViewboxPosition:
                set_viewbox_pos(_wr.desktop(), x, y);
                break;

            case PageProperties::Dimension::Scale:
                set_document_scale(_wr.desktop(), x); // uniform scale; y cannot be changed in the UI
        }
        _wr.setUpdating(false);
    });

    _page->signal_check_toggled().connect([=](bool checked, PageProperties::Check element){
        if (_wr.isUpdating() || !_wr.desktop()) return;

        _wr.setUpdating(true);
        switch (element) {
            case PageProperties::Check::Checkerboard:
                set_namedview_bool(_wr.desktop(), _("Toggle checkerboard"), SPAttr::INKSCAPE_DESK_CHECKERBOARD, checked);
                break;
            case PageProperties::Check::Border:
                set_namedview_bool(_wr.desktop(), _("Toggle page border"), SPAttr::SHOWBORDER, checked);
                break;
            case PageProperties::Check::BorderOnTop:
                set_namedview_bool(_wr.desktop(), _("Toggle border on top"), SPAttr::BORDERLAYER, checked);
                break;
            case PageProperties::Check::Shadow:
                set_namedview_bool(_wr.desktop(), _("Toggle page shadow"), SPAttr::SHOWPAGESHADOW, checked);
                break;
            case PageProperties::Check::AntiAlias:
                set_namedview_bool(_wr.desktop(), _("Toggle anti-aliasing"), SPAttr::SHAPE_RENDERING, checked);
                break;
        }
        _wr.setUpdating(false);
    });

    _page->signal_unit_changed().connect([=](const Inkscape::Util::Unit* unit, PageProperties::Units element){
        if (_wr.isUpdating() || !_wr.desktop()) return;

        if (element == PageProperties::Units::Display) {
            // display only units
            display_unit_change(unit);
        }
        else if (element == PageProperties::Units::Document) {
            // document (svg width/height) unit
            // document_unit_change(unit);
        }
    });

    _page->signal_resize_to_fit().connect([=](){
        if (Verb* verb = Verb::get(SP_VERB_FIT_CANVAS_TO_SELECTION_OR_DRAWING)) {
            if (SPAction* action = verb->get_action(Inkscape::ActionContext(getDesktop()))) {
                sp_action_perform(action, nullptr);
            }
        }
    });
}

void DocumentProperties::build_guides()
{
    _page_guides->show();

    Gtk::Label *label_gui = Gtk::manage (new Gtk::Label);
    label_gui->set_markup (_("<b>Guides</b>"));

    _rcp_gui.set_margin_start(0);
    _rcp_hgui.set_margin_start(0);
    _rcp_gui.set_hexpand();
    _rcp_hgui.set_hexpand();
    _rcb_sgui.set_hexpand();
    auto inner = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
    inner->add(_rcb_sgui);
    inner->add(_rcb_lgui);
    inner->add(_rcp_gui);
    inner->add(_rcp_hgui);
    auto spacer = Gtk::manage(new Gtk::Label());
    Gtk::Widget *const widget_array[] =
    {
        label_gui, nullptr,
        inner,     spacer,
        nullptr,   nullptr,
        nullptr,   &_create_guides_btn,
        nullptr,   &_delete_guides_btn
    };
    attach_all(_page_guides->table(), widget_array, G_N_ELEMENTS(widget_array));
    inner->set_hexpand(false);

    _create_guides_btn.signal_clicked().connect(sigc::mem_fun(*this, &DocumentProperties::create_guides_around_page));
    _delete_guides_btn.signal_clicked().connect(sigc::mem_fun(*this, &DocumentProperties::delete_all_guides));
}

void DocumentProperties::create_guides_around_page()
{
    Verb *verb = Verb::get( SP_VERB_EDIT_GUIDES_AROUND_PAGE );
    if (verb) {
        SPAction *action = verb->get_action(Inkscape::ActionContext(getDesktop()));
        if (action) {
            sp_action_perform(action, nullptr);
        }
    }
}

void DocumentProperties::delete_all_guides()
{
    Verb *verb = Verb::get( SP_VERB_EDIT_DELETE_ALL_GUIDES );
    if (verb) {
        SPAction *action = verb->get_action(Inkscape::ActionContext(getDesktop()));
        if (action) {
            sp_action_perform(action, nullptr);
        }
    }
}

/// Populates the available color profiles combo box
void DocumentProperties::populate_available_profiles(){
    _AvailableProfilesListStore->clear(); // Clear any existing items in the combo box

    // Iterate through the list of profiles and add the name to the combo box.
    bool home = true; // initial value doesn't matter, it's just to avoid a compiler warning
    bool first = true;
    for (auto &profile: ColorProfile::getProfileFilesWithNames()) {
        Gtk::TreeModel::Row row;

        // add a separator between profiles from the user's home directory and system profiles
        if (!first && profile.isInHome != home)
        {
          row = *(_AvailableProfilesListStore->append());
          row[_AvailableProfilesListColumns.fileColumn] = "<separator>";
          row[_AvailableProfilesListColumns.nameColumn] = "<separator>";
          row[_AvailableProfilesListColumns.separatorColumn] = true;
        }
        home = profile.isInHome;
        first = false;

        row = *(_AvailableProfilesListStore->append());
        row[_AvailableProfilesListColumns.fileColumn] = profile.filename;
        row[_AvailableProfilesListColumns.nameColumn] = profile.name;
        row[_AvailableProfilesListColumns.separatorColumn] = false;
    }
}

/**
 * Cleans up name to remove disallowed characters.
 * Some discussion at http://markmail.org/message/bhfvdfptt25kgtmj
 * Allowed ASCII first characters:  ':', 'A'-'Z', '_', 'a'-'z'
 * Allowed ASCII remaining chars add: '-', '.', '0'-'9',
 *
 * @param str the string to clean up.
 */
static void sanitizeName( Glib::ustring& str )
{
    if (str.size() > 0) {
        char val = str.at(0);
        if (((val < 'A') || (val > 'Z'))
            && ((val < 'a') || (val > 'z'))
            && (val != '_')
            && (val != ':')) {
          str.insert(0, "_");
        }
        for (Glib::ustring::size_type i = 1; i < str.size(); i++) {
            char val = str.at(i);
            if (((val < 'A') || (val > 'Z'))
                && ((val < 'a') || (val > 'z'))
                && ((val < '0') || (val > '9'))
                && (val != '_')
                && (val != ':')
                && (val != '-')
                && (val != '.')) {
                str.replace(i, 1, "-");
            }
        }
    }
}

/// Links the selected color profile in the combo box to the document
void DocumentProperties::linkSelectedProfile()
{
    //store this profile in the SVG document (create <color-profile> element in the XML)
    if (auto document = getDocument()){
        // Find the index of the currently-selected row in the color profiles combobox
        Gtk::TreeModel::iterator iter = _AvailableProfilesList.get_active();
        if (!iter)
            return;

        // Read the filename and description from the list of available profiles
        Glib::ustring file = (*iter)[_AvailableProfilesListColumns.fileColumn];
        Glib::ustring name = (*iter)[_AvailableProfilesListColumns.nameColumn];

        std::vector<SPObject *> current = document->getResourceList( "iccprofile" );
        for (auto obj : current) {
            Inkscape::ColorProfile* prof = reinterpret_cast<Inkscape::ColorProfile*>(obj);
            if (!strcmp(prof->href, file.c_str()))
                return;
        }
        Inkscape::XML::Document *xml_doc = document->getReprDoc();
        Inkscape::XML::Node *cprofRepr = xml_doc->createElement("svg:color-profile");
        gchar* tmp = g_strdup(name.c_str());
        Glib::ustring nameStr = tmp ? tmp : "profile"; // TODO add some auto-numbering to avoid collisions
        sanitizeName(nameStr);
        cprofRepr->setAttribute("name", nameStr);
        cprofRepr->setAttribute("xlink:href", Glib::filename_to_uri(Glib::filename_from_utf8(file)));
        cprofRepr->setAttribute("id", file);


        // Checks whether there is a defs element. Creates it when needed
        Inkscape::XML::Node *defsRepr = sp_repr_lookup_name(xml_doc, "svg:defs");
        if (!defsRepr) {
            defsRepr = xml_doc->createElement("svg:defs");
            xml_doc->root()->addChild(defsRepr, nullptr);
        }

        g_assert(document->getDefs());
        defsRepr->addChild(cprofRepr, nullptr);

        // TODO check if this next line was sometimes needed. It being there caused an assertion.
        //Inkscape::GC::release(defsRepr);

        // inform the document, so we can undo
        DocumentUndo::done(document, _("Link Color Profile"), "");

        populate_linked_profiles_box();
    }
}

struct _cmp {
  bool operator()(const SPObject * const & a, const SPObject * const & b)
  {
    const Inkscape::ColorProfile &a_prof = reinterpret_cast<const Inkscape::ColorProfile &>(*a);
    const Inkscape::ColorProfile &b_prof = reinterpret_cast<const Inkscape::ColorProfile &>(*b);
    gchar *a_name_casefold = g_utf8_casefold(a_prof.name, -1 );
    gchar *b_name_casefold = g_utf8_casefold(b_prof.name, -1 );
    int result = g_strcmp0(a_name_casefold, b_name_casefold);
    g_free(a_name_casefold);
    g_free(b_name_casefold);
    return result < 0;
  }
};

template <typename From, typename To>
struct static_caster { To * operator () (From * value) const { return static_cast<To *>(value); } };

void DocumentProperties::populate_linked_profiles_box()
{
    _LinkedProfilesListStore->clear();
    if (auto document = getDocument()) {
        std::vector<SPObject *> current = document->getResourceList( "iccprofile" );
        if (! current.empty()) {
            _emb_profiles_observer.set((*(current.begin()))->parent);
        }

        std::set<Inkscape::ColorProfile *> _current;
        std::transform(current.begin(),
                       current.end(),
                       std::inserter(_current, _current.begin()),
                       static_caster<SPObject, Inkscape::ColorProfile>());

        for (auto &profile: _current) {
            Gtk::TreeModel::Row row = *(_LinkedProfilesListStore->append());
            row[_LinkedProfilesListColumns.nameColumn] = profile->name;
    //        row[_LinkedProfilesListColumns.previewColumn] = "Color Preview";
        }
    }
}

void DocumentProperties::external_scripts_list_button_release(GdkEventButton* event)
{
    if((event->type == GDK_BUTTON_RELEASE) && (event->button == 3)) {
        _ExternalScriptsContextMenu.popup_at_pointer(reinterpret_cast<GdkEvent *>(event));
    }
}

void DocumentProperties::embedded_scripts_list_button_release(GdkEventButton* event)
{
    if((event->type == GDK_BUTTON_RELEASE) && (event->button == 3)) {
        _EmbeddedScriptsContextMenu.popup_at_pointer(reinterpret_cast<GdkEvent *>(event));
    }
}

void DocumentProperties::linked_profiles_list_button_release(GdkEventButton* event)
{
    if((event->type == GDK_BUTTON_RELEASE) && (event->button == 3)) {
        _EmbProfContextMenu.popup_at_pointer(reinterpret_cast<GdkEvent *>(event));
    }
}

void DocumentProperties::cms_create_popup_menu(Gtk::Widget& parent, sigc::slot<void> rem)
{
    Gtk::MenuItem* mi = Gtk::manage(new Gtk::MenuItem(_("_Remove"), true));
    _EmbProfContextMenu.append(*mi);
    mi->signal_activate().connect(rem);
    mi->show();
    _EmbProfContextMenu.accelerate(parent);
}


void DocumentProperties::external_create_popup_menu(Gtk::Widget& parent, sigc::slot<void> rem)
{
    Gtk::MenuItem* mi = Gtk::manage(new Gtk::MenuItem(_("_Remove"), true));
    _ExternalScriptsContextMenu.append(*mi);
    mi->signal_activate().connect(rem);
    mi->show();
    _ExternalScriptsContextMenu.accelerate(parent);
}

void DocumentProperties::embedded_create_popup_menu(Gtk::Widget& parent, sigc::slot<void> rem)
{
    Gtk::MenuItem* mi = Gtk::manage(new Gtk::MenuItem(_("_Remove"), true));
    _EmbeddedScriptsContextMenu.append(*mi);
    mi->signal_activate().connect(rem);
    mi->show();
    _EmbeddedScriptsContextMenu.accelerate(parent);
}

void DocumentProperties::onColorProfileSelectRow()
{
    Glib::RefPtr<Gtk::TreeSelection> sel = _LinkedProfilesList.get_selection();
    if (sel) {
        _unlink_btn.set_sensitive(sel->count_selected_rows () > 0);
    }
}


void DocumentProperties::removeSelectedProfile(){
    Glib::ustring name;
    if(_LinkedProfilesList.get_selection()) {
        Gtk::TreeModel::iterator i = _LinkedProfilesList.get_selection()->get_selected();

        if(i){
            name = (*i)[_LinkedProfilesListColumns.nameColumn];
        } else {
            return;
        }
    }
    if (auto document = getDocument()) {
        std::vector<SPObject *> current = document->getResourceList( "iccprofile" );
        for (auto obj : current) {
            Inkscape::ColorProfile* prof = reinterpret_cast<Inkscape::ColorProfile*>(obj);
            if (!name.compare(prof->name)){
                prof->deleteObject(true, false);
                DocumentUndo::done(document, _("Remove linked color profile"), "");
                break; // removing the color profile likely invalidates part of the traversed list, stop traversing here.
            }
        }
    }

    populate_linked_profiles_box();
    onColorProfileSelectRow();
}

bool DocumentProperties::_AvailableProfilesList_separator(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreeModel::iterator& iter)
{
    bool separator = (*iter)[_AvailableProfilesListColumns.separatorColumn];
    return separator;
}

void DocumentProperties::build_cms()
{
    _page_cms->show();
    Gtk::Label *label_link= Gtk::manage (new Gtk::Label("", Gtk::ALIGN_START));
    label_link->set_markup (_("<b>Linked Color Profiles:</b>"));
    Gtk::Label *label_avail = Gtk::manage (new Gtk::Label("", Gtk::ALIGN_START));
    label_avail->set_markup (_("<b>Available Color Profiles:</b>"));

    _unlink_btn.set_tooltip_text(_("Unlink Profile"));
    docprops_style_button(_unlink_btn, INKSCAPE_ICON("list-remove"));

    gint row = 0;

    label_link->set_hexpand();
    label_link->set_halign(Gtk::ALIGN_START);
    label_link->set_valign(Gtk::ALIGN_CENTER);
    _page_cms->table().attach(*label_link, 0, row, 3, 1);

    row++;

    _LinkedProfilesListScroller.set_hexpand();
    _LinkedProfilesListScroller.set_valign(Gtk::ALIGN_CENTER);
    _page_cms->table().attach(_LinkedProfilesListScroller, 0, row, 3, 1);

    row++;

    Gtk::Box* spacer = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    spacer->set_size_request(SPACE_SIZE_X, SPACE_SIZE_Y);

    spacer->set_hexpand();
    spacer->set_valign(Gtk::ALIGN_CENTER);
    _page_cms->table().attach(*spacer, 0, row, 3, 1);

    row++;

    label_avail->set_hexpand();
    label_avail->set_halign(Gtk::ALIGN_START);
    label_avail->set_valign(Gtk::ALIGN_CENTER);
    _page_cms->table().attach(*label_avail, 0, row, 3, 1);

    row++;

    _AvailableProfilesList.set_hexpand();
    _AvailableProfilesList.set_valign(Gtk::ALIGN_CENTER);
    _page_cms->table().attach(_AvailableProfilesList, 0, row, 1, 1);

    _unlink_btn.set_halign(Gtk::ALIGN_CENTER);
    _unlink_btn.set_valign(Gtk::ALIGN_CENTER);
    _page_cms->table().attach(_unlink_btn, 2, row, 1, 1);

    // Set up the Available Profiles combo box
    _AvailableProfilesListStore = Gtk::ListStore::create(_AvailableProfilesListColumns);
    _AvailableProfilesList.set_model(_AvailableProfilesListStore);
    _AvailableProfilesList.pack_start(_AvailableProfilesListColumns.nameColumn);
    _AvailableProfilesList.set_row_separator_func(sigc::mem_fun(*this, &DocumentProperties::_AvailableProfilesList_separator));
    _AvailableProfilesList.signal_changed().connect( sigc::mem_fun(*this, &DocumentProperties::linkSelectedProfile) );

    populate_available_profiles();

    //# Set up the Linked Profiles combo box
    _LinkedProfilesListStore = Gtk::ListStore::create(_LinkedProfilesListColumns);
    _LinkedProfilesList.set_model(_LinkedProfilesListStore);
    _LinkedProfilesList.append_column(_("Profile Name"), _LinkedProfilesListColumns.nameColumn);
//    _LinkedProfilesList.append_column(_("Color Preview"), _LinkedProfilesListColumns.previewColumn);
    _LinkedProfilesList.set_headers_visible(false);
// TODO restore?    _LinkedProfilesList.set_fixed_height_mode(true);

    populate_linked_profiles_box();

    _LinkedProfilesListScroller.add(_LinkedProfilesList);
    _LinkedProfilesListScroller.set_shadow_type(Gtk::SHADOW_IN);
    _LinkedProfilesListScroller.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
    _LinkedProfilesListScroller.set_size_request(-1, 90);

    _unlink_btn.signal_clicked().connect(sigc::mem_fun(*this, &DocumentProperties::removeSelectedProfile));

    _LinkedProfilesList.get_selection()->signal_changed().connect( sigc::mem_fun(*this, &DocumentProperties::onColorProfileSelectRow) );

    _LinkedProfilesList.signal_button_release_event().connect_notify(sigc::mem_fun(*this, &DocumentProperties::linked_profiles_list_button_release));
    cms_create_popup_menu(_LinkedProfilesList, sigc::mem_fun(*this, &DocumentProperties::removeSelectedProfile));

    if (auto document = getDocument()) {
        std::vector<SPObject *> current = document->getResourceList( "defs" );
        if (!current.empty()) {
            _emb_profiles_observer.set((*(current.begin()))->parent);
        }
        _emb_profiles_observer.signal_changed().connect(sigc::mem_fun(*this, &DocumentProperties::populate_linked_profiles_box));
        onColorProfileSelectRow();
    }
}

void DocumentProperties::build_scripting()
{
    _page_scripting->show();

    _page_scripting->table().attach(_scripting_notebook, 0, 0, 1, 1);

    _scripting_notebook.append_page(*_page_external_scripts, _("External scripts"));
    _scripting_notebook.append_page(*_page_embedded_scripts, _("Embedded scripts"));

    //# External scripts tab
    _page_external_scripts->show();
    Gtk::Label *label_external= Gtk::manage (new Gtk::Label("", Gtk::ALIGN_START));
    label_external->set_markup (_("<b>External script files:</b>"));

    _external_add_btn.set_tooltip_text(_("Add the current file name or browse for a file"));
    docprops_style_button(_external_add_btn, INKSCAPE_ICON("list-add"));

    _external_remove_btn.set_tooltip_text(_("Remove"));
    docprops_style_button(_external_remove_btn, INKSCAPE_ICON("list-remove"));

    gint row = 0;

    label_external->set_hexpand();
    label_external->set_halign(Gtk::ALIGN_START);
    label_external->set_valign(Gtk::ALIGN_CENTER);
    _page_external_scripts->table().attach(*label_external, 0, row, 3, 1);

    row++;

    _ExternalScriptsListScroller.set_hexpand();
    _ExternalScriptsListScroller.set_valign(Gtk::ALIGN_CENTER);
    _page_external_scripts->table().attach(_ExternalScriptsListScroller, 0, row, 3, 1);

    row++;

    Gtk::Box* spacer_external = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    spacer_external->set_size_request(SPACE_SIZE_X, SPACE_SIZE_Y);

    spacer_external->set_hexpand();
    spacer_external->set_valign(Gtk::ALIGN_CENTER);
    _page_external_scripts->table().attach(*spacer_external, 0, row, 3, 1);

    row++;

    _script_entry.set_hexpand();
    _script_entry.set_valign(Gtk::ALIGN_CENTER);
    _page_external_scripts->table().attach(_script_entry, 0, row, 1, 1);

    _external_add_btn.set_halign(Gtk::ALIGN_CENTER);
    _external_add_btn.set_valign(Gtk::ALIGN_CENTER);
    _external_add_btn.set_margin_start(2);
    _external_add_btn.set_margin_end(2);

    _page_external_scripts->table().attach(_external_add_btn, 1, row, 1, 1);

    _external_remove_btn.set_halign(Gtk::ALIGN_CENTER);
    _external_remove_btn.set_valign(Gtk::ALIGN_CENTER);
    _page_external_scripts->table().attach(_external_remove_btn, 2, row, 1, 1);

    //# Set up the External Scripts box
    _ExternalScriptsListStore = Gtk::ListStore::create(_ExternalScriptsListColumns);
    _ExternalScriptsList.set_model(_ExternalScriptsListStore);
    _ExternalScriptsList.append_column(_("Filename"), _ExternalScriptsListColumns.filenameColumn);
    _ExternalScriptsList.set_headers_visible(true);
// TODO restore?    _ExternalScriptsList.set_fixed_height_mode(true);


    //# Embedded scripts tab
    _page_embedded_scripts->show();
    Gtk::Label *label_embedded= Gtk::manage (new Gtk::Label("", Gtk::ALIGN_START));
    label_embedded->set_markup (_("<b>Embedded script files:</b>"));

    _embed_new_btn.set_tooltip_text(_("New"));
    docprops_style_button(_embed_new_btn, INKSCAPE_ICON("list-add"));

    _embed_remove_btn.set_tooltip_text(_("Remove"));
    docprops_style_button(_embed_remove_btn, INKSCAPE_ICON("list-remove"));

    _embed_button_box.set_layout (Gtk::BUTTONBOX_START);
    _embed_button_box.add(_embed_new_btn);
    _embed_button_box.add(_embed_remove_btn);

    row = 0;

    label_embedded->set_hexpand();
    label_embedded->set_halign(Gtk::ALIGN_START);
    label_embedded->set_valign(Gtk::ALIGN_CENTER);
    _page_embedded_scripts->table().attach(*label_embedded, 0, row, 3, 1);

    row++;

    _EmbeddedScriptsListScroller.set_hexpand();
    _EmbeddedScriptsListScroller.set_valign(Gtk::ALIGN_CENTER);
    _page_embedded_scripts->table().attach(_EmbeddedScriptsListScroller, 0, row, 3, 1);

    row++;

    _embed_button_box.set_hexpand();
    _embed_button_box.set_valign(Gtk::ALIGN_CENTER);
    _page_embedded_scripts->table().attach(_embed_button_box, 0, row, 1, 1);

    row++;

    Gtk::Box* spacer_embedded = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    spacer_embedded->set_size_request(SPACE_SIZE_X, SPACE_SIZE_Y);
    spacer_embedded->set_hexpand();
    spacer_embedded->set_valign(Gtk::ALIGN_CENTER);
    _page_embedded_scripts->table().attach(*spacer_embedded, 0, row, 3, 1);

    row++;

    //# Set up the Embedded Scripts box
    _EmbeddedScriptsListStore = Gtk::ListStore::create(_EmbeddedScriptsListColumns);
    _EmbeddedScriptsList.set_model(_EmbeddedScriptsListStore);
    _EmbeddedScriptsList.append_column(_("Script ID"), _EmbeddedScriptsListColumns.idColumn);
    _EmbeddedScriptsList.set_headers_visible(true);
// TODO restore?    _EmbeddedScriptsList.set_fixed_height_mode(true);

    //# Set up the Embedded Scripts content box
    Gtk::Label *label_embedded_content= Gtk::manage (new Gtk::Label("", Gtk::ALIGN_START));
    label_embedded_content->set_markup (_("<b>Content:</b>"));

    label_embedded_content->set_hexpand();
    label_embedded_content->set_halign(Gtk::ALIGN_START);
    label_embedded_content->set_valign(Gtk::ALIGN_CENTER);
    _page_embedded_scripts->table().attach(*label_embedded_content, 0, row, 3, 1);

    row++;

    _EmbeddedContentScroller.set_hexpand();
    _EmbeddedContentScroller.set_valign(Gtk::ALIGN_CENTER);
    _page_embedded_scripts->table().attach(_EmbeddedContentScroller, 0, row, 3, 1);

    _EmbeddedContentScroller.add(_EmbeddedContent);
    _EmbeddedContentScroller.set_shadow_type(Gtk::SHADOW_IN);
    _EmbeddedContentScroller.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    _EmbeddedContentScroller.set_size_request(-1, 140);

    _EmbeddedScriptsList.signal_cursor_changed().connect(sigc::mem_fun(*this, &DocumentProperties::changeEmbeddedScript));
    _EmbeddedScriptsList.get_selection()->signal_changed().connect( sigc::mem_fun(*this, &DocumentProperties::onEmbeddedScriptSelectRow) );

    _ExternalScriptsList.get_selection()->signal_changed().connect( sigc::mem_fun(*this, &DocumentProperties::onExternalScriptSelectRow) );

    _EmbeddedContent.get_buffer()->signal_changed().connect(sigc::mem_fun(*this, &DocumentProperties::editEmbeddedScript));

    populate_script_lists();

    _ExternalScriptsListScroller.add(_ExternalScriptsList);
    _ExternalScriptsListScroller.set_shadow_type(Gtk::SHADOW_IN);
    _ExternalScriptsListScroller.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
    _ExternalScriptsListScroller.set_size_request(-1, 90);

    _external_add_btn.signal_clicked().connect(sigc::mem_fun(*this, &DocumentProperties::addExternalScript));

    _EmbeddedScriptsListScroller.add(_EmbeddedScriptsList);
    _EmbeddedScriptsListScroller.set_shadow_type(Gtk::SHADOW_IN);
    _EmbeddedScriptsListScroller.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
    _EmbeddedScriptsListScroller.set_size_request(-1, 90);

    _embed_new_btn.signal_clicked().connect(sigc::mem_fun(*this, &DocumentProperties::addEmbeddedScript));

    _external_remove_btn.signal_clicked().connect(sigc::mem_fun(*this, &DocumentProperties::removeExternalScript));
    _embed_remove_btn.signal_clicked().connect(sigc::mem_fun(*this, &DocumentProperties::removeEmbeddedScript));

    _ExternalScriptsList.signal_button_release_event().connect_notify(sigc::mem_fun(*this, &DocumentProperties::external_scripts_list_button_release));
    external_create_popup_menu(_ExternalScriptsList, sigc::mem_fun(*this, &DocumentProperties::removeExternalScript));

    _EmbeddedScriptsList.signal_button_release_event().connect_notify(sigc::mem_fun(*this, &DocumentProperties::embedded_scripts_list_button_release));
    embedded_create_popup_menu(_EmbeddedScriptsList, sigc::mem_fun(*this, &DocumentProperties::removeEmbeddedScript));

//TODO: review this observers code:
    if (auto document = getDocument()) {
        std::vector<SPObject *> current = document->getResourceList( "script" );
        if (! current.empty()) {
            _scripts_observer.set((*(current.begin()))->parent);
        }
        _scripts_observer.signal_changed().connect(sigc::mem_fun(*this, &DocumentProperties::populate_script_lists));
        onEmbeddedScriptSelectRow();
        onExternalScriptSelectRow();
    }
}

void DocumentProperties::build_metadata()
{
    using Inkscape::UI::Widget::EntityEntry;

    _page_metadata1->show();

    Gtk::Label *label = Gtk::manage (new Gtk::Label);
    label->set_markup (_("<b>Dublin Core Entities</b>"));
    label->set_halign(Gtk::ALIGN_START);
    label->set_valign(Gtk::ALIGN_CENTER);
    _page_metadata1->table().attach (*label, 0,0,2,1);

     /* add generic metadata entry areas */
    struct rdf_work_entity_t * entity;
    int row = 1;
    for (entity = rdf_work_entities; entity && entity->name; entity++, row++) {
        if ( entity->editable == RDF_EDIT_GENERIC ) {
            EntityEntry *w = EntityEntry::create (entity, _wr);
            _rdflist.push_back (w);

            w->_label.set_halign(Gtk::ALIGN_START);
            w->_label.set_valign(Gtk::ALIGN_CENTER);
            _page_metadata1->table().attach(w->_label, 0, row, 1, 1);

            w->_packable->set_hexpand();
            w->_packable->set_valign(Gtk::ALIGN_CENTER);
            _page_metadata1->table().attach(*w->_packable, 1, row, 1, 1);
        }
    }

    Gtk::Button *button_save = Gtk::manage (new Gtk::Button(_("_Save as default"),true));
    button_save->set_tooltip_text(_("Save this metadata as the default metadata"));
    Gtk::Button *button_load = Gtk::manage (new Gtk::Button(_("Use _default"),true));
    button_load->set_tooltip_text(_("Use the previously saved default metadata here"));

    auto box_buttons = Gtk::manage (new Gtk::ButtonBox);

    box_buttons->set_layout(Gtk::BUTTONBOX_END);
    box_buttons->set_spacing(4);
    box_buttons->pack_start(*button_save, true, true, 6);
    box_buttons->pack_start(*button_load, true, true, 6);
    _page_metadata1->pack_end(*box_buttons, false, false, 0);

    button_save->signal_clicked().connect(sigc::mem_fun(*this, &DocumentProperties::save_default_metadata));
    button_load->signal_clicked().connect(sigc::mem_fun(*this, &DocumentProperties::load_default_metadata));

    _page_metadata2->show();

    row = 0;
    Gtk::Label *llabel = Gtk::manage (new Gtk::Label);
    llabel->set_markup (_("<b>License</b>"));
    llabel->set_halign(Gtk::ALIGN_START);
    llabel->set_valign(Gtk::ALIGN_CENTER);
    _page_metadata2->table().attach(*llabel, 0, row, 2, 1);

    /* add license selector pull-down and URI */
    ++row;
    _licensor.init (_wr);

    _licensor.set_hexpand();
    _licensor.set_valign(Gtk::ALIGN_CENTER);
    _page_metadata2->table().attach(_licensor, 0, row, 2, 1);
}

void DocumentProperties::addExternalScript(){

    auto document = getDocument();
    if (!document)
        return;

    if (_script_entry.get_text().empty() ) {
        // Click Add button with no filename, show a Browse dialog
        browseExternalScript();
    }

    if (!_script_entry.get_text().empty()) {
        Inkscape::XML::Document *xml_doc = document->getReprDoc();
        Inkscape::XML::Node *scriptRepr = xml_doc->createElement("svg:script");
        scriptRepr->setAttributeOrRemoveIfEmpty("xlink:href", _script_entry.get_text());
        _script_entry.set_text("");

        xml_doc->root()->addChild(scriptRepr, nullptr);

        // inform the document, so we can undo
        DocumentUndo::done(document, _("Add external script..."), "");

        populate_script_lists();
    }
}

static Inkscape::UI::Dialog::FileOpenDialog * selectPrefsFileInstance = nullptr;

void  DocumentProperties::browseExternalScript() {

    //# Get the current directory for finding files
    static Glib::ustring open_path;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();


    Glib::ustring attr = prefs->getString(_prefs_path);
    if (!attr.empty()) open_path = attr;

    //# Test if the open_path directory exists
    if (!Inkscape::IO::file_test(open_path.c_str(),
              (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))
        open_path = "";

    //# If no open path, default to our home directory
    if (open_path.empty()) {
        open_path = g_get_home_dir();
        open_path.append(G_DIR_SEPARATOR_S);
    }

    //# Create a dialog
    SPDesktop *desktop = getDesktop();
    if (desktop && !selectPrefsFileInstance) {
        selectPrefsFileInstance =
              Inkscape::UI::Dialog::FileOpenDialog::create(
                 *desktop->getToplevel(),
                 open_path,
                 Inkscape::UI::Dialog::CUSTOM_TYPE,
                 _("Select a script to load"));
        selectPrefsFileInstance->addFilterMenu("Javascript Files", "*.js");
    }

    //# Show the dialog
    bool const success = selectPrefsFileInstance->show();

    if (!success) {
        return;
    }

    //# User selected something.  Get name and type
    Glib::ustring fileName = selectPrefsFileInstance->getFilename();

    _script_entry.set_text(fileName);
}

void DocumentProperties::addEmbeddedScript(){
    if(auto document = getDocument()) {
        Inkscape::XML::Document *xml_doc = document->getReprDoc();
        Inkscape::XML::Node *scriptRepr = xml_doc->createElement("svg:script");

        xml_doc->root()->addChild(scriptRepr, nullptr);

        // inform the document, so we can undo
        DocumentUndo::done(document, _("Add embedded script..."), "");
        populate_script_lists();
    }
}

void DocumentProperties::removeExternalScript(){
    Glib::ustring name;
    if(_ExternalScriptsList.get_selection()) {
        Gtk::TreeModel::iterator i = _ExternalScriptsList.get_selection()->get_selected();

        if(i){
            name = (*i)[_ExternalScriptsListColumns.filenameColumn];
        } else {
            return;
        }
    }

    auto document = getDocument();
    if (!document)
        return;
    std::vector<SPObject *> current = document->getResourceList( "script" );
    for (auto obj : current) {
        if (obj) {
            SPScript* script = dynamic_cast<SPScript *>(obj);
            if (script && (name == script->xlinkhref)) {

                //XML Tree being used directly here while it shouldn't be.
                Inkscape::XML::Node *repr = obj->getRepr();
                if (repr){
                    sp_repr_unparent(repr);

                    // inform the document, so we can undo
                    DocumentUndo::done(document, _("Remove external script"), "");
                }
            }
        }
    }

    populate_script_lists();
}

void DocumentProperties::removeEmbeddedScript(){
    Glib::ustring id;
    if(_EmbeddedScriptsList.get_selection()) {
        Gtk::TreeModel::iterator i = _EmbeddedScriptsList.get_selection()->get_selected();

        if(i){
            id = (*i)[_EmbeddedScriptsListColumns.idColumn];
        } else {
            return;
        }
    }

    if (auto document = getDocument()) {
        if (auto obj = document->getObjectById(id)) {
            //XML Tree being used directly here while it shouldn't be.
            if (auto repr = obj->getRepr()){
                sp_repr_unparent(repr);

                // inform the document, so we can undo
                DocumentUndo::done(document, _("Remove embedded script"), "");
            }
        }
    }

    populate_script_lists();
}

void DocumentProperties::onExternalScriptSelectRow()
{
    Glib::RefPtr<Gtk::TreeSelection> sel = _ExternalScriptsList.get_selection();
    if (sel) {
        _external_remove_btn.set_sensitive(sel->count_selected_rows () > 0);
    }
}

void DocumentProperties::onEmbeddedScriptSelectRow()
{
    Glib::RefPtr<Gtk::TreeSelection> sel = _EmbeddedScriptsList.get_selection();
    if (sel) {
        _embed_remove_btn.set_sensitive(sel->count_selected_rows () > 0);
    }
}

void DocumentProperties::changeEmbeddedScript(){
    Glib::ustring id;
    if(_EmbeddedScriptsList.get_selection()) {
        Gtk::TreeModel::iterator i = _EmbeddedScriptsList.get_selection()->get_selected();

        if(i){
            id = (*i)[_EmbeddedScriptsListColumns.idColumn];
        } else {
            return;
        }
    }

    auto document = getDocument();
    if (!document)
        return;

    bool voidscript=true;
    std::vector<SPObject *> current = document->getResourceList( "script" );
    for (auto obj : current) {
        if (id == obj->getId()){
            int count = (int) obj->children.size();

            if (count>1)
                g_warning("TODO: Found a script element with multiple (%d) child nodes! We must implement support for that!", count);

            //XML Tree being used directly here while it shouldn't be.
            SPObject* child = obj->firstChild();
            //TODO: shouldn't we get all children instead of simply the first child?

            if (child && child->getRepr()){
                const gchar* content = child->getRepr()->content();
                if (content){
                    voidscript=false;
                    _EmbeddedContent.get_buffer()->set_text(content);
                }
            }
        }
    }

    if (voidscript)
        _EmbeddedContent.get_buffer()->set_text("");
}

void DocumentProperties::editEmbeddedScript(){
    Glib::ustring id;
    if(_EmbeddedScriptsList.get_selection()) {
        Gtk::TreeModel::iterator i = _EmbeddedScriptsList.get_selection()->get_selected();

        if(i){
            id = (*i)[_EmbeddedScriptsListColumns.idColumn];
        } else {
            return;
        }
    }

    auto document = getDocument();
    if (!document)
        return;

    for (auto obj : document->getResourceList("script")) {
        if (id == obj->getId()) {
            //XML Tree being used directly here while it shouldn't be.
            Inkscape::XML::Node *repr = obj->getRepr();
            if (repr){
                auto tmp = obj->children | boost::adaptors::transformed([](SPObject& o) { return &o; });
                std::vector<SPObject*> vec(tmp.begin(), tmp.end());
                for (auto &child: vec) {
                    child->deleteObject();
                }
                obj->appendChildRepr(document->getReprDoc()->createTextNode(_EmbeddedContent.get_buffer()->get_text().c_str()));

                //TODO repr->set_content(_EmbeddedContent.get_buffer()->get_text());

                // inform the document, so we can undo
                DocumentUndo::done(document, _("Edit embedded script"), "");
            }
        }
    }
}

void DocumentProperties::populate_script_lists(){
    _ExternalScriptsListStore->clear();
    _EmbeddedScriptsListStore->clear();
    auto document = getDocument();
    if (!document)
        return;

    std::vector<SPObject *> current = getDocument()->getResourceList( "script" );
    if (!current.empty()) {
        SPObject *obj = *(current.begin());
        g_assert(obj != nullptr);
        _scripts_observer.set(obj->parent);
    }
    for (auto obj : current) {
        SPScript* script = dynamic_cast<SPScript *>(obj);
        g_assert(script != nullptr);
        if (script->xlinkhref)
        {
            Gtk::TreeModel::Row row = *(_ExternalScriptsListStore->append());
            row[_ExternalScriptsListColumns.filenameColumn] = script->xlinkhref;
        }
        else // Embedded scripts
        {
            Gtk::TreeModel::Row row = *(_EmbeddedScriptsListStore->append());
            row[_EmbeddedScriptsListColumns.idColumn] = obj->getId();
        }
    }
}

/**
* Called for _updating_ the dialog. DO NOT call this a lot. It's expensive!
*/
void DocumentProperties::update_gridspage()
{
    SPNamedView *nv = getDesktop()->getNamedView();

    int prev_page_count = _grids_notebook.get_n_pages();
    int prev_page_pos = _grids_notebook.get_current_page();

    //remove all tabs
    while (_grids_notebook.get_n_pages() != 0) {
        _grids_notebook.remove_page(-1); // this also deletes the page.
    }

    //add tabs
    for(auto grid : nv->grids) {
        if (!grid->repr->attribute("id")) continue; // update_gridspage is called again when "id" is added
        Glib::ustring name(grid->repr->attribute("id"));
        const char *icon = nullptr;
        switch (grid->getGridType()) {
            case GRID_RECTANGULAR:
                icon = "grid-rectangular";
                break;
            case GRID_AXONOMETRIC:
                icon = "grid-axonometric";
                break;
            default:
                break;
        }
        _grids_notebook.append_page(*grid->newWidget(), _createPageTabLabel(name, icon));
    }
    _grids_notebook.show_all();

    int cur_page_count = _grids_notebook.get_n_pages();
    if (cur_page_count > 0) {
        _grids_button_remove.set_sensitive(true);

        // The following is not correct if grid added/removed via XML
        if (cur_page_count == prev_page_count + 1) {
            _grids_notebook.set_current_page(cur_page_count - 1);
        } else if (cur_page_count == prev_page_count) {
            _grids_notebook.set_current_page(prev_page_pos);
        } else if (cur_page_count == prev_page_count - 1) {
            _grids_notebook.set_current_page(prev_page_pos < 1 ? 0 : prev_page_pos - 1);
        }
    } else {
        _grids_button_remove.set_sensitive(false);
    }
}

/**
 * Build grid page of dialog.
 */
void DocumentProperties::build_gridspage()
{
    /// \todo FIXME: gray out snapping when grid is off.
    /// Dissenting view: you want snapping without grid.

    _grids_label_crea.set_markup(_("<b>Creation</b>"));
    _grids_label_def.set_markup(_("<b>Defined grids</b>"));
    _grids_hbox_crea.pack_start(_grids_combo_gridtype, true, true);
    _grids_hbox_crea.pack_start(_grids_button_new, true, true);

    for (gint t = 0; t <= GRID_MAXTYPENR; t++) {
        _grids_combo_gridtype.append( CanvasGrid::getName( (GridType) t ) );
    }
    _grids_combo_gridtype.set_active_text( CanvasGrid::getName(GRID_RECTANGULAR) );

    _grids_space.set_size_request (SPACE_SIZE_X, SPACE_SIZE_Y);

    _grids_vbox.set_name("NotebookPage");
    _grids_vbox.set_border_width(4);
    _grids_vbox.set_spacing(4);
    _grids_vbox.pack_start(_grids_label_crea, false, false);
    _grids_vbox.pack_start(_grids_hbox_crea, false, false);
    _grids_vbox.pack_start(_grids_space, false, false);
    _grids_vbox.pack_start(_grids_label_def, false, false);
    _grids_vbox.pack_start(_grids_notebook, false, false);
    _grids_vbox.pack_start(_grids_button_remove, false, false);
}


void DocumentProperties::update_viewbox(SPDesktop* desktop) {
    if (!desktop) return;

    auto* document = desktop->getDocument();
    if (!document) return;

    using UI::Widget::PageProperties;
    SPRoot* root = document->getRoot();
    if (root->viewBox_set) {
        auto& vb = root->viewBox;
        _page->set_dimension(PageProperties::Dimension::ViewboxPosition, vb.min()[Geom::X], vb.min()[Geom::Y]);
        _page->set_dimension(PageProperties::Dimension::ViewboxSize, vb.width(), vb.height());
    }

    Geom::Scale scale = document->getDocumentScale();
    _page->set_dimension(PageProperties::Dimension::Scale, scale[Geom::X], scale[Geom::Y]);
}

/**
 * Update dialog widgets from desktop. Also call updateWidget routines of the grids.
 */
void DocumentProperties::update_widgets()
{
    auto desktop = getDesktop();
    auto document = getDocument();
    if (_wr.isUpdating() || !document) return;

    auto nv = desktop->getNamedView();
    auto pm = nv->getPageManager();

    _wr.setUpdating(true);
    // set_sensitive(true);

    SPRoot *root = document->getRoot();

    double doc_w = root->width.value;
    Glib::ustring doc_w_unit = unit_table.getUnit(root->width.unit)->abbr;
    if (doc_w_unit == "") {
        doc_w_unit = "px";
    } else if (doc_w_unit == "%" && root->viewBox_set) {
        doc_w_unit = "px";
        doc_w = root->viewBox.width();
    }
    double doc_h = root->height.value;
    Glib::ustring doc_h_unit = unit_table.getUnit(root->height.unit)->abbr;
    if (doc_h_unit == "") {
        doc_h_unit = "px";
    } else if (doc_h_unit == "%" && root->viewBox_set) {
        doc_h_unit = "px";
        doc_h = root->viewBox.height();
    }

    using UI::Widget::PageProperties;
    _page->set_dimension(PageProperties::Dimension::PageSize, doc_w, doc_h);
    _page->set_unit(PageProperties::Units::Document, doc_w_unit);

    update_viewbox_ui(desktop);
    update_scale_ui(desktop);

    if (nv->display_units) {
        _page->set_unit(PageProperties::Units::Display, nv->display_units->abbr);
    }
    _page->set_check(PageProperties::Check::Checkerboard, nv->desk_checkerboard);
    _page->set_color(PageProperties::Color::Desk, nv->desk_color);
    _page->set_color(PageProperties::Color::Background, pm->background_color);
    _page->set_check(PageProperties::Check::Border, pm->border_show);
    _page->set_check(PageProperties::Check::BorderOnTop, pm->border_on_top);
    _page->set_color(PageProperties::Color::Border, pm->border_color);
    _page->set_check(PageProperties::Check::Shadow, pm->shadow_show);

    _page->set_check(PageProperties::Check::AntiAlias, root->style->shape_rendering.computed != SP_CSS_SHAPE_RENDERING_CRISPEDGES);

    //-----------------------------------------------------------guide page

    _rcb_sgui.setActive (nv->showguides);
    _rcb_lgui.setActive (nv->lockguides);
    _rcp_gui.setRgba32 (nv->guidecolor);
    _rcp_hgui.setRgba32 (nv->guidehicolor);

    //-----------------------------------------------------------grids page

    update_gridspage();

    //------------------------------------------------Color Management page

    populate_linked_profiles_box();
    populate_available_profiles();

    //-----------------------------------------------------------meta pages
    /* update the RDF entities */
    if (auto document = getDocument()) {
        for (auto & it : _rdflist)
            it->update(document);

        _licensor.update(document);
    }
    _wr.setUpdating (false);
}

// TODO: copied from fill-and-stroke.cpp factor out into new ui/widget file?
Gtk::Box&
DocumentProperties::_createPageTabLabel(const Glib::ustring& label, const char *label_image)
{
    Gtk::Box *_tab_label_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
    _tab_label_box->set_spacing(4);

    auto img = Gtk::manage(sp_get_icon_image(label_image, Gtk::ICON_SIZE_MENU));
    _tab_label_box->pack_start(*img);

    Gtk::Label *_tab_label = Gtk::manage(new Gtk::Label(label, true));
    _tab_label_box->pack_start(*_tab_label);
    _tab_label_box->show_all();

    return *_tab_label_box;
}

//--------------------------------------------------------------------

void DocumentProperties::on_response (int id)
{
    if (id == Gtk::RESPONSE_DELETE_EVENT || id == Gtk::RESPONSE_CLOSE)
    {
        _rcp_gui.closeWindow();
        _rcp_hgui.closeWindow();
    }

    if (id == Gtk::RESPONSE_CLOSE)
        hide();
}

void DocumentProperties::load_default_metadata()
{
    /* Get the data RDF entities data from preferences*/
    for (auto & it : _rdflist) {
        it->load_from_preferences ();
    }
}

void DocumentProperties::save_default_metadata()
{
    /* Save these RDF entities to preferences*/
    if (auto document = getDocument()) {
        for (auto & it : _rdflist) {
            it->save_to_preferences(document);
        }
    }
}

void DocumentProperties::watch_connection::connect(Inkscape::XML::Node* node, const Inkscape::XML::NodeEventVector& vector, void* data) {
    disconnect();
    if (!node) return;

    _node = node;
    _data = data;
    node->addListener(&vector, data);
}

void DocumentProperties::watch_connection::disconnect() {
    if (_node) {
        _node->removeListenerByData(_data);
        _node = nullptr;
        _data = nullptr;
    }
}

void DocumentProperties::documentReplaced()
{
    _root_connection.disconnect();
    _namedview_connection.disconnect();

    if (auto desktop = getDesktop()) {
        _wr.setDesktop(desktop);
        _namedview_connection.connect(desktop->getNamedView()->getRepr(), _repr_events, this);
        if (auto document = desktop->getDocument()) {
            _root_connection.connect(document->getRoot()->getRepr(), _repr_events, this);
        }
        populate_linked_profiles_box();
        update_widgets();
    }
}

void DocumentProperties::update()
{
    update_widgets();
}

static void on_child_added(Inkscape::XML::Node */*repr*/, Inkscape::XML::Node */*child*/, Inkscape::XML::Node */*ref*/, void *data)
{
    if (DocumentProperties *dialog = static_cast<DocumentProperties *>(data))
        dialog->update_gridspage();
}

static void on_child_removed(Inkscape::XML::Node */*repr*/, Inkscape::XML::Node */*child*/, Inkscape::XML::Node */*ref*/, void *data)
{
    if (DocumentProperties *dialog = static_cast<DocumentProperties *>(data))
        dialog->update_gridspage();
}



/**
 * Called when XML node attribute changed; updates dialog widgets.
 */
static void on_repr_attr_changed(Inkscape::XML::Node *, gchar const *, gchar const *, gchar const *, bool, gpointer data)
{
    if (DocumentProperties *dialog = static_cast<DocumentProperties *>(data))
        dialog->update_widgets();
}


/*########################################################################
# BUTTON CLICK HANDLERS    (callbacks)
########################################################################*/

void DocumentProperties::onNewGrid()
{
    if (auto desktop = getDesktop()) {
        Inkscape::XML::Node *repr = desktop->getNamedView()->getRepr();
        Glib::ustring typestring = _grids_combo_gridtype.get_active_text();
        CanvasGrid::writeNewGridToRepr(repr, getDocument(), CanvasGrid::getGridTypeFromName(typestring.c_str()));

        // toggle grid showing to ON:
        desktop->showGrids(true);
    }
}


void DocumentProperties::onRemoveGrid()
{
    gint pagenum = _grids_notebook.get_current_page();
    if (pagenum == -1) // no pages
      return;

    SPNamedView *nv = getDesktop()->getNamedView();
    Inkscape::CanvasGrid * found_grid = nullptr;
    if( pagenum < (gint)nv->grids.size())
        found_grid = nv->grids[pagenum];

    if (auto document = getDocument()) {
        if (found_grid) {
            // delete the grid that corresponds with the selected tab
            // when the grid is deleted from SVG, the SPNamedview handler automatically deletes the object, so found_grid becomes an invalid pointer!
            found_grid->repr->parent()->removeChild(found_grid->repr);
            DocumentUndo::done(document, _("Remove grid"), INKSCAPE_ICON("document-properties"));
        }
    }
}

/* This should not effect anything in the SVG tree (other than "inkscape:document-units").
   This should only effect values displayed in the GUI. */
void DocumentProperties::display_unit_change(const Inkscape::Util::Unit* doc_unit)
{
    SPDocument *document = getDocument();
    // Don't execute when change is being undone
    if (!document || !DocumentUndo::getUndoSensitive(document)) {
        return;
    }
    // Don't execute when initializing widgets
    if (_wr.isUpdating()) {
        return;
    }

    Inkscape::XML::Node *repr = getDesktop()->getNamedView()->getRepr();
    /*Inkscape::Util::Unit const *old_doc_unit = unit_table.getUnit("px");
    if(repr->attribute("inkscape:document-units")) {
        old_doc_unit = unit_table.getUnit(repr->attribute("inkscape:document-units"));
    }*/
    // Inkscape::Util::Unit const *doc_unit = _rum_deflt.getUnit();

    // Set document unit
    Inkscape::SVGOStringStream os;
    os << doc_unit->abbr;
    repr->setAttribute("inkscape:document-units", os.str());

    // Disable changing of SVG Units. The intent here is to change the units in the UI, not the units in SVG.
    // This code should be moved (and fixed) once we have an "SVG Units" setting that sets what units are used in SVG data.
#if 0
    // Set viewBox
    if (doc->getRoot()->viewBox_set) {
        gdouble scale = Inkscape::Util::Quantity::convert(1, old_doc_unit, doc_unit);
        doc->setViewBox(doc->getRoot()->viewBox*Geom::Scale(scale));
    } else {
        Inkscape::Util::Quantity width = doc->getWidth();
        Inkscape::Util::Quantity height = doc->getHeight();
        doc->setViewBox(Geom::Rect::from_xywh(0, 0, width.value(doc_unit), height.value(doc_unit)));
    }

    // TODO: Fix bug in nodes tool instead of switching away from it
    if (get_active_tool(get_desktop()) == "Node") {
        set_active_tool(get_desktop(), "Select");
    }

    // Scale and translate objects
    // set transform options to scale all things with the transform, so all things scale properly after the viewbox change.
    /// \todo this "low-level" code of changing viewbox/unit should be moved somewhere else

    // save prefs
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool transform_stroke      = prefs->getBool("/options/transform/stroke", true);
    bool transform_rectcorners = prefs->getBool("/options/transform/rectcorners", true);
    bool transform_pattern     = prefs->getBool("/options/transform/pattern", true);
    bool transform_gradient    = prefs->getBool("/options/transform/gradient", true);

    prefs->setBool("/options/transform/stroke", true);
    prefs->setBool("/options/transform/rectcorners", true);
    prefs->setBool("/options/transform/pattern", true);
    prefs->setBool("/options/transform/gradient", true);
    {
        ShapeEditor::blockSetItem(true);
        gdouble viewscale = 1.0;
        Geom::Rect vb = doc->getRoot()->viewBox;
        if ( !vb.hasZeroArea() ) {
            gdouble viewscale_w = doc->getWidth().value("px") / vb.width();
            gdouble viewscale_h = doc->getHeight().value("px")/ vb.height();
            viewscale = std::min(viewscale_h, viewscale_w);
        }
        gdouble scale = Inkscape::Util::Quantity::convert(1, old_doc_unit, doc_unit);
        doc->getRoot()->scaleChildItemsRec(Geom::Scale(scale), Geom::Point(-viewscale*doc->getRoot()->viewBox.min()[Geom::X] +
                                                                            (doc->getWidth().value("px") - viewscale*doc->getRoot()->viewBox.width())/2,
                                                                            viewscale*doc->getRoot()->viewBox.min()[Geom::Y] +
                                                                            (doc->getHeight().value("px") + viewscale*doc->getRoot()->viewBox.height())/2),
                                                                            false);
        ShapeEditor::blockSetItem(false);
    }
    prefs->setBool("/options/transform/stroke",      transform_stroke);
    prefs->setBool("/options/transform/rectcorners", transform_rectcorners);
    prefs->setBool("/options/transform/pattern",     transform_pattern);
    prefs->setBool("/options/transform/gradient",    transform_gradient);
#endif

    document->setModifiedSinceSave();

    DocumentUndo::done(document, _("Changed default display unit"), "");
}

} // namespace Dialog
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
