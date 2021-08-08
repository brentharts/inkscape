#ifndef SP_PREVIEW_UTIL_H
#define SP_PREVIEW_UTIL_H

#include <glibmm/main.h>

#include "desktop.h"
#include "display/drawing.h"
#include "document.h"

namespace Inkscape {
namespace UI {
namespace PREVIEW {

// takes doc, drawing, icon, and icon name to produce pixels
guchar *sp_icon_doc_icon(SPDocument *doc, Inkscape::Drawing &drawing, const gchar *name, unsigned int psize,
                         unsigned &stride, Geom::OptRect *dboxIn = nullptr);

void overlayPixels(guchar *px, int width, int height, int stride, unsigned r, unsigned g, unsigned b);

} // namespace PREVIEW
} // namespace UI
} // namespace Inkscape

#endif