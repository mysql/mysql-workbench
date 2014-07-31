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
    ${PROJECT_SOURCE_DIR}/library/forms
    ${PROJECT_SOURCE_DIR}/plugins
    ${PROJECT_SOURCE_DIR}/plugins/db/backend
    ${PROJECT_SOURCE_DIR}/plugins/db.mysql.sync/backend
    ${PROJECT_SOURCE_DIR}/ext/scintilla/include
    backend
    ${VSQLITE_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    ${GRT_INCLUDE_DIRS}
    ${GTK2_INCLUDE_DIRS}
    ${SIGC++_INCLUDE_DIRS}
    ${PCRE_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${MYSQLCPPCONN_INCLUDE_DIRS}
    ${ANTLR3C_INCLUDE_DIRS}
)

add_library(db.mysql.wbp MODULE
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

set_target_properties(db.mysql.wbp PROPERTIES
#	LINK_FLAGS "-module"
	PREFIX ""
  VERSION   ${WB_VERSION}
  SOVERSION ${WB_VERSION}
)

#target_link_libraries(db.mysql.wbp linux_utilities wbscintilla)

install(TARGETS db.mysql.wbp DESTINATION ${WB_PACKAGE_PLUGINS_DIR})
