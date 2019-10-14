// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Filter effect selection selection widget
 *
 * Author:
 *   Nicholas Bishop <nicholasbishop@gmail.com>
 *   Tavmjong Bah
 *
 * Copyright (C) 2007, 2017 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "filter-effect-chooser.h"

#include "document.h"


using Inkscape::Util::EnumData;
using Inkscape::Util::EnumDataConverter;


namespace Inkscape {
    
    const EnumData<Inkscape::CSSBlendMode> CSSBlendModeData[SP_CSS_BLEND_ENDMODE] = {
        {SP_CSS_BLEND_NORMAL,       _("Normal"),      "normal"},
        {SP_CSS_BLEND_MULTIPLY,     _("Multiply"),    "multiply"},
        {SP_CSS_BLEND_SCREEN,       _("Screen"),      "screen"},
        {SP_CSS_BLEND_DARKEN,       _("Darken"),      "darken"},
        {SP_CSS_BLEND_LIGHTEN,      _("Lighten"),     "lighten"},
    // New in Compositing and Blending Level 1
        {SP_CSS_BLEND_OVERLAY,      _("Overlay"),     "overlay"},
        {SP_CSS_BLEND_COLORDODGE,   _("Color Dodge"), "color-dodge"},
        {SP_CSS_BLEND_COLORBURN,    _("Color Burn"),  "color-burn"},
        {SP_CSS_BLEND_HARDLIGHT,    _("Hard Light"),  "hard-light"},
        {SP_CSS_BLEND_SOFTLIGHT,    _("Soft Light"),  "soft-light"},
        {SP_CSS_BLEND_DIFFERENCE,   _("Difference"),  "difference"},
        {SP_CSS_BLEND_EXCLUSION,    _("Exclusion"),   "exclusion"},
        {SP_CSS_BLEND_HUE,          _("Hue"),         "hue"},
        {SP_CSS_BLEND_SATURATION,   _("Saturation"),  "saturation"},
        {SP_CSS_BLEND_COLOR,        _("Color"),       "color"},
        {SP_CSS_BLEND_LUMINOSITY,   _("Luminosity"),  "luminosity"}
    };
#ifdef WITH_CSSBLEND
    const EnumDataConverter<Inkscape::CSSBlendMode> CSSBlendModeConverter(CSSBlendModeData, SP_CSS_BLEND_ENDMODE);
#else
    // Disable new blend modes in GUI until widely implemented.
    const EnumDataConverter<Inkscape::CSSBlendMode> CSSBlendModeConverter(CSSBlendModeData, SP_CSS_BLEND_LUMINOSITY);
#endif

namespace UI {
namespace Widget {

SimpleFilterModifier::SimpleFilterModifier(int flags)
    : _flags( flags )
    , _lb_blend(_("Blend mode:"))
    , _blend(CSSBlendModeConverter, SP_ATTR_INVALID, false)
    , _blur(   _("Blur (%)"   ), 0, 0, 100, 1, 0.1, 1)
    , _opacity(_("Opacity (%)"), 0, 0, 100, 1, 0.1, 1)
{
    set_name("SimpleFilterModifier");

    _flags = flags;

    if (flags & BLEND) {
        add(_hb_blend);
        _lb_blend.set_use_underline();
        _hb_blend.set_halign(Gtk::ALIGN_END);
        _hb_blend.set_valign(Gtk::ALIGN_CENTER);
        _hb_blend.set_margin_end(5);
        _lb_blend.set_mnemonic_widget(_blend);
        _hb_blend.pack_start(_lb_blend, false, false, 5);
        _hb_blend.pack_start(_blend, false, false, 5);
        Gtk::Separator *separator = Gtk::manage(new Gtk::Separator());
        separator->set_margin_top(8);
        separator->set_margin_bottom(8);
        add(*separator);
    }

    if (flags & BLUR) {
        add(_blur);
    }

    if (flags & OPACITY) {
        add(_opacity);
    }
    show_all_children();

    _blend.signal_changed().connect(signal_blend_changed());
    _blur.signal_value_changed().connect(signal_blur_changed());
    _opacity.signal_value_changed().connect(signal_opacity_changed());
}

sigc::signal<void>& SimpleFilterModifier::signal_blend_changed()
{
    return _signal_blend_changed;
}

sigc::signal<void>& SimpleFilterModifier::signal_blur_changed()
{
    return _signal_blur_changed;
}

sigc::signal<void>& SimpleFilterModifier::signal_opacity_changed()
{
    return _signal_opacity_changed;
}

const Glib::ustring SimpleFilterModifier::get_blend_mode()
{
    const Util::EnumData<Inkscape::CSSBlendMode> *d = _blend.get_active_data();
    if (d) {
        return _blend.get_active_data()->key;
    } else
        return "normal";
}

void SimpleFilterModifier::set_blend_mode(const int val)
{
    _blend.set_active(val);
}

double SimpleFilterModifier::get_blur_value() const
{
    return _blur.get_value();
}

void SimpleFilterModifier::set_blur_value(const double val)
{
    _blur.set_value(val);
}

double SimpleFilterModifier::get_opacity_value() const
{
    return _opacity.get_value();
}

void SimpleFilterModifier::set_opacity_value(const double val)
{
    _opacity.set_value(val);
}

}
}
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
