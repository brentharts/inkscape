// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Singleton class to access the preferences file - implementation.
 */
/* Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2008,2009 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>
#include <glibmm/fileutils.h>
#include <glibmm/convert.h>
#include <glibmm/i18n.h>
#include <glib/gstdio.h>
#include "preferences.h"
#include "preferences-skeleton.h"
#include "inkscape.h"
#include "xml/node-observer.h"
#include "xml/node-iterators.h"
#include "xml/attribute-record.h"
#include "util/units.h"
#include "attribute-rel-util.h"
#include "io/resource.h"

#define PREFERENCES_FILE_NAME "preferences.xml"

using Inkscape::Util::unit_table;

namespace Inkscape {

static Inkscape::XML::Document *loadImpl( std::string const& prefsFilename, Glib::ustring & errMsg );
static void migrateDetails( Inkscape::XML::Document *from, Inkscape::XML::Document *to );

static Inkscape::XML::Document *migrateFromDoc = nullptr;

// private inner class definition

/**
 * XML - prefs observer bridge.
 *
 * This is an XML node observer that watches for changes in the XML document storing the preferences.
 * It is used to implement preference observers.
 */
class Preferences::PrefNodeObserver : public XML::NodeObserver {
public:
    PrefNodeObserver(Observer &o, Glib::ustring filter)
        : _observer(o)
        , _filter(std::move(filter))
    {}
    ~PrefNodeObserver() override = default;
    void notifyAttributeChanged(XML::Node &node, GQuark name, const char *, const char *) override;
private:
    Observer &_observer;
    Glib::ustring const _filter;
};

Preferences::Preferences()
{
    char *path = Inkscape::IO::Resource::profile_path(PREFERENCES_FILE_NAME);
    _prefs_filename = path;
    g_free(path);

    _loadDefaults();
    _load();

    _initialized = true;
}

Preferences::~Preferences()
{
    // unref XML document
    Inkscape::GC::release(_prefs_doc);
}

/**
 * Load internal defaults.
 *
 * In the future this will try to load the system-wide file before falling
 * back to the internal defaults.
 */
void Preferences::_loadDefaults()
{
    _prefs_doc = sp_repr_read_mem(preferences_skeleton, PREFERENCES_SKELETON_SIZE, nullptr);
#ifdef _WIN32
    setBool("/options/desktopintegration/value", 1);
#endif
#if defined(GDK_WINDOWING_QUARTZ)
    // No maximise for Quartz, see lp:1302627
    setInt("/options/defaultwindowsize/value", -1);
#endif

}

/**
 * Load the user's customized preferences.
 *
 * Tries to load the user's preferences.xml file. If there is none, creates it.
 */
void Preferences::_load()
{
    Glib::ustring const not_saved = _("Inkscape will run with default settings, "
                                      "and new settings will not be saved. ");

    // NOTE: After we upgrade to Glib 2.16, use Glib::ustring::compose

    // 1. Does the file exist?
    if (!g_file_test(_prefs_filename.c_str(), G_FILE_TEST_EXISTS)) {
        char *_prefs_dir = Inkscape::IO::Resource::profile_path(nullptr);
        // No - we need to create one.
        // Does the profile directory exist?
        if (!g_file_test(_prefs_dir, G_FILE_TEST_EXISTS)) {
            // No - create the profile directory
            if (g_mkdir_with_parents(_prefs_dir, 0755)) {
                // the creation failed
                //_reportError(Glib::ustring::compose(_("Cannot create profile directory %1."),
                //    Glib::filename_to_utf8(_prefs_dir)), not_saved);
                gchar *msg = g_strdup_printf(_("Cannot create profile directory %s."), _prefs_dir);
                _reportError(msg, not_saved);
                g_free(msg);
                return;
            }
        } else if (!g_file_test(_prefs_dir, G_FILE_TEST_IS_DIR)) {
            // The profile dir is not actually a directory
            //_reportError(Glib::ustring::compose(_("%1 is not a valid directory."),
            //    Glib::filename_to_utf8(_prefs_dir)), not_saved);
            gchar *msg = g_strdup_printf(_("%s is not a valid directory."), _prefs_dir);
            _reportError(msg, not_saved);
            g_free(msg);
            return;
        }
        // create some subdirectories for user stuff
        char const *user_dirs[] = {"extensions", "fonts", "icons", "keys", "palettes", "templates", nullptr};
        for (int i=0; user_dirs[i]; ++i) {
            // XXX Why are we doing this here? shouldn't this be an IO load item?
            char *dir = Inkscape::IO::Resource::profile_path(user_dirs[i]);
            if (!g_file_test(dir, G_FILE_TEST_EXISTS))
                g_mkdir(dir, 0755);
            g_free(dir);
        }
        // The profile dir exists and is valid.
        if (!g_file_set_contents(_prefs_filename.c_str(), preferences_skeleton, PREFERENCES_SKELETON_SIZE, nullptr)) {
            // The write failed.
            //_reportError(Glib::ustring::compose(_("Failed to create the preferences file %1."),
            //    Glib::filename_to_utf8(_prefs_filename)), not_saved);
            gchar *msg = g_strdup_printf(_("Failed to create the preferences file %s."),
                Glib::filename_to_utf8(_prefs_filename).c_str());
            _reportError(msg, not_saved);
            g_free(msg);
            return;
        }

        if ( migrateFromDoc ) {
            migrateDetails( migrateFromDoc, _prefs_doc );
        }

        // The prefs file was just created.
        // We can return now and skip the rest of the load process.
        _writable = true;
        return;
    }

    // Yes, the pref file exists.
    Glib::ustring errMsg;
    Inkscape::XML::Document *prefs_read = loadImpl( _prefs_filename, errMsg );

    if ( prefs_read ) {
        // Merge the loaded prefs with defaults.
        _prefs_doc->root()->mergeFrom(prefs_read->root(), "id");
        Inkscape::GC::release(prefs_read);
        _writable = true;
    } else {
        _reportError(errMsg, not_saved);
    }
}

//_reportError(msg, not_saved);
static Inkscape::XML::Document *loadImpl( std::string const& prefsFilename, Glib::ustring & errMsg )
{
    // 2. Is it a regular file?
    if (!g_file_test(prefsFilename.c_str(), G_FILE_TEST_IS_REGULAR)) {
        gchar *msg = g_strdup_printf(_("The preferences file %s is not a regular file."),
            Glib::filename_to_utf8(prefsFilename).c_str());
        errMsg = msg;
        g_free(msg);
        return nullptr;
    }

    // 3. Is the file readable?
    gchar *prefs_xml = nullptr; gsize len = 0;
    if (!g_file_get_contents(prefsFilename.c_str(), &prefs_xml, &len, nullptr)) {
        gchar *msg = g_strdup_printf(_("The preferences file %s could not be read."),
            Glib::filename_to_utf8(prefsFilename).c_str());
        errMsg = msg;
        g_free(msg);
        return nullptr;
    }

    // 4. Is it valid XML?
    Inkscape::XML::Document *prefs_read = sp_repr_read_mem(prefs_xml, len, nullptr);
    g_free(prefs_xml);
    if (!prefs_read) {
        gchar *msg = g_strdup_printf(_("The preferences file %s is not a valid XML document."),
            Glib::filename_to_utf8(prefsFilename).c_str());
        errMsg = msg;
        g_free(msg);
        return nullptr;
    }

    // 5. Basic sanity check: does the root element have a correct name?
    if (strcmp(prefs_read->root()->name(), "inkscape")) {
        gchar *msg = g_strdup_printf(_("The file %s is not a valid Inkscape preferences file."),
            Glib::filename_to_utf8(prefsFilename).c_str());
        errMsg = msg;
        g_free(msg);
        Inkscape::GC::release(prefs_read);
        return nullptr;
    }

    return prefs_read;
}

static void migrateDetails( Inkscape::XML::Document *from, Inkscape::XML::Document *to )
{
    // TODO pull in additional prefs with more granularity
    to->root()->mergeFrom(from->root(), "id");
}

/**
 * Flush all pref changes to the XML file.
 */
void Preferences::save()
{
    // no-op if the prefs file is not writable
    if (_writable) {
        // sp_repr_save_file uses utf-8 instead of the glib filename encoding.
        // I don't know why filenames are kept in utf-8 in Inkscape and then
        // converted to filename encoding when necessary through special functions
        // - wouldn't it be easier to keep things in the encoding they are supposed
        // to be in?

        // No, it would not. There are many reasons, one key reason being that the
        // rest of GTK+ is explicitly UTF-8. From an engineering standpoint, keeping
        // the filesystem encoding would change things from a one-to-many problem to
        // instead be a many-to-many problem. Also filesystem encoding can change
        // from one run of the program to the next, so can not be stored.
        // There are many other factors, so ask if you would like to learn them. - JAC
        Glib::ustring utf8name = Glib::filename_to_utf8(_prefs_filename);
        if (!utf8name.empty()) {
            sp_repr_save_file(_prefs_doc, utf8name.c_str());
        }
    }
}

/**
 * Deletes the preferences.xml file
 */
void Preferences::reset()
{
    time_t sptime = time (nullptr);
    struct tm *sptm = localtime (&sptime);
    gchar sptstr[256];
    strftime(sptstr, 256, "%Y_%m_%d_%H_%M_%S", sptm);

    char *new_name = g_strdup_printf("%s_%s.xml", _prefs_filename.c_str(), sptstr);


    if (g_file_test(_prefs_filename.c_str(), G_FILE_TEST_EXISTS)) {
        //int retcode = g_unlink (_prefs_filename.c_str());
        int retcode = g_rename (_prefs_filename.c_str(), new_name );
        if (retcode == 0) g_warning("%s %s.", _("Preferences file was backed up to"), new_name);
        else g_warning("%s", _("There was an error trying to reset the preferences file."));
    }

    g_free(new_name);
    _observer_map.clear();
    Inkscape::GC::release(_prefs_doc);
    _prefs_doc = nullptr;
    _loadDefaults();
    _load();
    save();
}

bool Preferences::getLastError( Glib::ustring& primary, Glib::ustring& secondary )
{
    bool result = _hasError;
    if ( _hasError ) {
        primary = _lastErrPrimary;
        secondary = _lastErrSecondary;
        _hasError = false;
        _lastErrPrimary.clear();
        _lastErrSecondary.clear();
    } else {
        primary.clear();
        secondary.clear();
    }
    return result;
}

// Now for the meat.

/**
 * Get names of all entries in the specified path.
 *
 * @param path Preference path to query.
 * @return A vector containing all entries in the given directory.
 */
std::vector<Preferences::Entry> Preferences::getAllEntries(Glib::ustring const &path)
{
    std::vector<Entry> temp;
    if (auto node = _getNode(path, false)) {
        for (auto const &iter : node->attributeList()) {
            temp.push_back(Entry(path + '/' + g_quark_to_string(iter.key), Inkscape::Util::to_cstr(iter.value)));
        }
    }
    return temp;
}

/**
 * Get the paths to all subdirectories of the specified path.
 *
 * @param path Preference path to query.
 * @return A vector containing absolute paths to all subdirectories in the given path.
 */
std::vector<Glib::ustring> Preferences::getAllDirs(Glib::ustring const &path)
{
    std::vector<Glib::ustring> temp;
    if (auto node = _getNode(path, false)) {
        for (Inkscape::XML::NodeSiblingIterator i = node->firstChild(); i; ++i) {
            if (i->attribute("id") == nullptr) {
                continue;
            }
            temp.push_back(path + '/' + i->attribute("id"));
        }
    }
    return temp;
}

// getter methods

Preferences::Entry const Preferences::getEntry(Glib::ustring const &pref_path)
{
    char const *val = _getRawValue(pref_path);
    return Entry(pref_path, val);
}

// setter methods

/**
 * Set a boolean attribute of a preference.
 *
 * @param pref_path Path of the preference to modify.
 * @param value The new value of the pref attribute.
 */
void Preferences::setBool(Glib::ustring const &pref_path, bool value)
{
    /// @todo Boolean values should be stored as "true" and "false",
    /// but this is not possible due to an interaction with event contexts.
    /// Investigate this in depth.
    _setRawValue(pref_path, ( value ? "1" : "0" ));
}

/**
 * Set an point attribute of a preference.
 *
 * @param pref_path Path of the preference to modify.
 * @param value The new value of the pref attribute.
 */
void Preferences::setPoint(Glib::ustring const &pref_path, Geom::Point value)
{
    setDouble(pref_path + "/x", value[Geom::X]);
    setDouble(pref_path + "/y", value[Geom::Y]);
}

/**
 * Set an integer attribute of a preference.
 *
 * @param pref_path Path of the preference to modify.
 * @param value The new value of the pref attribute.
 */
void Preferences::setInt(Glib::ustring const &pref_path, int value)
{
    _setRawValue(pref_path, Glib::ustring::compose("%1",value));
}

/**
 * Set an unsigned integer attribute of a preference.
 *
 * @param pref_path Path of the preference to modify.
 * @param value The new value of the pref attribute.
 */
void Preferences::setUInt(Glib::ustring const &pref_path, unsigned int value)
{
    _setRawValue(pref_path, Glib::ustring::compose("%1",value));
}

/**
 * Set a floating point attribute of a preference.
 *
 * @param pref_path Path of the preference to modify.
 * @param value The new value of the pref attribute.
 */
void Preferences::setDouble(Glib::ustring const &pref_path, double value)
{
    static constexpr auto digits10 = std::numeric_limits<double>::digits10; // number of decimal digits that are ensured to be precise
    _setRawValue(pref_path, Glib::ustring::format(std::setprecision(digits10), value));
}

/**
 * Set a floating point attribute of a preference.
 *
 * @param pref_path Path of the preference to modify.
 * @param value The new value of the pref attribute.
 * @param unit_abbr The string of the unit (abbreviated).
 */
void Preferences::setDoubleUnit(Glib::ustring const &pref_path, double value, Glib::ustring const &unit_abbr)
{
    static constexpr auto digits10 = std::numeric_limits<double>::digits10; // number of decimal digits that are ensured to be precise
    Glib::ustring str = Glib::ustring::compose("%1%2", Glib::ustring::format(std::setprecision(digits10), value), unit_abbr);
    _setRawValue(pref_path, str);
}

void Preferences::setColor(Glib::ustring const &pref_path, guint32 value)
{
    gchar buf[16];
    g_snprintf(buf, 16, "#%08x", value);
    _setRawValue(pref_path, buf);
}

/**
 * Set a string attribute of a preference.
 *
 * @param pref_path Path of the preference to modify.
 * @param value The new value of the pref attribute.
 */
void Preferences::setString(Glib::ustring const &pref_path, Glib::ustring const &value)
{
    _setRawValue(pref_path, value);
}

void Preferences::setStyle(Glib::ustring const &pref_path, SPCSSAttr *style)
{
    Glib::ustring css_str;
    sp_repr_css_write_string(style, css_str);
    _setRawValue(pref_path, css_str);
}

void Preferences::mergeStyle(Glib::ustring const &pref_path, SPCSSAttr *style)
{
    SPCSSAttr *current = getStyle(pref_path);
    sp_repr_css_merge(current, style);
    sp_attribute_purge_default_style(current, SP_ATTRCLEAN_DEFAULT_REMOVE);
    Glib::ustring css_str;
    sp_repr_css_write_string(current, css_str);
    _setRawValue(pref_path, css_str);
    sp_repr_css_attr_unref(current);
}

/**
 *  Remove an entry
 *  Make sure observers have been removed before calling
 */
void Preferences::remove(Glib::ustring const &pref_path)
{
    cachedRawValue.erase(pref_path.raw());

    Inkscape::XML::Node *node = _getNode(pref_path, false);
    if (node && node->parent()) {
        node->parent()->removeChild(node);
    } else { //Handle to remove also attributes in path not only the container node
        // verify path
        g_assert( pref_path.at(0) == '/' );
        if (_prefs_doc == nullptr){
            return;
        }
        node = _prefs_doc->root();
        Inkscape::XML::Node *child = nullptr;
        gchar **splits = g_strsplit(pref_path.c_str(), "/", 0);
        if ( splits ) {
            for (int part_i = 0; splits[part_i]; ++part_i) {
                // skip empty path segments
                if (!splits[part_i][0]) {
                    continue;
                }
                if (!node->firstChild()) {
                    node->removeAttribute(splits[part_i]);
                    g_strfreev(splits);
                    return;
                }
                for (child = node->firstChild(); child; child = child->next()) {
                    if (!strcmp(splits[part_i], child->attribute("id"))) {
                        break;
                    }
                }
                node = child;
            }
        }
        g_strfreev(splits);
    }
}

/**
 * Class that holds additional information for registered Observers.
 */
class Preferences::_ObserverData
{
public:
    _ObserverData(Inkscape::XML::Node *node, bool isAttr) : _node(node), _is_attr(isAttr) {}

    Inkscape::XML::Node *_node; ///< Node at which the wrapping PrefNodeObserver is registered
    bool _is_attr; ///< Whether this Observer watches a single attribute
};

Preferences::Observer::Observer(Glib::ustring path) :
    observed_path(std::move(path))
{
}

Preferences::Observer::~Observer()
{
    // on destruction remove observer to prevent invalid references
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->removeObserver(*this);
}

void Preferences::PrefNodeObserver::notifyAttributeChanged(XML::Node &node, GQuark name, const char *, char const *new_value)
{
    // filter out attributes we don't watch
    gchar const *attr_name = g_quark_to_string(name);
    if ( _filter.empty() || (_filter == attr_name) ) {
        _ObserverData *d = Preferences::_get_pref_observer_data(_observer);
        Glib::ustring notify_path = _observer.observed_path;

        if (!d->_is_attr) {
            std::vector<gchar const *> path_fragments;
            notify_path.reserve(256); // this will make appending operations faster

            // walk the XML tree, saving each of the id attributes in a vector
            // we terminate when we hit the observer's attachment node, because the path to this node
            // is already stored in notify_path
            for (XML::NodeParentIterator n = &node; static_cast<XML::Node*>(n) != d->_node; ++n) {
                path_fragments.push_back(n->attribute("id"));
            }
            // assemble the elements into a path
            for (std::vector<gchar const *>::reverse_iterator i = path_fragments.rbegin(); i != path_fragments.rend(); ++i) {
                notify_path.push_back('/');
                notify_path.append(*i);
            }

            // append attribute name
            notify_path.push_back('/');
            notify_path.append(attr_name);
        }

        _observer.notify(Entry(notify_path, new_value));
    }
}

/**
 * Find the XML node to observe.
 */
XML::Node *Preferences::_findObserverNode(Glib::ustring const &pref_path, Glib::ustring &node_key, Glib::ustring &attr_key, bool create)
{
    // first assume that the last path element is an entry.
    _keySplit(pref_path, node_key, attr_key);

    // find the node corresponding to the "directory".
    Inkscape::XML::Node *node = _getNode(node_key, create), *child;
    if (!node) return nullptr;

    for (child = node->firstChild(); child; child = child->next()) {
        // If there is a node with id corresponding to the attr key,
        // this means that the last part of the path is actually a key (folder).
        // Change values accordingly.
        if (attr_key == child->attribute("id")) {
            node = child;
            attr_key = "";
            node_key = pref_path;
            break;
        }
    }
    return node;
}

void Preferences::addObserver(Observer &o)
{
    // prevent adding the same observer twice
    if ( _observer_map.find(&o) == _observer_map.end() )
        return;

    Glib::ustring node_key, attr_key;
    if (auto node = _findObserverNode(o.observed_path, node_key, attr_key, true)) {
        // set additional data
        o._data.reset(new _ObserverData(node, !attr_key.empty()));

        _observer_map[&o].reset(new PrefNodeObserver(o, attr_key));

        // if we watch a single pref, we want to receive notifications only for a single node
        if (o._data->_is_attr) {
            node->addObserver( *(_observer_map[&o]) );
        } else {
            node->addSubtreeObserver( *(_observer_map[&o]) );
        }
    }
}

void Preferences::removeObserver(Observer &o)
{
    // prevent removing an observer which was not added
    auto it = _observer_map.find(&o);
    if (it != _observer_map.end()) {
        Inkscape::XML::Node *node = o._data->_node;
        _ObserverData *priv_data = o._data.get();

        if (priv_data->_is_attr) {
            node->removeObserver(*it->second);
        } else {
            node->removeSubtreeObserver(*it->second);
        }

        _observer_map.erase(it);
    }
}


/**
 * Get the XML node corresponding to the given pref key.
 *
 * @param pref_key Preference key (path) to get.
 * @param create Whether to create the corresponding node if it doesn't exist.
 * @param separator The character used to separate parts of the pref key.
 * @return XML node corresponding to the specified key.
 *
 * Derived from former inkscape_get_repr(). Private because it assumes that the backend is
 * a flat XML file, which may not be the case e.g. if we are using GConf (in future).
 */
Inkscape::XML::Node *Preferences::_getNode(Glib::ustring const &pref_key, bool create)
{
    // verify path
    g_assert( pref_key.empty() || pref_key.at(0) == '/' ); // empty corresponds to root node
    // No longer necessary, can cause problems with input devices which have a dot in the name
    // g_assert( pref_key.find('.') == Glib::ustring::npos );

    if (_prefs_doc == nullptr){
        return nullptr;
    }
    Inkscape::XML::Node *node = _prefs_doc->root();
    Inkscape::XML::Node *child = nullptr;
    gchar **splits = g_strsplit(pref_key.c_str(), "/", 0);

    if ( splits ) {
        for (int part_i = 0; splits[part_i]; ++part_i) {
            // skip empty path segments
            if (!splits[part_i][0]) {
                continue;
            }

            for (child = node->firstChild(); child; child = child->next()) {
                if (child->attribute("id") == nullptr) {
                    continue;
                }
                if (!strcmp(splits[part_i], child->attribute("id"))) {
                    break;
                }
            }

            // If the previous loop found a matching key, child now contains the node
            // matching the processed key part. If no node was found then it is NULL.
            if (!child) {
                if (create) {
                    // create the rest of the key
                    while(splits[part_i]) {
                        child = node->document()->createElement("group");
                        child->setAttribute("id", splits[part_i]);
                        node->appendChild(child);

                        ++part_i;
                        node = child;
                    }
                    g_strfreev(splits);
                    splits = nullptr;
                    return node;
                } else {
                    g_strfreev(splits);
                    splits = nullptr;
                    return nullptr;
                }
            }

            node = child;
        }
        g_strfreev(splits);
    }
    return node;
}

char const *Preferences::_getRawValue(Glib::ustring const &path)
{

    if (_initialized){
        // check in cache first
        auto it = cachedRawValue.find(path.raw());
        if (it != cachedRawValue.end()) {
            if (it->second) {
                return it->second->c_str();
            } else {
                return nullptr;
            }
        }
    }

    // create node and attribute keys
    Glib::ustring node_key, attr_key;
    _keySplit(path, node_key, attr_key);

    // retrieve the attribute
    char const *result = nullptr;
    if (auto node = _getNode(node_key, false)) {
        gchar const *attr = node->attribute(attr_key.c_str());
        if (attr) {
            result = attr;
        }
    }

    if (_initialized) {
        if (result) {
            cachedRawValue[path.raw()] = std::make_optional<Glib::ustring>(result);
        } else {
            cachedRawValue[path.raw()] = {};
        }
    }

    return result;
}

void Preferences::_setRawValue(Glib::ustring const &path, Glib::ustring const &value)
{
    // create node and attribute keys
    Glib::ustring node_key, attr_key;
    _keySplit(path, node_key, attr_key);

    // update cache first, so by the time notification change fires and observers are called,
    // they have access to current settings even if they watch a group
    if (_initialized) {
        cachedRawValue[path.raw()] = value;
    }

    // set the attribute
    Inkscape::XML::Node *node = _getNode(node_key, true);
    node->setAttributeOrRemoveIfEmpty(attr_key, value);
}

// The _extract* methods are where the actual work is done - they define how preferences are stored
// in the XML file.

bool Preferences::_extractBool(Entry const &v)
{
    if (v.bool_cache) return *v.bool_cache;
    auto const &s = *v._value;
    if (s.size() == 0 || s == "0" || s == "false") {
        v.bool_cache = false;
    } else {
        v.bool_cache = true;
    }
    return *v.bool_cache;
}

int Preferences::_extractInt(Entry const &v)
{
    if (v.int_cache) return *v.int_cache;
    auto const &s = *v._value;
    if (s == "true") {
        v.int_cache = 1;
    } else if ( s == "false") {
        v.int_cache = 0;
    } else {
        int val = 0;

        // TODO: We happily save unsigned integers (notably RGBA values) as signed integers and overflow as needed.
        //       We should consider adding an unsigned integer type to preferences or use HTML colors where appropriate
        //       (the latter would breaks backwards compatibility, though)
        errno = 0;
        val = static_cast<int>(strtol(s.c_str(), nullptr, 0));
        if (errno == ERANGE) {
            errno = 0;
            val = static_cast<int>(strtoul(s.c_str(), nullptr, 0));
            if (errno == ERANGE) {
                g_warning("Integer preference out of range: '%s' (raw value: %s)", v._pref_path.c_str(), s.c_str());
                val = 0;
            }
        }

        v.int_cache = val;
    }
    return *v.int_cache;
}

unsigned int Preferences::_extractUInt(Entry const &v)
{
    if (v.uint_cache) return *v.uint_cache;
    auto const &s = *v._value;

    // Note: 'strtoul' can also read overflowed (i.e. negative) signed int values that we used to save before we
    //       had the unsigned type, so this is fully backwards compatible and can be replaced seamlessly
    unsigned int val = 0;
    errno = 0;
    val = static_cast<unsigned int>(strtoul(s.c_str(), nullptr, 0));
    if (errno == ERANGE) {
        g_warning("Unsigned integer preference out of range: '%s' (raw value: %s)", v._pref_path.c_str(), s.c_str());
        val = 0;
    }

    v.uint_cache = val;
    return *v.uint_cache;
}

double Preferences::_extractDouble(Entry const &v)
{
    if (v.double_cache) return *v.double_cache;
    auto const &s = *v._value;
    v.double_cache = g_ascii_strtod(s.c_str(), nullptr);
    return *v.double_cache;
}

double Preferences::_extractDouble(Entry const &v, Glib::ustring const &requested_unit)
{
    double val = _extractDouble(v);
    Glib::ustring unit = _extractUnit(v);

    if (unit.length() == 0) {
        // no unit specified, don't do conversion
        return val;
    }
    return val * (unit_table.getUnit(unit)->factor / unit_table.getUnit(requested_unit)->factor); /// \todo rewrite using Quantity class, so the standard code handles unit conversion
}

Glib::ustring Preferences::_extractString(Entry const &v)
{
    return *v._value;
}

Glib::ustring Preferences::_extractUnit(Entry const &v)
{
    if (v.unit_cache) return *v.unit_cache;
    auto const &str = *v._value;
    gchar const *e;
    g_ascii_strtod(str.c_str(), (char **) &e);
    if (e == str.c_str()) {
        v.unit_cache = "";
        return *v.unit_cache;
    }

    if (e[0] == 0) {
        /* Unitless */
        v.unit_cache = "";
    } else {
        v.unit_cache = e;
    }
    return *v.unit_cache;
}

guint32 Preferences::_extractColor(Entry const &v)
{
    if (v.color_cache) return *v.color_cache;
    auto const &s = *v._value;
    std::istringstream hr(s.raw());
    guint32 color;
    if (s[0] == '#') {
        hr.ignore(1);
        hr >> std::hex >> color;
    } else {
        hr >> color;
    }
    v.color_cache = color;
    return *v.color_cache;
}

SPCSSAttr *Preferences::_extractStyle(Entry const &v)
{
    if (v.style_cache) return *v.style_cache;
    SPCSSAttr *style = sp_repr_css_attr_new();
    sp_repr_css_attr_add_from_string(style, v._value->c_str());
    v.style_cache = style;
    return *v.style_cache;
}

SPCSSAttr *Preferences::_extractInheritedStyle(Entry const &v)
{
    // This is the dirtiest extraction method. Generally we ignore whatever was in v._value
    // and just get the style using sp_repr_css_attr_inherited. To implement this in GConf,
    // we'll have to walk up the tree and call sp_repr_css_attr_add_from_string
    Glib::ustring node_key, attr_key;
    _keySplit(v._pref_path, node_key, attr_key);

    Inkscape::XML::Node *node = _getNode(node_key, false);
    return sp_repr_css_attr_inherited(node, attr_key.c_str());
}

// XML backend helper: Split the path into a node key and an attribute key.
void Preferences::_keySplit(Glib::ustring const &pref_path, Glib::ustring &node_key, Glib::ustring &attr_key)
{
    // everything after the last slash
    attr_key = pref_path.substr(pref_path.rfind('/') + 1, Glib::ustring::npos);
    // everything before the last slash
    node_key = pref_path.substr(0, pref_path.rfind('/'));
}

void Preferences::_reportError(Glib::ustring const &msg, Glib::ustring const &secondary)
{
    _hasError = true;
    _lastErrPrimary = msg;
    _lastErrSecondary = secondary;
    if (_errorHandler) {
        _errorHandler->handleError(msg, secondary);
    }
}

void Preferences::setErrorHandler(ErrorReporter* handler)
{
    _errorHandler = handler;
}

void Preferences::unload(bool save)
{
    if (_instance)
    {
        if (save) {
            _instance->save();
        }
        delete _instance;
        _instance = nullptr;
    }
}

Glib::ustring Preferences::getPrefsFilename() const
{ //
    return Glib::filename_to_utf8(_prefs_filename);
}

Preferences *Preferences::_instance = nullptr;


PrefObserver Preferences::PreferencesObserver::create(
    Glib::ustring path, std::function<void (const Preferences::Entry& new_value)> callback) {
    assert(callback);

    return PrefObserver(new Preferences::PreferencesObserver(std::move(path), std::move(callback)));
}

Preferences::PreferencesObserver::PreferencesObserver(Glib::ustring path, std::function<void (const Preferences::Entry&)> callback) :
    Observer(std::move(path)), _callback(std::move(callback)) {

    auto prefs = Inkscape::Preferences::get();
    prefs->addObserver(*this);
}

void Preferences::PreferencesObserver::notify(Preferences::Entry const& new_val) {
    _callback(new_val);
}

PrefObserver Preferences::createObserver(Glib::ustring path, std::function<void (const Preferences::Entry&)> callback) {
    return Preferences::PreferencesObserver::create(path, std::move(callback));
}

PrefObserver Preferences::createObserver(Glib::ustring path, std::function<void ()> callback) {
    return createObserver(std::move(path), [=](const Entry&) { callback(); });
}

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
