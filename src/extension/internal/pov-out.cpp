// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A simple utility for exporting Inkscape svg Shapes as PovRay bezier
 * prisms.  Note that this is output-only, and would thus seem to be
 * better placed as an 'export' rather than 'output'.  However, Export
 * handles all or partial documents, while this outputs ALL shapes in
 * the current SVG document.
 *
 *  For information on the PovRay file format, see:
 *      http://www.povray.org
 *
 * Authors:
 *   Bob Jamison <ishmal@inkscape.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2004-2008 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "pov-out.h"
#include <inkscape.h>
#include <inkscape-version.h>
#include <display/curve.h>
#include <extension/system.h>
#include <2geom/pathvector.h>
#include <2geom/rect.h>
#include <2geom/curves.h>
#include "helper/geom.h"
#include "helper/geom-curves.h"
#include <io/sys.h>

#include "object/sp-root.h"
#include "object/sp-path.h"
#include "style.h"

#include <string>
#include <cstdio>
#include <cstdarg>
#include "document.h"
#include "extension/extension.h"
#include "util/safe-printf.h"


namespace Inkscape
{
namespace Extension
{
namespace Internal
{


//########################################################################
//# M E S S A G E S
//########################################################################

static void err(const char *fmt, ...)
{
    va_list args;
    g_log(nullptr,  G_LOG_LEVEL_WARNING, "Pov-out err: ");
    va_start(args, fmt);
    g_logv(nullptr, G_LOG_LEVEL_WARNING, fmt, args);
    va_end(args);
    g_log(nullptr,  G_LOG_LEVEL_WARNING, "\n");
}




//########################################################################
//# U T I L I T Y
//########################################################################



static double effective_opacity(SPItem const *item)
{
    // TODO investigate this. The early return seems that it would abort early.
    // Plus is will emit a warning, which may not be proper here.
    double ret = 1.0;
    for (SPObject const *obj = item; obj; obj = obj->parent) {
        g_return_val_if_fail(obj->style, ret);
        ret *= SP_SCALE24_TO_FLOAT(obj->style->opacity.value);
    }
    return ret;
}





//########################################################################
//# OUTPUT FORMATTING
//########################################################################

PovOutput::PovOutput() :
    outbuf (),
    nrNodes (0),
    nrSegments (0),
    nrShapes (0),
    idIndex (0),
    minx (0),
    miny (0),
    maxx (0),
    maxy (0)
{
}

/**
 * We want to control floating output format
 */
static PovOutput::String dstr(double d)
{
    char dbuf[G_ASCII_DTOSTR_BUF_SIZE+1];
    g_ascii_formatd(dbuf, G_ASCII_DTOSTR_BUF_SIZE,
                  "%.8f", (gdouble)d);
    PovOutput::String s = dbuf;
    return s;
}

#define DSTR(d) (dstr(d).c_str())


/**
 *  Output data to the buffer, printf()-style
 */
void PovOutput::out(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    gchar *output = g_strdup_vprintf(fmt, args);
    va_end(args);
    outbuf.append(output);
    g_free(output);
}





/**
 *  Output a 2d vector
 */
void PovOutput::vec2(double a, double b)
{
    out("<%s, %s>", DSTR(a), DSTR(b));
}



/**
 * Output a 3d vector
 */
void PovOutput::vec3(double a, double b, double c)
{
    out("<%s, %s, %s>", DSTR(a), DSTR(b), DSTR(c));
}



/**
 *  Output a v4d ector
 */
void PovOutput::vec4(double a, double b, double c, double d)
{
    out("<%s, %s, %s, %s>", DSTR(a), DSTR(b), DSTR(c), DSTR(d));
}



/**
 *  Output an rgbf color vector
 */
void PovOutput::rgbf(double r, double g, double b, double f)
{
    //"rgbf < %1.3f, %1.3f, %1.3f %1.3f>"
    out("rgbf ");
    vec4(r, g, b, f);
}



/**
 *  Output one bezier's start, start-control, end-control, and end nodes
 */
void PovOutput::segment(int segNr,
                        double startX,     double startY,
                        double startCtrlX, double startCtrlY,
                        double endCtrlX,   double endCtrlY,
                        double endX,       double endY)
{
    //"    /*%4d*/ <%f, %f>, <%f, %f>, <%f,%f>, <%f,%f>"
    out("    /*%4d*/ ", segNr);
    vec2(startX,     startY);
    out(", ");
    vec2(startCtrlX, startCtrlY);
    out(", ");
    vec2(endCtrlX,   endCtrlY);
    out(", ");
    vec2(endX,       endY);
}





/**
 * Output the file header
 */
bool PovOutput::doHeader()
{
    time_t tim = time(nullptr);
    out("/*###################################################################\n");
    out("### This PovRay document was generated by Inkscape\n");
    out("### http://www.inkscape.org\n");
    out("### Created: %s",   ctime(&tim));
    out("### Version: %s\n", Inkscape::version_string);
    out("#####################################################################\n");
    out("### NOTES:\n");
    out("### ============\n");
    out("### POVRay information can be found at\n");
    out("### http://www.povray.org\n");
    out("###\n");
    out("### The 'AllShapes' objects at the bottom are provided as a\n");
    out("### preview of how the output would look in a trace.  However,\n");
    out("### the main intent of this file is to provide the individual\n");
    out("### shapes for inclusion in a POV project.\n");
    out("###\n");
    out("### For an example of how to use this file, look at\n");
    out("### share/examples/istest.pov\n");
    out("###\n");
    out("### If you have any problems with this output, please see the\n");
    out("### Inkscape project at http://www.inkscape.org, or visit\n");
    out("### the #inkscape channel on irc.freenode.net . \n");
    out("###\n");
    out("###################################################################*/\n");
    out("\n\n");
    out("/*###################################################################\n");
    out("##   Exports in this file\n");
    out("##==========================\n");
    out("##    Shapes   : %d\n", nrShapes);
    out("##    Segments : %d\n", nrSegments);
    out("##    Nodes    : %d\n", nrNodes);
    out("###################################################################*/\n");
    out("\n\n\n");
    return true;
}



/**
 *  Output the file footer
 */
bool PovOutput::doTail()
{
    out("\n\n");
    out("/*###################################################################\n");
    out("### E N D    F I L E\n");
    out("###################################################################*/\n");
    out("\n\n");
    return true;
}



/**
 *  Output the curve data to buffer
 */
bool PovOutput::doCurve(SPItem *item, const String &id)
{
    using Geom::X;
    using Geom::Y;

    //### Get the Shape
    if (!is<SPShape>(item))//Bulia's suggestion.  Allow all shapes
        return true;

    auto shape = cast<SPShape>(item);
    if (shape->curve()->is_empty()) {
        return true;
    }

    nrShapes++;

    PovShapeInfo shapeInfo;
    shapeInfo.id    = id;
    shapeInfo.color = "";

    //Try to get the fill color of the shape
    SPStyle *style = shape->style;
    /* fixme: Handle other fill types, even if this means translating gradients to a single
           flat colour. */
    if (style && style->fill.isColor()) {
        auto rgba = *style->fill.getColor().converted(Colors::Space::Type::RGB);
        rgba.addOpacity(style->fill_opacity);
        rgba.addOpacity(effective_opacity(shape));
        String rgbf = "rgbf <";
        rgbf.append(dstr(rgba[0]));         rgbf.append(", ");
        rgbf.append(dstr(rgba[1]));         rgbf.append(", ");
        rgbf.append(dstr(rgba[2]));         rgbf.append(", ");
        rgbf.append(dstr(1.0 - rgba[3])); rgbf.append(">");
        shapeInfo.color += rgbf;
    }

    povShapes.push_back(shapeInfo); //passed all tests.  save the info

    // convert the path to only lineto's and cubic curveto's:
    Geom::Affine tf = item->i2dt_affine();
    Geom::PathVector pathv = pathv_to_linear_and_cubic_beziers(shape->curve()->get_pathvector() * tf);

    /*
     * We need to know the number of segments (NR_CURVETOs/LINETOs, including
     * closing line segment) before we write out segment data. Since we are
     * going to skip degenerate (zero length) paths, we need to loop over all
     * subpaths and segments first.
     */
    int segmentCount = 0;
    /**
     * For all Subpaths in the <path>
     */
    for (const auto & pit : pathv)
    {
        /**
         * For all segments in the subpath, including extra closing segment defined by 2geom
         */
        for (Geom::Path::const_iterator cit = pit.begin(); cit != pit.end_closed(); ++cit)
        {

            // Skip zero length segments.
            if( !cit->isDegenerate() ) ++segmentCount;
        }
    }

    out("/*###################################################\n");
    out("### PRISM:  %s\n", id.c_str());
    out("###################################################*/\n");
    out("#declare %s = prism {\n", id.c_str());
    out("    linear_sweep\n");
    out("    bezier_spline\n");
    out("    1.0, //top\n");
    out("    0.0, //bottom\n");
    out("    %d //nr points\n", segmentCount * 4);
    int segmentNr = 0;

    nrSegments += segmentCount;

    /**
     *   at moment of writing, 2geom lacks proper initialization of empty intervals in rect...
     */
    Geom::Rect cminmax( pathv.front().initialPoint(), pathv.front().initialPoint() );


    /**
     * For all Subpaths in the <path>
     */
    for (const auto & pit : pathv)
        {

        cminmax.expandTo(pit.initialPoint());

        /**
         * For all segments in the subpath, including extra closing segment defined by 2geom
         */
        for (Geom::Path::const_iterator cit = pit.begin(); cit != pit.end_closed(); ++cit)
            {

            // Skip zero length segments
            if( cit->isDegenerate() )
                continue;

            if( is_straight_curve(*cit) )
                {
                Geom::Point p0 = cit->initialPoint();
                Geom::Point p1 = cit->finalPoint();
                segment(segmentNr++,
                        p0[X], p0[Y], p0[X], p0[Y], p1[X], p1[Y], p1[X], p1[Y] );
                nrNodes += 8;
                }
            else if(Geom::CubicBezier const *cubic = dynamic_cast<Geom::CubicBezier const*>(&*cit))
            {
                std::vector<Geom::Point> points = cubic->controlPoints();
                Geom::Point p0 = points[0];
                Geom::Point p1 = points[1];
                Geom::Point p2 = points[2];
                Geom::Point p3 = points[3];
                segment(segmentNr++,
                            p0[X],p0[Y], p1[X],p1[Y], p2[X],p2[Y], p3[X],p3[Y]);
                nrNodes += 8;
                }
            else
            {
                err("logical error, because pathv_to_linear_and_cubic_beziers was used");
                return false;
                }

            if (segmentNr < segmentCount)
                out(",\n");
            else
                out("\n");
            if (segmentNr > segmentCount)
                {
                err("Too many segments");
                return false;
                }

            cminmax.expandTo(cit->finalPoint());

            }
        }

    out("}\n");

    double cminx = cminmax.min()[X];
    double cmaxx = cminmax.max()[X];
    double cminy = cminmax.min()[Y];
    double cmaxy = cminmax.max()[Y];

    out("#declare %s_MIN_X    = %s;\n", id.c_str(), DSTR(cminx));
    out("#declare %s_CENTER_X = %s;\n", id.c_str(), DSTR((cmaxx+cminx)/2.0));
    out("#declare %s_MAX_X    = %s;\n", id.c_str(), DSTR(cmaxx));
    out("#declare %s_WIDTH    = %s;\n", id.c_str(), DSTR(cmaxx-cminx));
    out("#declare %s_MIN_Y    = %s;\n", id.c_str(), DSTR(cminy));
    out("#declare %s_CENTER_Y = %s;\n", id.c_str(), DSTR((cmaxy+cminy)/2.0));
    out("#declare %s_MAX_Y    = %s;\n", id.c_str(), DSTR(cmaxy));
    out("#declare %s_HEIGHT   = %s;\n", id.c_str(), DSTR(cmaxy-cminy));
    if (shapeInfo.color.length()>0)
        out("#declare %s_COLOR    = %s;\n",
                id.c_str(), shapeInfo.color.c_str());
    out("/*###################################################\n");
    out("### end %s\n", id.c_str());
    out("###################################################*/\n\n\n\n");

    if (cminx < minx)
        minx = cminx;
    if (cmaxx > maxx)
        maxx = cmaxx;
    if (cminy < miny)
        miny = cminy;
    if (cmaxy > maxy)
        maxy = cmaxy;

    return true;
}

/**
 *  Descend the svg tree recursively, translating data
 */
bool PovOutput::doTreeRecursive(SPDocument *doc, SPObject *obj)
{
    String id;
    if (!obj->getId()) {
        char buf[16];
        safeprintf(buf, "id%d", idIndex++);
        id = buf;
    }
    else {
        id = obj->getId();
    }

    if (is<SPItem>(obj)) {
        auto item = cast<SPItem>(obj);
        if (!doCurve(item, id)) {
            return false;
        }
    }

    /**
     * Descend into children
     */
    for (auto &child: obj->children) {
        if (!doTreeRecursive(doc, &child))
            return false;
    }

    return true;
}

/**
 *  Output the curve data to buffer
 */
bool PovOutput::doTree(SPDocument *doc)
{
    double bignum = 1000000.0;
    minx  =  bignum;
    maxx  = -bignum;
    miny  =  bignum;
    maxy  = -bignum;

    if (!doTreeRecursive(doc, doc->getRoot()))
        return false;

    //## Let's make a union of all of the Shapes
    if (!povShapes.empty())
        {
        String id = "AllShapes";
        char *pfx = (char *)id.c_str();
        out("/*###################################################\n");
        out("### UNION OF ALL SHAPES IN DOCUMENT\n");
        out("###################################################*/\n");
        out("\n\n");
        out("/**\n");
        out(" * Allow the user to redefine the finish{}\n");
        out(" * by declaring it before #including this file\n");
        out(" */\n");
        out("#ifndef (%s_Finish)\n", pfx);
        out("#declare %s_Finish = finish {\n", pfx);
        out("    phong 0.5\n");
        out("    reflection 0.3\n");
        out("    specular 0.5\n");
        out("}\n");
        out("#end\n");
        out("\n\n");
        out("#declare %s = union {\n", id.c_str());
        for (auto & povShape : povShapes)
            {
            out("    object { %s\n", povShape.id.c_str());
            out("        texture { \n");
            if (povShape.color.length()>0)
                out("            pigment { %s }\n", povShape.color.c_str());
            else
                out("            pigment { rgb <0,0,0> }\n");
            out("            finish { %s_Finish }\n", pfx);
            out("            } \n");
            out("        } \n");
            }
        out("}\n\n\n\n");


        double zinc   = 0.2 / (double)povShapes.size();
        out("/*#### Same union, but with Z-diffs (actually Y in pov) ####*/\n");
        out("\n\n");
        out("/**\n");
        out(" * Allow the user to redefine the Z-Increment\n");
        out(" */\n");
        out("#ifndef (AllShapes_Z_Increment)\n");
        out("#declare AllShapes_Z_Increment = %s;\n", DSTR(zinc));
        out("#end\n");
        out("\n");
        out("#declare AllShapes_Z_Scale = 1.0;\n");
        out("\n\n");
        out("#declare %s_Z = union {\n", pfx);

        for (auto & povShape : povShapes)
            {
            out("    object { %s\n", povShape.id.c_str());
            out("        texture { \n");
            if (povShape.color.length()>0)
                out("            pigment { %s }\n", povShape.color.c_str());
            else
                out("            pigment { rgb <0,0,0> }\n");
            out("            finish { %s_Finish }\n", pfx);
            out("            } \n");
            out("        scale <1, %s_Z_Scale, 1>\n", pfx);
            out("        } \n");
            out("#declare %s_Z_Scale = %s_Z_Scale + %s_Z_Increment;\n\n",
                    pfx, pfx, pfx);
            }

        out("}\n");

        out("#declare %s_MIN_X    = %s;\n", pfx, DSTR(minx));
        out("#declare %s_CENTER_X = %s;\n", pfx, DSTR((maxx+minx)/2.0));
        out("#declare %s_MAX_X    = %s;\n", pfx, DSTR(maxx));
        out("#declare %s_WIDTH    = %s;\n", pfx, DSTR(maxx-minx));
        out("#declare %s_MIN_Y    = %s;\n", pfx, DSTR(miny));
        out("#declare %s_CENTER_Y = %s;\n", pfx, DSTR((maxy+miny)/2.0));
        out("#declare %s_MAX_Y    = %s;\n", pfx, DSTR(maxy));
        out("#declare %s_HEIGHT   = %s;\n", pfx, DSTR(maxy-miny));
        out("/*##############################################\n");
        out("### end %s\n", id.c_str());
        out("##############################################*/\n");
        out("\n\n");
        }

    return true;
}


//########################################################################
//# M A I N    O U T P U T
//########################################################################



/**
 *  Set values back to initial state
 */
void PovOutput::reset()
{
    nrNodes    = 0;
    nrSegments = 0;
    nrShapes   = 0;
    idIndex    = 0;
    outbuf.clear();
    povShapes.clear();
}



/**
 * Saves the Shapes of an Inkscape SVG file as PovRay spline definitions
 */
void PovOutput::saveDocument(SPDocument *doc, gchar const *filename_utf8)
{
    reset();

    //###### SAVE IN POV FORMAT TO BUFFER
    //# Lets do the curves first, to get the stats
    if (!doTree(doc))
        {
        err("Could not output curves for %s", filename_utf8);
        return;
        }

    String curveBuf = outbuf;
    outbuf.clear();

    if (!doHeader())
        {
        err("Could not write header for %s", filename_utf8);
        return;
        }

    outbuf.append(curveBuf);

    if (!doTail())
        {
        err("Could not write footer for %s", filename_utf8);
        return;
        }




    //###### WRITE TO FILE
    Inkscape::IO::dump_fopen_call(filename_utf8, "L");
    FILE *f = Inkscape::IO::fopen_utf8name(filename_utf8, "w");
    if (!f)
        return;

    for (String::iterator iter = outbuf.begin() ; iter!=outbuf.end(); ++iter)
        {
        int ch = *iter;
        fputc(ch, f);
        }

    fclose(f);
}




//########################################################################
//# EXTENSION API
//########################################################################



#include "clear-n_.h"



/**
 * API call to save document
*/
void
PovOutput::save(Inkscape::Extension::Output */*mod*/,
                        SPDocument *doc, gchar const *filename_utf8)
{
    /* See comments in JavaFSOutput::save re the name `filename_utf8'. */
    saveDocument(doc, filename_utf8);
}



/**
 * Make sure that we are in the database
 */
bool PovOutput::check (Inkscape::Extension::Extension */*module*/)
{
    /* We don't need a Key
    if (NULL == Inkscape::Extension::db.get(SP_MODULE_KEY_OUTPUT_POV))
        return FALSE;
    */

    return true;
}



/**
 * This is the definition of PovRay output.  This function just
 * calls the extension system with the memory allocated XML that
 * describes the data.
*/
void
PovOutput::init()
{
    // clang-format off
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("PovRay Output") "</name>\n"
            "<id>org.inkscape.output.pov</id>\n"
            "<output>\n"
                "<extension>.pov</extension>\n"
                "<mimetype>text/x-povray-script</mimetype>\n"
                "<filetypename>" N_("PovRay (*.pov) (paths and shapes only)") "</filetypename>\n"
                "<filetypetooltip>" N_("PovRay Raytracer File") "</filetypetooltip>\n"
            "</output>\n"
        "</inkscape-extension>",
        std::make_unique<PovOutput>());
    // clang-format on
}





}  // namespace Internal
}  // namespace Extension
}  // namespace Inkscape


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
