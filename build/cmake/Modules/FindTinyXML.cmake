# Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved.
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

# - Find tinyxml
# Find the native TinyXML includes and library
#
#  TINYXML_INCLUDE_DIRS - where to find tinyxml.h, etc.
#  TINYXML_LIBRARIES    - List of libraries to link to.
#  TINYXML_FOUND        - True if tinyxml found.


IF (TINYXML_INCLUDE_DIRS)
  # Already in cache, be silent
  SET(TINYXML_FIND_QUIETLY TRUE)
ENDIF (TINYXML_INCLUDE_DIRS)

FIND_PATH(TINYXML_INCLUDE_DIR tinyxml.h)

SET(TINYXML_NAMES tinyxml)
FIND_LIBRARY(TINYXML_LIBRARY NAMES ${TINYXML_NAMES} )

# handle the QUIETLY and REQUIRED arguments and set TINYXML_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(TINYXML DEFAULT_MSG TINYXML_LIBRARY TINYXML_INCLUDE_DIR)

IF(TINYXML_FOUND)
  SET( TINYXML_LIBRARIES ${TINYXML_LIBRARY} )
  SET( TINYXML_INCLUDE_DIRS ${TINYXML_INCLUDE_DIR} )
ELSE(TINYXML_FOUND)
  SET( TINYXML_LIBRARIES )
  SET( TINYXML_INCLUDE_DIRS )
ENDIF(TINYXML_FOUND)

MARK_AS_ADVANCED( TINYXML_LIBRARIES TINYXML_INCLUDE_DIRS )
