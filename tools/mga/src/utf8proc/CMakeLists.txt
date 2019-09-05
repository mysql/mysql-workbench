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

set(libutf8proc_HEADER_FILES
  ${PROJECT_SOURCE_DIR}/src/utf8proc/utf8proc.h
  ${PROJECT_SOURCE_DIR}/src/utf8proc/utf8proc_data.h
)

set(libutf8proc_SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/src/utf8proc/utf8proc.c
)

set(libutf8proc_SOURCES ${libutf8proc_SOURCE_FILES} ${libutf8proc_HEADER_FILES})

add_library(utf8proc STATIC ${libutf8proc_SOURCES})

target_include_directories(utf8proc
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(utf8proc
                      PROPERTIES VERSION   ${MGA_VERSION}
                                 SOVERSION ${MGA_VERSION}
                                 OUTPUT_NAME utf8proc
                                 ARCHIVE_OUTPUT_DIRECTORY ${INSTALL_LIBDIR})

install(TARGETS utf8proc
        ARCHIVE DESTINATION lib)
