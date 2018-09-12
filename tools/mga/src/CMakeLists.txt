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

add_subdirectory(utf8proc)
add_subdirectory(duktape)
add_subdirectory(aal)

include_directories(
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/src/aal
  ${PROJECT_SOURCE_DIR}/src/library
  ${PROJECT_SOURCE_DIR}/src/duktape
  ${PROJECT_SOURCE_DIR}/src/duktape/extra/module-node
  ${PROJECT_SOURCE_DIR}/src/utf8proc
  SYSTEM ${MySQLCppConn_INCLUDE_DIRS}
)

if (UNIX AND NOT APPLE)
  include_directories(
  SYSTEM ${ATSPI_INCLUDE_DIRS}
  SYSTEM ${GLIBMM_INCLUDE_DIRS}
  SYSTEM ${GIOMM_INCLUDE_DIRS}
  SYSTEM ${CAIROMM_INCLUDE_DIRS}
  SYSTEM ${GTKMM_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/src/aal/linux)

elseif (APPLE) 
  include_directories(
  ${PROJECT_SOURCE_DIR}/src/aal/macos)
endif()

include_directories(${mga_INCLUDE_DIRS})

set(mga_HEADER_FILES
  ${PROJECT_SOURCE_DIR}/src/context-management.h
  ${PROJECT_SOURCE_DIR}/src/application-context.h
  ${PROJECT_SOURCE_DIR}/src/common.h
  ${PROJECT_SOURCE_DIR}/src/duktape/duk_config.h
  ${PROJECT_SOURCE_DIR}/src/duktape/extra/module-node/duk_module_node.h
  ${PROJECT_SOURCE_DIR}/src/library/apath.h
  ${PROJECT_SOURCE_DIR}/src/library/db.h
  ${PROJECT_SOURCE_DIR}/src/library/child_process.h
  ${PROJECT_SOURCE_DIR}/src/library/debugger.h
  ${PROJECT_SOURCE_DIR}/src/library/events.h
  ${PROJECT_SOURCE_DIR}/src/library/filesystem.h
  ${PROJECT_SOURCE_DIR}/src/library/global.h
  ${PROJECT_SOURCE_DIR}/src/library/glob.h
  ${PROJECT_SOURCE_DIR}/src/library/jsexport.h
  ${PROJECT_SOURCE_DIR}/src/library/os.h
  ${PROJECT_SOURCE_DIR}/src/library/path.h
  ${PROJECT_SOURCE_DIR}/src/library/platform.h
  ${PROJECT_SOURCE_DIR}/src/library/process.h
  ${PROJECT_SOURCE_DIR}/src/library/dbresult.h
  ${PROJECT_SOURCE_DIR}/src/library/streams.h
  ${PROJECT_SOURCE_DIR}/src/library/types.h
  ${PROJECT_SOURCE_DIR}/src/library/uielement.h
  ${PROJECT_SOURCE_DIR}/src/library/utilities.h
  ${PROJECT_SOURCE_DIR}/src/library/scripting-context.h
  ${PROJECT_SOURCE_DIR}/src/library/tty.h
  ${PROJECT_SOURCE_DIR}/src/library/internal-tests.h
)
set(mga_SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/src/main.cpp
  ${PROJECT_SOURCE_DIR}/src/context-management.cpp
  ${PROJECT_SOURCE_DIR}/src/application-context.cpp
  ${PROJECT_SOURCE_DIR}/src/library/apath.cpp
  ${PROJECT_SOURCE_DIR}/src/library/db.cpp
  ${PROJECT_SOURCE_DIR}/src/library/child_process.cpp
  ${PROJECT_SOURCE_DIR}/src/library/debugger.cpp
  ${PROJECT_SOURCE_DIR}/src/library/events.cpp
  ${PROJECT_SOURCE_DIR}/src/library/filesystem.cpp
  ${PROJECT_SOURCE_DIR}/src/library/global.cpp
  ${PROJECT_SOURCE_DIR}/src/library/glob.cpp
  ${PROJECT_SOURCE_DIR}/src/library/jsexport.cpp
  ${PROJECT_SOURCE_DIR}/src/library/os.cpp
  ${PROJECT_SOURCE_DIR}/src/library/path.cpp
  ${PROJECT_SOURCE_DIR}/src/library/platform.cpp
  ${PROJECT_SOURCE_DIR}/src/library/process.cpp
  ${PROJECT_SOURCE_DIR}/src/library/dbresult.cpp
  ${PROJECT_SOURCE_DIR}/src/library/streams.cpp
  ${PROJECT_SOURCE_DIR}/src/library/types.cpp
  ${PROJECT_SOURCE_DIR}/src/library/uielement.cpp
  ${PROJECT_SOURCE_DIR}/src/library/utilities.cpp
  ${PROJECT_SOURCE_DIR}/src/library/scripting-context.cpp
  ${PROJECT_SOURCE_DIR}/src/library/tty.cpp
  ${PROJECT_SOURCE_DIR}/src/library/internal-tests.cpp
)

if (UNIX AND NOT APPLE)
  list(APPEND mga_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/src/library/platform-linux.cpp
    )
elseif(APPLE)
  list(APPEND mga_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/src/library/platform-mac.mm
    )
endif()

set(mga_SOURCES ${mga_SOURCE_FILES} ${mga_HEADER_FILES})
add_executable(mga ${mga_SOURCES})

if (UNIX AND NOT APPLE)
  target_link_libraries(mga utf8proc duktape aal ${GLIBMM_LIBRARIES} ${GIOMM_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${MySQLCppConn_LIBRARIES} ${OPENSSL_LIBRARIES} ${X11_Xinerama_LIB})
elseif (APPLE)
  foreach(mga_file ${mga_SOURCE_FILES})
    set_source_files_properties(${mga_file} PROPERTIES COMPILE_FLAGS "-x objective-c++")
  endforeach()
  target_link_libraries(mga utf8proc duktape aal ${CMAKE_THREAD_LIBS_INIT} ${MySQLCppConn_LIBRARIES})
endif()

target_compile_options(mga PUBLIC ${MGA_CXXFLAGS})
if(BUILD_WITH_COVERAGE AND UNIX)
  target_link_libraries(mga --coverage)
endif()

install(TARGETS mga RUNTIME COMPONENT main DESTINATION ${INSTALL_BINDIR})
