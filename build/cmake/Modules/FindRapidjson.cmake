# Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; version 2 of the
# License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301  USA
#
# Find the native RAPIDJSON includes
#
#  RAPIDJSON_INCLUDE_DIRS - where to find RAPIDJSON/RAPIDJSON.h, etc.
#  RAPIDJSON_FOUND        - True if RAPIDJSON found.


if (RAPIDJSON_INCLUDE_DIRS)
  # Already in cache, be silent
  set(RAPIDJSON_FIND_QUIETLY TRUE)
endif ()

find_path(RAPIDJSON_INCLUDE_DIRS NAMES rapidjson/rapidjson.h PATHS ${RAPIDJSON_INCLUDE_DIR} DOC "Include directory for RAPIDJSON.")
if(NOT RAPIDJSON_INCLUDE_DIRS)
  message(FATAL_ERROR "Unable to find rapidjson/rapidjson.h")
endif(NOT RAPIDJSON_INCLUDE_DIRS)


set(RAPIDJSON_HEADER_FILE ${RAPIDJSON_INCLUDE_DIRS}/rapidjson/rapidjson.h)
file(STRINGS ${RAPIDJSON_HEADER_FILE} RAPIDJSON_VERSION_LINE_MAJOR REGEX "#define RAPIDJSON_MAJOR_VERSION[ ]+[0-9]+")
if (RAPIDJSON_VERSION_LINE_MAJOR)
  file(STRINGS ${RAPIDJSON_HEADER_FILE} RAPIDJSON_VERSION_LINE_MINOR REGEX "#define RAPIDJSON_MINOR_VERSION[ ]+[0-9]+")
  file(STRINGS ${RAPIDJSON_HEADER_FILE} RAPIDJSON_VERSION_LINE_MICRO REGEX "#define RAPIDJSON_PATCH_VERSION[ ]+[0-9]+")
  string(REGEX MATCH "([0-9]+)" RAPIDJSON_VERSION_MAJOR "${RAPIDJSON_VERSION_LINE_MAJOR}")
  string(REGEX MATCH "([0-9]+)" RAPIDJSON_VERSION_MINOR "${RAPIDJSON_VERSION_LINE_MINOR}")
  string(REGEX MATCH "([0-9]+)" RAPIDJSON_VERSION_MICRO "${RAPIDJSON_VERSION_LINE_MICRO}")
  set(RAPIDJSON_VERSION_STRING ${RAPIDJSON_VERSION_MAJOR}.${RAPIDJSON_VERSION_MINOR}.${RAPIDJSON_VERSION_MICRO})
  # handle the QUIETLY and REQUIRED arguments and set LibSSH_FOUND to TRUE if
  # all listed variables are TRUE
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Rapidjson
                                FOUND_VAR RAPIDJSON_FOUND
                                REQUIRED_VARS RAPIDJSON_INCLUDE_DIRS
                                VERSION_VAR RAPIDJSON_VERSION_STRING)
else(RAPIDJSON_VERSION_LINE_MAJOR)
  message(FATAL_ERROR " Unable to detect Rapidjson version")
  set(RAPIDJSON_FOUND FALSE)
endif(RAPIDJSON_VERSION_LINE_MAJOR)

add_library(RAPIDJSON_Iface INTERFACE)
add_library(Rapidjson::Rapidjson ALIAS RAPIDJSON_Iface)
target_include_directories(RAPIDJSON_Iface SYSTEM INTERFACE
	${RAPIDJSON_INCLUDE_DIRS})
target_compile_definitions(RAPIDJSON_Iface INTERFACE
  -DRAPIDJSON_HAS_STDSTRING)

mark_as_advanced(RAPIDJSON_INCLUDE_DIRS)
