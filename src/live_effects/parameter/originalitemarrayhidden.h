// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_ORIGINALITEMARRAYHIDDEN_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_ORIGINALITEMARRAYHIDDEN_H

/*
 * Inkscape::LivePathEffectParameters
 *
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <vector>

#include "live_effects/parameter/parameter.h"
#include "live_effects/parameter/item-reference.h"

#include "svg/svg.h"
#include "svg/stringstream.h"
#include "item-reference.h"

class SPObject;
namespace Inkscape {

namespace LivePathEffect {

class OriginalItemArrayHiddenParam : public Parameter {
public:
    class ModelColumns;
    
    OriginalItemArrayHiddenParam( const Glib::ustring& label,
                const Glib::ustring& tip,
                const Glib::ustring& key,
                Inkscape::UI::Widget::Registry* wr,
                Effect* effect,
                bool preserve_slots = true);

    ~OriginalItemArrayHiddenParam() override;
    Gtk::Widget * param_newWidget() override;
    bool param_readSVGValue(const gchar * strvalue) override;
    Glib::ustring param_getSVGValue() const override;
    Glib::ustring param_getDefaultSVGValue() const override;
    void param_set_default() override;
    void param_update_default(const gchar * default_value) override{};
    std::vector<LPEItemRef*> _vector;
    size_t getLastInsertPos() const {return _last;} 
    void setLastInsertPos(size_t last) {_last = last;}
    void clear();
    void setUpdating(bool updating) {_updating = updating;}
    bool getUpdating() const {return _updating;}
    void setPreserveSlots(bool preserve_slots) {_preserve_slots = preserve_slots;}
    bool getPreserveSlots() {return _preserve_slots;}
    void setCheckIds(bool check_ids) {_check_ids = check_ids;}
    bool getCheckIds() const {return _check_ids;}
    void link(SPObject *to, size_t pos = Glib::ustring::npos);
    void multilink(std::vector<SPObject *> objs);
    void unlink(SPObject *to);
    void remove_link(SPObject *to);    
    
protected:
    void unlink(LPEItemRef* to);
    void remove_link(LPEItemRef* to);
    void linked_changed(SPObject *old_obj, SPObject *new_obj, LPEItemRef* to);
    void linked_modified(SPObject *linked_obj, guint flags, LPEItemRef* to);
    void linked_transformed(Geom::Affine const *, SPItem *, LPEItemRef* to);
    void linked_delete(SPObject *deleted, LPEItemRef* to);
    
private:
    bool _updating = false;
    size_t _last = Glib::ustring::npos;
    void update();
    bool _preserve_slots = false;
    bool _check_ids = false;
    OriginalItemArrayHiddenParam(const OriginalItemArrayHiddenParam&) = delete;
    OriginalItemArrayHiddenParam& operator=(const OriginalItemArrayHiddenParam&) = delete;
};

} //namespace LivePathEffect

} //namespace Inkscape

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
