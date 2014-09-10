-- Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; version 2 of the License.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

SOURCE ./before_setup.sql

SOURCE ./functions/extract_schema_from_file_name.sql
SOURCE ./functions/extract_table_from_file_name.sql
SOURCE ./functions/format_bytes.sql
SOURCE ./functions/format_path.sql
SOURCE ./functions/format_statement.sql
SOURCE ./functions/format_time.sql
SOURCE ./functions/ps_is_account_enabled.sql
SOURCE ./functions/ps_is_instrument_default_enabled.sql
SOURCE ./functions/ps_is_instrument_default_timed.sql
SOURCE ./functions/ps_thread_id.sql
SOURCE ./functions/ps_thread_stack.sql

SOURCE ./procedures/create_synonym_db.sql

SOURCE ./procedures/ps_statement_avg_latency_histogram.sql
SOURCE ./procedures/ps_trace_statement_digest.sql
SOURCE ./procedures/ps_trace_thread.sql

SOURCE ./procedures/ps_setup_disable_background_threads.sql
SOURCE ./procedures/ps_setup_disable_consumers.sql
SOURCE ./procedures/ps_setup_disable_instrument.sql
SOURCE ./procedures/ps_setup_disable_thread.sql

SOURCE ./procedures/ps_setup_enable_background_threads.sql
SOURCE ./procedures/ps_setup_enable_consumers.sql
SOURCE ./procedures/ps_setup_enable_instrument.sql
SOURCE ./procedures/ps_setup_enable_thread.sql

SOURCE ./procedures/ps_setup_reload_saved.sql
SOURCE ./procedures/ps_setup_reset_to_default.sql
SOURCE ./procedures/ps_setup_save.sql
SOURCE ./procedures/ps_setup_show_disabled.sql
SOURCE ./procedures/ps_setup_show_disabled_consumers.sql
SOURCE ./procedures/ps_setup_show_disabled_instruments.sql
SOURCE ./procedures/ps_setup_show_enabled.sql
SOURCE ./procedures/ps_setup_show_enabled_consumers.sql
SOURCE ./procedures/ps_setup_show_enabled_instruments.sql
SOURCE ./procedures/ps_truncate_all_tables.sql

SOURCE ./views/i_s/innodb_buffer_stats_by_schema.sql
SOURCE ./views/i_s/innodb_buffer_stats_by_table.sql
SOURCE ./views/i_s/schema_object_overview.sql

SOURCE ./views/p_s/ps_check_lost_instrumentation.sql
SOURCE ./views/p_s/processlist.sql

SOURCE ./views/p_s/latest_file_io.sql
SOURCE ./views/p_s/io_by_thread_by_latency.sql
SOURCE ./views/p_s/io_global_by_file_by_bytes.sql
SOURCE ./views/p_s/io_global_by_file_by_latency.sql
SOURCE ./views/p_s/io_global_by_wait_by_bytes.sql
SOURCE ./views/p_s/io_global_by_wait_by_latency.sql

SOURCE ./views/p_s/schema_index_statistics.sql
SOURCE ./views/p_s/schema_table_statistics.sql
SOURCE ./views/p_s/schema_table_statistics_with_buffer.sql
SOURCE ./views/p_s/schema_tables_with_full_table_scans.sql
SOURCE ./views/p_s/schema_unused_indexes.sql

SOURCE ./views/p_s/statement_analysis.sql
SOURCE ./views/p_s/statements_with_errors_or_warnings.sql
SOURCE ./views/p_s/statements_with_full_table_scans.sql
SOURCE ./views/p_s/statements_with_runtimes_in_95th_percentile.sql
SOURCE ./views/p_s/statements_with_sorting.sql
SOURCE ./views/p_s/statements_with_temp_tables.sql

SOURCE ./views/p_s/user_summary_by_file_io_type.sql
SOURCE ./views/p_s/user_summary_by_file_io.sql
SOURCE ./views/p_s/user_summary_by_statement_type.sql
SOURCE ./views/p_s/user_summary_by_statement_latency.sql
SOURCE ./views/p_s/user_summary_by_stages.sql
SOURCE ./views/p_s/user_summary.sql

SOURCE ./views/p_s/host_summary_by_file_io_type.sql
SOURCE ./views/p_s/host_summary_by_file_io.sql
SOURCE ./views/p_s/host_summary_by_statement_type.sql
SOURCE ./views/p_s/host_summary_by_statement_latency.sql
SOURCE ./views/p_s/host_summary_by_stages.sql
SOURCE ./views/p_s/host_summary.sql

SOURCE ./views/p_s/wait_classes_global_by_avg_latency.sql
SOURCE ./views/p_s/wait_classes_global_by_latency.sql
SOURCE ./views/p_s/waits_by_user_by_latency.sql
SOURCE ./views/p_s/waits_by_host_by_latency.sql
SOURCE ./views/p_s/waits_global_by_latency.sql

SOURCE ./after_setup.sql

