add_library(wbpublic
    grt/refresh_ui.cpp
    grt/editor_base.cpp
    grt/parse_utils.cpp
    grt/common.cpp
    grt/spatial_handler.cpp
    grt/grt_dispatcher.cpp
    grt/grt_manager.cpp
    grt/grt_reporter.cpp
    grt/grt_message_list.cpp
    grt/grt_shell.cpp
    grt/grt_string_list_model.cpp
    grt/grt_value_inspector.cpp
    grt/icon_manager.cpp
    grt/plugin_manager.cpp
    grt/tree_model.cpp
    grt/validation_manager.cpp
    grt/grt_threaded_task.cpp
    grt/action_list.cpp
    grtdb/charset_list.cpp
    grtdb/charset_utils.cpp
    grtdb/db_helpers.cpp
    grtdb/db_object_filter.cpp
    grtdb/db_object_helpers.cpp
    grtdb/db_object_master_filter.cpp
    grtdb/dbobject_roles.cpp
    grtdb/diff_dbobjectmatch.cpp
    grtdb/editor_dbobject.cpp
    grtdb/editor_routine.cpp
    grtdb/editor_routinegroup.cpp
    grtdb/editor_schema.cpp
    grtdb/editor_table.cpp
    grtdb/editor_user.cpp
    grtdb/editor_user_role.cpp
    grtdb/editor_view.cpp
    grtdb/role_tree_model.cpp
    grtdb/sync_profile.cpp
    objimpl/GrtStoredNote.cpp
    objimpl/db/db_DatabaseObject.cpp
    objimpl/db/db_Schema.cpp
    objimpl/db/db_Table.cpp
    objimpl/db/db_ForeignKey.cpp
    objimpl/db/db_Column.cpp
    objimpl/db/db_Index.cpp
    objimpl/db/db_Trigger.cpp
    objimpl/db/db_RoutineGroup.cpp
    grtsqlparser/module_utils.cpp
    grtsqlparser/sql_facade.cpp
    grtsqlparser/sql_parser_base.cpp
    grtsqlparser/sql_inserts_loader.cpp
    grtsqlparser/sql_semantic_check.cpp
    grtsqlparser/sql_normalizer.cpp
    grtsqlparser/sql_statement_decomposer.cpp
    grtsqlparser/sql_specifics.cpp
    grtsqlparser/mysql_parser_services.cpp
    sqlide/sqlide_generics.cpp
    sqlide/sql_editor_be.cpp
    sqlide/var_grid_model_be.cpp
    sqlide/recordset_be.cpp
    sqlide/recordset_data_storage.cpp
    sqlide/recordset_cdbc_storage.cpp
    sqlide/recordset_sql_storage.cpp
    sqlide/recordset_sqlite_storage.cpp
    sqlide/recordset_table_inserts_storage.cpp
    sqlide/recordset_text_storage.cpp
    sqlide/table_inserts_loader_be.cpp
    sqlide/sql_script_run_wizard.cpp
    sqlide/column_width_cache.cpp
    wbcanvas/figure_common.cpp
    wbcanvas/badge_figure.cpp
    wbcanvas/connection_figure.cpp
    wbcanvas/table_figure.cpp
    wbcanvas/table_figure_wb.cpp
    wbcanvas/table_figure_idef1x.cpp
    wbcanvas/table_figure_simple.cpp
    wbcanvas/view_figure.cpp
    wbcanvas/layer_figure.cpp
    wbcanvas/image_figure.cpp
    wbcanvas/note_figure.cpp
    wbcanvas/routine_group_figure.cpp
    wbcanvas/base_bridge.cpp
    wbcanvas/model_model_impl.cpp
    wbcanvas/model_diagram_impl.cpp
    wbcanvas/model_figure_impl.cpp
    wbcanvas/model_layer_impl.cpp
    wbcanvas/model_object_impl.cpp
    wbcanvas/model_connection_impl.cpp
    wbcanvas/workbench_physical_viewfigure_impl.cpp
    wbcanvas/workbench_physical_connection_impl.cpp
    wbcanvas/workbench_physical_model_impl.cpp
    wbcanvas/workbench_physical_diagram_impl.cpp
    wbcanvas/workbench_physical_routinegroupfigure_impl.cpp
    wbcanvas/workbench_physical_tablefigure_impl.cpp
    wbcanvas/workbench_model_notefigure_impl.cpp
    wbcanvas/workbench_model_imagefigure_impl.cpp
    objimpl/model/model_Connection.cpp
    objimpl/model/model_Figure.cpp
    objimpl/model/model_Layer.cpp
    objimpl/model/model_Model.cpp
    objimpl/model/model_Object.cpp
    objimpl/model/model_Diagram.cpp
    objimpl/workbench.model/workbench_model_NoteFigure.cpp
    objimpl/workbench.model/workbench_model_ImageFigure.cpp
    objimpl/workbench.physical/workbench_physical_Connection.cpp
    objimpl/workbench.physical/workbench_physical_RoutineGroupFigure.cpp
    objimpl/workbench.physical/workbench_physical_TableFigure.cpp
    objimpl/workbench.physical/workbench_physical_Diagram.cpp
    objimpl/workbench.physical/workbench_physical_Model.cpp
    objimpl/workbench.physical/workbench_physical_ViewFigure.cpp
    objimpl/workbench.logical/workbench_logical_Model.cpp
    objimpl/workbench.logical/workbench_logical_Diagram.cpp
    objimpl/db.query/db_query_Resultset.cpp
    objimpl/db.query/db_query_QueryBuffer.cpp
    objimpl/db.query/db_query_Editor.cpp
    objimpl/db.query/db_query_EditableResultset.cpp
    objimpl/db.migration/db_migration_Migration.cpp
    objimpl/db.mgmt/db_mgmt_SSHConnection.cpp
    objimpl/db.mgmt/db_mgmt_SSHFile.cpp
    objimpl/ui/ui_db_ConnectPanel.cpp
    objimpl/ui/ui_ObjectEditor.cpp
    objimpl/ui/ui_ObjectEditor_impl.cpp
    objimpl/wrapper/grt_PyObject.cpp
    objimpl/wrapper/mforms_ObjectReference.cpp
    objimpl/wrapper/parser_ContextReference.cpp
    grtui/grtdb_connect_panel.cpp
    grtui/grtdb_connect_dialog.cpp
    grtui/confirm_save_dialog.cpp
    grtui/binary_data_editor.cpp
    grtui/geom_draw_box.cpp
    grtui/grtdb_connection_editor.cpp
    grtui/grtdb_object_filter.cpp
    grtui/string_list_editor.cpp
    grtui/grt_wizard_form.cpp
    grtui/grt_wizard_plugin.cpp
    grtui/wizard_progress_page.cpp
    grtui/wizard_finished_page.cpp
    grtui/wizard_schema_filter_page.cpp
    grtui/wizard_object_filter_page.cpp
    grtui/wizard_view_text_page.cpp
    grtui/db_conn_be.cpp
    grtui/gui_plugin_base.cpp
    grtui/text_input_dialog.cpp
    grtui/checkbox_list_control.cpp
    grtui/file_charset_dialog.cpp
    grtui/inserts_export_form.cpp
)

target_include_directories(wbpublic
  PRIVATE
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}/library
)

target_include_directories(wbpublic
 SYSTEM
  PRIVATE
    ${GLIB_INCLUDE_DIRS}
    ${MySQLCppConn_INCLUDE_DIRS}
    ${GDAL_INCLUDE_DIRS}
)


target_compile_options(wbpublic PRIVATE ${WB_CXXFLAGS})

target_link_libraries(wbpublic
  PUBLIC
    ${VSQLITE_LIBRARIES}
    ${ANTLR4_LIBRARIES}
    wbpublic_Iface
  PRIVATE 
    wbbase mdcanvas mforms cdbc grt mtemplate parsers 
    ${CAIRO_LIBRARIES} ${OPENGL_LIBRARIES} ${PCRE_LIBRARIES} ${GDAL_LIBRARIES})

add_library(wbpublic_Iface INTERFACE)
add_library(wbpublic::wbpublic ALIAS wbpublic_Iface)
target_include_directories(wbpublic_Iface INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/generated>
)

target_include_directories(wbpublic_Iface
 SYSTEM INTERFACE
    $<BUILD_INTERFACE:${VSQLITE_INCLUDE_DIR}>
)


if(BUILD_FOR_GCOV)
  target_link_libraries(wbpublic PRIVATE gcov)
endif()
set_source_files_properties(sqlide/recordset_be.cpp
          PROPERTIES COMPILE_FLAGS "-Wno-nonnull-compare")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set_source_files_properties(sqlide/grammar-parser/ANTLRv3Lexer.c
          PROPERTIES COMPILE_FLAGS "-Wno-tautological-compare")
endif()



set_target_properties(wbpublic
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})
if(COMMAND cotire)
    set_target_properties(wbpublic PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(wbpublic)
endif()

install(TARGETS wbpublic DESTINATION ${WB_INSTALL_LIB_DIR})
