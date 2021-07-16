add_library(cdbc
    src/driver_manager.cpp
    src/sql_batch_exec.cpp
)

target_include_directories(cdbc
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/generated>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/library/forms>
)

target_include_directories(cdbc
 SYSTEM
  PUBLIC
    $<BUILD_INTERFACE:${MySQLCppConn_INCLUDE_DIRS}>

)

target_compile_options(cdbc PRIVATE ${WB_CXXFLAGS})

target_link_libraries(cdbc 
  PUBLIC 
    ${MySQLCppConn_LIBRARIES}
    wbprivate::wbprivate
    wbssh::wbssh
  PRIVATE
    Boost::boost
    grt::grt
    wbbase::wbbase
)

if(BUILD_FOR_GCOV)
  target_link_libraries(cdbc PRIVATE gcov)
endif()

set_target_properties(cdbc
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

if(COMMAND cotire)
    set_target_properties(cdbc PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

    cotire(cdbc)
endif()
install(TARGETS cdbc DESTINATION ${WB_INSTALL_LIB_DIR})
