// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Combobox for selecting dash patterns - implementation.
 */
/* Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "marker-combo-box.h"

#include <glibmm/fileutils.h>
#include <glibmm/i18n.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/menubutton.h>

#include <boost/algorithm/string.hpp>

#include "desktop-style.h"
#include "path-prefix.h"

#include "helper/stock-items.h"
#include "ui/icon-loader.h"
#include "ui/svg-renderer.h"

#include "io/resource.h"
#include "io/sys.h"

#include "object/sp-defs.h"
#include "object/sp-marker.h"
#include "object/sp-root.h"
#include "style.h"

#include "ui/cache/svg_preview_cache.h"
#include "ui/dialog-events.h"
#include "ui/util.h"

#include "ui/widget/spinbutton.h"
#include "ui/widget/stroke-style.h"

static Inkscape::UI::Cache::SvgPreview svg_preview_cache;

// size of marker image in a list
static const int ITEM_WIDTH = 36;
static const int ITEM_HEIGHT = 32;

namespace Inkscape {
namespace UI {
namespace Widget {

// get widget from builder or throw
template<class W> W& get_widget(Glib::RefPtr<Gtk::Builder>& builder, const char* id) {
    W* widget;
    builder->get_widget(id, widget);
    if (!widget) {
        throw std::runtime_error("Missing widget in a glade resource file");
    }
    return *widget;
}

static Glib::RefPtr<Gtk::Builder> create_builder() {
    auto glade = Inkscape::IO::Resource::get_filename(Inkscape::IO::Resource::UIS, "marker-popup.glade");
    Glib::RefPtr<Gtk::Builder> builder;
    try {
        return Gtk::Builder::create_from_file(glade);
    }
    catch (Glib::Error& ex) {
        g_error("Cannot load glade file: %s", + ex.what().c_str());
        throw;
    }
}

MarkerComboBox::MarkerComboBox(gchar const *id, int l) :
            combo_id(id),
            loc(l),
            updating(false),
            markerCount(0),
    _builder(create_builder()),
    _marker_list(get_widget<Gtk::FlowBox>(_builder, "flowbox")),
    _preview(get_widget<Gtk::Image>(_builder, "preview")),
    _marker_name(get_widget<Gtk::Label>(_builder, "marker-id")),
    _link_scale(get_widget<Gtk::Button>(_builder, "link-scale")),
    _scale_x(get_widget<Gtk::SpinButton>(_builder, "scale-x")),
    _scale_y(get_widget<Gtk::SpinButton>(_builder, "scale-y")),
    _menu_btn(get_widget<Gtk::MenuButton>(_builder, "menu-btn"))
{
    _background_color = 0x808080ff;
    _foreground_color = 0x808080ff;

    add(_menu_btn);

    _link_scale.signal_clicked().connect([=](){
        _scale_linked = !_scale_linked;
        update_scale_link();
    });
    // auto& link = 
    _marker_store = Gio::ListStore<MarkerItem>::create(); //marker_columns);
    _marker_list.bind_list_store(_marker_store, [=](const Glib::RefPtr<MarkerItem>& item){
        // g_warning("create widg %p", item.get());
        auto image = Gtk::make_managed<Gtk::Image>(item->pix);
        image->show();
        auto box = new Gtk::FlowBoxChild();
        box->add(*image);
        if (item->separator) {
            image->set_sensitive(false);
            image->set_can_focus(false);
            image->set_size_request(-1, 10);
            box->set_sensitive(false);
            box->set_can_focus(false);
        }
        _widgets_to_markers[image] = item;
        auto alloc = image->get_allocation();
        // box->set_size_request(alloc.get_width(), alloc.get_height());
        box->set_size_request(item->width, item->height);
        // image->set_data("marker", item.get());
        return box;
        // return image;
    });
    auto& btn_box = get_widget<Gtk::Box>(_builder, "btn-box");
    // btn_box.pack_start(image_renderer, false, true);
    // btn_box.add_attribute(image_renderer, "pixbuf", marker_columns.pixbuf);

    // gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(gobj()), MarkerComboBox::separator_cb, nullptr, nullptr);

    _sandbox = ink_markers_preview_doc(combo_id);

    // init_combo();
    set_sensitive(true);
    // this->get_style_context()->add_class("combobright");

    _marker_list.signal_selected_children_changed().connect([=](){
        g_warning("%s sel chg", combo_id);
        auto item = get_active();
        if (!item && !_marker_list.get_selected_children().empty()) {
            _marker_list.unselect_all();
        }
        update_preview();
        // _signal_changed.emit();
    });
    _marker_list.signal_child_activated().connect([=](Gtk::FlowBoxChild* box){
        g_warning("%s activated %p", combo_id, box);
        if (box->get_sensitive()) _signal_changed.emit();
    });

    update_preview();
    update_scale_link();
    show();
}

MarkerComboBox::~MarkerComboBox() {
    delete combo_id;

    if (doc) {
        modified_connection.disconnect();
    }
}

void MarkerComboBox::update_scale_link() {
    _link_scale.remove();
    _link_scale.add(get_widget<Gtk::Image>(_builder, _scale_linked ? "image-linked" : "image-unlinked"));

    _scale_y.set_sensitive(!_scale_linked);
    if (_scale_linked) {
        _scale_y.set_text(_scale_x.get_text());
    }
}

void MarkerComboBox::update_preview() {
    auto item = get_active();
    // auto sel = _marker_list.get_selected_children();

    Cairo::RefPtr<Cairo::Surface> surface;
    Glib::ustring label;
    auto alloc = _preview.get_allocation();

    if (auto item = get_active()) {
        // auto image = sel.front()->get_child();
        // auto item = static_cast<MarkerItem*>(image ? image->get_data("marker") : nullptr);
        if (item->source && !item->id.empty()) {
            Inkscape::Drawing drawing;
            unsigned const visionkey = SPItem::display_key_new(1);
            drawing.setRoot(_sandbox->getRoot()->invoke_show(drawing, visionkey, SP_ITEM_SHOW_DISPLAY));
            // generate preview
            auto size = Geom::IntPoint(alloc.get_width() - 10, alloc.get_height() - 10);
            if (size.x() > 0 && size.y() > 0) {
                surface = create_marker_image(size, item->id.c_str(), item->source, drawing, visionkey, true, false, 2.60);
            }
            _sandbox->getRoot()->invoke_hide(visionkey);
            label = item->label;
        }
    }

    _preview.set(surface);
    std::ostringstream ost;
    ost << "<small>" << label << "</small>";
    _marker_name.set_markup(ost.str().c_str());
}

bool MarkerComboBox::MarkerItem::operator == (const MarkerItem& item) const {
    return
        id == item.id &&
        label == item.label &&
        separator == item.separator &&
        stock == item.stock &&
        history == item.history &&
        source == item.source &&
        width == item.width &&
        height == item.height;
}

void MarkerComboBox::set_active(Glib::RefPtr<MarkerItem> item) {
    bool selected = false;
    if (item) {
        _marker_list.foreach([=,&selected](Gtk::Widget& widget){
            if (auto box = dynamic_cast<Gtk::FlowBoxChild*>(&widget)) {
                if (auto marker = _widgets_to_markers[box->get_child()]) {
                    if (*marker.get() == *item.get()) {
g_warning("%s reselec: %p", combo_id, box);                        
                        _marker_list.select_child(*box);
                        // _marker_list.grab_focus();
                        // _marker_list.set_focus_child(*box);
                        // box->grab_focus();
                        _marker_list.set_focus_child(*box);
                        _marker_list.grab_focus();
                        box->grab_focus();
                        selected = true;
                    }
                }
            }
        });
    }

    if (!selected) {
        _marker_list.unselect_all();
    }
}

Glib::RefPtr<MarkerComboBox::MarkerItem> MarkerComboBox::get_active() {
    //
    auto sel = _marker_list.get_selected_children();
    if (sel.size() == 1) {
        auto item = _widgets_to_markers[sel.front()->get_child()];
g_warning("%s cur sel: %p  id %s", combo_id, sel.front(), item ? item->id.c_str() : "-");
        if (item && item->separator) {
            return Glib::RefPtr<MarkerItem>();
        }
        return item;
    }
    else {
        return Glib::RefPtr<MarkerItem>();
    }
}

void MarkerComboBox::setDocument(SPDocument *document)
{
    if (doc != document) {

        if (doc) {
            modified_connection.disconnect();
        }

        doc = document;

        if (doc) {
            modified_connection = doc->getDefs()->connectModified( sigc::hide(sigc::hide(sigc::bind(sigc::ptr_fun(&MarkerComboBox::handleDefsModified), this))) );
        }

        refreshHistory();
    }
}

void
MarkerComboBox::handleDefsModified(MarkerComboBox *self)
{
    self->refreshHistory();
}

void
MarkerComboBox::refreshHistory()
{
    if (updating)
        return;

    updating = true;

    std::vector<SPMarker *> ml = get_marker_list(doc);

    /*
     * Seems to be no way to get notified of changes just to markers,
     * so listen to changes in all defs and check if the number of markers has changed here
     * to avoid unnecessary refreshes when things like gradients change
    */
    // if (markerCount != ml.size()) {
        // auto iter = get_active();
        // const char *active = iter ? iter->get_value(marker_columns.marker) : nullptr;
        sp_marker_list_from_doc(doc, true);
        // set_selected(active);
        markerCount = ml.size();
    // }

    updating = false;
}

cairo_surface_t* create_separator(double alpha) {
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 30, 1);
        // cairo_surface_set_device_scale(_surface, _device_scale, _device_scale);
    cairo_t* ctx = cairo_create(surface);
    cairo_set_source_rgba(ctx, 0.5, 0.5, 0.5, alpha);
    cairo_paint(ctx);
    cairo_surface_flush(surface);
    return surface;
}

static Cairo::RefPtr<Cairo::Surface> g_image_none(new Cairo::Surface(create_separator(1)));

Glib::RefPtr<MarkerComboBox::MarkerItem> MarkerComboBox::add_separator(bool filler) {
    auto item = Glib::RefPtr<MarkerItem>(new MarkerItem);
    item->history = false;
    item->separator = true;
    item->id = "None";
    item->label = filler ? "filler" : "Separator";
    item->stock = false;
    if (!filler) {
        static Cairo::RefPtr<Cairo::Surface> separator(new Cairo::Surface(create_separator(0.7)));
        item->pix = separator;
    }
    item->height = 10;
    item->width = -1;
    return item;
}

/**
 * Init the combobox widget to display markers from markers.svg
 */
void
MarkerComboBox::init_combo()
{
    if (updating)
        return;
// g_warning("combo init");
    static SPDocument *markers_doc = nullptr;

    // add separator
    // for (int i = 0; i < 8; ++i) {
        // add_separator(0);
    // }
    // Gtk::TreeModel::Row row_sep = *(marker_store->append());
    // row_sep[marker_columns.label] = "Separator";
    // row_sep[marker_columns.marker] = g_strdup("None");
    // row_sep[marker_columns.stock] = false;
    // row_sep[marker_columns.history] = false;
    // row_sep[marker_columns.separator] = true;

    // find and load markers.svg
    if (markers_doc == nullptr) {
        using namespace Inkscape::IO::Resource;
        auto markers_source = get_path_string(SYSTEM, MARKERS, "markers.svg");
        if (Glib::file_test(markers_source, Glib::FILE_TEST_IS_REGULAR)) {
            markers_doc = SPDocument::createNewDoc(markers_source.c_str(), false);
        }
    }

    // load markers from markers.svg
    if (markers_doc) {
        sp_marker_list_from_doc(markers_doc, false);
    }

    refreshHistory();
}

/**
 * Sets the current marker in the marker combobox.
 */
void MarkerComboBox::set_current(SPObject *marker)
{
    updating = true;
    std::string id;

    if (marker != nullptr) {
        if (auto markname = marker->getRepr()->attribute("id")) {
            id = markname;
        }
    }
    // else {
    //     set_selected(nullptr);
    // }
g_warning("%s set cur: %p %s", combo_id, marker, id.c_str());

    Cairo::RefPtr<Cairo::Surface> preview;
    bool selected = false;
    if (!id.empty()) {
        _marker_list.foreach([=,&selected,&preview](Gtk::Widget& widget){
            if (auto box = dynamic_cast<Gtk::FlowBoxChild*>(&widget)) {
                if (auto marker = _widgets_to_markers[box->get_child()]) {
                    if (marker->id == id) {
                        _marker_list.select_child(*box);
                        preview = marker->pix;
                        selected = true;
                    }
                }
            }
        });
    }

    if (!selected) {
        _marker_list.unselect_all();
    }

    auto& img = get_widget<Gtk::Image>(_builder, "preview-img");
    if (!marker) {
        preview = g_image_none;
    }
    img.set(preview);

    updating = false;
}
/**
 * Return a uri string representing the current selected marker used for setting the marker style in the document
 */
std::string MarkerComboBox::get_active_marker_uri()
{
    //todo
    /* Get Marker */
    auto item = get_active();
    if (!item) {
        return std::string();
    }
    // const gchar *markid = nullptr; // get_active()->get_value(marker_columns.marker);
    // if (!markid)
    // {
    //     return nullptr;
    // }

    // gchar const *marker = "";
    std::string marker;

    if (item->id != "none") {
    // if (strcmp(markid, "none")) {
        bool stockid = item->stock; // get_active()->get_value(marker_columns.stock);

        std::string markurn = stockid ? "urn:inkscape:marker:" + item->id : item->id;
        SPObject* mark = get_stock_item(markurn.c_str(), stockid);
        // g_free(markurn);
        if (mark) {
            Inkscape::XML::Node* repr = mark->getRepr();
            auto id = repr->attribute("id");
            if (id) {
                std::ostringstream ost;
                ost << "url(#" << id << ")";
                marker = ost.str();
            }
        }
    } else {
        marker = item->id;
    }

    return marker;
}

void MarkerComboBox::set_selected(const gchar *name, gboolean retry/*=true*/) {
    // _marker_list.select_child();
#if 0 // todo
    if (!name) {
        set_active(0);
        return;
    }

    for(Gtk::TreeIter iter = marker_store->children().begin();
        iter != marker_store->children().end(); ++iter) {
            Gtk::TreeModel::Row row = (*iter);
            if (row[marker_columns.marker] &&
                    !strcmp(row[marker_columns.marker], name)) {
                set_active(iter);
                return;
            }
    }

    // Didn't find it in the list, try refreshing from the doc
    if (retry) {
        sp_marker_list_from_doc(doc, true);
        set_selected(name, false);
    }
#endif
}


/**
 * Pick up all markers from source, except those that are in
 * current_doc (if non-NULL), and add items to the combo.
 */
void MarkerComboBox::sp_marker_list_from_doc(SPDocument *source, gboolean history)
{
    std::vector<SPMarker *> ml = get_marker_list(source);
g_warning("add markers from doc, hist: %d", (int)history);
    remove_markers(history); // Seem to need to remove 2x
    // remove_markers(history);
    add_markers(ml, source, history);

    update_store();
}

void MarkerComboBox::update_store() {
    _marker_store->freeze_notify();

    auto selected = get_active();

    _marker_store->remove_all();
    _widgets_to_markers.clear();

    // recent and user-defined markers come first
    for (auto&& item : _history_items) {
        _marker_store->append(item);
    }

    // separator
    if (!_history_items.empty()) {
        // add empty boxes to fill up the row to 'max' elements and then
        // extra ones to create entire new empty row (a separator of sorts)
        auto max = _marker_list.get_max_children_per_line();
        auto fillup = max - _history_items.size() % max;

        for (int i = 0; i < fillup; ++i) {
            _marker_store->append(add_separator(true));
        }
        for (int i = 0; i < max; ++i) {
            _marker_store->append(add_separator(false));
        }
    }

    // stock markers
    for (auto&& item : _stock_items) {
        _marker_store->append(item);
    }

    _marker_store->thaw_notify();

    // reselect current
    set_active(selected);
}
/**
 *  Returns a list of markers in the defs of the given source document as a vector
 *  Returns NULL if there are no markers in the document.
 */
std::vector<SPMarker *> MarkerComboBox::get_marker_list (SPDocument *source)
{
    std::vector<SPMarker *> ml;
    if (source == nullptr)
        return ml;

    SPDefs *defs = source->getDefs();
    if (!defs) {
        return ml;
    }

    for (auto& child: defs->children)
    {
        if (SP_IS_MARKER(&child)) {
            ml.push_back(SP_MARKER(&child));
        }
    }
    return ml;
}

/**
 * Remove history or non-history markers from the combo
 */
void MarkerComboBox::remove_markers (gboolean history)
{
    if (history) {
        _history_items.clear();
    }
    else {
        _stock_items.clear();
    }
/*
    _marker_store->freeze_notify();

    std::deque<guint> remove;
    const auto N = _marker_store->get_n_items();
    for (guint i = 0; i < N; ++i) {
        auto item = _marker_store->get_item(i);
        if (item->history == history && !item->separator) {
            remove.push_front(i);
        }
    }
    for (auto index : remove) {
        _marker_store->remove(index);
    }

    _marker_store->thaw_notify();
*/
    //_marker_store->splice();
#if 0 //todo
    // Having the model set causes assertions when erasing rows, temporarily disconnect
    // _marker_list.unset_model();
    for(Gtk::TreeIter iter = marker_store->children().begin();
        iter != marker_store->children().end(); ++iter) {
            Gtk::TreeModel::Row row = (*iter);
            if (row[marker_columns.history] == history && row[marker_columns.separator] == false) {
                marker_store->erase(iter);
                iter = marker_store->children().begin();
            }
    }

    // _marker_list.bind_list_store .set_model(marker_store);
#endif
}

/**
 * Adds markers in marker_list to the combo
 */
void MarkerComboBox::add_markers (std::vector<SPMarker *> const& marker_list, SPDocument *source, gboolean history)
{
    // _marker_store->freeze_notify();
    // _marker_store->remove_all();

    // Do this here, outside of loop, to speed up preview generation:
    Inkscape::Drawing drawing;
    unsigned const visionkey = SPItem::display_key_new(1);
    drawing.setRoot(_sandbox->getRoot()->invoke_show(drawing, visionkey, SP_ITEM_SHOW_DISPLAY));
#if 0 // todo
    // Find the separator,
    Gtk::TreeIter sep_iter;
    for(Gtk::TreeIter iter = marker_store->children().begin();
        iter != marker_store->children().end(); ++iter) {
            Gtk::TreeModel::Row row = (*iter);
            if (row[marker_columns.separator]) {
                sep_iter = iter;
            }
    }
#endif
    if (history) {
        // add "None"
        // static Cairo::RefPtr<Cairo::Surface> none(new Cairo::Surface(create_separator(1)));
        auto item = Glib::RefPtr<MarkerItem>(new MarkerItem);
        item->pix = g_image_none;
        item->history = true;
        item->separator = false;
        item->id = "None";
        item->label = "None";
        item->stock = false;
        item->width = ITEM_WIDTH;
        item->height = ITEM_HEIGHT;
        _history_items.push_back(item);
        // _marker_store->append(item);
        // Gtk::TreeModel::Row row = *(marker_store->prepend());
        // row[marker_columns.label] = C_("Marker", "None");
        // row[marker_columns.stock] = false;
        // row[marker_columns.marker] = g_strdup("None");
        // row[marker_columns.pixbuf] = sp_get_icon_pixbuf("no-marker", Gtk::ICON_SIZE_SMALL_TOOLBAR);
        // row[marker_columns.history] = true;
        // row[marker_columns.separator] = false;
    }

    for (auto i:marker_list) {

        Inkscape::XML::Node *repr = i->getRepr();
        gchar const *markid = repr->attribute("inkscape:stockid") ? repr->attribute("inkscape:stockid") : repr->attribute("id");

        // generate preview
        auto pixbuf = create_marker_image(Geom::IntPoint(ITEM_WIDTH, ITEM_HEIGHT), repr->attribute("id"), source, drawing, visionkey, false, true, 1.50);

        // Add history before separator, others after
        // Gtk::TreeModel::Row row;
        // if (history)
            // row = *(marker_store->insert(sep_iter));
        // else
            // row = *(marker_store->append());
/*
        row[marker_columns.label] = ink_ellipsize_text(markid, 20);
        // Non "stock" markers can also have "inkscape:stockid" (when using extension ColorMarkers),
        // So use !is_history instead to determine is it is "stock" (ie in the markers.svg file)
        row[marker_columns.stock] = !history;
        row[marker_columns.marker] = repr->attribute("id");
        row[marker_columns.pixbuf] = pixbuf;
        row[marker_columns.history] = history;
        row[marker_columns.separator] = false;
*/
        auto item = Glib::RefPtr<MarkerItem>(new MarkerItem);
        item->source = source;
        item->pix = pixbuf;
        if (auto id = repr->attribute("id")) {
            item->id = id;
        }
        item->label = markid ? markid : "";
        item->stock = !history;
        item->history = history;
        item->width = ITEM_WIDTH + 4;
        item->height = ITEM_HEIGHT + 4;

        if (history) {
            _history_items.push_back(item);
        }
        else {
            _stock_items.push_back(item);
        }
    }
/*
    if (history) {
        auto N = _marker_store->get_n_items();
        auto in_history = 0;
        int last = 0;
        for (int i = 0; i < N; ++i) {
            auto item = _marker_store->get_item(i);
            if (item->history) {
                in_history++;
                last = i;
            }
        }
        auto fill = in_history % _marker_list.get_max_children_per_line();
        if (fill > 0) {
            auto fill_up = fill - _marker_list.get_max_children_per_line();
            for (int i = 0; i < fill_up; ++i) {
                add_separator(last + 1);
            }
        }
    }
*/
    _sandbox->getRoot()->invoke_hide(visionkey);

    // _marker_store->thaw_notify();
}

/*
 * Remove from the cache and recreate a marker image
 */
void
MarkerComboBox::update_marker_image(gchar const *mname)
{
    gchar *cache_name = g_strconcat(combo_id, mname, nullptr);
    Glib::ustring key = svg_preview_cache.cache_key(doc->getDocumentFilename(), cache_name, 24);
    g_free (cache_name);
    svg_preview_cache.remove_preview_from_cache(key);

    Inkscape::Drawing drawing;
    unsigned const visionkey = SPItem::display_key_new(1);
    drawing.setRoot(_sandbox->getRoot()->invoke_show(drawing, visionkey, SP_ITEM_SHOW_DISPLAY));
    auto pixbuf = create_marker_image(Geom::IntPoint(ITEM_WIDTH, ITEM_HEIGHT), mname, doc, drawing, visionkey, false, true, 1.00);
    _sandbox->getRoot()->invoke_hide(visionkey);

#if 0 // todo
    for(const auto & iter : marker_store->children()) {
            Gtk::TreeModel::Row row = iter;
            if (row[marker_columns.marker] && row[marker_columns.history] &&
                    !strcmp(row[marker_columns.marker], mname)) {
                row[marker_columns.pixbuf] = pixbuf;
                return;
            }
    }
#endif
}
/**
 * Creates a copy of the marker named mname, determines its visible and renderable
 * area in the bounding box, and then renders it. This allows us to fill in
 * preview images of each marker in the marker combobox.
 */
Cairo::RefPtr<Cairo::Surface>
MarkerComboBox::create_marker_image(Geom::IntPoint pixel_size, gchar const *mname,
    SPDocument *source, Inkscape::Drawing &drawing, unsigned /*visionkey*/, bool checkerboard, bool no_clip, double scale)
{
    // Retrieve the marker named 'mname' from the source SVG document
    SPObject const *marker = source->getObjectById(mname);
    if (marker == nullptr) {
        g_warning("bad mname: %s", mname);
        return Cairo::RefPtr<Cairo::Surface>(); // sp_get_icon_pixbuf("bad-marker", Gtk::ICON_SIZE_SMALL_TOOLBAR);
    }

    /* Get from cache right away */
#if 0 //todo
    gchar *cache_name = g_strconcat(combo_id, mname, nullptr);
    Glib::ustring key = svg_preview_cache.cache_key(source->getDocumentFilename(), cache_name, psize);
    g_free (cache_name);
    GdkPixbuf *pixbuf = svg_preview_cache.get_preview_from_cache(key); // no ref created
    if(pixbuf) {
        return Glib::wrap(pixbuf, true);
    }
#endif
    // Create a copy repr of the marker with id="sample"
    Inkscape::XML::Document *xml_doc = _sandbox->getReprDoc();
    Inkscape::XML::Node *mrepr = marker->getRepr()->duplicate(xml_doc);
    mrepr->setAttribute("id", "sample");

    // Replace the old sample in the sandbox by the new one
    Inkscape::XML::Node *defsrepr = _sandbox->getObjectById("defs")->getRepr();
    SPObject *oldmarker = _sandbox->getObjectById("sample");
    if (oldmarker) {
        oldmarker->deleteObject(false);
    }

    // TODO - This causes a SIGTRAP on windows
    defsrepr->appendChild(mrepr);

    Inkscape::GC::release(mrepr);

    // If the marker color is a url link to a pattern or gradient copy that too
    SPObject *mk = source->getObjectById(mname);
    SPCSSAttr *css_marker = sp_css_attr_from_object(mk->firstChild(), SP_STYLE_FLAG_ALWAYS);
    //const char *mfill = sp_repr_css_property(css_marker, "fill", "none");
    const char *mstroke = sp_repr_css_property(css_marker, "fill", "none");

    if (!strncmp (mstroke, "url(", 4)) {
        SPObject *linkObj = getMarkerObj(mstroke, source);
        if (linkObj) {
            Inkscape::XML::Node *grepr = linkObj->getRepr()->duplicate(xml_doc);
            SPObject *oldmarker = _sandbox->getObjectById(linkObj->getId());
            if (oldmarker) {
                oldmarker->deleteObject(false);
            }
            defsrepr->appendChild(grepr);
            Inkscape::GC::release(grepr);

            if (SP_IS_GRADIENT(linkObj)) {
                SPGradient *vector = sp_gradient_get_forked_vector_if_necessary (SP_GRADIENT(linkObj), false);
                if (vector) {
                    Inkscape::XML::Node *grepr = vector->getRepr()->duplicate(xml_doc);
                    SPObject *oldmarker = _sandbox->getObjectById(vector->getId());
                    if (oldmarker) {
                        oldmarker->deleteObject(false);
                    }
                    defsrepr->appendChild(grepr);
                    Inkscape::GC::release(grepr);
                }
            }
        }
    }

// Uncomment this to get the sandbox documents saved (useful for debugging)
    // FILE *fp = fopen (g_strconcat(combo_id, mname, ".svg", nullptr), "w");
    // sp_repr_save_stream(_sandbox->getReprDoc(), fp);
    // fclose (fp);

    // object to render; note that the id is the same as that of the combo we're building
    SPObject *object = _sandbox->getObjectById(combo_id);
    _sandbox->getRoot()->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    _sandbox->ensureUpToDate();

    if (object == nullptr || !SP_IS_ITEM(object)) {
        g_warning("no obj: %s", combo_id);
        return Cairo::RefPtr<Cairo::Surface>(); // sp_get_icon_pixbuf("bad-marker", Gtk::ICON_SIZE_SMALL_TOOLBAR);
        // return sp_get_icon_pixbuf("bad-marker", Gtk::ICON_SIZE_SMALL_TOOLBAR); // sandbox broken?
    }

    auto context = get_style_context();
    Gdk::RGBA fg = context->get_color(get_state_flags());
    auto fgcolor = rgba_to_css_color(fg);
    fg.set_red(1 - fg.get_red());
    fg.set_green(1 - fg.get_green());
    fg.set_blue(1 - fg.get_blue());
    auto bgcolor = rgba_to_css_color(fg);
    auto objects = _sandbox->getObjectsBySelector(".colors");
    for (auto el : objects) {
        if (SPCSSAttr* css = sp_repr_css_attr(el->getRepr(), "style")) {
            sp_repr_css_set_property(css, "fill", bgcolor.c_str());
            sp_repr_css_set_property(css, "stroke", fgcolor.c_str());
            el->changeCSS(css, "style");
            sp_repr_css_attr_unref(css);
        }
    }

    SPItem *item = SP_ITEM(object);
    // Find object's bbox in document
    Geom::OptRect dbox = item->documentVisualBounds();

    if (!dbox) {
        g_warning("no dbox");
        return Cairo::RefPtr<Cairo::Surface>(); // sp_get_icon_pixbuf("bad-marker", Gtk::ICON_SIZE_SMALL_TOOLBAR);
        // return sp_get_icon_pixbuf("bad-marker", Gtk::ICON_SIZE_SMALL_TOOLBAR);
    }

    /* Update to renderable state */
    const double device_scale = get_scale_factor();
    auto surface = render_surface(drawing, scale, *dbox, pixel_size, device_scale, checkerboard ? &_background_color : nullptr, no_clip);
    cairo_surface_set_device_scale(surface, device_scale, device_scale);
    // svg_preview_cache.set_preview_in_cache(key, pixbuf);
    return Cairo::RefPtr<Cairo::Surface>(new Cairo::Surface(surface, false));
}

// capture background color when styles change
void MarkerComboBox::on_style_updated() {
    auto background = _background_color;
    if (auto wnd = dynamic_cast<Gtk::Window*>(this->get_toplevel())) {
        auto color = wnd->get_style_context()->get_background_color();
        background =
            gint32(0xff * color.get_red()) << 24 |
            gint32(0xff * color.get_green()) << 16 |
            gint32(0xff * color.get_blue()) << 8 |
            0xff;
    }

    auto context = get_style_context();
    Gdk::RGBA color = context->get_color(get_state_flags());
    auto foreground =
        gint32(0xff * color.get_red()) << 24 |
        gint32(0xff * color.get_green()) << 16 |
        gint32(0xff * color.get_blue()) << 8 |
        0xff;
    if (foreground != _foreground_color || background != _background_color) {
        _foreground_color = foreground;
        _background_color = background;
        // theme changed?
        init_combo();
    }
}

gboolean MarkerComboBox::separator_cb (GtkTreeModel *model, GtkTreeIter *iter, gpointer /*data*/) {

    gboolean sep = FALSE;
    gtk_tree_model_get(model, iter, 4, &sep, -1);
    return sep;
}

/*
      <filter id="softGlow" height="1.2" width="1.2" x="0.0" y="0.0">
      <!-- <feMorphology operator="dilate" radius="1" in="SourceAlpha" result="thicken" id="feMorphology2" /> -->
      <!-- Use a gaussian blur to create the soft blurriness of the glow -->
      <feGaussianBlur in="SourceAlpha" stdDeviation="3" result="blurred" id="feGaussianBlur4" />
      <!-- Change the color -->
      <feFlood flood-color="rgb(255,255,255)" result="glowColor" id="feFlood6" flood-opacity="0.70" />
      <!-- Color in the glows -->
      <feComposite in="glowColor" in2="blurred" operator="in" result="softGlow_colored" id="feComposite8" />
      <!--	Layer the effects together -->
      <feMerge id="feMerge14">
        <feMergeNode in="softGlow_colored" id="feMergeNode10" />
        <feMergeNode in="SourceGraphic" id="feMergeNode12" />
      </feMerge>
      </filter>
*/

/**
 * Returns a new document containing default start, mid, and end markers.
 * Note 1: group IDs are matched against "combo_id" to render correct preview object.
 * Note 2: paths/lines are kept outside of groups, so they dont' inflate visible bounds
 * Note 3: invisible rects inside groups keep visual bounds from getting too small, so we can see relative marker sizes
 */
std::unique_ptr<SPDocument> MarkerComboBox::ink_markers_preview_doc(const gchar* group_id)
{
gchar const *buffer = R"A(
    <svg xmlns="http://www.w3.org/2000/svg"
         xmlns:xlink="http://www.w3.org/1999/xlink"
         id="MarkerSample">

    <defs id="defs">
    </defs>

    <path id="line-marker-start" class="line colors" style="stroke-width:2;stroke-opacity:0.2" d="M 12.5,13 l 1000,0" />
    <!-- <g id="marker-start" class="group" style="filter:url(#softGlow)"> -->
    <g id="marker-start" class="group" style="filter:url(#softGlow)">
      <path class="colors" style="stroke-width:1.7;stroke-opacity:0;marker-start:url(#sample)"
       d="M 12.5,13 L 25,13"/>
      <rect x="0" y="0" width="25" height="25" style="fill:none;stroke:none"/>
    </g>

    <path id="line-marker-mid" class="line colors" style="stroke-width:2;stroke-opacity:0.2" d="M -1000,13 L 1000,13" />
    <g id="marker-mid" class="group">
      <path class="colors" style="stroke-width:1.7;stroke-opacity:0;marker-mid:url(#sample)"
       d="M 0,13 L 12.5,13 L 25,13"/>
      <rect x="0" y="0" width="25" height="25" style="fill:none;stroke:none"/>
    </g>

    <path id="line-marker-end" class="line colors" style="stroke-width:2;stroke-opacity:0.2" d="M -1000,13 L 12.5,13" />
    <g id="marker-end" class="group">
      <path class="colors" style="stroke-width:1.7;stroke-opacity:0;marker-end:url(#sample)"
       d="M 0,13 L 12.5,13"/>
      <rect x="0" y="0" width="25" height="25" style="fill:none;stroke:none"/>
    </g>

  </svg>
)A";

    auto document = std::unique_ptr<SPDocument>(SPDocument::createNewDocFromMem(buffer, strlen(buffer), false));
    // only leave requested group, so nothing else gets rendered
    for (auto&& group : document->getObjectsByClass("group")) {
        if (strcmp(group->getId(), group_id) != 0) {
            group->deleteObject();
        }
    }
    std::string id = "line-";
    id += group_id;
    for (auto&& line : document->getObjectsByClass("line")) {
        if (strcmp(line->getId(), id.c_str()) != 0) {
            line->deleteObject();
        }
    }
    return document;
}

} // namespace Widget
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
