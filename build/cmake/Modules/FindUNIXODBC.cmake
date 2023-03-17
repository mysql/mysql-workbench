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

if (UNIXODBC_INCLUDE_PATH)
    find_path(UNIXODBC_INCLUDE_DIRS unixodbc.h
      PATHS ${UNIXODBC_INCLUDE_PATH} 
            ${CMAKE_SYSTEM_INCLUDE_PATH}
            /usr/include
            /usr/local/include
            /usr/include/x86_64-linux-gnu
    )
else()
    find_path(UNIXODBC_INCLUDE_DIRS unixodbc.h
      PATHS ${CMAKE_SYSTEM_INCLUDE_PATH}
            /usr/include
            /usr/local/include
            /usr/include/x86_64-linux-gnu
    )
endif(UNIXODBC_INCLUDE_PATH)

if (UNIXODBC_INCLUDE_DIRS AND UNIXODBC_LIBRARIES)
  set(UNIXODBC_FOUND true)
endif(UNIXODBC_INCLUDE_DIRS AND UNIXODBC_LIBRARIES)

if (UNIXODBC_CONFIG_PATH)

  if (UNIXODBC_LIBRARIES_PATH)
    # Converto to a list of library argments
    string(REPLACE " " ";" UNIXODBC_LIB_ARGS ${UNIXODBC_LIBRARIES_PATH})
    # Parse the list in order to find the library path
    foreach(UNIXODBC_LIB_ARG ${UNIXODBC_LIB_ARGS})
      string(REPLACE "-L" "" UNIXODBC_LIB_ARG_CLEAR ${UNIXODBC_LIB_ARG})
      if(NOT ${UNIXODBC_LIB_ARG_CLEAR} STREQUAL ${UNIXODBC_LIB_ARG})
        set(UNIXODBC_SUPPLIED_LIB_DIR ${UNIXODBC_LIB_ARG_CLEAR})
      else()
        set(UNIXODBC_SUPPLIED_LIB_DIR ${UNIXODBC_LIB_ARG})
      endif()
    endforeach(UNIXODBC_LIB_ARG)
  
    find_library(lib_odbc NAMES libodbc.so HINTS ${UNIXODBC_SUPPLIED_LIB_DIR})
    find_library(lib_odbcinst NAMES libodbcinst.so HINTS ${UNIXODBC_SUPPLIED_LIB_DIR})
    
    unset(UNIXODBC_LIB_ARG_CLEAR)
    unset(UNIXODBC_LIB_ARG)
    unset(UNIXODBC_LIB_ARGS)
    
    set(UNIXODBC_LIBRARIES ${lib_odbc} ${lib_odbcinst})

  else()
    exec_program(${UNIXODBC_CONFIG_PATH} ARGS --libs
                    OUTPUT_VARIABLE UNIXODBC_LIBRARIES)
  endif(UNIXODBC_LIBRARIES_PATH)

  find_program(UNIXODBC_CONFIG_PATH odbc-config)


  if (NOT EXISTS ${UNIXODBC_CONFIG_PATH})
    MESSAGE(FATAL_ERROR "odbc-config not found in " ${UNIXODBC_CONFIG_PATH})
  endif()

  exec_program(${UNIXODBC_CONFIG_PATH} ARGS --cflags
                  OUTPUT_VARIABLE UNIXODBC_DEFINITIONS)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(UNIXODBC DEFAULT_MSG
    UNIXODBC_CONFIG_PATH UNIXODBC_LIBRARIES UNIXODBC_DEFINITIONS
           )

  mark_as_advanced(
    UNIXODBC_CONFIG_PATH
    UNIXODBC_LIBRARIES
    UNIXODBC_DEFINITIONS
    UNIXODBC_INCLUDE_DIRS
    )
endif (UNIXODBC_CONFIG_PATH)