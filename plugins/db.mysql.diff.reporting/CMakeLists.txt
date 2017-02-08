include_directories(.
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/backend
    ${PROJECT_SOURCE_DIR}/ext
    ${PROJECT_SOURCE_DIR}/ext/scintilla/include
    ${PROJECT_SOURCE_DIR}/modules/db.mysql/src
    ${PROJECT_SOURCE_DIR}/library/cdbc/src
    ${PROJECT_SOURCE_DIR}/modules/interfaces
    ${PROJECT_SOURCE_DIR}/modules/
    ${PROJECT_SOURCE_DIR}/library/grt/src/diff
    ${PROJECT_SOURCE_DIR}/library/forms
    ${PROJECT_SOURCE_DIR}/library/forms/mforms
    ${PROJECT_SOURCE_DIR}/plugins
    ${PROJECT_SOURCE_DIR}/plugins/db.mysql
    ${PROJECT_SOURCE_DIR}
    backend
    SYSTEM ${VSQLITE_INCLUDE_DIRS}
    SYSTEM ${LIBXML2_INCLUDE_DIR}
    SYSTEM ${CAIRO_INCLUDE_DIRS}
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${SIGC++_INCLUDE_DIRS}
    SYSTEM ${PCRE_INCLUDE_DIRS}
    SYSTEM ${MySQLCppConn_INCLUDE_DIRS}
    SYSTEM ${Boost_INCLUDE_DIRS}
)

add_library(db.mysql.diff.reporting.wbp MODULE
    backend/db_mysql_diff_reporting.cpp
    backend/register_plugin.cpp
    frontend/diff_reporting.cpp
    ${PROJECT_SOURCE_DIR}/plugins/db.mysql/backend/db_plugin_be.cpp
    ${PROJECT_SOURCE_DIR}/plugins/db.mysql/backend/wb_plugin_be.cpp
)

target_compile_options(db.mysql.diff.reporting.wbp PUBLIC ${WB_CXXFLAGS})

set_target_properties(db.mysql.diff.reporting.wbp PROPERTIES
#	LINK_FLAGS "-module"
	PREFIX ""
)

target_link_libraries(db.mysql.diff.reporting.wbp grt mforms wbscintilla ${GTK3_LIBRARIES} ${LIBXML2_LIBRARIES})

if(BUILD_FOR_TESTS)
  target_link_libraries(db.mysql.diff.reporting.wbp gcov)
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
