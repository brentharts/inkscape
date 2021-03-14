// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * LPE Boolean operation test
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2020 Authors
 *
 * Released under GNU GPL version 2 or later, read the file 'COPYING' for more information
 */

#include <gtest/gtest.h>
#include <testfiles/lpes-test.h>
#include <src/document.h>
#include <src/inkscape.h>
#include <src/object/sp-lpe-item.h>

using namespace Inkscape;
using namespace Inkscape::LivePathEffect;

class LPEInterpolatePointsTest : public LPESTest {};

// INKSCAPE 0.92.5

TEST_F(LPEInterpolatePointsTest, path_0_92_5)
{
   std::string svg = R""""(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<svg
   xmlns:svg="http://www.w3.org/2000/svg"
   xmlns="http://www.w3.org/2000/svg"
   xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd"
   xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape"
   width="210mm"
   height="297mm"
   viewBox="0 0 210 297"
   version="1.1"
   id="svg27"
   inkscape:version="0.92.5 (2060ec1f9f, 2020-04-08)">
  <defs
     id="defs21">
    <inkscape:path-effect
       effect="interpolate_points"
       id="path-effect48"
       is_visible="true"
       interpolator_type="SpiroInterpolator" />
    <inkscape:path-effect
       effect="interpolate_points"
       id="path-effect44"
       is_visible="true"
       interpolator_type="Linear" />
    <inkscape:path-effect
       effect="interpolate_points"
       id="path-effect42"
       is_visible="true"
       interpolator_type="CubicBezierFit" />
    <inkscape:path-effect
       effect="interpolate_points"
       id="path-effect40"
       is_visible="true"
       interpolator_type="CentripetalCatmullRom" />
    <inkscape:path-effect
       effect="interpolate_points"
       id="path-effect32"
       is_visible="true"
       interpolator_type="CubicBezierJohan" />
  </defs>
  <g
     inkscape:label="Layer 1"
     inkscape:groupmode="layer"
     id="layer1">
    <path
       inkscape:original-d="M 34.773811,74.75 99.785714,18.053571 148.92262,74.75 c 14.37149,19.33515 40.82143,61.9881 40.82143,61.9881 0,0 -21.53285,46.44182 -40.82143,59.72023 L 96.005952,242.57143 34.773811,196.45833 48.380954,91.380952 Z"
       inkscape:path-effect="#path-effect44"
       style="fill:#ffff00;stroke:#ff0000;stroke-width:0.26458332"
       d="M 34.773811,74.75 99.785714,18.053571 148.92262,74.75 189.74405,136.7381 148.92262,196.45833 96.005952,242.57143 34.773811,196.45833 48.380954,91.380952 34.773811,74.75"
       id="path05"
       inkscape:connector-curvature="0"
       sodipodi:nodetypes="ccccccccc"
       transform="matrix(0.42453817,0,0,0.42453817,4.1897188,12.67491)" />
    <path
       transform="matrix(0.42453817,0,0,0.42453817,58.61829,221.31777)"
       sodipodi:nodetypes="ccccccccc"
       inkscape:connector-curvature="0"
       id="path04"
       d="M 34.773811,74.75 99.785714,18.053571 C 114.81123,817.30703 133.71669,839.12102 148.92262,74.75 c 3.70507,-186.2461 7.17917,-414.82088 10.79067,-618.61822 3.61151,-203.79735 7.41394,-384.61953 11.48423,-463.92898 4.07029,-79.3095 8.4604,-49.94 12.10323,148.20204 1.82142,99.07101 3.4289,239.42261 4.58048,411.84434 1.15159,172.42174 1.8364,377.163782 1.86282,584.48892 0.0267,209.1746 -0.61764,417.41955 -1.74475,593.6509 -1.1271,176.23135 -2.72569,320.6694 -4.54915,423.1939 -3.64691,205.0488 -8.08204,238.4334 -12.19015,159.1971 -4.10812,-79.2363 -7.9406,-263.4798 -11.56164,-473.06381 -3.62103,-209.58401 -7.08466,-446.27069 -10.77574,-643.25786 -8.12591,-433.66758 -17.32333,-667.74216 -26.53395,-663.60853 -9.21062,4.13362 -18.41473,247.67782 -26.382718,709.72163 -6.87445,398.63242 -12.720285,948.48837 -18.879662,1450.59307 -6.159376,502.1048 -12.88988,969.5892 -20.613277,1156.6182 -3.861698,93.5145 -7.931641,113.5866 -11.815677,29.8324 -3.884035,-83.7542 -7.572756,-274.4447 -10.302106,-563.1982 -2.72935,-288.7535 -4.446913,-677.6421 -4.466293,-1074.9686 -0.01938,-397.32649 1.73572,-797.59617 4.844874,-1044.98997 1.889446,-150.342262 4.235045,-242.560169 6.642477,-261.150888 2.407431,-18.590718 4.866802,36.522143 6.964666,156.07351 L 34.773811,74.75"
       style="fill:#ffff00;stroke:#ff0000;stroke-width:0.26458332"
       inkscape:path-effect="#path-effect48"
       inkscape:original-d="M 34.773811,74.75 99.785714,18.053571 148.92262,74.75 c 14.37149,19.33515 40.82143,61.9881 40.82143,61.9881 0,0 -21.53285,46.44182 -40.82143,59.72023 L 96.005952,242.57143 34.773811,196.45833 48.380954,91.380952 Z" />
    <path
       transform="matrix(0.42453817,0,0,0.42453817,102.46353,6.627291)"
       sodipodi:nodetypes="ccccccccc"
       inkscape:connector-curvature="0"
       id="path03"
       d="m 34.773811,74.75 c 77.958849,-67.9873741 51.3127,-82.9014221 114.148809,0 14.94469,19.716942 40.82143,37.24741 40.82143,61.9881 0,24.11292 -24.83372,41.66959 -40.82143,59.72023 -26.68232,30.1252 -83.914868,95.98077 -114.148809,0 -66.552596,-211.278094 52.415383,-57.64509 0,-121.70833"
       style="fill:#ffff00;stroke:#ff0000;stroke-width:0.26458332"
       inkscape:path-effect="#path-effect42"
       inkscape:original-d="M 34.773811,74.75 99.785714,18.053571 148.92262,74.75 c 14.37149,19.33515 40.82143,61.9881 40.82143,61.9881 0,0 -21.53285,46.44182 -40.82143,59.72023 L 96.005952,242.57143 34.773811,196.45833 48.380954,91.380952 Z" />
    <path
       transform="matrix(0.42453817,0,0,0.42453817,1.9218616,141.18681)"
       sodipodi:nodetypes="ccccccccc"
       inkscape:connector-curvature="0"
       id="path02"
       d="M 34.773811,74.75 C 45.609128,65.300595 80.24073,16.687853 99.785714,18.053571 118.01347,19.327247 133.89644,54.910908 148.92262,74.75 c 14.94544,19.732495 40.99742,41.57625 40.82143,61.9881 -0.17374,20.15123 -25.04821,41.98079 -40.82143,59.72023 -15.53716,17.47395 -34.35247,45.45073 -52.916668,46.1131 -19.400194,0.6922 -51.975866,-24.11401 -61.232141,-46.1131 C 23.891281,170.59418 53.917011,110.74431 48.380954,91.380952 45.88785,82.660873 37.041668,77.521825 34.773811,74.75"
       style="fill:#ffff00;stroke:#ff0000;stroke-width:0.26458332"
       inkscape:path-effect="#path-effect40"
       inkscape:original-d="M 34.773811,74.75 99.785714,18.053571 148.92262,74.75 c 14.37149,19.33515 40.82143,61.9881 40.82143,61.9881 0,0 -21.53285,46.44182 -40.82143,59.72023 L 96.005952,242.57143 34.773811,196.45833 48.380954,91.380952 Z" />
    <path
       style="fill:#ffff00;stroke:#ff0000;stroke-width:0.26458332"
       d="m 34.773811,74.75 c 13.002381,0 52.009523,-56.696429 65.011903,-56.696429 9.827386,0 39.309526,56.696429 49.136906,56.696429 8.16429,0 32.65714,61.9881 40.82143,61.9881 -8.16429,0 -32.65714,59.72023 -40.82143,59.72023 -10.58333,0 -42.33333,46.1131 -52.916668,46.1131 -12.246428,0 -48.985713,-46.1131 -61.232141,-46.1131 2.721429,0 10.885714,-105.077378 13.607143,-105.077378 C 45.659525,91.380952 37.49524,74.75 34.773811,74.75"
       id="path01"
       inkscape:path-effect="#path-effect32"
       inkscape:original-d="M 34.773811,74.75 99.785714,18.053571 148.92262,74.75 c 14.37149,19.33515 40.82143,61.9881 40.82143,61.9881 0,0 -21.53285,46.44182 -40.82143,59.72023 L 96.005952,242.57143 34.773811,196.45833 48.380954,91.380952 Z"
       inkscape:connector-curvature="0"
       sodipodi:nodetypes="ccccccccc"
       transform="matrix(0.42453817,0,0,0.42453817,110.02305,128.33562)" />
  </g>
</svg>
)"""";

    SPDocument *doc = SPDocument::createNewDocFromMem(svg.c_str(), svg.size(), true);
    doc->ensureUpToDate();

    auto lpeitem01 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path01"));
    auto lpeitem02 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path02"));
    auto lpeitem03 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path03"));
    auto lpeitem04 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path04"));
    auto lpeitem05 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path05"));

    ASSERT_TRUE(lpeitem01 != nullptr);
    ASSERT_TRUE(lpeitem02 != nullptr);
    ASSERT_TRUE(lpeitem03 != nullptr);
    ASSERT_TRUE(lpeitem04 != nullptr);
    ASSERT_TRUE(lpeitem05 != nullptr);

    const gchar *d01 = lpeitem01->getAttribute("d");
    const gchar *d02 = lpeitem02->getAttribute("d");
    const gchar *d03 = lpeitem03->getAttribute("d");
    const gchar *d04 = lpeitem04->getAttribute("d");
    const gchar *d05 = lpeitem05->getAttribute("d");

    sp_lpe_item_update_patheffect (lpeitem01, false, true);
    sp_lpe_item_update_patheffect (lpeitem02, false, true);
    sp_lpe_item_update_patheffect (lpeitem03, false, true);
    sp_lpe_item_update_patheffect (lpeitem04, false, true);
    sp_lpe_item_update_patheffect (lpeitem05, false, true);

    pathCompare(d01, lpeitem01->getAttribute("d"));
    pathCompare(d02, lpeitem02->getAttribute("d"));
    pathCompare(d03, lpeitem03->getAttribute("d"));
    pathCompare(d04, lpeitem04->getAttribute("d"));
    pathCompare(d05, lpeitem05->getAttribute("d"));
}


// INKSCAPE 1.0.2

TEST_F(LPEInterpolatePointsTest, multi_PX_1_0_2)
{
   std::string svg = R""""(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<svg
   xmlns:svg="http://www.w3.org/2000/svg"
   xmlns="http://www.w3.org/2000/svg"
   xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd"
   xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape"
   width="250"
   height="250"
   viewBox="0 0 250 250.00001"
   version="1.1"
   id="svg27"
   inkscape:version="1.0.2 (e86c870879, 2021-01-15)">
  <defs
     id="defs21">
    <inkscape:path-effect
       effect="interpolate_points"
       id="path-effect48"
       is_visible="true"
       interpolator_type="SpiroInterpolator"
       lpeversion="0" />
    <inkscape:path-effect
       effect="interpolate_points"
       id="path-effect44"
       is_visible="true"
       interpolator_type="Linear"
       lpeversion="0" />
    <inkscape:path-effect
       effect="interpolate_points"
       id="path-effect42"
       is_visible="true"
       interpolator_type="CubicBezierFit"
       lpeversion="0" />
    <inkscape:path-effect
       effect="interpolate_points"
       id="path-effect40"
       is_visible="true"
       interpolator_type="CentripetalCatmullRom"
       lpeversion="0" />
    <inkscape:path-effect
       effect="interpolate_points"
       id="path-effect32"
       is_visible="true"
       interpolator_type="CubicBezierJohan"
       lpeversion="0" />
  </defs>
  <g
     inkscape:label="Layer 1"
     inkscape:groupmode="layer"
     id="layer1">
    <path
       inkscape:original-d="M 34.773811,74.75 99.785714,18.053571 130.24112,95.961128 c 14.37149,19.335152 59.50293,40.776972 59.50293,40.776972 0,0 -21.53285,46.44182 -40.82143,59.72023 L 96.005952,242.57143 34.773811,196.45833 48.380954,91.380952 Z"
       inkscape:path-effect="#path-effect44"
       style="fill:#ffff00;stroke:#ff0000;stroke-width:0.264583"
       d="M 34.773811,74.75 99.785714,18.053571 130.24112,95.961128 189.74405,136.7381 148.92262,196.45833 96.005952,242.57143 34.773811,196.45833 48.380954,91.380952 34.773811,74.75"
       id="path05"
       inkscape:connector-curvature="0"
       sodipodi:nodetypes="ccccccccc"
       transform="matrix(0.42453817,0,0,0.42453817,4.1897188,12.67491)" />
    <path
       transform="matrix(0.42453817,0,0,0.42453817,58.61829,221.31777)"
       sodipodi:nodetypes="ccccccccc"
       inkscape:connector-curvature="0"
       id="path04"
       d="M 34.773811,74.75 99.785714,18.053571 C 106.95915,506.55326 115.59038,777.99385 124.2686,788.00717 132.94681,798.02049 141.63838,546.56775 148.92262,74.75 c 4.14986,-268.79658 7.81732,-604.56491 11.74671,-904.67256 3.92939,-300.10764 8.24,-569.67094 13.0688,-675.93464 2.4144,-53.1319 4.93675,-63.9261 7.36272,-16.3578 2.42598,47.5683 4.75221,154.8596 6.59089,320.1134 1.83868,165.2539 3.16933,389.60745 3.59216,633.18189 0.42283,243.57444 -0.097,504.911059 -1.53985,705.65781 -1.14397,159.16758 -2.82214,276.26416 -4.65995,344.42111 -1.83782,68.15696 -3.83012,89.0334 -5.78675,77.31372 C 175.38411,535.03358 171.67421,385.62674 168.00138,248.55834 156.37101,-185.48246 144.22794,-514.85458 131.82572,-568.6805 119.4235,-622.50642 106.69919,-387.97832 96.005952,242.57143 89.145288,647.12508 83.309013,1200.32 77.142927,1704.4476 c -6.166085,504.1276 -12.918549,972.1746 -20.663381,1157.9879 -3.872415,92.9066 -7.952494,111.9177 -11.842763,26.7958 -3.89027,-85.1219 -7.581113,-277.4836 -10.306902,-567.8083 -2.725788,-290.3247 -4.434023,-680.6542 -4.44104,-1078.8843 -0.007,-398.23011 1.763099,-798.84182 4.88497,-1046.08037 1.892009,-149.838853 4.236823,-241.715619 6.643135,-260.297694 2.406312,-18.582075 4.864384,36.205736 6.964008,155.220316 L 34.773811,74.75"
       style="fill:#ffff00;stroke:#ff0000;stroke-width:0.264583"
       inkscape:path-effect="#path-effect48"
       inkscape:original-d="M 34.773811,74.75 99.785714,18.053571 148.92262,74.75 c 14.37149,19.33515 40.82143,61.9881 40.82143,61.9881 0,0 -2.45409,98.54183 -21.74267,111.82024 L 96.005952,242.57143 34.773811,196.45833 48.380954,91.380952 Z" />
    <path
       transform="matrix(0.42453817,0,0,0.42453817,102.46353,6.627291)"
       sodipodi:nodetypes="ccccccccc"
       inkscape:connector-curvature="0"
       id="path03"
       d="M 34.773811,74.75 C 98.674757,19.022429 103.7156,-22.536384 176.4255,73.391806 c 13.03371,17.195732 18.02451,42.288644 13.31855,63.346294 -5.25902,23.53243 -24.83372,41.66959 -40.82143,59.72023 -26.68232,30.1252 -83.914868,95.98077 -114.148809,0 -66.552596,-211.278094 52.415383,-57.64509 0,-121.70833"
       style="fill:#ffff00;stroke:#ff0000;stroke-width:0.264583"
       inkscape:path-effect="#path-effect42"
       inkscape:original-d="M 34.773811,74.75 99.785714,18.053571 176.4255,73.391806 c 14.37149,19.33515 13.31855,63.346294 13.31855,63.346294 0,0 -21.53285,46.44182 -40.82143,59.72023 L 96.005952,242.57143 34.773811,196.45833 48.380954,91.380952 Z" />
    <path
       transform="matrix(0.42453817,0,0,0.42453817,1.9218616,141.18681)"
       sodipodi:nodetypes="ccccccccc"
       inkscape:connector-curvature="0"
       id="path02"
       d="m 34.773811,74.75 c 10.835317,-9.449405 53.812326,-61.747756 65.011903,-56.696429 11.778326,5.312359 -14.768811,75.658158 0.591306,95.405249 15.11197,19.42807 83.53673,7.70413 89.36703,23.27928 5.16012,13.78482 -25.04821,41.98079 -40.82143,59.72023 -15.53716,17.47395 -34.35247,45.45073 -52.916668,46.1131 -19.400194,0.6922 -51.975866,-24.11401 -61.232141,-46.1131 C 23.891281,170.59418 53.917011,110.74431 48.380954,91.380952 45.88785,82.660873 37.041668,77.521825 34.773811,74.75"
       style="fill:#ffff00;stroke:#ff0000;stroke-width:0.264583"
       inkscape:path-effect="#path-effect40"
       inkscape:original-d="m 34.773811,74.75 65.011903,-56.696429 0.591306,95.405249 c 14.37149,19.33515 89.36703,23.27928 89.36703,23.27928 0,0 -21.53285,46.44182 -40.82143,59.72023 L 96.005952,242.57143 34.773811,196.45833 48.380954,91.380952 Z" />
    <path
       style="fill:#ffff00;stroke:#ff0000;stroke-width:0.264583"
       d="m 34.773811,74.75 c 13.002381,0 52.009522,-56.696429 65.011903,-56.696429 9.827386,0 39.309526,56.696429 49.136906,56.696429 -3.46857,0 -13.8743,61.7758 -17.34287,61.7758 3.46857,0 13.8743,59.93253 17.34287,59.93253 -10.58333,0 -42.33333,46.1131 -52.916668,46.1131 -12.246428,0 -48.985713,-46.1131 -61.232141,-46.1131 2.721429,0 10.885714,-105.077378 13.607143,-105.077378 C 45.659525,91.380952 37.49524,74.75 34.773811,74.75"
       id="path01"
       inkscape:path-effect="#path-effect32"
       inkscape:original-d="M 34.773811,74.75 99.785714,18.053571 148.92262,74.75 c 14.37149,19.33515 -17.34287,61.7758 -17.34287,61.7758 0,0 36.63145,46.65412 17.34287,59.93253 L 96.005952,242.57143 34.773811,196.45833 48.380954,91.380952 Z"
       inkscape:connector-curvature="0"
       sodipodi:nodetypes="ccccccccc"
       transform="matrix(0.42453817,0,0,0.42453817,110.02305,128.33562)" />
  </g>
</svg>
)"""";

    SPDocument *doc = SPDocument::createNewDocFromMem(svg.c_str(), svg.size(), true);
    doc->ensureUpToDate();

    auto lpeitem01 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path01"));
    auto lpeitem02 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path02"));
    auto lpeitem03 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path03"));
    auto lpeitem04 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path04"));
    auto lpeitem05 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path05"));

    ASSERT_TRUE(lpeitem01 != nullptr);
    ASSERT_TRUE(lpeitem02 != nullptr);
    ASSERT_TRUE(lpeitem03 != nullptr);
    ASSERT_TRUE(lpeitem04 != nullptr);
    ASSERT_TRUE(lpeitem05 != nullptr);

    const gchar *d01 = lpeitem01->getAttribute("d");
    const gchar *d02 = lpeitem02->getAttribute("d");
    const gchar *d03 = lpeitem03->getAttribute("d");
    const gchar *d04 = lpeitem04->getAttribute("d");
    const gchar *d05 = lpeitem05->getAttribute("d");

    sp_lpe_item_update_patheffect (lpeitem01, false, true);
    sp_lpe_item_update_patheffect (lpeitem02, false, true);
    sp_lpe_item_update_patheffect (lpeitem03, false, true);
    sp_lpe_item_update_patheffect (lpeitem04, false, true);
    sp_lpe_item_update_patheffect (lpeitem05, false, true);

    pathCompare(d01, lpeitem01->getAttribute("d"));
    pathCompare(d02, lpeitem02->getAttribute("d"));
    pathCompare(d03, lpeitem03->getAttribute("d"));
    pathCompare(d04, lpeitem04->getAttribute("d"));
    pathCompare(d05, lpeitem05->getAttribute("d"));
}

TEST_F(LPEInterpolatePointsTest, multi_MM_1_0_2)
{
   std::string svg = R""""(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<svg
   xmlns:svg="http://www.w3.org/2000/svg"
   xmlns="http://www.w3.org/2000/svg"
   xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd"
   xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape"
   width="250mm"
   height="250mm"
   viewBox="0 0 250 250"
   version="1.1"
   id="svg27"
   inkscape:version="1.0.2 (e86c870879, 2021-01-15)">
  <defs
     id="defs21">
    <inkscape:path-effect
       effect="interpolate_points"
       id="path-effect48"
       is_visible="true"
       interpolator_type="SpiroInterpolator"
       lpeversion="0" />
    <inkscape:path-effect
       effect="interpolate_points"
       id="path-effect44"
       is_visible="true"
       interpolator_type="Linear"
       lpeversion="0" />
    <inkscape:path-effect
       effect="interpolate_points"
       id="path-effect42"
       is_visible="true"
       interpolator_type="CubicBezierFit"
       lpeversion="0" />
    <inkscape:path-effect
       effect="interpolate_points"
       id="path-effect40"
       is_visible="true"
       interpolator_type="CentripetalCatmullRom"
       lpeversion="0" />
    <inkscape:path-effect
       effect="interpolate_points"
       id="path-effect32"
       is_visible="true"
       interpolator_type="CubicBezierJohan"
       lpeversion="0" />
  </defs>
  <g
     inkscape:label="Layer 1"
     inkscape:groupmode="layer"
     id="layer1">
    <path
       inkscape:original-d="M 34.773811,74.75 99.785714,18.053571 130.24112,95.961128 c 14.37149,19.335152 59.50293,40.776972 59.50293,40.776972 0,0 -21.53285,46.44182 -40.82143,59.72023 L 96.005952,242.57143 34.773811,196.45833 48.380954,91.380952 Z"
       inkscape:path-effect="#path-effect44"
       style="fill:#ffff00;stroke:#ff0000;stroke-width:0.264583"
       d="M 34.773811,74.75 99.785714,18.053571 130.24112,95.961128 189.74405,136.7381 148.92262,196.45833 96.005952,242.57143 34.773811,196.45833 48.380954,91.380952 34.773811,74.75"
       id="path05"
       inkscape:connector-curvature="0"
       sodipodi:nodetypes="ccccccccc"
       transform="matrix(0.42453817,0,0,0.42453817,4.1897188,12.67491)" />
    <path
       transform="matrix(0.42453817,0,0,0.42453817,58.61829,221.31777)"
       sodipodi:nodetypes="ccccccccc"
       inkscape:connector-curvature="0"
       id="path04"
       d="M 34.773811,74.75 99.785714,18.053571 C 106.95915,506.55326 115.59038,777.99385 124.2686,788.00717 132.94681,798.02049 141.63838,546.56775 148.92262,74.75 c 4.14986,-268.79658 7.81732,-604.56491 11.74671,-904.67256 3.92939,-300.10764 8.24,-569.67094 13.0688,-675.93464 2.4144,-53.1319 4.93675,-63.9261 7.36272,-16.3578 2.42598,47.5683 4.75221,154.8596 6.59089,320.1134 1.83868,165.2539 3.16933,389.60745 3.59216,633.18189 0.42283,243.57444 -0.097,504.911059 -1.53985,705.65781 -1.14397,159.16758 -2.82214,276.26416 -4.65995,344.42111 -1.83782,68.15696 -3.83012,89.0334 -5.78675,77.31372 C 175.38411,535.03358 171.67421,385.62674 168.00138,248.55834 156.37101,-185.48246 144.22794,-514.85458 131.82572,-568.6805 119.4235,-622.50642 106.69919,-387.97832 96.005952,242.57143 89.145288,647.12508 83.309013,1200.32 77.142927,1704.4476 c -6.166085,504.1276 -12.918549,972.1746 -20.663381,1157.9879 -3.872415,92.9066 -7.952494,111.9177 -11.842763,26.7958 -3.89027,-85.1219 -7.581113,-277.4836 -10.306902,-567.8083 -2.725788,-290.3247 -4.434023,-680.6542 -4.44104,-1078.8843 -0.007,-398.23011 1.763099,-798.84182 4.88497,-1046.08037 1.892009,-149.838853 4.236823,-241.715619 6.643135,-260.297694 2.406312,-18.582075 4.864384,36.205736 6.964008,155.220316 L 34.773811,74.75"
       style="fill:#ffff00;stroke:#ff0000;stroke-width:0.264583"
       inkscape:path-effect="#path-effect48"
       inkscape:original-d="M 34.773811,74.75 99.785714,18.053571 148.92262,74.75 c 14.37149,19.33515 40.82143,61.9881 40.82143,61.9881 0,0 -2.45409,98.54183 -21.74267,111.82024 L 96.005952,242.57143 34.773811,196.45833 48.380954,91.380952 Z" />
    <path
       transform="matrix(0.42453817,0,0,0.42453817,102.46353,6.627291)"
       sodipodi:nodetypes="ccccccccc"
       inkscape:connector-curvature="0"
       id="path03"
       d="M 34.773811,74.75 C 98.674757,19.022429 103.7156,-22.536384 176.4255,73.391806 c 13.03371,17.195732 18.02451,42.288644 13.31855,63.346294 -5.25902,23.53243 -24.83372,41.66959 -40.82143,59.72023 -26.68232,30.1252 -83.914868,95.98077 -114.148809,0 -66.552596,-211.278094 52.415383,-57.64509 0,-121.70833"
       style="fill:#ffff00;stroke:#ff0000;stroke-width:0.264583"
       inkscape:path-effect="#path-effect42"
       inkscape:original-d="M 34.773811,74.75 99.785714,18.053571 176.4255,73.391806 c 14.37149,19.33515 13.31855,63.346294 13.31855,63.346294 0,0 -21.53285,46.44182 -40.82143,59.72023 L 96.005952,242.57143 34.773811,196.45833 48.380954,91.380952 Z" />
    <path
       transform="matrix(0.42453817,0,0,0.42453817,1.9218616,141.18681)"
       sodipodi:nodetypes="ccccccccc"
       inkscape:connector-curvature="0"
       id="path02"
       d="m 34.773811,74.75 c 10.835317,-9.449405 53.812326,-61.747756 65.011903,-56.696429 11.778326,5.312359 -14.768811,75.658158 0.591306,95.405249 15.11197,19.42807 83.53673,7.70413 89.36703,23.27928 5.16012,13.78482 -25.04821,41.98079 -40.82143,59.72023 -15.53716,17.47395 -34.35247,45.45073 -52.916668,46.1131 -19.400194,0.6922 -51.975866,-24.11401 -61.232141,-46.1131 C 23.891281,170.59418 53.917011,110.74431 48.380954,91.380952 45.88785,82.660873 37.041668,77.521825 34.773811,74.75"
       style="fill:#ffff00;stroke:#ff0000;stroke-width:0.264583"
       inkscape:path-effect="#path-effect40"
       inkscape:original-d="m 34.773811,74.75 65.011903,-56.696429 0.591306,95.405249 c 14.37149,19.33515 89.36703,23.27928 89.36703,23.27928 0,0 -21.53285,46.44182 -40.82143,59.72023 L 96.005952,242.57143 34.773811,196.45833 48.380954,91.380952 Z" />
    <path
       style="fill:#ffff00;stroke:#ff0000;stroke-width:0.264583"
       d="m 34.773811,74.75 c 13.002381,0 52.009522,-56.696429 65.011903,-56.696429 9.827386,0 39.309526,56.696429 49.136906,56.696429 -3.46857,0 -13.8743,61.7758 -17.34287,61.7758 3.46857,0 13.8743,59.93253 17.34287,59.93253 -10.58333,0 -42.33333,46.1131 -52.916668,46.1131 -12.246428,0 -48.985713,-46.1131 -61.232141,-46.1131 2.721429,0 10.885714,-105.077378 13.607143,-105.077378 C 45.659525,91.380952 37.49524,74.75 34.773811,74.75"
       id="path01"
       inkscape:path-effect="#path-effect32"
       inkscape:original-d="M 34.773811,74.75 99.785714,18.053571 148.92262,74.75 c 14.37149,19.33515 -17.34287,61.7758 -17.34287,61.7758 0,0 36.63145,46.65412 17.34287,59.93253 L 96.005952,242.57143 34.773811,196.45833 48.380954,91.380952 Z"
       inkscape:connector-curvature="0"
       sodipodi:nodetypes="ccccccccc"
       transform="matrix(0.42453817,0,0,0.42453817,110.02305,128.33562)" />
  </g>
</svg>
)"""";

    SPDocument *doc = SPDocument::createNewDocFromMem(svg.c_str(), svg.size(), true);
    doc->ensureUpToDate();

    auto lpeitem01 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path01"));
    auto lpeitem02 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path02"));
    auto lpeitem03 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path03"));
    auto lpeitem04 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path04"));
    auto lpeitem05 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path05"));

    ASSERT_TRUE(lpeitem01 != nullptr);
    ASSERT_TRUE(lpeitem02 != nullptr);
    ASSERT_TRUE(lpeitem03 != nullptr);
    ASSERT_TRUE(lpeitem04 != nullptr);
    ASSERT_TRUE(lpeitem05 != nullptr);

    const gchar *d01 = lpeitem01->getAttribute("d");
    const gchar *d02 = lpeitem02->getAttribute("d");
    const gchar *d03 = lpeitem03->getAttribute("d");
    const gchar *d04 = lpeitem04->getAttribute("d");
    const gchar *d05 = lpeitem05->getAttribute("d");

    sp_lpe_item_update_patheffect (lpeitem01, false, true);
    sp_lpe_item_update_patheffect (lpeitem02, false, true);
    sp_lpe_item_update_patheffect (lpeitem03, false, true);
    sp_lpe_item_update_patheffect (lpeitem04, false, true);
    sp_lpe_item_update_patheffect (lpeitem05, false, true);

    pathCompare(d01, lpeitem01->getAttribute("d"));
    pathCompare(d02, lpeitem02->getAttribute("d"));
    pathCompare(d03, lpeitem03->getAttribute("d"));
    pathCompare(d04, lpeitem04->getAttribute("d"));
    pathCompare(d05, lpeitem05->getAttribute("d"));
}