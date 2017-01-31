include_directories(.
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/backend
    ${PROJECT_SOURCE_DIR}/modules/db.mysql/src
    ${PROJECT_SOURCE_DIR}/library/cdbc/src
    ${PROJECT_SOURCE_DIR}/library/forms/mforms
    ${PROJECT_SOURCE_DIR}/modules/interfaces
    ${PROJECT_SOURCE_DIR}/modules/
    ${PROJECT_SOURCE_DIR}/library/grt/src/diff
    ${PROJECT_SOURCE_DIR}/library/sql-parser/include
    ${PROJECT_SOURCE_DIR}/library/sql-parser/source
    ${PROJECT_SOURCE_DIR}/library/parsers
    ${PROJECT_SOURCE_DIR}/library/forms
    ${PROJECT_SOURCE_DIR}/plugins
    ${PROJECT_SOURCE_DIR}/plugins/db/backend
    ${PROJECT_SOURCE_DIR}/plugins/db.mysql.sync/backend
    ${PROJECT_SOURCE_DIR}/ext/scintilla/include
    ${PROJECT_SOURCE_DIR}/library/mysql.parser/
    backend
    SYSTEM ${VSQLITE_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    SYSTEM ${GRT_INCLUDE_DIRS}
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${SIGC++_INCLUDE_DIRS}
    SYSTEM ${PCRE_INCLUDE_DIRS}
    SYSTEM ${CAIRO_INCLUDE_DIRS}
    SYSTEM ${MySQLCppConn_INCLUDE_DIRS}
    SYSTEM ${ANTLR3C_INCLUDE_DIRS}
    SYSTEM ${Boost_INCLUDE_DIRS}
)

set(LIBRARY_SOURCES
    backend/db_frw_eng_be.cpp
    backend/db_mysql_sql_export.cpp
    backend/db_mysql_sql_script_sync.cpp
    backend/db_mysql_sql_sync.cpp
    backend/db_mysql_validation_page.cpp
    backend/db_plugin_be.cpp
    backend/db_alter_script_be.cpp
    backend/db_rev_eng_be.cpp
    backend/diff_tree.cpp
    backend/register_plugin.cpp
    backend/sql_import_be.cpp
    backend/wb_plugin_be.cpp
    frontend/db_forward_engineer.cpp
    frontend/db_forward_engineer_script.cpp
    frontend/db_reverse_engineer.cpp
    frontend/db_reverse_engineer_script.cpp
    frontend/db_synchronize_any.cpp
    frontend/db_synchronize_model.cpp
    frontend/schema_matching_page.cpp
    frontend/synchronize_differences_page.cpp
)

add_library(db.mysql.wbp MODULE ${LIBRARY_SOURCES})

target_compile_options(db.mysql.wbp PUBLIC ${WB_CXXFLAGS})

target_link_libraries(db.mysql.wbp gcov)

set_target_properties(db.mysql.wbp  PROPERTIES  PREFIX ""
                                                VERSION   ${WB_VERSION}
                                                SOVERSION ${WB_VERSION}
)
install(TARGETS db.mysql.wbp DESTINATION ${WB_PACKAGE_PLUGINS_DIR})

if (BUILD_FOR_TESTS)
  # Generate a regular shared library (as opposed to a module) with the sames contents
  # as the module library. The module library is not supposed to be linked (since it's a plugin)
  # but we need to link it on TUT. Until we find a better way to do this, we just generate
  # both libraries, but on different directories, so it doesn't interfeer with the packaging
  # process.
  add_library(db.mysql.wbp.debug ${LIBRARY_SOURCES})
  target_compile_options(db.mysql.wbp.debug PUBLIC ${WB_CXXFLAGS})
  target_link_libraries(db.mysql.wbp.debug gcov)
  set_target_properties(db.mysql.wbp.debug 
                        PROPERTIES  VERSION   ${WB_VERSION}
                                    SOVERSION ${WB_VERSION}
  )
  install(TARGETS db.mysql.wbp.debug DESTINATION "${WB_PACKAGE_PLUGINS_DIR}_tests")
endif()

if(COMMAND cotire)
    set_target_properties(db.mysql.wbp PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(db.mysql.wbp)
endif()
