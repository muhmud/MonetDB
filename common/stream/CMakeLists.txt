#[[
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 1997 - July 2008 CWI, August 2008 - 2020 MonetDB B.V.
#]]

set(stream_public_headers
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/stream.h>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/stream_socket.h>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/monetdb/stream.h>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/monetdb/stream_socket.h>)

add_library(stream
  SHARED)

target_sources(stream
  PRIVATE
  stream.c
  PUBLIC
  ${stream_public_headers})

target_include_directories(stream
  PRIVATE
  $<$<BOOL:${Iconv_IS_BUILT_IN}>:${CMAKE_REQUIRED_INCLUDES}>
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/monetdb>)

if(WIN32)
  target_compile_definitions(stream
    PRIVATE
    LIBSTREAM)
endif()

target_link_libraries(stream
  PUBLIC
  $<$<BOOL:${ZLIB_FOUND}>:ZLIB::ZLIB>
  $<$<BOOL:${BZIP2_FOUND}>:BZip2::BZip2>
  $<$<BOOL:${CURL_FOUND}>:CURL::libcurl>
  $<$<BOOL:${LIBLZMA_FOUND}>:LibLZMA::LibLZMA>
  $<$<BOOL:${LZ4_FOUND}>:LZ4::LZ4>
  $<$<BOOL:${SNAPPY_FOUND}>:SNAPPY::SNAPPY>
  $<$<BOOL:${Iconv_FOUND}>:Iconv::Iconv>
  PRIVATE
  matomic
  monetdb_config_header
  $<$<PLATFORM_ID:Windows>:ws2_32>)

set_target_properties(stream
  PROPERTIES
  VERSION ${STREAM_VERSION}
  SOVERSION ${STREAM_VERSION_MAJOR}
  PUBLIC_HEADER "${stream_public_headers}")

if (NOT WIN32)
  if(ZLIB_FOUND)
    set(PKG_ZLIB "zlib")
  endif()

  if(BZIP2_FOUND)
    set(PKG_BZIP2 "bzip2")
  endif()

  if(CURL_FOUND)
    set(PKG_CURL "${CMAKE_SHARED_LIBRARY_PREFIX}curl")
  endif()

  if(LZ4_FOUND)
    set(PKG_LZ4 "${CMAKE_SHARED_LIBRARY_PREFIX}lz4")
  endif()

  if(LIBLZMA_FOUND)
    set(PKG_LZMA "${CMAKE_SHARED_LIBRARY_PREFIX}lzma")
  endif()

  if(SNAPPY_FOUND)
    set(PKG_SNAPPY "snappy")
  endif()

  if(ICONV_FOUND AND NOT Iconv_IS_BUILT_IN)
    get_filename_component(ICONV_LIBRARIES_PATH
      "${ICONV_LIBRARIES}"
      DIRECTORY)
    set(PKG_LIBICONV "-L${ICONV_LIBRARIES_PATH} -l${ICONV_LIBRARIES}")
  endif()

  configure_file(monetdb-stream.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/monetdb-stream.pc @ONLY)

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/monetdb-stream.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    COMPONENT streamdev)
endif()

install(TARGETS
  stream
  EXPORT streamTargets
  RUNTIME
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT stream
  LIBRARY
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT stream
  NAMELINK_COMPONENT streamdev
  ARCHIVE
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT stream
  PUBLIC_HEADER
  COMPONENT streamdev
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/monetdb
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  COMPONENT streamdev)

install(EXPORT streamTargets
  FILE streamTargets.cmake
  NAMESPACE MonetDB::
  DESTINATION ${EXPORT_TARGET_PATH}
  COMPONENT streamdev)
