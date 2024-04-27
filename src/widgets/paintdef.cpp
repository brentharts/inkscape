// SPDX-License-Identifier: GPL-2.0-or-later OR MPL-1.1 OR LGPL-2.1-or-later
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Eek Color Definition.
 *
 * The Initial Developer of the Original Code is
 * Jon A. Cruz.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "paintdef.h"

#include <cstdint>
#include <cstdio>
#include <glibmm/i18n.h>
#include <glibmm/regex.h>
#include <glibmm/stringutils.h>
#include <glibmm/ustring.h>
#include <string_view>

PaintDef::PaintDef(Rgb8bit const &rgb, std::string description, Glib::ustring tooltip)
    : description(std::move(description))
    , tooltip(std::move(tooltip))
    , type(ColorType::RGB)
    , rgb(rgb)
{
}

std::string PaintDef::get_color_id() const
{
    if (type == ColorType::NONE) {
        return "none";
    }
    if (!description.empty() && description[0] != '#') {
        auto name = Glib::ustring(std::move(description));
        // Convert description to ascii, strip out symbols, remove duplicate dashes and prefixes
        static auto const reg1 = Glib::Regex::create("[^[:alnum:]]");
        name = reg1->replace(name, 0, "-", static_cast<Glib::Regex::MatchFlags>(0));
        static auto const reg2 = Glib::Regex::create("-{2,}");
        name = reg2->replace(name, 0, "-", static_cast<Glib::Regex::MatchFlags>(0));
        static auto const reg3 = Glib::Regex::create("(^-|-$)");
        name = reg3->replace(name, 0, "", static_cast<Glib::Regex::MatchFlags>(0));
        // Move important numbers from the start where they are invalid xml, to the end.
        static auto const reg4 = Glib::Regex::create("^(\\d+)(-?)([^\\d]*)");
        name = reg4->replace(name, 0, "\\3\\2\\1", static_cast<Glib::Regex::MatchFlags>(0));
        return name.lowercase();
    }
    auto [r, g, b] = rgb;
    char buf[12];
    std::snprintf(buf, 12, "rgb%02x%02x%02x", r, g, b);
    return buf;
}

const Glib::ustring& PaintDef::get_tooltip() const {
    return tooltip;
}

std::vector<char> PaintDef::getMIMEData(std::string_view mime_type) const
{
    unsigned const r = rgb[0];
    unsigned const g = rgb[1];
    unsigned const b = rgb[2];

    if (mime_type == mimeTEXT) {
        constexpr size_t buflen = 8;
        char tmp[buflen];
        std::snprintf(tmp, buflen, "#%02x%02x%02x", r, g, b);
        return {tmp, tmp + buflen};
    }
    if (mime_type == mimeX_COLOR) {
        constexpr size_t buflen = 4;
        uint16_t tmp[buflen] = {(uint16_t)((r << 8) | r), (uint16_t)((g << 8) | g), (uint16_t)((b << 8) | b),
                                uint16_t{0xffff}};
        return {(char *)tmp, (char *)tmp + buflen * sizeof(uint16_t) / sizeof(char)};
    }
    if (mime_type == mimeOSWB_COLOR) {
        std::string tmp("<paint>");
        switch (get_type()) {
            case PaintDef::ColorType::NONE:
                tmp += "<nocolor/>";
                break;
            default:
                tmp += std::string("<color name=\"") + description + "\">";
                tmp += "<sRGB r=\"";
                tmp += Glib::Ascii::dtostr(r / 255.0);
                tmp += "\" g=\"";
                tmp += Glib::Ascii::dtostr(g / 255.0);
                tmp += "\" b=\"";
                tmp += Glib::Ascii::dtostr(b / 255.0);
                tmp += "\"/></color>";
        }
        tmp += "</paint>";
        return {tmp.c_str(), tmp.c_str() + tmp.size()};
    }
    return {};
}

bool PaintDef::fromMIMEData(std::string_view mime_type, std::span<char const> data)
{
    if (mime_type == mimeX_COLOR) {
        if (data.size() != 8) {
            return false;
        }
        // Careful about endian issues.
        type = PaintDef::ColorType::RGB;
        auto const vals = reinterpret_cast<uint16_t const *>(data.data());
        rgb[0] = vals[0] >> 8;
        rgb[1] = vals[1] >> 8;
        rgb[2] = vals[2] >> 8;
        return true;
    }
    if (mime_type == mimeOSWB_COLOR) {
        std::string xml(data.data(), data.size());
        if (xml.find("<nocolor/>") != std::string::npos) {
            type = PaintDef::ColorType::NONE;
            rgb = {0, 0, 0};
            return true;
        }
        if (auto pos = xml.find("<sRGB"); pos != std::string::npos) {
            std::string srgb = xml.substr(pos, xml.find(">", pos));
            type = PaintDef::ColorType::RGB;
            if (auto numPos = srgb.find("r="); numPos != std::string::npos) {
                double dbl = Glib::Ascii::strtod(srgb.substr(numPos + 3));
                rgb[0] = static_cast<uint8_t>(255 * dbl);
            }
            if (auto numPos = srgb.find("g="); numPos != std::string::npos) {
                double dbl = Glib::Ascii::strtod(srgb.substr(numPos + 3));
                rgb[1] = static_cast<uint8_t>(255 * dbl);
            }
            if (auto numPos = srgb.find("b="); numPos != std::string::npos) {
                double dbl = Glib::Ascii::strtod(srgb.substr(numPos + 3));
                rgb[2] = static_cast<uint8_t>(255 * dbl);
            }
            if (auto pos = xml.find("<color "); pos != std::string::npos) {
                std::string colorTag = xml.substr(pos, xml.find(">", pos));
                if (auto namePos = colorTag.find("name="); namePos != std::string::npos) {
                    char quote = colorTag[namePos + 5];
                    auto endPos = colorTag.find(quote, namePos + 6);
                    description = colorTag.substr(namePos + 6, endPos - (namePos + 6));
                }
            }
            return true;
        }
    }

    return false;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
