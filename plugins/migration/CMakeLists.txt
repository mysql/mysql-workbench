include_directories(
    ${ODBC_INCLUDE_DIRS} 
    ${PYTHON_INCLUDE_DIRS} 
    ${MYSQL_INCLUDE_DIRS} 
    ${PROJECT_SOURCE_DIR}/backend/wbprivate
)

add_definitions(${ODBC_DEFINITIONS})

if (UNIX)
  configure_file(wbcopytables.in wbcopytables)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/wbcopytables DESTINATION ${WB_INSTALL_BIN_DIR})
  
  add_executable(wbcopytables-bin
      copytable/copytable.cpp
      copytable/python_copy_data_source.cpp
      copytable/main.cpp
      copytable/converter.cpp
  )
  
  target_link_libraries(wbcopytables-bin wbbase ${MYSQL_LIBRARIES} ${ODBC_LIBRARIES} ${PCRE_LIBRARIES} ${PYTHON_LIBRARIES})
  
  install(TARGETS wbcopytables-bin DESTINATION ${WB_INSTALL_DIR_EXECUTABLE})
else()
  add_executable(wbcopytables
      copytable/copytable.cpp
      copytable/python_copy_data_source.cpp
      copytable/main.cpp
      copytable/converter.cpp
  )
  
  target_link_libraries(wbcopytables wbbase ${MYSQL_LIBRARIES} ${ODBC_LIBRARIES} ${PCRE_LIBRARIES} ${PYTHON_LIBRARIES})
  
  install(TARGETS wbcopytables DESTINATION ${WB_INSTALL_BIN_DIR})
endif()

set(PY_FILES 
    frontend/migration_project_management.py
    frontend/migration_schema_selection.py
    frontend/migration_overview.py
    frontend/datatype_mapping_editor.py
    frontend/migration_schema_mappings.py
    frontend/migration_schema_creation.py
    frontend/migration_source_selection.py
    frontend/migration_object_migration.py
    frontend/migration_object_selection.py
    frontend/migration_data_transfer.py
    frontend/migration_ui_style.py
    frontend/migration_toolbars.py
    frontend/migration_main.py
    frontend/migration_summary.py
    frontend/migration_object_editing.py
    frontend/migration_bulk_copy_data.py
    backend/migration.py
    backend/DataMigrator.py
    dbcopy/db_copy_grt.py
    dbcopy/db_copy_main.py
    dbcopy/db_copy_overview.py
    dbcopy/db_copy_source_target.py
    dbcopy/db_copy_schema_selection.py
    dbcopy/db_copy_progress.py
    dbcopy/db_copy_report.py
    migration_grt.py
)

install(FILES ${PY_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR})
