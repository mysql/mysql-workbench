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

set(aal_HEADER_FILE
    ${PROJECT_SOURCE_DIR}/src/aal/aalcommon.h
    ${PROJECT_SOURCE_DIR}/src/aal/geometry.h
    ${PROJECT_SOURCE_DIR}/src/aal/role.h
    )

set(aal_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/src/aal/aalcommon.cpp
    ${PROJECT_SOURCE_DIR}/src/aal/geometry.cpp
    ${PROJECT_SOURCE_DIR}/src/aal/role.cpp
    )



if (UNIX AND NOT APPLE)
  list(APPEND aal_HEADER_FILES
    linux/accessible.h
    linux/virtualkeygenerator.h
    )

  list(APPEND aal_SOURCE_FILES
    linux/accessible.cpp
    linux/virtualkeygenerator.cpp
    )
elseif(APPLE)
  list(APPEND aal_HEADER_FILES
    macos/accessible.h
    )

  list(APPEND aal_SOURCE_FILES
    macos/accessible.cpp
    )
   set_source_files_properties(${PROJECT_SOURCE_DIR}/src/aal/macos/accessible.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc")
   set_source_files_properties(${PROJECT_SOURCE_DIR}/src/aal/macos/accessible.h PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc")

   set_source_files_properties(${PROJECT_SOURCE_DIR}/src/aal/aalcommon.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc")
endif()

set(aal_SOURCES ${aal_SOURCE_FILES} ${aal_HEADER_FILES})
add_library(aal STATIC ${aal_SOURCES})


if (UNIX AND NOT APPLE)
  target_include_directories(aal
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/linux 
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src
      ${PROJECT_SOURCE_DIR}/src/library
      ${PROJECT_SOURCE_DIR}/src/duktape
    )

    target_include_directories(aal
      SYSTEM 
        PUBLIC
          ${ATSPI_INCLUDE_DIRS}
          ${GLIBMM_INCLUDE_DIRS}
          ${CAIROMM_INCLUDE_DIRS}
          ${GTKMM_INCLUDE_DIRS}
          ${MySQLCppConn_INCLUDE_DIRS}
    )

elseif(WIN32)
  target_include_directories(aal
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE 
      windows)

elseif(APPLE)
  target_include_directories(aal
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/macos
    PRIVATE 
        macos
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/library
        ${PROJECT_SOURCE_DIR}/src/duktape
  )
  
  target_include_directories(aal
    SYSTEM 
      PUBLIC
        ${MySQLCppConn_INCLUDE_DIRS}
  )
  
endif()


target_compile_options(aal PRIVATE ${MGA_CXXFLAGS})

if (UNIX AND NOT APPLE)
  target_link_libraries(aal PRIVATE ${ATSPI_LIBRARIES} ${GTKMM_LIBRARIES} ${CAIROMM_LIBRARIES} X11 Xtst)
elseif(APPLE)
  target_link_libraries(aal PRIVATE ${Cocoa_LIBRARIES})
endif()


set_target_properties(aal
                      PROPERTIES VERSION   ${MGA_VERSION}
                                 SOVERSION ${MGA_VERSION}
                                 OUTPUT_NAME aal
                                 ARCHIVE_OUTPUT_DIRECTORY ${INSTALL_LIBDIR})
