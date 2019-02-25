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

add_library(db.mysql.wbp ${LIBRARY_SOURCES})

target_include_directories(db.mysql.wbp
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend
    ${CMAKE_CURRENT_SOURCE_DIR}/backend
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/modules/
)

target_compile_options(db.mysql.wbp PRIVATE ${WB_CXXFLAGS})
target_link_libraries(db.mysql.wbp PUBLIC db.mysql.wbp_Iface PRIVATE wbpublic grt cdbc db.mysql.grt parsers mforms)
if(BUILD_FOR_GCOV)
  target_link_libraries(db.mysql.wbp PRIVATE gcov)
endif(BUILD_FOR_GCOV)

add_library(db.mysql.wbp_Iface INTERFACE)
add_library(db.mysql.wbp::db.mysql.wbp ALIAS db.mysql.wbp_Iface)

target_include_directories(db.mysql.wbp_Iface INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> 
)

set_target_properties(db.mysql.wbp  PROPERTIES  PREFIX ""
                                                VERSION   ${WB_VERSION}
                                                SOVERSION ${WB_VERSION}
)
install(TARGETS db.mysql.wbp DESTINATION ${WB_PACKAGE_PLUGINS_DIR})

if(COMMAND cotire)
    set_target_properties(db.mysql.wbp PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(db.mysql.wbp)
endif()
