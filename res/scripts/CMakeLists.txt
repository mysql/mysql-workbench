set(LIBRARY_FILES 
    python/wb.py
    python/db_utils.py
    python/grt_python_debugger.py
)
install(FILES ${LIBRARY_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/libraries)


set(SHELL_SNIPPETS_FILES
    snippets/shell_snippets.py.txt
)
install(FILES ${SHELL_SNIPPETS_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR})


set(VBSDATA_FILES
    shell/mysql_system_status.vbs
    shell/mysql_system_status_rmt.vbs
    python/mysqlwbmeb.py
)
install(FILES ${VBSDATA_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR})


set(TEMPLATE_FILES
    script_templates/eer_model_object_grt.py.txt
    script_templates/eer_model_utility_grt.py.txt
    script_templates/generic_grt.py.txt
    script_templates/sql_editor_catalog_grt.py.txt
    script_templates/sql_editor_filter_grt.py.txt
    script_templates/sql_editor_resultset_grt.py.txt
    script_templates/sql_editor_utility_grt.py.txt
)

install(FILES ${TEMPLATE_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/script_templates)

# Do not edit WBSYS_FILES manualy. The containing files are updated 
# by res/scripts/update_wbsys.py script
# SYS_BEGIN

set(SYS_FILES
    sys/after_setup.sql
    sys/before_setup.sql
    sys/sys_56.sql
    sys/sys_57.sql
    sys/sys_reports.js
)

install(FILES ${SYS_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/sys)

install(FILES sys/generate_sql_file.sh
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
        DESTINATION ${WB_PACKAGE_SHARED_DIR}/sys)

set(SYS_FUNCTIONS_FILES
    sys/functions/extract_schema_from_file_name.sql
    sys/functions/extract_table_from_file_name.sql
    sys/functions/format_bytes.sql
    sys/functions/format_path.sql
    sys/functions/format_statement.sql
    sys/functions/format_time.sql
    sys/functions/ps_is_account_enabled.sql
    sys/functions/ps_is_instrument_default_enabled.sql
    sys/functions/ps_is_instrument_default_timed.sql
    sys/functions/ps_is_account_enabled_57.sql
	sys/functions/ps_is_consumer_enabled.sql
	sys/functions/ps_is_thread_instrumented.sql
    sys/functions/ps_thread_id.sql
    sys/functions/ps_thread_stack.sql
    sys/functions/sys_get_config.sql
)

install(FILES ${SYS_FUNCTIONS_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/sys/functions)

set(SYS_VIEWS_P_S_FILES
    sys/views/p_s/host_summary.sql
    sys/views/p_s/host_summary_57.sql
    sys/views/p_s/host_summary_by_file_io.sql
    sys/views/p_s/host_summary_by_file_io_type.sql
    sys/views/p_s/host_summary_by_stages.sql
    sys/views/p_s/host_summary_by_statement_latency.sql
    sys/views/p_s/host_summary_by_statement_type.sql
    sys/views/p_s/io_by_thread_by_latency.sql
    sys/views/p_s/io_global_by_file_by_bytes.sql
    sys/views/p_s/io_global_by_file_by_latency.sql
    sys/views/p_s/io_global_by_wait_by_bytes.sql
    sys/views/p_s/io_global_by_wait_by_latency.sql
    sys/views/p_s/latest_file_io.sql
    sys/views/p_s/memory_by_user.sql
    sys/views/p_s/memory_global_by_current_allocated.sql
    sys/views/p_s/memory_global_total.sql
    sys/views/p_s/processlist.sql
    sys/views/p_s/processlist_57.sql
    sys/views/p_s/ps_check_lost_instrumentation.sql
    sys/views/p_s/schema_index_statistics.sql
    sys/views/p_s/schema_table_statistics.sql
    sys/views/p_s/schema_table_statistics_with_buffer.sql
    sys/views/p_s/schema_tables_with_full_table_scans.sql
    sys/views/p_s/schema_unused_indexes.sql
    sys/views/p_s/statement_analysis.sql
    sys/views/p_s/statements_with_errors_or_warnings.sql
    sys/views/p_s/statements_with_full_table_scans.sql
    sys/views/p_s/statements_with_runtimes_in_95th_percentile.sql
    sys/views/p_s/statements_with_sorting.sql
    sys/views/p_s/statements_with_temp_tables.sql
    sys/views/p_s/user_summary.sql
    sys/views/p_s/user_summary_57.sql
    sys/views/p_s/user_summary_by_file_io.sql
    sys/views/p_s/user_summary_by_file_io_type.sql
    sys/views/p_s/user_summary_by_stages.sql
    sys/views/p_s/user_summary_by_statement_latency.sql
    sys/views/p_s/user_summary_by_statement_type.sql
    sys/views/p_s/wait_classes_global_by_avg_latency.sql
    sys/views/p_s/wait_classes_global_by_latency.sql
    sys/views/p_s/waits_by_host_by_latency.sql
    sys/views/p_s/waits_by_user_by_latency.sql
    sys/views/p_s/waits_global_by_latency.sql
    sys/views/p_s/memory_by_host_by_current_bytes.sql
	sys/views/p_s/memory_by_thread_by_current_bytes.sql
	sys/views/p_s/memory_by_user_by_current_bytes.sql
	sys/views/p_s/memory_global_by_current_bytes.sql
	sys/views/p_s/x_host_summary.sql
	sys/views/p_s/x_host_summary_57.sql
	sys/views/p_s/x_host_summary_by_file_io.sql
	sys/views/p_s/x_host_summary_by_file_io_type.sql
	sys/views/p_s/x_host_summary_by_stages.sql
	sys/views/p_s/x_host_summary_by_statement_latency.sql
	sys/views/p_s/x_host_summary_by_statement_type.sql
	sys/views/p_s/x_io_by_thread_by_latency.sql
	sys/views/p_s/x_io_global_by_file_by_bytes.sql
	sys/views/p_s/x_io_global_by_file_by_latency.sql
	sys/views/p_s/x_io_global_by_wait_by_bytes.sql
	sys/views/p_s/x_io_global_by_wait_by_latency.sql
	sys/views/p_s/x_latest_file_io.sql
	sys/views/p_s/x_memory_by_host_by_current_bytes.sql
	sys/views/p_s/x_memory_by_thread_by_current_bytes.sql
	sys/views/p_s/x_memory_by_user_by_current_bytes.sql
	sys/views/p_s/x_memory_global_by_current_bytes.sql
	sys/views/p_s/x_memory_global_total.sql
	sys/views/p_s/x_processlist.sql
	sys/views/p_s/x_processlist_57.sql
	sys/views/p_s/x_ps_digest_95th_percentile_by_avg_us.sql
	sys/views/p_s/x_ps_digest_avg_latency_distribution.sql
	sys/views/p_s/x_ps_schema_table_statistics_io.sql
	sys/views/p_s/x_schema_index_statistics.sql
	sys/views/p_s/x_schema_table_statistics.sql
	sys/views/p_s/x_schema_table_statistics_with_buffer.sql
	sys/views/p_s/x_schema_tables_with_full_table_scans.sql
	sys/views/p_s/x_statement_analysis.sql
	sys/views/p_s/x_statements_with_errors_or_warnings.sql
	sys/views/p_s/x_statements_with_full_table_scans.sql
	sys/views/p_s/x_statements_with_runtimes_in_95th_percentile.sql
	sys/views/p_s/x_statements_with_sorting.sql
	sys/views/p_s/x_statements_with_temp_tables.sql
	sys/views/p_s/x_user_summary.sql
	sys/views/p_s/x_user_summary_57.sql
	sys/views/p_s/x_user_summary_by_file_io.sql
	sys/views/p_s/x_user_summary_by_file_io_type.sql
	sys/views/p_s/x_user_summary_by_stages.sql
	sys/views/p_s/x_user_summary_by_statement_latency.sql
	sys/views/p_s/x_user_summary_by_statement_type.sql
	sys/views/p_s/x_wait_classes_global_by_avg_latency.sql
	sys/views/p_s/x_wait_classes_global_by_latency.sql
	sys/views/p_s/x_waits_by_host_by_latency.sql
	sys/views/p_s/x_waits_by_user_by_latency.sql
	sys/views/p_s/x_waits_global_by_latency.sql
)

install(FILES ${SYS_VIEWS_P_S_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/sys/views/p_s)

set(SYS_PROCEDURES_FILES
    sys/procedures/create_synonym_db.sql
    sys/procedures/ps_setup_disable_background_threads.sql
    sys/procedures/ps_setup_disable_consumers.sql
    sys/procedures/ps_setup_disable_instrument.sql
    sys/procedures/ps_setup_disable_thread.sql
    sys/procedures/ps_setup_enable_background_threads.sql
    sys/procedures/ps_setup_enable_consumers.sql
    sys/procedures/ps_setup_enable_instrument.sql
    sys/procedures/ps_setup_enable_thread.sql
    sys/procedures/ps_setup_reload_saved.sql
    sys/procedures/ps_setup_reset_to_default.sql
    sys/procedures/ps_setup_reset_to_default_57.sql
    sys/procedures/ps_setup_save.sql
    sys/procedures/ps_setup_show_disabled.sql
    sys/procedures/ps_setup_show_disabled_consumers.sql
    sys/procedures/ps_setup_show_disabled_instruments.sql
    sys/procedures/ps_setup_show_enabled.sql
    sys/procedures/ps_setup_show_enabled_consumers.sql
    sys/procedures/ps_setup_show_enabled_instruments.sql
    sys/procedures/ps_statement_avg_latency_histogram.sql
    sys/procedures/ps_trace_statement_digest.sql
    sys/procedures/ps_trace_thread.sql
    sys/procedures/ps_truncate_all_tables.sql
    sys/procedures/ps_setup_disable_consumer.sql
	sys/procedures/ps_setup_enable_consumer.sql
	sys/procedures/ps_setup_reset_to_default_57_after.sql
	sys/procedures/ps_setup_reset_to_default_57_before.sql
)

install(FILES ${SYS_PROCEDURES_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/sys/procedures)

set(SYS_VIEWS_I_S_FILES
    sys/views/i_s/innodb_buffer_stats_by_schema.sql
    sys/views/i_s/innodb_buffer_stats_by_table.sql
    sys/views/i_s/schema_object_overview.sql
    sys/views/i_s/innodb_lock_waits.sql
	sys/views/i_s/x_innodb_buffer_stats_by_schema.sql
	sys/views/i_s/x_innodb_buffer_stats_by_table.sql
	sys/views/i_s/x_innodb_lock_waits.sql
)

install(FILES ${SYS_VIEWS_I_S_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/sys/views/i_s)

set(SYS_TABLES_FILES
	sys/tables/sys_config.sql
	sys/tables/sys_config_data.sql
)

install(FILES ${SYS_TABLES_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/sys/tables)

set(SYS_TRIGGERS_FILES
	sys/triggers/sys_config_insert_set_user.sql
	sys/triggers/sys_config_update_set_user.sql
)

install(FILES ${SYS_TRIGGERS_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/sys/triggers)
# SYS_END
