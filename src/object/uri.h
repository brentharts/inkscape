// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2003 MenTaLguY
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_URI_H
#define INKSCAPE_URI_H

#include <exception>
#include <libxml/uri.h>
#include <string>

namespace Inkscape {

/**
 * Represents an URI as per RFC 2396.
 *
 * Typical use-cases of this class:
 * - converting between relative and absolute URIs
 * - converting URIs to/from filenames (alternative: Glib functions, but those only handle absolute paths)
 * - generic handling of data/file/http URIs (e.g. URI::getContents and URI::getMimeType)
 *
 * Wraps libxml2's URI functions. Direct usage of libxml2's C-API is discouraged if favor of
 * Inkscape::URI. (At the time of writing this, no de-factor standard C++ URI library exists, so
 * wrapping libxml2 seems like a good solution)
 *
 * Implementation detail: Immutable type, copies share a ref-counted data pointer.
 */
class URI {
public:

    /* Blank constructor */
    URI();

    /**
     * Copy constructor.
     */
    URI(URI const &uri);

    /**
     * Constructor from a C-style ASCII string.
     *
     * @param preformed Properly quoted C-style string to be represented.
     * @param baseuri If @a preformed is a relative URI, use @a baseuri to make it absolute
     *
     * @throw MalformedURIException
     */
    explicit URI(char const *preformed, char const *baseuri = nullptr);
    explicit URI(char const *preformed, URI const &baseuri);

    /**
     * Destructor.
     */
    ~URI();

    /**
     * Determines if the URI represented is an 'opaque' URI.
     *
     * @return \c true if the URI is opaque, \c false if hierarchial.
     */
    bool isOpaque() const { return _impl->isOpaque(); }

    /**
     * Determines if the URI represented is 'relative' as per RFC 2396.
     *
     * Relative URI references are distinguished by not beginning with a
     * scheme name.
     *
     * @return \c true if the URI is relative, \c false if it is absolute.
     */
    bool isRelative() const { return _impl->isRelative(); }

    /**
     * Determines if the relative URI represented is a 'net-path' as per RFC 2396.
     *
     * A net-path is one that starts with "//".
     *
     * @return \c true if the URI is relative and a net-path, \c false otherwise.
     */
    bool isNetPath() const { return _impl->isNetPath(); }

    /**
     * Determines if the relative URI represented is a 'relative-path' as per RFC 2396.
     *
     * A relative-path is one that starts with no slashes.
     *
     * @return \c true if the URI is relative and a relative-path, \c false otherwise.
     */
    bool isRelativePath() const { return _impl->isRelativePath(); }

    /**
     * Determines if the relative URI represented is a 'absolute-path' as per RFC 2396.
     *
     * An absolute-path is one that starts with a single "/".
     *
     * @return \c true if the URI is relative and an absolute-path, \c false otherwise.
     */
    bool isAbsolutePath() const { return _impl->isAbsolutePath(); }

    /**
     * Return the scheme, e.g.\ "http", or \c NULL if this is not an absolute URI.
     */
    const char *getScheme() const { return _impl->getScheme(); }

    /**
     * Return the path.
     *
     * Example: "http://host/foo/bar?query#frag" -> "/foo/bar"
     *
     * For an opaque URI, this is identical to getOpaque()
     */
    const char *getPath() const { return _impl->getPath(); }

    /**
     * Return the query, which is the part between "?" and the optional fragment hash ("#")
     */
    const char *getQuery() const { return _impl->getQuery(); }

    /**
     * Return the fragment, which is everything after "#"
     */
    const char *getFragment() const { return _impl->getFragment(); }

    /**
     * For an opaque URI, return everything between the scheme colon (":") and the optional
     * fragment hash ("#"). For non-opaque URIs, return NULL.
     */
    const char *getOpaque() const { return _impl->getOpaque(); }

    /**
     * @deprecated The regular constructor auto-detects UTF-8 characters and percent-encodes them.
     *
     * @todo remove, it's unused and percent-encodes most reserved characters, including "%", ":", "?", "#".
     */
    static URI fromUtf8( char const* path );

    /**
     * Construct a "file" URI from an absolute filename.
     */
    static URI from_native_filename(char const *path);

    /**
     * URI of a local directory. The URI path will end with a slash.
     */
    static URI from_dirname(char const *path);

    /**
     * Convenience function for the common use case given a xlink:href attribute and a local
     * directory as the document base. Returns an empty URI on failure.
     */
    static URI from_href_and_basedir(char const *href, char const *basedir);

    /**
     * @deprecated Use ::toNativeFilename() instead
     *
     * @todo remove
     */
    const std::string getFullPath(std::string const &base) const;

    /**
     * Convert this URI to a native filename.
     *
     * @throw Glib::ConvertError If this is not a "file" URI
     */
    std::string toNativeFilename() const;

    /**
     * Return the string representation of this URI
     *
     * @param baseuri Return a relative path if this URI shares protocol and host with @a baseuri
     */
    std::string str(char const *baseuri = nullptr) const;

    /**
     * Get the MIME type (e.g.\ "image/png")
     */
    std::string getMimeType() const;

    /**
     * Return the contents of the file
     */
    std::string getContents() const;

    /**
     * Return a CSS formatted url value
     *
     * @param baseuri Return a relative path if this URI shares protocol and host with @a baseuri
     */
    std::string cssStr(char const *baseuri = nullptr) const {
        return "url(" + str(baseuri) + ")";
    }

    /**
     * True if the scheme equals the given string (not case sensitive)
     */
    bool hasScheme(const char *scheme) const;

    /**
     * Assignment operator.
     */
    URI &operator=(URI const &uri);

private:
    class Impl {
    public:
        static Impl *create(xmlURIPtr uri);
        void reference();
        void unreference();

        bool isOpaque() const;
        bool isRelative() const;
        bool isNetPath() const;
        bool isRelativePath() const;
        bool isAbsolutePath() const;
        const char *getScheme() const;
        const char *getPath() const;
        const char *getQuery() const;
        const char *getFragment() const;
        const char *getOpaque() const;
        char *toString() const;
    private:
        Impl(xmlURIPtr uri);
        ~Impl();
        int _refcount;
        xmlURIPtr _uri;
    };
    Impl *_impl;
};

}  /* namespace Inkscape */

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
