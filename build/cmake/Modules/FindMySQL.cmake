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
#  MySQL_FOUND            - True if mysql was found
#  MySQL_INCLUDE_DIRS     - the include dir where mysql.h lives
#  MySQL_LIBRARIES        - list of mysql libraries

include(FindPackageHandleStandardArgs)

if (MySQL_LIBRARIES AND MySQL_INCLUDE_DIRS)
    # in cache already, be silent and skip the rest
    set(MySQL_FOUND TRUE)
else ()
    if (EXISTS ${MySQL_CONFIG_PATH})

        execute_process(COMMAND ${MySQL_CONFIG_PATH} --version 
                        RESULT_VARIABLE MySQL_ProcessResult
                        OUTPUT_VARIABLE MySQL_VERSION
                        ERROR_VARIABLE MySQL_ProcessError
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                        ERROR_STRIP_TRAILING_WHITESPACE)

        execute_process(COMMAND ${MySQL_CONFIG_PATH} --variable=pkgincludedir 
                        RESULT_VARIABLE MySQL_ProcessResult
                        OUTPUT_VARIABLE MySQL_INCLUDE_DIRS
                        ERROR_VARIABLE MySQL_ProcessError
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                        ERROR_STRIP_TRAILING_WHITESPACE)
                        
        execute_process(COMMAND ${MySQL_CONFIG_PATH} --libs 
                        RESULT_VARIABLE MySQL_ProcessResult
                        OUTPUT_VARIABLE MySQL_LIBRARIES
                        ERROR_VARIABLE MySQL_ProcessError
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                        ERROR_STRIP_TRAILING_WHITESPACE)

        find_package_handle_standard_args(MySQL
                                          FOUND_VAR     MySQL_FOUND
                                          REQUIRED_VARS MySQL_INCLUDE_DIRS  MySQL_LIBRARIES
                                          VERSION_VAR   MySQL_VERSION
                                        )
        message("MySQL_INCLUDE_DIRS: ${MySQL_INCLUDE_DIRS}")
        message("MySQL_LIBRARIES: ${MySQL_LIBRARIES}")
#         mark_as_advanced(MySQL_INCLUDE_DIRS MySQL_LIBRARIES)
    else ()
        # Find the include dir:
        find_path(MySQL_INCLUDE_DIRS  mysql.h
                                      /usr/include/mysql
                                      /usr/local/include/mysql
                                      /opt/mysql/mysql/include
                                      /opt/mysql/mysql/include/mysql
                                      /usr/local/mysql/include
                                      /usr/local/mysql/include/mysql
                                      $ENV{ProgramFiles}/MySQL/*/include
                                      $ENV{SystemDrive}/MySQL/*/include
                 )

        find_file(MySQL_VERSION_FILE mysql_version.h ${MySQL_INCLUDE_DIRS})
        file(STRINGS "${MySQL_VERSION_FILE}" MySQL_VERSION_LINE REGEX "MYSQL_SERVER_VERSION")
        string(REGEX REPLACE "#define MYSQL_SERVER_VERSION[ \t]+\"([0-9.]+)\"" "\\1" MySQL_VERSION "${MySQL_VERSION_LINE}")
        
        # Find the library:
        set(MySQL_LIBRARY_NAMES mysqlclient mysqlclient_r)
        find_library(MySQL_LIBRARIES
                     NAMES ${MySQL_LIBRARY_NAMES}
                     PATHS  /usr/lib
                            /usr/local/lib
                            /usr/local/mysql/lib
                            /opt/mysql/mysql/lib
                            $ENV{ProgramFiles}/MySQL/*/lib
                     PATH_SUFFIXES mysql
                   )

        find_package_handle_standard_args(MySQL
                                          FOUND_VAR     MySQL_FOUND
                                          REQUIRED_VARS MySQL_INCLUDE_DIRS  MySQL_LIBRARIES
                                          VERSION_VAR   MySQL_VERSION
                                          )
#         mark_as_advanced(MySQL_INCLUDE_DIRS MySQL_LIBRARIES)
    endif ()
endif ()
