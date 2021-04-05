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
#include <src/inkscape.h>

using namespace Inkscape;

class LPESTest : public ::testing::Test {
   protected:
      void SetUp() override;
      void pathCompare(const gchar *a, const gchar *b, const gchar *id, double precission = 0.001);
      void TearDown( ) override;
      // you can override custom threshold from svg file using in 
      // root svg from global and override with per shape "inkscape:test-threshold"
      void testDoc(Glib::ustring svg, double precission = 0.001);
      std::vector< const gchar *> failed;
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