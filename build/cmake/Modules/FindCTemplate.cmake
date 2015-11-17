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
# Find the native CTemplate includes and library
#
#  CTemplate_INCLUDE_DIRS - where to find template_annotator.h, etc.
#  CTemplate_LIBRARIES    - List of libraries when using ctemplate.
#  CTemplate_FOUND        - True if ctemplate found.


if(CTemplate_INCLUDE_DIRS)
  # Already in cache, be silent
  set(CTemplate_FIND_QUIETLY TRUE)
endif(CTemplate_INCLUDE_DIRS)

find_path(CTemplate_INCLUDE_DIR template_annotator.h
          PATHS ${CMAKE_SYSTEM_INCLUDE_PATH}/ctemplate
                /usr/include/ctemplate
                /usr/local/include/ctemplate
)


set(CTemplate_NAMES ctemplate)


if(CTemplate_LIBRARIES)
  # Converto to a list of library argments
  string(REPLACE " " ";" CTemplate_LIB_ARGS ${CTemplate_LIBRARIES})

  # Parse the list in order to find the library path
  foreach(CTemplate_LIB_ARG ${CTemplate_LIB_ARGS})
    string(REPLACE "-L" "" CTemplate_LIB_ARG_CLEAR ${CTemplate_LIB_ARG})
    if(NOT ${CTemplate_LIB_ARG_CLEAR} STREQUAL ${CTemplate_LIB_ARG})
      set(CTemplate_SUPPLIED_LIB_DIR ${CTemplate_LIB_ARG_CLEAR})
    endif()
  endforeach(CTemplate_LIB_ARG)
  find_library(CTemplate_LIBRARY NAMES ${CTemplate_NAMES} HINTS ${CTemplate_SUPPLIED_LIB_DIR})
  
  unset(CTemplate_LIB_ARG_CLEAR)
  unset(CTemplate_LIB_ARG)
  unset(CTemplate_LIB_ARGS)
else()
  find_library(CTemplate_LIBRARY NAMES ${CTemplate_NAMES})
endif()

get_filename_component(CTemplate_LIB_FILENAME ${CTemplate_LIBRARY} NAME_WE)
get_filename_component(CTemplate_LIB_DIRECTORY ${CTemplate_LIBRARY} DIRECTORY)
get_filename_component(CTemplate_LIB_BASE_DIRECTORY ${CTemplate_LIB_DIRECTORY} DIRECTORY)

set(CTemplate_BIN_DIR "${CTemplate_LIB_BASE_DIRECTORY}/bin")

# Allow to call find_program twice in order to find wothout the default paths and then with them.
# If it finds on the first call, it will use the cached one.
find_program(CTemplate_VARNAMES NAMES make_tpl_varnames_h ctemplate-make_tpl_varnames_h
                                PATHS ${CTemplate_BIN_DIR}
                                NO_DEFAULT_PATH
            )
find_program(CTemplate_VARNAMES NAMES make_tpl_varnames_h ctemplate-make_tpl_varnames_h)

# Get CTemplate version
execute_process(COMMAND ${CTemplate_VARNAMES} --version
                COMMAND grep ctemplate
                COMMAND awk "{print $5}"
                COMMAND awk -F ")" "{print $1}"
                RESULT_VARIABLE CTemplate_RUN_RESULT
                OUTPUT_VARIABLE CTemplate_RUN_OUTPUT
                ERROR_VARIABLE  CTemplate_RUN_ERROR)

string(STRIP ${CTemplate_RUN_OUTPUT} CTemplate_VERSION_STRING)

# handle the QUIETLY and REQUIRED arguments and set CTemplate_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CTemplate 
                                  FOUND_VAR CTemplate_FOUND
                                  REQUIRED_VARS CTemplate_LIBRARY CTemplate_INCLUDE_DIR
                                  VERSION_VAR CTemplate_VERSION_STRING)

if(CTemplate_FOUND)
  set(CTemplate_LIBRARIES "-L${CTemplate_LIB_DIRECTORY} -l${CTemplate_NAMES}")
  set(CTemplate_INCLUDE_DIRS ${CTemplate_INCLUDE_DIR})
else(CTemplate_FOUND)
  unset(CTemplate_LIBRARIES)
  unset(CTemplate_INCLUDE_DIRS)
endif(CTemplate_FOUND)


mark_as_advanced(CTemplate_LIBRARIES CTemplate_INCLUDE_DIRS CTemplate_LIB_FILENAME CTemplate_LIB_DIRECTORY CTemplate_LIB_BASE_DIRECTORY)
