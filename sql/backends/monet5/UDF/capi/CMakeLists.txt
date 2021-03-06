#[[
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 1997 - July 2008 CWI, August 2008 - 2020 MonetDB B.V.
#]]

if(CINTEGRATION)
  # TODO add code too regenerate the cheader.text.h file
  add_library(capi MODULE)
	
  target_sources(capi
    PRIVATE
    capi.c capi.h
    cheader.h cheader.text.h)

  target_include_directories(capi
    PRIVATE
    $<TARGET_PROPERTY:mal,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:atoms,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:malmodules,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:sql,INCLUDE_DIRECTORIES>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${INCLUDEDIR}/monetdb>)

  target_link_libraries(capi
    PRIVATE
    mutils
    stream
    bat
    monetdb5
    monetdb_config_header)

  set_target_properties(capi
    PROPERTIES
    OUTPUT_NAME
    _capi)

  install(TARGETS
    capi
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/monetdb5
    COMPONENT server
    ARCHIVE
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/monetdb5
    PUBLIC_HEADER
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/monetdb
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

# vim: set ts=2:sw=2:et
