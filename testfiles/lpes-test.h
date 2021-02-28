// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * LPE test file wrapper
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2020 Authors
 *
 * Released under GNU GPL version 2 or later, read the file 'COPYING' for more information
 */

#include <gtest/gtest.h>
#include <src/svg/svg.h>
#include <src/inkscape.h>

using namespace Inkscape;

class LPESTest : public ::testing::Test {
   protected:
      void SetUp() override
      {
         // setup hidden dependency
         Application::create(false);
      }
      void pathCompare(const gchar *a, const gchar *b, double precission = 0.001) {
         Geom::PathVector apv = sp_svg_read_pathv(a);
         Geom::PathVector bpv = sp_svg_read_pathv(b);
         size_t totala = apv.curveCount();
         size_t totalb = bpv.curveCount();
         ASSERT_TRUE(totala == totalb);
         std::vector<Geom::Coord> pos;
         for (size_t i = 0; i < apv.curveCount(); i++) {
            Geom::Point pointa = apv.pointAt(float(i)+0.2);
            Geom::Point pointb = bpv.pointAt(float(i)+0.2);
            Geom::Point pointc = apv.pointAt(float(i)+0.4);
            Geom::Point pointd = bpv.pointAt(float(i)+0.4);
            Geom::Point pointe = apv.pointAt(float(i));
            Geom::Point pointf = bpv.pointAt(float(i));
            ASSERT_NEAR(pointa[Geom::X], pointb[Geom::X], precission);
            ASSERT_NEAR(pointa[Geom::Y], pointb[Geom::Y], precission);
            ASSERT_NEAR(pointc[Geom::X], pointd[Geom::X], precission);
            ASSERT_NEAR(pointc[Geom::Y], pointd[Geom::Y], precission);
            ASSERT_NEAR(pointe[Geom::X], pointf[Geom::X], precission);
            ASSERT_NEAR(pointe[Geom::Y], pointf[Geom::Y], precission);
         }
      }
};

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :