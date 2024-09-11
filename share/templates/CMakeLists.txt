# SPDX-License-Identifier: GPL-2.0-or-later

FILE(GLOB ICON_FILES "${CMAKE_CURRENT_SOURCE_DIR}/icons/*.svg")
install(FILES ${ICON_FILES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/templates/icons)

file(GLOB _FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.svg")

#add_custom_command(
#  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/templates.h
#  COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/i18n.py ${_FILES} > ${CMAKE_CURRENT_BINARY_DIR}/templates.h
#  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/i18n.py ${_FILES}
#)
#set_source_files_properties(${CMAKE_BINARY_DIR}/templates.h PROPERTIES GENERATED TRUE)
#add_custom_target(templates_h ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/templates.h)
#install(FILES ${_FILES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/templates)

message(STATUS "Generation of localized default templates will be skipped (building without NLS)")
