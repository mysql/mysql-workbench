if (UNIX)
  configure_file(wbcopytables.in wbcopytables)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/wbcopytables DESTINATION ${WB_INSTALL_BIN_DIR})
  
  add_executable(wbcopytables-bin
      copytable/copytable.cpp
      copytable/python_copy_data_source.cpp
      copytable/main.cpp
      copytable/converter.cpp
  )

  target_compile_options(wbcopytables-bin PUBLIC ${WB_CXXFLAGS})
  target_compile_options(wbcopytables-bin PRIVATE ${ODBC_DEFINITIONS})
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(wbcopytables-bin PUBLIC -fPIE)
  else()
    target_compile_options(wbcopytables-bin PUBLIC -fPIE -pie)
  endif()

  target_link_libraries(
      wbcopytables-bin 
    PRIVATE
      wbbase 
      wbssh
      wbprivate
      wbpublic
      ${GMODULE_LIBRARIES}
      ${MySQL_LIBRARIES} 
      ${ODBC_LIBRARIES} 
      ${PYTHON_LIBRARIES}
      ${LIBXML2_LIBRARIES} 
      ${LIBZIP_LIBRARIES}
  )
  
  target_include_directories(wbcopytables-bin
   SYSTEM
    PRIVATE
      ${ODBC_INCLUDE_DIRS}  
      ${MySQL_INCLUDE_DIRS} 
  )

  if(BUILD_FOR_GCOV)
    target_link_libraries(wbcopytables-bin PRIVATE gcov)
  endif()
   if(COMMAND cotire)
       set_target_properties(wbcopytables-bin PROPERTIES
           COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS};/usr/include/unixodbc_conf.h;/usr/include/sql.h;/usr/include/sqltypes.h;/usr/include/sqlext.h;/usr/include/sqlucode.h")

       cotire(wbcopytables-bin) 
   endif()
  install(TARGETS wbcopytables-bin DESTINATION ${WB_INSTALL_DIR_EXECUTABLE})
else()
  add_executable(wbcopytables
      copytable/copytable.cpp
      copytable/python_copy_data_source.cpp
      copytable/main.cpp
      copytable/converter.cpp
  )

  target_compile_options(wbcopytables PRIVATE ${WB_CXXFLAGS})
  target_compile_options(wbcopytables PRIVATE ${ODBC_DEFINITIONS})
  target_include_directories(wbcopytables
   SYSTEM
    PRIVATE
     ${ODBC_INCLUDE_DIRS}  
     ${MySQL_INCLUDE_DIRS} 
  )

  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(wbcopytables PUBLIC -fPIE)
  else()
    target_compile_options(wbcopytables PUBLIC -fPIE -pie)
  endif()

   target_link_libraries(
      wbcopytables 
    PRIVATE
      wbbase 
      wbssh
      wbprivate
      wbpublic
      ${GMODULE_LIBRARIES}
      ${MySQL_LIBRARIES} 
      ${ODBC_LIBRARIES} 
      ${PYTHON_LIBRARIES}
      ${LIBXML2_LIBRARIES} 
      ${LIBZIP_LIBRARIES}
  )
   
   if(BUILD_FOR_GCOV)
    target_link_libraries(wbcopytables PRIVATE gcov)
   endif()
   if(COMMAND cotire)
       set_target_properties(wbcopytables PROPERTIES
           COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS};/usr/include/unixodbc_conf.h;/usr/include/sql.h;/usr/include/sqltypes.h;/usr/include/sqlext.h;/usr/include/sqlucode.h")

       cotire(wbcopytables) 
   endif()

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
