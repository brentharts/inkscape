// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 * Copyright (C) 2008 Johan Engelen
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_DISPLAY_CURVE_H
#define SEEN_DISPLAY_CURVE_H

#include <2geom/pathvector.h>
#include <cstddef>
#include <boost/optional.hpp>
#include <list>
#include <memory>
#include <utility>

class SPCurve;

/**
 * Wrapper around a Geom::PathVector object.
 */
class SPCurve {
    //! Preferred smart pointer type for SPCurve
    using smart_pointer = ::std::unique_ptr<SPCurve>;

public:
    /* Constructors */
    explicit SPCurve();
    explicit SPCurve(Geom::PathVector  pathv);

    static smart_pointer new_from_rect(Geom::Rect const &rect, bool all_four_sides = false);

    virtual ~SPCurve();

    // Don't implement these:
    SPCurve(const SPCurve&) = delete;
    SPCurve& operator=(const SPCurve&) = delete;

    void set_pathvector(Geom::PathVector const & new_pathv);
    Geom::PathVector const & get_pathvector() const;

    smart_pointer ref();
    smart_pointer copy() const;

    [[deprecated("Use std::unique_ptr<SPCurve>")]] std::nullptr_t unref()
    {
        _unref();
        return nullptr;
    }

    size_t get_segment_count() const;
    size_t nodes_in_path() const;

    bool is_empty() const;
    bool is_unset() const;
    bool is_closed() const;
    bool is_equal(SPCurve * other) const;
    Geom::Curve const * last_segment() const;
    Geom::Path const * last_path() const;
    Geom::Curve const * first_segment() const;
    Geom::Path const * first_path() const;
    boost::optional<Geom::Point> first_point() const;
    boost::optional<Geom::Point> last_point() const;
    boost::optional<Geom::Point> second_point() const;
    boost::optional<Geom::Point> penultimate_point() const;

    void reset();

    void moveto(Geom::Point const &p);
    void moveto(double x, double y);
    void lineto(Geom::Point const &p);
    void lineto(double x, double y);
    void quadto(Geom::Point const &p1, Geom::Point const &p2);
    void quadto(double x1, double y1, double x2, double y2);
    void curveto(Geom::Point const &p0, Geom::Point const &p1, Geom::Point const &p2);
    void curveto(double x0, double y0, double x1, double y1, double x2, double y2);
    void closepath();
    void closepath_current();
    void backspace();

    void transform(Geom::Affine const &m);
    void stretch_endpoints(Geom::Point const &, Geom::Point const &);
    void move_endpoints(Geom::Point const &, Geom::Point const &);
    void last_point_additive_move(Geom::Point const & p);

    void append(SPCurve const &curve2, bool use_lineto = false);
    [[deprecated("Use reference overload")]] void append(SPCurve const *curve2, bool use_lineto)
    {
        append(*curve2, use_lineto);
    }

    void append_continuous(SPCurve const &c1, double tolerance = 0.0625) { append_continuous(&c1, tolerance); }
    SPCurve * append_continuous(SPCurve const *c1, double tolerance);

    smart_pointer create_reverse() const;

    std::list<smart_pointer> split() const;

    friend class std::default_delete<SPCurve>;
    friend class CurveTest; // for ref count test

private:
    void _unref();

protected:
    size_t _refcount;

    Geom::PathVector _pathv;
};

/**
 * Specialized deleter allows using std::unique_ptr<SPCurve> with shared references.
 *
 * @verbatim
   auto curve1 = std::make_unique<SPCurve>();
   auto curve2 = curve1->ref();
   @endverbatim
 *
 * Is equivalent to:
 * @verbatim
   auto curve1 = new SPCurve();
   auto curve2 = curve1->ref().release();
   curve2->unref();
   curve1->unref();
   @endverbatim
 */
template <>
inline void ::std::default_delete<SPCurve>::operator()(SPCurve *ptr) const
// This is `noexcept` in libc++ but not in libstdc++
#ifdef _LIBCPP_VERSION
    noexcept
#endif
{
    ptr->_unref();
}

#endif // !SEEN_DISPLAY_CURVE_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
