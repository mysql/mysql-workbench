set(PY_FILES 
    backend/opts.py
    backend/wb_admin_config_file_be.py
    backend/wb_admin_control.py
    backend/wb_admin_security_be.py
    backend/wb_admin_user_privileges.py
    backend/wb_admin_variable_list.py
    backend/wb_common.py
    backend/wb_log_reader.py
    backend/wb_server_control.py
    backend/wb_server_management.py
    backend/wba_monitor_be.py
    backend/wb_admin_perfschema_instrumentation_be.py
    frontend/wb_admin_config_file_ui.py
    frontend/wb_admin_configuration_startup.py
    frontend/wb_admin_connections.py
    frontend/wb_admin_export.py
    frontend/wb_admin_export_options.py
    frontend/wb_admin_grt.py
    frontend/wb_admin_logs.py
    frontend/wb_admin_main.py
    frontend/wb_admin_monitor.py
    frontend/wb_admin_security.py
    frontend/wb_admin_server_status.py
    frontend/wb_admin_ui_profile.py
    frontend/wb_admin_utils.py
    frontend/wb_admin_variables.py
    frontend/wb_execute_window.py
    frontend/wba_ssh_ui.py
    frontend/wb_admin_perfschema.py
    frontend/wb_admin_perfschema_instrumentation.py
    frontend/wb_admin_perfschema_reports.py
    frontend/wb_admin_performance_dashboard.py
)

if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/internal)
    set(PY_WBA_MEB_FILES 
        internal/wba_meb/__init__.py
        internal/wba_meb/wb_admin_meb.py
        internal/wba_meb/wb_admin_meb_common.py
        internal/wba_meb/wb_admin_meb_config.py
        internal/wba_meb/wb_admin_meb_editor.py
        internal/wba_meb/wb_admin_meb_management.py
        internal/wba_meb/wb_admin_meb_overview.py
        internal/wba_meb/wb_admin_meb_restore.py
        internal/wba_meb/wb_admin_meb_syshelpers.py
    )
    install(FILES ${PY_WBA_MEB_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR}/wba_meb)
    
    set(PY_WBA_AUDIT_FILES 
        internal/wba_audit/__init__.py
        internal/wba_audit/wb_admin_audit_browser.py
        internal/wba_audit/wb_admin_audit_inspector.py
        internal/wba_audit/wb_admin_audit_log_parser.py
    )
    install(FILES ${PY_WBA_AUDIT_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR}/wba_audit)
    
    set(PY_WBA_FIREWALL_FILES 
        internal/wba_firewall/__init__.py
        internal/wba_firewall/wb_admin_firewall.py
        internal/wba_firewall/wb_admin_firewall_utils.py
        internal/wba_firewall/wb_admin_firewall_install.py
        internal/wba_firewall/wb_admin_firewall_manager.py
    )
    install(FILES ${PY_WBA_FIREWALL_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR}/wba_firewall)
endif ()

install(FILES ${PY_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR})
