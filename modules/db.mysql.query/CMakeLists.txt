include_directories(.
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${SIGC++_INCLUDE_DIRS}
    SYSTEM ${GRT_INCLUDE_DIRS}
    SYSTEM ${MySQLCppConn_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}/modules/db.mysql/src
    ${PROJECT_SOURCE_DIR}/library/cdbc/src
    SYSTEM ${Boost_INCLUDE_DIRS}
)

add_library(db.mysql.query.grt
    src/dbquery.cpp
)

target_compile_options(db.mysql.query.grt PUBLIC ${WB_CXXFLAGS})

target_link_libraries(db.mysql.query.grt grt ${GRT_LIBRARIES} ${GTK3_LIBRARIES} ${SIGC++_LIBRARIES} ${MySQLCppConn_LIBRARIES})

if(BUILD_FOR_TESTS)
  target_link_libraries(db.mysql.query.grt gcov)
endif()

set_target_properties(db.mysql.query.grt
                      PROPERTIES PREFIX    ""
                                 VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

if(COMMAND cotire)
    set_target_properties(db.mysql.query.grt PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(db.mysql.query.grt)
endif()

install(TARGETS db.mysql.query.grt DESTINATION ${WB_PYTHON_MODULES_DIR})
