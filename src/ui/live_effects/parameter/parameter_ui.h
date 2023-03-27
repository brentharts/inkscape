// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_UI_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_UI_H

/*
 * Inkscape::LivePathEffectParameters
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/parameter/parameter.h"

namespace Inkscape {
namespace LivePathEffect {
    class Parameter;
}

namespace UI {
namespace LivePathEffect {

using namespace Inkscape::LivePathEffect;

class ParameterUI {
  public:
    virtual ~ParameterUI(){};
    ParameterUI(const ParameterUI&) = delete;
    ParameterUI& operator=(const ParameterUI&) = delete;
    ParameterUI(Inkscape::LivePathEffect::Parameter *_parameter);
    Inkscape::LivePathEffect::Parameter * parameter;
};

} // namespace LivePathEffect
} // namespace UI 
} // namespace Inkscape

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
