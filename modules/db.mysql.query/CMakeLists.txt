add_library(db.mysql.query.grt
    src/dbquery.cpp
)

target_include_directories(db.mysql.query.grt
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_compile_options(db.mysql.query.grt PRIVATE ${WB_CXXFLAGS})

target_link_libraries(db.mysql.query.grt PRIVATE grt cdbc ${GTK3_LIBRARIES} ${SIGC++_LIBRARIES} ${MySQLCppConn_LIBRARIES})

if(BUILD_FOR_GCOV)
  target_link_libraries(db.mysql.query.grt PRIVATE gcov)
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
