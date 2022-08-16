#include "font-tags.h"
#include <glibmm/i18n.h>

namespace Inkscape {

FontTags::FontTags() {}

FontTags& FontTags::get() {
    //todo
    static bool init = true;
    static FontTags ft;
    if (init) {
        init = false;
        ft.add_tag(FontTag{"favorites",  _("Favorites")});
        ft.add_tag(FontTag{"sans",       _("Sans Serif")});
        ft.add_tag(FontTag{"serif",      _("Serif")});
        ft.add_tag(FontTag{"script",     _("Script")});
        ft.add_tag(FontTag{"decorative", _("Decorative")});
        ft.add_tag(FontTag{"symbols",    _("Symbols")});
        ft.add_tag(FontTag{"monospace",  _("Monospace")});
        ft.add_tag(FontTag{"variable",   _("Variable")});
        ft.add_tag(FontTag{"oblique",    _("Oblique")});
    }
    return ft;
}

std::vector<FontTag> FontTags::get_tags() const {
    //
    return _tags;
    // return std::vector<FontTag>();
}

void FontTags::add_tag(const FontTag& tag) {
    _tags.push_back(tag);
}

std::set<std::string> FontTags::get_font_tags(Glib::RefPtr<Pango::FontFace>& face) const {
    auto it = _map.find(face);
    if (it != _map.end()) {
        return it->second;
    }
    return std::set<std::string>();
}

void FontTags::tag_font(Glib::RefPtr<Pango::FontFace>& face, std::string tag) {
    g_assert(find_tag(tag));

    _map[face].insert(tag);
}

const std::vector<FontTag>& FontTags::get_selected_tags() const {
    return _selected;
}

const FontTag* FontTags::find_tag(const std::string& tag_id) const {
    auto it = std::find_if(begin(_tags), end(_tags), [&](const FontTag& ft){ return ft.tag == tag_id; });
    if (it != end(_tags)) {
        return &(*it);
    }
    return nullptr;
}

bool FontTags::is_tag_selected(const std::string& tag_id) const {
    auto it = std::find_if(begin(_selected), end(_selected), [&](const FontTag& ft){ return ft.tag == tag_id; });
    return it != end(_selected);
}

bool FontTags::deselect_all() {
    if (!_selected.empty()) {
        _selected.clear();
        _signal_tag_changed.emit(nullptr, false);
        return true;
    }
    return false;
}

bool FontTags::select_tag(const std::string& tag_id, bool selected) {
    bool modified = false;

    const FontTag* tag = find_tag(tag_id);
    if (!tag) return modified;

    auto it = std::find(begin(_selected), end(_selected), *tag);
    if (it != _selected.end()) {
        // it is selected already

        if (!selected) {
            _selected.erase(it);
            modified = true;
        }
    }
    else {
        // not selected yet

        if (selected) {
            _selected.push_back(*tag);
            modified = true;
        }
    }

    if (modified) {
        _signal_tag_changed.emit(tag, selected);
    }

    return modified;
}

sigc::signal<void (const FontTag*, bool)>& FontTags::get_signal_tag_changed() {
    return _signal_tag_changed;
}

} // namespace
