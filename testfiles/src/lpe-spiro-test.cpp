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
#include <testfiles/lpespaths-test.h>
#include <src/inkscape.h>

using namespace Inkscape;
using namespace Inkscape::LivePathEffect;

class LPESpiroTest : public LPESPathsTest {};

// INKSCAPE 0.92.5

TEST_F(LPESpiroTest, mixed_0_92_5)
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
   inkscape:version="0.92.5 (2060ec1f9f, 2020-04-08)"
   inkscape:test-threshold="1.0">
  <defs
     id="defs2">
    <inkscape:path-effect
       effect="skeletal"
       id="path-effect60"
       is_visible="true"
       pattern="M 0,5 C 0,2.24 2.24,0 5,0 7.76,0 10,2.24 10,5 10,7.76 7.76,10 5,10 2.24,10 0,7.76 0,5 Z"
       copytype="single_stretched"
       prop_scale="1"
       scale_y_rel="false"
       spacing="0"
       normal_offset="0"
       tang_offset="0"
       prop_units="false"
       vertical_pattern="false"
       fuse_tolerance="0" />
    <inkscape:path-effect
       effect="spiro"
       id="path-effect58"
       is_visible="true" />
    <inkscape:path-effect
       effect="skeletal"
       id="path-effect54"
       is_visible="true"
       pattern="M 0,5 C 0,2.24 2.24,0 5,0 7.76,0 10,2.24 10,5 10,7.76 7.76,10 5,10 2.24,10 0,7.76 0,5 Z"
       copytype="single_stretched"
       prop_scale="1"
       scale_y_rel="false"
       spacing="0"
       normal_offset="0"
       tang_offset="0"
       prop_units="false"
       vertical_pattern="false"
       fuse_tolerance="0" />
    <inkscape:path-effect
       effect="spiro"
       id="path-effect52"
       is_visible="true" />
    <inkscape:path-effect
       effect="skeletal"
       id="path-effect48"
       is_visible="true"
       pattern="M 0,5 C 0,2.24 2.24,0 5,0 7.76,0 10,2.24 10,5 10,7.76 7.76,10 5,10 2.24,10 0,7.76 0,5 Z"
       copytype="single_stretched"
       prop_scale="1"
       scale_y_rel="false"
       spacing="0"
       normal_offset="0"
       tang_offset="0"
       prop_units="false"
       vertical_pattern="false"
       fuse_tolerance="0" />
    <inkscape:path-effect
       effect="spiro"
       id="path-effect46"
       is_visible="true" />
    <inkscape:path-effect
       effect="skeletal"
       id="path-effect42"
       is_visible="true"
       pattern="M 0,5 C 0,2.24 2.24,0 5,0 7.76,0 10,2.24 10,5 10,7.76 7.76,10 5,10 2.24,10 0,7.76 0,5 Z"
       copytype="single_stretched"
       prop_scale="1"
       scale_y_rel="false"
       spacing="0"
       normal_offset="0"
       tang_offset="0"
       prop_units="false"
       vertical_pattern="false"
       fuse_tolerance="0" />
    <inkscape:path-effect
       effect="spiro"
       id="path-effect40"
       is_visible="true" />
  </defs>
  <g id="t" transform="scale(0.445)">
  <path
     style="fill:#000000;stroke:none;stroke-width:0.26458332px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1"
     d="m 41.44006,35.399686 c 0.224812,0.553036 20.243494,-14.522103 62.20634,-13.354755 18.25684,0.502344 40.44091,4.537793 62.48026,15.748922 13.08527,6.639856 26.48925,16.001608 37.31563,28.875483 0,8e-6 10e-6,1.3e-5 2e-5,1.7e-5 9.84908,11.707094 17.5263,26.720615 19.69595,43.526387 2.01956,15.70293 -1.24465,32.9429 -11.0944,46.97559 -9.40521,13.34396 -25.01497,22.97444 -42.36918,23.82097 -4.14748,0.17115 -8.3024,-0.18556 -12.3653,-1.0366 -12.08841,-2.53213 -22.9859,-9.348 -30.34952,-18.95373 -8.95891,-12.45154 -10.40734,-29.25908 -4.44017,-43.59722 -9.11675,7.18366 -16.0078,17.53373 -19.5577,28.87342 -4.983781,11.16063 -6.13306,23.57667 -3.941358,34.75983 0,0 10e-6,2e-5 10e-6,2e-5 2.613768,13.40965 9.986518,25.1228 19.304188,33.1116 10.72636,9.2159 23.98622,13.66056 35.97991,14.16522 15.17319,0.65166 28.41744,-4.70206 37.53053,-11.20097 10.33254,-7.35876 16.2633,-16.54391 19.41607,-22.47957 3.28268,-6.18022 4.39428,-9.86588 4.82897,-9.7544 0.29485,0.0756 0.19856,4.00046 -2.469,10.83393 -2.54633,6.52292 -8.03247,16.734 -18.79741,25.33709 -9.49784,7.58144 -23.70085,14.12495 -40.67327,13.9923 -13.43617,-0.11546 -28.40672,-4.60671 -40.90268,-14.81064 -10.84904,-8.87553 -19.511131,-21.96741 -22.818299,-37.43076 7e-6,0 -5e-6,-2e-5 -5e-6,-2e-5 -2.746852,-12.89898 -1.654998,-27.16646 3.993691,-40.26967 6.411213,-11.39384 16.852663,-21.43982 30.227333,-27.81728 -2.70398,12.99601 0.80589,28.68979 10.21856,40.24069 5.81762,7.61493 14.77421,13.15739 24.5087,15.21736 3.27177,0.69236 6.59691,0.9825 9.87789,0.83458 13.80635,-0.49189 26.92459,-8.62737 34.77766,-19.54209 8.35996,-11.5633 11.24825,-26.51365 9.70724,-40.12711 -1.65238,-14.663782 -8.41682,-28.234224 -17.1285,-39.026192 0,-5e-6 -10e-6,-9e-6 -2e-5,-1.4e-5 C 186.99143,60.412355 174.63217,51.446121 162.43382,44.845539 141.90084,33.762388 120.8654,29.223368 103.3593,28.002309 83.945124,26.654308 68.06292,29.261622 57.733664,31.573176 47.232071,33.923296 41.650437,35.917211 41.44006,35.399686 Z"
     id="path04"
     inkscape:connector-curvature="0"
     inkscape:path-effect="#path-effect40;#path-effect42"
     inkscape:original-d="m 41.44006,35.399686 c 40.94699,1.973072 81.89372,3.946408 122.84018,5.92001 40.94646,1.973601 3.4536,89.786534 5.18001,134.680194 1.7264,44.89367 -45.88007,-59.94009 -45.88007,-59.94009 0,0 -19.23976,43.90647 -28.860041,65.8601 -9.620279,21.95363 80.907051,1.97307 121.360171,2.96" />
  <path
     style="fill:#000000;stroke:none;stroke-width:0.26458332px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1"
     d="m 63.640093,72.399741 c -0.288529,0.950576 -8.829359,-1.930576 -20.716536,2.829913 -5.269897,2.104646 -11.153361,6.053549 -15.382565,12.25643 -3.376718,4.93848 -5.688536,11.405404 -5.663025,18.331176 3e-6,0 0,10e-6 0,10e-6 0.01275,6.05691 1.85645,12.55275 5.662137,18.0085 4.066067,5.85763 10.581692,10.61627 18.058522,12.47214 7.58998,2.97613 16.168836,3.66626 23.678325,1.8489 0.248026,-0.06 0.494427,-0.12269 0.73913,-0.18799 -4.213983,5.64698 -9.945981,8.85764 -15.447369,9.36332 -6.625612,-0.91547 -12.431249,-4.5902 -16.105722,-9.82632 -3.815138,-5.50362 -4.999054,-12.23846 -3.646495,-18.40516 1.471194,-6.53262 5.635059,-11.89317 10.993167,-15.00696 3.537065,-1.99886 7.269118,-3.00363 10.7715,-3.38881 3.664337,-0.41677 7.169596,-0.31546 10.319221,-0.1131 7.434449,0.36473 12.716871,1.13699 17.899425,-0.37492 6.0066,-1.603067 10.47099,-5.790365 12.885677,-9.809732 2.764745,-4.571908 3.309885,-9.33764 3.404545,-12.279629 0.0996,-3.095668 -0.20809,-4.814705 0.29012,-4.977776 0.36021,-0.117905 1.56406,1.322204 2.35326,4.696783 0.74534,3.187025 1.14369,8.766571 -1.55512,14.861815 -2.383684,5.349794 -7.400384,11.239009 -15.29785,14.054379 -6.673684,2.25414 -13.145357,2.05784 -20.263304,1.9127 -3.002308,-0.0133 -6.06045,0.0242 -8.867246,0.50007 -2.664218,0.46554 -5.279246,1.23559 -7.359418,2.5904 -3.127245,1.95234 -5.647972,5.46731 -6.365617,9.28263 -0.70919,3.57962 0.141337,7.76459 2.389493,10.83317 2.112565,2.94563 5.726354,5.1539 9.452749,5.60339 4.483862,1.82352 9.45041,2.07802 13.237022,1.08579 -0.25888,0.19304 -0.519565,0.3826 -0.781985,0.56866 -7.9413,5.63064 -17.041007,7.77668 -25.1815,6.69311 -9.95171,-2.64184 -18.171221,-8.95295 -23.320664,-16.80875 -4.766656,-7.30028 -6.771325,-15.6314 -6.446184,-23.3888 0,0 3e-6,-1e-5 3e-6,-1e-5 0.382092,-8.855215 3.725627,-16.66288 8.303084,-22.476828 5.738163,-7.272118 13.155228,-11.284986 19.551362,-13.041154 15.48846,-4.239087 22.326045,2.57537 22.411858,2.292653 z"
     id="path03"
     inkscape:connector-curvature="0"
     inkscape:path-effect="#path-effect46;#path-effect48"
     inkscape:original-d="M 63.640093,72.399741 C 50.320339,90.406169 37.00032,108.41286 23.680034,126.41982 10.35975,144.42678 69.5601,138.25984 69.5601,138.25984 c 0,0 -14.306422,-20.22696 -21.460031,-30.34005 -7.153608,-10.113083 25.160303,-2.96027 37.740055,-4.44 12.579755,-1.47974 10.360282,-20.226965 15.540026,-30.340049" />
  <path
     style="fill:#000000;stroke:none;stroke-width:0.26458332px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1"
     d="m 145.78021,68.699734 c -0.54429,-0.0099 -4.81845,-10.140831 3.87169,-25.071657 3.89388,-6.678699 10.3726,-13.47916 19.56967,-17.804763 5.37421,-2.521624 11.46504,-4.077303 17.89136,-4.306206 2.90799,-0.103581 5.78129,0.06886 8.57728,0.497679 9.76897,1.507633 18.00867,6.017722 23.97106,11.561621 8.0951,7.54799 11.49317,16.40077 12.60812,22.413799 1.14662,6.183794 0.15146,9.679549 -0.64905,9.749531 -1.02446,0.08956 -1.86555,-3.195207 -4.23063,-8.217313 -2.32001,-4.926407 -6.36517,-11.864672 -13.41034,-17.339457 -5.19389,-4.052383 -12.01988,-7.304702 -19.72005,-8.340855 -2.20836,-0.299833 -4.48266,-0.41417 -6.78842,-0.331001 -5.08994,0.183594 -9.96378,1.313649 -14.29539,3.139607 -7.41629,3.116219 -13.1447,8.178816 -17.02771,13.270209 -8.18778,10.715754 -8.79529,20.807523 -10.36759,20.778806 z"
     id="path02"
     inkscape:connector-curvature="0"
     inkscape:path-effect="#path-effect52;#path-effect54"
     inkscape:original-d="m 145.78021,68.699734 c 23.6803,-10.606945 47.36033,-21.213628 71.04011,-31.820046 23.67976,-10.606417 9.86694,19.239765 14.80002,28.860043" />
  <path
     style="fill:#000000;stroke:none;stroke-width:0.26458332px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1"
     d="m 148.00022,87.939762 c 1.86534,-1.375687 19.93129,22.170108 43.43085,56.950128 7.14611,-15.93729 15.5595,-34.64514 24.11942,-53.515182 -0.0348,0.07673 4.64569,-10.225145 4.61089,-10.148412 -16.97017,-2.046756 -32.64607,-3.994748 -47.29774,-5.900042 -0.42109,10.969522 -0.76375,20.434469 -1.05269,29.604886 0,0 1e-5,-1e-5 1e-5,-1e-5 18.68223,-12.39543 31.66511,-19.054085 32.42935,-17.731371 0.75857,1.312916 -11.63678,9.343372 -34.17075,20.748681 0,0 -1e-5,0 -1e-5,0 -0.29378,-10.963789 -0.62675,-22.501857 -1.05269,-36.995212 16.92713,2.154857 35.83259,4.653001 56.3024,7.419976 -0.0328,0.07029 -5.30458,11.335975 -5.33742,11.406262 -9.98872,21.317364 -19.86305,42.235564 -28.09235,59.610344 -28.36777,-35.51878 -45.74038,-60.084853 -43.88927,-61.450048 z"
     id="path01"
     inkscape:connector-curvature="0"
     inkscape:path-effect="#path-effect58;#path-effect60"
     inkscape:original-d="m 148.00022,87.939762 c 14.30695,19.486428 43.66006,59.200088 43.66006,59.200088 0,0 31.08004,-67.3401 31.08004,-67.3401 0,0 -51.80007,-6.660009 -51.80007,-6.660009 0,0 0,33.300049 0,33.300049 0,0 33.30005,-19.240027 33.30005,-19.240027" />
   </g>
</svg>
)"""";

   testDoc(svg);
}


// INKSCAPE 1.0.2

TEST_F(LPESpiroTest, spiro_MM_1_0_2)
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
       effect="skeletal"
       id="path-effect60"
       is_visible="true"
       pattern="m -0.63412468,0.85439748 c 0,-2.75999998 2.23999998,-4.99999998 4.99999998,-4.99999998 2.76,0 5,2.24 5,4.99999998 0,2.76000002 -2.24,4.99999972 -5,4.99999972 -2.76,0 -4.99999998,-2.2399997 -4.99999998,-4.99999972 z"
       copytype="single_stretched"
       prop_scale="1"
       scale_y_rel="false"
       spacing="0"
       normal_offset="0"
       tang_offset="0"
       prop_units="false"
       vertical_pattern="false"
       fuse_tolerance="0" />
    <inkscape:path-effect
       effect="spiro"
       id="path-effect58"
       is_visible="true" />
    <inkscape:path-effect
       effect="skeletal"
       id="path-effect54"
       is_visible="true"
       pattern="m -0.63412468,0.85439748 c 0,-2.75999998 2.23999998,-4.99999998 4.99999998,-4.99999998 2.76,0 5,2.24 5,4.99999998 0,2.76000002 -2.24,4.99999972 -5,4.99999972 -2.76,0 -4.99999998,-2.2399997 -4.99999998,-4.99999972 z"
       copytype="single_stretched"
       prop_scale="1"
       scale_y_rel="false"
       spacing="0"
       normal_offset="0"
       tang_offset="0"
       prop_units="false"
       vertical_pattern="false"
       fuse_tolerance="0" />
    <inkscape:path-effect
       effect="spiro"
       id="path-effect52"
       is_visible="true" />
    <inkscape:path-effect
       effect="skeletal"
       id="path-effect48"
       is_visible="true"
       pattern="m -0.63412468,0.85439748 c 0,-2.75999998 2.23999998,-4.99999998 4.99999998,-4.99999998 2.76,0 5,2.24 5,4.99999998 0,2.76000002 -2.24,4.99999972 -5,4.99999972 -2.76,0 -4.99999998,-2.2399997 -4.99999998,-4.99999972 z"
       copytype="single_stretched"
       prop_scale="1"
       scale_y_rel="false"
       spacing="0"
       normal_offset="0"
       tang_offset="0"
       prop_units="false"
       vertical_pattern="false"
       fuse_tolerance="0" />
    <inkscape:path-effect
       effect="spiro"
       id="path-effect46"
       is_visible="true" />
    <inkscape:path-effect
       effect="skeletal"
       id="path-effect42"
       is_visible="true"
       pattern="m -0.63412468,0.85439748 c 0,-2.75999998 2.23999998,-4.99999998 4.99999998,-4.99999998 2.76,0 5,2.24 5,4.99999998 0,2.76000002 -2.24,4.99999972 -5,4.99999972 -2.76,0 -4.99999998,-2.2399997 -4.99999998,-4.99999972 z"
       copytype="single_stretched"
       prop_scale="1"
       scale_y_rel="false"
       spacing="0"
       normal_offset="0"
       tang_offset="0"
       prop_units="false"
       vertical_pattern="false"
       fuse_tolerance="0" />
    <inkscape:path-effect
       effect="spiro"
       id="path-effect40"
       is_visible="true" />
  </defs>
  <g id="t" transform="scale(0.445)">
  <path
     style="fill:#000000;stroke:none;stroke-width:0.264583px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1"
     d="m 40.805935,31.254083 c -0.420754,-1.03505 21.588095,-14.484698 62.206345,-13.354756 18.38966,0.506 40.45345,4.544175 62.48026,15.748923 13.08527,6.639856 26.48925,16.001609 37.31563,28.875483 0,4e-6 2e-5,1.8e-5 2e-5,1.8e-5 9.88559,11.750487 17.52571,26.716092 19.69595,43.526389 2.01956,15.70293 -1.24465,32.9429 -11.0944,46.97559 -9.40521,13.34396 -25.01497,22.97444 -42.36918,23.82097 -4.14748,0.17115 -8.3024,-0.18556 -12.36531,-1.0366 -12.06968,-2.52821 -22.98564,-9.34768 -30.34951,-18.95373 -8.95891,-12.45154 -10.40734,-29.25908 -4.44017,-43.59722 -9.11675,7.18366 -16.0078,17.53373 -19.5577,28.87342 -4.983785,11.16063 -6.133064,23.57667 -3.941363,34.75983 0,0 1e-5,2e-5 1e-5,2e-5 2.613773,13.40965 9.986513,25.1228 19.304193,33.1116 10.72636,9.2159 23.98621,13.66057 35.97991,14.16522 15.17319,0.65167 28.41744,-4.70206 37.53053,-11.20097 10.33254,-7.35876 16.2633,-16.54391 19.41607,-22.47957 3.28268,-6.18023 4.39428,-9.86588 4.82897,-9.75441 0.43469,0.11148 0.18305,4.04021 -2.469,10.83394 -2.54633,6.52292 -8.03247,16.734 -18.79741,25.33709 -9.49784,7.58144 -23.70085,14.12495 -40.67327,13.99231 -13.43618,-0.11547 -28.40672,-4.60671 -40.90269,-14.81065 -10.84904,-8.87553 -19.511127,-21.96741 -22.818294,-37.43076 7e-6,0 -5e-6,-2e-5 -5e-6,-2e-5 -2.746852,-12.89898 -1.654996,-27.16646 3.993693,-40.26967 6.411216,-11.39384 16.852656,-21.43982 30.227336,-27.81728 -2.70398,12.99601 0.80589,28.68979 10.21856,40.24069 5.81762,7.61493 14.7742,13.15739 24.50869,15.21736 3.27178,0.69236 6.59692,0.9825 9.8779,0.83458 13.80635,-0.49189 26.92459,-8.62737 34.77766,-19.54209 8.35996,-11.5633 11.24825,-26.51365 9.70724,-40.12711 -1.65238,-14.663783 -8.41682,-28.234226 -17.1285,-39.026195 0,-4e-6 -1e-5,-9e-6 -1e-5,-1.3e-5 C 186.35731,56.266752 173.99805,47.300518 161.7997,40.699936 141.27361,29.620502 120.36383,25.08701 102.72518,23.856705 c -19.414179,-1.348 -35.296384,1.259314 -45.62564,3.570868 -10.501593,2.35012 -16.083228,4.344035 -16.293605,3.82651 z"
     id="path04"
     inkscape:connector-curvature="0"
     inkscape:path-effect="#path-effect40;#path-effect42"
     inkscape:original-d="m 40.805935,31.254083 c 40.94699,1.973072 81.893725,3.946408 122.840185,5.92001 40.94646,1.973601 3.4536,89.786537 5.18001,134.680197 1.7264,44.89367 -45.88007,-59.94009 -45.88007,-59.94009 0,0 -19.23976,43.90647 -28.860046,65.8601 -9.620279,21.95363 80.907056,1.97307 121.360176,2.96" />
  <path
     style="fill:#000000;stroke:none;stroke-width:0.264583px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1"
     d="m 63.005968,68.254138 c -0.288529,0.950576 -8.82936,-1.930576 -20.716537,2.829913 -5.269897,2.104647 -11.153361,6.05355 -15.382565,12.256431 -3.376718,4.93848 -5.688536,11.405405 -5.663025,18.331178 2e-6,0 1e-6,1e-5 0,1e-5 0.01275,6.05691 1.856451,12.55275 5.662138,18.0085 4.066067,5.85763 10.581692,10.61627 18.058522,12.47214 7.592171,2.97699 16.192384,3.66056 23.678322,1.8489 0.248027,-0.06 0.494429,-0.12269 0.739133,-0.18799 -4.213983,5.64698 -9.945981,8.85764 -15.447369,9.36332 -6.625611,-0.91547 -12.431248,-4.5902 -16.105721,-9.82633 -3.815137,-5.50361 -4.999053,-12.23845 -3.646494,-18.40515 1.471193,-6.53262 5.635058,-11.89317 10.993164,-15.006957 3.537065,-1.998859 7.269119,-3.003629 10.7715,-3.38881 3.664338,-0.416772 7.169597,-0.315459 10.319221,-0.113103 7.43445,0.36473 12.716872,1.136992 17.899426,-0.374927 6.006601,-1.603062 10.470991,-5.79036 12.885679,-9.809727 2.764749,-4.571908 3.309888,-9.337639 3.404548,-12.279628 0.0991,-3.080772 -0.14725,-4.834618 0.29012,-4.977777 0.43736,-0.143158 1.57114,1.352487 2.35326,4.696784 0.78212,3.344297 1.15576,8.739321 -1.55512,14.861814 -2.383688,5.349794 -7.400388,11.239011 -15.297855,14.054381 -6.673684,2.25414 -13.145357,2.05784 -20.263304,1.9127 -3.002308,-0.0133 -6.06045,0.0242 -8.867246,0.50007 -2.664218,0.46554 -5.279246,1.23559 -7.359418,2.5904 -3.127244,1.95234 -5.647971,5.46731 -6.365616,9.28263 -0.709189,3.57962 0.141338,7.76459 2.389494,10.83317 2.112565,2.94563 5.726353,5.1539 9.452748,5.60339 4.484169,1.82365 9.462284,2.07491 13.237021,1.08579 -0.258881,0.19304 -0.519567,0.3826 -0.781988,0.56866 -7.941299,5.63064 -17.041005,7.77668 -25.181497,6.69311 -9.95171,-2.64184 -18.17122,-8.95295 -23.320664,-16.80875 -4.766657,-7.30028 -6.771326,-15.6314 -6.446185,-23.3888 0,0 3e-6,-10e-6 3e-6,-10e-6 0.382092,-8.855215 3.725627,-16.662881 8.303084,-22.47683 5.738163,-7.272118 13.155228,-11.284987 19.551363,-13.041155 14.433012,-3.950218 22.700387,1.342076 22.411858,2.292653 z"
     id="path03"
     inkscape:connector-curvature="0"
     inkscape:path-effect="#path-effect46;#path-effect48"
     inkscape:original-d="M 63.005968,68.254138 C 49.686214,86.260566 36.366195,104.26726 23.045909,122.27422 c -13.3202837,18.00696 45.880066,11.84002 45.880066,11.84002 0,0 -14.306422,-20.22696 -21.460031,-30.34005 -7.153608,-10.113086 25.160303,-2.96027 37.740055,-4.440002 12.579755,-1.47974 10.360282,-20.226966 15.540031,-30.34005" />
  <path
     style="fill:#000000;stroke:none;stroke-width:0.264583px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1"
     d="m 145.14609,64.554131 c -1.5723,-0.02872 -4.29583,-11.038763 3.87169,-25.071656 3.89388,-6.6787 10.3726,-13.479161 19.56967,-17.804764 5.37421,-2.521624 11.46504,-4.077302 17.89136,-4.306205 2.90799,-0.103582 5.78129,0.06886 8.57728,0.497678 9.76897,1.507633 18.00867,6.017722 23.97106,11.561621 8.0951,7.54799 11.49317,16.40077 12.60812,22.413799 1.13383,6.114857 0.28168,9.668165 -0.64905,9.749531 -0.93072,0.08137 -1.87128,-3.207379 -4.23063,-8.217313 -2.32001,-4.926407 -6.36517,-11.864672 -13.41034,-17.339457 -5.19411,-4.052552 -12.07137,-7.31163 -19.72005,-8.340855 -2.20836,-0.299833 -4.48266,-0.41417 -6.78842,-0.331001 -5.08994,0.183594 -9.96378,1.313649 -14.29539,3.139607 -7.41629,3.116219 -13.1447,8.178816 -17.02771,13.270209 -8.18778,10.715754 -8.79529,20.807523 -10.36759,20.778806 z"
     id="path02"
     inkscape:connector-curvature="0"
     inkscape:path-effect="#path-effect52;#path-effect54"
     inkscape:original-d="m 145.14609,64.554131 c 23.6803,-10.606945 47.36033,-21.213628 71.04011,-31.820046 23.67976,-10.606417 9.86694,19.239765 14.80002,28.860043" />
  <path
     style="fill:#000000;stroke:none;stroke-width:0.264583px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1"
     d="m 147.3661,83.794159 c 1.5563,-1.147771 19.80712,21.986331 43.43085,56.950131 7.14611,-15.93729 15.5595,-34.64514 24.11942,-53.515184 -0.0348,0.07673 4.64569,-10.225146 4.61089,-10.148413 -16.97017,-2.046757 -32.64607,-3.994748 -47.29774,-5.900042 -0.42109,10.969523 -0.76375,20.434471 -1.05269,29.604889 0,0 0,-1e-5 0,-1e-5 18.76272,-12.448827 31.75167,-18.904298 32.42936,-17.731374 0.67769,1.172924 -11.72291,9.386971 -34.17075,20.748684 0,0 -1e-5,0 -1e-5,0 -0.29378,-10.96379 -0.62675,-22.501859 -1.05269,-36.995215 16.92713,2.154857 35.83259,4.653001 56.3024,7.419976 -0.0328,0.07029 -5.30458,11.335976 -5.33742,11.406263 C 209.359,106.95122 199.48467,127.86943 191.25537,145.24421 163.026,109.89872 145.8098,84.94193 147.3661,83.794159 Z"
     id="path01"
     inkscape:connector-curvature="0"
     inkscape:path-effect="#path-effect58;#path-effect60"
     inkscape:original-d="m 147.3661,83.794159 c 14.30695,19.486431 43.66006,59.200091 43.66006,59.200091 0,0 31.08004,-67.340103 31.08004,-67.340103 0,0 -51.80007,-6.660009 -51.80007,-6.660009 0,0 0,33.300052 0,33.300052 0,0 33.30005,-19.24003 33.30005,-19.24003" />
   </g>
</svg>
)"""";

   testDoc(svg);
}

// INKSCAPE 1.0.2

TEST_F(LPESpiroTest, spiro_PX_1_0_2)
{
   std::string svg = R""""(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<svg
   xmlns:dc="http://purl.org/dc/elements/1.1/"
   xmlns:cc="http://creativecommons.org/ns#"
   xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
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
       effect="skeletal"
       id="path-effect60"
       is_visible="true"
       pattern="m -11.607038,-1.5988035 c 0,-2.76 2.2399995,-5 4.9999995,-5 2.76,0 5,2.24 5,5 0,2.76 -2.24,4.9999997 -5,4.9999997 -2.76,0 -4.9999995,-2.2399997 -4.9999995,-4.9999997 z"
       copytype="single_stretched"
       prop_scale="1"
       scale_y_rel="false"
       spacing="0"
       normal_offset="0"
       tang_offset="0"
       prop_units="false"
       vertical_pattern="false"
       fuse_tolerance="0"
       lpeversion="0"
       hide_knot="false" />
    <inkscape:path-effect
       effect="spiro"
       id="path-effect58"
       is_visible="true"
       lpeversion="0" />
    <inkscape:path-effect
       effect="skeletal"
       id="path-effect54"
       is_visible="true"
       pattern="m -11.607038,-1.5988035 c 0,-2.76 2.2399995,-5 4.9999995,-5 2.76,0 5,2.24 5,5 0,2.76 -2.24,4.9999997 -5,4.9999997 -2.76,0 -4.9999995,-2.2399997 -4.9999995,-4.9999997 z"
       copytype="single_stretched"
       prop_scale="1"
       scale_y_rel="false"
       spacing="0"
       normal_offset="0"
       tang_offset="0"
       prop_units="false"
       vertical_pattern="false"
       fuse_tolerance="0"
       lpeversion="0"
       hide_knot="false" />
    <inkscape:path-effect
       effect="spiro"
       id="path-effect52"
       is_visible="true"
       lpeversion="0" />
    <inkscape:path-effect
       effect="skeletal"
       id="path-effect48"
       is_visible="true"
       pattern="m -11.607038,-1.5988035 c 0,-2.76 2.2399995,-5 4.9999995,-5 2.76,0 5,2.24 5,5 0,2.76 -2.24,4.9999997 -5,4.9999997 -2.76,0 -4.9999995,-2.2399997 -4.9999995,-4.9999997 z"
       copytype="single_stretched"
       prop_scale="1"
       scale_y_rel="false"
       spacing="0"
       normal_offset="0"
       tang_offset="0"
       prop_units="false"
       vertical_pattern="false"
       fuse_tolerance="0"
       lpeversion="0"
       hide_knot="false" />
    <inkscape:path-effect
       effect="spiro"
       id="path-effect46"
       is_visible="true"
       lpeversion="0" />
    <inkscape:path-effect
       effect="skeletal"
       id="path-effect42"
       is_visible="true"
       pattern="m -11.607038,-1.5988035 c 0,-2.76 2.2399995,-5 4.9999995,-5 2.76,0 5,2.24 5,5 0,2.76 -2.24,4.9999997 -5,4.9999997 -2.76,0 -4.9999995,-2.2399997 -4.9999995,-4.9999997 z"
       copytype="single_stretched"
       prop_scale="1"
       scale_y_rel="false"
       spacing="0"
       normal_offset="0"
       tang_offset="0"
       prop_units="false"
       vertical_pattern="false"
       fuse_tolerance="0"
       lpeversion="0"
       hide_knot="false" />
    <inkscape:path-effect
       effect="spiro"
       id="path-effect40"
       is_visible="true"
       lpeversion="0" />
  </defs>
  <g id="t" transform="scale(0.445)">
  <path
     style="fill:#000000;stroke:none;stroke-width:0.264583px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1"
     d="m 29.833021,28.800882 c -0.420754,-1.03505 21.588094,-14.484699 62.206348,-13.354756 18.389651,0.505999 40.453451,4.544174 62.480261,15.748923 13.08527,6.639856 26.48925,16.001609 37.31563,28.875484 0,3e-6 2e-5,1.7e-5 2e-5,1.7e-5 9.88559,11.750487 17.52571,26.716093 19.69595,43.52639 2.01956,15.70293 -1.24465,32.9429 -11.0944,46.97559 -9.40521,13.34396 -25.01497,22.97444 -42.36918,23.82097 -4.14747,0.17115 -8.30239,-0.18556 -12.3653,-1.0366 -12.06968,-2.52821 -22.98565,-9.34768 -30.34952,-18.95373 -8.95891,-12.45154 -10.40734,-29.25908 -4.44017,-43.59722 -9.11675,7.18366 -16.007803,17.53373 -19.557705,28.87342 -4.983782,11.16063 -6.133063,23.57667 -3.941362,34.75983 0,0 10e-6,2e-5 10e-6,2e-5 2.61377,13.40965 9.986516,25.1228 19.304187,33.11161 10.72637,9.21589 23.98622,13.66056 35.97992,14.16521 15.17319,0.65167 28.41744,-4.70205 37.53053,-11.20097 10.33254,-7.35876 16.2633,-16.54391 19.41608,-22.47957 3.28267,-6.18023 4.39427,-9.86588 4.82896,-9.75441 0.43469,0.11148 0.18305,4.04021 -2.469,10.83394 -2.54633,6.52292 -8.03247,16.734 -18.79741,25.33709 -9.49784,7.58145 -23.70086,14.12496 -40.67327,13.99231 -13.43618,-0.11546 -28.40673,-4.60671 -40.90269,-14.81065 -10.849043,-8.87553 -19.511132,-21.96741 -22.818298,-37.43076 7e-6,0 -5e-6,-2e-5 -5e-6,-2e-5 -2.746851,-12.89898 -1.654995,-27.16646 3.993695,-40.26967 6.411219,-11.39384 16.852662,-21.43982 30.227338,-27.81728 -2.70398,12.99601 0.80589,28.68979 10.21856,40.24069 5.81763,7.61493 14.77421,13.1574 24.5087,15.21737 3.27177,0.69235 6.59691,0.98249 9.87789,0.83457 13.80635,-0.49189 26.92459,-8.62737 34.77766,-19.54209 8.35996,-11.5633 11.24825,-26.51365 9.70724,-40.12711 -1.65238,-14.663784 -8.41682,-28.234227 -17.1285,-39.026195 0,0 -1e-5,-1.4e-5 -1e-5,-1.4e-5 C 175.3844,53.813552 163.02514,44.847317 150.82679,38.246735 130.3007,27.1673 109.39092,22.633809 91.752267,21.403504 72.338088,20.055503 56.455882,22.662817 46.126626,24.974372 c -10.501593,2.35012 -16.083228,4.344035 -16.293605,3.82651 z"
     id="path04"
     inkscape:connector-curvature="0"
     inkscape:path-effect="#path-effect40;#path-effect42"
     inkscape:original-d="m 29.833021,28.800882 c 40.94699,1.973072 81.893729,3.946408 122.840189,5.92001 40.94646,1.973601 3.4536,89.786538 5.18001,134.680198 1.7264,44.89367 -45.88007,-59.94009 -45.88007,-59.94009 0,0 -19.239764,43.90647 -28.86005,65.8601 -9.620279,21.95363 80.90706,1.97307 121.36018,2.96" />
  <path
     style="fill:#000000;stroke:none;stroke-width:0.264583px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1"
     d="m 52.033054,65.800937 c -0.288529,0.950576 -8.829359,-1.930576 -20.716537,2.829914 -5.269898,2.104646 -11.153361,6.053549 -15.382565,12.256431 -3.376718,4.93848 -5.688537,11.405405 -5.663025,18.331178 3e-6,1e-6 0,8e-6 0,8e-6 0.01275,6.056912 1.856451,12.552752 5.662138,18.008502 4.066067,5.85763 10.581692,10.61626 18.058522,12.47214 7.592168,2.97699 16.192378,3.66056 23.678314,1.8489 0.248029,-0.06 0.494434,-0.12269 0.739141,-0.18799 -4.213983,5.64698 -9.945981,8.85764 -15.447369,9.36332 -6.625611,-0.91547 -12.431247,-4.5902 -16.10572,-9.82633 -3.815137,-5.50361 -4.999053,-12.23845 -3.646495,-18.40515 1.471194,-6.53262 5.635058,-11.89317 10.993164,-15.006957 3.537065,-1.998859 7.269118,-3.00363 10.7715,-3.388811 3.664337,-0.416771 7.169596,-0.315459 10.319221,-0.113102 7.43445,0.364729 12.716872,1.136992 17.899426,-0.374928 6.006601,-1.603062 10.470991,-5.79036 12.885679,-9.809727 2.764749,-4.571907 3.309888,-9.337638 3.404552,-12.279626 0.09913,-3.080772 -0.147251,-4.834618 0.290115,-4.977776 0.437366,-0.143158 1.571143,1.352487 2.353261,4.696783 0.782119,3.344297 1.155757,8.73932 -1.555123,14.861812 -2.383685,5.349794 -7.400385,11.239011 -15.297852,14.054382 -6.673684,2.25414 -13.145357,2.05784 -20.263304,1.9127 -3.002308,-0.0133 -6.06045,0.0242 -8.867246,0.50007 -2.664218,0.46554 -5.279246,1.23559 -7.359418,2.5904 -3.127244,1.95234 -5.64797,5.46731 -6.365615,9.28263 -0.70919,3.57962 0.141337,7.76459 2.389493,10.83317 2.112565,2.94562 5.726353,5.1539 9.452748,5.60339 4.484169,1.82365 9.462284,2.07491 13.237021,1.08579 -0.258884,0.19304 -0.519573,0.3826 -0.781997,0.56867 -7.941297,5.63063 -17.040999,7.77667 -25.181488,6.6931 -9.95171,-2.64184 -18.17122,-8.95295 -23.3206641,-16.80875 -4.7666569,-7.30028 -6.7713262,-15.6314 -6.4461853,-23.388801 0,0 3.4e-6,-1e-5 3.4e-6,-1e-5 0.382091,-8.855215 3.725626,-16.66288 8.303083,-22.476829 5.738163,-7.272119 13.155229,-11.284988 19.551363,-13.041156 14.433013,-3.950218 22.700388,1.342076 22.411859,2.292653 z"
     id="path03"
     inkscape:connector-curvature="0"
     inkscape:path-effect="#path-effect46;#path-effect48"
     inkscape:original-d="M 52.033054,65.800937 C 38.7133,83.807365 25.393281,101.81406 12.072995,119.82102 c -13.3202835,18.00696 45.880066,11.84002 45.880066,11.84002 0,0 -14.306422,-20.22696 -21.460031,-30.34005 -7.153608,-10.113087 25.160303,-2.960271 37.740055,-4.440003 12.579755,-1.47974 10.360282,-20.226966 15.540031,-30.34005" />
  <path
     style="fill:#000000;stroke:none;stroke-width:0.264583px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1"
     d="m 134.17318,62.10093 c -1.5723,-0.02872 -4.29583,-11.038763 3.87169,-25.071657 3.89388,-6.678699 10.3726,-13.47916 19.56967,-17.804763 5.3742,-2.521623 11.46503,-4.077301 17.89135,-4.306205 2.90799,-0.103582 5.7813,0.06886 8.57729,0.497678 9.76897,1.507633 18.00867,6.017723 23.97106,11.561621 8.09509,7.547989 11.49317,16.400767 12.60812,22.413795 1.13383,6.114855 0.28167,9.668163 -0.64905,9.749529 -0.93073,0.08136 -1.87128,-3.207378 -4.23063,-8.217311 -2.32001,-4.926406 -6.36517,-11.864669 -13.41034,-17.339453 -5.19411,-4.052552 -12.07137,-7.31163 -19.72005,-8.340855 -2.20836,-0.299833 -4.48266,-0.41417 -6.78842,-0.331001 -5.08994,0.183595 -9.96378,1.31365 -14.29539,3.139607 -7.41629,3.116219 -13.1447,8.178816 -17.02771,13.270209 -8.18778,10.715755 -8.79529,20.807523 -10.36759,20.778806 z"
     id="path02"
     inkscape:connector-curvature="0"
     inkscape:path-effect="#path-effect52;#path-effect54"
     inkscape:original-d="m 134.17318,62.10093 c 23.6803,-10.606945 47.36033,-21.213628 71.04011,-31.820046 23.67976,-10.606417 9.86694,19.239765 14.80002,28.860043" />
  <path
     style="fill:#000000;stroke:none;stroke-width:0.264583px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1"
     d="m 136.39319,81.340958 c 1.5563,-1.147771 19.80712,21.986322 43.43085,56.950132 7.14611,-15.93729 15.5595,-34.64514 24.11942,-53.515177 -0.0348,0.07673 4.64569,-10.225154 4.61089,-10.148421 -16.97017,-2.046757 -32.64607,-3.994748 -47.29774,-5.900043 -0.42109,10.969524 -0.76375,20.434472 -1.05269,29.604886 0,0 0,-3e-6 0,-3e-6 18.76272,-12.448829 31.75166,-18.904298 32.42935,-17.731375 0.67769,1.172924 -11.72291,9.38697 -34.17074,20.748683 0,0 -10e-6,0 -10e-6,0 -0.29378,-10.963791 -0.62675,-22.50186 -1.05269,-36.995216 16.92713,2.154857 35.83259,4.653001 56.3024,7.419976 -0.0328,0.07029 -5.30458,11.335985 -5.33743,11.406272 -9.98872,21.317358 -19.86304,42.235558 -28.09234,59.610338 -28.22937,-35.3455 -45.44557,-60.302281 -43.88927,-61.450052 z"
     id="path01"
     inkscape:connector-curvature="0"
     inkscape:path-effect="#path-effect58;#path-effect60"
     inkscape:original-d="m 136.39319,81.340958 c 14.30695,19.486432 43.66006,59.200092 43.66006,59.200092 0,0 31.08004,-67.340104 31.08004,-67.340104 0,0 -51.80007,-6.660009 -51.80007,-6.660009 0,0 0,33.300052 0,33.300052 0,0 33.30005,-19.24003 33.30005,-19.24003" />
   </g>
</svg>
)"""";

   testDoc(svg);
}