add_library(wbssh 
    SSHSftp.cpp
    SSHCommon.cpp
    SSHSession.cpp
    SSHTunnelHandler.cpp
    SSHTunnelManager.cpp
)

target_include_directories(wbssh
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_compile_options(wbssh PRIVATE ${WB_CXXFLAGS})

target_link_libraries(wbssh 
  PUBLIC 
    ssh
  PRIVATE 
    wbbase
)

add_library(wbssh_Iface INTERFACE)
add_library(wbssh::wbssh ALIAS wbssh_Iface)
target_include_directories(wbssh_Iface INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

if(BUILD_FOR_GCOV)
  target_link_libraries(wbssh PRIVATE gcov)
endif()

if(COMMAND cotire)
set_target_properties(wbssh PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")
cotire(wbssh)
endif()

set_target_properties(wbssh
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

install(TARGETS wbssh DESTINATION ${WB_INSTALL_LIB_DIR})

