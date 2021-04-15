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

class LPELattice2Test : public LPESTest {};

// INKSCAPE 0.92.5

TEST_F(LPELattice2Test, path_0_92_5)
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
   id="svg8"
   inkscape:version="0.92.5 (2060ec1f9f, 2020-04-08)">
  <defs
     id="defs2">
    <inkscape:path-effect
       effect="lattice2"
       gridpoint0="109.61309,35.440479"
       gridpoint1="173.86904,59.630955"
       gridpoint2="127.75595,87.601192"
       gridpoint3="198.81547,102.72024"
       gridpoint4="131.91368,35.440479"
       gridpoint5="176.51488,35.440479"
       gridpoint6="131.91368,102.72024"
       gridpoint7="176.51488,102.72024"
       gridpoint8x9="154.21428,35.440479"
       gridpoint10x11="154.21428,102.72024"
       gridpoint12="145.14285,74.93899"
       gridpoint13="198.81547,52.260419"
       gridpoint14="109.61309,85.9003"
       gridpoint15="170.08928,82.120538"
       gridpoint16="131.91368,52.260419"
       gridpoint17="176.51488,52.260419"
       gridpoint18="131.91368,85.9003"
       gridpoint19="176.51488,85.9003"
       gridpoint20x21="154.21428,52.260419"
       gridpoint22x23="154.21428,85.9003"
       gridpoint24x26="109.61309,69.08036"
       gridpoint25x27="198.81547,69.08036"
       gridpoint28x30="131.91368,69.08036"
       gridpoint29x31="176.51488,69.08036"
       gridpoint32x33x34x35="154.21428,69.08036"
       id="path-effect836"
       is_visible="true"
       horizontal_mirror="false"
       vertical_mirror="false"
       live_update="true" />
    <inkscape:path-effect
       effect="lattice2"
       gridpoint0="30.238099,159.03868"
       gridpoint1="117.17262,159.03868"
       gridpoint2="30.238099,221.78273"
       gridpoint3="117.17262,221.78273"
       gridpoint4="49.136911,165.46428"
       gridpoint5="98.27381,165.46428"
       gridpoint6="49.136911,215.35713"
       gridpoint7="98.27381,215.35713"
       gridpoint8x9="75.229999,189.86662"
       gridpoint10x11="75.229999,190.95479"
       gridpoint12="18.898813,170.37797"
       gridpoint13="128.51191,170.37797"
       gridpoint14="18.898813,210.44344"
       gridpoint15="128.51191,210.44344"
       gridpoint16="46.302089,170.37797"
       gridpoint17="101.10863,170.37797"
       gridpoint18="46.302089,210.44344"
       gridpoint19="101.10863,210.44344"
       gridpoint20x21="76.550793,157.59327"
       gridpoint22x23="76.550793,223.22814"
       gridpoint24x26="18.898813,190.41071"
       gridpoint25x27="128.51191,190.41071"
       gridpoint28x30="46.302089,190.41071"
       gridpoint29x31="101.10863,190.41071"
       gridpoint32x33x34x35="73.70536,190.41071"
       id="path-effect824"
       is_visible="true"
       horizontal_mirror="true"
       vertical_mirror="true"
       live_update="true" />
    <inkscape:path-effect
       effect="lattice2"
       gridpoint0="34.77381,42.999999"
       gridpoint1="86.93452,42.244047"
       gridpoint2="38.553572,83.82143"
       gridpoint3="89.95833,112.54762"
       gridpoint4="34.962797,28.636904"
       gridpoint5="71.626486,28.636904"
       gridpoint6="34.962797,112.54762"
       gridpoint7="71.626486,112.54762"
       gridpoint8x9="53.294641,28.636904"
       gridpoint10x11="53.294641,112.54762"
       gridpoint12="16.630953,49.614583"
       gridpoint13="89.95833,49.614583"
       gridpoint14="16.630953,91.569941"
       gridpoint15="89.95833,91.569941"
       gridpoint16="34.962797,49.614583"
       gridpoint17="71.626486,49.614583"
       gridpoint18="34.962797,91.569941"
       gridpoint19="71.626486,91.569941"
       gridpoint20x21="53.294641,49.614583"
       gridpoint22x23="53.294641,91.569941"
       gridpoint24x26="16.630953,70.592262"
       gridpoint25x27="89.95833,70.592262"
       gridpoint28x30="34.962797,70.592262"
       gridpoint29x31="71.626486,70.592262"
       gridpoint32x33x34x35="53.294641,70.592262"
       id="path-effect818"
       is_visible="true"
       horizontal_mirror="false"
       vertical_mirror="false"
       live_update="true" />
  </defs>
  <g
     inkscape:label="Layer 1"
     inkscape:groupmode="layer"
     id="layer1">
    <path
       style="fill:#ffff00;stroke:#000000;stroke-width:3;stroke-miterlimit:4;stroke-dasharray:none"
       d="m 34.77381,42.999999 c 17.397857,-0.247271 52.16071,-0.755952 52.16071,-0.755952 1.015721,23.449992 3.02381,70.303573 3.02381,70.303573 C 72.826463,102.98211 38.553572,83.82143 38.553572,83.82143 Z"
       id="path05"
       inkscape:path-effect="#path-effect818"
       inkscape:original-d="M 16.630953,28.636904 H 89.95833 V 112.54762 H 16.630953 Z"
       inkscape:connector-curvature="0" />
    <path
       style="fill:#ffff00;stroke:#000000;stroke-width:3;stroke-miterlimit:4;stroke-dasharray:none"
       id="path04"
       inkscape:path-effect="#path-effect824"
       sodipodi:type="arc"
       sodipodi:cx="73.70536"
       sodipodi:cy="190.41071"
       sodipodi:rx="54.806545"
       sodipodi:ry="40.065475"
       d="m 117.17262,190.41071 c 0,4.11439 -1.31326,8.31717 -3.60751,10.92344 -2.32202,2.63782 -5.45601,3.4437 -9.38918,0.0581 -4.335768,-3.73216 -7.997388,-10.57636 -13.347966,-19.2616 -5.105343,-8.28717 -10.032527,-14.98835 -15.597965,-14.98835 -5.582169,0 -10.47619,6.70118 -15.818499,14.98835 -5.588374,8.66887 -9.731939,15.54581 -14.703035,19.2616 -4.54946,3.40063 -8.180433,2.5647 -10.693282,-0.0581 -2.486821,-2.59563 -3.777084,-6.81969 -3.777084,-10.92344 0,-4.10376 1.290263,-8.32782 3.777084,-10.92344 2.512849,-2.6228 6.143822,-3.45873 10.693282,-0.0581 4.971096,3.71579 9.114661,10.59273 14.703035,19.2616 5.342309,8.28717 10.23633,14.98835 15.818499,14.98835 5.565438,0 10.492622,-6.70118 15.597965,-14.98835 5.350578,-8.68524 9.012198,-15.52944 13.347966,-19.2616 3.93317,-3.3856 7.06716,-2.57972 9.38918,0.0581 2.29425,2.60626 3.60751,6.80904 3.60751,10.92344 z" />
    <g
       id="g03"
       inkscape:path-effect="#path-effect836">
      <path
         id="path02"
         d="m 167.16156,78.414449 c -0.88858,5.999457 0.29613,8.794669 2.84366,13.304491 1.89307,3.351267 2.40728,5.383646 -4.61182,3.890222 -6.59684,-1.403584 -16.63976,-5.678309 -21.52086,-10.126728 -5.2156,-4.753264 -2.5027,-7.097804 -0.49785,-11.369866 1.01993,-2.173344 1.5427,-4.46748 1.14014,-7.644245 -0.40312,-3.181142 -1.62626,-6.458685 -3.02364,-9.929299 -2.78565,-6.918564 -4.19965,-10.678422 2.15577,-8.285794 5.73087,2.157506 15.94831,8.874599 20.56071,14.39578 5.12969,6.140395 3.75521,10.355212 2.95389,15.765439 z"
         style="fill:#ffff00;stroke:#000000;stroke-width:3;stroke-miterlimit:4;stroke-dasharray:none"
         inkscape:original-d="M 186.72022,69.08036 A 29.860117,33.639881 0 0 1 156.86011,102.72024 29.860117,33.639881 0 0 1 126.99999,69.08036 29.860117,33.639881 0 0 1 156.86011,35.440479 29.860117,33.639881 0 0 1 186.72022,69.08036 Z"
         inkscape:connector-curvature="0" />
      <path
         id="path01"
         d="m 172.25466,77.532893 c -0.70122,3.292112 -1.98827,6.993694 -4.1988,8.480626 -2.49769,1.680088 -7.23763,1.692123 -14.20355,0.02223 -6.89939,-1.653942 -13.44271,-4.418104 -16.21117,-6.984107 -2.84598,-2.637849 -1.4848,-4.527739 -0.56675,-7.616419 0.92146,-3.100117 0.53235,-7.780509 1.41496,-11.508273 0.9237,-3.901291 4.02495,-5.244381 11.31945,-3.109419 6.83131,1.999396 15.06688,6.409442 18.99501,10.011753 4.16666,3.82105 4.22061,7.089698 3.45085,10.703606 z"
         style="fill:#ffff00;stroke:#000000;stroke-width:3;stroke-miterlimit:4;stroke-dasharray:none"
         inkscape:original-d="M 198.81547,66.056549 A 44.601189,23.056549 0 0 1 154.21428,89.113098 44.601189,23.056549 0 0 1 109.61309,66.056549 44.601189,23.056549 0 0 1 154.21428,43 44.601189,23.056549 0 0 1 198.81547,66.056549 Z"
         inkscape:connector-curvature="0" />
    </g>
  </g>
</svg>
)"""";

    SPDocument *doc = SPDocument::createNewDocFromMem(svg.c_str(), svg.size(), true);
    doc->ensureUpToDate();

    auto lpeitem01 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path01"));
    auto lpeitem02 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path02"));
    auto lpeitem03 = dynamic_cast<SPLPEItem *>(doc->getObjectById("g03"));
    auto lpeitem04 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path04"));
    auto lpeitem05 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path05"));

    ASSERT_TRUE(lpeitem01 != nullptr);
    ASSERT_TRUE(lpeitem02 != nullptr);
    ASSERT_TRUE(lpeitem03 != nullptr);
    ASSERT_TRUE(lpeitem04 != nullptr);
    ASSERT_TRUE(lpeitem05 != nullptr);
    
    const gchar *d01 = lpeitem01->getAttribute("d");
    const gchar *d02 = lpeitem02->getAttribute("d");
    const gchar *d04 = lpeitem04->getAttribute("d");
    const gchar *d05 = lpeitem05->getAttribute("d");

    sp_lpe_item_update_patheffect (lpeitem03, false, true);
    sp_lpe_item_update_patheffect (lpeitem04, false, true);
    sp_lpe_item_update_patheffect (lpeitem05, false, true);

    pathCompare(d01, lpeitem01->getAttribute("d"));
    pathCompare(d02, lpeitem02->getAttribute("d"));
    pathCompare(d04, lpeitem04->getAttribute("d"));
    pathCompare(d05, lpeitem05->getAttribute("d"));
}


// INKSCAPE 1.0.2

TEST_F(LPELattice2Test, multi_PX_1_0_2)
{
   std::string svg = R""""(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<svg
   xmlns:svg="http://www.w3.org/2000/svg"
   xmlns="http://www.w3.org/2000/svg"
   xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd"
   xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape"
   width="793.70081"
   height="1122.5197"
   viewBox="0 0 793.7008 1122.5197"
   version="1.1"
   id="svg8"
   inkscape:version="1.0.2 (e86c870879, 2021-01-15)">
  <defs
     id="defs2">
    <inkscape:path-effect
       effect="bend_path"
       id="path-effect33"
       is_visible="true"
       lpeversion="1"
       bendpath="m 109.61309,69.080359 c 13.5182,20.818475 10.90902,62.341741 89.20238,0"
       prop_scale="1"
       scale_y_rel="false"
       vertical="false"
       hide_knot="false"
       bendpath-nodetypes="cc" />
    <inkscape:path-effect
       effect="lattice2"
       gridpoint0="120.38541,45.26786"
       gridpoint1="188.04315,45.26786"
       gridpoint2="120.38541,92.892859"
       gridpoint3="188.04315,92.892859"
       gridpoint4="131.91368,35.440479"
       gridpoint5="176.51488,35.440479"
       gridpoint6="131.91368,102.72024"
       gridpoint7="176.51488,102.72024"
       gridpoint8x9="154.21428,35.440479"
       gridpoint10x11="154.21428,102.72024"
       gridpoint12="125.67708,58.875002"
       gridpoint13="182.75148,58.875002"
       gridpoint14="125.67708,79.285717"
       gridpoint15="182.75148,79.285717"
       gridpoint16="131.91368,52.260419"
       gridpoint17="176.51488,52.260419"
       gridpoint18="131.91368,85.9003"
       gridpoint19="176.51488,85.9003"
       gridpoint20x21="154.21428,52.260419"
       gridpoint22x23="154.21428,85.9003"
       gridpoint24x26="109.61309,69.08036"
       gridpoint25x27="198.81547,69.08036"
       gridpoint28x30="131.91368,69.08036"
       gridpoint29x31="176.51488,69.08036"
       gridpoint32x33x34x35="154.21428,69.08036"
       id="path-effect31"
       is_visible="true"
       horizontal_mirror="true"
       vertical_mirror="true"
       live_update="true"
       perimetral="true"
       lpeversion="0" />
    <inkscape:path-effect
       effect="lattice2"
       gridpoint0="109.61309,35.440479"
       gridpoint1="173.86904,59.630955"
       gridpoint2="127.75595,87.601192"
       gridpoint3="198.81547,102.72024"
       gridpoint4="131.91368,35.440479"
       gridpoint5="176.51488,35.440479"
       gridpoint6="131.91368,102.72024"
       gridpoint7="176.51488,102.72024"
       gridpoint8x9="154.21428,35.440479"
       gridpoint10x11="154.21428,102.72024"
       gridpoint12="145.14285,74.93899"
       gridpoint13="198.81547,52.260419"
       gridpoint14="109.61309,85.9003"
       gridpoint15="170.08928,82.120538"
       gridpoint16="131.91368,52.260419"
       gridpoint17="176.51488,52.260419"
       gridpoint18="131.91368,85.9003"
       gridpoint19="176.51488,85.9003"
       gridpoint20x21="154.21428,52.260419"
       gridpoint22x23="154.21428,85.9003"
       gridpoint24x26="109.61309,69.08036"
       gridpoint25x27="198.81547,69.08036"
       gridpoint28x30="131.91368,69.08036"
       gridpoint29x31="176.51488,69.08036"
       gridpoint32x33x34x35="154.21428,69.08036"
       id="path-effect836"
       is_visible="true"
       horizontal_mirror="false"
       vertical_mirror="false"
       live_update="true"
       lpeversion="0"
       perimetral="false" />
    <inkscape:path-effect
       effect="lattice2"
       gridpoint0="30.2381,159.03868"
       gridpoint1="117.17262,159.03868"
       gridpoint2="30.2381,221.78273"
       gridpoint3="117.17262,221.78273"
       gridpoint4="49.136911,165.46428"
       gridpoint5="98.27381,165.46428"
       gridpoint6="49.136911,215.35713"
       gridpoint7="98.27381,215.35713"
       gridpoint8x9="75.229999,189.86662"
       gridpoint10x11="75.229999,190.95479"
       gridpoint12="18.898811,170.37797"
       gridpoint13="128.51191,170.37797"
       gridpoint14="18.898811,210.44344"
       gridpoint15="128.51191,210.44344"
       gridpoint16="46.30209,170.37797"
       gridpoint17="101.10863,170.37797"
       gridpoint18="46.30209,210.44344"
       gridpoint19="101.10863,210.44344"
       gridpoint20x21="108.84218,147.06279"
       gridpoint22x23="108.84218,233.75862"
       gridpoint24x26="18.898811,190.41071"
       gridpoint25x27="128.51191,190.41071"
       gridpoint28x30="46.30209,190.41071"
       gridpoint29x31="101.10863,190.41071"
       gridpoint32x33x34x35="73.70536,190.41071"
       id="path-effect824"
       is_visible="true"
       horizontal_mirror="true"
       vertical_mirror="true"
       live_update="true"
       lpeversion="0"
       perimetral="false" />
    <inkscape:path-effect
       effect="lattice2"
       gridpoint0="34.77381,42.999999"
       gridpoint1="78.995188,59.230301"
       gridpoint2="38.553572,83.82143"
       gridpoint3="89.95833,112.54762"
       gridpoint4="34.962797,28.636904"
       gridpoint5="71.626486,28.636904"
       gridpoint6="34.962797,112.54762"
       gridpoint7="71.626486,112.54762"
       gridpoint8x9="53.294641,28.636904"
       gridpoint10x11="53.294641,112.54762"
       gridpoint12="16.630953,49.614583"
       gridpoint13="89.95833,49.614583"
       gridpoint14="16.630953,91.569941"
       gridpoint15="89.95833,91.569941"
       gridpoint16="44.79254,71.131568"
       gridpoint17="71.626486,49.614583"
       gridpoint18="34.962797,91.569941"
       gridpoint19="71.626486,91.569941"
       gridpoint20x21="55.708534,33.976843"
       gridpoint22x23="50.553546,106.00316"
       gridpoint24x26="16.630953,70.592262"
       gridpoint25x27="89.95833,70.592262"
       gridpoint28x30="34.962797,70.592262"
       gridpoint29x31="58.717158,25.340978"
       gridpoint32x33x34x35="53.294641,70.592262"
       id="path-effect818"
       is_visible="true"
       horizontal_mirror="false"
       vertical_mirror="false"
       live_update="true"
       lpeversion="0"
       perimetral="false" />
  </defs>
  <g
     inkscape:label="Layer 1"
     inkscape:groupmode="layer"
     id="layer1">
    <g
       id="g08"
       inkscape:path-effect="#path-effect31">
      <path
         id="path07"
         d="m 167.16156,69.08036 c 0,4.56117 1.76459,9.413422 0.79533,14.499369 -0.99049,5.197329 -5.26031,9.313128 -11.73582,9.31313 -6.46575,10e-7 -11.18172,-4.115797 -12.68876,-9.313127 -1.30406,-4.497301 -0.15762,-10.795748 -0.15762,-14.499372 0,-3.703624 -1.14644,-10.002071 0.15762,-14.499373 1.50704,-5.19733 6.22301,-9.313128 12.68876,-9.313127 6.47551,2e-6 10.74533,4.115801 11.73582,9.31313 0.96926,5.085947 -0.79533,9.938199 -0.79533,14.49937 z"
         style="fill:#ffff00;stroke:#000000;stroke-width:3;stroke-miterlimit:4;stroke-dasharray:none"
         inkscape:original-d="M 186.72022,69.08036 A 29.860117,33.639881 0 0 1 156.86011,102.72024 29.860117,33.639881 0 0 1 126.99999,69.08036 29.860117,33.639881 0 0 1 156.86011,35.440479 29.860117,33.639881 0 0 1 186.72022,69.08036 Z"
         inkscape:connector-curvature="0" />
      <path
         id="path06"
         d="m 172.10895,67.529675 c -0.48041,2.887616 -1.10783,6.328888 -3.56289,8.746619 -2.67542,2.634739 -7.79711,4.442405 -14.33178,4.442405 -6.53467,0 -11.65636,-1.807666 -14.33178,-4.442405 -2.45506,-2.417731 -3.08248,-5.859003 -3.56289,-8.746619 -0.49463,-2.97303 -0.98965,-6.618626 1.58353,-9.584541 2.68736,-3.097511 8.67136,-5.280327 16.31114,-5.280327 7.63978,0 13.62378,2.182816 16.31114,5.280327 2.57318,2.965915 2.07816,6.611511 1.58353,9.584541 z"
         style="fill:#ffff00;stroke:#000000;stroke-width:3;stroke-miterlimit:4;stroke-dasharray:none"
         inkscape:original-d="M 198.81547,66.056549 A 44.601189,23.056549 0 0 1 154.21428,89.113098 44.601189,23.056549 0 0 1 109.61309,66.056549 44.601189,23.056549 0 0 1 154.21428,43 44.601189,23.056549 0 0 1 198.81547,66.056549 Z"
         inkscape:connector-curvature="0" />
    </g>
    <path
       style="fill:#ffff00;stroke:#000000;stroke-width:3;stroke-miterlimit:4;stroke-dasharray:none"
       d="m 34.77381,42.999999 c 14.75205,5.419055 44.221378,16.230302 44.221378,16.230302 -0.587152,-2.851166 7.444692,36.187571 6.85754,33.336405 C 88.13326,103.66351 89.95833,112.54762 89.95833,112.54762 72.828301,102.98058 38.553572,83.82143 38.553572,83.82143 Z"
       id="path05"
       inkscape:path-effect="#path-effect818"
       inkscape:original-d="M 16.630953,28.636904 H 89.95833 V 112.54762 H 16.630953 Z"
       inkscape:connector-curvature="0" />
    <path
       style="fill:#ffff00;stroke:#000000;stroke-width:3;stroke-miterlimit:4;stroke-dasharray:none"
       id="path04"
       inkscape:path-effect="#path-effect824"
       sodipodi:type="arc"
       sodipodi:cx="73.70536"
       sodipodi:cy="190.41071"
       sodipodi:rx="54.806545"
       sodipodi:ry="40.065475"
       d="m 117.17262,190.41071 c 0,4.16018 -1.41206,8.2956 -3.01639,10.99721 -1.66435,2.80267 -3.19155,3.81812 -5.94387,0.9151 -2.84172,-2.99731 -7.59882,-10.79761 -13.939094,-19.15437 -6.595576,-8.69325 -13.5232,-16.02636 -19.043267,-16.02636 -5.651193,0 -8.497467,7.33311 -12.373196,16.02636 -4.04928,9.08253 -7.956344,15.84539 -14.111913,19.15437 -5.671396,3.0487 -11.026968,1.74189 -14.138586,-0.9151 -3.152831,-2.69219 -4.368204,-6.88745 -4.368204,-10.99721 0,-4.10977 1.215373,-8.30503 4.368204,-10.99721 3.111618,-2.657 8.46719,-3.9638 14.138586,-0.9151 6.155569,3.30897 10.062633,10.07183 14.111913,19.15436 3.875729,8.69325 6.722003,16.02636 12.373196,16.02636 5.520067,0 12.447691,-7.33311 19.043267,-16.02636 6.340274,-8.35676 11.097374,-16.15705 13.939094,-19.15436 2.75232,-2.90303 4.27952,-1.88758 5.94387,0.9151 1.60433,2.7016 3.01639,6.83702 3.01639,10.99721 z" />
    <g
       id="g03"
       inkscape:path-effect="#path-effect836;#path-effect33"
       transform="translate(13.484028,73.20516)">
      <path
         id="path02"
         d="m 169.41039,101.06185 c 1.79896,5.84358 4.51378,7.53625 10.159,9.50084 2.07923,0.72359 3.57593,1.39595 3.27758,2.43124 -0.28549,0.99066 -2.39452,2.48946 -7.62685,4.40446 -3.46376,1.26772 -7.6862,2.41916 -12.23196,3.20625 -6.88862,1.37364 -14.99611,1.43321 -22.08421,-0.22967 -2.23251,-0.47184 -4.23735,-1.21049 -5.91017,-2.0581 -5.92422,-3.00177 -6.1617,-5.73356 -5.32136,-6.95714 1.01876,-1.48338 3.60742,-2.54471 5.45958,-4.47844 1.93114,-2.01621 2.49387,-4.29063 2.16926,-7.505847 -0.32371,-3.206335 -1.31138,-6.608606 -1.07891,-10.085677 0.24007,-3.590669 1.67556,-5.77507 2.28776,-7.245403 0.96046,-2.306771 -0.45635,-0.969363 -0.45615,-0.897536 8.7e-4,0.301114 0.13372,0.722002 0.47008,1.281408 1.75213,1.953987 5.19365,3.097558 10.43433,4.320754 3.45356,0.727747 7.0421,1.442001 9.65846,2.247728 4.24815,1.308246 6.52061,2.847634 7.99765,4.940629 1.46236,2.0722 1.9763,4.462135 2.79591,7.124504 z"
         style="fill:#ffff00;stroke:#000000;stroke-width:3;stroke-miterlimit:4;stroke-dasharray:none"
         inkscape:original-d="M 186.72022,69.08036 A 29.860117,33.639881 0 0 1 156.86011,102.72024 29.860117,33.639881 0 0 1 126.99999,69.08036 29.860117,33.639881 0 0 1 156.86011,35.440479 29.860117,33.639881 0 0 1 186.72022,69.08036 Z"
         inkscape:connector-curvature="0" />
      <path
         id="path01"
         d="m 175.06724,96.765512 c 0.89635,3.301888 1.26067,7.410448 -0.8548,10.298648 -1.87906,2.56543 -6.43204,5.4256 -13.6757,7.6249 0,0 0,0 0,0 -1.7562,0.56664 -3.79078,1.08739 -6.08414,1.48826 -4.47098,0.78152 -9.18371,0.96704 -13.78204,0.47642 0,0 0,-1e-5 0,-1e-5 -6.74675,-0.6252 -12.26737,-3.67164 -15.4461,-6.58864 -0.68665,-0.68178 -1.22989,-1.33226 -1.64593,-1.92521 -1.82329,-2.59856 -1.40249,-4.03442 -0.61718,-4.79364 0.86295,-0.83427 2.37267,-1.26336 3.78165,-2.18401 2.06262,-1.347774 5.36583,-6.809166 7.00525,-9.206536 0.17575,-0.256999 0.34636,-0.499948 0.51035,-0.730531 0.73647,-1.027029 1.39064,-1.838495 1.94893,-2.205816 0.61372,-0.403785 1.29299,-0.4599 2.72509,-0.07654 0,0 0,1e-6 0,1e-6 0.73486,0.155873 1.61346,0.317803 2.72399,0.450393 2.40022,0.28657 5.10264,0.352402 8.55287,0.374879 5.46203,-0.04738 11.83153,-0.223098 15.46394,0.145506 6.57136,0.666841 8.38032,3.118463 9.39382,6.85192 0,1e-6 0,2e-6 0,3e-6 z"
         style="fill:#ffff00;stroke:#000000;stroke-width:3;stroke-miterlimit:4;stroke-dasharray:none"
         inkscape:original-d="M 198.81547,66.056549 A 44.601189,23.056549 0 0 1 154.21428,89.113098 44.601189,23.056549 0 0 1 109.61309,66.056549 44.601189,23.056549 0 0 1 154.21428,43 44.601189,23.056549 0 0 1 198.81547,66.056549 Z"
         inkscape:connector-curvature="0" />
    </g>
  </g>
</svg>
)"""";

    SPDocument *doc = SPDocument::createNewDocFromMem(svg.c_str(), svg.size(), true);
    doc->ensureUpToDate();

    auto lpeitem01 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path01"));
    auto lpeitem02 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path02"));
    auto lpeitem03 = dynamic_cast<SPLPEItem *>(doc->getObjectById("g03"));
    auto lpeitem04 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path04"));
    auto lpeitem05 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path05"));
    auto lpeitem06 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path06"));
    auto lpeitem07 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path07"));
    auto lpeitem08 = dynamic_cast<SPLPEItem *>(doc->getObjectById("g08"));

    ASSERT_TRUE(lpeitem01 != nullptr);
    ASSERT_TRUE(lpeitem02 != nullptr);
    ASSERT_TRUE(lpeitem03 != nullptr);
    ASSERT_TRUE(lpeitem04 != nullptr);
    ASSERT_TRUE(lpeitem05 != nullptr);
    ASSERT_TRUE(lpeitem06 != nullptr);
    ASSERT_TRUE(lpeitem07 != nullptr);
    ASSERT_TRUE(lpeitem08 != nullptr);
    
    const gchar *d01 = lpeitem01->getAttribute("d");
    const gchar *d02 = lpeitem02->getAttribute("d");
    const gchar *d04 = lpeitem04->getAttribute("d");
    const gchar *d05 = lpeitem05->getAttribute("d");
    const gchar *d06 = lpeitem06->getAttribute("d");
    const gchar *d07 = lpeitem07->getAttribute("d");

    sp_lpe_item_update_patheffect (lpeitem03, false, true);
    sp_lpe_item_update_patheffect (lpeitem04, false, true);
    sp_lpe_item_update_patheffect (lpeitem05, false, true);
    sp_lpe_item_update_patheffect (lpeitem08, false, true);

    pathCompare(d01, lpeitem01->getAttribute("d"));
    pathCompare(d02, lpeitem02->getAttribute("d"));
    pathCompare(d04, lpeitem04->getAttribute("d"));
    pathCompare(d05, lpeitem05->getAttribute("d"));
    pathCompare(d06, lpeitem06->getAttribute("d"));
    pathCompare(d07, lpeitem07->getAttribute("d"));
}

TEST_F(LPELattice2Test, multi_MM_1_0_2)
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
   id="svg8"
   inkscape:version="1.0.2 (e86c870879, 2021-01-15)">
  <defs
     id="defs2">
    <inkscape:path-effect
       effect="bend_path"
       id="path-effect33"
       is_visible="true"
       lpeversion="1"
       bendpath="m 109.61309,69.080359 c 13.5182,20.818475 10.90902,62.341741 89.20238,0"
       prop_scale="1"
       scale_y_rel="false"
       vertical="false"
       hide_knot="false"
       bendpath-nodetypes="cc" />
    <inkscape:path-effect
       effect="lattice2"
       gridpoint0="120.38541,45.26786"
       gridpoint1="188.04315,45.26786"
       gridpoint2="120.38541,92.892859"
       gridpoint3="188.04315,92.892859"
       gridpoint4="131.91368,35.440479"
       gridpoint5="176.51488,35.440479"
       gridpoint6="131.91368,102.72024"
       gridpoint7="176.51488,102.72024"
       gridpoint8x9="154.21428,35.440479"
       gridpoint10x11="154.21428,102.72024"
       gridpoint12="125.67708,58.875002"
       gridpoint13="182.75148,58.875002"
       gridpoint14="125.67708,79.285717"
       gridpoint15="182.75148,79.285717"
       gridpoint16="131.91368,52.260419"
       gridpoint17="176.51488,52.260419"
       gridpoint18="131.91368,85.9003"
       gridpoint19="176.51488,85.9003"
       gridpoint20x21="154.21428,52.260419"
       gridpoint22x23="154.21428,85.9003"
       gridpoint24x26="109.61309,69.08036"
       gridpoint25x27="198.81547,69.08036"
       gridpoint28x30="131.91368,69.08036"
       gridpoint29x31="176.51488,69.08036"
       gridpoint32x33x34x35="154.21428,69.08036"
       id="path-effect31"
       is_visible="true"
       horizontal_mirror="true"
       vertical_mirror="true"
       live_update="true"
       perimetral="true" />
    <inkscape:path-effect
       effect="lattice2"
       gridpoint0="109.61309,35.440479"
       gridpoint1="173.86904,59.630955"
       gridpoint2="127.75595,87.601192"
       gridpoint3="198.81547,102.72024"
       gridpoint4="131.91368,35.440479"
       gridpoint5="176.51488,35.440479"
       gridpoint6="131.91368,102.72024"
       gridpoint7="176.51488,102.72024"
       gridpoint8x9="154.21428,35.440479"
       gridpoint10x11="154.21428,102.72024"
       gridpoint12="145.14285,74.93899"
       gridpoint13="198.81547,52.260419"
       gridpoint14="109.61309,85.9003"
       gridpoint15="170.08928,82.120538"
       gridpoint16="131.91368,52.260419"
       gridpoint17="176.51488,52.260419"
       gridpoint18="131.91368,85.9003"
       gridpoint19="176.51488,85.9003"
       gridpoint20x21="154.21428,52.260419"
       gridpoint22x23="154.21428,85.9003"
       gridpoint24x26="109.61309,69.08036"
       gridpoint25x27="198.81547,69.08036"
       gridpoint28x30="131.91368,69.08036"
       gridpoint29x31="176.51488,69.08036"
       gridpoint32x33x34x35="154.21428,69.08036"
       id="path-effect836"
       is_visible="true"
       horizontal_mirror="false"
       vertical_mirror="false"
       live_update="true" />
    <inkscape:path-effect
       effect="lattice2"
       gridpoint0="30.2381,159.03868"
       gridpoint1="117.17262,159.03868"
       gridpoint2="30.2381,221.78273"
       gridpoint3="117.17262,221.78273"
       gridpoint4="49.136911,165.46428"
       gridpoint5="98.27381,165.46428"
       gridpoint6="49.136911,215.35713"
       gridpoint7="98.27381,215.35713"
       gridpoint8x9="75.229999,189.86662"
       gridpoint10x11="75.229999,190.95479"
       gridpoint12="18.898812,170.37797"
       gridpoint13="128.51191,170.37797"
       gridpoint14="18.898812,210.44344"
       gridpoint15="128.51191,210.44344"
       gridpoint16="46.30209,170.37797"
       gridpoint17="101.10863,170.37797"
       gridpoint18="46.30209,210.44344"
       gridpoint19="101.10863,210.44344"
       gridpoint20x21="108.84218,147.06279"
       gridpoint22x23="108.84218,233.75862"
       gridpoint24x26="18.898812,190.41071"
       gridpoint25x27="128.51191,190.41071"
       gridpoint28x30="46.30209,190.41071"
       gridpoint29x31="101.10863,190.41071"
       gridpoint32x33x34x35="73.70536,190.41071"
       id="path-effect824"
       is_visible="true"
       horizontal_mirror="true"
       vertical_mirror="true"
       live_update="true"
       lpeversion="0"
       perimetral="false" />
    <inkscape:path-effect
       effect="lattice2"
       gridpoint0="34.77381,42.999999"
       gridpoint1="78.995188,59.230301"
       gridpoint2="38.553572,83.82143"
       gridpoint3="89.95833,112.54762"
       gridpoint4="34.962797,28.636904"
       gridpoint5="71.626486,28.636904"
       gridpoint6="34.962797,112.54762"
       gridpoint7="71.626486,112.54762"
       gridpoint8x9="53.294641,28.636904"
       gridpoint10x11="53.294641,112.54762"
       gridpoint12="16.630953,49.614583"
       gridpoint13="89.95833,49.614583"
       gridpoint14="16.630953,91.569941"
       gridpoint15="89.95833,91.569941"
       gridpoint16="44.79254,71.131568"
       gridpoint17="71.626486,49.614583"
       gridpoint18="34.962797,91.569941"
       gridpoint19="71.626486,91.569941"
       gridpoint20x21="55.708534,33.976843"
       gridpoint22x23="50.553546,106.00316"
       gridpoint24x26="16.630953,70.592262"
       gridpoint25x27="89.95833,70.592262"
       gridpoint28x30="34.962797,70.592262"
       gridpoint29x31="58.717158,25.340978"
       gridpoint32x33x34x35="53.294641,70.592262"
       id="path-effect818"
       is_visible="true"
       horizontal_mirror="false"
       vertical_mirror="false"
       live_update="true"
       lpeversion="0"
       perimetral="false" />
  </defs>
  <g
     inkscape:label="Layer 1"
     inkscape:groupmode="layer"
     id="layer1">
     <g
       id="g08"
       inkscape:path-effect="#path-effect31">
      <path
         id="path07"
         d="m 167.16156,69.08036 c 0,4.56117 1.76459,9.413422 0.79533,14.499369 -0.99049,5.197329 -5.26031,9.313128 -11.73582,9.31313 -6.46575,10e-7 -11.18172,-4.115797 -12.68876,-9.313127 -1.30406,-4.497302 -0.15762,-10.795748 -0.15762,-14.499372 0,-3.703624 -1.14644,-10.002071 0.15762,-14.499373 1.50704,-5.19733 6.22301,-9.313128 12.68876,-9.313127 6.47551,2e-6 10.74533,4.115801 11.73582,9.31313 0.96926,5.085947 -0.79533,9.938199 -0.79533,14.49937 z"
         style="fill:#ffff00;stroke:#000000;stroke-width:3;stroke-miterlimit:4;stroke-dasharray:none"
         inkscape:original-d="M 186.72022,69.08036 A 29.860117,33.639881 0 0 1 156.86011,102.72024 29.860117,33.639881 0 0 1 126.99999,69.08036 29.860117,33.639881 0 0 1 156.86011,35.440479 29.860117,33.639881 0 0 1 186.72022,69.08036 Z"
         inkscape:connector-curvature="0" />
      <path
         id="path06"
         d="m 172.10895,67.529675 c -0.48041,2.887616 -1.10783,6.328888 -3.56289,8.746619 -2.67542,2.634739 -7.79711,4.442405 -14.33178,4.442405 -6.53467,0 -11.65636,-1.807666 -14.33178,-4.442405 -2.45506,-2.417731 -3.08248,-5.859003 -3.56289,-8.746619 -0.49463,-2.973029 -0.98965,-6.618626 1.58353,-9.584541 2.68736,-3.09751 8.67136,-5.280327 16.31114,-5.280327 7.63978,0 13.62378,2.182817 16.31114,5.280327 2.57318,2.965915 2.07816,6.611512 1.58353,9.584541 z"
         style="fill:#ffff00;stroke:#000000;stroke-width:3;stroke-miterlimit:4;stroke-dasharray:none"
         inkscape:original-d="M 198.81547,66.056549 A 44.601189,23.056549 0 0 1 154.21428,89.113098 44.601189,23.056549 0 0 1 109.61309,66.056549 44.601189,23.056549 0 0 1 154.21428,43 44.601189,23.056549 0 0 1 198.81547,66.056549 Z"
         inkscape:connector-curvature="0" />
    </g>
    <path
       style="fill:#ffff00;stroke:#000000;stroke-width:3;stroke-miterlimit:4;stroke-dasharray:none"
       d="m 34.77381,42.999999 c 14.75205,5.419055 44.221378,16.230302 44.221378,16.230302 -0.587153,-2.851165 7.444692,36.18757 6.85754,33.336405 C 88.13326,103.66351 89.95833,112.54762 89.95833,112.54762 72.828301,102.98058 38.553572,83.82143 38.553572,83.82143 Z"
       id="path05"
       inkscape:path-effect="#path-effect818"
       inkscape:original-d="M 16.630953,28.636904 H 89.95833 V 112.54762 H 16.630953 Z"
       inkscape:connector-curvature="0" />
    <path
       style="fill:#ffff00;stroke:#000000;stroke-width:3;stroke-miterlimit:4;stroke-dasharray:none"
       id="path04"
       inkscape:path-effect="#path-effect824"
       sodipodi:type="arc"
       sodipodi:cx="73.70536"
       sodipodi:cy="190.41071"
       sodipodi:rx="54.806545"
       sodipodi:ry="40.065475"
       d="m 117.17262,190.41071 c 0,4.16018 -1.41206,8.2956 -3.01639,10.99721 -1.66435,2.80267 -3.19155,3.81812 -5.94387,0.9151 -2.84172,-2.99731 -7.59882,-10.79761 -13.939093,-19.15437 -6.595577,-8.69325 -13.523201,-16.02636 -19.043268,-16.02636 -5.651193,0 -8.497468,7.33311 -12.373196,16.02636 -4.049281,9.08253 -7.956344,15.84539 -14.111914,19.15437 -5.671395,3.0487 -11.026968,1.74189 -14.138585,-0.9151 -3.152831,-2.69219 -4.368204,-6.88745 -4.368204,-10.99721 0,-4.10977 1.215373,-8.30503 4.368204,-10.99721 3.111617,-2.657 8.46719,-3.9638 14.138585,-0.9151 6.15557,3.30897 10.062633,10.07183 14.111914,19.15436 3.875728,8.69325 6.722003,16.02636 12.373196,16.02636 5.520067,0 12.447691,-7.33311 19.043268,-16.02636 6.340273,-8.35676 11.097373,-16.15705 13.939093,-19.15436 2.75232,-2.90303 4.27952,-1.88758 5.94387,0.9151 1.60433,2.7016 3.01639,6.83702 3.01639,10.99721 z" />
    
    <g
       id="g03"
       inkscape:path-effect="#path-effect836;#path-effect33"
       transform="translate(13.484028,73.20516)">
      <path
         id="path02"
         d="m 169.41039,101.06185 c 1.79896,5.84358 4.51378,7.53625 10.159,9.50084 2.07923,0.72359 3.57593,1.39595 3.27758,2.43124 -0.28549,0.99066 -2.39452,2.48946 -7.62685,4.40446 -3.46376,1.26772 -7.6862,2.41916 -12.23196,3.20625 -6.88862,1.37364 -14.99611,1.43321 -22.08421,-0.22967 -2.23251,-0.47184 -4.23735,-1.21049 -5.91017,-2.0581 -5.92422,-3.00177 -6.1617,-5.73356 -5.32136,-6.95714 1.01876,-1.48338 3.60742,-2.54471 5.45958,-4.47844 1.93114,-2.01621 2.49387,-4.29063 2.16926,-7.505847 -0.32371,-3.206335 -1.31138,-6.608606 -1.07891,-10.085677 0.24007,-3.590669 1.67556,-5.77507 2.28776,-7.245403 0.96046,-2.306771 -0.45635,-0.969363 -0.45615,-0.897536 8.7e-4,0.301114 0.13372,0.722002 0.47008,1.281408 1.75213,1.953987 5.19365,3.097558 10.43433,4.320754 3.45356,0.727747 7.0421,1.442001 9.65846,2.247728 4.24815,1.308246 6.52061,2.847634 7.99765,4.940629 1.46236,2.0722 1.9763,4.462135 2.79591,7.124504 z"
         style="fill:#ffff00;stroke:#000000;stroke-width:3;stroke-miterlimit:4;stroke-dasharray:none"
         inkscape:original-d="M 186.72022,69.08036 A 29.860117,33.639881 0 0 1 156.86011,102.72024 29.860117,33.639881 0 0 1 126.99999,69.08036 29.860117,33.639881 0 0 1 156.86011,35.440479 29.860117,33.639881 0 0 1 186.72022,69.08036 Z"
         inkscape:connector-curvature="0" />
      <path
         id="path01"
         d="m 175.06724,96.765512 c 0.89635,3.301888 1.26067,7.410448 -0.8548,10.298648 -1.87906,2.56543 -6.43204,5.4256 -13.6757,7.6249 0,0 0,0 0,0 -1.7562,0.56664 -3.79078,1.08739 -6.08414,1.48826 -4.47098,0.78152 -9.18371,0.96704 -13.78204,0.47642 0,0 0,-1e-5 0,-1e-5 -6.74675,-0.6252 -12.26737,-3.67164 -15.4461,-6.58864 -0.68665,-0.68178 -1.22989,-1.33226 -1.64593,-1.92521 -1.82329,-2.59856 -1.40249,-4.03442 -0.61718,-4.79364 0.86295,-0.83427 2.37267,-1.26336 3.78165,-2.18401 2.06262,-1.347774 5.36583,-6.809166 7.00525,-9.206536 0.17575,-0.256999 0.34636,-0.499948 0.51035,-0.730531 0.73647,-1.027029 1.39064,-1.838495 1.94893,-2.205816 0.61372,-0.403785 1.29299,-0.4599 2.72509,-0.07654 0,0 0,1e-6 0,1e-6 0.73486,0.155873 1.61346,0.317803 2.72399,0.450393 2.40022,0.28657 5.10264,0.352402 8.55287,0.374879 5.46203,-0.04738 11.83153,-0.223098 15.46394,0.145506 6.57136,0.666841 8.38032,3.118463 9.39382,6.85192 0,1e-6 0,2e-6 0,3e-6 z"
         style="fill:#ffff00;stroke:#000000;stroke-width:3;stroke-miterlimit:4;stroke-dasharray:none"
         inkscape:original-d="M 198.81547,66.056549 A 44.601189,23.056549 0 0 1 154.21428,89.113098 44.601189,23.056549 0 0 1 109.61309,66.056549 44.601189,23.056549 0 0 1 154.21428,43 44.601189,23.056549 0 0 1 198.81547,66.056549 Z"
         inkscape:connector-curvature="0" />
    </g>
  </g>
</svg>
)"""";

    SPDocument *doc = SPDocument::createNewDocFromMem(svg.c_str(), svg.size(), true);
    doc->ensureUpToDate();

    auto lpeitem01 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path01"));
    auto lpeitem02 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path02"));
    auto lpeitem03 = dynamic_cast<SPLPEItem *>(doc->getObjectById("g03"));
    auto lpeitem04 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path04"));
    auto lpeitem05 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path05"));
    auto lpeitem06 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path06"));
    auto lpeitem07 = dynamic_cast<SPLPEItem *>(doc->getObjectById("path07"));
    auto lpeitem08 = dynamic_cast<SPLPEItem *>(doc->getObjectById("g08"));

    ASSERT_TRUE(lpeitem01 != nullptr);
    ASSERT_TRUE(lpeitem02 != nullptr);
    ASSERT_TRUE(lpeitem03 != nullptr);
    ASSERT_TRUE(lpeitem04 != nullptr);
    ASSERT_TRUE(lpeitem05 != nullptr);
    ASSERT_TRUE(lpeitem06 != nullptr);
    ASSERT_TRUE(lpeitem07 != nullptr);
    ASSERT_TRUE(lpeitem08 != nullptr);
    
    const gchar *d01 = lpeitem01->getAttribute("d");
    const gchar *d02 = lpeitem02->getAttribute("d");
    const gchar *d04 = lpeitem04->getAttribute("d");
    const gchar *d05 = lpeitem05->getAttribute("d");
    const gchar *d06 = lpeitem06->getAttribute("d");
    const gchar *d07 = lpeitem07->getAttribute("d");

    sp_lpe_item_update_patheffect (lpeitem03, false, true);
    sp_lpe_item_update_patheffect (lpeitem04, false, true);
    sp_lpe_item_update_patheffect (lpeitem05, false, true);
    sp_lpe_item_update_patheffect (lpeitem08, false, true);

    pathCompare(d01, lpeitem01->getAttribute("d"));
    pathCompare(d02, lpeitem02->getAttribute("d"));
    pathCompare(d04, lpeitem04->getAttribute("d"));
    pathCompare(d05, lpeitem05->getAttribute("d"));
    pathCompare(d06, lpeitem06->getAttribute("d"));
    pathCompare(d07, lpeitem07->getAttribute("d"));
}