// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_EFFECT_UI_H
#define INKSCAPE_EFFECT_UI_H

#include "live_effects/effect.h"

namespace Inkscape {
    namespace LivePathEffect {
        class LPEFilletChamfer;
    }
namespace UI {
namespace LivePathEffect {

class EffectUI {
public:
    virtual ~EffectUI(){};
    EffectUI(const EffectUI&) = delete;
    EffectUI& operator=(const EffectUI&) = delete;
    EffectUI(Inkscape::LivePathEffect::Effect *_effect);
    Inkscape::LivePathEffect::Effect *effect;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :