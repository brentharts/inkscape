#include <2geom/point.h>

#include "node.h"
#include "svg/stringstream.h"
#include "svg/css-ostringstream.h"
#include "svg/svg-length.h"

namespace Inkscape{
namespace XML {

void Node::setAttribute(Inkscape::Util::const_char_ptr key, Inkscape::Util::const_char_ptr value)
{
    this->setAttributeImpl(key.data(), value.data());
}

unsigned int Node::getAttributeBoolean(gchar const *key, unsigned int *val)
{
    gchar const *v;

    g_return_val_if_fail(key != nullptr, FALSE);
    g_return_val_if_fail(val != nullptr, FALSE);

    v = this->attribute(key);

    if (v != nullptr) {
        if (!g_ascii_strcasecmp(v, "true") || !g_ascii_strcasecmp(v, "yes") || !g_ascii_strcasecmp(v, "y") ||
            (atoi(v) != 0)) {
            *val = TRUE;
        } else {
            *val = FALSE;
        }
        return TRUE;
    } else {
        *val = FALSE;
        return FALSE;
    }
}

unsigned int Node::getAttributeInt(gchar const *key, int *val)
{
    gchar const *v;

    g_return_val_if_fail(key != nullptr, FALSE);
    g_return_val_if_fail(val != nullptr, FALSE);

    v = this->attribute(key);

    if (v != nullptr) {
        *val = atoi(v);
        return TRUE;
    }

    return FALSE;
}

unsigned int Node::getAttributeDouble(gchar const *key, double *val)
{
    g_return_val_if_fail(key != nullptr, FALSE);
    g_return_val_if_fail(val != nullptr, FALSE);

    gchar const *v = this->attribute(key);

    if (v != nullptr) {
        *val = g_ascii_strtod(v, nullptr);
        return TRUE;
    }

    return FALSE;
}

unsigned int Node::setAttributeBoolean(gchar const *key, unsigned int val)
{
    g_return_val_if_fail(key != nullptr, FALSE);

    this->setAttribute(key, (val) ? "true" : "false");
    return true;
}

unsigned int Node::setAttributeInt(gchar const *key, int val)
{
    gchar c[32];

    g_return_val_if_fail(key != nullptr, FALSE);

    g_snprintf(c, 32, "%d", val);

    this->setAttribute(key, c);
    return true;
}

unsigned int Node::setAttributeCssDouble(gchar const *key, double val)
{
    g_return_val_if_fail(key != nullptr, FALSE);

    Inkscape::CSSOStringStream os;
    os << val;

    this->setAttribute(key, os.str());
    return true;
}

unsigned int Node::setAttributeSvgDouble(gchar const *key, double val)
{
    g_return_val_if_fail(key != nullptr, FALSE);
    g_return_val_if_fail(val == val, FALSE); // tests for nan

    Inkscape::SVGOStringStream os;
    os << val;

    this->setAttribute(key, os.str());
    return true;
}

unsigned int Node::setAttributeSvgNonDefaultDouble(gchar const *key, double val, double default_value)
{
    if (val == default_value) {
        this->removeAttribute(key);
        return true;
    }
    return this->setAttributeSvgDouble(key, val);
}

unsigned int Node::setAttributeSvgLength(gchar const *key, SVGLength &val)
{
    g_return_val_if_fail(key != nullptr, FALSE);

    this->setAttribute(key, val.write());
    return true;
}

unsigned Node::setAttributePoint(gchar const *key, Geom::Point const &val)
{
    g_return_val_if_fail(key != nullptr, FALSE);

    Inkscape::SVGOStringStream os;
    os << val[Geom::X] << "," << val[Geom::Y];

    this->setAttribute(key, os.str());
    return true;
}

unsigned int Node::getAttributePoint(gchar const *key, Geom::Point *val)
{
    g_return_val_if_fail(key != nullptr, FALSE);
    g_return_val_if_fail(val != nullptr, FALSE);

    gchar const *v = this->attribute(key);

    g_return_val_if_fail(v != nullptr, FALSE);

    gchar **strarray = g_strsplit(v, ",", 2);

    if (strarray && strarray[0] && strarray[1]) {
        double newx, newy;
        newx = g_ascii_strtod(strarray[0], nullptr);
        newy = g_ascii_strtod(strarray[1], nullptr);
        g_strfreev(strarray);
        *val = Geom::Point(newx, newy);
        return TRUE;
    }

    g_strfreev(strarray);
    return FALSE;
}

void Node::setAttributeOrRemoveIfEmpty(Inkscape::Util::const_char_ptr key, Inkscape::Util::const_char_ptr value)
{
    this->setAttributeImpl(key.data(), (value.data() == nullptr || value.data()[0] == '\0') ? nullptr : value.data());
}

} // namespace XML
} // namespace Inkscape