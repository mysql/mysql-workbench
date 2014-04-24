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

install(TARGETS cdbc DESTINATION ${WB_INSTALL_LIB_DIR})
