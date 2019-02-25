add_library(db.mysql.diff.reporting.wbp
    backend/db_mysql_diff_reporting.cpp
    backend/register_plugin.cpp
    frontend/diff_reporting.cpp
)

target_include_directories(db.mysql.diff.reporting.wbp
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/backend
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/modules/
)

target_compile_options(db.mysql.diff.reporting.wbp PRIVATE ${WB_CXXFLAGS})

set_target_properties(db.mysql.diff.reporting.wbp PROPERTIES
	PREFIX ""
)

target_link_libraries(db.mysql.diff.reporting.wbp 
  PRIVATE wbpublic grt mforms wbscintilla cdbc db.mysql.wbp)

if(BUILD_FOR_GCOV)
  target_link_libraries(db.mysql.diff.reporting.wbp PRIVATE gcov)
endif()

set_target_properties(db.mysql.diff.reporting.wbp
                      PROPERTIES PREFIX    ""
                                 VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

if(COMMAND cotire)
    set_target_properties(db.mysql.diff.reporting.wbp PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(db.mysql.diff.reporting.wbp)
endif()

install(TARGETS db.mysql.diff.reporting.wbp DESTINATION ${WB_PACKAGE_PLUGINS_DIR})
