# Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

set(libduktape_HEADER_FILES
  ${PROJECT_SOURCE_DIR}/src/duktape/duktape.h
  ${PROJECT_SOURCE_DIR}/src/duktape/extra/module-node/duk_module_node.h
)

set(libduktape_SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/src/duktape/duktape.c
  ${PROJECT_SOURCE_DIR}/src/duktape/extra/module-node/duk_module_node.c
)

set(libduktape_SOURCES ${libduktape_SOURCE_FILES} ${libduktape_HEADER_FILES})

add_library(duktape STATIC ${libduktape_SOURCES})

target_include_directories(duktape
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/extra/module-node
)

foreach(duksrc_file ${libduktape_SOURCE_FILES})
 set_source_files_properties(${duksrc_file} PROPERTIES LANGUAGE CXX)
endforeach()

set_target_properties(duktape
                      PROPERTIES VERSION   ${MGA_VERSION}
                                 SOVERSION ${MGA_VERSION}
                                 OUTPUT_NAME duktape
                                 ARCHIVE_OUTPUT_DIRECTORY ${INSTALL_LIBDIR})

install(TARGETS duktape
        ARCHIVE DESTINATION lib)
