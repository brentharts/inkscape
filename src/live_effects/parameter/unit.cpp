// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Maximilian Albert 2008 <maximilian.albert@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "unit.h"

#include <glibmm/i18n.h>

#include "live_effects/effect.h"
#include "ui/icon-names.h"
#include "ui/widget/registered-widget.h"
#include "util/units.h"

using Inkscape::Util::unit_table;

namespace Inkscape {

namespace LivePathEffect {


UnitParam::UnitParam( const Glib::ustring& label, const Glib::ustring& tip,
                              const Glib::ustring& key, Inkscape::UI::Widget::Registry* wr,
                              Effect* effect, Glib::ustring default_unit)
    : Parameter(label, tip, key, wr, effect)
{
    defunit = unit_table.getUnit(default_unit);
    unit = defunit;
}

UnitParam::~UnitParam() = default;

bool
UnitParam::param_readSVGValue(const gchar * strvalue)
{
    if (strvalue) {
        param_set_value(*unit_table.getUnit(strvalue));
        return true;
    }
    return false;
}

Glib::ustring
UnitParam::param_getSVGValue() const
{
    return unit->abbr;
}

Glib::ustring
UnitParam::param_getDefaultSVGValue() const
{
    return defunit->abbr;
}

void
UnitParam::param_set_default()
{
    param_set_value(*defunit);
}

void 
UnitParam::param_update_default(const gchar * default_unit)
{
    defunit = unit_table.getUnit((Glib::ustring)default_unit);
}

void
UnitParam::param_set_value(Inkscape::Util::Unit const &val)
{
    param_effect->refresh_widgets = true;
    unit = new Inkscape::Util::Unit(val);
}

const gchar *
UnitParam::get_abbreviation() const
{
    return unit->abbr.c_str();
}

Gtk::Widget *
UnitParam::param_newWidget()
{
    auto const unit_menu = Gtk::make_managed<UI::Widget::RegisteredUnitMenu>( param_label,
                                                                              param_key,
                                                                             *param_wr,
                                                                              param_effect->getRepr(),
                                                                              param_effect->getSPDoc() );

    unit_menu->setUnit(unit->abbr);
    unit_menu->set_undo_parameters(_("Change unit parameter"), INKSCAPE_ICON("dialog-path-effects"));
    return unit_menu;
}

} /* namespace LivePathEffect */
} /* namespace Inkscape */

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
