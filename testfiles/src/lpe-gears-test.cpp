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
#include <src/inkscape.h>

using namespace Inkscape;
using namespace Inkscape::LivePathEffect;

class LPEGearsTest : public LPESTest {};

// INKSCAPE 0.92.5

TEST_F(LPEGearsTest, path_0_92_5)
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
   inkscape:version="0.92.5 (2060ec1f9f, 2020-04-08)"
   sodipodi:docname="1.svg"
   inkscape:test-threshold="0.01">
  <defs
     id="defs2">
    <inkscape:path-effect
       effect="gears"
       id="path-effect12"
       is_visible="true"
       teeth="17"
       phi="5.9"
       min_radius="6.2" />
  </defs>
  <g
     inkscape:label="Layer 1"
     inkscape:groupmode="layer"
     id="layer1">
    <path
       style="fill:#ffff00;stroke:#ff0000;stroke-width:0.26458332"
       id="path01"
       inkscape:path-effect="#path-effect12"
       sodipodi:type="arc"
       sodipodi:cx="76.729164"
       sodipodi:cy="103.47619"
       sodipodi:rx="49.514877"
       sodipodi:ry="59.720234"
       d="m 120.26727,99.48557 c 0,0 1.97572,-2.880358 8.12583,-5.919733 2.50265,1.856896 4.90337,3.847367 7.19171,5.962746 -1.84757,6.606667 -4.31211,9.081747 -4.31211,9.081747 l -6.16059,6.1654 c 2.97301,2.97069 5.6661,6.20888 8.04516,9.67354 l 7.18498,-4.93367 c 0,0 2.88282,-1.97214 9.71557,-2.5846 1.66286,2.63557 3.18243,5.35887 4.55207,8.15804 -4.1094,5.49312 -7.30162,6.91077 -7.30162,6.91077 l -7.97178,3.5236 c 1.69911,3.84406 3.04057,7.83644 4.0074,11.92655 l 8.48204,-2.005 c 0,0 3.40057,-0.79757 9.99317,1.09961 0.59849,3.05828 1.03168,6.14661 1.29766,9.25154 -5.81626,3.6377 -9.30502,3.80645 -9.30502,3.80645 l -8.70634,0.40592 c 0.19574,4.19827 0.004,8.40564 -0.57158,12.56882 l 8.63356,1.19446 c 0,0 3.45905,0.48471 8.92113,4.6353 -0.5467,3.06797 -1.25841,6.10423 -2.13201,9.09558 -6.73759,1.29097 -10.05172,0.18804 -10.05172,0.18804 l -8.26506,-2.76658 c -1.33407,3.98548 -3.03236,7.83962 -5.07336,11.5136 l 7.61907,4.2326 c 0,0 3.05036,1.70154 6.64423,7.54497 -1.61805,2.66331 -3.37852,5.23744 -5.27374,7.7112 -6.74896,-1.23009 -9.44088,-3.45575 -9.44088,-3.45575 l -6.70752,-5.56544 c -2.68371,3.23442 -5.65959,6.21481 -8.88996,8.9034 l 5.57557,6.69911 c 0,0 2.22972,2.68855 3.47001,9.43564 -2.47089,1.89895 -5.04237,3.66331 -7.70323,5.28539 -5.84885,-3.58504 -7.55499,-6.63283 -7.55499,-6.63283 l -4.24412,-7.61266 c -3.67088,2.04655 -7.52246,3.75066 -11.505912,5.09075 l 2.779062,8.26086 c 0,0 1.10794,3.31247 -0.17286,10.052 -2.990015,0.87812 -6.025202,1.59441 -9.092343,2.14575 -4.158831,-5.4558 -4.648769,-8.91411 -4.648769,-8.91411 l -1.207508,-8.63175 c -4.162299,0.58227 -8.369379,0.77996 -12.567938,0.59057 l -0.392766,8.70694 c 0,0 -0.163484,3.48901 -3.792385,9.31076 -3.105326,-0.26129 -6.194308,-0.68981 -9.253496,-1.28369 -1.907133,-6.58972 -1.1147,-9.99148 -1.1147,-9.99148 l 1.992178,-8.48507 c -4.091569,-0.96064 -8.085968,-2.29607 -11.93259,-3.98937 l -3.511553,7.97709 c 0,0 -1.412823,3.19436 -6.899728,7.31206 -2.801242,-1.36542 -5.526833,-2.88087 -8.164909,-4.53975 0.602135,-6.83367 2.569916,-9.71946 2.569916,-9.71946 l 4.922809,-7.19243 c -3.468249,-2.37382 -6.710502,-5.06201 -9.68568,-8.03053 l -6.156085,6.1699 c 0,0 -2.471351,2.46828 -9.075224,4.32582 -2.118834,-2.28514 -4.1129299,-4.68285 -5.9736051,-7.18269 3.0300801,-6.15469 5.9074501,-8.13477 5.9074501,-8.13477 l 7.188587,-4.92841 c -2.376523,-3.4664 -4.428745,-7.14431 -6.130665,-10.98712 l -7.9692034,3.52942 c 0,0 -3.1961107,1.40885 -10.0250615,0.75536 -1.1502664,-2.89624 -2.1435522,-5.85239 -2.975533,-8.85557 5.0487963,-4.64449 8.4471503,-5.45143 8.4471503,-5.45143 l 8.4835066,-1.9988 C 9.1391104,174.80337 8.5540833,170.63247 8.3552761,166.43435 l -8.70603751,0.41228 c 0,0 -3.48921739,0.15914 -9.62095829,-2.91712 -0.026349,-3.11618 0.1153239,-6.23153 0.4244011,-9.33246 6.3856449,-2.50702 9.84601626,-2.03184 9.84601626,-2.03184 L 8.93138,153.76598 c 0.5790245,-4.16275 1.540203,-8.26333 2.871358,-12.24978 l -8.2670723,-2.76054 c 0,0 -3.31108598,-1.11206 -7.9174915,-6.19562 1.1011276,-2.91528 2.358625,-5.76907 3.76701758,-8.54895 6.86007672,-0.031 9.91512322,1.66215 9.91512322,1.66215 l 7.615966,4.23817 c 2.043683,-3.67248 4.421256,-7.14893 7.102593,-10.38532 l -6.711594,-5.56054 c 0,0 -2.685774,-2.23306 -5.144723,-8.63737 2.07989,-2.32065 4.28338,-4.52747 6.600877,-6.610861 6.408017,2.449271 8.645138,5.131671 8.645138,5.131671 l 5.570671,6.70318 c 3.232332,-2.68622 6.705193,-5.06904 10.374584,-7.11827 L 39.10415,95.824342 c 0,0 -1.697734,-3.052485 -1.677133,-9.912601 2.777753,-1.412591 5.629642,-2.674398 8.543253,-3.779929 5.090519,4.59872 6.207578,7.908122 6.207578,7.908122 l 2.773027,8.262892 c 3.984435,-1.337176 8.083557,-2.304549 12.245428,-2.889862 l -1.213817,-8.630859 c 0,0 -0.480405,-3.459649 2.016965,-9.849074 3.100464,-0.313762 6.215589,-0.460142 9.331813,-0.438501 3.085519,6.127086 2.931652,9.61654 2.931652,9.61654 l -0.399131,8.706651 c 4.198419,0.192464 8.370191,0.771189 12.462461,1.728842 l 1.985974,-8.486517 c 0,0 0.801806,-3.399569 5.43866,-8.455377 3.00444,0.827443 5.96209,1.816261 8.86006,2.96215 0.6638,6.827956 -0.74021,10.026191 -0.74021,10.026191 l -3.51738,7.97453 c 3.84538,1.69611 7.52638,3.74277 10.99637,6.11406 l 4.91755,-7.19603 M 36.035672,40.152209 c 0,0 -3.698766,-0.350277 -8.672081,-5.494786 0.491361,-2.665963 1.197876,-5.287743 2.112613,-7.839616 6.884861,-1.948978 10.2588,-0.393308 10.2588,-0.393308 l 8.067696,3.779648 c 1.577511,-3.367214 3.730822,-6.43314 6.362934,-9.05967 l -6.29305,-6.306424 c 0,0 -2.608266,-2.645849 -3.11122,-9.7835566 2.090053,-1.7264053 4.316523,-3.2806659 6.657564,-4.64753284 6.526888,2.93249994 8.11151,6.29293794 8.11151,6.29293794 l 3.750703,8.0811935 C 66.653988,13.215664 70.274259,12.25115 73.978875,11.931 L 73.21181,3.0549047 c 0,0 -0.297329,-3.70339913 3.905416,-9.4944928 2.710786,0.020957 5.41542,0.2614712 8.087366,0.7191826 3.114913,6.44182857 2.168756,10.0346487 2.168756,10.0346487 l -2.321287,8.6014588 c 3.589991,0.968835 6.983257,2.557039 10.026946,4.693071 l 5.117843,-7.292543 c 0,0 2.15273,-3.0280878 9.09466,-4.7628503 2.06311,1.7585136 3.98038,3.6812631 5.733,5.7493843 -1.75457,6.936954 -4.78878,9.081036 -4.78878,9.081036 l -7.30712,5.097005 c 2.12733,3.049773 3.70585,6.447558 4.66443,10.040299 l 8.60805,-2.29673 c 0,0 3.59551,-0.9359 10.02842,2.197383 0.45008,2.673242 0.68288,5.378551 0.6961,8.089385 -5.80306,4.186202 -9.5056,3.878306 -9.5056,3.878306 l -8.87387,-0.792393 c -0.33072,3.703687 -1.30556,7.321191 -2.88061,10.689557 l 8.07045,3.77375 c 0,0 3.35591,1.594205 6.26977,8.129436 -1.37354,2.337131 -2.93415,4.559155 -4.66652,6.644273 -7.13624,-0.523321 -9.77463,-3.139127 -9.77463,-3.139127 l -6.288442,-6.311022 c -2.634031,2.624605 -5.70609,4.769158 -9.077792,6.337054 l 3.756609,8.078449 c 0,0 1.546035,3.378365 -0.422583,10.257636 -2.554474,0.90745 -5.178258,1.60648 -7.845614,2.090232 -5.130295,-4.987976 -5.470015,-8.687728 -5.470015,-8.687728 l -0.760576,-8.876653 c -3.704849,0.317441 -7.436674,-0.01442 -11.027372,-0.980628 l -2.314998,8.603153 c 0,0 -0.987239,3.581749 -6.917199,7.586174 -2.540138,-0.946837 -4.999401,-2.097885 -7.353663,-3.441853 -0.723825,-7.118701 1.394089,-10.171243 1.394089,-10.171243 l 5.123167,-7.2888 c -3.042127,-2.138257 -5.687556,-4.791246 -7.817122,-7.839463 l -7.303391,5.102346 c 0,0 -3.058573,2.109194 -10.17518,1.365056 -1.337244,-2.358088 -2.481269,-4.820626 -3.420853,-7.363455 4.021331,-5.918508 7.605883,-6.895522 7.605883,-6.895522 l 8.609725,-2.290436 C 44.903313,48.3748 44.582106,44.642043 44.910119,40.938115 l -8.874447,-0.785906 m 83.110928,27.71077 c 0,0 -0.7115,-3.721364 3.16853,-9.851018 2.6144,-0.22593 5.24342,-0.22593 7.85782,0 3.88004,6.129654 3.16853,9.851018 3.16853,9.851018 l -1.74601,8.761062 c 3.51967,0.701443 6.8671,2.087995 9.85188,4.08078 l 4.96038,-7.429623 c 0,0 2.1283,-3.134512 9.20622,-4.72523 2.00841,1.688903 3.86741,3.547897 5.55631,5.556313 -1.59072,7.077922 -4.72523,9.206214 -4.72523,9.206214 l -7.42962,4.960389 c 1.99278,2.984776 3.37934,6.332209 4.08078,9.851876 l 8.76106,-1.746013 c 0,0 3.72137,-0.711504 9.85102,3.168535 0.22593,2.614398 0.22593,5.243418 0,7.857818 -6.12965,3.88003 -9.85102,3.16853 -9.85102,3.16853 l -8.76106,-1.74601 c -0.70144,3.51966 -2.088,6.8671 -4.08078,9.85187 l 7.42962,4.96039 c 0,0 3.13451,2.12829 4.72523,9.20622 -1.6889,2.00841 -3.5479,3.86741 -5.55631,5.55631 -7.07792,-1.59072 -9.20622,-4.72523 -9.20622,-4.72523 l -4.96038,-7.42962 c -2.98478,1.99278 -6.33221,3.37933 -9.85188,4.08078 l 1.74601,8.76106 c 0,0 0.71151,3.72136 -3.16853,9.85102 -2.6144,0.22593 -5.24342,0.22593 -7.85782,0 -3.88003,-6.12966 -3.16853,-9.85102 -3.16853,-9.85102 l 1.74601,-8.76106 c -3.51966,-0.70145 -6.8671,-2.088 -9.85187,-4.08078 l -4.96039,7.42962 c 0,0 -2.12829,3.13451 -9.206216,4.72523 -2.008416,-1.6889 -3.86741,-3.5479 -5.556313,-5.55631 1.590718,-7.07793 4.72523,-9.20622 4.72523,-9.20622 l 7.429619,-4.96039 c -1.99278,-2.98477 -3.37933,-6.33221 -4.080777,-9.85187 l -8.761061,1.74601 c 0,0 -3.721364,0.7115 -9.851018,-3.16853 -0.22593,-2.6144 -0.22593,-5.24342 0,-7.857818 6.129654,-3.880039 9.851018,-3.168535 9.851018,-3.168535 l 8.761061,1.746013 c 0.701447,-3.519667 2.087997,-6.8671 4.080777,-9.851876 l -7.429619,-4.960389 c 0,0 -3.134512,-2.128292 -4.72523,-9.206214 1.688903,-2.008416 3.547897,-3.86741 5.556313,-5.556313 7.077926,1.590718 9.206216,4.72523 9.206216,4.72523 l 4.96039,7.429623 c 2.98477,-1.992785 6.33221,-3.379337 9.85187,-4.08078 l -1.74601,-8.761062" />
  </g>
</svg>
)"""";

   testDoc(svg);
}


// INKSCAPE 1.0.2

TEST_F(LPEGearsTest, multi_PX_1_0_2)
{
   std::string svg = R""""(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<svg
   xmlns:svg="http://www.w3.org/2000/svg"
   xmlns="http://www.w3.org/2000/svg"
   xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd"
   xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape"
   width="250"
   height="250"
   viewBox="0 0 250 250"
   version="1.1"
   id="svg8"
   inkscape:version="1.0.2 (e86c870879, 2021-01-15)">
  <defs
     id="defs2">
    <inkscape:path-effect
       effect="gears"
       id="path-effect12"
       is_visible="true"
       teeth="17"
       phi="5.9"
       min_radius="6.2"
       lpeversion="0" />
  </defs>
  <g
     inkscape:label="Layer 1"
     inkscape:groupmode="layer"
     id="layer1">
    <path
       style="fill:#ffff00;stroke:#ff0000;stroke-width:0.264583"
       id="path01"
       inkscape:path-effect="#path-effect12"
       sodipodi:type="arc"
       sodipodi:cx="76.729164"
       sodipodi:cy="103.47619"
       sodipodi:rx="49.514877"
       sodipodi:ry="59.720234"
       d="m 120.26727,99.48557 c 0,0 1.97572,-2.880358 8.12583,-5.919733 2.50265,1.856896 4.90337,3.847367 7.19171,5.962746 -1.84757,6.606667 -4.31211,9.081747 -4.31211,9.081747 l -6.16059,6.1654 c 2.97301,2.97069 5.6661,6.20888 8.04516,9.67354 l 7.18498,-4.93367 c 0,0 2.88282,-1.97214 9.71557,-2.5846 1.66286,2.63557 3.18243,5.35887 4.55207,8.15804 -4.1094,5.49312 -7.30162,6.91077 -7.30162,6.91077 l -7.97178,3.5236 c 1.69911,3.84406 3.04057,7.83644 4.0074,11.92655 l 8.48204,-2.005 c 0,0 3.40057,-0.79757 9.99317,1.09961 0.59849,3.05828 1.03168,6.14661 1.29766,9.25154 -5.81626,3.6377 -9.30502,3.80645 -9.30502,3.80645 l -8.70634,0.40592 c 0.19574,4.19827 0.004,8.40564 -0.57158,12.56882 l 8.63356,1.19446 c 0,0 3.45905,0.48471 8.92113,4.6353 -0.5467,3.06797 -1.25841,6.10423 -2.13201,9.09558 -6.73759,1.29097 -10.05172,0.18804 -10.05172,0.18804 l -8.26506,-2.76658 c -1.33407,3.98548 -3.03236,7.83962 -5.07336,11.5136 l 7.61907,4.2326 c 0,0 3.05036,1.70154 6.64423,7.54497 -1.61805,2.66331 -3.37852,5.23744 -5.27374,7.7112 -6.74896,-1.23009 -9.44088,-3.45575 -9.44088,-3.45575 l -6.70752,-5.56544 c -2.68371,3.23442 -5.65959,6.21481 -8.88996,8.9034 l 5.57557,6.69911 c 0,0 2.22972,2.68855 3.47001,9.43564 -2.47089,1.89895 -5.04237,3.66331 -7.70323,5.28539 -5.84885,-3.58504 -7.55499,-6.63283 -7.55499,-6.63283 l -4.24412,-7.61266 c -3.67088,2.04655 -7.52246,3.75066 -11.505912,5.09075 l 2.779062,8.26086 c 0,0 1.10794,3.31247 -0.17286,10.052 -2.990015,0.87812 -6.025202,1.59441 -9.092343,2.14575 -4.158831,-5.4558 -4.648769,-8.91411 -4.648769,-8.91411 l -1.207508,-8.63175 c -4.162299,0.58227 -8.369379,0.77996 -12.567938,0.59057 l -0.392766,8.70694 c 0,0 -0.163484,3.48901 -3.792385,9.31076 -3.105326,-0.26129 -6.194308,-0.68981 -9.253496,-1.28369 -1.907133,-6.58972 -1.1147,-9.99148 -1.1147,-9.99148 l 1.992178,-8.48507 c -4.091569,-0.96064 -8.085968,-2.29607 -11.93259,-3.98937 l -3.511553,7.97709 c 0,0 -1.412823,3.19436 -6.899728,7.31206 -2.801242,-1.36542 -5.526833,-2.88087 -8.164909,-4.53975 0.602135,-6.83367 2.569916,-9.71946 2.569916,-9.71946 l 4.922809,-7.19243 c -3.468249,-2.37382 -6.710502,-5.06201 -9.68568,-8.03053 l -6.156085,6.1699 c 0,0 -2.471351,2.46828 -9.075224,4.32582 -2.118834,-2.28514 -4.1129299,-4.68285 -5.9736051,-7.18269 3.0300801,-6.15469 5.9074501,-8.13477 5.9074501,-8.13477 l 7.188587,-4.92841 c -2.376523,-3.4664 -4.428745,-7.14431 -6.130665,-10.98712 l -7.9692034,3.52942 c 0,0 -3.1961107,1.40885 -10.0250615,0.75536 -1.1502664,-2.89624 -2.1435522,-5.85239 -2.975533,-8.85557 5.0487963,-4.64449 8.4471503,-5.45143 8.4471503,-5.45143 l 8.4835066,-1.9988 C 9.1391104,174.80337 8.5540833,170.63247 8.3552761,166.43435 l -8.70603751,0.41228 c 0,0 -3.48921739,0.15914 -9.62095829,-2.91712 -0.026349,-3.11618 0.1153239,-6.23153 0.4244011,-9.33246 6.3856449,-2.50702 9.84601626,-2.03184 9.84601626,-2.03184 L 8.93138,153.76598 c 0.5790245,-4.16275 1.540203,-8.26333 2.871358,-12.24978 l -8.2670723,-2.76054 c 0,0 -3.31108598,-1.11206 -7.9174915,-6.19562 1.1011276,-2.91528 2.358625,-5.76907 3.76701758,-8.54895 6.86007672,-0.031 9.91512322,1.66215 9.91512322,1.66215 l 7.615966,4.23817 c 2.043683,-3.67248 4.421256,-7.14893 7.102593,-10.38532 l -6.711594,-5.56054 c 0,0 -2.685774,-2.23306 -5.144723,-8.63737 2.07989,-2.32065 4.28338,-4.52747 6.600877,-6.610861 6.408017,2.449271 8.645138,5.131671 8.645138,5.131671 l 5.570671,6.70318 c 3.232332,-2.68622 6.705193,-5.06904 10.374584,-7.11827 L 39.10415,95.824342 c 0,0 -1.697734,-3.052485 -1.677133,-9.912601 2.777753,-1.412591 5.629642,-2.674398 8.543253,-3.779929 5.090519,4.59872 6.207578,7.908122 6.207578,7.908122 l 2.773027,8.262892 c 3.984435,-1.337176 8.083557,-2.304549 12.245428,-2.889862 l -1.213817,-8.630859 c 0,0 -0.480405,-3.459649 2.016965,-9.849074 3.100464,-0.313762 6.215589,-0.460142 9.331813,-0.438501 3.085519,6.127086 2.931652,9.61654 2.931652,9.61654 l -0.399131,8.706651 c 4.198419,0.192464 8.370191,0.771189 12.462461,1.728842 l 1.985974,-8.486517 c 0,0 0.801806,-3.399569 5.43866,-8.455377 3.00444,0.827443 5.96209,1.816261 8.86006,2.96215 0.6638,6.827956 -0.74021,10.026191 -0.74021,10.026191 l -3.51738,7.97453 c 3.84538,1.69611 7.52638,3.74277 10.99637,6.11406 l 4.91755,-7.19603 M 36.035672,40.152209 c 0,0 -3.698766,-0.350277 -8.672081,-5.494786 0.491361,-2.665963 1.197876,-5.287743 2.112613,-7.839616 6.884861,-1.948978 10.2588,-0.393308 10.2588,-0.393308 l 8.067696,3.779648 c 1.577511,-3.367214 3.730822,-6.43314 6.362934,-9.05967 l -6.29305,-6.306424 c 0,0 -2.608266,-2.645849 -3.11122,-9.7835566 2.090053,-1.7264053 4.316523,-3.2806659 6.657564,-4.64753284 6.526888,2.93249994 8.11151,6.29293794 8.11151,6.29293794 l 3.750703,8.0811935 C 66.653988,13.215664 70.274259,12.25115 73.978875,11.931 L 73.21181,3.0549047 c 0,0 -0.297329,-3.70339913 3.905416,-9.4944928 2.710786,0.020957 5.41542,0.2614712 8.087366,0.7191826 3.114913,6.44182857 2.168756,10.0346487 2.168756,10.0346487 l -2.321287,8.6014588 c 3.589991,0.968835 6.983257,2.557039 10.026946,4.693071 l 5.117843,-7.292543 c 0,0 2.15273,-3.0280878 9.09466,-4.7628503 2.06311,1.7585136 3.98038,3.6812631 5.733,5.7493843 -1.75457,6.936954 -4.78878,9.081036 -4.78878,9.081036 l -7.30712,5.097005 c 2.12733,3.049773 3.70585,6.447558 4.66443,10.040299 l 8.60805,-2.29673 c 0,0 3.59551,-0.9359 10.02842,2.197383 0.45008,2.673242 0.68288,5.378551 0.6961,8.089385 -5.80306,4.186202 -9.5056,3.878306 -9.5056,3.878306 l -8.87387,-0.792393 c -0.33072,3.703687 -1.30556,7.321191 -2.88061,10.689557 l 8.07045,3.77375 c 0,0 3.35591,1.594205 6.26977,8.129436 -1.37354,2.337131 -2.93415,4.559155 -4.66652,6.644273 -7.13624,-0.523321 -9.77463,-3.139127 -9.77463,-3.139127 l -6.288442,-6.311022 c -2.634031,2.624605 -5.70609,4.769158 -9.077792,6.337054 l 3.756609,8.078449 c 0,0 1.546035,3.378365 -0.422583,10.257636 -2.554474,0.90745 -5.178258,1.60648 -7.845614,2.090232 -5.130295,-4.987976 -5.470015,-8.687728 -5.470015,-8.687728 l -0.760576,-8.876653 c -3.704849,0.317441 -7.436674,-0.01442 -11.027372,-0.980628 l -2.314998,8.603153 c 0,0 -0.987239,3.581749 -6.917199,7.586174 -2.540138,-0.946837 -4.999401,-2.097885 -7.353663,-3.441853 -0.723825,-7.118701 1.394089,-10.171243 1.394089,-10.171243 l 5.123167,-7.2888 c -3.042127,-2.138257 -5.687556,-4.791246 -7.817122,-7.839463 l -7.303391,5.102346 c 0,0 -3.058573,2.109194 -10.17518,1.365056 -1.337244,-2.358088 -2.481269,-4.820626 -3.420853,-7.363455 4.021331,-5.918508 7.605883,-6.895522 7.605883,-6.895522 l 8.609725,-2.290436 C 44.903313,48.3748 44.582106,44.642043 44.910119,40.938115 l -8.874447,-0.785906 m 83.110928,27.71077 c 0,0 -0.7115,-3.721364 3.16853,-9.851018 2.6144,-0.22593 5.24342,-0.22593 7.85782,0 3.88004,6.129654 3.16853,9.851018 3.16853,9.851018 l -1.74601,8.761062 c 3.51967,0.701443 6.8671,2.087995 9.85188,4.08078 l 4.96038,-7.429623 c 0,0 2.1283,-3.134512 9.20622,-4.72523 2.00841,1.688903 3.86741,3.547897 5.55631,5.556313 -1.59072,7.077922 -4.72523,9.206214 -4.72523,9.206214 l -7.42962,4.960389 c 1.99278,2.984776 3.37934,6.332209 4.08078,9.851876 l 8.76106,-1.746013 c 0,0 3.72137,-0.711504 9.85102,3.168535 0.22593,2.614398 0.22593,5.243418 0,7.857818 -6.12965,3.88003 -9.85102,3.16853 -9.85102,3.16853 l -8.76106,-1.74601 c -0.70144,3.51966 -2.088,6.8671 -4.08078,9.85187 l 7.42962,4.96039 c 0,0 3.13451,2.12829 4.72523,9.20622 -1.6889,2.00841 -3.5479,3.86741 -5.55631,5.55631 -7.07792,-1.59072 -9.20622,-4.72523 -9.20622,-4.72523 l -4.96038,-7.42962 c -2.98478,1.99278 -6.33221,3.37933 -9.85188,4.08078 l 1.74601,8.76106 c 0,0 0.71151,3.72136 -3.16853,9.85102 -2.6144,0.22593 -5.24342,0.22593 -7.85782,0 -3.88003,-6.12966 -3.16853,-9.85102 -3.16853,-9.85102 l 1.74601,-8.76106 c -3.51966,-0.70145 -6.8671,-2.088 -9.85187,-4.08078 l -4.96039,7.42962 c 0,0 -2.12829,3.13451 -9.206216,4.72523 -2.008416,-1.6889 -3.86741,-3.5479 -5.556313,-5.55631 1.590718,-7.07793 4.72523,-9.20622 4.72523,-9.20622 l 7.429619,-4.96039 c -1.99278,-2.98477 -3.37933,-6.33221 -4.080777,-9.85187 l -8.761061,1.74601 c 0,0 -3.721364,0.7115 -9.851018,-3.16853 -0.22593,-2.6144 -0.22593,-5.24342 0,-7.857818 6.129654,-3.880039 9.851018,-3.168535 9.851018,-3.168535 l 8.761061,1.746013 c 0.701447,-3.519667 2.087997,-6.8671 4.080777,-9.851876 l -7.429619,-4.960389 c 0,0 -3.134512,-2.128292 -4.72523,-9.206214 1.688903,-2.008416 3.547897,-3.86741 5.556313,-5.556313 7.077926,1.590718 9.206216,4.72523 9.206216,4.72523 l 4.96039,7.429623 c 2.98477,-1.992785 6.33221,-3.379337 9.85187,-4.08078 l -1.74601,-8.761062"
       transform="scale(0.5)" />
  </g>
</svg>
)"""";

   testDoc(svg);
}

TEST_F(LPEGearsTest, multi_MM_1_0_2)
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
   id="svg8"
   inkscape:version="1.0.2 (e86c870879, 2021-01-15)">
  <defs
     id="defs2">
    <inkscape:path-effect
       effect="gears"
       id="path-effect12"
       is_visible="true"
       teeth="17"
       phi="5.9"
       min_radius="6.2"
       lpeversion="0" />
  </defs>
  <g
     inkscape:label="Layer 1"
     inkscape:groupmode="layer"
     id="layer1">
    <path
       style="fill:#ffff00;stroke:#ff0000;stroke-width:0.264583"
       id="path01"
       inkscape:path-effect="#path-effect12"
       sodipodi:type="arc"
       sodipodi:cx="76.729164"
       sodipodi:cy="103.47619"
       sodipodi:rx="49.514877"
       sodipodi:ry="59.720234"
       d="m 120.26727,99.48557 c 0,0 1.97572,-2.880358 8.12583,-5.919733 2.50265,1.856896 4.90337,3.847367 7.19171,5.962746 -1.84757,6.606667 -4.31211,9.081747 -4.31211,9.081747 l -6.16059,6.1654 c 2.97301,2.97069 5.6661,6.20888 8.04516,9.67354 l 7.18498,-4.93367 c 0,0 2.88282,-1.97214 9.71557,-2.5846 1.66286,2.63557 3.18243,5.35887 4.55207,8.15804 -4.1094,5.49312 -7.30162,6.91077 -7.30162,6.91077 l -7.97178,3.5236 c 1.69911,3.84406 3.04057,7.83644 4.0074,11.92655 l 8.48204,-2.005 c 0,0 3.40057,-0.79757 9.99317,1.09961 0.59849,3.05828 1.03168,6.14661 1.29766,9.25154 -5.81626,3.6377 -9.30502,3.80645 -9.30502,3.80645 l -8.70634,0.40592 c 0.19574,4.19827 0.004,8.40564 -0.57158,12.56882 l 8.63356,1.19446 c 0,0 3.45905,0.48471 8.92113,4.6353 -0.5467,3.06797 -1.25841,6.10423 -2.13201,9.09558 -6.73759,1.29097 -10.05172,0.18804 -10.05172,0.18804 l -8.26506,-2.76658 c -1.33407,3.98548 -3.03236,7.83962 -5.07336,11.5136 l 7.61907,4.2326 c 0,0 3.05036,1.70154 6.64423,7.54497 -1.61805,2.66331 -3.37852,5.23744 -5.27374,7.7112 -6.74896,-1.23009 -9.44088,-3.45575 -9.44088,-3.45575 l -6.70752,-5.56544 c -2.68371,3.23442 -5.65959,6.21481 -8.88996,8.9034 l 5.57557,6.69911 c 0,0 2.22972,2.68855 3.47001,9.43564 -2.47089,1.89895 -5.04237,3.66331 -7.70323,5.28539 -5.84885,-3.58504 -7.55499,-6.63283 -7.55499,-6.63283 l -4.24412,-7.61266 c -3.67088,2.04655 -7.52246,3.75066 -11.505912,5.09075 l 2.779062,8.26086 c 0,0 1.10794,3.31247 -0.17286,10.052 -2.990015,0.87812 -6.025202,1.59441 -9.092343,2.14575 -4.158831,-5.4558 -4.648769,-8.91411 -4.648769,-8.91411 l -1.207508,-8.63175 c -4.162299,0.58227 -8.369379,0.77996 -12.567938,0.59057 l -0.392766,8.70694 c 0,0 -0.163484,3.48901 -3.792385,9.31076 -3.105326,-0.26129 -6.194308,-0.68981 -9.253496,-1.28369 -1.907133,-6.58972 -1.1147,-9.99148 -1.1147,-9.99148 l 1.992178,-8.48507 c -4.091569,-0.96064 -8.085968,-2.29607 -11.93259,-3.98937 l -3.511553,7.97709 c 0,0 -1.412823,3.19436 -6.899728,7.31206 -2.801242,-1.36542 -5.526833,-2.88087 -8.164909,-4.53975 0.602135,-6.83367 2.569916,-9.71946 2.569916,-9.71946 l 4.922809,-7.19243 c -3.468249,-2.37382 -6.710502,-5.06201 -9.68568,-8.03053 l -6.156085,6.1699 c 0,0 -2.471351,2.46828 -9.075224,4.32582 -2.118834,-2.28514 -4.1129299,-4.68285 -5.9736051,-7.18269 3.0300801,-6.15469 5.9074501,-8.13477 5.9074501,-8.13477 l 7.188587,-4.92841 c -2.376523,-3.4664 -4.428745,-7.14431 -6.130665,-10.98712 l -7.9692034,3.52942 c 0,0 -3.1961107,1.40885 -10.0250615,0.75536 -1.1502664,-2.89624 -2.1435522,-5.85239 -2.975533,-8.85557 5.0487963,-4.64449 8.4471503,-5.45143 8.4471503,-5.45143 l 8.4835066,-1.9988 C 9.1391104,174.80337 8.5540833,170.63247 8.3552761,166.43435 l -8.70603751,0.41228 c 0,0 -3.48921739,0.15914 -9.62095829,-2.91712 -0.026349,-3.11618 0.1153239,-6.23153 0.4244011,-9.33246 6.3856449,-2.50702 9.84601626,-2.03184 9.84601626,-2.03184 L 8.93138,153.76598 c 0.5790245,-4.16275 1.540203,-8.26333 2.871358,-12.24978 l -8.2670723,-2.76054 c 0,0 -3.31108598,-1.11206 -7.9174915,-6.19562 1.1011276,-2.91528 2.358625,-5.76907 3.76701758,-8.54895 6.86007672,-0.031 9.91512322,1.66215 9.91512322,1.66215 l 7.615966,4.23817 c 2.043683,-3.67248 4.421256,-7.14893 7.102593,-10.38532 l -6.711594,-5.56054 c 0,0 -2.685774,-2.23306 -5.144723,-8.63737 2.07989,-2.32065 4.28338,-4.52747 6.600877,-6.610861 6.408017,2.449271 8.645138,5.131671 8.645138,5.131671 l 5.570671,6.70318 c 3.232332,-2.68622 6.705193,-5.06904 10.374584,-7.11827 L 39.10415,95.824342 c 0,0 -1.697734,-3.052485 -1.677133,-9.912601 2.777753,-1.412591 5.629642,-2.674398 8.543253,-3.779929 5.090519,4.59872 6.207578,7.908122 6.207578,7.908122 l 2.773027,8.262892 c 3.984435,-1.337176 8.083557,-2.304549 12.245428,-2.889862 l -1.213817,-8.630859 c 0,0 -0.480405,-3.459649 2.016965,-9.849074 3.100464,-0.313762 6.215589,-0.460142 9.331813,-0.438501 3.085519,6.127086 2.931652,9.61654 2.931652,9.61654 l -0.399131,8.706651 c 4.198419,0.192464 8.370191,0.771189 12.462461,1.728842 l 1.985974,-8.486517 c 0,0 0.801806,-3.399569 5.43866,-8.455377 3.00444,0.827443 5.96209,1.816261 8.86006,2.96215 0.6638,6.827956 -0.74021,10.026191 -0.74021,10.026191 l -3.51738,7.97453 c 3.84538,1.69611 7.52638,3.74277 10.99637,6.11406 l 4.91755,-7.19603 M 36.035672,40.152209 c 0,0 -3.698766,-0.350277 -8.672081,-5.494786 0.491361,-2.665963 1.197876,-5.287743 2.112613,-7.839616 6.884861,-1.948978 10.2588,-0.393308 10.2588,-0.393308 l 8.067696,3.779648 c 1.577511,-3.367214 3.730822,-6.43314 6.362934,-9.05967 l -6.29305,-6.306424 c 0,0 -2.608266,-2.645849 -3.11122,-9.7835566 2.090053,-1.7264053 4.316523,-3.2806659 6.657564,-4.64753284 6.526888,2.93249994 8.11151,6.29293794 8.11151,6.29293794 l 3.750703,8.0811935 C 66.653988,13.215664 70.274259,12.25115 73.978875,11.931 L 73.21181,3.0549047 c 0,0 -0.297329,-3.70339913 3.905416,-9.4944928 2.710786,0.020957 5.41542,0.2614712 8.087366,0.7191826 3.114913,6.44182857 2.168756,10.0346487 2.168756,10.0346487 l -2.321287,8.6014588 c 3.589991,0.968835 6.983257,2.557039 10.026946,4.693071 l 5.117843,-7.292543 c 0,0 2.15273,-3.0280878 9.09466,-4.7628503 2.06311,1.7585136 3.98038,3.6812631 5.733,5.7493843 -1.75457,6.936954 -4.78878,9.081036 -4.78878,9.081036 l -7.30712,5.097005 c 2.12733,3.049773 3.70585,6.447558 4.66443,10.040299 l 8.60805,-2.29673 c 0,0 3.59551,-0.9359 10.02842,2.197383 0.45008,2.673242 0.68288,5.378551 0.6961,8.089385 -5.80306,4.186202 -9.5056,3.878306 -9.5056,3.878306 l -8.87387,-0.792393 c -0.33072,3.703687 -1.30556,7.321191 -2.88061,10.689557 l 8.07045,3.77375 c 0,0 3.35591,1.594205 6.26977,8.129436 -1.37354,2.337131 -2.93415,4.559155 -4.66652,6.644273 -7.13624,-0.523321 -9.77463,-3.139127 -9.77463,-3.139127 l -6.288442,-6.311022 c -2.634031,2.624605 -5.70609,4.769158 -9.077792,6.337054 l 3.756609,8.078449 c 0,0 1.546035,3.378365 -0.422583,10.257636 -2.554474,0.90745 -5.178258,1.60648 -7.845614,2.090232 -5.130295,-4.987976 -5.470015,-8.687728 -5.470015,-8.687728 l -0.760576,-8.876653 c -3.704849,0.317441 -7.436674,-0.01442 -11.027372,-0.980628 l -2.314998,8.603153 c 0,0 -0.987239,3.581749 -6.917199,7.586174 -2.540138,-0.946837 -4.999401,-2.097885 -7.353663,-3.441853 -0.723825,-7.118701 1.394089,-10.171243 1.394089,-10.171243 l 5.123167,-7.2888 c -3.042127,-2.138257 -5.687556,-4.791246 -7.817122,-7.839463 l -7.303391,5.102346 c 0,0 -3.058573,2.109194 -10.17518,1.365056 -1.337244,-2.358088 -2.481269,-4.820626 -3.420853,-7.363455 4.021331,-5.918508 7.605883,-6.895522 7.605883,-6.895522 l 8.609725,-2.290436 C 44.903313,48.3748 44.582106,44.642043 44.910119,40.938115 l -8.874447,-0.785906 m 83.110928,27.71077 c 0,0 -0.7115,-3.721364 3.16853,-9.851018 2.6144,-0.22593 5.24342,-0.22593 7.85782,0 3.88004,6.129654 3.16853,9.851018 3.16853,9.851018 l -1.74601,8.761062 c 3.51967,0.701443 6.8671,2.087995 9.85188,4.08078 l 4.96038,-7.429623 c 0,0 2.1283,-3.134512 9.20622,-4.72523 2.00841,1.688903 3.86741,3.547897 5.55631,5.556313 -1.59072,7.077922 -4.72523,9.206214 -4.72523,9.206214 l -7.42962,4.960389 c 1.99278,2.984776 3.37934,6.332209 4.08078,9.851876 l 8.76106,-1.746013 c 0,0 3.72137,-0.711504 9.85102,3.168535 0.22593,2.614398 0.22593,5.243418 0,7.857818 -6.12965,3.88003 -9.85102,3.16853 -9.85102,3.16853 l -8.76106,-1.74601 c -0.70144,3.51966 -2.088,6.8671 -4.08078,9.85187 l 7.42962,4.96039 c 0,0 3.13451,2.12829 4.72523,9.20622 -1.6889,2.00841 -3.5479,3.86741 -5.55631,5.55631 -7.07792,-1.59072 -9.20622,-4.72523 -9.20622,-4.72523 l -4.96038,-7.42962 c -2.98478,1.99278 -6.33221,3.37933 -9.85188,4.08078 l 1.74601,8.76106 c 0,0 0.71151,3.72136 -3.16853,9.85102 -2.6144,0.22593 -5.24342,0.22593 -7.85782,0 -3.88003,-6.12966 -3.16853,-9.85102 -3.16853,-9.85102 l 1.74601,-8.76106 c -3.51966,-0.70145 -6.8671,-2.088 -9.85187,-4.08078 l -4.96039,7.42962 c 0,0 -2.12829,3.13451 -9.206216,4.72523 -2.008416,-1.6889 -3.86741,-3.5479 -5.556313,-5.55631 1.590718,-7.07793 4.72523,-9.20622 4.72523,-9.20622 l 7.429619,-4.96039 c -1.99278,-2.98477 -3.37933,-6.33221 -4.080777,-9.85187 l -8.761061,1.74601 c 0,0 -3.721364,0.7115 -9.851018,-3.16853 -0.22593,-2.6144 -0.22593,-5.24342 0,-7.857818 6.129654,-3.880039 9.851018,-3.168535 9.851018,-3.168535 l 8.761061,1.746013 c 0.701447,-3.519667 2.087997,-6.8671 4.080777,-9.851876 l -7.429619,-4.960389 c 0,0 -3.134512,-2.128292 -4.72523,-9.206214 1.688903,-2.008416 3.547897,-3.86741 5.556313,-5.556313 7.077926,1.590718 9.206216,4.72523 9.206216,4.72523 l 4.96039,7.429623 c 2.98477,-1.992785 6.33221,-3.379337 9.85187,-4.08078 l -1.74601,-8.761062"
       transform="scale(0.5)" />
  </g>
</svg>
)"""";

   testDoc(svg);
}