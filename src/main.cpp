#define __MAIN_C__

/** \file
 * Inkscape - an ambitious vector drawing program
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   Davide Puricelli <evo@debian.org>
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Masatake YAMATO  <jet@gyve.org>
 *   F.J.Franklin <F.J.Franklin@sheffield.ac.uk>
 *   Michael Meeks <michael@helixcode.com>
 *   Chema Celorio <chema@celorio.com>
 *   Pawel Palucha
 *   Bryce Harrington <bryce@bryceharrington.com>
 * ... and various people who have worked with various projects
 *
 * Copyright (C) 1999-2004 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

// Putting the following in main.cpp appears a natural choice.

/** \mainpage The Inkscape Source Code Documentation
 * While the standard doxygen documentation can be accessed through the links
 * in the header, the following documents are additionally available to the 
 * interested reader.
 * 
 * \section groups Main directory documentation
 * Inkscape's classes and files in the main directory can be grouped into 
 * the following categories:
 * 
 * - \subpage ObjectTree - inkscape's SVG canvas
 * - \subpage Tools - the tools UI
 * - \subpage UI - inkscape's user interface
 * - \subpage XmlTree - XML backbone of the document
 * - \subpage Rendering - rendering and buffering
 * - \subpage OtherServices - what doesn't fit in the above
 *
 * See also the <a href="dirs.html">other directories</a> until doxygen
 * allows setting links to those doc files.
 * 
 * \section extlinks Links to external documentation
 *
 * \subsection liblinks External documentation on libraries used in inkscape
 *
 * <a href="http://www.gtkmm.org/gtkmm2/docs/">Gtkmm</a>
 * <a href="http://www.gtkmm.org/gtkmm2/docs/reference/html/dir_000003.html">atkmm</a>
 * <a href="http://www.gtkmm.org/gtkmm2/docs/reference/html/dir_000009.html">gdkmm</a>
 * <a href="http://www.gtkmm.org/gtkmm2/docs/reference/html/dir_000007.html">pangomm</a>
 * <a href="http://libsigc.sourceforge.net/libsigc1_2/reference/html/modules.html">libsigc++</a>
 * <a href="http://www.gtk.org/api/">GTK+</a>
 * <a href="http://developer.gnome.org/doc/API/2.0/gdk-pixbuf/index.html">gdk-pixbuf</a>
 * <a href="http://developer.gnome.org/doc/API/2.0/gobject/index.html">GObject</a>
 * <a href="http://developer.gnome.org/doc/API/2.0/atk/index.html">atk</a>
 * <a href="http://developer.gnome.org/doc/API/2.0/pango/index.html">pango</a>
 * <a href="http://developer.gnome.org/doc/API/2.0/gnome-vfs-2.0/">GnomeVFS</a>
 * <a href="http://libsigc.sourceforge.net/libsigc2/docs/index.html">libsigc</a>
 * <a href="http://developer.gnome.org/doc/API/2.0/ORBit/index.html">ORBit</a>
 * <a href="http://developer.gnome.org/doc/API/2.0/libbonobo/index.html">bonobo</a>
 * <a href="http://developer.gnome.org/doc/API/2.0/bonobo-activation/index.html">bonobo-activation</a>
 * <a href="http://xmlsoft.org/XSLT/html/libxslt-lib.html#LIBXSLT-LIB">libxslt</a>
 * <a href="http://xmlsoft.org/html/index.html">libxml2</a>
 *
 * \subsection stdlinks External standards documentation 
 *
 * <a href="http://www.w3.org/TR/SVG/">SVG1.1</a>
 * <a href="http://www.w3.org/TR/SVG12/">SVG1.2</a>
 * <a href="http://www.w3.org/TR/SVGMobile/">SVGMobile</a>
 * <a href="http://www.w3.org/Graphics/SVG/Test/">SVGTest</a>
 * <a href="http://www.libpng.org/pub/png/">PNG</a>
 * <a href="http://www.w3.org/TR/xslt">XSLT</a>
 * <a href="http://partners.adobe.com/public/developer/ps/index_specs.html">PS</a>
 * <a href="http://developer.gnome.org/projects/gup/hig/">Gnome-HIG</a>
 */

/** \page ObjectTree Object Tree Classes and Files
 * Inkscape::ObjectHierarchy [\ref object-hierarchy.cpp, \ref object-hierarchy.h]
 * - SPObject [\ref sp-object.cpp, \ref sp-object.h, \ref object-edit.cpp, \ref sp-object-repr.cpp]
 *   - SPDefs [\ref sp-defs.cpp, \ref sp-defs.h]
 *   - SPFlowline [\ref sp-flowdiv.cpp, \ref sp-flowdiv.h]
 *   - SPFlowregionbreak [\ref sp-flowdiv.cpp, \ref sp-flowdiv.h]
 *   - SPGuide [\ref sp-guide.cpp, \ref sp-guide.h]
 *   - SPItem [\ref sp-item.cpp, \ref sp-item.h, \ref sp-item-notify-moveto.cpp, \ref sp-item-rm-unsatisfied-cns.cpp, \ref sp-item-transform.cpp, \ref sp-item-update-cns.cpp, ]
 *     - SPFlowdiv [\ref sp-flowdiv.cpp, \ref sp-flowdiv.h]
 *     - SPFlowpara [\ref sp-flowdiv.cpp, \ref sp-flowdiv.h]
 *     - SPFlowregion [\ref sp-flowregion.cpp, \ref sp-flowregion.h]
 *     - SPFlowregionExclude [\ref sp-flowregion.cpp, \ref sp-flowregion.h]
 *     - SPFlowtext [\ref sp-flowtext.cpp, \ref sp-flowtext.h]
 *     - SPFlowtspan [\ref sp-flowdiv.cpp, \ref sp-flowdiv.h]
 *     - SPGroup [\ref sp-item-group.cpp, \ref sp-item-group.h]
 *       - SPAnchor [\ref sp-anchor.cpp, \ref sp-anchor.h]
 *       - SPMarker [\ref sp-marker.cpp, \ref sp-marker.h]
 *       - SPRoot [\ref sp-root.cpp, \ref sp-root.h]
 *       - SPSymbol [\ref sp-symbol.cpp, \ref sp-symbol.h]
 *     - SPImage [\ref sp-image.cpp, \ref sp-image.h]
 *     - SPShape [\ref sp-shape.cpp, \ref sp-shape.h, \ref marker-status.cpp]
 *       - SPGenericEllipse [\ref sp-ellipse.cpp, \ref sp-ellipse.h]
 *         - SPArc
 *         - SPCircle
 *         - SPEllipse
 *       - SPLine [\ref sp-line.cpp, \ref sp-line.h]
 *       - SPOffset [\ref sp-offset.cpp, \ref sp-offset.h]
 *       - SPPath [\ref sp-path.cpp, \ref sp-path.h, \ref path-chemistry.cpp, \ref nodepath.cpp, \ref nodepath.h, \ref splivarot.cpp]
 *       - SPPolygon [\ref sp-polygon.cpp, \ref sp-polygon.h]
 *         - SPStar [\ref sp-star.cpp, \ref sp-star.h]
 *       - SPPolyLine [\ref sp-polyline.cpp, \ref sp-polyline.h]
 *       - SPRect [\ref sp-rect.cpp, \ref sp-rect.h]
 *       - SPSpiral [\ref sp-spiral.cpp, \ref sp-spiral.h]
 *     - SPText [\ref sp-text.cpp, \ref sp-text.h, \ref text-chemistry.cpp, \ref text-editing.cpp]
 *     - SPTextPath [\ref sp-tspan.cpp, \ref sp-tspan.h]
 *     - SPTSpan [\ref sp-tspan.cpp, \ref sp-tspan.h]
 *     - SPUse [\ref sp-use.cpp, \ref sp-use.h]
 *   - SPMetadata [\ref sp-metadata.cpp, \ref sp-metadata.h]
 *   - SPObjectGroup [\ref sp-object-group.cpp, \ref sp-object-group.h]
 *     - SPClipPath [\ref sp-clippath.cpp, \ref sp-clippath.h]
 *     - SPMask [\ref sp-mask.cpp, \ref sp-mask.h]
 *     - SPNamedView [\ref sp-namedview.cpp, \ref sp-namedview.h]
 *   - SPPaintServer [\ref sp-paint-server.cpp, \ref sp-paint-server.h]
 *     - SPGradient [\ref sp-gradient.cpp, \ref sp-gradient.h, \ref gradient-chemistry.cpp, \ref sp-gradient-reference.h, \ref sp-gradient-spread.h, \ref sp-gradient-units.h, \ref sp-gradient-vector.h]
 *       - SPLinearGradient
 *       - SPRadialGradient
 *     - SPPattern [\ref sp-pattern.cpp, \ref sp-pattern.h]
 *   - SPSkeleton [\ref sp-skeleton.cpp, \ref sp-skeleton.h]
 *   - SPStop [\ref sp-stop.h]
 *   - SPString [\ref sp-string.cpp, \ref sp-string.h]
 *   - SPStyleElem [\ref sp-style-elem.cpp, \ref sp-style-elem.h]
 *
 */
/** \page Tools Tools Related Classes and Files
 * 
 * SPSelCue [\ref selcue.cpp, \ref selcue.h, \ref rubberband.cpp]
 * Inkscape::Selection [\ref selection.cpp, \ref selection.h, \ref selection-chemistry.cpp]
 * SPSelTrans [\ref seltrans.cpp, \ref seltrans.h]
 * 
 * \section Event Context Class Hierarchy
 * 
 *- SPEventContext[\ref event-context.cpp, \ref event-context.h]
 * - SPArcContext [\ref arc-context.cpp, \ref arc-context.h]
 * - SPDrawContext [\ref draw-context.cpp, \ref draw-context.h]
 *   - SPPenContext [\ref pen-context.cpp, \ref pen-context.h]
 *   - SPPencilContext [\ref pencil-context.cpp, \ref pencil-context.h]
 *   - SPConnectorContext [\ref connector-context.cpp, \ref connector-context.h, \ref sp-conn-end.cpp, \ref sp-conn-end-pair.cpp]
 * - SPGradientContext [\ref gradient-context.cpp, \ref gradient-context.h, \ref gradient-drag.cpp, \ref gradient-toolbar.cpp]
 * - SPRectContext [\ref rect-context.cpp, \ref rect-context.h]
 * - SPSelectContext [\ref select-context.cpp, \ref select-context.h]
 * - SPSpiralContext [\ref spiral-context.cpp, \ref spiral-context.h]
 * - SPStarContext [\ref star-context.cpp, \ref star-context.h]
 *   
 * SPNodeContext [\ref node-context.cpp, \ref node-context.h]
 * 
 * SPZoomContext [\ref zoom-context.cpp, \ref zoom-context.h]
 * 
 * SPDynaDrawContext [\ref dyna-draw-context.cpp, \ref dyna-draw-context.h] 
 * 
 * SPDropperContext [\ref dropper-context.cpp, \ref dropper-context.h] 
 */
/** \page UI User Interface Classes and Files
 *
 * - Inkscape::UI::View::View [\ref ui/view/view.cpp, \ref ui/view/view.h]
 *   - Inkscape::UI::View::Edit [\ref ui/view/edit.cpp, \ref ui/view/edit.h]
 *   - SPDesktop [\ref desktop.cpp, \ref desktop-affine.cpp, \ref desktop-events.cpp, \ref desktop-handles.cpp, \ref desktop-style.cpp, \ref desktop.h, \ref desktop-affine.h, \ref desktop-events.h, \ref desktop-handles.h, \ref desktop-style.h]
 *   - SPSVGView [\ref svg-view.cpp, \ref svg-view.h]
 *
 * SPDesktopWidget [\ref desktop-widget.h] SPSVGSPViewWidget [\ref svg-view.cpp]
 * SPDocument [\ref document.cpp, \ref document.h] 
 * 
 * SPDrawAnchor [\ref draw-anchor.cpp, \ref draw-anchor.h] 
 * SPKnot [\ref knot.cpp, \ref knot.h, \ref knot-enums.h]
 * SPKnotHolder [\ref knotholder.cpp, \ref knotholder.h, \ref knot-holder-entity.h]
 *
 * [\ref layer-fns.cpp, \ref selection-describer.h]
 * Inkscape::MessageContext [\ref message-context.h] 
 * Inkscape::MessageStack [\ref message-stack.h, \ref message.h] 
 *
 * Snapper, GridSnapper, GuideSnapper [\ref snap.cpp, \ref snap.h]
 *
 * SPGuide [\ref sp-guide.cpp, \ref sp-guide.h, \ref satisfied-guide-cns.cpp, \ref sp-guide-attachment.h, \ref sp-guide-constraint.h]
 * 
 * [\ref help.cpp] [\ref inkscape.cpp] [\ref inkscape-stock.cpp]
 * [\ref interface.cpp, \ref memeq.h] [\ref main.cpp, \ref winmain.cpp] 
 * [\ref menus-skeleton.h, \ref preferences-skeleton.h]
 * [\ref object-ui.cpp] [\ref select-toolbar.cpp] [\ref shortcuts.cpp]
 * [\ref sp-cursor.cpp] [\ref text-edit.cpp] [\ref toolbox.cpp, \ref ui/widget/toolbox.cpp] 
 * Inkscape::Verb [\ref verbs.h]
 *
 */
/** \page XmlTree CSS/XML Tree Classes and Files
 *
 * SPStyle [\ref style.cpp, \ref style.h]
 * Media [\ref media.cpp, \ref media.h]
 * [\ref attributes.cpp, \ref attributes.h] 
 *
 * - Inkscape::URIReference [\ref uri-references.cpp, \ref uri-references.h]
 *   - SPClipPathReference [\ref sp-clippath.h]
 *   - SPGradientReference [\ref sp-gradient-reference.h]
 *   - SPMarkerReference [\ref sp-marker.h]
 *   - SPMaskReference [\ref sp-mask.h]
 *   - SPUseReference [\ref sp-use-reference.h]
 *     - SPUsePath
 */
/** \page Rendering Rendering Related Classes and Files
 *
 * SPColor [\ref color.cpp, \ref color.h, \ref color-rgba.h] 
 * [\ref geom.cpp] [\ref isnan.h] [\ref mod360.cpp]
 */
/** \page OtherServices Classes and Files From Other Services
 * [\ref inkview.cpp, \ref slideshow.cpp] [\ref sp-animation.cpp]
 * 
 * Inkscape::GC
 *
 * [\ref sp-metrics.cpp, \ref sp-metrics.h]
 * 
 * [\ref prefs-utils.cpp] [\ref print.cpp]
 *
 * - Inkscape::GZipBuffer [\ref streams-gzip.h]
 * - Inkscape::JarBuffer [\ref streams-jar.h]
 * - Inkscape::ZlibBuffer [\ref streams-zlib.h]
 * - Inkscape::URIHandle [\ref streams-handles.h]
 *   - Inkscape::FileHandle
 * [\ref dir-util.cpp] [\ref file.cpp] 
 * Inkscape::URI [\ref uri.h, \ref extract-uri.cpp, \ref uri-references.cpp]
 * Inkscape::BadURIException [\ref bad-uri-exception.h] 
 * 
 * Inkscape::Whiteboard::UndoStackObserver [\ref undo-stack-observer.cpp, \ref composite-undo-stack-observer.cpp]
 * [\ref document-undo.cpp]
 * 
 * {\ref dialogs/} [\ref approx-equal.h] [\ref decimal-round.h] [\ref enums.h] [\ref unit-constants.h]
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "path-prefix.h"

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#include <string.h>
#include <locale.h>

#include <popt.h>
#ifndef POPT_TABLEEND
#define POPT_TABLEEND { NULL, '\0', 0, 0, 0, NULL, NULL }
#endif /* Not def: POPT_TABLEEND */

#include <libxml/tree.h>
#include <glib-object.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkbox.h>

#include <gtk/gtkmain.h>

#include "gc-core.h"

#include "macros.h"
#include "file.h"
#include "document.h"
#include "sp-object.h"
#include "interface.h"
#include "print.h"
#include "slideshow.h"
#include "color.h"
#include "sp-item.h"
#include "sp-root.h"
#include "unit-constants.h"

#include "svg/svg.h"
#include "svg/stringstream.h"

#include "inkscape-private.h"
#include "inkscape-stock.h"
#include "inkscape_version.h"

#include "sp-namedview.h"
#include "sp-guide.h"
#include "sp-object-repr.h"
#include "xml/repr.h"

#include "io/sys.h"

#include "debug/logger.h"

#include <extension/extension.h>
#include <extension/system.h>
#include <extension/db.h>
#include <extension/output.h>

#ifdef WIN32
#include "extension/internal/win32.h"
#endif
#include "extension/init.h"

#include <glibmm/i18n.h>
#include <gtkmm/main.h>

#ifndef HAVE_BIND_TEXTDOMAIN_CODESET
#define bind_textdomain_codeset(p,c)
#endif

#include "application/application.h"

enum {
    SP_ARG_NONE,
    SP_ARG_NOGUI,
    SP_ARG_GUI,
    SP_ARG_FILE,
    SP_ARG_PRINT,
    SP_ARG_EXPORT_PNG,
    SP_ARG_EXPORT_DPI,
    SP_ARG_EXPORT_AREA,
    SP_ARG_EXPORT_AREA_DRAWING,
    SP_ARG_EXPORT_AREA_SNAP,
    SP_ARG_EXPORT_WIDTH,
    SP_ARG_EXPORT_HEIGHT,
    SP_ARG_EXPORT_ID,
    SP_ARG_EXPORT_ID_ONLY,
    SP_ARG_EXPORT_USE_HINTS,
    SP_ARG_EXPORT_BACKGROUND,
    SP_ARG_EXPORT_BACKGROUND_OPACITY,
    SP_ARG_EXPORT_SVG,
    SP_ARG_EXPORT_PS,
    SP_ARG_EXPORT_EPS,
    SP_ARG_EXPORT_TEXT_TO_PATH,
    SP_ARG_EXPORT_BBOX_PAGE,
    SP_ARG_EXTENSIONDIR,
    SP_ARG_SLIDESHOW,
    SP_ARG_QUERY_X,
    SP_ARG_QUERY_Y,
    SP_ARG_QUERY_WIDTH,
    SP_ARG_QUERY_HEIGHT,
    SP_ARG_QUERY_ID,
    SP_ARG_VERSION,
    SP_ARG_NEW_GUI,
    SP_ARG_VACUUM_DEFS,
    SP_ARG_LAST
};

int sp_main_gui(int argc, char const **argv);
int sp_main_console(int argc, char const **argv);
static void sp_do_export_png(SPDocument *doc);
static void do_export_ps(SPDocument* doc, gchar const* uri, char const *mime);
static void do_query_dimension (SPDocument *doc, bool extent, NR::Dim2 const axis, const gchar *id);


static gchar *sp_global_printer = NULL;
static gboolean sp_global_slideshow = FALSE;
static gchar *sp_export_png = NULL;
static gchar *sp_export_dpi = NULL;
static gchar *sp_export_area = NULL;
static gboolean sp_export_area_drawing = FALSE;
static gchar *sp_export_width = NULL;
static gchar *sp_export_height = NULL;
static gchar *sp_export_id = NULL;
static gchar *sp_export_background = NULL;
static gchar *sp_export_background_opacity = NULL;
static gboolean sp_export_area_snap = FALSE;
static gboolean sp_export_use_hints = FALSE;
static gboolean sp_export_id_only = FALSE;
static gchar *sp_export_svg = NULL;
static gchar *sp_export_ps = NULL;
static gchar *sp_export_eps = NULL;
static gboolean sp_export_text_to_path = FALSE;
static gboolean sp_export_bbox_page = FALSE;
static gboolean sp_query_x = FALSE;
static gboolean sp_query_y = FALSE;
static gboolean sp_query_width = FALSE;
static gboolean sp_query_height = FALSE;
static gchar *sp_query_id = NULL;
static int sp_new_gui = FALSE;
static gboolean sp_vacuum_defs = FALSE;

static gchar *sp_export_png_utf8 = NULL;
static gchar *sp_export_svg_utf8 = NULL;
static gchar *sp_global_printer_utf8 = NULL;


static GSList *sp_process_args(poptContext ctx);
struct poptOption options[] = {
    {"version", 'V', 
     POPT_ARG_NONE, NULL, SP_ARG_VERSION,
     N_("Print the Inkscape version number"), 
     NULL},

    {"without-gui", 'z', 
     POPT_ARG_NONE, NULL, SP_ARG_NOGUI,
     N_("Do not use X server (only process files from console)"),
     NULL},

    {"with-gui", 'g', 
     POPT_ARG_NONE, NULL, SP_ARG_GUI,
     N_("Try to use X server (even if $DISPLAY is not set)"),
     NULL},

    {"file", 'f', 
     POPT_ARG_STRING, NULL, SP_ARG_FILE,
     N_("Open specified document(s) (option string may be excluded)"),
     N_("FILENAME")},

    {"print", 'p', 
     POPT_ARG_STRING, &sp_global_printer, SP_ARG_PRINT,
     N_("Print document(s) to specified output file (use '| program' for pipe)"),
     N_("FILENAME")},

    {"export-png", 'e', 
     POPT_ARG_STRING, &sp_export_png, SP_ARG_EXPORT_PNG,
     N_("Export document to a PNG file"),
     N_("FILENAME")},

    {"export-dpi", 'd', 
     POPT_ARG_STRING, &sp_export_dpi, SP_ARG_EXPORT_DPI,
     N_("The resolution used for exporting SVG into bitmap (default 90)"),
     N_("DPI")},

    {"export-area", 'a', 
     POPT_ARG_STRING, &sp_export_area, SP_ARG_EXPORT_AREA,
     N_("Exported area in SVG user units (default is the canvas; 0,0 is lower-left corner)"),
     N_("x0:y0:x1:y1")},

    {"export-area-drawing", 'D', 
     POPT_ARG_NONE, &sp_export_area_drawing, SP_ARG_EXPORT_AREA_DRAWING,
     N_("Exported area is the entire drawing (not canvas)"),
     NULL},

    {"export-area-snap", 0,
     POPT_ARG_NONE, &sp_export_area_snap, SP_ARG_EXPORT_AREA_SNAP,
     N_("Snap the bitmap export area outwards to the nearest integer values (in SVG user units)"),
     NULL},

    {"export-width", 'w', 
     POPT_ARG_STRING, &sp_export_width, SP_ARG_EXPORT_WIDTH,
     N_("The width of exported bitmap in pixels (overrides export-dpi)"), 
     N_("WIDTH")},

    {"export-height", 'h', 
     POPT_ARG_STRING, &sp_export_height, SP_ARG_EXPORT_HEIGHT,
     N_("The height of exported bitmap in pixels (overrides export-dpi)"), 
     N_("HEIGHT")},

    {"export-id", 'i', 
     POPT_ARG_STRING, &sp_export_id, SP_ARG_EXPORT_ID,
     N_("The ID of the object to export (overrides export-area)"), 
     N_("ID")},

    {"export-id-only", 'j', 
     POPT_ARG_NONE, &sp_export_id_only, SP_ARG_EXPORT_ID_ONLY,
     // TRANSLATORS: this means: "Only export the object whose id is given in --export-id".
     //  See "man inkscape" for details.
     N_("Export just the object with export-id, hide all others (only with export-id)"), 
     NULL},

    {"export-use-hints", 't', 
     POPT_ARG_NONE, &sp_export_use_hints, SP_ARG_EXPORT_USE_HINTS,
     N_("Use stored filename and DPI hints when exporting (only with export-id)"), 
     NULL},

    {"export-background", 'b', 
     POPT_ARG_STRING, &sp_export_background, SP_ARG_EXPORT_BACKGROUND,
     N_("Background color of exported bitmap (any SVG-supported color string)"), 
     N_("COLOR")},

    {"export-background-opacity", 'y', 
     POPT_ARG_STRING, &sp_export_background_opacity, SP_ARG_EXPORT_BACKGROUND_OPACITY,
     N_("Background opacity of exported bitmap (either 0.0 to 1.0, or 1 to 255)"), 
     N_("VALUE")},

    {"export-plain-svg", 'l', 
     POPT_ARG_STRING, &sp_export_svg, SP_ARG_EXPORT_SVG,
     N_("Export document to plain SVG file (no sodipodi or inkscape namespaces)"), 
     N_("FILENAME")},

    {"export-ps", 'P',
     POPT_ARG_STRING, &sp_export_ps, SP_ARG_EXPORT_PS,
     N_("Export document to a PS file"),
     N_("FILENAME")},

    {"export-eps", 'E',
     POPT_ARG_STRING, &sp_export_eps, SP_ARG_EXPORT_EPS,
     N_("Export document to an EPS file"),
     N_("FILENAME")},

    {"export-text-to-path", 'T',
     POPT_ARG_NONE, &sp_export_text_to_path, SP_ARG_EXPORT_TEXT_TO_PATH,
     N_("Convert text object to paths on export (EPS)"),
     NULL},

    {"export-bbox-page", 'B',
     POPT_ARG_NONE, &sp_export_bbox_page, SP_ARG_EXPORT_BBOX_PAGE,
     N_("Export files with the bounding box set to the page size (EPS)"),
     NULL},

    {"query-x", 'X',
     POPT_ARG_NONE, &sp_query_x, SP_ARG_QUERY_X,
     // TRANSLATORS: "--query-id" is an Inkscape command line option; see "inkscape --help"
     N_("Query the X coordinate of the drawing or, if specified, of the object with --query-id"),
     NULL},

    {"query-y", 'Y',
     POPT_ARG_NONE, &sp_query_y, SP_ARG_QUERY_Y,
     // TRANSLATORS: "--query-id" is an Inkscape command line option; see "inkscape --help"
     N_("Query the Y coordinate of the drawing or, if specified, of the object with --query-id"),
     NULL},

    {"query-width", 'W',
     POPT_ARG_NONE, &sp_query_width, SP_ARG_QUERY_WIDTH,
     // TRANSLATORS: "--query-id" is an Inkscape command line option; see "inkscape --help"
     N_("Query the width of the drawing or, if specified, of the object with --query-id"),
     NULL},

    {"query-height", 'H',
     POPT_ARG_NONE, &sp_query_height, SP_ARG_QUERY_HEIGHT,
     // TRANSLATORS: "--query-id" is an Inkscape command line option; see "inkscape --help"
     N_("Query the height of the drawing or, if specified, of the object with --query-id"),
     NULL},

    {"query-id", 'I', 
     POPT_ARG_STRING, &sp_query_id, SP_ARG_QUERY_ID,
     N_("The ID of the object whose dimensions are queried"), 
     N_("ID")},

    {"extension-directory", 'x',
     POPT_ARG_NONE, NULL, SP_ARG_EXTENSIONDIR,
     // TRANSLATORS: this option makes Inkscape print the name (path) of the extension directory
     N_("Print out the extension directory and exit"),
     NULL},

    {"slideshow", 's', 
     POPT_ARG_NONE, &sp_global_slideshow, SP_ARG_SLIDESHOW,
     N_("Show given files one-by-one, switch to next on any key/mouse event"), 
     NULL},

    {"new-gui", 'G', 
     POPT_ARG_NONE, &sp_new_gui, SP_ARG_NEW_GUI,
     N_("Use the new Gtkmm GUI interface"),
     NULL},

    {"vacuum-defs", 0,
     POPT_ARG_NONE, &sp_vacuum_defs, SP_ARG_VACUUM_DEFS,
     N_("Remove unused definitions from the defs section(s) of the document"),
     NULL},

    POPT_AUTOHELP POPT_TABLEEND
};

int
main(int argc, char **argv)
{
#ifdef HAVE_FPSETMASK
    /* This is inherited from Sodipodi code, where it was in #ifdef __FreeBSD__.  It's probably
       safe to remove: the default mask is already 0 in C99, and in current FreeBSD according to
       the fenv man page on www.freebsd.org, and in glibc according to (libc)FP Exceptions. */
    fpsetmask(fpgetmask() & ~(FP_X_DZ | FP_X_INV));
#endif

#ifdef ENABLE_NLS
#ifdef WIN32
    gchar *pathBuf = g_strconcat(g_path_get_dirname(argv[0]), "\\", PACKAGE_LOCALE_DIR, NULL);
    bindtextdomain(GETTEXT_PACKAGE, pathBuf);
    g_free(pathBuf);
#else
#ifdef ENABLE_BINRELOC
    bindtextdomain(GETTEXT_PACKAGE, BR_LOCALEDIR(""));
#else
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
#endif
#endif
#endif

    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

#ifdef ENABLE_NLS
    textdomain(GETTEXT_PACKAGE);
#endif

    LIBXML_TEST_VERSION

    Inkscape::GC::init();

    Inkscape::Debug::Logger::init();

    gboolean use_gui;
#ifndef WIN32
    use_gui = (getenv("DISPLAY") != NULL);
#else
    /*
      Set the current directory to the directory of the
      executable.  This seems redundant, but is needed for
      when inkscape.exe is executed from another directory.
      We use relative paths on win32.
      HKCR\svgfile\shell\open\command is a good example
    */
    /// \todo FIXME BROKEN - non-UTF-8 sneaks in here.
    char *homedir = g_path_get_dirname(argv[0]);
    SetCurrentDirectory(homedir);
    g_free(homedir);

    use_gui = TRUE;
#endif
    /* Test whether with/without GUI is forced */
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-z")
            || !strcmp(argv[i], "--without-gui")
            || !strcmp(argv[i], "-p")
            || !strncmp(argv[i], "--print", 7)
            || !strcmp(argv[i], "-e")
            || !strncmp(argv[i], "--export-png", 12)
            || !strcmp(argv[i], "-l")
            || !strncmp(argv[i], "--export-plain-svg", 12)
            || !strcmp(argv[i], "-i")
            || !strncmp(argv[i], "--export-area-drawing", 21)
            || !strcmp(argv[i], "-D")
            || !strncmp(argv[i], "--export-id", 12)
            || !strcmp(argv[i], "-P")
            || !strncmp(argv[i], "--export-ps", 11)
            || !strcmp(argv[i], "-E")
            || !strncmp(argv[i], "--export-eps", 12)
            || !strcmp(argv[i], "-W")
            || !strncmp(argv[i], "--query-width", 13)
            || !strcmp(argv[i], "-H")
            || !strncmp(argv[i], "--query-height", 14)
            || !strcmp(argv[i], "-X")
            || !strncmp(argv[i], "--query-x", 13)
            || !strcmp(argv[i], "-Y")
            || !strncmp(argv[i], "--query-y", 14)
            || !strcmp(argv[i], "--vacuum-defs")
           )
        {
            /* main_console handles any exports -- not the gui */
            use_gui = FALSE;
            break;
        } else if (!strcmp(argv[i], "-g") || !strcmp(argv[i], "--with-gui")) {
            use_gui = TRUE;
            break;
        } else if (!strcmp(argv[i], "-G") || !strcmp(argv[i], "--new-gui")) {
            sp_new_gui = TRUE;
            break;
        }
    }

    /// \todo  Should this be a static object (see inkscape.cpp)?
    Inkscape::NSApplication::Application app(argc, argv, use_gui, sp_new_gui);

#ifdef WIN32
    {
#ifdef NONONONO
        MessageBoxA( NULL, "GetCommandLineW() getting called", "GetCommandLineW", MB_OK | MB_ICONINFORMATION );

        wchar_t* line = GetCommandLineW();
        if ( line )
        {
            gchar* blerb = g_utf16_to_utf8( (gunichar2*)line,
                                            -1,
                                            NULL,
                                            NULL,
                                            NULL );
            if ( blerb )
            {
                gchar *safe = Inkscape::IO::sanitizeString(blerb);
                {
                    char tmp[1024];
                    snprintf( tmp, sizeof(tmp), "GetCommandLineW() = '%s'", safe );
                    MessageBoxA( NULL, tmp, "GetCommandLineW", MB_OK | MB_ICONINFORMATION );
                }
            }
            int numArgs = 0;
            wchar_t** parsed = CommandLineToArgvW( line, &numArgs );
            if ( parsed )
            {
                for ( int i = 0; i < numArgs; i++ )
                {
                    gchar* replacement = g_utf16_to_utf8( (gunichar2*)parsed[i],
                                                          -1,
                                                          NULL,
                                                          NULL,
                                                          NULL );
                    if ( replacement )
                    {
                        gchar *safe2 = Inkscape::IO::sanitizeString(replacement);

                        if ( safe2 )
                        {
                            {
                                char tmp[1024];
                                snprintf( tmp, sizeof(tmp), "    [%2d] = '%s'", i, safe2 );
                                MessageBoxA( NULL, tmp, "GetCommandLineW", MB_OK | MB_ICONINFORMATION );
                            }
/*
                            GtkWidget *w = gtk_message_dialog_new( NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                                                   "     [%2d] = '%s'", i, safe2 );
                            gtk_dialog_run( GTK_DIALOG(w) );
                            gtk_widget_destroy(w);
*/

                            g_free( safe2 );
                        }
                        g_free( replacement );
                    }
                }
            }
            else
            {
                MessageBoxA( NULL, "Unable to process command-line", "CommandLineToArgvW", MB_OK | MB_ICONINFORMATION );
/*
                GtkWidget *w = gtk_message_dialog_new( NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                                       "Unable to fetch result from CommandLineToArgvW()" );
                gtk_dialog_run( GTK_DIALOG(w) );
                gtk_widget_destroy(w);
*/
            }
        }
        else
        {
            {
                MessageBoxA( NULL,  "Unable to fetch result from GetCommandLineW()", "GetCommandLineW", MB_OK | MB_ICONINFORMATION );
/*
                GtkWidget *w = gtk_message_dialog_new( NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                                       "Unable to fetch result from GetCommandLineW()" );
                gtk_dialog_run( GTK_DIALOG(w) );
                gtk_widget_destroy(w);
*/
            }

            char* line2 = GetCommandLineA();
            if ( line2 )
            {
                gchar *safe = Inkscape::IO::sanitizeString(line2);
                {
                    {
                        char tmp[1024];
                        snprintf( tmp, sizeof(tmp), "GetCommandLineA() = '%s'", safe );
                        MessageBoxA( NULL, tmp, "GetCommandLineA", MB_OK | MB_ICONINFORMATION );
                    }
/*
                    GtkWidget *w = gtk_message_dialog_new( NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                                           "GetCommandLineA() = '%s'", safe );
                    gtk_dialog_run( GTK_DIALOG(w) );
                    gtk_widget_destroy(w);
*/
                }
            }
            else
            {
                MessageBoxA( NULL, "Unable to fetch result from GetCommandLineA()", "GetCommandLineA", MB_OK | MB_ICONINFORMATION );
/*
                GtkWidget *w = gtk_message_dialog_new( NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                                       "Unable to fetch result from GetCommandLineA()" );
                gtk_dialog_run( GTK_DIALOG(w) );
                gtk_widget_destroy(w);
*/
            }
        }
#endif // NONONONO
    }
#endif // WIN32

    return app.run();
}

void fixupSingleFilename( gchar **orig, gchar **spare )
{
    if ( orig && *orig && **orig ) {
        GError *error = NULL;
        gchar *newFileName = Inkscape::IO::locale_to_utf8_fallback(*orig, -1, NULL, NULL, &error);
        if ( newFileName )
        {
            *orig = newFileName;
            if ( spare ) {
                *spare = newFileName;
            }
//             g_message("Set a replacement fixup");
        }
    }
}

GSList *fixupFilenameEncoding( GSList* fl )
{
    GSList *newFl = NULL;
    while ( fl ) {
        gchar *fn = static_cast<gchar*>(fl->data);
        fl = g_slist_remove( fl, fl->data );
        gchar *newFileName = Inkscape::IO::locale_to_utf8_fallback(fn, -1, NULL, NULL, NULL);
        if ( newFileName ) {

            if ( 0 )
            {
                gchar *safeFn = Inkscape::IO::sanitizeString(fn);
                gchar *safeNewFn = Inkscape::IO::sanitizeString(newFileName);
                GtkWidget *w = gtk_message_dialog_new( NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                                       "Note: Converted '%s' to '%s'", safeFn, safeNewFn );
                gtk_dialog_run (GTK_DIALOG (w));
                gtk_widget_destroy (w);
                g_free(safeNewFn);
                g_free(safeFn);
            }

            g_free( fn );
            fn = newFileName;
            newFileName = 0;
        }
        else
            if ( 0 )
        {
            gchar *safeFn = Inkscape::IO::sanitizeString(fn);
            GtkWidget *w = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "Error: Unable to convert '%s'", safeFn );
            gtk_dialog_run (GTK_DIALOG (w));
            gtk_widget_destroy (w);
            g_free(safeFn);
        }
        newFl = g_slist_append( newFl, fn );
    }
    return newFl;
}

int sp_common_main( int argc, char const **argv, GSList **flDest )
{
    /// \todo fixme: Move these to some centralized location (Lauris)
    sp_object_type_register("sodipodi:namedview", SP_TYPE_NAMEDVIEW);
    sp_object_type_register("sodipodi:guide", SP_TYPE_GUIDE);


    // temporarily switch gettext encoding to locale, so that help messages can be output properly
    gchar const *charset;
    g_get_charset(&charset);
#ifdef WIN32
#ifdef NONONONO
    {
        char tmp[1024];
        snprintf( tmp, sizeof(tmp), "Encoding is '%s'", charset );
        MessageBoxA( NULL, tmp, "g_get_charset", MB_OK | MB_ICONINFORMATION );
    }

//    checkEncoding();
#endif // NONONONO
#endif // WIN32
    bind_textdomain_codeset(GETTEXT_PACKAGE, charset);

//    checkEncoding();

    poptContext ctx = poptGetContext(NULL, argc, argv, options, 0);
    poptSetOtherOptionHelp(ctx, _("[OPTIONS...] [FILE...]\n\nAvailable options:"));
    g_return_val_if_fail(ctx != NULL, 1);

    /* Collect own arguments */
    GSList *fl = sp_process_args(ctx);
    poptFreeContext(ctx);

    // now switch gettext back to UTF-8 (for GUI)
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

    // Now let's see if the file list still holds up
    fl = fixupFilenameEncoding( fl );

    // Check the globals for filename-fixup
    fixupSingleFilename( &sp_export_png, &sp_export_png_utf8 );
    fixupSingleFilename( &sp_export_svg, &sp_export_svg_utf8 );
    fixupSingleFilename( &sp_global_printer, &sp_global_printer_utf8 );

    // Return the list if wanted, else free it up.
    if ( flDest ) {
        *flDest = fl;
        fl = 0;
    } else {
        while ( fl ) {
            g_free( fl->data );
            fl = g_slist_remove( fl, fl->data );
        }
    }
    return 0;
}

int
sp_main_gui(int argc, char const **argv)
{
    Gtk::Main main_instance (&argc, const_cast<char ***>(&argv));

    GSList *fl = NULL;
    int retVal = sp_common_main( argc, argv, &fl );
    g_return_val_if_fail(retVal == 0, 1);

    inkscape_gtk_stock_init();

    /* Set default icon */
    gchar *filename = (gchar *) g_build_filename (INKSCAPE_APPICONDIR, "inkscape.png", NULL);
    if (Inkscape::IO::file_test(filename, (GFileTest)(G_FILE_TEST_IS_REGULAR | G_FILE_TEST_IS_SYMLINK))) {
        gtk_window_set_default_icon_from_file(filename, NULL);
    }
    g_free (filename);
    filename = 0;

    if (!sp_global_slideshow) {
        gboolean create_new = TRUE;

        /// \todo FIXME BROKEN - non-UTF-8 sneaks in here.
        inkscape_application_init(argv[0], true);

        while (fl) {
            if (sp_file_open((gchar *)fl->data,NULL)) {
                create_new=FALSE;
            }
            fl = g_slist_remove(fl, fl->data);
        }
        if (create_new) {
            sp_file_new_default();
        }
    } else {
        if (fl) {
            GtkWidget *ss;
            /// \todo FIXME BROKEN - non-UTF-8 sneaks in here.
            inkscape_application_init(argv[0], true);
            ss = sp_slideshow_new(fl);
            if (ss) gtk_widget_show(ss);
        } else {
            g_warning ("No slides to display");
            exit(0);
        }
    }

    main_instance.run();

#ifdef WIN32
    //We might not need anything here
    //sp_win32_finish(); <-- this is a NOP func
#endif

    return 0;
}

int
sp_main_console(int argc, char const **argv)
{
    /* We are started in text mode */

    /* Do this g_type_init(), so that we can use Xft/Freetype2 (Pango)
     * in a non-Gtk environment.  Used in libnrtype's
     * FontInstance.cpp and FontFactory.cpp.
     * http://mail.gnome.org/archives/gtk-list/2003-December/msg00063.html
     */
    g_type_init();
    char **argv2 = const_cast<char **>(argv);
    gtk_init_check( &argc, &argv2 );
    //setlocale(LC_ALL, "");

    GSList *fl = NULL;
    int retVal = sp_common_main( argc, argv, &fl );
    g_return_val_if_fail(retVal == 0, 1);

    if (fl == NULL) {
        g_print("Nothing to do!\n");
        exit(0);
    }

    inkscape_application_init(argv[0], false);

    while (fl) {
        SPDocument *doc;

        doc = Inkscape::Extension::open(NULL, (gchar *)fl->data);
        if (doc == NULL) {
            doc = Inkscape::Extension::open(Inkscape::Extension::db.get(SP_MODULE_KEY_INPUT_SVG), (gchar *)fl->data);
        }
        if (doc == NULL) {
            g_warning("Specified document %s cannot be opened (is it valid SVG file?)", (gchar *) fl->data);
        } else {
            if (sp_vacuum_defs) {
                vacuum_document(doc);
            }
            if (sp_vacuum_defs && !sp_export_svg) {
                // save under the name given in the command line
                sp_repr_save_file(doc->rdoc, (gchar *)fl->data, SP_SVG_NS_URI);
            }
            if (sp_global_printer) {
                sp_print_document_to_file(doc, sp_global_printer);
            }
            if (sp_export_png || sp_export_id || sp_export_area_drawing) {
                sp_do_export_png(doc);
            }
            if (sp_export_svg) {
                Inkscape::XML::Document *rdoc;
                Inkscape::XML::Node *repr;
                rdoc = sp_repr_document_new("svg:svg");
                repr = rdoc->root();
                repr = sp_document_root(doc)->updateRepr(repr, SP_OBJECT_WRITE_BUILD);
                sp_repr_save_file(repr->document(), sp_export_svg, SP_SVG_NS_URI);
            }
            if (sp_export_ps) {
                do_export_ps(doc, sp_export_ps, "image/x-postscript");
            }
            if (sp_export_eps) {
                do_export_ps(doc, sp_export_eps, "image/x-e-postscript");
            }
            if (sp_query_width || sp_query_height) {
                do_query_dimension (doc, true, sp_query_width? NR::X : NR::Y, sp_query_id);
            } else if (sp_query_x || sp_query_y) {
                do_query_dimension (doc, false, sp_query_x? NR::X : NR::Y, sp_query_id);
            }
        }
        fl = g_slist_remove(fl, fl->data);
    }

    inkscape_unref();

    return 0;
}

static void
do_query_dimension (SPDocument *doc, bool extent, NR::Dim2 const axis, const gchar *id)
{
    SPObject *o = NULL;

    if (id) {
        o = doc->getObjectById(id);
        if (o) {
            if (!SP_IS_ITEM (o)) {
                g_warning("Object with id=\"%s\" is not a visible item. Cannot query dimensions.", id);
                return;
            }
        } else {
            g_warning("Object with id=\"%s\" is not found. Cannot query dimensions.", id);
            return;
        }
    } else {
        o = SP_DOCUMENT_ROOT(doc);
    }

    if (o) {
        sp_document_ensure_up_to_date (doc);
        NR::Rect area = sp_item_bbox_desktop((SPItem *) o);

        Inkscape::SVGOStringStream os;
        if (extent) {
            os << area.extent(axis);
        } else {
            os << area.min()[axis];
        }
        g_print ("%s\n", os.str().c_str());
    }
}


static void
sp_do_export_png(SPDocument *doc)
{
    const gchar *filename = NULL;
    gdouble dpi = 0.0;

    if (sp_export_use_hints && (!sp_export_id && !sp_export_area_drawing)) {
        g_warning ("--export-use-hints can only be used with --export-id or --export-area-drawing; ignored.");
    }

    GSList *items = NULL;

    NRRect area;
    if (sp_export_id || sp_export_area_drawing) {

        SPObject *o = NULL;
        if (sp_export_id) {
            o = doc->getObjectById(sp_export_id);
        } else if (sp_export_area_drawing) {
            o = SP_DOCUMENT_ROOT (doc);
        }

        if (o) {
            if (!SP_IS_ITEM (o)) {
                g_warning("Object with id=\"%s\" is not a visible item. Nothing exported.", sp_export_id);
                return;
            }
            if (sp_export_area) {
                g_warning ("Object with id=\"%s\" is being exported; --export-area is ignored.", sp_export_id);
            }

            items = g_slist_prepend (items, SP_ITEM(o));

            if (sp_export_id_only) {
                g_print("Exporting only object with id=\"%s\"; all other objects hidden\n", sp_export_id);
            }

            if (sp_export_use_hints) {

                // retrieve export filename hint
                const gchar *fn_hint = SP_OBJECT_REPR(o)->attribute("inkscape:export-filename");
                if (fn_hint) {
                    if (sp_export_png) {
                        g_warning ("Using export filename from the command line (--export-png). Filename hint %s is ignored.", fn_hint);
                        filename = sp_export_png;
                    } else {
                        filename = fn_hint;
                    }
                } else {
                    g_warning ("Export filename hint not found for the object.");
                    filename = sp_export_png;
                }

                // retrieve export dpi hints
                const gchar *dpi_hint = SP_OBJECT_REPR(o)->attribute("inkscape:export-xdpi"); // only xdpi, ydpi is always the same now
                if (dpi_hint) {
                    if (sp_export_dpi || sp_export_width || sp_export_height) {
                        g_warning ("Using bitmap dimensions from the command line (--export-dpi, --export-width, or --export-height). DPI hint %s is ignored.", dpi_hint);
                    } else {
                        dpi = atof(dpi_hint);
                    }
                } else {
                    g_warning ("Export DPI hint not found for the object.");
                }

            }

            // write object bbox to area
            sp_document_ensure_up_to_date (doc);
            sp_item_invoke_bbox((SPItem *) o, &area, sp_item_i2r_affine((SPItem *) o), TRUE);
        } else {
            g_warning("Object with id=\"%s\" was not found in the document. Nothing exported.", sp_export_id);
            return;
        }
    } else if (sp_export_area) {
        /* Try to parse area (given in SVG pixels) */
        if (!sscanf(sp_export_area, "%lg:%lg:%lg:%lg", &area.x0, &area.y0, &area.x1, &area.y1) == 4) {
            g_warning("Cannot parse export area '%s'; use 'x0:y0:x1:y1'. Nothing exported.", sp_export_area);
            return;
        }
        if ((area.x0 >= area.x1) || (area.y0 >= area.y1)) {
            g_warning("Export area '%s' has negative width or height. Nothing exported.", sp_export_area);
            return;
        }
    } else {
        /* Export the whole canvas */
        sp_document_ensure_up_to_date (doc);
        area.x0 = SP_ROOT(doc->root)->x.computed;
        area.y0 = SP_ROOT(doc->root)->y.computed;
        area.x1 = area.x0 + sp_document_width (doc);
        area.y1 = area.y0 + sp_document_height (doc);
    }

    // set filename and dpi from options, if not yet set from the hints
    if (!filename) {
        if (!sp_export_png) {
            g_warning ("No export filename given and no filename hint. Nothing exported.");
            return;
        }
        filename = sp_export_png;
    }

    if (sp_export_dpi && dpi == 0.0) {
        dpi = atof(sp_export_dpi);
        if ((dpi < 0.1) || (dpi > 10000.0)) {
            g_warning("DPI value %s out of range [0.1 - 10000.0]. Nothing exported.", sp_export_dpi);
            return;
        }
        g_print("DPI: %g\n", dpi);
    } 

    if (sp_export_area_snap) {
        area.x0 = std::floor (area.x0);
        area.y0 = std::floor (area.y0);
        area.x1 = std::ceil (area.x1);
        area.y1 = std::ceil (area.y1);
    }

    // default dpi
    if (dpi == 0.0)
        dpi = PX_PER_IN;

    gint width = 0;
    gint height = 0;

    if (sp_export_width) {
        width = atoi(sp_export_width);
        if ((width < 1) || (width > 65536)) {
            g_warning("Export width %d out of range (1 - 65536). Nothing exported.", width);
            return;
        }
        dpi = (gdouble) width * PX_PER_IN / (area.x1 - area.x0);
    }

    if (sp_export_height) {
        height = atoi(sp_export_height);
        if ((height < 1) || (height > 65536)) {
            g_warning("Export height %d out of range (1 - 65536). Nothing exported.", width);
            return;
        }
        dpi = (gdouble) height * PX_PER_IN / (area.y1 - area.y0);
    }

    if (!sp_export_width) {
        width = (gint) ((area.x1 - area.x0) * dpi / PX_PER_IN + 0.5);
    }

    if (!sp_export_height) {
        height = (gint) ((area.y1 - area.y0) * dpi / PX_PER_IN + 0.5);
    }

    guint32 bgcolor = 0x00000000;
    if (sp_export_background) {
        // override the page color
        bgcolor = sp_svg_read_color(sp_export_background, 0xffffff00);
        bgcolor |= 0xff; // default is no opacity
    } else {
        // read from namedview
        Inkscape::XML::Node *nv = sp_repr_lookup_name (doc->rroot, "sodipodi:namedview");
        if (nv && nv->attribute("pagecolor"))
            bgcolor = sp_svg_read_color(nv->attribute("pagecolor"), 0xffffff00);
        if (nv && nv->attribute("inkscape:pageopacity"))
            bgcolor |= SP_COLOR_F_TO_U(sp_repr_get_double_attribute (nv, "inkscape:pageopacity", 1.0));
    }

    if (sp_export_background_opacity) {
        // override opacity
        gfloat value;
        if (sp_svg_number_read_f (sp_export_background_opacity, &value)) {
            if (value > 1.0) {
                value = CLAMP (value, 1.0f, 255.0f);
                bgcolor &= (guint32) 0xffffff00;
                bgcolor |= (guint32) floor(value);
            } else {
                value = CLAMP (value, 0.0f, 1.0f);
                bgcolor &= (guint32) 0xffffff00;
                bgcolor |= SP_COLOR_F_TO_U(value);
            }
        }
    }

    g_print("Background RRGGBBAA: %08x\n", bgcolor);

    g_print("Area %g:%g:%g:%g exported to %d x %d pixels (%g dpi)\n", area.x0, area.y0, area.x1, area.y1, width, height, dpi);

    g_print("Bitmap saved as: %s\n", filename);
  
    if ((width >= 1) && (height >= 1) && (width < 65536) && (height < 65536)) {
        sp_export_png_file(doc, filename, area.x0, area.y0, area.x1, area.y1, width, height, bgcolor, NULL, NULL, true, sp_export_id_only ? items : NULL);
    } else {
        g_warning("Calculated bitmap dimensions %d %d are out of range (1 - 65535). Nothing exported.", width, height);
    }

    g_slist_free (items);
}


/**
 *  Perform an export of either PS or EPS.
 *
 *  \param doc Document to export.
 *  \param uri URI to export to.
 *  \param mime MIME type to export as.
 */

static void do_export_ps(SPDocument* doc, gchar const* uri, char const* mime)
{
    /** \todo
     * FIXME: I've no idea if this is the `proper' way to do this.
     * If anyone feels qualified to say that it is, perhaps they
     * could remove this comment.
     */

    Inkscape::Extension::DB::OutputList o;
    Inkscape::Extension::db.get_output_list(o);
    Inkscape::Extension::DB::OutputList::const_iterator i = o.begin();
    while (i != o.end() && strcmp( (*i)->get_mimetype(), mime ) != 0) {
        i++;
    }

    if (i == o.end())
    {
        g_warning ("Could not find an extension to export this file.");
        return;
    }

    bool old_text_to_path = false;
    bool old_bbox_page = false;
    
    try {
        old_text_to_path = (*i)->get_param_bool("textToPath");
        (*i)->set_param_bool("textToPath", sp_export_text_to_path);
    }
    catch (...) {
        g_warning ("Could not set export-text-to-path option for this export.");
    }

    try {
        old_bbox_page = (*i)->get_param_bool("pageBoundingBox");
        (*i)->set_param_bool("pageBoundingBox", sp_export_bbox_page);
    }
    catch (...) {
        g_warning ("Could not set export-bbox-page option for this export.");
    }
    
    (*i)->save(doc, uri);

    try {
        (*i)->set_param_bool("textToPath", old_text_to_path);
        (*i)->set_param_bool("pageBoundingBox", old_bbox_page);
    }
    catch (...) {
        
    }
}

static GSList *
sp_process_args(poptContext ctx)
{
    GSList *fl = NULL;

    gint a;
    while ((a = poptGetNextOpt(ctx)) >= 0) {
        switch (a) {
            case SP_ARG_FILE: {
                gchar const *fn = poptGetOptArg(ctx);
                if (fn != NULL) {
                    fl = g_slist_append(fl, g_strdup(fn));
                }
                break;
            }
            case SP_ARG_VERSION: {
                printf("Inkscape %s (%s)\n", INKSCAPE_VERSION, __DATE__);
                exit(0);
                break;
            }
            case SP_ARG_EXTENSIONDIR: {
                printf("%s\n", INKSCAPE_EXTENSIONDIR);
                exit(0);
                break;
            }
            default: {
                break;
            }
        }
    }

    gchar const ** const args = poptGetArgs(ctx);
    if (args != NULL) {
        for (unsigned i = 0; args[i] != NULL; i++) {
            fl = g_slist_append(fl, g_strdup(args[i]));
        }
    }

    return fl;
}


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
