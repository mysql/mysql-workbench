add_library(wbprivate
    model/canvas_floater.cpp
    model/mini_view.cpp
    model/relationship_canvas_floater.cpp
    model/wb_catalog_tree_view.cpp
    model/wb_component.cpp
    model/wb_component_basic.cpp
    model/wb_component_logical.cpp
    model/wb_component_physical.cpp
    model/wb_component_physical_relationship.cpp
    model/wb_context_model.cpp
    model/wb_diagram_options.cpp
    model/wb_layer_tree.cpp
    model/wb_model_diagram_form.cpp
    model/wb_overview_diagram.cpp
    model/wb_overview_physical.cpp
    model/wb_overview_physical_schema.cpp
    model/wb_overview_privileges.cpp
    model/wb_physical_model_diagram_features.cpp
    model/wb_user_datatypes.cpp
    model/wb_history_tree.cpp
    model/wb_template_list.cpp
    sqlide/db_sql_editor_history_be.cpp
    sqlide/db_sql_editor_log.cpp
    sqlide/wb_sql_editor_form.cpp
    sqlide/wb_sql_editor_buffer.cpp
    sqlide/wb_sql_editor_form_ui.cpp
    sqlide/wb_sql_editor_help.cpp
    sqlide/wb_sql_editor_tree_controller.cpp
    sqlide/execute_routine_wizard.cpp
    sqlide/wb_sql_editor_panel.cpp
    sqlide/wb_sql_editor_result_panel.cpp
    sqlide/wb_context_sqlide.cpp
    sqlide/result_form_view.cpp
    sqlide/wb_live_schema_tree.cpp
    sqlide/wb_sql_editor_snippets.cpp
    sqlide/query_side_palette.cpp
    sqlide/spatial_data_view.cpp
    sqlide/spatial_draw_box.cpp
    workbench/metaclasses.cpp
    workbench/upgrade_helper.cpp
    workbench/wb_command_ui.cpp
    workbench/wb_context_ui.cpp
    workbench/wb_context_ui_home.cpp
    workbench/wb_context.cpp
    workbench/wb_model_file.cpp
    workbench/wb_model_file_upgrade.cpp
    workbench/wb_module.cpp
    workbench/wb_tunnel.cpp
    workbench/wb_overview.cpp
    workbench/wb_tunnel.cpp
    workbench/wb_db_schema.cpp
    workbench/about_box.cpp
    workbench/SSHSessionWrapper.cpp
    workbench/SSHFileWrapper.cpp
    workbench/license_view.cpp
    ${PROJECT_SOURCE_DIR}/frontend/common/new_server_instance_wizard.cpp
    ${PROJECT_SOURCE_DIR}/frontend/common/preferences_form.cpp
    ${PROJECT_SOURCE_DIR}/frontend/common/new_connection_wizard.cpp
    ${PROJECT_SOURCE_DIR}/frontend/common/document_properties_form.cpp
    ${PROJECT_SOURCE_DIR}/frontend/common/user_defined_type_editor.cpp
    ${PROJECT_SOURCE_DIR}/frontend/common/grt_shell_window.cpp
    ${PROJECT_SOURCE_DIR}/frontend/common/grt_code_editor.cpp
    ${PROJECT_SOURCE_DIR}/frontend/common/plugin_manager_window.cpp
    ${PROJECT_SOURCE_DIR}/frontend/common/plugin_install_window.cpp
    ${PROJECT_SOURCE_DIR}/frontend/common/select_option_dialog.cpp
    ${PROJECT_SOURCE_DIR}/frontend/common/snippet_list.cpp
    ${PROJECT_SOURCE_DIR}/frontend/common/server_instance_editor.cpp
    ${PROJECT_SOURCE_DIR}/frontend/common/grt_python_debugger.cpp
    ${PROJECT_SOURCE_DIR}/frontend/common/advanced_sidebar.cpp
    ${PROJECT_SOURCE_DIR}/frontend/common/snippet_popover.cpp
    ${PROJECT_SOURCE_DIR}/frontend/common/grt_plugin_wizard.cpp
)

target_compile_definitions(wbprivate PRIVATE USE_BUNDLED_MYSQLDUMP)

target_include_directories(wbprivate
  PRIVATE
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}/frontend/common/
)

target_compile_options(wbprivate PRIVATE ${WB_CXXFLAGS})

target_link_libraries(wbprivate
  PUBLIC
    ${GDAL_LIBRARIES}
    wbprivate_Iface
    wbssh::wbssh
  PRIVATE
    grt
    mdcanvas
    wbssh
    cdbc
    parsers
    mforms
    wbpublic::wbpublic
    Rapidjson::Rapidjson
)

add_library(wbprivate_Iface INTERFACE)
add_library(wbprivate::wbprivate ALIAS wbprivate_Iface)

target_include_directories(wbprivate_Iface INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/workbench>

)

target_include_directories(wbprivate_Iface
 SYSTEM INTERFACE
    ${VSQLITE_INCLUDE_DIRS}
    ${GDAL_INCLUDE_DIRS}
)

add_dependencies(wbprivate generate_parser_mysql)


if(BUILD_FOR_GCOV)
  target_link_libraries(wbprivate PRIVATE gcov)
endif()

set_target_properties(wbprivate
                      PROPERTIES VERSION   ${WB_VERSION}
                      SOVERSION ${WB_VERSION})

if(COMMAND cotire)
  set_target_properties(wbprivate PROPERTIES
    COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(wbprivate)
endif()
install(TARGETS wbprivate DESTINATION ${WB_INSTALL_LIB_DIR})
