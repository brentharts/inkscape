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

#include <testfiles/lpes-test.h>
#include <gtest/gtest.h>
#include <src/svg/svg.h>
#include <src/object/sp-root.h>
#include <src/helper-fns.h>

using namespace Inkscape;

void LPESTest::SetUp()
{
   // setup hidden dependency
   Application::create(false);
}

void LPESTest::pathCompare(const gchar *a, const gchar *b, const gchar *id, double precission) {
   failed.push_back(id);
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
   failed.pop_back();
}

void LPESTest::TearDown( ) { 
   Glib::ustring ids = "";
   for (auto fail : failed) {
      if (ids != "") {
         ids += ",";
      }
      ids += fail;
   }
   if (ids != "") {
      FAIL() << "[FAILED IDS] " << ids; 
   }
}

// you can override custom threshold from svg file using in 
// root svg from global and override with per shape "inkscape:test-threshold"
void LPESTest::testDoc(Glib::ustring svg, double precission) {
   SPDocument *doc = SPDocument::createNewDocFromMem(svg.c_str(), svg.size(), true);
   doc->ensureUpToDate();
   std::vector< SPObject *> objs;
   std::vector<const gchar *> ds;
   for (auto obj : doc->getObjectsByElement("path")) {
      if (obj->getAttribute("d")) {
         ds.push_back(obj->getAttribute("d"));
         objs.push_back(obj);
      }
   }
   for (auto obj : doc->getObjectsByElement("ellipse")) {
      if (obj->getAttribute("d")) {
         ds.push_back(obj->getAttribute("d"));
         objs.push_back(obj);
      }
   }
   for (auto obj : doc->getObjectsByElement("circle")) {
      if (obj->getAttribute("d")) {
         ds.push_back(obj->getAttribute("d"));
         objs.push_back(obj);
      }
   }
   for (auto obj : doc->getObjectsByElement("rect")) {
      if (obj->getAttribute("d")) {
         ds.push_back(obj->getAttribute("d"));
         objs.push_back(obj);
      }
   }
   SPLPEItem *lpeitem = dynamic_cast<SPLPEItem *>(doc->getRoot());
   sp_lpe_item_update_patheffect (lpeitem, false, true);
   if (lpeitem->getAttribute("inkscape:test-threshold")) {
      precission = helperfns_read_number(lpeitem->getAttribute("inkscape:test-threshold"));
   }
   if (doc->getObjectsByElement("clipPath").size() || doc->getObjectsByElement("mask").size()) {
      // we need to double update because clippaths
      sp_lpe_item_update_patheffect (lpeitem, false, true);
   }

   size_t index = 0;
   for (auto obj : objs) {
      if (obj->getAttribute("inkscape:test-threshold")) {
         precission = helperfns_read_number(obj->getAttribute("inkscape:test-threshold"));
      }
      if (!obj->getAttribute("inkscape:test-ignore")) {
         pathCompare(ds[index], obj->getAttribute("d"), obj->getAttribute("id"), precission);
      }
      index++;
   }
}


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
