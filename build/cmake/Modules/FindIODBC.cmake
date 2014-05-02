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

find_program(IODBC_PATH iodbc-config)

if (IODBC_PATH)
  execute_process(COMMAND iodbc-config --cflags
                  OUTPUT_VARIABLE IODBC_DEFINITIONS
                 )
  find_library(IODBC_LIB iodbc)
  find_library(IODBCINST_LIB iodbcinst)
  if (IODBC_LIB)
    set(IODBC_LIBRARIES "iodbc")
  endif ()
  if (IODBCINST_LIB)
    if (IODBC_LIBRARIES)
      set(IODBC_LIBRARIES "${IODBC_LIBRARIES};iodbcinst")
    else ()
      set(IODBC_LIBRARIES "iodbcinst")
    endif ()
  endif ()
  set(IODBC_INCLUDE_DIRS "")
endif (IODBC_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(IODBC DEFAULT_MSG
	IODBC_PATH IODBC_LIBRARIES
			   )

mark_as_advanced(
  IODBC_LIB
  IODBC_PATH
  IODBCINST_LIB
  )
