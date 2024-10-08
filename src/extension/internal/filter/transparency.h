// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_INKSCAPE_EXTENSION_INTERNAL_FILTER_TRANSPARENCY_H__
#define SEEN_INKSCAPE_EXTENSION_INTERNAL_FILTER_TRANSPARENCY_H__
/* Change the 'TRANSPARENCY' above to be your file name */

/*
 * Copyright (C) 2011 Authors:
 *   Ivan Louette (filters)
 *   Nicolas Dufour (UI) <nicoduf@yahoo.fr>
 *
 * Fill and transparency filters
 *   Blend
 *   Channel transparency
 *   Light eraser
 *   Opacity
 *   Silhouette
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
    \brief    Custom predefined Blend filter.
    
    Blend objects with background images or with themselves

    Filter's parameters:
    * Source (enum [SourceGraphic,BackgroundImage], default BackgroundImage) -> blend (in2)
    * Mode (enum, all blend modes, default Multiply) -> blend (mode)
*/

class Blend : public Inkscape::Extension::Internal::Filter::Filter {
protected:
    gchar const * get_filter_text (Inkscape::Extension::Extension * ext) override;

public:
    Blend ( ) : Filter() { };
    ~Blend ( ) override { if (_filter != nullptr) g_free((void *)_filter); return; }

    static void init () {
        // clang-format off
        Inkscape::Extension::build_from_mem(
            "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
              "<name>" N_("Blend") "</name>\n"
              "<id>org.inkscape.effect.filter.Blend</id>\n"
              "<param name=\"source\" gui-text=\"" N_("Source:") "\" type=\"optiongroup\" appearance=\"combo\">\n"
                "<option value=\"BackgroundImage\">" N_("Background") "</option>\n"
                "<option value=\"SourceGraphic\">" N_("Image") "</option>\n"
              "</param>\n"
              "<param name=\"mode\" gui-text=\"" N_("Mode:") "\" type=\"optiongroup\" appearance=\"combo\">\n"
                "<option value=\"multiply\">" N_("Multiply") "</option>\n"
                "<option value=\"normal\">" N_("Normal") "</option>\n"
                "<option value=\"screen\">" N_("Screen") "</option>\n"
                "<option value=\"darken\">" N_("Darken") "</option>\n"
                "<option value=\"lighten\">" N_("Lighten") "</option>\n"
              "</param>\n"
              "<effect>\n"
                "<object-type>all</object-type>\n"
                "<effects-menu>\n"
                  "<submenu name=\"" N_("Filters") "\">\n"
                    "<submenu name=\"" N_("Fill and Transparency") "\"/>\n"
                  "</submenu>\n"
                "</effects-menu>\n"
                "<menu-tip>" N_("Blend objects with background images or with themselves") "</menu-tip>\n"
              "</effect>\n"
            "</inkscape-extension>\n", std::make_unique<Blend>());
        // clang-format on
    };

};

gchar const *
Blend::get_filter_text (Inkscape::Extension::Extension * ext)
{
    if (_filter != nullptr) g_free((void *)_filter);

    std::ostringstream source;
    std::ostringstream mode;

    source << ext->get_param_optiongroup("source");
    mode << ext->get_param_optiongroup("mode");

    // clang-format off
    _filter = g_strdup_printf(
        "<filter xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\" style=\"color-interpolation-filters:sRGB;\" inkscape:label=\"Blend\">\n"
          "<feBlend in2=\"%s\" mode=\"%s\" result=\"blend\" />\n"
        "</filter>\n", source.str().c_str(), mode.str().c_str() );
    // clang-format on

    return _filter;
}; /* Blend filter */

/**
    \brief    Custom predefined Channel transparency filter.
    
    Channel transparency filter.

    Filter's parameters:
    * Saturation (0.->1., default 1.) -> colormatrix1 (values)
    * Red (-10.->10., default -1.) -> colormatrix2 (values)
    * Green (-10.->10., default 0.5) -> colormatrix2 (values)
    * Blue (-10.->10., default 0.5) -> colormatrix2 (values)
    * Alpha (-10.->10., default 1.) -> colormatrix2 (values)
    * Flood colors (guint, default 16777215) -> flood (flood-opacity, flood-color)
    * Inverted (boolean, default false) -> composite1 (operator, true='in', false='out')
    
    Matrix:
      1  0  0  0  0
      0  1  0  0  0
      0  0  1  0  0
      R  G  B  A  0
*/
class ChannelTransparency : public Inkscape::Extension::Internal::Filter::Filter {
protected:
    gchar const * get_filter_text (Inkscape::Extension::Extension * ext) override;

public:
    ChannelTransparency ( ) : Filter() { };
    ~ChannelTransparency ( ) override { if (_filter != nullptr) g_free((void *)_filter); return; }
    
    static void init () {
        // clang-format off
        Inkscape::Extension::build_from_mem(
            "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
              "<name>" N_("Channel Transparency") "</name>\n"
              "<id>org.inkscape.effect.filter.ChannelTransparency</id>\n"
              "<param name=\"red\" gui-text=\"" N_("Red") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"-10.\" max=\"10.\">-1</param>\n"
              "<param name=\"green\" gui-text=\"" N_("Green") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"-10.\" max=\"10.\">0.5</param>\n"
              "<param name=\"blue\" gui-text=\"" N_("Blue") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"-10.\" max=\"10.\">0.5</param>\n"
              "<param name=\"alpha\" gui-text=\"" N_("Alpha") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"-10.\" max=\"10.\">1</param>\n"
              "<param name=\"invert\" gui-text=\"" N_("Inverted") "\" type=\"bool\">false</param>\n"
              "<effect>\n"
                "<object-type>all</object-type>\n"
                "<effects-menu>\n"
                  "<submenu name=\"" N_("Filters") "\">\n"
                    "<submenu name=\"" N_("Fill and Transparency") "\"/>\n"
                  "</submenu>\n"
                "</effects-menu>\n"
                "<menu-tip>" N_("Replace RGB with transparency") "</menu-tip>\n"
              "</effect>\n"
            "</inkscape-extension>\n", std::make_unique<ChannelTransparency>());
        // clang-format on
    };
};

gchar const *
ChannelTransparency::get_filter_text (Inkscape::Extension::Extension * ext)
{
    if (_filter != nullptr) g_free((void *)_filter);

    std::ostringstream red;
    std::ostringstream green;
    std::ostringstream blue;
    std::ostringstream alpha;
    std::ostringstream invert;

    red << ext->get_param_float("red");
    green << ext->get_param_float("green");
    blue << ext->get_param_float("blue");
    alpha << ext->get_param_float("alpha");

    if (!ext->get_param_bool("invert")) {
        invert << "in";
    } else {
        invert << "xor";
    }
    
    // clang-format off
    _filter = g_strdup_printf(
        "<filter xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\" inkscape:label=\"Channel Transparency\" style=\"color-interpolation-filters:sRGB;\" >\n"
          "<feColorMatrix values=\"1 0 0 0 0 0 1 0 0 0 0 0 1 0 0 %s %s %s %s 0 \" in=\"SourceGraphic\" result=\"colormatrix\" />\n"
          "<feComposite in=\"colormatrix\" in2=\"SourceGraphic\" operator=\"%s\" result=\"composite1\" />\n"
        "</filter>\n", red.str().c_str(), green.str().c_str(), blue.str().c_str(), alpha.str().c_str(),
                       invert.str().c_str());
    // clang-format on

    return _filter;
}; /* Channel transparency filter */

/**
    \brief    Custom predefined LightEraser filter.
    
    Make the lightest parts of the object progressively transparent.

    Filter's parameters:
    * Expansion (0.->1000., default 50) -> colormatrix (4th value, multiplicator)
    * Erosion (1.->1000., default 100) -> colormatrix (first 3 values, multiplicator)
    * Global opacity (0.->1., default 1.) -> composite (k2)
    * Inverted (boolean, default false) -> colormatrix (values, true: first 3 values positive, 4th negative)
    
*/
class LightEraser : public Inkscape::Extension::Internal::Filter::Filter {
protected:
    gchar const * get_filter_text (Inkscape::Extension::Extension * ext) override;

public:
    LightEraser ( ) : Filter() { };
    ~LightEraser ( ) override { if (_filter != nullptr) g_free((void *)_filter); return; }
    
    static void init () {
        // clang-format off
        Inkscape::Extension::build_from_mem(
            "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
              "<name>" N_("Light Eraser") "</name>\n"
              "<id>org.inkscape.effect.filter.LightEraser</id>\n"
              "<param name=\"expand\" gui-text=\"" N_("Expansion") "\" type=\"float\" appearance=\"full\"  min=\"0\" max=\"1000\">50</param>\n"
              "<param name=\"erode\" gui-text=\"" N_("Erosion") "\" type=\"float\" appearance=\"full\" min=\"1\" max=\"1000\">100</param>\n"
              "<param name=\"opacity\" gui-text=\"" N_("Global opacity") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"0.\" max=\"1.\">1</param>\n"
              "<param name=\"invert\" gui-text=\"" N_("Inverted") "\" type=\"bool\">false</param>\n"
              "<effect>\n"
                "<object-type>all</object-type>\n"
                "<effects-menu>\n"
                  "<submenu name=\"" N_("Filters") "\">\n"
                    "<submenu name=\"" N_("Fill and Transparency") "\"/>\n"
                  "</submenu>\n"
                "</effects-menu>\n"
                "<menu-tip>" N_("Make the lightest parts of the object progressively transparent") "</menu-tip>\n"
              "</effect>\n"
            "</inkscape-extension>\n", std::make_unique<LightEraser>());
        // clang-format on
    };
};

gchar const *
LightEraser::get_filter_text (Inkscape::Extension::Extension * ext)
{
    if (_filter != nullptr) g_free((void *)_filter);

    std::ostringstream expand;
    std::ostringstream erode;
    std::ostringstream opacity;

    opacity << ext->get_param_float("opacity");

    if (ext->get_param_bool("invert")) {
        expand << (ext->get_param_float("erode") * 0.2125) << " "
               << (ext->get_param_float("erode") * 0.7154) << " "
               << (ext->get_param_float("erode") * 0.0721);
        erode << (-ext->get_param_float("expand"));
    } else {
        expand << (-ext->get_param_float("erode") * 0.2125) << " "
               << (-ext->get_param_float("erode") * 0.7154) << " "
               << (-ext->get_param_float("erode") * 0.0721);
        erode << ext->get_param_float("expand");
    }

    // clang-format off
    _filter = g_strdup_printf(
        "<filter xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\" inkscape:label=\"Light Eraser\" style=\"color-interpolation-filters:sRGB;\" >\n"
          "<feColorMatrix values=\"1 0 0 0 0 0 1 0 0 0 0 0 1 0 0 %s %s 0 \" result=\"colormatrix\" />\n"
          "<feComposite in2=\"colormatrix\" operator=\"arithmetic\" k2=\"%s\" result=\"composite\" />\n"
        "</filter>\n", expand.str().c_str(), erode.str().c_str(), opacity.str().c_str());
    // clang-format on

    return _filter;
}; /* Light Eraser filter */


/**
    \brief    Custom predefined Opacity filter.
    
    Set opacity and strength of opacity boundaries.

    Filter's parameters:
    * Expansion (0.->1000., default 5) -> colormatrix (last-1th value)
    * Erosion (0.->1000., default 1) -> colormatrix (last value)
    * Global opacity (0.->1., default 1.) -> composite (k2)
    
*/
class Opacity : public Inkscape::Extension::Internal::Filter::Filter {
protected:
    gchar const * get_filter_text (Inkscape::Extension::Extension * ext) override;

public:
    Opacity ( ) : Filter() { };
    ~Opacity ( ) override { if (_filter != nullptr) g_free((void *)_filter); return; }
    
    static void init () {
        // clang-format off
        Inkscape::Extension::build_from_mem(
            "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
              "<name>" N_("Opacity") "</name>\n"
              "<id>org.inkscape.effect.filter.Opacity</id>\n"
              "<param name=\"expand\" gui-text=\"" N_("Expansion") "\" type=\"float\" appearance=\"full\"  min=\"1\" max=\"1000\">5</param>\n"
              "<param name=\"erode\" gui-text=\"" N_("Erosion") "\" type=\"float\" appearance=\"full\" min=\"0\" max=\"1000\">1</param>\n"
              "<param name=\"opacity\" gui-text=\"" N_("Global opacity") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"0.\" max=\"1.\">1</param>\n"
              "<effect>\n"
                "<object-type>all</object-type>\n"
                "<effects-menu>\n"
                  "<submenu name=\"" N_("Filters") "\">\n"
                    "<submenu name=\"" N_("Fill and Transparency") "\"/>\n"
                  "</submenu>\n"
                "</effects-menu>\n"
                "<menu-tip>" N_("Set opacity and strength of opacity boundaries") "</menu-tip>\n"
              "</effect>\n"
            "</inkscape-extension>\n", std::make_unique<Opacity>());
        // clang-format on
    };
};

gchar const *
Opacity::get_filter_text (Inkscape::Extension::Extension * ext)
{
    if (_filter != nullptr) g_free((void *)_filter);

    std::ostringstream matrix;
    std::ostringstream opacity;

    opacity << ext->get_param_float("opacity");

    matrix << (ext->get_param_float("expand")) << " "
           << (-ext->get_param_float("erode"));

    // clang-format off
    _filter = g_strdup_printf(
        "<filter xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\" inkscape:label=\"Opacity\" style=\"color-interpolation-filters:sRGB;\" >\n"
          "<feColorMatrix values=\"1 0 0 0 0 0 1 0 0 0 0 0 1 0 0 0 0 0 %s \" result=\"colormatrix\" />\n"
          "<feComposite in2=\"colormatrix\" operator=\"arithmetic\" k2=\"%s\" result=\"composite\" />\n"
        "</filter>\n", matrix.str().c_str(), opacity.str().c_str());
    // clang-format on

    return _filter;
}; /* Opacity filter */

/**
    \brief    Custom predefined Silhouette filter.
    
    Repaint anything visible monochrome

    Filter's parameters:
    * Blur (0.01->50., default 0.01) -> blur (stdDeviation)
    * Cutout (boolean, default False) -> composite (false=in, true=out)
    * Color (guint, default 0,0,0,255) -> flood (flood-color, flood-opacity)
*/

class Silhouette : public Inkscape::Extension::Internal::Filter::Filter {
protected:
    gchar const * get_filter_text (Inkscape::Extension::Extension * ext) override;

public:
    Silhouette ( ) : Filter() { };
    ~Silhouette ( ) override { if (_filter != nullptr) g_free((void *)_filter); return; }

    static void init () {
        // clang-format off
        Inkscape::Extension::build_from_mem(
            "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
              "<name>" N_("Silhouette") "</name>\n"
              "<id>org.inkscape.effect.filter.Silhouette</id>\n"
              "<param name=\"blur\" gui-text=\"" N_("Blur") "\" type=\"float\" appearance=\"full\" precision=\"2\" min=\"0.01\" max=\"50.00\">0.01</param>\n"
              "<param name=\"cutout\" gui-text=\"" N_("Cutout") "\" type=\"bool\">false</param>\n"
              "<param name=\"color\" gui-text=\"" N_("Color") "\" type=\"color\">255</param>\n"
              "<effect>\n"
                "<object-type>all</object-type>\n"
                "<effects-menu>\n"
                  "<submenu name=\"" N_("Filters") "\">\n"
                    "<submenu name=\"" N_("Fill and Transparency") "\"/>\n"
                  "</submenu>\n"
                "</effects-menu>\n"
                "<menu-tip>" N_("Repaint anything visible monochrome") "</menu-tip>\n"
              "</effect>\n"
            "</inkscape-extension>\n", std::make_unique<Silhouette>());
        // clang-format on
    };

};

gchar const *
Silhouette::get_filter_text (Inkscape::Extension::Extension * ext)
{
    if (_filter != nullptr) g_free((void *)_filter);

    std::ostringstream cutout;
    std::ostringstream blur;

    auto color = ext->get_param_color("color");
    if (ext->get_param_bool("cutout"))
        cutout << "out";
    else
        cutout << "in";
    blur << ext->get_param_float("blur");

    // clang-format off
    _filter = g_strdup_printf(
        "<filter xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\" style=\"color-interpolation-filters:sRGB;\" inkscape:label=\"Silhouette\">\n"
          "<feFlood flood-opacity=\"%f\" flood-color=\"%s\" result=\"flood\" />\n"
          "<feComposite in=\"flood\" in2=\"SourceGraphic\" operator=\"%s\" result=\"composite\" />\n"
          "<feGaussianBlur stdDeviation=\"%s\" />\n"
        "</filter>\n", color.getOpacity(), color.toString(false).c_str(), cutout.str().c_str(), blur.str().c_str());
    // clang-format on

    return _filter;
}; /* Silhouette filter */

}; /* namespace Filter */
}; /* namespace Internal */
}; /* namespace Extension */
}; /* namespace Inkscape */

/* Change the 'TRANSPARENCY' below to be your file name */
#endif /* SEEN_INKSCAPE_EXTENSION_INTERNAL_FILTER_TRANSPARENCY_H__ */
