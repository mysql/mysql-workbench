import raw_opts
import raw_vars
import options_layout
import pprint
from variable_column_widths import variable_column_widths

print(('-----------------------------------\nRunning %s\n-----------------------------------\n' % __file__))

print("Converting raw_opts.py to opts.py")


def hack_option(option):
    if option['name'] == 'validate_password_dictionary_file':
        option['values'][0]['type'] = 'fileedit'
        default = option['values'][0].get('default')
        if not default:
             option['values'][0]['default'] = '{configdir}/mysql_password_dictionary.txt'
    elif option['name'] == 'ft_stopword_file':
        option['values'][0]['type'] = 'fileedit'
        default = option['values'][0].get('default')
        if not default:
            option['values'][0]['default'] = '{configdir}/mysql_ft_stopword_file.txt'
    elif option['name'] == 'socket':
        option['values'][0]['type'] = 'filebrowse'
    elif option['name'] == 'ndb-recv-thread-cpu-mask':
        option['values'][0]['type'] = 'string'
    elif option['name'] == 'innodb_data_file_path':
        option['values'][0]['type'] = 'string'
    elif option['name'] in ('slow_query_log_file', 'general_log_file', 'log_bin_basename', 'relay_log_basename',
                            'relay_log_info_file', ''):
        option['values'][0]['type'] = 'filedownload'
    elif option['name'] in ('plugin_dir', ):
        for x in option['values']:
            x['default'] = x['default'].replace('BASEDIR/', '{basedir}/')


option_dict = {}
for option in raw_opts.ropts:
    option_dict[option['name']] = option


sys_vars = {}
stat_vars = {}

def find_var(varlist, option):
    for v in varlist:
        if v[0] == option:
            return v
    return None


print("Generating opts.py")
out=open("opts.py", "w+")
out.write("opts_list = [\n")
handled_options = set()
for section, section_items in options_layout.layout:
    out.write("( '%s', [\n" % section)
    for g, (group, group_items) in enumerate(section_items):
        out.write("\t( '%s', [\n" % group)
        for i, option in enumerate(group_items):
            if option in handled_options:
                print(("Option already handled: %s" % option))
            elif option not in option_dict:
                print(("Option not in dictionalry: %s" % option))
            else:
            #if option in option_dict and not option in handled_options:
                handled_options.add(option)
                info = option_dict[option]
                hack_option(info)
                out.write("\t\t"+repr(info))
                if i < len(group_items)-1:
                    out.write(",\n")
        out.write("])")
        if g < len(section_items)-1:
            out.write(",\n")
    out.write("], %s" % str(variable_column_widths[section]))
    out.write("),\n")
out.write("]\n")

unhandled_options = set(option_dict.keys()) - handled_options
if unhandled_options:
    print("The following options were not handled:", "\n".join(sorted(list(unhandled_options))))

out.close()

def normalize_dict_keys(d):
    return d
    out = {}
    for k, v in list(d.items()):
        out[k.replace("-","_")] = v
    return out

from variable_groups import variable_groups
variable_groups = normalize_dict_keys(dict(variable_groups))

system_var_list = []
for var in raw_vars.system_vars_list:
    current_var = (var['name'], var['description'], var['dynamic'], variable_groups.get(var['name'].replace("-","_"), []))
    system_var_list.append(current_var)


from status_groups import variable_groups
status_groups = normalize_dict_keys(dict(variable_groups))

status_var_list = []
for var in raw_vars.status_vars_list:
    current_var = (var['name'], var['description'], var['dynamic'], status_groups.get(var['name'].replace("-","_"), []))
    #status_var_list.append(tuple(current_var + [status_groups.get(var['name'].replace("-","_"), [])]))
    status_var_list.append(current_var)

print("Generating wb_admin_variable_list.py")
out=open("wb_admin_variable_list.py", "w+")

pp = pprint.PrettyPrinter(indent=2, stream=out)
out.write("system_variable_list = ")
pp.pprint(system_var_list)
out.write("\n\n\n")
out.write("status_variable_list = ")
pp.pprint(status_var_list)
# Add persistable variables
out.write("""

ro_persistable = ['audit_log_buffer_size', 'audit_log_current_session', 'audit_log_file', 'audit_log_filter_id',
                  'audit_log_format', 'audit_log_policy', 'audit_log_strategy', 'auto_generate_certs', 'back_log',
                  'basedir', 'binlog_gtid_simple_recovery', 'bind_address', 'character_sets_dir', 'character_set_system',
                  'core_file', 'daemon_memcached_enable_binlog', 'daemon_memcached_engine_lib_name', 'daemon_memcached_engine_lib_path',
                  'daemon_memcached_r_batch_size', 'daemon_memcached_w_batch_size', 'datadir', 'date_format', 'datetime_format',
                  'default_authentication_plugin', 'disabled_storage_engines', 'ft_max_word_len', 'ft_min_word_len',
                  'ft_query_expansion_limit', 'ft_stopword_file', 'have_compress', 'have_crypt', 'have_dynamic_loading',
                  'have_geometry', 'have_openssl', 'have_profiling', 'have_query_cache', 'have_rtree_keys', 'have_ssl',
                  'have_statement_timeout', 'have_symlink', 'hostname', 'ignore_builtin_innodb', 'init_file',
                  'innodb_adaptive_hash_index_parts', 'innodb_api_disable_rowlock', 'innodb_api_enable_binlog',
                  'innodb_api_enable_mdl', 'innodb_autoinc_lock_mode', 'innodb_buffer_pool_chunk_size',
                  'innodb_buffer_pool_debug', 'innodb_buffer_pool_instances', 'innodb_buffer_pool_load_at_startup',
                  'innodb_data_file_path', 'innodb_data_home_dir', 'innodb_doublewrite', 'innodb_flush_method',
                  'innodb_force_load_corrupted', 'innodb_force_recovery', 'innodb_ft_cache_size', 'innodb_ft_max_token_size',
                  'innodb_ft_min_token_size', 'innodb_ft_sort_pll_degree', 'innodb_ft_total_cache_size',
                  'innodb_log_buffer_size', 'innodb_log_file_size', 'innodb_log_files_in_group', 'innodb_log_group_home_dir',
                  'innodb_numa_interleave', 'innodb_open_files', 'innodb_page_cleaners', 'innodb_page_size',
                  'innodb_purge_threads', 'innodb_read_io_threads', 'innodb_read_only', 'innodb_rollback_on_timeout',
                  'innodb_scan_directories', 'innodb_sort_buffer_size', 'innodb_sync_array_size', 'innodb_sync_debug',
                  'innodb_temp_data_file_path', 'innodb_undo_directory', 'innodb_undo_tablespaces', 'innodb_use_native_aio',
                  'innodb_version', 'innodb_write_io_threads', 'language', 'large_files_support', 'large_pages',
                  'large_page_size', 'lc_messages_dir', 'license', 'log_error', 'locked_in_memory', 'log_bin',
                  'log_bin_basename', 'log_bin_index', 'log_bin_use_v1_row_events', 'log_slave_updates',
                  'lower_case_file_system', 'lower_case_table_names', 'max_digest_length', 'mecab_rc_file',
                  'metadata_locks_cache_size', 'metadata_locks_hash_instances', 'myisam_mmap_size',
                  'myisam_recover_options', 'mysqlx_bind_address', 'mysqlx_port', 'mysqlx_port_open_timeout',
                  'mysqlx_socket', 'mysqlx_ssl', 'mysqlx_ssl_ca', 'mysqlx_ssl_capath', 'mysqlx_ssl_cert',
                  'mysqlx_ssl_cipher', 'mysqlx_ssl_crl', 'mysqlx_ssl_crlpath', 'mysqlx_ssl_key', 'named_pipe',
                  'ndb_batch_size', 'ndb_cluster_connection_pool', 'ndb_cluster_connection_pool_nodeids',
                  'ndb_log_apply_status', 'ndb_log_transaction_id', 'ndb_optimized_node_selection',
                  'Ndb_slave_max_replicated_epoch', 'ndb_version', 'ndb_version_string', 'ndb_wait_setup',
                  'ndbinfo_database', 'ndbinfo_version', 'ngram_token_size', 'old', 'open_files_limit',
                  'performance_schema', 'performance_schema_accounts_size', 'performance_schema_digests_size',
                  'performance_schema_error_size', 'performance_schema_events_stages_history_long_size',
                  'performance_schema_events_stages_history_size', 'performance_schema_events_statements_history_long_size',
                  'performance_schema_events_statements_history_size', 'performance_schema_events_transactions_history_long_size',
                  'performance_schema_events_transactions_history_size', 'performance_schema_events_waits_history_long_size',
                  'performance_schema_events_waits_history_size', 'performance_schema_hosts_size',
                  'performance_schema_max_cond_classes', 'performance_schema_max_cond_instances',
                  'performance_schema_max_digest_length', 'performance_schema_max_file_classes',
                  'performance_schema_max_file_handles', 'performance_schema_max_file_instances',
                  'performance_schema_max_index_stat', 'performance_schema_max_memory_classes', 'performance_schema_max_metadata_locks',
                  'performance_schema_max_mutex_classes', 'performance_schema_max_mutex_instances',
                  'performance_schema_max_prepared_statements_instances', 'performance_schema_max_program_instances',
                  'performance_schema_max_rwlock_classes', 'performance_schema_max_rwlock_instances',
                  'performance_schema_max_socket_classes', 'performance_schema_max_socket_instances',
                  'performance_schema_max_sql_text_length', 'performance_schema_max_stage_classes',
                  'performance_schema_max_statement_classes', 'performance_schema_max_statement_stack',
                  'performance_schema_max_table_handles', 'performance_schema_max_table_instances',
                  'performance_schema_max_table_lock_stat', 'performance_schema_max_thread_classes',
                  'performance_schema_max_thread_instances', 'performance_schema_session_connect_attrs_size',
                  'performance_schema_setup_actors_size', 'performance_schema_setup_objects_size',
                  'performance_schema_users_size', 'persisted_globals_load', 'pid_file', 'plugin_dir',
                  'port', 'protocol_version', 'relay_log', 'relay_log_basename', 'relay_log_index',
                  'relay_log_info_file', 'relay_log_recovery', 'relay_log_space_limit', 'server_uuid',
                  'sha256_password_auto_generate_rsa_keys', 'sha256_password_private_key_path', 'sha256_password_public_key_path',
                  'shared_memory', 'shared_memory_base_name', 'skip_external_locking', 'skip_name_resolve',
                  'skip_networking', 'skip_show_database', 'slave_load_tmpdir', 'slave_type_conversions',
                  'socket', 'ssl_ca', 'ssl_capath', 'ssl_cert', 'ssl_cipher', 'ssl_crl', 'ssl_crlpath',
                  'ssl_key', 'system_time_zone', 'table_open_cache_instances', 'thread_handling', 'thread_stack',
                  'time_format', 'tls_version', 'tmpdir', 'version', 'version_comment', 'version_compile_machine',
                  'version_compile_os', 'version_tokens_session_number', 'report_host', 'report_port',
                  'report_password', 'report_user', 'daemon_memcached_option', '',
                  'From the above list we will allow following list of variables to be persisted.',
                  'audit_log_buffer_size', 'audit_log_policy', 'audit_log_strategy', 'back_log',
                  'binlog_gtid_simple_recovery', 'daemon_memcached_enable_binlog', 'daemon_memcached_r_batch_size',
                  'daemon_memcached_w_batch_size', 'date_format', 'datetime_format', 'disabled_storage_engines',
                  'ft_max_word_len', 'ft_min_word_len', 'ft_query_expansion_limit', 'ignore_builtin_innodb',
                  'innodb_adaptive_hash_index_parts', 'innodb_api_disable_rowlock', 'innodb_api_enable_binlog',
                  'innodb_api_enable_mdl', 'innodb_autoinc_lock_mode', 'innodb_buffer_pool_chunk_size',
                  'innodb_buffer_pool_debug', 'innodb_buffer_pool_instances', 'innodb_doublewrite',
                  'innodb_flush_method', 'innodb_force_recovery', 'innodb_ft_cache_size', 'innodb_ft_max_token_size',
                  'innodb_ft_min_token_size', 'innodb_ft_sort_pll_degree', 'innodb_ft_total_cache_size',
                  'innodb_log_buffer_size', 'innodb_log_file_size', 'innodb_log_files_in_group', 'innodb_numa_interleave',
                  'innodb_open_files', 'innodb_page_cleaners', 'innodb_purge_threads', 'innodb_read_io_threads',
                  'innodb_rollback_on_timeout', 'innodb_sort_buffer_size', 'innodb_sync_array_size',
                  'innodb_sync_debug', 'innodb_use_native_aio', 'innodb_write_io_threads', 'large_pages',
                  'log_slave_updates', 'lower_case_table_names', 'max_digest_length', 'metadata_locks_cache_size',
                  'metadata_locks_hash_instances', 'myisam_mmap_size', 'myisam_recover_options', 'mysqlx_bind_address',
                  'mysqlx_port', 'mysqlx_port_open_timeout', 'mysqlx_socket', 'mysqlx_ssl', 'mysqlx_ssl_ca',
                  'mysqlx_ssl_capath', 'mysqlx_ssl_cert', 'mysqlx_ssl_cipher', 'mysqlx_ssl_crl', 'mysqlx_ssl_crlpath',
                  'mysqlx_ssl_key', 'ndb_batch_size', 'ndb_cluster_connection_pool', 'ndb_cluster_connection_pool_nodeids',
                  'ndb_log_apply_status', 'ndb_log_transaction_id', 'ndb_optimized_node_selection',
                  'Ndb_slave_max_replicated_epoch', 'ndb_wait_setup', 'ndbinfo_database', 'ngram_token_size',
                  'old', 'open_files_limit', 'performance_schema', 'performance_schema_accounts_size',
                  'performance_schema_digests_size', 'performance_schema_error_size', 'performance_schema_events_stages_history_long_size',
                  'performance_schema_events_stages_history_size', 'performance_schema_events_statements_history_long_size',
                  'performance_schema_events_statements_history_size', 'performance_schema_events_transactions_history_long_size',
                  'performance_schema_events_transactions_history_size', 'performance_schema_events_waits_history_long_size',
                  'performance_schema_events_waits_history_size', 'performance_schema_hosts_size',
                  'performance_schema_max_cond_classes', 'performance_schema_max_cond_instances', 'performance_schema_max_digest_length',
                  'performance_schema_max_file_classes', 'performance_schema_max_file_handles', 'performance_schema_max_file_instances',
                  'performance_schema_max_index_stat', 'performance_schema_max_memory_classes', 'performance_schema_max_metadata_locks',
                  'performance_schema_max_mutex_classes', 'performance_schema_max_mutex_instances',
                  'performance_schema_max_prepared_statements_instances', 'performance_schema_max_program_instances',
                  'performance_schema_max_rwlock_classes', 'performance_schema_max_rwlock_instances',
                  'performance_schema_max_socket_classes', 'performance_schema_max_socket_instances',
                  'performance_schema_max_sql_text_length', 'performance_schema_max_stage_classes',
                  'performance_schema_max_statement_classes', 'performance_schema_max_statement_stack',
                  'performance_schema_max_table_handles', 'performance_schema_max_table_instances',
                  'performance_schema_max_table_lock_stat', 'performance_schema_max_thread_classes',
                  'performance_schema_max_thread_instances', 'performance_schema_session_connect_attrs_size',
                  'performance_schema_setup_actors_size', 'performance_schema_setup_objects_size',
                  'performance_schema_users_size', 'relay_log_recovery', 'relay_log_space_limit',
                  'skip_name_resolve', 'skip_show_database', 'slave_type_conversions', 'ssl_cipher',
                  'table_open_cache_instances', 'thread_handling', 'thread_stack', 'time_format',
                  'tls_version', 'report_host', 'report_port', 'report_password', 'report_user']
    """)
out.close()

