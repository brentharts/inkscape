// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Test for SVG colors
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2010 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "svg/svg-length.h"
#include "svg/svg.h"

#include <glib.h>
#include <gtest/gtest.h>
#include <utility>

struct test_t
{
    char const *str;
    SVGLength::Unit unit;
    float value;
    float computed;
};

test_t absolute_tests[12] = {
    // clang-format off
    {"0",            SVGLength::NONE,   0        ,   0},
    {"1",            SVGLength::NONE,   1        ,   1},
    {"1.00001",      SVGLength::NONE,   1.00001  ,   1.00001},
    {"1px",          SVGLength::PX  ,   1        ,   1},
    {".1px",         SVGLength::PX  ,   0.1      ,   0.1},
    {"100pt",        SVGLength::PT  , 100        ,  400.0/3.0},
    {"1e2pt",        SVGLength::PT  , 100        ,  400.0/3.0},
    {"3pc",          SVGLength::PC  ,   3        ,  48},
    {"-3.5pc",       SVGLength::PC  ,  -3.5      ,  -3.5*16.0},
    {"1.2345678mm",  SVGLength::MM  ,   1.2345678,   1.2345678f*96.0/25.4}, // TODO: More precise constants? (a 7 digit constant when the default precision is 8 digits?)
    {"123.45678cm", SVGLength::CM   , 123.45678  , 123.45678f*96.0/2.54},   // Note that svg_length_read is casting the result from g_ascii_strtod to float.
    {"73.162987in",  SVGLength::INCH,  73.162987 ,  73.162987f*96.0/1.00}};
test_t relative_tests[3] = {
    {"123em", SVGLength::EM,      123, 123. *  7.},
    {"123ex", SVGLength::EX,      123, 123. * 13.},
    {"123%",  SVGLength::PERCENT, 1.23, 1.23 * 19.}};
const char* fail_tests[8] = {
    "123 px",
    "123e",
    "123e+m",
    "123ec",
    "123pxt",
    "--123",
    "",
    "px"};
// clang-format on

TEST(SvgLengthTest, testRead)
{
    for (size_t i = 0; i < G_N_ELEMENTS(absolute_tests); i++) {
        SVGLength len;
        ASSERT_TRUE( len.read(absolute_tests[i].str)) << absolute_tests[i].str;
        ASSERT_EQ( len.unit, absolute_tests[i].unit) << absolute_tests[i].str;
        ASSERT_EQ( len.value, absolute_tests[i].value) << absolute_tests[i].str;
        ASSERT_EQ( len.computed, absolute_tests[i].computed) << absolute_tests[i].str;
    }
    for (size_t i = 0; i < G_N_ELEMENTS(relative_tests); i++) {
        SVGLength len;
        ASSERT_TRUE( len.read(relative_tests[i].str)) << relative_tests[i].str;
        len.update(7, 13, 19);
        ASSERT_EQ( len.unit, relative_tests[i].unit) << relative_tests[i].str;
        ASSERT_EQ( len.value, relative_tests[i].value) << relative_tests[i].str;
        ASSERT_EQ( len.computed, relative_tests[i].computed) << relative_tests[i].str;
    }
    for (size_t i = 0; i < G_N_ELEMENTS(fail_tests); i++) {
        SVGLength len;
        ASSERT_TRUE( !len.read(fail_tests[i])) << fail_tests[i];
    }
}

TEST(SvgLengthTest, testReadOrUnset)
{
    for (size_t i = 0; i < G_N_ELEMENTS(absolute_tests); i++) {
        SVGLength len;
        len.readOrUnset(absolute_tests[i].str);
        ASSERT_EQ( len.unit, absolute_tests[i].unit) << absolute_tests[i].str;
        ASSERT_EQ( len.value, absolute_tests[i].value) << absolute_tests[i].str;
        ASSERT_EQ( len.computed, absolute_tests[i].computed) << absolute_tests[i].str;
    }
    for (size_t i = 0; i < G_N_ELEMENTS(relative_tests); i++) {
        SVGLength len;
        len.readOrUnset(relative_tests[i].str);
        len.update(7, 13, 19);
        ASSERT_EQ( len.unit, relative_tests[i].unit) << relative_tests[i].str;
        ASSERT_EQ( len.value, relative_tests[i].value) << relative_tests[i].str;
        ASSERT_EQ( len.computed, relative_tests[i].computed) << relative_tests[i].str;
    }
    for (size_t i = 0; i < G_N_ELEMENTS(fail_tests); i++) {
        SVGLength len;
        len.readOrUnset(fail_tests[i], SVGLength::INCH, 123, 456);
        ASSERT_EQ( len.unit, SVGLength::INCH) << fail_tests[i];
        ASSERT_EQ( len.value, 123) << fail_tests[i];
        ASSERT_EQ( len.computed, 456) << fail_tests[i];
    }
}

TEST(SvgLengthTest, testReadAbsolute)
{
    for (size_t i = 0; i < G_N_ELEMENTS(absolute_tests); i++) {
        SVGLength len;
        ASSERT_TRUE( len.readAbsolute(absolute_tests[i].str)) << absolute_tests[i].str;
        ASSERT_EQ( len.unit, absolute_tests[i].unit) << absolute_tests[i].str;
        ASSERT_EQ( len.value, absolute_tests[i].value) << absolute_tests[i].str;
        ASSERT_EQ( len.computed, absolute_tests[i].computed) << absolute_tests[i].str;
    }
    for (size_t i = 0; i < G_N_ELEMENTS(relative_tests); i++) {
        SVGLength len;
        ASSERT_TRUE( !len.readAbsolute(relative_tests[i].str)) << relative_tests[i].str;
    }
    for (size_t i = 0; i < G_N_ELEMENTS(fail_tests); i++) {
        SVGLength len;
        ASSERT_TRUE( !len.readAbsolute(fail_tests[i])) << fail_tests[i];
    }
}

TEST(SvgLengthTest, testToFromString)
{
    SVGLength len;
    ASSERT_TRUE(len.fromString("10", "mm", 3.7795277));
    ASSERT_EQ(len.unit, SVGLength::NONE);
    ASSERT_EQ(len.write(), "10");
    ASSERT_EQ(len.toString("mm", 3.7795277), "10mm");
    ASSERT_EQ(len.toString("in", 3.7795277), "0.3937008in");
    ASSERT_EQ(len.toString("", 3.7795277), "37.795277");
}

struct eq_test_t
{
    char const *a;
    char const *b;
    bool equal;
};
eq_test_t eq_tests[4] = {
    {"", "", true},
    {"1", "1", true},
    {"10mm", "10mm", true},
    {"20mm", "10mm", false},
};

TEST(SvgLengthTest, testEquality)
{
    for (size_t i = 0; i < G_N_ELEMENTS(eq_tests); i++) {
        SVGLength len_a;
        SVGLength len_b;
        len_a.read(eq_tests[i].a);
        len_b.read(eq_tests[i].b);
        if (eq_tests[i].equal) {
            ASSERT_TRUE(len_a == len_b) << eq_tests[i].a << " == " << eq_tests[i].b;
        } else {
            ASSERT_TRUE(len_a != len_b) << eq_tests[i].a << " != " << eq_tests[i].b;
        }
    }
}

TEST(SvgLengthTest, testEnumMappedToString)
{
    for (int i = (static_cast<int>(SVGLength::NONE) + 1); i <= static_cast<int>(SVGLength::LAST_UNIT); i++) {
        SVGLength::Unit target = static_cast<SVGLength::Unit>(i);
        // PX is a special case where we don't have a unit string
        if ((target != SVGLength::PX)) {
            gchar const *val = sp_svg_length_get_css_units(target);
            ASSERT_NE(val, "") << i;
        }
    }
}

TEST(SvgLengthTest, testStringsAreValidSVG)
{
    gchar const *valid[] = {"", "em", "ex", "px", "pt", "pc", "cm", "mm", "in", "%"};
    std::set<std::string> validStrings(valid, valid + G_N_ELEMENTS(valid));
    for (int i = (static_cast<int>(SVGLength::NONE) + 1); i <= static_cast<int>(SVGLength::LAST_UNIT); i++) {
        SVGLength::Unit target = static_cast<SVGLength::Unit>(i);
        gchar const *val = sp_svg_length_get_css_units(target);
        ASSERT_TRUE( validStrings.find(std::string(val)) != validStrings.end()) << i;
    }
}

TEST(SvgLengthTest, testValidSVGStringsSupported)
{
    // Note that "px" is omitted from the list, as it will be assumed to be so if not explicitly set.
    gchar const *valid[] = {"em", "ex", "pt", "pc", "cm", "mm", "in", "%"};
    std::set<std::string> validStrings(valid, valid + G_N_ELEMENTS(valid));
    for (int i = (static_cast<int>(SVGLength::NONE) + 1); i <= static_cast<int>(SVGLength::LAST_UNIT); i++) {
        SVGLength::Unit target = static_cast<SVGLength::Unit>(i);
        gchar const *val = sp_svg_length_get_css_units(target);
        std::set<std::string>::iterator iter = validStrings.find(std::string(val));
        if (iter != validStrings.end()) {
            validStrings.erase(iter);
        }
    }
    ASSERT_EQ(validStrings.size(), 0u) << validStrings.size();
}

TEST(SvgLengthTest, testPlaces)
{
    struct testd_t
    {
        std::string str;
        double val;
        int prec;
        int minexp;
    };

    testd_t const precTests[] = {
        {"7.6e5", 761929.18978947023, 2, -8},
        {"76000", 76192.918978947023, 2, -8},
        {"7600", 7619.2918978947023, 2, -8},
        {"760", 761.92918978947023, 2, -8},
        {"761.9", 761.92918978947023, 4, -8},
        {"76.19", 76.192918978947023, 4, -8},
        {"7.619", 7.6192918978947023, 4, -8},
        {"0.7619", 0.76192918978947023, 4, -8},
        {"0.07619", 0.076192918978947023, 4, -8},
        {"0.007619", 0.0076192918978947023, 4, -8},
        {"7.619e-4", 0.00076192918978947023, 4, -8},
        {"7.619e-5", 0.000076192918978947023, 4, -8},
        {"7.619e-6", 0.0000076192918978947023, 4, -8},
        {"7.619e-7", 0.00000076192918978947023, 4, -8},
        {"7.619e-8", 0.000000076192918978947023, 4, -8},

        // Number smaller than 10^min_exp is truncated
        {"0", 0.0000000076192918978947023, 4, -8},
        {"0", -0.0000000076192918978947023, 4, -8},

        // trailing 0s are omitted
        {"7", 7, 4, -8},
        {"70", 70, 4, -8},
        {"0.7", 0.7, 4, -8},
        {"7e10", 7e10, 4, -8},
        {"7e-8", 7e-8, 4, -8},

        // Exponent of 10 edge cases
        {"1e-7", 1e-7, 4, -8},
        {"1e-6", 1e-6, 4, -8},
        {"1e-5", 1e-5, 4, -8},
        {"1e-4", 1e-4, 4, -8},
        {"0.001", 1e-3, 4, -8},
        {"0.01", 1e-2, 4, -8},
        {"0.1", 1e-1, 4, -8},
        {"1", 1, 4, -8},
        {"10", 10, 4, -8},
        {"100", 100, 4, -8},
        {"1000", 1000, 4, -8},
        {"10000", 1e4, 4, -8},
        {"100000", 1e5, 4, -8},
        {"1000000", 1e6, 4, -8},
        {"1e7", 1e7, 4, -8},
    };

    for (auto test : precTests) {
        std::string res = sp_svg_number_write_de(test.val, test.prec, test.minexp);
        EXPECT_EQ(res.length(), test.str.length()) << "Number of chars written";
        EXPECT_EQ(res, test.str) << "Numeric string written";
    }
}

// TODO: More tests

// vim: filetype=cpp:expandtab:shiftwidth=4:softtabstop=4:fileencoding=utf-8:textwidth=99 :
