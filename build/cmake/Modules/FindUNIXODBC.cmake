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

find_path(UNIXODBC_INCLUDE_DIR unixodbc_conf.h)

if (UNIXODBC_INCLUDE_DIR)
  find_library(UNIXODBC_LIBRARY odbc)

  set(UNIXODBC_INCLUDE_DIRS ${UNIXODBC_INCLUDE_DIR} )
  set(UNIXODBC_LIBRARIES ${UNIXODBC_LIBRARY} )

  include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set UNIXODBC_FOUND to TRUE
# if all listed variables are TRUE
  find_package_handle_standard_args(unixODBC  DEFAULT_MSG
                                    UNIXODBC_LIBRARY UNIXODBC_INCLUDE_DIR)

  mark_as_advanced(UNIXODBC_INCLUDE_DIR UNIXODBC_LIBRARY)
endif (UNIXODBC_INCLUDE_DIR)
