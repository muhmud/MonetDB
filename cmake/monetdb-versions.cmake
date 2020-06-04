#[[
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 1997 - July 2008 CWI, August 2008 - 2020 MonetDB B.V.
#]]

set(GDK_VERSION_MAJOR "18")
set(GDK_VERSION_MINOR "4")
set(GDK_VERSION_PATCH "0")
set(GDK_VERSION "${GDK_VERSION_MAJOR}.${GDK_VERSION_MINOR}.${GDK_VERSION_PATCH}")

set(MAPI_VERSION_MAJOR "12")
set(MAPI_VERSION_MINOR "1")
set(MAPI_VERSION_PATCH "0")
set(MAPI_VERSION "${MAPI_VERSION_MAJOR}.${MAPI_VERSION_MINOR}.${MAPI_VERSION_PATCH}")

set(MONETDB5_VERSION_MAJOR "27")
set(MONETDB5_VERSION_MINOR "3")
set(MONETDB5_VERSION_PATCH "0")
set(MONETDB5_VERSION "${MONETDB5_VERSION_MAJOR}.${MONETDB5_VERSION_MINOR}.${MONETDB5_VERSION_PATCH}")

set(STREAM_VERSION_MAJOR "13")
set(STREAM_VERSION_MINOR "2")
set(STREAM_VERSION_PATCH "0")
set(STREAM_VERSION "${STREAM_VERSION_MAJOR}.${STREAM_VERSION_MINOR}.${STREAM_VERSION_PATCH}")

set(MONETDB_VERSION_MAJOR "11")
set(MONETDB_VERSION_MINOR "35")
set(MONETDB_VERSION_PATCH "18")


if(RELEASE_VERSION)
  math(EXPR MONETDB_VERSION_PATCH "${MONETDB_VERSION_PATCH} + 1")
  set(MONETDB_RELEASE "default")
#else()
#  set(MONETDB_RELEASE "unreleased")
endif()
set(MONETDB_VERSION "${MONETDB_VERSION_MAJOR}.${MONETDB_VERSION_MINOR}.${MONETDB_VERSION_PATCH}")
