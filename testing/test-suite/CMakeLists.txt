
cmake_minimum_required(VERSION 3.8)

project(WORKBENCH_UNIT_TESTS)


set(workbench_dir "${CMAKE_SOURCE_DIR}/../..") # This must come from the parent
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${workbench_dir}/build/cmake/Modules/")

#if(ENABLE_COTIRE AND CMAKE_VERSION VERSION_GREATER "2.8.11")
#  include(cotire)
#  set_property(DIRECTORY PROPERTY COTIRE_UNITY_LINK_LIBRARIES_INIT "COPY_UNITY")
#endif()

if(DISABLE_SPECIAL_CHARACTERS)
  add_compile_definitions(DISABLE_SPECIAL_CHARACTERS)
endif()

set(path_to_binaries "${workbench_dir}/wb-build/usr/local")
set(path_to_libraries "${path_to_binaries}/lib/mysql-workbench")
set(path_to_modules "${path_to_libraries}/modules")
set(path_to_plugins "${path_to_libraries}/plugins")

find_package(PkgConfig REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(MySQLCppConn 1.1.8 REQUIRED)
# find_package(VSqlite REQUIRED)
# find_package(X11 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Boost REQUIRED)
find_package(ANTLR4 REQUIRED)

pkg_check_modules(PCRE REQUIRED libpcre libpcrecpp)
pkg_check_modules(GLIB REQUIRED glib-2.0)
# pkg_check_modules(SIGC REQUIRED sigc++-2.0)
pkg_check_modules(GTHREAD REQUIRED gthread-2.0)
# pkg_check_modules(GMODULE REQUIRED gmodule-2.0)
pkg_check_modules(CAIRO REQUIRED cairo>=1.5.12)
pkg_check_modules(LIBZIP REQUIRED libzip)
find_package(GDAL REQUIRED)
find_package(MySQL 5.6.0 REQUIRED)


# Determine std c++ version
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
 set(CMAKE_CXX_STANDARD 17)
 set(CMAKE_CXX_STANDARD_REQUIRED ON)
 if(CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "8.0" OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "8.0")
   message("Using C++17 standard")
   set(CMAKE_CXX_STANDARD 17)
 else()
   message(FATAL_ERROR "Insufficient gcc version. You need gcc v8.0 or newer to build this project")
 endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
 set(CMAKE_CXX_STANDARD 17)
 set(CMAKE_CXX_STANDARD_REQUIRED ON)
 if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "5.0")
   message("Using C++17 standard")
   set(CMAKE_CXX_STANDARD 17)
 else()
   message(FATAL_ERROR "Insufficient clang version. You need clang 5.0 or newer to build this project")
 endif()
endif()

add_subdirectory(casmine)
#add_subdirectory(tests)

add_executable(wbtests-bin
  main.cpp
  casmine/helpers.cpp
  tests/wb_references.cpp
  tests/wb_test_helpers.cpp
  tests/grt_test_helpers.cpp
  tests/wb_connection_helpers.cpp
  tests/model_mockup.cpp

  tests/casmine_specs.cpp

  tests/library/cdbc/dbc_general_specs.cpp
  tests/library/cdbc/dbc_connection_specs.cpp
  tests/library/cdbc/dbc_metadata_specs.cpp
  tests/library/cdbc/dbc_result_set_specs.cpp
  tests/library/cdbc/dbc_statement_specs.cpp

  tests/library/forms/stub/src/stub_app.cpp
  tests/library/forms/stub/src/stub_base.cpp
  tests/library/forms/stub/src/stub_drawbox.cpp
  tests/library/forms/stub/src/stub_form.cpp
  tests/library/forms/stub/src/stub_listbox.cpp
  tests/library/forms/stub/src/stub_menu.cpp
  tests/library/forms/stub/src/stub_mforms.cpp
  tests/library/forms/stub/src/stub_selector.cpp
  tests/library/forms/stub/src/stub_textbox.cpp
  tests/library/forms/stub/src/stub_textentry.cpp
  tests/library/forms/stub/src/stub_treenode.cpp
  tests/library/forms/stub/src/stub_utilities.cpp
  tests/library/forms/stub/src/stub_view.cpp
  tests/library/forms/stub/src/stub_wizard.cpp
  tests/library/forms/utilities_specs.cpp
  tests/library/forms/code_editor_specs.cpp

  tests/library/base/commandlineparser_specs.cpp
  tests/library/base/fileutilities_specs.cpp
  tests/library/mtemplates/mtemplate_specs.cpp
  tests/library/base/sqlstring_specs.cpp
  tests/library/base/stringutilities_specs.cpp
  tests/library/base/threading_specs.cpp
  tests/library/base/utf8string_specs.cpp
  tests/library/base/config_file_specs.cpp

  tests/library/mysql.canvas/mysqlcanvas_specs.cpp
#  tests/library/sqlparser_specs.cpp

#  tests/library/dbc_specs.cpp

  tests/library/grt/comparer_specs.cpp
  tests/library/grt/diff_tree_specs.cpp
  tests/library/grt/grtdiff_alter_specs.cpp
  tests/library/grt/grtdiff_db_specs.cpp
  tests/library/grt/grtlistdiff_specs.cpp
  tests/library/grt/grtpp_serialization_specs.cpp
  tests/library/grt/grtpp_specs.cpp
  tests/library/grt/grtpp_util_specs.cpp
  tests/library/grt/modulenative_specs.cpp
  tests/library/grt/object_specs.cpp
  tests/library/grt/struct_specs.cpp
  tests/library/grt/sync_profile_specs.cpp
  tests/library/grt/value_specs.cpp

  tests/library/parsers/mysql_parser_specs.cpp
  
  tests/backend/wbpublic/grt/common_specs.cpp
  tests/backend/wbpublic/grt/grt_dispatcher_specs.cpp
  tests/backend/wbpublic/grt/shell_specs.cpp
  tests/backend/wbpublic/grtdb/editor_table_specs.cpp
  tests/backend/wbpublic/grtdb/table_inserts_specs.cpp
  tests/backend/wbpublic/grt/nodeid_specs.cpp
  tests/backend/wbpublic/grt/tree_model_specs.cpp
  tests/backend/wbpublic/grt/grt_inspector_value_specs.cpp
  
  tests/backend/wbpublic/sqlide/recordset_specs.cpp
  tests/backend/wbpublic/sqlide/sql_editor_be_autocomplete_specs.cpp
  
  tests/backend/wbprivate/workbench/ssh_specs.cpp
  tests/backend/wbprivate/workbench/overview_specs.cpp
  tests/backend/wbprivate/workbench/wb_module_specs.cpp
  tests/backend/wbprivate/workbench/wb_undo_diagram_specs.cpp
  tests/backend/wbprivate/workbench/wb_undo_editors_specs.cpp
  tests/backend/wbprivate/workbench/wb_undo_others_specs.cpp
  tests/backend/wbprivate/workbench/wb_model_file_specs.cpp
  tests/backend/wbprivate/workbench/wb_context_specs.cpp
  tests/backend/wbprivate/workbench/wb_copy_paste_specs.cpp
  tests/backend/wbprivate/workbench/wb_lowlevel_specs.cpp
  tests/backend/wbprivate/sqlide/wb_sql_editor_help_specs.cpp
  tests/backend/wbprivate/sqlide/wb_sql_editor_form_specs.cpp
  tests/backend/wbprivate/sqlide/wb_live_schema_tree_specs.cpp
  
  tests/modules/db.mysql/db_mysql_gen_grant_specs.cpp
  tests/modules/db.mysql/sql_create_specs.cpp

  tests/modules/db.mysql.parser/mysql_parser_module_specs.cpp
  tests/modules/db.mysql.parser/parse_datatypes_specs.cpp
  
  tests/modules/db.mysql.sqlparser/mysql_invalid_sql_parser_specs.cpp
  tests/modules/db.mysql.sqlparser/mysql_sql_facade_specs.cpp
  tests/modules/db.mysql.sqlparser/mysql_sql_parser_specs.cpp
  tests/modules/db.mysql.sqlparser/mysql_sql_statement_decomposer_specs.cpp
  
  tests/plugins/db.mysql/backend/db_mysql_plugin_specs.cpp
  tests/plugins/db.mysql/backend/db_mysql_sql_export_specs.cpp
  tests/plugins/db.mysql/backend/model_diff_apply_specs.cpp
  
  tests/plugins/db.mysql.editors/backend/mysql_routinegroup_editor_specs.cpp
  tests/plugins/db.mysql.editors/backend/mysql_table_editor_specs.cpp
)

target_include_directories(wbtests-bin
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/library/forms/stub
    ${PROJECT_SOURCE_DIR}/casmine

    ${workbench_dir}
    ${workbench_dir}/library
    ${workbench_dir}/library/base
    ${workbench_dir}/library/base/base
    ${workbench_dir}/library/cdbc/src
    ${workbench_dir}/library/grt/src
    ${workbench_dir}/library/mysql.canvas/src
    ${workbench_dir}/library/sql.parser/include
    ${workbench_dir}/library/sql.parser/source
    ${workbench_dir}/library/ssh
    ${workbench_dir}/library/forms
    ${workbench_dir}/library/parsers

    ${workbench_dir}/modules
    ${workbench_dir}/modules/wb.model/src
    ${workbench_dir}/modules/db.mysql.sqlparser/src
    ${workbench_dir}/modules/db.mysql/src

    ${workbench_dir}/plugins/db.mysql
    ${workbench_dir}/plugins/db.mysql/backend
    ${workbench_dir}/plugins/db.mysql.editors/backend

    ${workbench_dir}/backend/wbpublic
    ${workbench_dir}/backend/wbprivate
    ${workbench_dir}/backend/wbprivate/workbench
    ${workbench_dir}/backend/wbprivate/model

    ${workbench_dir}/generated/
    ${workbench_dir}/generated/grti
    ${workbench_dir}/ext/scintilla/include/
    ${MySQL_INCLUDE_DIRS}
    SYSTEM ${MySQLCppConn_INCLUDE_DIRS}
  PUBLIC
    ${PROJECT_SOURCE_DIR}
    casmine
    SYSTEM ${GLIB_INCLUDE_DIRS}
    SYSTEM ${LIBXML2_INCLUDE_DIR}
    SYSTEM ${LibSSH_INCLUDE_DIR}
    SYSTEM ${VSQLITE_INCLUDE_DIR}
    SYSTEM ${ANTLR4_INCLUDE_DIR}
)

target_compile_definitions(wbtests-bin
	PRIVATE
		ENABLE_TESTING
		RAPIDJSON_HAS_STDSTRING
	)

target_link_libraries(wbtests-bin
  PUBLIC
    casmine
#    workbenchtests
    ${path_to_libraries}/libwbbase.so
    ${path_to_libraries}/libgrt.so
    ${path_to_libraries}/libmdcanvas.so
    ${path_to_libraries}/libmtemplate.so
    ${path_to_libraries}/libmforms.so
    ${path_to_libraries}/libwbprivate.so
    ${path_to_libraries}/libwbpublic.so
    ${path_to_libraries}/libwbssh.so
    ${path_to_libraries}/libcdbc.so
    ${path_to_libraries}/libparsers.so
    ${path_to_libraries}/libsqlparser.so
    ${path_to_libraries}/plugins/db.mysql.wbp.so

#    ${path_to_plugins}/libdb.mysql.wbp.debug.so
#    ${path_to_plugins}/libdb.mysql.editors.wbp.debug.so
    ${path_to_plugins}/db.mysql.editors.wbp.so

    ${path_to_modules}/db.mysql.grt.so
    ${path_to_modules}/db.mysql.sqlparser.grt.so
    ${path_to_modules}/wb.model.grt.so

    ${ANTLR4_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${GTHREAD_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${GDAL_LIBRARIES}
    ${MySQL_LIBRARIES}
    ${LibSSH_LIBRARIES}
    ${LIBZIP_LIBRARIES}
    ${PCRE_LIBRARIES}
    ${MySQLCppConn_LIBRARIES}
    stdc++fs
  PRIVATE
)
