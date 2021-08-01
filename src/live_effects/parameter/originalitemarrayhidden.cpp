// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/parameter/originalitemarrayhidden.h"
#include "live_effects/lpeobject.h"

#include <gtkmm/widget.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/imagemenuitem.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/scrolledwindow.h>

#include <glibmm/i18n.h>

#include "inkscape.h"
#include "originalitem.h"
#include "svg/stringstream.h"
#include "svg/svg.h"
#include "ui/clipboard.h"
#include "ui/icon-loader.h"

#include "object/uri.h"

#include "live_effects/effect.h"

#include "verbs.h"
#include "document-undo.h"
#include "document.h"

namespace Inkscape {

namespace LivePathEffect {


OriginalItemArrayHiddenParam::OriginalItemArrayHiddenParam( const Glib::ustring& label,
        const Glib::ustring& tip,
        const Glib::ustring& key,
        Inkscape::UI::Widget::Registry* wr,
        Effect* effect,
        bool preserve_slots)
: Parameter(label, tip, key, wr, effect), 
        _vector(), _preserve_slots(preserve_slots)
{    
    param_widget_is_visible(false);
}

OriginalItemArrayHiddenParam::~OriginalItemArrayHiddenParam()
{
    _updating = true;
    while (!_vector.empty()) {
        LPEItemRef *w = _vector.back();
        if (w) {
            delete w;
        }
        w = nullptr;
        _vector.pop_back();
    }
    _updating = false;
}

void OriginalItemArrayHiddenParam::param_set_default()
{
    
}

Gtk::Widget *
OriginalItemArrayHiddenParam::param_newWidget()
{
    return nullptr;
}


void
OriginalItemArrayHiddenParam::link(SPObject *obj, size_t pos)
{
    _updating = true;
    Glib::ustring itemid = "";
    if (obj && obj->getId()) {
        itemid = obj->getId();
        itemid.insert(itemid.begin(), '#');
    }
    size_t counter = 0;
    bool inserted = false;
    bool foundOne = false;
    Inkscape::SVGOStringStream os;
    for (auto &iter : _vector) {
        if (counter == pos) {
            _last = counter;
            if (foundOne) {
                os << ";";
            } else {
                foundOne = true;
            }
            os << itemid.c_str();
            inserted = true;
            if (iter) {
                LPEItemRef *w = iter;
                if (w) {
                    delete w;
                }
                w = nullptr;
                iter = nullptr;
            }
        } else {
            if (foundOne) {
                os << ";";
            } else {
                foundOne = true;
            }
            os <<  (iter == nullptr ? "" : iter->href);
        }
        counter++;
    }

    if (!inserted) {
        if (foundOne) {
            os << ";";
        }
        _last = counter;
        os << itemid.c_str();
    }
    Glib::ustring val = os.str().erase (os.str().find_last_not_of(';') + 1, std::string::npos );
    _vector.clear();
    param_readSVGValue(val.c_str());
    param_write_to_repr(val.c_str());
    _updating = false;
}

void
OriginalItemArrayHiddenParam::multilink(std::vector<SPObject *> objs)
{
    if (objs.empty()) {
        return;
    }
    _updating = true;
    clear();
    bool foundOne = false;
    Inkscape::SVGOStringStream os;
    for (auto &obj : objs) {
        Glib::ustring itemid = "";
        if (obj && obj->getId()) {
            itemid = obj->getId();
            itemid.insert(itemid.begin(), '#');
        }
        if (foundOne) {
            os << ";";
        } else {
            foundOne = true;
        }
        os <<  itemid;
    }
    Glib::ustring val = os.str().erase (os.str().find_last_not_of(';') + 1, std::string::npos );
    _vector.clear();
    param_readSVGValue(val.c_str());
    param_write_to_repr(val.c_str());
    _updating = false;
}

void
OriginalItemArrayHiddenParam::remove_link(SPObject *obj)
{
    if (!obj) {
        return;
    }
    for (std::vector<LPEItemRef*>::iterator iter = _vector.begin(); iter != _vector.end(); ++iter) {
        LPEItemRef *w = *iter;
        if (w && w->getObject() == obj) {
            remove_link(w);
        }
    }
}

void
OriginalItemArrayHiddenParam::unlink(SPObject *obj)
{
    if (!obj) {
        return;
    }
    for (std::vector<LPEItemRef*>::iterator iter = _vector.begin(); iter != _vector.end(); ++iter) {
        LPEItemRef *w = *iter;
        if (w && w->getObject() == obj) {
            unlink(w);
        }
    }
}

void OriginalItemArrayHiddenParam::unlink(LPEItemRef* to)
{
    if (!to) {
        return;
    }
    to->linked_delete_connection.disconnect();
    to->linked_modified_connection.disconnect();
    to->linked_transformed_connection.disconnect();
    to->linked_changed_connection.disconnect();
    to->detach();
    if (to->href) {
        g_free(to->href);
        to->href = nullptr;
    }    
}
        
void OriginalItemArrayHiddenParam::clear()
{
    _updating = true;
    for (std::vector<LPEItemRef*>::reverse_iterator iter = _vector.rbegin(); iter != _vector.rend(); ++iter) {
        LPEItemRef *w = *iter;
        delete w;
        w = nullptr;
    }
    _vector.clear();
    _updating = false;
}

void OriginalItemArrayHiddenParam::remove_link(LPEItemRef* to)
{
    if (!to) {
        return;
    }
    _updating = true;
    std::vector<LPEItemRef*> _nvect = _vector;
    _vector.clear();
    for (auto &w : _nvect) {
        if (w == to) {
            if (w) {
                delete w;
            }
            if (_preserve_slots) {
                _vector.push_back(nullptr);
            }
            w = nullptr;
        } else {
            _vector.push_back(w);
        }
    }
    _updating = false;
}

void OriginalItemArrayHiddenParam::linked_delete(SPObject */*deleted*/, LPEItemRef* to)
{
    if (_updating) {
        return;
    }
    if (!to) {
        return;
    }
    remove_link(to);
    auto full = param_getSVGValue();
    param_write_to_repr(full.c_str());
}


void OriginalItemArrayHiddenParam::linked_changed(SPObject *old_obj, SPObject *new_obj, LPEItemRef* to)
{
    if (_updating) {
        return;
    }
    if (!to) {
        return;
    }
    to->linked_delete_connection.disconnect();
    to->linked_transformed_connection.disconnect();
    to->linked_modified_connection.disconnect();
    if (new_obj) {
        //param_effect->getLPEObj()->connectRelease(sigc::mem_fun(*this, &OriginalItemArrayHiddenParam::lpeobject_released));
        to->linked_delete_connection = new_obj->connectDelete(sigc::bind<LPEItemRef*>(sigc::mem_fun(*this, &OriginalItemArrayHiddenParam::linked_delete), to));
        to->linked_modified_connection = new_obj->connectModified(sigc::bind<LPEItemRef*>(sigc::mem_fun(*this, &OriginalItemArrayHiddenParam::linked_modified), to));
        SPItem *item = dynamic_cast<SPItem *>(new_obj);
        if (item) {
            to->linked_transformed_connection = item->connectTransformed(sigc::bind<LPEItemRef*>(sigc::mem_fun(*this, &OriginalItemArrayHiddenParam::linked_transformed), to));
        }
        linked_modified(new_obj, SP_OBJECT_MODIFIED_FLAG, to);
    } else {
        param_effect->getLPEObj()->requestModified(SP_OBJECT_MODIFIED_FLAG);
    }
}

void
OriginalItemArrayHiddenParam::linked_transformed(Geom::Affine const *rel_transf, SPItem *item, LPEItemRef* to)  
{
    param_effect->getLPEObj()->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void OriginalItemArrayHiddenParam::linked_modified(SPObject *linked_obj, guint flags, LPEItemRef* to)
{
    if (_updating) {
        return;
    }
    _updating = true;
    if (!to) {
        return;
    }
    if (param_effect->effectType() != SLICE) {
        param_effect->getLPEObj()->requestModified(SP_OBJECT_MODIFIED_FLAG);
    }
    _updating = false;
}

bool OriginalItemArrayHiddenParam::param_readSVGValue(const gchar* strvalue)
{
    if (strvalue) {
        _updating = true;
        while (!_vector.empty()) {
            if (!_vector.back()) {
                _vector.pop_back();
                continue;
            }
            LPEItemRef *w = _vector.back();
            if (w) {
                unlink(w);
            }
            _vector.pop_back();
            w = nullptr;
        }
        if (strvalue && strvalue != "") {
            gchar ** strarray = g_strsplit(strvalue, ";", 0);
            for (gchar ** iter = strarray; *iter != nullptr; iter++) {
                bool hrefset = false;
                if ((*iter)[0] == '#') {
                    std::string id(*iter+1);
                    SPObject *lpeobj = dynamic_cast<SPObject *>(param_effect->getLPEObj());
                    if (lpeobj) {
                        if (lpeobj->document->getObjectById(id) || param_effect->is_load) {
                            LPEItemRef* w = new LPEItemRef(lpeobj);
                            if (w) {
                                w->href = g_strdup(*iter);
                                w->attach(URI(w->href));
                                linked_changed(nullptr, w->getObject(), w);
                                _vector.push_back(w);
                                hrefset = true;
                            }
                        }
                    }
                } 
                if (!hrefset) {
                    _vector.push_back(nullptr);
                }
            }
            g_strfreev (strarray);
        }
        _updating = false;
        return true;
    }
    return false;
}

Glib::ustring
OriginalItemArrayHiddenParam::param_getSVGValue() const
{
    Inkscape::SVGOStringStream os;
    bool foundOne = false;
    for (auto iter : _vector) {
        if (foundOne) {
            os << ";";
        } else {
            foundOne = true;
        }
        os << (iter == nullptr ? "" : iter->href);
    }
    return os.str();
}

Glib::ustring
OriginalItemArrayHiddenParam::param_getDefaultSVGValue() const
{
    return "";
}

void OriginalItemArrayHiddenParam::update()
{
    for (auto & iter : _vector) {
        SPObject *linked_obj = nullptr;
        if (iter) {
            linked_obj = iter->getObject();
        }
        linked_modified(linked_obj, SP_OBJECT_MODIFIED_FLAG, iter);
    }
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
