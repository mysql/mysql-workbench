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

# - Find mysql client library and includes
#
#  MYSQL_FOUND            - True if mysql was found
#  MYSQL_INCLUDE_DIRS     - the include dir where mysql.h lives
#  MYSQL_LIBRARIES        - list of mysql libraries

if (MYSQL_LIBRARIES AND MYSQL_INCLUDE_DIRS)
  # in cache already, be silent and skip the rest
  set(MYSQL_FOUND TRUE)
else ()
   if (MYSQL_CONFIG_PATH)
    EXEC_PROGRAM(${MYSQL_CONFIG_PATH} ARGS --variable=pkgincludedir RETURN_VALUE _return_VALUE OUTPUT_VARIABLE MYSQL_INCLUDE_DIRS)
    if (NOT _return_VALUE)
      EXEC_PROGRAM(${MYSQL_CONFIG_PATH} ARGS --libs RETURN_VALUE _return_VALUE OUTPUT_VARIABLE MYSQL_LIBRARIES)

      include(FindPackageHandleStandardArgs)
      find_package_handle_standard_args(MySQL  DEFAULT_MSG
                                    MYSQL_INCLUDE_DIRS MYSQL_LIBRARIES
				   )
      mark_as_advanced(MYSQL_INCLUDE_DIRS MYSQL_LIBRARIES)
    else ()
      MESSAGE(FATAL_ERROR "Cannot find ${MYSQL_CONFIG_PATH} (MYSQL_CONFIG_PATH)") 
    endif ()
  else ()
    # Find the include dir:
    FIND_PATH(MYSQL_INCLUDE_DIRS mysql.h
		/usr/include/mysql
		/usr/local/include/mysql
		/opt/mysql/mysql/include
		/opt/mysql/mysql/include/mysql
		/usr/local/mysql/include
		/usr/local/mysql/include/mysql
		$ENV{ProgramFiles}/MySQL/*/include
		$ENV{SystemDrive}/MySQL/*/include
	   )

    # Find the library:
    SET(MYSQL_LIBRARY_NAMES mysqlclient mysqlclient_r)
    FIND_LIBRARY(MYSQL_LIBRARIES
               NAMES ${MYSQL_LIBRARY_NAMES}
               PATHS /usr/lib
	             /usr/local/lib
		     /usr/local/mysql/lib
		     /opt/mysql/mysql/lib
		     $ENV{ProgramFiles}/MySQL/*/lib
               PATH_SUFFIXES mysql
              )
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(MySQL  DEFAULT_MSG
                                    MYSQL_INCLUDE_DIRS MYSQL_LIBRARIES
				   )
    mark_as_advanced(MYSQL_INCLUDE_DIRS MYSQL_LIBRARIES)
  endif ()
endif ()
