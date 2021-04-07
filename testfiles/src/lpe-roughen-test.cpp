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

class LPERoughenTest : public LPESTest {};

// INKSCAPE 0.92.5

TEST_F(LPERoughenTest, path_0_92_5)
{
   std::string svg = R""""(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!-- Created with Inkscape (http://www.inkscape.org/) -->

<svg
   xmlns:dc="http://purl.org/dc/elements/1.1/"
   xmlns:cc="http://creativecommons.org/ns#"
   xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
   xmlns:svg="http://www.w3.org/2000/svg"
   xmlns="http://www.w3.org/2000/svg"
   xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd"
   xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape"
   width="210mm"
   height="297mm"
   viewBox="0 0 210 297"
   version="1.1"
   id="svg8"
   inkscape:version="0.92.3 (unknown)"
   sodipodi:docname="1.svg">
  <defs
     id="defs2">
    <inkscape:path-effect
       effect="roughen"
       id="path-effect1112"
       is_visible="true"
       method="size"
       max_segment_size="10"
       segments="2"
       displace_x="10;1"
       displace_y="10;1"
       global_randomize="1;1"
       handles="along"
       shift_nodes="true"
       fixed_displacement="false"
       spray_tool_friendly="false" />
    <inkscape:path-effect
       effect="roughen"
       id="path-effect827"
       is_visible="true"
       method="size"
       max_segment_size="10"
       segments="2"
       displace_x="10;1"
       displace_y="10;1"
       global_randomize="1;1"
       handles="smooth"
       shift_nodes="true"
       fixed_displacement="false"
       spray_tool_friendly="false" />
    <inkscape:path-effect
       effect="roughen"
       id="path-effect822"
       is_visible="true"
       method="segments"
       max_segment_size="10"
       segments="2"
       displace_x="10;1"
       displace_y="10;1"
       global_randomize="1;1"
       handles="rand"
       shift_nodes="true"
       fixed_displacement="false"
       spray_tool_friendly="false" />
    <inkscape:path-effect
       effect="roughen"
       id="path-effect817"
       is_visible="true"
       method="segments"
       max_segment_size="10"
       segments="2"
       displace_x="10;1"
       displace_y="10;1"
       global_randomize="1;1"
       handles="along"
       shift_nodes="true"
       fixed_displacement="false"
       spray_tool_friendly="false" />
  </defs>
  <sodipodi:namedview
     id="base"
     pagecolor="#ffffff"
     bordercolor="#666666"
     borderopacity="1.0"
     inkscape:pageopacity="0.0"
     inkscape:pageshadow="2"
     inkscape:zoom="0.7"
     inkscape:cx="282.31395"
     inkscape:cy="652.46562"
     inkscape:document-units="mm"
     inkscape:current-layer="layer1"
     showgrid="false"
     inkscape:window-width="1920"
     inkscape:window-height="1016"
     inkscape:window-x="0"
     inkscape:window-y="27"
     inkscape:window-maximized="1" />
  <metadata
     id="metadata5">
    <rdf:RDF>
      <cc:Work
         rdf:about="">
        <dc:format>image/svg+xml</dc:format>
        <dc:type
           rdf:resource="http://purl.org/dc/dcmitype/StillImage" />
        <dc:title></dc:title>
      </cc:Work>
    </rdf:RDF>
  </metadata>
  <g
     inkscape:label="Layer 1"
     inkscape:groupmode="layer"
     id="layer1">
    <path
       style="fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:0.86199427"
       id="path815"
       inkscape:path-effect="#path-effect817"
       sodipodi:type="arc"
       sodipodi:cx="61.988094"
       sodipodi:cy="70.592255"
       sodipodi:rx="48.75893"
       sodipodi:ry="43.845234"
       d="m 103.80946,73.344025 c 0,12.107527 5.73556,26.45827 -3.08807,34.392695 -8.823633,7.93443 -33.248778,8.24526 -46.713185,8.24526 -13.464406,0 -27.963671,1.32865 -34.79233,-6.9919 -6.828658,-8.32055 -4.602434,-24.662491 -4.602434,-36.770018 0,-12.107527 3.78242,-21.948382 12.259981,-30.289761 8.477562,-8.341379 17.011594,-12.589286 30.476,-12.589286 13.464407,0 31.428066,11.707777 41.411364,18.993705 9.983304,7.285928 5.048674,12.901778 5.048674,25.009305 z" />
    <path
       style="fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:0.86199427"
       inkscape:transform-center-x="2.7892134"
       inkscape:transform-center-y="-1.8103274"
       d="m 85.512557,212.32123 -16.634443,-2.95104 -27.365796,-9.16534 -3.736968,16.55051 -19.903045,4.40482 1.258374,-11.94376 1.328259,-15.03152 -5.72346,-12.07427 -17.4440411,-15.61248 15.1772671,-4.44141 12.992667,-7.8406 4.265326,-17.54682 1.124762,-6.51106 6.358955,14.64259 17.966469,8.44446 22.524245,7.25918 15.56105,-15.1978 -12.328064,16.38841 -12.383911,17.75498 5.76618,13.92962 z"
       id="path819"
       inkscape:path-effect="#path-effect822"
       inkscape:original-d="M 79.752974,207.79761 49.492207,198.66036 24.809866,218.40841 24.148818,186.80514 -2.2599547,169.43333 27.592261,159.03868 35.953083,128.55426 55.063815,153.73329 86.639864,152.2647 68.598726,178.22083 Z"
       inkscape:connector-curvature="0" />
    <path
       style="fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:0.86199427"
       inkscape:transform-center-x="-1.3064587"
       inkscape:transform-center-y="-3.5501659"
       d="m 162.48877,200.39119 c 3.15937,4.89779 -6.5989,-2.07155 -8.4217,-4.18017 -3.3788,-3.9086 -11.81801,-4.92955 -16.04438,-7.9013 -2.02201,-1.42178 -4.57382,-3.10009 -5.82725,-5.23056 -1.08286,-1.84055 -0.95831,-4.9613 -2.43949,-6.89949 -1.75101,-2.29127 -8.62982,-0.93109 -9.63188,1.77295 -0.92493,2.49591 -4.41899,0.73578 -6.82809,1.86773 -2.95365,1.38782 -5.32221,3.84877 -8.0098,5.69999 -2.09702,1.44443 -4.73178,2.89538 -7.400642,3.36636 -2.6878,0.47432 1.606041,-5.13961 2.189732,-7.8058 0.58958,-2.69307 -0.568393,-6.72644 0.13008,-9.39334 0.64363,-2.45748 2.23627,-5.1042 3.34475,-7.38996 0.9783,-2.01729 2.6689,-5.55046 4.02896,-8.31317 1.2302,-2.49893 3.22624,-14.36257 0.81581,-12.96691 -3.47733,2.01341 -9.14337,0.75841 -12.413575,-1.57642 -1.828219,-1.3053 1.288747,-3.53391 -0.48012,-4.91858 -1.709588,-1.33825 -9.675483,-2.37999 -14.390919,-3.96943 -2.899923,-0.97749 0.809883,1.5828 3.801496,0.93837 4.784094,-1.03054 5.885343,-7.89905 10.759601,-8.33629 4.608707,-0.41342 16.308407,7.92926 20.775387,9.1364 2.83857,0.76708 6.44392,-2.29362 9.60369,-1.56226 2.65407,0.61431 -8.9316,-11.47637 -6.21683,-11.24938 3.71702,0.31079 10.43609,-5.08043 12.08478,-8.42628 0.5925,-1.20242 1.00329,-2.62099 1.26641,-3.93539 0.33302,-1.66358 3.01243,-3.10298 4.7526,-2.61296 2.20856,0.62191 -0.88876,4.81748 0.76841,6.4044 1.83021,1.75263 -6.26022,7.39987 -5.32521,9.7551 1.4142,3.56222 -1.64974,6.85874 1.56672,8.94291 1.67448,1.08502 11.03941,-2.21572 12.99122,1.69075 1.21332,2.42839 0.2974,0.72476 2.98364,1.11639 4.33629,0.63218 6.88561,-0.13768 11.1814,0.72792 3.34773,0.67456 13.90305,1.75753 17.18074,2.71622 2.44036,0.71379 -2.17308,1.57389 -1.44248,1.85968 2.49325,0.97527 -12.58198,-0.84249 -11.24264,1.47562 1.50977,2.61308 0.85846,6.79288 -1.08919,9.09815 -1.69216,2.00287 -6.42594,6.92061 -8.35473,8.69673 -1.67059,1.53836 0.46285,-0.52579 0.76575,-0.68614 2.67206,-1.4146 -7.91636,12.8879 -4.9097,12.57007 2.28568,-0.24161 -2.09508,-2.12287 -1.9481,0.17084 0.33111,5.16734 1.52091,9.11441 2.81896,14.127 0.75503,2.91565 5.44722,10.32247 8.60659,15.22025 z"
       id="path824"
       inkscape:path-effect="#path-effect827"
       inkscape:original-d="m 156.86011,194.56844 -28.19425,-17.99448 -30.807177,13.02441 8.401257,-32.37493 -21.906886,-25.2746 33.386526,-2.01433 17.26797,-28.64497 12.23274,31.13001 32.57908,7.57103 -25.82627,21.25373 z" />
    <g
       id="g1110"
       inkscape:path-effect="#path-effect1112">
      <path
         id="path829"
         d="m 193.83116,44.78952 c 0,2.692882 4.22015,14.360175 3.32637,16.782183 3.37423,8.529369 -10.39949,5.783438 -12.3962,7.216651 -8.11047,1.638219 -4.56397,6.807272 -4.69126,6.896075 -8.14378,9.300743 -2.97509,1.895815 -5.40814,-0.05701 -1.25684,-1.008778 -5.73748,-1.995849 -8.7365,-1.995849 -2.98513,0 -10.44273,2.096296 -13.04254,1.192263 -6.77229,1.852068 0.68987,5.582007 -0.80891,4.169647 1.49783,4.512008 -13.23734,-6.510821 -15.51797,-13.065487 -8.09844,-0.584645 0.0899,-7.86296 1.50442,-11.686416 0.98405,-2.659818 0.38285,-3.902755 0.38285,-5.529939 0,-2.692881 1.64551,-5.717553 2.65359,-8.202725 1.99484,-2.624726 1.53559,-4.259208 3.01015,-6.165116 1.19036,-2.228104 2.11527,0.911694 4.64784,-0.7595 0.93731,3.374756 2.99272,2.259205 6.93237,-0.08373 2.69497,-1.602713 5.39996,-9.964138 7.85264,-9.964138 2.98513,0 4.34825,-1.152772 7.27084,-0.358729 1.84626,-0.629771 7.26826,2.391752 10.46843,4.482138 2.71131,1.427011 4.96023,3.67832 6.86948,5.778494 2.12969,2.231644 9.00178,4.578106 10.04329,6.883794 1.12266,2.485359 -4.36075,1.841073 -4.36075,4.467394 z"
         style="fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:0.86199427"
         inkscape:original-d="m 194.27977,47.724701 a 27.025299,24.379465 0 0 1 -27.0253,24.379465 27.025299,24.379465 0 0 1 -27.0253,-24.379465 27.025299,24.379465 0 0 1 27.0253,-24.379465 27.025299,24.379465 0 0 1 27.0253,24.379465 z" />
      <path
         id="path831"
         d="m 256.12466,82.495094 c 0,3.783604 -1.25854,9.315143 -2.29138,12.593626 -0.40714,3.110761 -3.31587,6.0716 -5.41083,8.6856 -2.44756,1.8587 -5.64557,5.11556 -7.95451,6.60631 -2.37922,1.53612 -3.34849,1.84623 -6.13187,1.84623 -2.81813,0 -5.47544,-1.48205 -7.91648,-2.87409 -2.30641,-2.04226 -6.57803,-7.97944 -8.37545,-10.217659 -3.25791,-6.225951 0.20871,0.449952 -0.38661,-0.670642 -1.43512,-2.701405 -3.06994,-2.941577 -3.06994,-8.408187 0,-3.783603 3.76762,-3.983378 4.73923,-7.75779 4.35418,-0.603337 2.43768,-8.903415 3.99919,-13.019689 1.28936,-3.035458 1.58095,-10.705511 3.24698,-12.825662 1.84375,-2.346321 -0.0689,2.513954 2.67062,2.513955 2.81813,-10e-7 3.2619,-1.497755 6.18208,-0.261449 0.97733,-0.666804 5.49375,2.814132 8.80596,6.232203 0.69715,1.285192 8.28132,5.815979 10.3368,9.498863 1.85258,3.319339 1.55621,4.850153 1.55621,8.058381 z"
         style="fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:0.86199427"
         inkscape:original-d="M 254.75595,84.766365 A 20.410715,27.403273 0 0 1 234.34523,112.16964 20.410715,27.403273 0 0 1 213.93451,84.766365 20.410715,27.403273 0 0 1 234.34523,57.363092 20.410715,27.403273 0 0 1 254.75595,84.766365 Z" />
    </g>
  </g>
</svg>
)"""";

   testDoc(svg);
}


// INKSCAPE 1.0.2

TEST_F(LPERoughenTest, multi_PX_1_0_2)
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
   width="793.70081"
   height="1122.5197"
   viewBox="0 0 793.70081 1122.5197"
   version="1.1"
   id="svg8"
   inkscape:version="1.0.2 (e86c870879, 2021-01-15)"
   sodipodi:docname="1.svg">
  <defs
     id="defs2">
    <inkscape:path-effect
       effect="roughen"
       id="path-effect1112"
       is_visible="true"
       method="size"
       max_segment_size="10"
       segments="2"
       displace_x="10;1"
       displace_y="10;1"
       global_randomize="1;1"
       handles="along"
       shift_nodes="true"
       fixed_displacement="false"
       spray_tool_friendly="false"
       lpeversion="0" />
    <inkscape:path-effect
       effect="roughen"
       id="path-effect827"
       is_visible="true"
       method="size"
       max_segment_size="10"
       segments="2"
       displace_x="10;1"
       displace_y="10;1"
       global_randomize="1;1"
       handles="smooth"
       shift_nodes="true"
       fixed_displacement="false"
       spray_tool_friendly="false"
       lpeversion="0" />
    <inkscape:path-effect
       effect="roughen"
       id="path-effect822"
       is_visible="true"
       method="segments"
       max_segment_size="10"
       segments="2"
       displace_x="10;1"
       displace_y="10;1"
       global_randomize="1;1"
       handles="rand"
       shift_nodes="true"
       fixed_displacement="false"
       spray_tool_friendly="false"
       lpeversion="0" />
    <inkscape:path-effect
       effect="roughen"
       id="path-effect817"
       is_visible="true"
       method="segments"
       max_segment_size="10"
       segments="2"
       displace_x="10;1"
       displace_y="10;1"
       global_randomize="1;1"
       handles="along"
       shift_nodes="true"
       fixed_displacement="false"
       spray_tool_friendly="false"
       lpeversion="0" />
  </defs>
  <sodipodi:namedview
     id="base"
     pagecolor="#ffffff"
     bordercolor="#666666"
     borderopacity="1.0"
     inkscape:pageopacity="0.0"
     inkscape:pageshadow="2"
     inkscape:zoom="0.24748737"
     inkscape:cx="-184.95691"
     inkscape:cy="899.86107"
     inkscape:document-units="px"
     inkscape:current-layer="layer1"
     showgrid="false"
     inkscape:window-width="1920"
     inkscape:window-height="1016"
     inkscape:window-x="0"
     inkscape:window-y="27"
     inkscape:window-maximized="1"
     inkscape:document-rotation="0"
     units="px" />
  <metadata
     id="metadata5">
    <rdf:RDF>
      <cc:Work
         rdf:about="">
        <dc:format>image/svg+xml</dc:format>
        <dc:type
           rdf:resource="http://purl.org/dc/dcmitype/StillImage" />
        <dc:title></dc:title>
      </cc:Work>
    </rdf:RDF>
  </metadata>
  <g
     inkscape:label="Layer 1"
     inkscape:groupmode="layer"
     id="layer1">
    <path
       style="fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:3.1324"
       id="path815"
       inkscape:path-effect="#path-effect817"
       sodipodi:type="arc"
       sodipodi:cx="251.04749"
       sodipodi:cy="255.6395"
       sodipodi:rx="177.18523"
       sodipodi:ry="159.32932"
       d="m 421.29515,258.39127 c 0,43.99757 -8.63905,87.21933 -40.70327,116.05227 -32.06422,28.83293 -88.59593,42.06977 -137.52428,42.06977 -48.92835,0 -95.53419,-11.59735 -125.60344,-40.81641 -30.069246,-29.21906 -42.217624,-74.43202 -42.217624,-118.4296 0,-43.99757 18.157022,-82.70945 49.875174,-111.94933 31.71815,-29.23989 72.35875,-46.413795 121.2871,-46.413795 48.92835,0 98.99859,24.633775 132.22247,52.818205 33.22389,28.18444 42.66387,62.67131 42.66387,106.66889 z" />
    <path
       style="fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:3.1324"
       inkscape:transform-center-x="18.266201"
       inkscape:transform-center-y="-12.3149"
       d="m 321.36292,758.75406 -56.4864,-14.98437 -67.21776,-21.19867 -36.24241,42.55775 -52.40849,30.41205 0.38781,-53.56374 0.45769,-56.6515 -40.502531,-34.95209 -52.223111,-38.49029 54.491186,-18.13066 52.306586,-21.52985 15.27612,-57.69333 12.13556,-46.65757 31.52686,47.80215 43.13438,41.60402 64.10836,5.32511 57.14517,-17.13186 -36.08737,50.57137 -36.14321,51.93795 20.45578,52.88081 z"
       id="path819"
       inkscape:path-effect="#path-effect822"
       inkscape:original-d="m 315.60334,754.23044 -109.96469,-33.20391 -89.69323,71.76252 -2.40218,-114.84323 -95.966914,-63.12744 108.480054,-37.77315 30.38241,-110.77744 69.44655,91.49815 114.74429,-5.33672 -65.55975,94.32206 z"
       inkscape:connector-curvature="0" />
    <path
       style="fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:3.1324"
       inkscape:transform-center-x="-10.78605"
       inkscape:transform-center-y="-35.794478"
       d="m 595.20504,705.96772 c -0.11209,0.68308 -0.96817,3.63958 -3.0275,1.3004 -3.61835,-4.11006 -12.64014,-4.93798 -17.12002,-8.0869 -1.98534,-1.3955 -5.69459,-1.8913 -7.60699,-3.38519 -2.28499,-1.78494 -8.20071,3.73497 -10.4236,1.87325 -3.23756,-2.71152 -5.87223,-5.65388 -8.74039,-8.75353 -2.11373,-2.28433 -4.74942,-5.10609 -6.86425,-7.3894 -1.84817,-1.9954 -5.84119,-8.80073 -7.73003,-10.75768 -0.93511,-0.96882 -1.79224,-1.81045 -2.76301,-2.74352 -2.21444,-2.12843 -6.39893,-5.53095 -8.77651,-7.47544 -1.79145,-1.46513 -3.5064,-3.3745 -5.36567,-4.75255 -2.4501,-1.81597 -3.26219,-2.29123 -5.83694,-3.92567 -3.2259,-2.04779 -4.86862,-6.95751 -8.10302,-8.99185 -2.57769,-1.62128 -6.52778,-0.26153 -9.35079,-1.6991 -2.77662,-1.41395 -0.6047,1.73946 -3.60359,2.58533 -4.61904,1.30286 -3.34671,7.57416 -7.80389,9.35363 -4.20146,1.67737 -11.59878,2.18375 -15.94067,3.45408 -2.09145,0.61191 -1.9692,3.19821 -4.05003,3.84532 -3.46971,1.07901 -4.83525,8.03646 -8.32054,9.06404 -3.35741,0.98988 -15.24137,-1.65237 -18.67464,-0.97064 -0.48896,0.0971 0.97459,-3.48128 0.48691,-3.37791 -3.95819,0.83903 -7.79888,1.99679 -11.56934,3.46475 -2.95312,1.14974 -9.78324,7.36439 -12.6603,8.69304 -1.14798,0.53014 1.1687,-6.97146 -0.005,-6.50171 -5.00748,2.00364 -9.63797,2.83866 -14.14351,5.80338 -2.94057,1.93494 -11.88326,0.98253 -14.61303,3.20499 -2.41112,1.96304 1.66664,7.11896 -0.86556,7.92562 -2.95864,0.94249 7.23828,-9.03935 4.7204,-10.85654 -1.91932,-1.38519 0.84692,-5.00797 1.33362,-7.32436 0.60785,-2.89299 4.0826,-3.6206 4.727,-6.50567 0.79652,-3.56619 -3.84534,-11.0722 -3.20538,-14.66978 0.38311,-2.15363 0.80131,-4.18756 1.33338,-6.30931 0.77498,-3.09035 -1.12732,-3.01403 -0.20285,-6.06299 1.28637,-4.24253 0.96271,-10.25507 2.45776,-14.42862 0.94231,-2.63051 -1.14958,-8.65126 -0.0872,-11.23558 1.0233,-2.48913 1.98835,-6.05767 3.28549,-8.41571 1.35605,-2.46512 4.09215,-1.76247 5.58639,-4.14636 1.89764,-3.02747 4.57274,-7.12764 5.88435,-10.45124 0.99373,-2.51811 1.72239,-4.95354 2.32628,-7.59242 0.66289,-2.89674 1.4797,-6.56663 3.07862,-9.47851 1.50663,-2.74379 -5.81174,-3.47047 -7.01143,-6.36167 -1.20269,-2.89842 -5.93927,-2.60827 -7.95982,-5.00925 -2.1426,-2.54601 -0.54569,-6.0327 -2.56405,-8.67828 -2.20938,-2.89595 -2.50091,-8.6373 -4.86786,-11.40596 -1.88929,-2.20993 -10.51738,-4.81347 -12.54035,-6.90173 -1.03045,-1.0637 -1.98679,-2.21552 -2.99057,-3.30443 -2.12565,-2.30595 -1.07737,-4.58841 -3.12601,-6.96304 -2.5106,-2.91009 -5.93218,-2.61598 -8.55096,-5.42912 -2.5476,-2.73667 -7.01454,-8.30503 -9.37667,-11.2033 -1.26306,-1.54973 -0.14197,-3.06203 -1.38971,-4.62412 -2.28516,-2.86086 -7.01009,-5.21995 -9.55553,-7.8519 -1.90584,-1.97062 -3.33979,-10.38541 -4.90835,-12.63378 -1.66061,-2.38031 -8.29429,-3.77037 -11.33202,-7.01108 -2.22448,-2.37313 6.42654,0.74339 9.64886,0.29989 3.2619,-0.44895 5.08214,-1.9459 8.37407,-2.01468 3.75412,-0.0784 6.1002,0.28069 9.85488,0.32508 3.7193,0.044 6.11116,-1.32493 9.8306,-1.29418 3.70815,0.0307 8.07043,0.64435 11.77766,0.73278 3.02574,0.0722 3.23183,-3.72899 6.25787,-3.67082 4.29742,0.0826 6.53585,0.56779 10.82096,0.90311 3.90747,0.30577 6.03347,-0.69594 9.94013,-0.37991 3.87081,0.31314 5.15388,-1.00579 9.01846,-0.62326 4.16061,0.41184 10.3076,2.84713 14.45133,3.40378 2.44627,0.32862 8.05338,-1.31515 10.51763,-1.17466 2.28308,0.13017 2.77129,-0.12118 5.02181,0.28455 2.97371,0.53611 12.89776,1.58043 19.37327,1.47319 3.01144,-0.0499 2.42986,-3.04202 3.59034,-5.82132 1.44544,-3.46177 7.72939,-5.58616 9.44664,-8.92145 1.27542,-2.47716 6.03707,-6.02779 7.12094,-8.59454 0.90568,-2.14475 8.78129,-9.3807 9.57898,-11.5679 0.24398,-0.66897 4.03027,-2.67028 4.25046,-3.34746 0.8226,-2.52987 1.82569,-5.40054 2.30987,-8.01635 0.5145,-2.77967 2.538,-6.96468 2.99064,-9.75509 0.36866,-2.27265 0.96427,-3.49262 1.34359,-5.76351 0.51329,-3.07301 1.43474,-11.44753 2.10495,-14.49017 0.2417,-1.09728 2.41144,-4.42993 2.71268,-5.51237 0.54607,-1.96225 1.26719,-4.11408 1.92128,-6.04301 0.92996,-2.74247 1.34285,-5.89226 2.57415,-8.5133 1.03304,-2.199 5.18086,-5.97174 8.29283,-8.36638 2.45928,-1.8924 -1.66118,-1.13782 -0.37134,1.68452 2.37994,5.20764 8.17336,4.60047 10.18375,9.96162 1.6449,4.38647 4.47263,5.54575 5.67887,10.07254 1.05945,3.97592 -0.659,5.42999 0.25845,9.44106 0.90941,3.97594 0.18742,7.58767 1.21476,11.53479 0.84032,3.22856 1.38002,6.13597 2.34006,9.33098 0.92944,3.0932 0.99705,5.91494 2.00511,8.98341 1.02536,3.12113 0.18898,8.21824 1.30614,11.30771 0.93912,2.59707 3.16249,6.07657 4.27519,8.60415 1.07254,2.43637 0.11784,6.03423 1.21626,8.45906 1.28558,2.83796 3.55555,2.07058 5.14503,4.7502 2.02416,3.41244 3.74113,7.38131 5.04628,11.12812 1.06427,3.05531 2.87916,6.47091 5.86211,7.69648 2.81321,1.15582 5.88152,1.40046 8.82794,2.15458 2.95842,0.75719 4.56417,4.35347 7.52841,5.08756 3.20941,0.7948 1.45422,-7.92463 4.70399,-7.31554 5.38308,1.00892 13.6589,8.77618 18.85857,10.49633 1.37234,0.454 8.2774,0.26812 9.66238,0.68195 1.27492,0.38095 -1.51875,4.38261 -0.24195,4.75719 3.95818,1.16122 8.03662,2.09657 12.09478,2.8361 2.9965,0.54606 6.24844,0.91019 9.25869,1.37441 2.91889,0.45012 5.25783,0.41946 8.17549,0.87746 3.19993,0.50231 4.64481,2.36377 7.82873,2.95921 3.49743,0.65406 6.92984,3.73156 10.45896,4.18453 3.05506,0.39212 7.27186,-1.2958 10.34666,-1.47686 2.94931,-0.17367 7.2764,3.29225 10.24141,5.88205 2.338,2.04213 -5.35961,8.38993 -8.40649,7.79575 -2.37977,-0.46409 -10.81965,4.60214 -12.78964,6.0156 -0.73565,0.52782 -7.52872,-4.59929 -8.24651,-4.04742 -2.59795,1.99744 -1.65179,11.76903 -3.79163,14.25099 -1.22973,1.42635 -3.29512,2.76497 -4.66286,4.05956 -2.11475,2.00166 -3.4299,3.58924 -5.67815,5.43969 -2.63016,2.16478 -10.51359,0.30719 -13.22669,2.36707 -2.18853,1.66161 -2.57803,5.15771 -4.53892,7.08268 -2.19405,2.15384 0.73374,7.60331 -1.51399,9.70107 -2.85679,2.66617 -10.43665,5.39389 -13.85795,7.28186 -1.52102,0.83934 -3.64273,2.02336 -5.12966,2.92172 -2.44575,1.47765 -4.59757,4.53452 -6.91598,6.20488 -2.18985,1.57773 -1.85746,7.23469 -0.22962,10.23329 1.407,2.59177 0.54655,5.52658 0.43249,8.47343 -0.1188,3.06911 -0.21273,7.25391 -0.14782,10.32464 0.0546,2.58037 -2.63231,7.01583 -2.55889,9.59573 0.0683,2.39913 -2.04777,4.51487 -1.87072,6.90843 0.23385,3.16136 -0.76107,7.02091 -0.36579,10.16617 0.34958,2.78158 1.83795,6.14536 2.26864,8.91555 0.40857,2.62791 2.92193,5.37668 3.28407,8.01139 0.3811,2.77274 0.70662,5.63518 0.90127,8.42721 0.20345,2.91819 0.46805,6.03869 0.55895,8.96255 0.0896,2.88357 2.34836,8.44739 2.37635,11.33221 0.0207,2.13861 -0.35991,4.36437 -0.61252,6.48811 -0.33981,2.85697 -0.77968,12.42552 -1.03769,15.29103 -0.2364,2.62552 -0.75015,1.20611 -0.86224,1.88919 z"
       id="path824"
       inkscape:path-effect="#path-effect827"
       inkscape:original-d="M 595.80318,706.15693 493.348,640.76672 l -111.95024,47.32944 30.52929,-117.64737 -79.60747,-91.84544 121.32338,-7.31984 62.75013,-104.09304 44.45258,113.1234 118.3892,27.5124 -93.85015,77.23398 z" />
    <g
       id="g1110"
       inkscape:path-effect="#path-effect1112"
       transform="matrix(3.6339029,0,0,3.6339029,25.788782,-0.88592023)">
      <path
         id="path829"
         d="m 193.83116,44.78952 c 0,2.692882 4.22015,14.360175 3.32637,16.782183 3.37423,8.529369 -10.39949,5.783438 -12.3962,7.216651 -8.11047,1.638219 -4.56397,6.807272 -4.69126,6.896075 -8.14378,9.300743 -2.97509,1.895815 -5.40814,-0.05701 -1.25684,-1.008778 -5.73748,-1.995849 -8.7365,-1.995849 -2.98513,0 -10.44273,2.096296 -13.04254,1.192263 -6.77229,1.852068 0.68987,5.582007 -0.80891,4.169647 1.49783,4.512008 -13.23734,-6.510821 -15.51797,-13.065487 -8.09844,-0.584645 0.0899,-7.86296 1.50442,-11.686416 0.98405,-2.659818 0.38285,-3.902755 0.38285,-5.529939 0,-2.692881 1.64551,-5.717553 2.65359,-8.202725 1.99484,-2.624726 1.53559,-4.259208 3.01015,-6.165116 1.19036,-2.228104 2.11527,0.911694 4.64784,-0.7595 0.93731,3.374756 2.99272,2.259205 6.93237,-0.08373 2.69497,-1.602713 5.39996,-9.964138 7.85264,-9.964138 2.98513,0 4.34825,-1.152772 7.27084,-0.358729 1.84626,-0.629771 7.26826,2.391752 10.46843,4.482138 2.71131,1.427011 4.96023,3.67832 6.86948,5.778494 2.12969,2.231644 9.00178,4.578106 10.04329,6.883794 1.12266,2.485359 -4.36075,1.841073 -4.36075,4.467394 z"
         style="fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:0.861994"
         inkscape:original-d="m 194.27977,47.724701 a 27.025299,24.379465 0 0 1 -27.0253,24.379465 27.025299,24.379465 0 0 1 -27.0253,-24.379465 27.025299,24.379465 0 0 1 27.0253,-24.379465 27.025299,24.379465 0 0 1 27.0253,24.379465 z" />
      <path
         id="path831"
         d="m 256.12466,82.495094 c 0,3.783604 -1.25854,9.315143 -2.29138,12.593626 -0.40714,3.110761 -3.31587,6.0716 -5.41083,8.6856 -2.44756,1.8587 -5.64557,5.11556 -7.95451,6.60631 -2.37922,1.53612 -3.34849,1.84623 -6.13187,1.84623 -2.81813,0 -5.47544,-1.48205 -7.91648,-2.87409 -2.30641,-2.04226 -6.57803,-7.97944 -8.37545,-10.217659 -3.25791,-6.225951 0.20871,0.449952 -0.38661,-0.670642 -1.43512,-2.701405 -3.06994,-2.941577 -3.06994,-8.408187 0,-3.783603 3.76762,-3.983378 4.73923,-7.75779 4.35418,-0.603337 2.43768,-8.903415 3.99919,-13.019689 1.28936,-3.035458 1.58095,-10.705511 3.24698,-12.825662 1.84375,-2.346321 -0.0689,2.513954 2.67062,2.513955 2.81813,-10e-7 3.2619,-1.497755 6.18208,-0.261449 0.97733,-0.666804 5.49375,2.814132 8.80596,6.232203 0.69715,1.285192 8.28132,5.815979 10.3368,9.498863 1.85258,3.319339 1.55621,4.850153 1.55621,8.058381 z"
         style="fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:0.861994"
         inkscape:original-d="M 254.75595,84.766365 A 20.410715,27.403273 0 0 1 234.34523,112.16964 20.410715,27.403273 0 0 1 213.93451,84.766365 20.410715,27.403273 0 0 1 234.34523,57.363092 20.410715,27.403273 0 0 1 254.75595,84.766365 Z" />
    </g>
  </g>
</svg>
)"""";

   testDoc(svg);
}

TEST_F(LPERoughenTest, multi_MM_1_0_2)
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
   width="210mm"
   height="297mm"
   viewBox="0 0 210 297"
   version="1.1"
   id="svg8"
   inkscape:version="1.0.2 (e86c870879, 2021-01-15)"
   sodipodi:docname="1.svg">
  <defs
     id="defs2">
    <inkscape:path-effect
       effect="roughen"
       id="path-effect1112"
       is_visible="true"
       method="size"
       max_segment_size="10"
       segments="2"
       displace_x="10;1"
       displace_y="10;1"
       global_randomize="1;1"
       handles="along"
       shift_nodes="true"
       fixed_displacement="false"
       spray_tool_friendly="false" />
    <inkscape:path-effect
       effect="roughen"
       id="path-effect827"
       is_visible="true"
       method="size"
       max_segment_size="10"
       segments="2"
       displace_x="10;1"
       displace_y="10;1"
       global_randomize="1;1"
       handles="smooth"
       shift_nodes="true"
       fixed_displacement="false"
       spray_tool_friendly="false" />
    <inkscape:path-effect
       effect="roughen"
       id="path-effect822"
       is_visible="true"
       method="segments"
       max_segment_size="10"
       segments="2"
       displace_x="10;1"
       displace_y="10;1"
       global_randomize="1;1"
       handles="rand"
       shift_nodes="true"
       fixed_displacement="false"
       spray_tool_friendly="false" />
    <inkscape:path-effect
       effect="roughen"
       id="path-effect817"
       is_visible="true"
       method="segments"
       max_segment_size="10"
       segments="2"
       displace_x="10;1"
       displace_y="10;1"
       global_randomize="1;1"
       handles="along"
       shift_nodes="true"
       fixed_displacement="false"
       spray_tool_friendly="false" />
  </defs>
  <sodipodi:namedview
     id="base"
     pagecolor="#ffffff"
     bordercolor="#666666"
     borderopacity="1.0"
     inkscape:pageopacity="0.0"
     inkscape:pageshadow="2"
     inkscape:zoom="0.24748737"
     inkscape:cx="-184.95691"
     inkscape:cy="899.86107"
     inkscape:document-units="mm"
     inkscape:current-layer="layer1"
     showgrid="false"
     inkscape:window-width="1920"
     inkscape:window-height="1016"
     inkscape:window-x="0"
     inkscape:window-y="27"
     inkscape:window-maximized="1"
     inkscape:document-rotation="0" />
  <metadata
     id="metadata5">
    <rdf:RDF>
      <cc:Work
         rdf:about="">
        <dc:format>image/svg+xml</dc:format>
        <dc:type
           rdf:resource="http://purl.org/dc/dcmitype/StillImage" />
        <dc:title></dc:title>
      </cc:Work>
    </rdf:RDF>
  </metadata>
  <g
     inkscape:label="Layer 1"
     inkscape:groupmode="layer"
     id="layer1">
    <path
       style="fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:0.773433"
       id="path815"
       inkscape:path-effect="#path-effect817"
       sodipodi:type="arc"
       sodipodi:cx="71.503349"
       sodipodi:cy="123.94576"
       sodipodi:rx="43.749462"
       sodipodi:ry="39.340595"
       d="m 108.31525,126.69753 c 0,10.86361 6.29626,24.08819 -1.62084,31.20744 -7.917091,7.11925 -31.089869,6.92588 -43.17095,6.92588 -12.08108,0 -25.327978,1.83285 -31.250101,-5.67252 -5.922123,-7.50538 -3.135194,-22.72115 -3.135194,-33.58476 0,-10.86361 3.221715,-19.5783 10.792741,-27.1045 7.571026,-7.5262 14.852691,-11.269908 26.933771,-11.269908 12.081081,0 28.792372,11.203578 37.869133,17.674328 9.07677,6.47075 3.58144,10.96044 3.58144,21.82404 z" />
    <path
       style="fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:0.773433"
       inkscape:transform-center-x="2.1855094"
       inkscape:transform-center-y="-1.400581"
       d="m 93.202657,251.57833 -15.079954,-2.48167 -25.811308,-8.69596 -2.469042,15.53606 -18.635119,3.39037 1.292332,-10.32031 1.362217,-13.40807 -4.366848,-11.18188 -16.087428,-14.72009 13.643766,-3.90744 11.459165,-7.30663 3.835833,-15.98085 0.695268,-4.94508 5.377242,13.34915 16.984755,7.15102 20.902191,7.33462 13.938993,-15.12236 -11.401293,15.05505 -11.457142,16.42162 5.193189,12.41027 z"
       id="path819"
       inkscape:path-effect="#path-effect822"
       inkscape:original-d="m 87.443074,247.05471 -27.15179,-8.1985 -22.146489,17.71915 -0.593132,-28.35637 -23.695548,-15.58703 26.785213,-9.32671 7.501835,-27.35247 17.147305,22.59215 28.33194,-1.31771 -16.1876,23.28941 z"
       inkscape:connector-curvature="0" />
    <path
       style="fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:0.773433"
       inkscape:transform-center-x="-1.2887018"
       inkscape:transform-center-y="-2.5910236"
       d="m 162.25692,241.00745 c 3.08542,4.62823 -6.07783,-1.81244 -7.69753,-3.71798 -3.1639,-3.72224 -11.33459,-4.61543 -15.31954,-7.4413 -1.83578,-1.30181 -4.13278,-2.85819 -5.16647,-4.85727 -0.86982,-1.68218 -0.42654,-4.53649 -1.65262,-6.34623 -1.45685,-2.15034 -8.12952,-1.05972 -8.8406,1.43842 -0.65299,2.29408 -3.8833,0.50925 -6.03751,1.53325 -2.70055,1.28371 -4.797,3.64122 -7.22668,5.38407 -1.86994,1.34134 -4.20754,2.66204 -6.60049,3.01317 -2.4171,0.35467 1.46187,-4.58555 1.97394,-6.97425 0.51784,-2.41563 -0.71226,-6.17207 -0.0857,-8.5618 0.57471,-2.19195 2.09664,-4.56336 3.13474,-6.57762 0.91537,-1.77613 2.53608,-4.97606 3.80739,-7.46242 1.13788,-2.2254 3.50482,-13.63145 1.37849,-12.31773 -3.27476,2.02326 -8.67817,1.22347 -11.86585,-0.93439 -1.63011,-1.10348 1.67871,-3.11415 0.0956,-4.28409 -1.54889,-1.14465 -9.293395,-1.91926 -13.826344,-3.29843 -2.653777,-0.80742 0.235356,1.58836 2.943966,0.99011 4.500276,-0.99397 5.312954,-7.8619 9.902178,-8.28559 4.35185,-0.40178 15.7499,8.0089 19.94337,9.23967 2.57772,0.75655 5.8492,-2.25087 8.72054,-1.56101 2.38125,0.57212 -9.09414,-10.78637 -6.66036,-10.51363 3.47232,0.38912 10.12478,-4.53585 11.63814,-7.68517 0.46703,-0.97188 0.78125,-2.14873 0.92418,-3.21749 0.19158,-1.43249 2.76157,-2.37071 4.2109,-1.86475 1.89602,0.6619 -1.05116,4.27559 0.45422,5.60483 1.6875,1.49006 -6.4709,6.86354 -5.63938,8.95555 1.35165,3.40058 -1.74211,6.23584 1.41099,8.09294 1.49826,0.88243 10.6042,-2.77954 12.51852,0.94155 1.11089,2.15935 -0.25721,0.57932 2.14685,0.92193 4.05531,0.57792 6.32687,-0.26435 10.34492,0.53241 3.06992,0.60876 13.34554,1.63146 16.34466,2.52607 2.19912,0.65597 -3.12161,1.24144 -2.28029,1.66196 2.13889,1.06907 -11.86383,-1.08714 -10.57929,0.92972 1.51571,2.37983 1.3415,6.36537 -0.43249,8.55944 -1.46739,1.81488 -5.97893,6.55095 -7.68504,8.1435 -1.48144,1.38285 0.88277,-0.92071 1.42939,-1.2319 2.3804,-1.3552 -7.68407,12.17122 -4.98334,11.71415 2.02935,-0.34345 -2.1494,-2.7399 -2.02124,-0.68569 0.30533,4.89387 1.51017,8.53898 2.77848,13.2755 0.71147,2.657 5.41388,9.73228 8.4993,14.3605 z"
       id="path824"
       inkscape:path-effect="#path-effect827"
       inkscape:original-d="m 156.62826,235.1847 -25.29759,-16.14574 -27.64205,11.68629 7.5381,-29.04875 -19.656176,-22.6779 29.956406,-1.80737 15.49387,-25.702 10.97595,27.93172 29.23192,6.79319 -23.17289,19.07013 z" />
    <g
       id="g1110"
       inkscape:path-effect="#path-effect1112"
       transform="matrix(0.89726048,0,0,0.89726048,15.883883,60.606119)">
      <path
         id="path829"
         d="m 193.83116,44.78952 c 0,2.692882 4.22015,14.360175 3.32637,16.782183 3.37423,8.529369 -10.39949,5.783438 -12.3962,7.216651 -8.11047,1.638219 -4.56397,6.807272 -4.69126,6.896075 -8.14378,9.300743 -2.97509,1.895815 -5.40814,-0.05701 -1.25684,-1.008778 -5.73748,-1.995849 -8.7365,-1.995849 -2.98513,0 -10.44273,2.096296 -13.04254,1.192263 -6.77229,1.852068 0.68987,5.582007 -0.80891,4.169647 1.49783,4.512008 -13.23734,-6.510821 -15.51797,-13.065487 -8.09844,-0.584645 0.0899,-7.86296 1.50442,-11.686416 0.98405,-2.659818 0.38285,-3.902755 0.38285,-5.529939 0,-2.692881 1.64551,-5.717553 2.65359,-8.202725 1.99484,-2.624726 1.53559,-4.259208 3.01015,-6.165116 1.19036,-2.228104 2.11527,0.911694 4.64784,-0.7595 0.93731,3.374756 2.99272,2.259205 6.93237,-0.08373 2.69497,-1.602713 5.39996,-9.964138 7.85264,-9.964138 2.98513,0 4.34825,-1.152772 7.27084,-0.358729 1.84626,-0.629771 7.26826,2.391752 10.46843,4.482138 2.71131,1.427011 4.96023,3.67832 6.86948,5.778494 2.12969,2.231644 9.00178,4.578106 10.04329,6.883794 1.12266,2.485359 -4.36075,1.841073 -4.36075,4.467394 z"
         style="fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:0.861994"
         inkscape:original-d="m 194.27977,47.724701 a 27.025299,24.379465 0 0 1 -27.0253,24.379465 27.025299,24.379465 0 0 1 -27.0253,-24.379465 27.025299,24.379465 0 0 1 27.0253,-24.379465 27.025299,24.379465 0 0 1 27.0253,24.379465 z" />
      <path
         id="path831"
         d="m 256.12466,82.495094 c 0,3.783604 -1.25854,9.315143 -2.29138,12.593626 -0.40714,3.110761 -3.31587,6.0716 -5.41083,8.6856 -2.44756,1.8587 -5.64557,5.11556 -7.95451,6.60631 -2.37922,1.53612 -3.34849,1.84623 -6.13187,1.84623 -2.81813,0 -5.47544,-1.48205 -7.91648,-2.87409 -2.30641,-2.04226 -6.57803,-7.97944 -8.37545,-10.217659 -3.25791,-6.225951 0.20871,0.449952 -0.38661,-0.670642 -1.43512,-2.701405 -3.06994,-2.941577 -3.06994,-8.408187 0,-3.783603 3.76762,-3.983378 4.73923,-7.75779 4.35418,-0.603337 2.43768,-8.903415 3.99919,-13.019689 1.28936,-3.035458 1.58095,-10.705511 3.24698,-12.825662 1.84375,-2.346321 -0.0689,2.513954 2.67062,2.513955 2.81813,-10e-7 3.2619,-1.497755 6.18208,-0.261449 0.97733,-0.666804 5.49375,2.814132 8.80596,6.232203 0.69715,1.285192 8.28132,5.815979 10.3368,9.498863 1.85258,3.319339 1.55621,4.850153 1.55621,8.058381 z"
         style="fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:0.861994"
         inkscape:original-d="M 254.75595,84.766365 A 20.410715,27.403273 0 0 1 234.34523,112.16964 20.410715,27.403273 0 0 1 213.93451,84.766365 20.410715,27.403273 0 0 1 234.34523,57.363092 20.410715,27.403273 0 0 1 254.75595,84.766365 Z" />
    </g>
  </g>
</svg>
)"""";

   testDoc(svg);
}