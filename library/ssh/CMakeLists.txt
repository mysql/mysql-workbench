include_directories(.
    SYSTEM ${LibSSH_INCLUDE_DIRS}
    SYSTEM ${Boost_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/backend/wbprivate/workbench
)

add_library(wbssh 
    SSHSftp.cpp
    SSHCommon.cpp
    SSHSession.cpp
    SSHTunnelHandler.cpp
    SSHTunnelManager.cpp
)

target_compile_options(wbssh PUBLIC ${WB_CXXFLAGS})

target_link_libraries(wbssh wbbase ${LibSSH_LIBRARIES})

if(BUILD_FOR_TESTS)
  target_link_libraries(wbssh gcov)
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

