# Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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

# - Find mysql client library and includes
#
#  MySQL_FOUND            - True if mysql was found
#  MySQL_INCLUDE_DIRS     - the include dir where mysql.h lives
#  MySQL_LIBRARIES        - list of mysql libraries


if (MySQLCppConn_INCLUDE_DIRS AND MySQLCppConn_LIBRARIES)
  # Already in cache, be silent
  set(MySQLCppConn_FIND_QUIETLY TRUE)
endif ()

find_path(MySQLCppConn_INCLUDE_DIR NAMES mysql_connection.h PATHS /usr/include /usr/local/include)

SET(MySQLCppConn_NAMES mysqlcppconn)

if(MySQLCppConn_LIBRARIES)
  # Converto to a list of library argments
  string(REPLACE " " ";" MySQLCppConn_LIB_ARGS ${MySQLCppConn_LIBRARIES})

  # Parse the list in order to find the library path
  foreach(MySQLCppConn_LIB_ARG ${MySQLCppConn_LIB_ARGS})
    string(REPLACE "-L" "" MySQLCppConn_LIB_ARG_CLEAR ${MySQLCppConn_LIB_ARG})
    if(NOT ${MySQLCppConn_LIB_ARG_CLEAR} STREQUAL ${MySQLCppConn_LIB_ARG})
      set(MySQLCppConn_SUPPLIED_LIB_DIR ${MySQLCppConn_LIB_ARG_CLEAR})
    endif()
  endforeach(MySQLCppConn_LIB_ARG)
  find_library(MySQLCppConn_LIBRARY NAMES ${MySQLCppConn_NAMES} HINTS ${MySQLCppConn_SUPPLIED_LIB_DIR})
  
  unset(MySQLCppConn_LIB_ARG_CLEAR)
  unset(MySQLCppConn_LIB_ARG)
  unset(MySQLCppConn_LIB_ARGS)
else()
  find_library(MySQLCppConn_LIBRARY NAMES ${MySQLCppConn_NAMES})
endif()

set(MySQLCppConn_HEADER_FILE ${MySQLCppConn_INCLUDE_DIR}/cppconn/version_info.h)
file(STRINGS ${MySQLCppConn_HEADER_FILE} MySQLCppConn_VERSION_LINE_MAJOR REGEX "#define MYCPPCONN_DM_MAJOR_VERSION[ ]+[0-9]+")
if (MySQLCppConn_VERSION_LINE_MAJOR)
  file(STRINGS ${MySQLCppConn_HEADER_FILE} MySQLCppConn_VERSION_LINE_MINOR REGEX "#define MYCPPCONN_DM_MINOR_VERSION[ ]+[0-9]+")
  file(STRINGS ${MySQLCppConn_HEADER_FILE} MySQLCppConn_VERSION_LINE_MICRO REGEX "#define MYCPPCONN_DM_PATCH_VERSION[ ]+[0-9]+")
  string(REGEX MATCH "([0-9]+)" MySQLCppConn_VERSION_MAJOR "${MySQLCppConn_VERSION_LINE_MAJOR}")
  string(REGEX MATCH "([0-9]+)" MySQLCppConn_VERSION_MINOR "${MySQLCppConn_VERSION_LINE_MINOR}")
  string(REGEX MATCH "([0-9]+)" MySQLCppConn_VERSION_MICRO "${MySQLCppConn_VERSION_LINE_MICRO}")
  set(MySQLCppConn_VERSION_STRING ${MySQLCppConn_VERSION_MAJOR}.${MySQLCppConn_VERSION_MINOR}.${MySQLCppConn_VERSION_MICRO})
  # handle the QUIETLY and REQUIRED arguments and set MySQLCppConn_FOUND to TRUE if 
  # all listed variables are TRUE
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(MySQLCppConn 
                                FOUND_VAR MySQLCppConn_FOUND
                                REQUIRED_VARS MySQLCppConn_LIBRARY MySQLCppConn_INCLUDE_DIR
                                VERSION_VAR MySQLCppConn_VERSION_STRING)
else(MySQLCppConn_VERSION_LINE_MAJOR)
  message(ERROR " Unable to detect MySQLCppConn version")
  set(MySQLCppConn_FOUND FALSE) 
endif(MySQLCppConn_VERSION_LINE_MAJOR)

IF(MySQLCppConn_FOUND)
  SET( MySQLCppConn_LIBRARIES ${MySQLCppConn_LIBRARY} )
  SET( MySQLCppConn_INCLUDE_DIRS ${MySQLCppConn_INCLUDE_DIR} )
ELSE(MySQLCppConn_FOUND)
  SET( MySQLCppConn_LIBRARIES )
  SET( MySQLCppConn_INCLUDE_DIRS )
ENDIF(MySQLCppConn_FOUND)

MARK_AS_ADVANCED( MySQLCppConn_LIBRARIES MySQLCppConn_INCLUDE_DIRS )
