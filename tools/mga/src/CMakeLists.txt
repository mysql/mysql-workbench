# Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

set(mga_HEADER_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/context-management.h
  ${CMAKE_CURRENT_SOURCE_DIR}/application-context.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common.h
  ${CMAKE_CURRENT_SOURCE_DIR}/duktape/duk_config.h
  ${CMAKE_CURRENT_SOURCE_DIR}/duktape/extra/module-node/duk_module_node.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/apath.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/db.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/child_process.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/debugger.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/events.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/filesystem.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/global.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/glob.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/jsexport.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/os.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/path.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/platform.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/process.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/dbresult.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/streams.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/types.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/uielement.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/utilities.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/utils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/scripting-context.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/tty.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/internal-tests.h
  ${CMAKE_CURRENT_SOURCE_DIR}/library/property.h
)
set(mga_SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/context-management.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/application-context.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/apath.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/db.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/child_process.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/debugger.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/events.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/filesystem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/global.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/glob.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/jsexport.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/os.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/path.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/process.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/dbresult.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/streams.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/types.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/uielement.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/utilities.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/scripting-context.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/tty.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/internal-tests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library/property.cpp
)

if (UNIX AND NOT APPLE)
  list(APPEND mga_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/library/platform-linux.cpp
    )
elseif(APPLE)
  list(APPEND mga_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/library/platform-mac.mm
    )
endif()

set(mga_SOURCES ${mga_SOURCE_FILES} ${mga_HEADER_FILES})
add_executable(mga ${mga_SOURCES})

target_include_directories(mga 
 PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/library
)

if (UNIX AND NOT APPLE)
  target_link_libraries(mga 
    PRIVATE 
      utf8proc 
      duktape 
      aal  
      ${CMAKE_THREAD_LIBS_INIT} 
      ${MySQLCppConn_LIBRARIES} 
      ${OPENSSL_LIBRARIES} 
      ${X11_Xinerama_LIB}
  )
elseif (APPLE)
  foreach(mga_file ${mga_SOURCE_FILES})
    set_source_files_properties(${mga_file} PROPERTIES COMPILE_FLAGS "-x objective-c++")
  endforeach()
  
  target_link_libraries(mga 
    PRIVATE 
      utf8proc 
      duktape 
      aal 
      ${CMAKE_THREAD_LIBS_INIT} 
      ${MySQLCppConn_LIBRARIES})
endif()

target_compile_options(mga PUBLIC ${MGA_CXXFLAGS})
if(BUILD_WITH_COVERAGE AND UNIX)
  target_link_libraries(mga PUBLIC --coverage)
endif()

install(TARGETS mga RUNTIME COMPONENT main DESTINATION ${INSTALL_BINDIR})
