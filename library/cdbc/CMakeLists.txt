include_directories(${PROJECT_SOURCE_DIR}/backend
    ${PROJECT_SOURCE_DIR}/generated 
    ${PROJECT_SOURCE_DIR}/library/grt/src
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/ext
    ${GRT_INCLUDE_DIRS}
    ${MYSQLCPPCONN_INCLUDE_DIRS}
)

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

if(COMMAND cotire)
    set_target_properties(cdbc PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

    cotire(cdbc)
endif()
install(TARGETS cdbc DESTINATION ${WB_INSTALL_LIB_DIR})
