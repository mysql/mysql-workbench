find_package(MySQLCppConn REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/backend
    ${PROJECT_SOURCE_DIR}/generated 
    ${PROJECT_SOURCE_DIR}/library/grt/src
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/ext
    ${GRT_INCLUDE_DIRS}
    ${MYSQLCPPCONN_INCLUDE_DIRS}
)

check_include_files ("cppconn/version_info.h" MYSQL_CONNECTOR_VERSION_FILE)

if (MYSQL_CONNECTOR_VERSION_FILE EQUAL "1")
  add_definitions(-DMYSQLCPPCONN_VERSION_AT_LEAST_1_1_5_DETECTED)
endif (MYSQL_CONNECTOR_VERSION_FILE EQUAL "1")

#TODO: Set other compiler flags
#if(CMAKE_COMPILER_IS_GNUCXX)
#    set(CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas)
#endif ()

add_library(cdbc
    src/driver_manager.cpp
    src/sql_batch_exec.cpp
)

target_link_libraries(cdbc ${MYSQLCPPCONN_LIBRARY})

set_target_properties(cdbc
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

install(TARGETS cdbc DESTINATION ${WB_INSTALL_LIB_DIR})
