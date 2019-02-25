set(MODULE_DATA_FILES
    res/editor_mysql_table.glade
    res/editor_mysql_table_live.glade
    res/editor_view.glade
    res/editor_user.glade
    res/editor_role.glade
    res/editor_relationship.glade
    res/editor_routine.glade
    res/editor_rg.glade
    res/editor_schema.glade
    res/live_editor_decoration.glade
)

install(FILES ${MODULE_DATA_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/modules/data)

set(LIBRARY_SOURCES
    ../backend/mysql_relationship_editor.cpp
    ../backend/mysql_routine_editor.cpp
    ../backend/mysql_routinegroup_editor.cpp
    ../backend/mysql_schema_editor.cpp
    ../backend/mysql_table_editor.cpp
    ../backend/mysql_view_editor.cpp
    ../backend/register_plugin.cpp
    mysql_table_editor_fe.cpp
    mysql_table_editor_column_page.cpp
    mysql_view_editor_fe.cpp
    mysql_user_editor_fe.cpp
    mysql_role_editor_fe.cpp
    mysql_relationship_editor_fe.cpp
    mysql_routine_editor_fe.cpp
    mysql_routinegroup_editor_fe.cpp
    mysql_table_editor_index_page.cpp
    mysql_table_editor_fk_page.cpp
    mysql_table_editor_trigger_page.cpp
    mysql_table_editor_part_page.cpp
    mysql_table_editor_opt_page.cpp
    mysql_editor_priv_page.cpp
    schema_editor_fe.cpp
)

add_library(db.mysql.editors.wbp ${LIBRARY_SOURCES})
target_include_directories(db.mysql.editors.wbp
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../backend
    ${PROJECT_SOURCE_DIR}/modules
)



target_compile_options(db.mysql.editors.wbp PRIVATE ${WB_CXXFLAGS})

add_dependencies(db.mysql.editors.wbp db.mysql.grt)
target_link_libraries(db.mysql.editors.wbp 
  PRIVATE 
    linux_utilities
    grt
    parsers
    wbpublic
    wbbase
    mforms
    wbprivate::wbprivate
)

if(BUILD_FOR_GCOV)
  target_link_libraries(db.mysql.editors.wbp PRIVATE gcov)
endif()

set_target_properties(db.mysql.editors.wbp
                      PROPERTIES PREFIX    ""
                                 VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})
install(TARGETS db.mysql.editors.wbp DESTINATION ${WB_PACKAGE_PLUGINS_DIR})

if(COMMAND cotire)
    set_target_properties(db.mysql.editors.wbp PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(db.mysql.editors.wbp)
endif()

