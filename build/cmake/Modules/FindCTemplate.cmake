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

# - Find ctemplate
# Find the native CTEMPLATE includes and library
#
#  CTEMPLATE_INCLUDE_DIRS - where to find template_annotator.h, etc.
#  CTEMPLATE_LIBRARIES    - List of libraries when using ctemplate.
#  CTEMPLATE_FOUND        - True if ctemplate found.


IF (CTEMPLATE_INCLUDE_DIRS)
  # Already in cache, be silent
  SET(CTEMPLATE_FIND_QUIETLY TRUE)
ENDIF (CTEMPLATE_INCLUDE_DIRS)

FIND_PATH(CTEMPLATE_INCLUDE_DIR template_annotator.h
	PATHS ${CMAKE_SYSTEM_INCLUDE_PATH}/ctemplate
	      /usr/include/ctemplate
	      /usr/local/include/ctemplate
)


SET(CTEMPLATE_NAMES ctemplate)
FIND_LIBRARY(CTEMPLATE_LIBRARY NAMES ${CTEMPLATE_NAMES} )

# handle the QUIETLY and REQUIRED arguments and set CTEMPLATE_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CTEMPLATE DEFAULT_MSG CTEMPLATE_LIBRARY CTEMPLATE_INCLUDE_DIR)

IF(CTEMPLATE_FOUND)
  SET( CTEMPLATE_LIBRARIES ${CTEMPLATE_LIBRARY} )
  SET( CTEMPLATE_INCLUDE_DIRS ${CTEMPLATE_INCLUDE_DIR} )
ELSE(CTEMPLATE_FOUND)
  SET( CTEMPLATE_LIBRARIES )
  SET( CTEMPLATE_INCLUDE_DIRS )
ENDIF(CTEMPLATE_FOUND)

MARK_AS_ADVANCED( CTEMPLATE_LIBRARIES CTEMPLATE_INCLUDE_DIRS )
