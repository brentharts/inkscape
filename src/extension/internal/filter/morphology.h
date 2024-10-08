// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_INKSCAPE_EXTENSION_INTERNAL_FILTER_MORPHOLOGY_H__
#define SEEN_INKSCAPE_EXTENSION_INTERNAL_FILTER_MORPHOLOGY_H__
/* Change the 'MORPHOLOGY' above to be your file name */

/*
 * Copyright (C) 2011 Authors:
 *   Ivan Louette (filters)
 *   Nicolas Dufour (UI) <nicoduf@yahoo.fr>
 *
 * Morphology filters
 *   Cross-smooth
 *   Outline
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
/* ^^^ Change the copyright to be you and your e-mail address ^^^ */

#include "filter.h"

#include "extension/internal/clear-n_.h"
#include "extension/system.h"
#include "extension/extension.h"

namespace Inkscape {
namespace Extension {
namespace Internal {
namespace Filter {

/**
    \brief    Custom predefined Cross-smooth filter.
    
    Smooth the outside of shapes and pictures.

    Filter's parameters:
    * Type (enum, default "Smooth edges") ->
        Inner = composite1 (operator="in")
        Outer = composite1 (operator="over")
        Open = composite1 (operator="XOR")
    * Width (0.01->30., default 10.) -> blur (stdDeviation)
    * Level (0.2->2., default 1.) -> composite2 (k2)
    * Dilatation (1.->100., default 10.) -> colormatrix1 (last-1 value)
    * Erosion (1.->100., default 1.) -> colormatrix1 (last value)
    * Antialiasing (0.01->1., default 1) -> blur2 (stdDeviation)
    * Blur content (boolean, default false) -> blend (true: in="colormatrix2", false: in="SourceGraphic")
*/

class Crosssmooth : public Inkscape::Extension::Internal::Filter::Filter {
protected:
    gchar const * get_filter_text (Inkscape::Extension::Extension * ext) override;

public:
    Crosssmooth ( ) : Filter() { };
    ~Crosssmooth ( ) override { if (_filter != nullptr) g_free((void *)_filter); return; }

    static void init () {
        // clang-format off
        Inkscape::Extension::build_from_mem(
            "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
              "<name>" N_("Cross-smooth") "</name>\n"
              "<id>org.inkscape.effect.filter.crosssmooth</id>\n"
              "<param name=\"type\" gui-text=\"" N_("Type:") "\" type=\"optiongroup\" appearance=\"combo\">\n"
                "<option value=\"in\">" N_("Inner") "</option>\n"
                "<option value=\"over\">" N_("Outer") "</option>\n"
                "<option value=\"xor\">" N_("Open (XOR)") "</option>\n"
              "</param>\n"
              "<param name=\"width\" gui-text=\"" N_("Width") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"0.01\" max=\"30.\">10</param>\n"
              "<param name=\"level\" gui-text=\"" N_("Level") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"0.2\" max=\"2\">1</param>\n"
              "<param name=\"dilat\" gui-text=\"" N_("Dilatation") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"1\" max=\"100\">10</param>\n"
              "<param name=\"erosion\" gui-text=\"" N_("Erosion") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"1\" max=\"100\">1</param>\n"
              "<param name=\"antialias\" gui-text=\"" N_("Antialiasing") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"0.01\" max=\"1\">1</param>\n"
              "<param name=\"content\" gui-text=\"" N_("Blur content") "\" type=\"bool\" >false</param>\n"

              "<effect>\n"
                "<object-type>all</object-type>\n"
                "<effects-menu>\n"
                  "<submenu name=\"" N_("Filters") "\">\n"
                    "<submenu name=\"" N_("Morphology") "\"/>\n"
                  "</submenu>\n"
                "</effects-menu>\n"
                "<menu-tip>" N_("Smooth edges and angles of shapes") "</menu-tip>\n"
              "</effect>\n"
            "</inkscape-extension>\n", std::make_unique<Crosssmooth>());
        // clang-format on
    };

};

gchar const *
Crosssmooth::get_filter_text (Inkscape::Extension::Extension * ext)
{
    if (_filter != nullptr) g_free((void *)_filter);

    std::ostringstream type;
    std::ostringstream width;
    std::ostringstream level;
    std::ostringstream dilat;
    std::ostringstream erosion;
    std::ostringstream antialias;
    std::ostringstream content;

    type << ext->get_param_optiongroup("type");
    width << ext->get_param_float("width");
    level << ext->get_param_float("level");
    dilat << ext->get_param_float("dilat");
    erosion << (1 - ext->get_param_float("erosion"));
    antialias << ext->get_param_float("antialias");

    if (ext->get_param_bool("content")) {
        content << "colormatrix2";
    } else {
        content << "SourceGraphic";
    }

    // clang-format off
    _filter = g_strdup_printf(
        "<filter xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\" style=\"color-interpolation-filters:sRGB;\" inkscape:label=\"Cross-smooth\">\n"
          "<feGaussianBlur in=\"SourceGraphic\" stdDeviation=\"%s\" result=\"blur1\" />\n"
          "<feComposite in=\"blur1\" in2=\"blur1\" operator=\"%s\" result=\"composite1\" />\n"
          "<feComposite in=\"composite1\" in2=\"composite1\" k2=\"%s\" operator=\"arithmetic\" result=\"composite2\" />\n"
          "<feColorMatrix in=\"composite2\" values=\"1 0 0 0 0 0 1 0 0 0 0 0 1 0 0 0 0 0 %s %s \" result=\"colormatrix1\" />\n"
          "<feGaussianBlur stdDeviation=\"%s\" result=\"blur2\" />\n"
          "<feColorMatrix in=\"blur2\" values=\"1 0 0 0 0 0 1 0 0 0 0 0 1 0 0 0 0 0 5 -1 \" result=\"colormatrix2\" />\n"
          "<feBlend in=\"%s\" in2=\"colormatrix2\" stdDeviation=\"17\" mode=\"normal\" result=\"blend\" />\n"
          "<feComposite in=\"blend\" in2=\"colormatrix2\" operator=\"in\" result=\"composite3\" />\n"
        "</filter>\n", width.str().c_str(), type.str().c_str(), level.str().c_str(),
                       dilat.str().c_str(), erosion.str().c_str(), antialias.str().c_str(),
                       content.str().c_str());
    // clang-format on

    return _filter;
}; /* Cross-smooth filter */

/**
    \brief    Custom predefined Outline filter.
    
    Adds a colorizable outline

    Filter's parameters:
    * Fill image (boolean, default false) -> true: composite2 (in="SourceGraphic"), false: composite2 (in="blur2")
    * Hide image (boolean, default false) -> true: composite4 (in="composite3"), false: composite4 (in="SourceGraphic")
    * Stroke type (enum, default over) -> composite2 (operator)
    * Stroke position (enum, default inside)
        * inside -> composite1 (operator="out", in="SourceGraphic", in2="blur1")
        * outside -> composite1 (operator="out", in="blur1", in2="SourceGraphic")
        * overlayed -> composite1 (operator="xor", in="blur1", in2="SourceGraphic")
    * Width 1 (0.01->20., default 4) -> blur1 (stdDeviation)
    * Dilatation 1 (1.->100., default 100) -> colormatrix1 (n-1th value)
    * Erosion 1 (0.->100., default 1) -> colormatrix1 (nth value 0->-100)
    * Width 2 (0.01->20., default 0.5) -> blur2 (stdDeviation)
    * Dilatation 2 (1.->100., default 50) -> colormatrix2 (n-1th value)
    * Erosion 2 (0.->100., default 5) -> colormatrix2 (nth value 0->-100)
    * Antialiasing (0.01->1., default 1) -> blur3 (stdDeviation)
    * Color (guint, default 0,0,0,255) -> flood (flood-color, flood-opacity)
    * Fill opacity (0.->1., default 1) -> composite5 (k2)
    * Stroke opacity (0.->1., default 1) -> composite5 (k3)
        
*/

class Outline : public Inkscape::Extension::Internal::Filter::Filter {
protected:
    gchar const * get_filter_text (Inkscape::Extension::Extension * ext) override;

public:
    Outline ( ) : Filter() { };
    ~Outline ( ) override { if (_filter != nullptr) g_free((void *)_filter); return; }

    static void init () {
        // clang-format off
        Inkscape::Extension::build_from_mem(
            "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
              "<name>" N_("Outline") "</name>\n"
              "<id>org.inkscape.effect.filter.Outline</id>\n"
              "<param name=\"tab\" type=\"notebook\">\n"
                "<page name=\"optionstab\" gui-text=\"Options\">\n"
                  "<param name=\"fill\" gui-text=\"" N_("Fill image") "\" type=\"bool\" >false</param>\n"
                  "<param name=\"outline\" gui-text=\"" N_("Hide image") "\" type=\"bool\" >false</param>\n"
                  "<param name=\"type\" gui-text=\"" N_("Composite type:") "\" type=\"optiongroup\" appearance=\"combo\">\n"
                    "<option value=\"over\">" N_("Over") "</option>\n"
                    "<option value=\"in\">" N_("In") "</option>\n"
                    "<option value=\"out\">" N_("Out") "</option>\n"
                    "<option value=\"atop\">" N_("Atop") "</option>\n"
                    "<option value=\"xor\">" N_("XOR") "</option>\n"
                  "</param>\n"
                  "<param name=\"position\" gui-text=\"" N_("Position:") "\" type=\"optiongroup\" appearance=\"combo\">\n"
                    "<option value=\"inside\">" N_("Inside") "</option>\n"
                    "<option value=\"outside\">" N_("Outside") "</option>\n"
                    "<option value=\"overlayed\">" N_("Overlayed") "</option>\n"
                  "</param>\n"
                  "<param name=\"width1\" gui-text=\"" N_("Width 1") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"0.01\" max=\"20.00\">4</param>\n"
                  "<param name=\"dilat1\" gui-text=\"" N_("Dilatation 1") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"1\" max=\"100\">100</param>\n"
                  "<param name=\"erosion1\" gui-text=\"" N_("Erosion 1") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"0\" max=\"100\">1</param>\n"
                  "<param name=\"width2\" gui-text=\"" N_("Width 2") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"0.01\" max=\"20.00\">0.5</param>\n"
                  "<param name=\"dilat2\" gui-text=\"" N_("Dilatation 2") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"1\" max=\"100\">50</param>\n"
                  "<param name=\"erosion2\" gui-text=\"" N_("Erosion 2") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"0\" max=\"100\">5</param>\n"
                  "<param name=\"antialias\" gui-text=\"" N_("Antialiasing") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"0.01\" max=\"1\">1</param>\n"
                  "<param name=\"smooth\" gui-text=\"" N_("Smooth") "\" type=\"bool\" >false</param>\n"
                "</page>\n"
                "<page name=\"co11tab\" gui-text=\"Color\">\n"
                  "<param name=\"color\" gui-text=\"" N_("Color") "\" type=\"color\">255</param>\n"
                  "<param name=\"fopacity\" gui-text=\"" N_("Fill opacity:") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"0\" max=\"1\">1</param>\n"
                  "<param name=\"sopacity\" gui-text=\"" N_("Stroke opacity:") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"0\" max=\"1\">1</param>\n"
                "</page>\n"
              "</param>\n"
              "<effect>\n"
                "<object-type>all</object-type>\n"
                "<effects-menu>\n"
                  "<submenu name=\"" N_("Filters") "\">\n"
                    "<submenu name=\"" N_("Morphology") "\"/>\n"
                  "</submenu>\n"
                "</effects-menu>\n"
                "<menu-tip>" N_("Adds a colorizable outline") "</menu-tip>\n"
              "</effect>\n"
            "</inkscape-extension>\n", std::make_unique<Outline>());
        // clang-format on
    };

};

gchar const *
Outline::get_filter_text (Inkscape::Extension::Extension * ext)
{
    if (_filter != nullptr) g_free((void *)_filter);

    std::ostringstream width1;
    std::ostringstream dilat1;
    std::ostringstream erosion1;
    std::ostringstream width2;
    std::ostringstream dilat2;
    std::ostringstream erosion2;
    std::ostringstream antialias;
    std::ostringstream fopacity;
    std::ostringstream sopacity;
    std::ostringstream smooth;

    std::ostringstream c1in;
    std::ostringstream c1in2;
    std::ostringstream c1op;
    std::ostringstream c2in;
    std::ostringstream c2op;
    std::ostringstream c4in;

    
    width1 << ext->get_param_float("width1");
    dilat1 << ext->get_param_float("dilat1");
    erosion1 << (- ext->get_param_float("erosion1"));
    width2 << ext->get_param_float("width2");
    dilat2 << ext->get_param_float("dilat2");
    erosion2 << (- ext->get_param_float("erosion2"));
    antialias << ext->get_param_float("antialias");
    auto color = ext->get_param_color("color");

    fopacity << ext->get_param_float("fopacity");
    sopacity << ext->get_param_float("sopacity");

    const gchar *position = ext->get_param_optiongroup("position");
    if((g_ascii_strcasecmp("inside", position) == 0)) {
    // Inside
        c1in << "SourceGraphic";
        c1in2 << "blur1";
        c1op << "out";
    } else if((g_ascii_strcasecmp("outside", position) == 0)) {
    // Outside
        c1in << "blur1";
        c1in2 << "SourceGraphic";
        c1op << "out";
    } else {
    // Overlayed
        c1in << "blur1";
        c1in2 << "SourceGraphic";
        c1op << "xor";
    }

    if (ext->get_param_bool("fill")) {
        c2in << "SourceGraphic";
    } else {
        c2in << "blur2";
    }

    c2op << ext->get_param_optiongroup("type");
        
    if (ext->get_param_bool("outline")) {
        c4in << "composite3";
    } else {
        c4in << "SourceGraphic";
    }

    if (ext->get_param_bool("smooth")) {
        smooth << "1 0";
    } else {
        smooth << "5 -1";
    }
     
    // clang-format off
    _filter = g_strdup_printf(
        "<filter xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\" style=\"color-interpolation-filters:sRGB;\" height=\"1.4\" width=\"1.4\" y=\"-0.2\" x=\"-0.2\" inkscape:label=\"Outline\">\n"
          "<feGaussianBlur in=\"SourceAlpha\" stdDeviation=\"%s\" result=\"blur1\" />\n"
          "<feComposite in=\"%s\" in2=\"%s\" operator=\"%s\" result=\"composite1\" />\n"
          "<feColorMatrix values=\"1 0 0 0 0 0 1 0 0 0 0 0 1 0 0 0 0 0 %s %s \" result=\"colormatrix1\" />\n"
          "<feGaussianBlur stdDeviation=\"%s\" result=\"blur2\" />\n"
          "<feComposite in=\"%s\" in2=\"blur2\" operator=\"%s\" result=\"composite2\" />\n"
          "<feColorMatrix values=\"1 0 0 0 0 0 1 0 0 0 0 0 1 0 0 0 0 0 %s %s \" result=\"colormatrix2\" />\n"
          "<feGaussianBlur stdDeviation=\"%s\" result=\"blur3\" />\n"
          "<feColorMatrix values=\"1 0 0 0 0 0 1 0 0 0 0 0 1 0 0 0 0 0 %s \" result=\"colormatrix3\" />\n"
          "<feFlood flood-opacity=\"%f\" flood-color=\"%s\" result=\"flood\" />\n"
          "<feComposite in=\"flood\" in2=\"colormatrix3\" k2=\"1\" operator=\"in\" result=\"composite3\" />\n"
          "<feComposite in=\"%s\" in2=\"colormatrix3\" operator=\"out\" result=\"composite4\" />\n"
          "<feComposite in=\"composite4\" in2=\"composite3\" k2=\"%s\" k3=\"%s\" operator=\"arithmetic\" result=\"composite5\" />\n"
        "</filter>\n", width1.str().c_str(), c1in.str().c_str(), c1in2.str().c_str(), c1op.str().c_str(),
                       dilat1.str().c_str(), erosion1.str().c_str(),
                       width2.str().c_str(), c2in.str().c_str(), c2op.str().c_str(),
                       dilat2.str().c_str(), erosion2.str().c_str(), antialias.str().c_str(), smooth.str().c_str(),
                       color.getOpacity(), color.toString(false).c_str(),
                       c4in.str().c_str(), fopacity.str().c_str(), sopacity.str().c_str() );
    // clang-format on

    return _filter;
}; /* Outline filter */

}; /* namespace Filter */
}; /* namespace Internal */
}; /* namespace Extension */
}; /* namespace Inkscape */

/* Change the 'MORPHOLOGY' below to be your file name */
#endif /* SEEN_INKSCAPE_EXTENSION_INTERNAL_FILTER_MORPHOLOGY_H__ */
