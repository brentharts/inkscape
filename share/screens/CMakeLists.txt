# SPDX-License-Identifier: GPL-2.0-or-later

file(GLOB _FILES "*.svg" "*.png")

filter_and_install_translated_content(_FILES ${INKSCAPE_SHARE_INSTALL}/screens)
install(FILES ${_FILES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/screens)

file(GLOB _ABOUT "about/*.svgz" "about/*.svg")
install(FILES ${_ABOUT} DESTINATION ${INKSCAPE_SHARE_INSTALL}/screens/about)
