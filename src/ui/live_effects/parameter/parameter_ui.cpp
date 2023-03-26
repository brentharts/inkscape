// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "parameter_ui.h"

namespace Inkscape {
namespace LivePathEffect {
    class Parameter;
}
namespace UI {
namespace LivePathEffect {

using namespace Inkscape::LivePathEffect;

ParameterUI::ParameterUI(Inkscape::LivePathEffect::Parameter *_parameter)
    :parameter(_parameter)
{

};

} /* namespace LivePathEffect */
} /* namespace UI */
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
