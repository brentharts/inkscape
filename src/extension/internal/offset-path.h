/// SPDX-License-Identifier: GPL-2.0-or-later
/// @auth:sabagara <sabagara@caret-works.net>
/// @brief Size Specification Offset Extension

#include "extension/implementation/implementation.h"

namespace Inkscape {
namespace Extension {

class Effect;
class Extension;

namespace Internal {

class OffsetPath : public Inkscape::Extension::Implementation::Implementation {
public:
    /// Load Extension
    bool load(Inkscape::Extension::Extension *module) override;

    /// Exec Offset
    void effect(Inkscape::Extension::Effect *module, Inkscape::UI::View::View *document, Inkscape::Extension::Implementation::ImplementationDocumentCache *docCache) override;

    /// Extension Setting
    Gtk::Widget *prefs_effect(Inkscape::Extension::Effect *module, Inkscape::UI::View::View *view, sigc::signal<void> *changeSignal, Inkscape::Extension::Implementation::ImplementationDocumentCache *docCache) override;

    static void init();
};

}; // namespace Internal
}; // namespace Extension
}; // namespace Inkscape

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
