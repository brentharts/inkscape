// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/parameter/originalitemmaphidden.h"

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


OriginalItemMapHiddenParam::OriginalItemMapHiddenParam( const Glib::ustring& label,
        const Glib::ustring& tip,
        const Glib::ustring& key,
        Inkscape::UI::Widget::Registry* wr,
        Effect* effect,
        bool preserve_slots)
: Parameter(label, tip, key, wr, effect), 
        _map()
{    
    param_widget_is_visible(false);
}

OriginalItemMapHiddenParam::~OriginalItemMapHiddenParam()
{
    _updating = true;
    for (auto &iter : _map) {
        LPEItemRef *w = _map.second.second.back();
        if (w) {
            unlink(w);
            delete w;
        }
        w = nullptr;
    }
    _map.clear();
    _updating = false;
}

void OriginalItemMapHiddenParam::param_set_default()
{
    
}

Gtk::Widget *
OriginalItemMapHiddenParam::param_newWidget()
{
    return nullptr;
}


void
OriginalItemMapHiddenParam::link(Glib::ustring name, SPObject *obj, Glib::ustring previous)
{
    _updating = true;
    Glib::ustring itemid = "";
    if (obj && obj->getId()) {
        itemid = obj->getId();
        itemid.insert(itemid.begin(), '#');
    }
    bool inserted = false;
    bool foundOne = false;
    Inkscape::SVGOStringStream os;
    for (auto &iter : _map) {
        if (iter.first == previous) {
            if (foundOne) {
                os << ";";
            } else {
                foundOne = true;
            }
            os << name << | << itemid.c_str();
            inserted = true;
            if (iter.second) {
                LPEItemRef *w = iter.second;
                if (w) {
                    unlink(w);
                    delete w;
                }
                w = nullptr;
                iter.second = nullptr;
            }
        } else {
            if (foundOne) {
                os << ";";
            } else {
                foundOne = true;
            }
            os << name << | <<  (iter == nullptr ? "" : iter.second.ref->href);
        }
        counter++;
    }

    if (!inserted) {
        if (foundOne) {
            os << ";";
        }
        os << name << | << itemid.c_str();
    }
    _map.clear();
    param_readSVGValue(val.c_str());
    param_write_to_repr(val.c_str());
    _updating = false;
}

void
OriginalItemMapHiddenParam::multilink(std::map<Glib::ustring,LPEItemRef*> objs)
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
        if (obj.second && obj.second->getId()) {
            itemid = obj.second->getId();
            itemid.insert(itemid.begin(), '#');
        }
        if (foundOne) {
            os << ";";
        } else {
            foundOne = true;
        }
        os << obj.first << | << itemid;
    }
    _map.clear();
    param_readSVGValue(val.c_str());
    param_write_to_repr(val.c_str());
    _updating = false;
}

void
OriginalItemMapHiddenParam::remove_link(Glib::ustring name)
{
    if (!obj) {
        return;
    }
    LPEItemRef *refitem = _map.find(name);
    if (refitem) {
        unlink(refitem);
        refitem = nullptr;
        _map.erase(name)
    }
}

void
OriginalItemMapHiddenParam::unlink(SPObject *obj)
{
    if (!obj) {
        return;
    }
    LPEItemRef *refitem = _map.find(name);
    if (refitem) {
        refitem->linked_delete_connection.disconnect();
        refitem->linked_modified_connection.disconnect();
        refitem->linked_transformed_connection.disconnect();
        refitem->linked_changed_connection.disconnect();
        refitem->ref.detach();
        if (refitem->href) {
            g_free(to->href);
            to->href = nullptr;
        }    
    
}

void OriginalItemMapHiddenParam::unlink(LPEItemRef* to)
{
    if (!to) {
        return;
    }
    to->linked_delete_connection.disconnect();
    to->linked_modified_connection.disconnect();
    to->linked_transformed_connection.disconnect();
    to->linked_changed_connection.disconnect();
    to->ref.detach();
    if (to->href) {
        g_free(to->href);
        to->href = nullptr;
    }    
}

void OriginalItemMapHiddenParam::clear()
{
    _updating = true;
    for (std::vector<LPEItemRef*>::reverse_iterator iter = _map.second.rbegin(); iter != _map.second.rend(); ++iter) {
        LPEItemRef *w = *iter;
        unlink(w);
        delete w;
        w = nullptr;
    }
    _map.clear();
    _updating = false;
}

void OriginalItemMapHiddenParam::remove_link(LPEItemRef* to)
{
    if (!to) {
        return;
    }
    _updating = true;
    std::vector<LPEItemRef*> _nvect = _map;
    _map.clear();
    for (auto &w : _nvect) {
        if (w == to) {
            if (w) {
                unlink(w);
                delete w;
            }
            if (!_preserve_slots) {
                _map.second.push_back(nullptr);
            }
            w = nullptr;
        } else {
            _map.second.push_back(w);
        }
    }
    _updating = false;
}

void OriginalItemMapHiddenParam::linked_delete(SPObject */*deleted*/, LPEItemRef* to)
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

void OriginalItemMapHiddenParam::linked_changed(SPObject */*old_obj*/, SPObject *new_obj, LPEItemRef* to)
{
    if (!to) {
        return;
    }
    to->linked_delete_connection.disconnect();
    to->linked_transformed_connection.disconnect();
    to->linked_modified_connection.disconnect();
    if (new_obj) {
        //to->linked_release_connection = new_obj->connectRelease(sigc::bind<1>(sigc::ptr_fun(*this, &OriginalItemArrayHiddenParam::linked_released), to));
        to->linked_delete_connection = new_obj->connectDelete(sigc::bind<LPEItemRef*>(sigc::mem_fun(*this, &OriginalItemMapHiddenParam::linked_delete), to));
        to->linked_modified_connection = new_obj->connectModified(sigc::bind<LPEItemRef*>(sigc::mem_fun(*this, &OriginalItemMapHiddenParam::linked_modified), to));
        SPItem *item = dynamic_cast<SPItem *>(new_obj);
        if (item) {
            to->linked_transformed_connection = item->connectTransformed(sigc::bind<LPEItemRef*>(sigc::mem_fun(*this, &OriginalItemMapHiddenParam::linked_transformed), to));
        }
        linked_modified(new_obj, SP_OBJECT_MODIFIED_FLAG, to);
    } else {
        SP_OBJECT(param_effect->getLPEObj())->requestModified(SP_OBJECT_MODIFIED_FLAG);
    }
}

void
OriginalItemMapHiddenParam::linked_transformed(Geom::Affine const *rel_transf, SPItem *item, LPEItemRef* to)  
{
    SP_OBJECT(param_effect->getLPEObj())->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void OriginalItemMapHiddenParam::linked_modified(SPObject *linked_obj, guint flags, LPEItemRef* to)
{
    if (_updating) {
        return;
    }
    _updating = true;
    if (!to) {
        return;
    }
    if (param_effect->effectType() != SLICE) {
        SP_OBJECT(param_effect->getLPEObj())->requestModified(SP_OBJECT_MODIFIED_FLAG);
    }
    _updating = false;
}

bool OriginalItemMapHiddenParam::param_readSVGValue(const gchar* strvalue) {
    if (strvalue) {
        _updating = true;
        while (!_map.empty()) {
            if (!_map.second.back()) {
                _map.second.pop_back();
                continue;
            }
            LPEItemRef *w = _map.second.back();
            if (w) {
                unlink(w);
                delete w;
            }
            _map.second.pop_back();
            w = nullptr;
        }
        gchar ** strarray = g_strsplit(strvalue, ";", 0);
        for (gchar ** iter = strarray; *iter != nullptr; iter++) {
            if ((*iter)[0] == '#') {
                LPEItemRef* w = new LPEItemRef((SPObject *)param_effect->getLPEObj());
                std::string id(*iter+1);
                if (w && (!_check_ids || param_effect->getLPEObj()->document->getObjectById(id))) {
                    w->href = g_strdup(*iter);
                    w->ref.attach(URI(w->href));
                    linked_changed(nullptr, w->ref.getObject(), w);
                    _map.second.push_back(w);
                } else {
                    _map.second.push_back(nullptr);
                }
            } else {
                _map.second.push_back(nullptr);
            }
        }
        g_strfreev (strarray);
        _updating = false;
        return true;
    }
    _updating = false;
    return false;
}

Glib::ustring
OriginalItemMapHiddenParam::param_getSVGValue() const
{
    Inkscape::SVGOStringStream os;
    bool foundOne = false;
    for (auto iter : _map) {
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
OriginalItemMapHiddenParam::param_getDefaultSVGValue() const
{
    return "";
}

void OriginalItemMapHiddenParam::update()
{
    for (auto & iter : _map) {
        SPObject *linked_obj = nullptr;
        if (iter) {
            linked_obj = iter->ref.getObject();
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
