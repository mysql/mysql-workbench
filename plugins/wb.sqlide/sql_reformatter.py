# Copyright (c) 2012, 2016, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; version 2 of the
# License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301  USA

import string
import grt

INDENTATION = "    "

IDENT_CHARS = string.ascii_letters + string.digits + "_"



KEYWORD_TOKENS = set( ['ACCESSIBLE_SYM', 'ACTION', 'ADD', 'ADDDATE_SYM', 'AFTER_SYM', 'AGAINST', 'AGGREGATE_SYM', 'ALGORITHM_SYM', 'ALL', 'ALTER', 'ANALYZE_SYM', 'AND_AND_SYM', 'AND_SYM', 'ANY_SYM', 'AS', 'ASC', 'ASCII_SYM', 'AT_SYM', 'AUTHORS_SYM', 'AUTOEXTEND_SIZE_SYM', 'AUTO_INC', 'AVG_ROW_LENGTH', 'AVG_SYM', 'BACKUP_SYM', 'BEFORE_SYM', 'BEGIN_SYM', 'BETWEEN_SYM', 'BIGINT', 'BINARY', 'BINLOG_SYM', 'BIN_NUM', 'BIT_AND', 'BIT_OR', 'BIT_SYM', 'BIT_XOR', 'BLOB_SYM', 'BLOCK_SYM', 'BOOLEAN_SYM', 'BOOL_SYM', 'BOTH', 'BTREE_SYM', 'BY', 'BYTE_SYM', 'CACHE_SYM', 'CALL_SYM', 'CASCADE', 'CASCADED', 'CASE_SYM', 'CAST_SYM', 'CATALOG_NAME_SYM', 'CHAIN_SYM', 'CHANGE', 'CHANGED', 'CHARSET', 'CHAR_SYM', 'CHECKSUM_SYM', 'CHECK_SYM', 'CIPHER_SYM', 'CLASS_ORIGIN_SYM', 'CLIENT_SYM', 'CLOSE_SYM', 'COALESCE', 'CODE_SYM', 'COLLATE_SYM', 'COLLATION_SYM', 'COLUMNS', 'COLUMN_NAME_SYM', 'COLUMN_SYM', 'COMMENT_SYM', 'COMMITTED_SYM', 'COMMIT_SYM', 'COMPACT_SYM', 'COMPLETION_SYM', 'COMPRESSED_SYM', 'CONCURRENT', 'CONDITION_SYM', 'CONNECTION_SYM', 'CONSISTENT_SYM', 'CONSTRAINT', 'CONSTRAINT_CATALOG_SYM', 'CONSTRAINT_NAME_SYM', 'CONSTRAINT_SCHEMA_SYM', 'CONTAINS_SYM', 'CONTEXT_SYM', 'CONTINUE_SYM', 'CONTRIBUTORS_SYM', 'CONVERT_SYM', 'COUNT_SYM', 'CPU_SYM', 'CREATE', 'CROSS', 'CUBE_SYM', 'CURDATE', 'CURRENT_USER', 'CURSOR_NAME_SYM', 'CURSOR_SYM', 'CURTIME', 'DATABASE', 'DATABASES', 'DATAFILE_SYM', 'DATA_SYM', 'DATETIME', 'DATE_ADD_INTERVAL', 'DATE_SUB_INTERVAL', 'DATE_SYM', 'DAY_HOUR_SYM', 'DAY_MICROSECOND_SYM', 'DAY_MINUTE_SYM', 'DAY_SECOND_SYM', 'DAY_SYM', 'DEALLOCATE_SYM', 'DECIMAL_NUM', 'DECIMAL_SYM', 'DECLARE_SYM', 'DEFAULT', 'DEFINER_SYM', 'DELAYED_SYM', 'DELAY_KEY_WRITE_SYM', 'DELETE_SYM', 'DESC', 'DESCRIBE', 'DES_KEY_FILE', 'DETERMINISTIC_SYM', 'DIRECTORY_SYM', 'DISABLE_SYM', 'DISCARD', 'DISK_SYM', 'DISTINCT', 'DIV_SYM', 'DOUBLE_SYM', 'DO_SYM', 'DROP', 'DUAL_SYM', 'DUMPFILE', 'DUPLICATE_SYM', 'DYNAMIC_SYM', 'EACH_SYM', 'EDIT_SYM', 'ELSE', 'ELSEIF_SYM', 'ENABLE_SYM', 'ENCLOSED', 'END', 'ENDS_SYM', 'END_OF_INPUT', 'ENGINES_SYM', 'ENGINE_SYM', 'ENUM', 'EQ', 'EQUAL_SYM', 'ERRORS', 'ERROR_SYM', 'ESCAPED', 'ESCAPE_SYM', 'EVENTS_SYM', 'EVENT_SYM', 'EVERY_SYM', 'EXECUTE_SYM', 'EXISTS', 'EXIT_SYM', 'EXPANSION_SYM', 'EXTENDED_SYM', 'EXTENT_SIZE_SYM', 'EXTRACT_SYM', 'FALSE_SYM', 'FAST_SYM', 'FAULTS_SYM', 'FETCH_SYM', 'FILE_SYM', 'FIRST_SYM', 'FIXED_SYM', 'FLOAT_NUM', 'FLOAT_SYM', 'FLUSH_SYM', 'FORCE_SYM', 'FOREIGN', 'FOR_SYM', 'FOUND_SYM', 'FROM', 'FULL', 'FULLTEXT_SYM', 'FUNCTION_SYM', 'GE', 'GENERAL', 'GEOMETRYCOLLECTION', 'GEOMETRY_SYM', 'GET_FORMAT', 'GLOBAL_SYM', 'GRANT', 'GRANTS', 'GROUP_CONCAT_SYM', 'GROUP_SYM', 'GT_SYM', 'HANDLER_SYM', 'HASH_SYM', 'HAVING', 'HELP_SYM', 'HEX_NUM', 'HIGH_PRIORITY', 'HOSTS_SYM', 'HOST_SYM', 'HOUR_MICROSECOND_SYM', 'HOUR_MINUTE_SYM', 'HOUR_SECOND_SYM', 'HOUR_SYM', 'IDENT', 'IDENTIFIED_SYM', 'IDENT_QUOTED', 'IF', 'IGNORE_SERVER_IDS_SYM', 'IGNORE_SYM', 'IMPORT', 'INDEXES', 'INDEX_SYM', 'INFILE', 'INITIAL_SIZE_SYM', 'INNER_SYM', 'INOUT_SYM', 'INSERT', 'INSERT_METHOD', 'INSTALL_SYM', 'INTERVAL_SYM', 'INTO', 'INT_SYM', 'INVOKER_SYM', 'IN_SYM', 'IO_SYM', 'IPC_SYM', 'IS', 'ISOLATION', 'ISSUER_SYM', 'ITERATE_SYM', 'JOIN_SYM', 'KEYS', 'KEY_BLOCK_SIZE', 'KEY_SYM', 'KILL_SYM', 'LANGUAGE_SYM', 'LAST_SYM', 'LE', 'LEADING', 'LEAVES', 'LEAVE_SYM', 'LEFT', 'LESS_SYM', 'LEVEL_SYM', 'LEX_HOSTNAME', 'LIKE', 'LIMIT', 'LINEAR_SYM', 'LINES', 'LINESTRING', 'LIST_SYM', 'LOAD', 'LOCAL_SYM', 'LOCKS_SYM', 'LOCK_SYM', 'LOGFILE_SYM', 'LOGS_SYM', 'LONGBLOB', 'LONGTEXT', 'LONG_NUM', 'LONG_SYM', 'LOOP_SYM', 'LOW_PRIORITY', 'LT', 'MASTER_CONNECT_RETRY_SYM', 'MASTER_HEARTBEAT_PERIOD_SYM', 'MASTER_HOST_SYM', 'MASTER_LOG_FILE_SYM', 'MASTER_LOG_POS_SYM', 'MASTER_PASSWORD_SYM', 'MASTER_PORT_SYM', 'MASTER_SERVER_ID_SYM', 'MASTER_SSL_CAPATH_SYM', 'MASTER_SSL_CA_SYM', 'MASTER_SSL_CERT_SYM', 'MASTER_SSL_CIPHER_SYM', 'MASTER_SSL_KEY_SYM', 'MASTER_SSL_SYM', 'MASTER_SSL_VERIFY_SERVER_CERT_SYM', 'MASTER_SYM', 'MASTER_USER_SYM', 'MATCH', 'MAX_CONNECTIONS_PER_HOUR', 'MAX_QUERIES_PER_HOUR', 'MAX_ROWS', 'MAX_SIZE_SYM', 'MAX_SYM', 'MAX_UPDATES_PER_HOUR', 'MAX_USER_CONNECTIONS_SYM', 'MAX_VALUE_SYM', 'MEDIUMBLOB', 'MEDIUMINT', 'MEDIUMTEXT', 'MEDIUM_SYM', 'MEMORY_SYM', 'MERGE_SYM', 'MESSAGE_TEXT_SYM', 'MICROSECOND_SYM', 'MIGRATE_SYM', 'MINUTE_MICROSECOND_SYM', 'MINUTE_SECOND_SYM', 'MINUTE_SYM', 'MIN_ROWS', 'MIN_SYM', 'MODE_SYM', 'MODIFIES_SYM', 'MODIFY_SYM', 'MOD_SYM', 'MONTH_SYM', 'MULTILINESTRING', 'MULTIPOINT', 'MULTIPOLYGON', 'MUTEX_SYM', 'MYSQL_ERRNO_SYM', 'NAMES_SYM', 'NAME_SYM', 'NATIONAL_SYM', 'NATURAL', 'NCHAR_STRING', 'NCHAR_SYM', 'NDBCLUSTER_SYM', 'NE', 'NEG', 'NEW_SYM', 'NEXT_SYM', 'NODEGROUP_SYM', 'NONE_SYM', 'NOT2_SYM', 'NOT_SYM', 'NOW_SYM', 'NO_SYM', 'NO_WAIT_SYM', 'NO_WRITE_TO_BINLOG', 'NULL_SYM', 'NUM', 'NUMERIC_SYM', 'NVARCHAR_SYM', 'OFFSET_SYM', 'OLD_PASSWORD', 'ON', 'ONE_SHOT_SYM', 'ONE_SYM', 'OPEN_SYM', 'OPTIMIZE', 'OPTION', 'OPTIONALLY', 'OPTIONS_SYM', 'OR2_SYM', 'ORDER_SYM', 'OR_OR_SYM', 'OR_SYM', 'OUTER', 'OUTFILE', 'OUT_SYM', 'OWNER_SYM', 'PACK_KEYS_SYM', 'PAGE_SYM', 'PARAM_MARKER', 'PARSER_SYM', 'PARTIAL', 'PARTITIONING_SYM', 'PARTITIONS_SYM', 'PARTITION_SYM', 'PASSWORD', 'PHASE_SYM', 'PLUGINS_SYM', 'PLUGIN_SYM', 'POINT_SYM', 'POLYGON', 'PORT_SYM', 'POSITION_SYM', 'PRECISION', 'PREPARE_SYM', 'PRESERVE_SYM', 'PREV_SYM', 'PRIMARY_SYM', 'PRIVILEGES', 'PROCEDURE_SYM', 'PROCESS', 'PROCESSLIST_SYM', 'PROFILES_SYM', 'PROFILE_SYM', 'PURGE', 'QUARTER_SYM', 'QUERY_SYM', 'QUICK', 'RANGE_SYM', 'READS_SYM', 'READ_ONLY_SYM', 'READ_SYM', 'READ_WRITE_SYM', 'REAL', 'REBUILD_SYM', 'RECOVER_SYM', 'REDOFILE_SYM', 'REDO_BUFFER_SIZE_SYM', 'REDUNDANT_SYM', 'REFERENCES', 'REGEXP', 'RELAY', 'RELAYLOG_SYM', 'RELAY_LOG_FILE_SYM', 'RELAY_LOG_POS_SYM', 'RELAY_THREAD', 'RELEASE_SYM', 'RELOAD', 'REMOVE_SYM', 'RENAME', 'REORGANIZE_SYM', 'REPAIR', 'REPEATABLE_SYM', 'REPEAT_SYM', 'REPLACE', 'REPLICATION', 'REQUIRE_SYM', 'RESET_SYM', 'RESIGNAL_SYM', 'RESOURCES', 'RESTORE_SYM', 'RESTRICT', 'RESUME_SYM', 'RETURNS_SYM', 'RETURN_SYM', 'REVOKE', 'RIGHT', 'ROLLBACK_SYM', 'ROLLUP_SYM', 'ROUTINE_SYM', 'ROWS_SYM', 'ROW_FORMAT_SYM', 'ROW_SYM', 'RTREE_SYM', 'SAVEPOINT_SYM', 'SCHEDULE_SYM', 'SCHEMA_NAME_SYM', 'SECOND_MICROSECOND_SYM', 'SECOND_SYM', 'SECURITY_SYM', 'SELECT_SYM', 'SEPARATOR_SYM', 'SERIALIZABLE_SYM', 'SERIAL_SYM', 'SERVER_SYM', 'SESSION_SYM', 'SET', 'SET_VAR', 'SHARE_SYM', 'SHIFT_LEFT', 'SHIFT_RIGHT', 'SHOW', 'SHUTDOWN', 'SIGNAL_SYM', 'SIGNED_SYM', 'SIMPLE_SYM', 'SLAVE', 'SLOW', 'SMALLINT', 'SNAPSHOT_SYM', 'SOCKET_SYM', 'SONAME_SYM', 'SOUNDS_SYM', 'SOURCE_SYM', 'SPATIAL_SYM', 'SQLEXCEPTION_SYM', 'SQLSTATE_SYM', 'SQLWARNING_SYM', 'SQL_BIG_RESULT', 'SQL_BUFFER_RESULT', 'SQL_CACHE_SYM', 'SQL_CALC_FOUND_ROWS', 'SQL_NO_CACHE_SYM', 'SQL_SMALL_RESULT', 'SQL_SYM', 'SQL_THREAD', 'SSL_SYM', 'STARTING', 'STARTS_SYM', 'START_SYM', 'STATUS_SYM', 'STDDEV_SAMP_SYM', 'STD_SYM', 'STOP_SYM', 'STORAGE_SYM', 'STRAIGHT_JOIN', 'STRING_SYM', 'SUBCLASS_ORIGIN_SYM', 'SUBDATE_SYM', 'SUBJECT_SYM', 'SUBPARTITIONS_SYM', 'SUBPARTITION_SYM', 'SUBSTRING', 'SUM_SYM', 'SUPER_SYM', 'SUSPEND_SYM', 'SWAPS_SYM', 'SWITCHES_SYM', 'SYSDATE', 'TABLES', 'TABLESPACE', 'TABLE_CHECKSUM_SYM', 'TABLE_NAME_SYM', 'TABLE_REF_PRIORITY', 'TABLE_SYM', 'TEMPORARY', 'TEMPTABLE_SYM', 'TERMINATED', 'TEXT_STRING', 'TEXT_SYM', 'THAN_SYM', 'THEN_SYM', 'TIMESTAMP', 'TIMESTAMP_ADD', 'TIMESTAMP_DIFF', 'TIME_SYM', 'TINYBLOB', 'TINYINT', 'TINYTEXT', 'TO_SYM', 'TRAILING', 'TRANSACTION_SYM', 'TRIGGERS_SYM', 'TRIGGER_SYM', 'TRIM', 'TRUE_SYM', 'TRUNCATE_SYM', 'TYPES_SYM', 'TYPE_SYM', 'UDF_RETURNS_SYM', 'ULONGLONG_NUM', 'UNCOMMITTED_SYM', 'UNDEFINED_SYM', 'UNDERSCORE_CHARSET', 'UNDOFILE_SYM', 'UNDO_BUFFER_SIZE_SYM', 'UNICODE_SYM', 'UNINSTALL_SYM', 'UNION_SYM', 'UNIQUE_SYM', 'UNKNOWN_SYM', 'UNLOCK_SYM', 'UNSIGNED', 'UNTIL_SYM', 'UPDATE_SYM', 'UPGRADE_SYM', 'USAGE', 'USER', 'USE_FRM', 'USE_SYM', 'USING', 'UTC_DATE_SYM', 'UTC_TIMESTAMP_SYM', 'UTC_TIME_SYM', 'VALUES', 'VALUE_SYM', 'VARBINARY', 'VARCHAR', 'VARIABLES', 'VARIANCE_SYM', 'VARYING', 'VAR_SAMP_SYM', 'VIEW_SYM', 'WAIT_SYM', 'WARNINGS', 'WEEK_SYM', 'WHEN_SYM', 'WHERE', 'WHILE_SYM', 'WITH', 'WITH_CUBE_SYM', 'WITH_ROLLUP_SYM', 'WORK_SYM', 'WRAPPER_SYM', 'WRITE_SYM', 'X509_SYM', 'XA_SYM', 'XML_SYM', 'XOR', 'YEAR_MONTH_SYM', 'YEAR_SYM', 'ZEROFILL'] )

KEYWORD_ONLY_NODES = set( ['sp_opt_fetch_noise', 'insert_lock_option', 'opt_var_ident_type', 'trg_event', 'opt_ignore', 'opt_ev_status', 'remember_end', 'opt_ignore_leaves', 'param_marker', 'opt_wild', 'show_engine_param', 'profile_def', 'opt_distinct', 'view_algorithm', 'IDENT_sys', 'opt_query_expansion', 'TEXT_STRING_sys', 'text_or_password', 'interval_time_stamp', 'opt_natural_language_mode', 'udf_type', 'fulltext', 'opt_one_phase', 'spatial_type', 'opt_chain', 'nvarchar', 'opt_match_clause', 'ev_on_completion', 'ascii', 'opt_local', 'charset', 'view_suid', 'keys_or_index', 'olap_opt', 'opt_with_read_lock', 'no_definer', 'union_option', 'opt_outer', 'opt_default', 'view_check_option', 'opt_full', 'not', 'ts_access_mode', 'opt_migrate', 'btree_or_rtree', 'opt_temporary', 'opt_no_write_to_binlog', 'sp_suid', 'reset_option', 'describe_command', 'NUM_literal', 'slave_thread_opt', 'option_type2', 'opt_join_or_resume', 'delete_option', 'handler_scan_function', 'keyword_sp', 'and', 'remember_name', 'opt_bin_mod', 'opt_checksum_type', 'opt_all', 'query_expression_option', 'require_list_element', 'not2', 'data_or_xml', 'lines_or_rows', 'part_value_item', 'comp_op', 'init_key_options', 'opt_privileges', 'opt_table_sym', 'subselect_end', 'have_partitioning', 'table_or_tables', 'opt_storage', 'row_types', 'table_alias', 'opt_as', 'get_select_lex', 'opt_var_type', 'normal_join', 'opt_linear', 'TEXT_STRING_literal', 'opt_primary', 'equal', 'nchar', 'dec_num', 'master_or_binary', 'real_type', 'field_length', 'mi_check_type', 'ulonglong_num', 'opt_and', 'trg_action_time', 'precision', 'load_data_lock', 'TEXT_STRING_filesystem', 'isolation_types', 'opt_release', 'sp_init_param', 'signal_condition_information_item_name', 'opt_table', 'select_derived_init', 'opt_extended_describe', 'from_or_in', 'ts_wait', 'unicode', 'opt_work', 'merge_insert_types', 'opt_profile_args', 'sp_handler_type', 'opt_unique', 'mi_repair_type', 'select_lock_type', 'opt_column', 'deallocate_or_drop', 'int_type', 'table_option', 'start_transaction_opts', 'if_exists', 'all_or_any', 'clear_privileges', 'date_time_type', 'or', 'order_dir', 'opt_option', 'field_option', 'opt_savepoint', 'lock_option', 'ulong_num', 'opt_low_priority', 'optional_braces', 'flush_option', 'index_hint_clause', 'remove_partitioning', 'opt_delete_option', 'view_replace', 'key_or_index', 'subselect_start', 'opt_to', 'char', 'index_hint_type', 'spatial', 'opt_value', 'opt_duplicate', 'sp_opt_inout', 'kill_option', 'opt_restrict', 'begin_or_start', 'handler_rkey_mode', 'opt_end_of_input'] )

NON_KEYWORD_TOKENS = set(["ident", "ident_or_text", "TEXT_STRING", "text_string", "TEXT_STRING_filesystem", "TEXT_STRING_literal", "TEXT_STRING_sys",
                                             "part_name"])

STRIP_TOKENS = set(['now'])


def dump_tree(f, ast, depth=0):
    sym, value, children = ast[0], ast[1], ast[2]

    if children:
        f.write("%s<%s, %s>\n" % ("  "*depth, sym, value))
        for c in children:
            dump_tree(f, c, depth+2)
        f.write("%s</%s, %s>\n" % ("  "*depth, sym, value))
    else:
        f.write("%s<%s, %s/>\n" % ("  "*depth, sym, value))


def dump(ast):
    import sys
    dump_tree(sys.stdout, ast)


class ASTHelper:
    def __init__(self, text):
        self.max_range = len(text)

    def get_ast_range(self, ast):
        offset = ast[3]
        b = ast[4]+offset if ast[4] is not None else self.max_range
        e = ast[5]+offset if ast[5] is not None else 0
        for c in ast[2]:
            b_, e_ = self.get_ast_range(c)
            b = min(b_, b)
            e = max(e_, e)
        return b, e      

def trim_ast(node):
    s = node[0]
    v = node[1]
    c = node[2]
    l = []
    for i in c:
        l.append(trim_ast(i))
    return (s, v, l)

def indent(text, count=1):
    return INDENTATION*count + ("\n"+(INDENTATION*count)).join(text.split("\n"))

def indent_head(text, count=1):
    return INDENTATION*count + text

def indent_tail(text, count=1):
    return ("\n"+(INDENTATION*count)).join(text.split("\n"))


def flatten_node_spaced(node):
    return (" ".join([node_value(ch) for ch in node_children(node) if node_value(ch)]))

def flatten_node_unspaced(node):
    return ("".join([node_value(ch) for ch in node_children(node) if node_value(ch)]))

def flatten_node(node):
    def flattenificate(node, pnode, out):
        ps, pn, pc = pnode
        s, n, c = node
        if n is not None:
            if s in KEYWORD_TOKENS or s in KEYWORD_ONLY_NODES:
                # space keywords, unless they follow a keyword with (
                if pn and pn[-1] == "(":
                    out.append(n)
                else:
                    if ps in KEYWORD_TOKENS or ps in KEYWORD_ONLY_NODES or not out:
                        out.append(n+" ")
                    else:
                        out.append(" "+n+" ")
            else:
                if out and out[-1] and out[-1][-1] not in " \n\t(" and n != ")":
                    out.append(" "+n)
                else:
                    out.append(n)
                #out.append(n)
        p = None, None, None
        for i in c:
            flattenificate(i, p, out)
            p = i

    l = []
    flattenificate(node, (None, None, None), l)
    # if last item ends in space, then strip it
    if l and l[-1] and l[-1][-1] == " ":
        l[-1] = l[-1][:-1]
    return "".join(l)

def find_child_node(node, symbol):
    if node:
        s, n, c = node[0], node[1], node[2]
        if s == symbol:
            return node
        for child in c:
            found = find_child_node(child, symbol)
            if found:
                return found
    return None


def find_child_nodes(node, symbol):
    matches = []
    if node:
        s, n, c = node[0], node[1], node[2]
        if s == symbol:
            matches.append(node)
        for child in c:
            matches += find_child_nodes(child, symbol)
    return matches


def node_symbol(node):
    return node[0]

def node_value(node):
    return node[1] if node else ""

def node_children(node):
    return node[2]

def node_direct_child(node, i):
    if i >= len(node[2]):
        return None
    return node[2][i]

def node_direct_child_named(node, name):
    for ch in node[2]:
        if node_symbol(ch) == name:
            return ch
    return None


def flatten_comma_sep_node(node, sep = "", newline_on_comma=False):
    def flatten(expr_node, out):
        s, n, c = expr_node
        if n:
            if n == ",":
                if newline_on_comma:
                    out.append(",\n")
                else:
                    out.append(", ")
            else:
                if out and out[-1] and out[-1][-1] not in " \n\t(" and n != ")":
                    out.append(" "+n)
                else:
                    out.append(n)
        for i in c:
            flatten(i, out)

    out = []
    flatten(node, out)
    return sep.join(out)


def flatten_comma_sep_node_multiline(node):
    text = ""
    after_nl = False
    for ch in node_children(node):
        v = node_value(ch)
        if v == ",":
            text += ",\n"
            after_nl = True
        else:
            if after_nl:
                text += indent(flatten_node(ch))
            else:
                text += indent_tail(flatten_node(ch))
            after_nl = False
    return text


#######################################################################################

class SQLPrettifier:
    opt_func_arg_per_line = False
    opt_expr_length_per_line = 40
    opt_max_statement_length = 80
    opt_max_subselect_length = 40
    opt_max_select_item_list_length = 60
    opt_always_break_select_parts = False
    opt_always_break_select_items = False
    opt_always_quote_identifiers = False

    def __init__(self, ast):
        self.ast = ast
        self.parent = []

        if grt.root.wb.options.options.get("DbSqlEditor:Reformatter:UpcaseKeywords", 0):
            self.ast = self.upcasify_keywords(ast)

        self.ast = self.strip_ast(self.ast)


    def strip_ast(self, node):
        # remove useless nodes from the tree
        symbol, value, children = node

        if len(children) == 1 and node_symbol(children[0]) in STRIP_TOKENS:
            if node_value(children[0]):
                grt.log_warning("Reformatter", "Node unexpectedly has a value: %s\n" % repr(children[0]))
                return node
            children = node_children(children[0])

        new_children = []
        # traverse the AST depth-first and build up the formatted expression bottom up
        for node in children:
            processed_node = self.strip_ast(node)
            if processed_node:
                new_children.append(processed_node)

        return symbol, value, new_children


    def upcasify_keywords(self, node):
        symbol, value, children = node

        new_symbol = symbol
        if value and symbol not in NON_KEYWORD_TOKENS:
            if symbol.upper() == 'HEX_NUM':
                new_value = '0x' + value[2:].upper()
            else:
                new_value = value.upper()
        else:
            new_value = value
        new_children = []
        # traverse the AST depth-first and build up the formatted expression bottom up
        for node in children:
            processed_node = self.upcasify_keywords(node)
            if processed_node:
                new_children.append(processed_node)

        return new_symbol, new_value, new_children



    def run(self):
        return self.traverse(self.ast, [], [])[1].rstrip()


    def traverse(self, ast, siblings, path):
        symbol, value, children = ast

        new_symbol = symbol
        new_value = value
        new_children = []
        # traverse the AST depth-first and build up the formatted expression bottom up
        for i, node in enumerate(children):
            processed_node = self.traverse(node, children[i+1:], path + [symbol])
            if processed_node:
                new_children.append(processed_node)


        self.current_path = path
        
        handler = getattr(self, "sym_"+symbol, None)
        if handler:
            new_value = handler((new_symbol, new_value, new_children))
        else:
            new_value = None

        if new_value is None:
            new_value = self.default_handler((new_symbol, value, new_children))

        new_children = []
        
        return new_symbol, new_value, new_children

    def default_handler(self, node):
        return flatten_node(node)

    ## Generic stuff
    def sym_text_literal(self, node):
        return flatten_node_unspaced(node)
    
    def sym_TEXT_STRING(self, node):
        v = node_value(node)
        if v is None:
            v = ""
        return "'%s'" % v.replace("'", r"\'")
    sym_text_string = sym_TEXT_STRING
    sym_TEXT_STRING_filesystem = sym_TEXT_STRING
    sym_TEXT_STRING_literal = sym_TEXT_STRING
    sym_TEXT_STRING_sys = sym_TEXT_STRING 

    def sym_ident(self, node):
        ident = node_value(node)
        if ident and not (ident[0] == '`' and ident[-1]=='`') and self.opt_always_quote_identifiers:
            return "`%s`" % ident
        else:
            return ident

    def sym_simple_ident_q(self, node):
        return flatten_node_unspaced(node)

    def sym_table_ident(self, node):
        return flatten_node_unspaced(node)

    def sym_table_wild(self, node):
        return flatten_node_unspaced(node)

    def sym_variable(self, node):
        return flatten_node_unspaced(node)
    
    def sym_variable_aux(self, node):
        return flatten_node_unspaced(node)

    def sym_select_var_ident(self, node):
        return flatten_node_unspaced(node)

    ### SELECT ###
    def sym_select_init(self, node):
        select = node_direct_child(node, 0)
        if node_symbol(select) == "SELECT_SYM":
            rest = flatten_node((node[0], node[1], node[2][1:]))
            return node_value(select)+" "+rest

    def sym_select_init2(self, node):
        return flatten_node(node)

    def sym_union_clause(self, node):
        return "\n"+self.default_handler(node)

    def sym_select_option_list(self, node):
        return flatten_node_spaced(node)
    
    def sym_select_part2(self, node):
        #  select_options select_item_list select_into select_lock_type
        select_options = node_direct_child_named(node, "select_options")
        select_item_list = node_direct_child_named(node, "select_item_list")
        select_into = node_direct_child_named(node, "select_into")
        select_lock_type = node_direct_child_named(node, "select_lock_type")
        
        flat = flatten_node_spaced(node)
        if "\n" not in flat:
            return flat
        if select_options:
            text = node_value(select_options)
        else:
            text = ""
        if select_item_list:
            text += "\n"+indent_head(node_value(select_item_list))
        if select_into:
            text += "\n"+node_value(select_into)
        if select_lock_type:
            text += "\n"+node_value(select_lock_type)
        return text
      
    def sym_subselect(self, node):
        return indent_tail(self.default_handler(node))
        
    def sym_select_part2_derived(self, node): # in subselects
        #  opt_query_expression_options select_item_list opt_select_from select_lock_type
        select_expr_options = node_direct_child_named(node, "opt_query_expression_options")
        select_options = node_direct_child_named(node, "select_options")
        select_item_list = node_direct_child_named(node, "select_item_list")
        select_from = node_direct_child_named(node, "opt_select_from")
        select_lock_type = node_direct_child_named(node, "select_lock_type")
        
        flat = flatten_node_spaced(node)
        if "\n" not in flat:
            return flat
        text = ""
        if select_options:
            text += node_value(select_options)
        if select_expr_options:
            text += node_value(select_expr_options)
        if select_item_list:
            text += "\n"+indent(node_value(select_item_list))
        if select_from:
            text += "\n"+node_value(select_from)
        if select_lock_type:
            text += "\n"+node_value(select_lock_type)
        return text
    sym_select_derived2 = sym_select_part2_derived

    def sym_select_item(self, node):
        return flatten_node_spaced(node)

    def sym_select_item_list(self, node):
        flat = flatten_comma_sep_node(node)
        one_item_per_line = self.opt_always_break_select_items
        if not one_item_per_line:
            # if the list of select items is too wide or has newlines in it, we put one item per line
            if "\n" in flat or len(flat) > self.opt_max_select_item_list_length:
                one_item_per_line = True
        
        if one_item_per_line:
            flat = flatten_comma_sep_node_multiline(node)
                
        return flat

    def sym_select_from(self, node):
        children = node_children(node)
        if children and node_symbol(children[1]) != "DUAL_SYM":
            from_kwd = node_value(children[0])
            join_table_list = children[1]
            text = from_kwd+"\n"+INDENTATION+node_value(join_table_list)+"\n"
            for ch in children[2:]:
                text += node_value(ch)+"\n"
            text = text.rstrip("\n")
            return text
    
    def sym_join_table(self, node):
        text = node_value(node_direct_child(node, 0))
        children = node_children(node)[1:]
        join_type = []
        for i in range(len(children)):
            sym = node_symbol(children[i])
            if sym in ["normal_join", "STRAIGHT_JOIN", "STRAIGHT_JOIN", "NATURAL", "JOIN_SYM", "opt_outer", "LEFT", "RIGHT"]:
                join_type.append(node_value(children[i]))
            else:
                break
        join_type = " ".join(join_type)
        if "select_derived_union" not in self.current_path: # try to generate more compact output for nested joins
            text += "\n"+indent(join_type)+"\n"
        else:
            text += "\n"+join_type+" "
        
        text += node_value(node_direct_child(node, i+1))
        on = node_direct_child_named(node, "ON")
        using = node_direct_child_named(node, "USING")
        if on:
            expr = node_value(node_direct_child_named(node, "expr"))
            text += " ON %s" % expr
        elif using:
            using_list = node_value(node_direct_child_named(node, "using_list"))
            text += " USING (%s)" % using_list
        return text
        
    def sym_normal_join(self, node):
        return flatten_node_spaced(node)
    
    def sym_table_factor(self, node):
        if node_value(node_direct_child(node, 0)) == "(":
            text = "(%s)"%(node_value(node_direct_child_named(node, "select_derived_union")))
            alias = node_direct_child_named(node, "opt_table_alias")
            if alias:
                text += " "+node_value(alias)
            return text
        elif node_symbol(node_direct_child(node, 0)) == "table_ident" and len(node_children(node)) == 2:
            text = node_value(node_direct_child(node, 0)) + " " + node_value(node_direct_child(node, 1))
            return text
    
    def sym_derived_table_list(self, node):
        if "select_derived_union" not in self.current_path:
            return flatten_comma_sep_node_multiline(node)
        else:
            return flatten_comma_sep_node(node)


    def sym_where_clause(self, node):
        children = node_children(node)
        return node_value(children[0]) + "\n" + indent(node_value(children[1]))
    
    ##

    def sym_udf_expr_list(self, node):
        flattened = flatten_comma_sep_node(node)
        if "select_derived_union" not in self.current_path:  # try to generate more compact output for nested joins
            try:
                if self.opt_func_arg_per_line or len(flattened) > self.opt_expr_length_per_line or "\n" in flattened:
                    flattened = flatten_comma_sep_node_multiline(node)
            except Exception, exc:
                print "Error formatting: %s"%exc
                print node
        return flattened

    sym_expr_list = sym_udf_expr_list

    def sym_function_call_generic(self, node):
        children = node_children(node)
        if node_symbol(children[0]) == "ident" and node_symbol(children[1]) == "46" and node_symbol(children[2]) == "ident":
            flattened = node_value(children[0])+"."+node_value(children[2])
            del children[0:3]
        else:
            flattened = node_value(children[0])
            del children[0]
        head = flattened
        flattened += flatten_comma_sep_node(node)
        if "select_derived_union" not in self.current_path:  # try to generate more compact output for nested joins
            try:
                if self.opt_func_arg_per_line or len(flattened) > self.opt_expr_length_per_line or "\n" in flattened:
                    #flattened = flatten_comma_sep_node_multiline(node)
                    tmp = head
                    tmp += indent_tail(flatten_comma_sep_node(node, newline_on_comma=True))
                    flattened = tmp
            except Exception, exc:
                print "Error formatting function: %s"%exc
                print node
        return flattened

    sym_function_call_keyword = sym_function_call_generic
    sym_function_call_conflict = sym_function_call_generic
    sym_geometry_function = sym_function_call_generic
    sym_function_call_nonkeyword = sym_function_call_generic    

    def sym_expr(self, node):
        otext = text = self.default_handler(node)
        try:
            if len(text) > self.opt_expr_length_per_line:
                children = node_children(node)
                text = node_value(children[0])
                i = 1
                while i < len(children)-1:
                    oper = node_value(children[i])
                    value = node_value(children[i+1])
                    text += "\n" + indent(oper) + " " + value
                    i += 2
                if i < len(children):
                    text += " "+node_value(children[-1])
        except:
            grt.log_error("SQLReformatter", "Error formatting expression: %s" % otext)
            import traceback
            traceback.print_exc()
        return text

    def sym_simple_expr(self, node):
        # some keywords in simpl_expr accept space before ( and some don't.. just leave them out always atm
        children = node_children(node)
        
        try:
            if node_symbol(children[0]) == "CAST_SYM":
                text = node_value(children[0])+node_value(children[1])
                if len(node_value(children[2])) > self.opt_expr_length_per_line:
                    text += indent_tail(node_value(children[2])) + "\n"
                    text += indent(node_value(children[3]) + " " +node_value(children[4]) + node_value(children[5]))
                else:
                    text += indent_tail(node_value(children[2])) + " "
                    text += node_value(children[3]) + " " +node_value(children[4]) + node_value(children[5])
                return text
            elif node_symbol(children[0]) == "CASE_SYM":
                #  CASE_SYM opt_expr when_list opt_else END
                opt_expr = node_direct_child_named(node, "opt_expr")
                when_list = node_direct_child_named(node, "when_list")
                opt_else = node_direct_child_named(node, "opt_else")
                
                if opt_expr:
                    text = node_value(children[0])+" "+node_value(opt_expr)+"\n"
                else:
                    text = node_value(children[0])+"\n"
                
                if when_list:
                    text += indent(node_value(when_list))+"\n"
                
                if opt_else:
                    text += indent(node_value(opt_else))+"\n"
                
                text += node_value(children[-1])
                return text
        except Exception:
            import traceback
            traceback.print_exc()
        
        if len(children) > 2 and node_value(children[1]) == "(":
            # join the ( to the previous symbol name
            s, v, c = children[0]
            children[0] = s, v+"(", c
            del children[1]
        return flatten_node(node).strip()

    def sym_bit_expr(self, node):
        return flatten_node_spaced(node)
    
    def sym_when_list(self, node):
        # when_list WHEN_SYM expr THEN_SYM expr
        children = node_children(node)
        i = 0
        l = []
        while i < len(children):
            expr1 = node_value(children[i+1])
            expr2 = node_value(children[i+3])
            if "\n" in expr1 or "\n" in expr2:
                line = node_value(children[i])+"\n"+indent(expr1)+"\n"+node_value(children[i+2])+"\n"+indent(expr2)
            else:
                line = node_value(children[i])+" "+expr1+" "+node_value(children[i+2])+" "+expr2
            i += 4
            l.append(line)
        return "\n".join(l)

    def sym_sum_expr(self, node):
        children = node_children(node)
        if node_symbol(children[0]) == "GROUP_CONCAT_SYM":            
            """GROUP_CONCAT_SYM '(' opt_distinct expr_list opt_gorder_clause opt_gconcat_separator ')'"""
            s, n, c = node

            text = node_value(c[0])
            i = 1
            if node_value(c[i]) == "(": # ( may have been already concatenated to GROUP_CONCAT node
                text += "("
                i+=1
            if node_symbol(c[i]) == "opt_distinct":
                text += flatten_node(c[i])+" "
                i+=1
            if node_symbol(c[i]) == "expr_list":
                text += node_value(c[i])
                i+=1
            else:
                print "Unexpected symbol in GROUP_CONCAT", node
                return "????"
            if node_symbol(c[i]) == "opt_gorder_clause":
                text += "\n"+indent(node_value(c[i]))
                i+=1
            if node_symbol(c[i]) == "opt_gconcat_separator":
                text += "\n"+indent(node_value(c[i]))
                i+=1
            text += ")"
            return text
        else:
            text = node_value(children[0])+node_value(children[1])
            text += flatten_node_spaced((node[0], node[1], children[2:-1]))
            text += node_value(children[-1])
            return text
    
    ### CREATE TABLE ###
    def sym_field_list(self, node):
        return "\n"+indent_head(flatten_comma_sep_node_multiline(node))+"\n"

    def sym_column_def(self, node):
        return flatten_node_spaced(node)

    def sym_key_def(self, node):
        #  normal_key_type opt_ident key_alg '(' key_list ')' normal_key_options
        normal_key_type = node_direct_child_named(node, "normal_key_type")
        if normal_key_type:
            opt_ident = node_direct_child_named(node, "opt_ident")
            key_alg = node_direct_child_named(node, "key_alg")
            key_list = node_direct_child_named(node, "key_list")
            normal_key_options = node_direct_child_named(node, "normal_key_options")

            text = node_value(normal_key_type)
            if opt_ident:
                text += " "+node_value(opt_ident)
            if key_alg and node_value(key_alg):
                text += " "+node_value(key_alg)
            text += " (%s)" % node_value(key_list)
            if normal_key_options:
                text += " "+node_value(normal_key_options)
            return text
                
        # opt_constraint FOREIGN KEY_SYM opt_ident '(' key_list ')' references
        opt_constraint = node_direct_child_named(node, "opt_constraint")
        foreign = node_direct_child_named(node, "FOREIGN")
        key = node_direct_child_named(node, "KEY_SYM")
        if foreign and key:
            key_list = node_direct_child_named(node, "key_list")
            references = node_direct_child_named(node, "references")
            if opt_constraint:
                text = node_value(opt_constraint)+" "
            else:
                text = ""
            text += "%s %s (%s)\n" % (node_value(foreign), node_value(key), node_value(key_list))
            text += indent(node_value(references))
            return text
        # opt_constraint constraint_key_type opt_ident key_alg '(' key_list ')' normal_key_options
        constraint_key_type = node_direct_child_named(node, "constraint_key_type")
        if constraint_key_type:
            opt_ident = node_direct_child_named(node, "opt_ident")
            key_alg = node_direct_child_named(node, "key_alg")
            key_list = node_direct_child_named(node, "key_list")
            normal_key_options = node_direct_child_named(node, "normal_key_options")

            if opt_constraint:
                text = node_value(opt_constraint)+" "
            else:
                text = ""
            text += node_value(constraint_key_type)
            if opt_ident:
                text += " "+node_value(opt_ident)
            if key_alg and node_value(key_alg):
                text += " "+node_value(key_alg)
            text += " (%s)" % node_value(key_list)
            if normal_key_options:
                text += " "+node_value(normal_key_options)
            return text

        # spatial opt_key_or_index opt_ident init_key_options '(' key_list ')' spatial_key_options
        # fulltext opt_key_or_index opt_ident init_key_options '(' key_list ')' fulltext_key_options
        # opt_constraint check_constraint
        return flatten_node_spaced(node)

    def sym_opt_constraint(self, node):
        return flatten_node_spaced(node)
    
    def sym_references(self, node):
        # REFERENCES table_ident opt_ref_list opt_match_clause opt_on_update_delete
        text = node_value(node_direct_child_named(node, "REFERENCES")) + " "+node_value(node_direct_child_named(node, "table_ident"))
        ref_list = node_direct_child_named(node, "opt_ref_list")
        match_clause = node_direct_child_named(node, "opt_match_clause")
        on_upd = node_direct_child_named(node, "opt_on_update_delete")
        if ref_list:
            text += " "+node_value(ref_list)
        if match_clause:
            text += "\n"+node_value(match_clause)
        if on_upd:
            text += "\n"+node_value(on_upd)
        return text

    def sym_opt_ref_list(self, node):
        return "(%s)"%node_value(node_direct_child_named(node, "ref_list"))
  
    def sym_field_spec(self, node):
        return flatten_node_spaced(node)
        
    def sym_type(self, node):
        return flatten_node_unspaced(node)
    
    def sym_field_length(self, node):
        return flatten_node(node)
    
    def sym_float_options(self, node):
        return flatten_comma_sep_node(node)
    
    def sym_field_opt_list(self, node):
        return " "+flatten_node_spaced(node)
    
    def sym_string_list(self, node):
        return flatten_comma_sep_node(node)
    
    def sym_opt_attribute_list(self, node):
        return flatten_node_spaced(node)

    def sym_create_table_option(self, node):
        if len(node_children(node)) > 1:
            name= []
            value= []
            l = name
            for ch in node_children(node):
                if node_symbol(ch) == "opt_equal":
                    l= value
                else:
                    l.append(node_value(ch))
            return "%s=%s" %(" ".join(name), " ".join(value))
        else:
            return node_value(node)
    sym_default_charset = sym_create_table_option
        
    def sym_create_table_options(self, node):
        text = flatten_node_spaced(node)
        if text:
            return " "+text

    def sym_user(self, node):
        return flatten_node_unspaced(node)
    

class UpdateSQLPrettifier(SQLPrettifier):
    def sym_SET(self, node):
        return "\n"+node_value(node)

    def sym_update_list(self, node):
        text = ""
        for ch in node_children(node):
            if node_symbol(ch) == "update_elem":
                text += "\n"+indent(self.default_handler(ch))
            else:
                text += node_value(ch)
        return text+"\n"

    def sym_where_clause(self, node):
        text = node_value(node_direct_child_named(node, "WHERE"))+"\n"
        text += indent(self.default_handler(node_direct_child_named(node, "expr")))
        return text


class DeleteSQLPrettifier(SQLPrettifier):
    def sym_where_clause(self, node):
        text = "\n"+node_value(node_direct_child_named(node, "WHERE"))+"\n"
        text += indent("")+self.default_handler(node_direct_child_named(node, "expr"))
        return text


class ViewSQLPrettifier(SQLPrettifier):
    _has_args = False
    
    def sym_view_algorithm(self, node):
        self._has_args = True
        return "\n"+indent(self.default_handler(node))

    def sym_definer(self, node):
        self._has_args = True
        return "\n"+indent(self.default_handler(node))       

    def sym_view_tail(self, node):
        # view_suid VIEW_SYM table_ident view_list_opt AS view_select
        suid = node_direct_child_named(node, "view_suid")
        if suid:
            if self._has_args:
                text = "\n"+indent(node_value(suid))
            else:
                text = node_value(suid)
        else:
            text = ""
        if self._has_args or suid:
            text += "\n"
        text += node_value(node_direct_child_named(node, "VIEW_SYM"))
        text += " "+node_value(node_direct_child_named(node, "table_ident"))
        if node_direct_child_named(node, "view_list_opt"):
            text += " "+node_value(node_direct_child_named(node, "view_list_opt"))
        text += " "+node_value(node_direct_child_named(node, "AS"))
        view_select = node_value(node_direct_child_named(node, "view_select"))
        if "\n" in view_select or self._has_args or suid:
            text += "\n"+indent(view_select)
        else:
            text += " "+view_select
        return text
    
    
def formatter_for_statement_ast(ast):
    statement = ast[2][0][2][0]

    if statement[0] == "select":
        return SQLPrettifier
    elif statement[0] == "update":
        return UpdateSQLPrettifier
    elif statement[0] == "delete":
        return DeleteSQLPrettifier
    elif statement[0] == "create":
        #unused create = statement[2][0]
        object = statement[2][1]
        if object[0] == "TABLE_SYM":
            return SQLPrettifier
        elif object[0] == "view_or_trigger_or_sp_or_event":
            # recursively look for VIEW_SYM 
            if find_child_node(object, "VIEW_SYM"):
                return ViewSQLPrettifier
        # SPs and functions not supported for now (esp. because comments are not maintained by parser)


