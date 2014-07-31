# Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved.
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

# - Find Connector/C++
# Find the Connector/C++  includes and library
#
#  MYSQLCPPCONN_INCLUDE_DIRS - where to find mysql_connection.h, etc.
#  MYSQLCPPCONN_LIBRARIES    - List of libraries when using connector/c++.
#  MYSQLCPPCONN_FOUND        - True if connector found.


IF (MYSQLCPPCONN_INCLUDE_DIRS)
  # Already in cache, be silent
  SET(MYSQLCPPCONN_FIND_QUIETLY TRUE)
ENDIF (MYSQLCPPCONN_INCLUDE_DIRS)

FIND_PATH(MYSQLCPPCONN_INCLUDE_DIR mysql_connection.h
	PATHS ${CMAKE_SYSTEM_INCLUDE_PATH}
	      /usr/include
          /usr/local/include
)


SET(MYSQLCPPCONN_NAMES mysqlcppconn)
IF(MYSQLCPPCONN_LIBRARY)
ELSE()
  FIND_LIBRARY(MYSQLCPPCONN_LIBRARY NAMES ${MYSQLCPPCONN_NAMES} )
ENDIF(MYSQLCPPCONN_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set MYSQLCPPCONN_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MYSQLCPPCONN DEFAULT_MSG MYSQLCPPCONN_LIBRARY MYSQLCPPCONN_INCLUDE_DIR)

IF(MYSQLCPPCONN_FOUND)
  SET( MYSQLCPPCONN_LIBRARIES ${MYSQLCPPCONN_LIBRARY} )
  SET( MYSQLCPPCONN_INCLUDE_DIRS ${MYSQLCPPCONN_INCLUDE_DIR} )
  TRY_COMPILE(MYSQLCPPCONN_VERSION_1_1_4
        ${CMAKE_BINARY_DIR}/try_compile ${CMAKE_CURRENT_LIST_DIR}/getColumnCharset.cpp 
        COMPILE_DEFINITIONS ${MYSQLCPPCONN_INCLUDE_DIRS})
  MESSAGE(STATUS "C/C++ version 1.1.4+: ${MYSQLCPPCONN_VERSION_1_1_4}")
ELSE(MYSQLCPPCONN_FOUND)
  SET( MYSQLCPPCONN_LIBRARIES )
  SET( MYSQLCPPCONN_INCLUDE_DIRS )
ENDIF(MYSQLCPPCONN_FOUND)

MARK_AS_ADVANCED( MYSQLCPPCONN_LIBRARIES MYSQLCPPCONN_INCLUDE_DIRS )
