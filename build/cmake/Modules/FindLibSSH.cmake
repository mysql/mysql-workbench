# Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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
# Find the native LibSSH includes and library
#
#  LibSSH_INCLUDE_DIRS - where to find libsshpp.hpp, etc.
#  LibSSH_LIBRARIES    - List of libraries when using LibSSH.
#  LibSSH_FOUND        - True if LibSSH found.


if (LibSSH_INCLUDE_DIRS AND LibSSH_INCLUDE_DIRS)
  # Already in cache, be silent
  set(LibSSH_FIND_QUIETLY TRUE)
endif ()

find_path(LibSSH_INCLUDE_DIR NAMES libssh/libsshpp.hpp PATHS /usr/include /usr/local/include)

SET(LibSSH_NAMES ssh)

if(LibSSH_LIBRARIES)
  # Converto to a list of library argments
  string(REPLACE " " ";" LibSSH_LIB_ARGS ${LibSSH_LIBRARIES})

  # Parse the list in order to find the library path
  foreach(LibSSH_LIB_ARG ${LibSSH_LIB_ARGS})
    string(REPLACE "-L" "" LibSSH_LIB_ARG_CLEAR ${LibSSH_LIB_ARG})
    if(NOT ${LibSSH_LIB_ARG_CLEAR} STREQUAL ${LibSSH_LIB_ARG})
      set(LibSSH_SUPPLIED_LIB_DIR ${LibSSH_LIB_ARG_CLEAR})
    endif()
  endforeach(LibSSH_LIB_ARG)
  find_library(LibSSH_LIBRARY NAMES ${LibSSH_NAMES} HINTS ${LibSSH_SUPPLIED_LIB_DIR})
  
  unset(LibSSH_LIB_ARG_CLEAR)
  unset(LibSSH_LIB_ARG)
  unset(LibSSH_LIB_ARGS)
else()
  find_library(LibSSH_LIBRARY NAMES ${LibSSH_NAMES})
endif()

set(LibSSH_HEADER_FILE ${LibSSH_INCLUDE_DIR}/libssh/libssh.h)
file(STRINGS ${LibSSH_HEADER_FILE} LibSSH_VERSION_LINE_MAJOR REGEX "#define LIBSSH_VERSION_MAJOR[ ]+[0-9]+")
if (LibSSH_VERSION_LINE_MAJOR)
  file(STRINGS ${LibSSH_HEADER_FILE} LibSSH_VERSION_LINE_MINOR REGEX "#define LIBSSH_VERSION_MINOR[ ]+[0-9]+")
  file(STRINGS ${LibSSH_HEADER_FILE} LibSSH_VERSION_LINE_MICRO REGEX "#define LIBSSH_VERSION_MICRO[ ]+[0-9]+")
  string(REGEX MATCH "([0-9]+)" LibSSH_VERSION_MAJOR "${LibSSH_VERSION_LINE_MAJOR}")
  string(REGEX MATCH "([0-9]+)" LibSSH_VERSION_MINOR "${LibSSH_VERSION_LINE_MINOR}")
  string(REGEX MATCH "([0-9]+)" LibSSH_VERSION_MICRO "${LibSSH_VERSION_LINE_MICRO}")
  set(LibSSH_VERSION_STRING ${LibSSH_VERSION_MAJOR}.${LibSSH_VERSION_MINOR}.${LibSSH_VERSION_MICRO})
  # handle the QUIETLY and REQUIRED arguments and set LibSSH_FOUND to TRUE if 
  # all listed variables are TRUE
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LibSSH 
                                FOUND_VAR LibSSH_FOUND
                                REQUIRED_VARS LibSSH_LIBRARY LibSSH_INCLUDE_DIR
                                VERSION_VAR LibSSH_VERSION_STRING)
else(LibSSH_VERSION_LINE_MAJOR)
  message(ERROR " Unable to detect libssh version")
  set(LibSSH_FOUND FALSE) 
endif(LibSSH_VERSION_LINE_MAJOR)

IF(LibSSH_FOUND)
  SET( LibSSH_LIBRARIES ${LibSSH_LIBRARY} )
  SET( LibSSH_INCLUDE_DIRS ${LibSSH_INCLUDE_DIR} )
ELSE(LibSSH_FOUND)
  SET( LibSSH_LIBRARIES )
  SET( LibSSH_INCLUDE_DIRS )
ENDIF(LibSSH_FOUND)

MARK_AS_ADVANCED( LibSSH_LIBRARIES LibSSH_INCLUDE_DIRS )
