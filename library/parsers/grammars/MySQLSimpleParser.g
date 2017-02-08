parser grammar MySQLSimpleParser;

/*
 * Copyright (c) 2012, 2016, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

/*
 * Merged in all changes up to mysql-trunk git revision [2405334] (04 Sep 2015).
 *
 * This is a duplicate of the main MySQL grammar, but without any tree output/rewriting.
 * Without that the parser is around 3 times faster and hence better suited for tasks like syntax
 * checking.
 * For more details read the comment in the main grammar.
 *
 *
 * Written by Mike Lischke. Direct all bug reports, omissions etc. to mike.lischke@oracle.com.
 */

//-------------------------------------------------------------------------------------------------

options {
  tokenVocab = MySQL;
  language = C;
  k = 2;  // Leave it at that. Two tokens lookahead is the minimum. Higher values increase the parser size significantly.
}

//-------------------------------------------------------------------------------------------------

@parser::header {

#define ANTLR3_HUGE
#ifndef _WIN32
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wparentheses"
#ifdef __APPLE__
// Comparison of unsigned expression >= 0 is always true.
#pragma GCC diagnostic ignored "-Wtautological-compare"
#else
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 6 )
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif
#endif
#endif

#include "MySQLLexer.h" // Not automatically included by the generator.
}

@parser::postinclude {
#ifdef __cplusplus
extern "C" { 
#endif

  // Custom error reporting function.
  void onMySQLParseError(struct ANTLR3_BASE_RECOGNIZER_struct *recognizer, pANTLR3_UINT8 *tokenNames); 

#ifdef __cplusplus
};
#endif
}

@parser::members {
}

@parser::apifuncs
{
 // Install custom error collector for the front end.
 RECOGNIZER->displayRecognitionError = onMySQLParseError;
}

//-------------------------------------------------------------------------------------------------

query:
	((statement | begin_work) SEMICOLON_SYMBOL?)? EOF
;

statement:
	// DDL
	alter_statement
	| create_statement
	| drop_statement
	| rename_table_statement
	| truncate_table_statement
	
	// DML
	| call_statement
	| delete_statement
	| do_statement
	| handler_statement
	| insert_statement
	| load_statement
	| replace_statement
	| select_statement
	| update_statement

	// Rules that cannot be used standalone, but are here for more complexer statements.
	| partitioning
	//| parse_gcol_expression
		
	| transaction_or_locking_statement
	
	| replication_statement
	
	| prepared_statement
	
	// Database administration
	| account_management_statement
	| table_administration_statement
	| install_uninstall_statment
	| {LA(1) == SET_SYMBOL && LA(2) != PASSWORD_SYMBOL}? set_statement // SET PASSWORD is handled in account_management_statement.
	| show_statement
	| other_administrative_statement
	
	// MySQL utilitity statements
	| utility_statement
	
	| {SERVER_VERSION >= 50604}? get_diagnostics
	| {SERVER_VERSION >= 50500}? signal_statement
	| {SERVER_VERSION >= 50500}? resignal_statement
;

//----------------- DDL statements -----------------------------------------------------------------

alter_statement:
  ALTER_SYMBOL
  (
	alter_database
	| alter_log_file_group
	| FUNCTION_SYMBOL function_ref routine_alter_options?
	| PROCEDURE_SYMBOL procedure_ref routine_alter_options?
	| alter_server
	| alter_table
	| alter_tablespace
	| {SERVER_VERSION >= 50100}? alter_event
	| alter_view
  )
;

alter_database:
	DATABASE_SYMBOL schema_ref
	(
		database_option+
		| UPGRADE_SYMBOL DATA_SYMBOL DIRECTORY_SYMBOL NAME_SYMBOL
	)
;

alter_event:
	definer_clause?
		EVENT_SYMBOL event_ref
		(ON_SYMBOL SCHEDULE_SYMBOL schedule)?
		(ON_SYMBOL COMPLETION_SYMBOL NOT_SYMBOL? PRESERVE_SYMBOL)?
		(RENAME_SYMBOL TO_SYMBOL identifier)?
		(ENABLE_SYMBOL | DISABLE_SYMBOL (ON_SYMBOL SLAVE_SYMBOL)?)?
		(COMMENT_SYMBOL string_literal)?
		(DO_SYMBOL compound_statement)?
;

alter_log_file_group:
	LOGFILE_SYMBOL GROUP_SYMBOL logfile_group_ref ADD_SYMBOL UNDOFILE_SYMBOL string_literal
		(INITIAL_SIZE_SYMBOL EQUAL_OPERATOR? size_number)? WAIT_SYMBOL? ENGINE_SYMBOL EQUAL_OPERATOR? engine_ref
;

alter_server:
	SERVER_SYMBOL server_ref server_options
;

alter_table:
	online_option? IGNORE_SYMBOL? TABLE_SYMBOL table_ref alter_table_commands?
;

alter_table_commands:
	DISCARD_SYMBOL TABLESPACE_SYMBOL
	| IMPORT_SYMBOL TABLESPACE_SYMBOL
	| alter_table_list_item (COMMA_SYMBOL alter_table_list_item)* (partitioning? | remove_partitioning)
	| remove_partitioning
	| partitioning
	| {SERVER_VERSION >= 50100}? alter_partition
;

alter_table_list_item:
	alter_table_list_entry
;

alter_table_list_entry:
	alter_table_options
	| ADD_SYMBOL COLUMN_SYMBOL?
		(
			column_definition (FIRST_SYMBOL | AFTER_SYMBOL identifier)?
			| OPEN_PAR_SYMBOL column_definition (COMMA_SYMBOL column_definition)* CLOSE_PAR_SYMBOL
		)
	| ADD_SYMBOL key_def
	| ALTER_SYMBOL COLUMN_SYMBOL? column_internal_ref (SET_SYMBOL DEFAULT_SYMBOL signed_literal | DROP_SYMBOL DEFAULT_SYMBOL)
	| CHANGE_SYMBOL COLUMN_SYMBOL? column_internal_ref field_spec (FIRST_SYMBOL | AFTER_SYMBOL identifier)?
	| MODIFY_SYMBOL COLUMN_SYMBOL? column_internal_ref data_type attribute* (FIRST_SYMBOL | AFTER_SYMBOL identifier)?
	| DROP_SYMBOL
		(
			(INDEX_SYMBOL | KEY_SYMBOL) column_ref
			| COLUMN_SYMBOL? column_internal_ref
			| PRIMARY_SYMBOL KEY_SYMBOL
			| FOREIGN_SYMBOL KEY_SYMBOL
				(
					// This part is no longer optional starting with 5.7.
					{SERVER_VERSION >= 50700}? => column_ref
					| {SERVER_VERSION < 50700}? => column_ref?
				)
		)
	| DISABLE_SYMBOL KEYS_SYMBOL
	| ENABLE_SYMBOL KEYS_SYMBOL
	| RENAME_SYMBOL (TO_SYMBOL | AS_SYMBOL)? table_ref
	| {SERVER_VERSION >= 50700}? RENAME_SYMBOL (INDEX_SYMBOL | KEY_SYMBOL) column_ref TO_SYMBOL column_ref
	| alter_order_by
	| CONVERT_SYMBOL TO_SYMBOL charset charset_name_or_default (COLLATE_SYMBOL collation_name_or_default)?
	| FORCE_SYMBOL
	| {SERVER_VERSION >= 50600}? alter_algorithm_option
	| {SERVER_VERSION >= 50600}? alter_lock_option
	| {SERVER_VERSION >= 50708}? UPGRADE_SYMBOL PARTITIONING_SYMBOL
;

key_def:
	(CONSTRAINT_SYMBOL column_ref?)?
		(
			PRIMARY_SYMBOL KEY_SYMBOL column_ref? index_type? index_columns normal_index_option*
			| FOREIGN_SYMBOL KEY_SYMBOL column_ref? index_columns reference_definition
			| UNIQUE_SYMBOL (INDEX_SYMBOL | KEY_SYMBOL)? column_ref? index_type? index_columns normal_index_option*
			| CHECK_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
		)
	| (INDEX_SYMBOL | KEY_SYMBOL) column_ref? index_type? index_columns normal_index_option*
	| (FULLTEXT_SYMBOL | SPATIAL_SYMBOL) (INDEX_SYMBOL | KEY_SYMBOL)? column_ref? index_columns normal_index_option*
;

alter_order_by:
	ORDER_SYMBOL BY_SYMBOL identifier direction (options { greedy = true; }: COMMA_SYMBOL identifier direction)*
;

alter_algorithm_option:
	ALGORITHM_SYMBOL EQUAL_OPERATOR? ( DEFAULT_SYMBOL | identifier )
;

alter_lock_option:
	LOCK_SYMBOL EQUAL_OPERATOR? (DEFAULT_SYMBOL | identifier)
;

index_lock_algorithm:
	{SERVER_VERSION >= 50600}? => 
		(
			alter_algorithm_option alter_lock_option?
			| alter_lock_option alter_algorithm_option?
		)
;

alter_partition:
	{LA(1) == ADD_SYMBOL && LA(2) == PARTITION_SYMBOL}? => ADD_SYMBOL PARTITION_SYMBOL no_write_to_bin_log?
		(
			OPEN_PAR_SYMBOL partition_definition CLOSE_PAR_SYMBOL
			| PARTITIONS_SYMBOL real_ulong_number
		)
	| DROP_SYMBOL PARTITION_SYMBOL identifier_list
	| REBUILD_SYMBOL PARTITION_SYMBOL no_write_to_bin_log? all_or_partition_name_list
	| OPTIMIZE_SYMBOL PARTITION_SYMBOL no_write_to_bin_log? all_or_partition_name_list no_write_to_bin_log? // yes, twice "no write to bin log".
	| ANALYZE_SYMBOL PARTITION_SYMBOL no_write_to_bin_log? all_or_partition_name_list
	| CHECK_SYMBOL PARTITION_SYMBOL all_or_partition_name_list check_option*
	| REPAIR_SYMBOL PARTITION_SYMBOL no_write_to_bin_log? all_or_partition_name_list repair_option*
	| COALESCE_SYMBOL PARTITION_SYMBOL no_write_to_bin_log? real_ulong_number
	| {SERVER_VERSION >= 50500}? TRUNCATE_SYMBOL PARTITION_SYMBOL all_or_partition_name_list
	| EXCHANGE_SYMBOL PARTITION_SYMBOL identifier WITH_SYMBOL TABLE_SYMBOL table_ref validation?
	| REORGANIZE_SYMBOL PARTITION_SYMBOL no_write_to_bin_log? (identifier_list INTO_SYMBOL partition_definitions)?
	| {SERVER_VERSION >= 50704}? DISCARD_SYMBOL PARTITION_SYMBOL all_or_partition_name_list TABLESPACE_SYMBOL
	| {SERVER_VERSION >= 50704}? IMPORT_SYMBOL PARTITION_SYMBOL all_or_partition_name_list TABLESPACE_SYMBOL
;

validation:
	{SERVER_VERSION >= 50706}? => (WITH_SYMBOL | WITHOUT_SYMBOL) VALIDATION_SYMBOL
;

remove_partitioning:
	{SERVER_VERSION >= 50100}? => REMOVE_SYMBOL PARTITIONING_SYMBOL
;

all_or_partition_name_list:
	ALL_SYMBOL
	| identifier_list
;

alter_tablespace:
	TABLESPACE_SYMBOL tablespace_ref
	(
		(ADD_SYMBOL | DROP_SYMBOL) DATAFILE_SYMBOL string_literal (alter_tablespace_option (COMMA_SYMBOL? alter_tablespace_option)*)?
		// The alternatives listed below are not documented but appear in the server grammar file.
		| CHANGE_SYMBOL DATAFILE_SYMBOL string_literal (change_tablespace_option (COMMA_SYMBOL? change_tablespace_option)*)?
		| READ_ONLY_SYMBOL
		| READ_WRITE_SYMBOL
		| NOT_SYMBOL ACCESSIBLE_SYMBOL
	)
;

alter_tablespace_option:
	INITIAL_SIZE_SYMBOL EQUAL_OPERATOR? size_number
	| AUTOEXTEND_SIZE_SYMBOL EQUAL_OPERATOR? size_number
	| MAX_SIZE_SYMBOL EQUAL_OPERATOR? size_number
	| STORAGE_SYMBOL? ENGINE_SYMBOL EQUAL_OPERATOR? engine_ref
	| (WAIT_SYMBOL | NO_WAIT_SYMBOL)
;

change_tablespace_option:
	INITIAL_SIZE_SYMBOL EQUAL_OPERATOR? size_number
	| AUTOEXTEND_SIZE_SYMBOL EQUAL_OPERATOR? size_number
	| MAX_SIZE_SYMBOL EQUAL_OPERATOR? size_number
;

alter_view:
	(ALGORITHM_SYMBOL EQUAL_OPERATOR (UNDEFINED_SYMBOL | MERGE_SYMBOL | TEMPTABLE_SYMBOL))?
		definer_clause?
		(SQL_SYMBOL SECURITY_SYMBOL (DEFINER_SYMBOL | INVOKER_SYMBOL))?
		VIEW_SYMBOL view_ref identifier_list_with_parentheses? AS_SYMBOL select_statement
		(WITH_SYMBOL (CASCADED_SYMBOL | LOCAL_SYMBOL)? CHECK_SYMBOL OPTION_SYMBOL)?
;

//--------------------------------------------------------------------------------------------------

// This is an optimized collector rule for all create statements so that we don't have disambiguities.
// In addition we have rules not referenced anywhere to parse individual create statements. These are used
// in object editors when parsing create statements only.
create_statement:
	CREATE_SYMBOL
	(
		create_table_tail
		| create_index_tail
		| create_database_tail
		| definer_clause?
			(
				{SERVER_VERSION >= 50100}? create_event_tail
				| create_view_tail
				| create_routine_or_udf
				| create_trigger_tail
			)
		| view_replace_or_algorithm definer_clause? create_view_tail
		| create_logfile_group_tail
		| create_server_tail
		| create_tablespace_tail
	)
;

create_database: // For external use only. Don't reference this in the normal grammar.
	CREATE_SYMBOL create_database_tail SEMICOLON_SYMBOL? EOF
;

create_database_tail:
	DATABASE_SYMBOL if_not_exists? schema_name database_option*
;

create_with_definer:
	CREATE_SYMBOL definer_clause?
;

create_event: // For external use only. Don't reference this in the normal grammar.
	create_with_definer create_event_tail SEMICOLON_SYMBOL? EOF
;

create_event_tail:
	EVENT_SYMBOL if_not_exists? event_name ON_SYMBOL SCHEDULE_SYMBOL schedule
		(ON_SYMBOL COMPLETION_SYMBOL NOT_SYMBOL? PRESERVE_SYMBOL)?
		(ENABLE_SYMBOL | DISABLE_SYMBOL (ON_SYMBOL SLAVE_SYMBOL)?)?
		(COMMENT_SYMBOL string_literal)?
		DO_SYMBOL compound_statement
;

create_routine: // For external use only. Don't reference this in the normal grammar.
	create_with_definer create_routine_or_udf SEMICOLON_SYMBOL? EOF
;

create_procedure: // For external use only. Don't reference this in the normal grammar.
	create_with_definer procedure_body SEMICOLON_SYMBOL? EOF
;

create_function: // For external use only. Don't reference this in the normal grammar.
	create_with_definer function_body SEMICOLON_SYMBOL? EOF
;

create_routine_or_udf:
	procedure_body
	| function_body
;

procedure_body:
	PROCEDURE_SYMBOL procedure_name OPEN_PAR_SYMBOL (procedure_parameter (COMMA_SYMBOL procedure_parameter)*)? CLOSE_PAR_SYMBOL
		routine_create_options? compound_statement
;

function_body: // Both built-in functions and UDFs.
	FUNCTION_SYMBOL
	(
		function_name OPEN_PAR_SYMBOL (function_parameter (COMMA_SYMBOL function_parameter)*)? CLOSE_PAR_SYMBOL RETURNS_SYMBOL
			data_type routine_create_options? compound_statement
		| udf_tail
	)
	| AGGREGATE_SYMBOL FUNCTION_SYMBOL udf_tail // AGGREGATE is optional and in order to avoid ambiquities we have two udf paths.
;

udf_tail:
	udf_name RETURNS_SYMBOL (STRING_SYMBOL | INT_SYMBOL | REAL_SYMBOL | DECIMAL_SYMBOL) SONAME_SYMBOL string_literal
;

routine_create_options:
	routine_create_option+
;
	
routine_create_option:
	routine_option
	| NOT_SYMBOL? DETERMINISTIC_SYMBOL
;

routine_alter_options:
	routine_create_option+
;
	
routine_option:
	COMMENT_SYMBOL string_literal
	| LANGUAGE_SYMBOL SQL_SYMBOL
	| NO_SYMBOL SQL_SYMBOL
	| CONTAINS_SYMBOL SQL_SYMBOL
	| READS_SYMBOL SQL_SYMBOL DATA_SYMBOL
	| MODIFIES_SYMBOL SQL_SYMBOL DATA_SYMBOL
	| SQL_SYMBOL SECURITY_SYMBOL (DEFINER_SYMBOL | INVOKER_SYMBOL)
;

create_index: // For external use only. Don't reference this in the normal grammar.
	CREATE_SYMBOL create_index_tail SEMICOLON_SYMBOL? EOF
;

create_index_tail:
	online_option?
	(
		UNIQUE_SYMBOL? INDEX_SYMBOL index_name index_type? create_index_target normal_index_option* index_lock_algorithm?
		| FULLTEXT_SYMBOL INDEX_SYMBOL index_name create_index_target fulltext_index_option* index_lock_algorithm?
		| SPATIAL_SYMBOL INDEX_SYMBOL index_name create_index_target spatial_index_option* index_lock_algorithm?
	)
;

create_index_target:
	ON_SYMBOL table_ref index_columns
;

create_logfile_group: // For external use only. Don't reference this in the normal grammar.
	CREATE_SYMBOL create_logfile_group_tail SEMICOLON_SYMBOL? EOF
;

create_logfile_group_tail:
	LOGFILE_SYMBOL GROUP_SYMBOL logfile_group_name ADD_SYMBOL (UNDOFILE_SYMBOL | REDOFILE_SYMBOL) string_literal
		logfile_group_options?
;

logfile_group_options:
	(logfile_group_option (COMMA_SYMBOL? logfile_group_option)*)
;

logfile_group_option:
	INITIAL_SIZE_SYMBOL EQUAL_OPERATOR? size_number
	| (UNDO_BUFFER_SIZE_SYMBOL | REDO_BUFFER_SIZE_SYMBOL) EQUAL_OPERATOR? size_number
	| NODEGROUP_SYMBOL EQUAL_OPERATOR? real_ulong_number
	| (WAIT_SYMBOL | NO_WAIT_SYMBOL)
	| COMMENT_SYMBOL EQUAL_OPERATOR? string_literal
	| STORAGE_SYMBOL? ENGINE_SYMBOL EQUAL_OPERATOR? engine_ref
;
	
create_server: // For external use only. Don't reference this in the normal grammar.
	CREATE_SYMBOL create_server_tail SEMICOLON_SYMBOL? EOF
;

create_server_tail:
	SERVER_SYMBOL server_name
		FOREIGN_SYMBOL DATA_SYMBOL WRAPPER_SYMBOL text_or_identifier server_options
;

server_options:
	OPTIONS_SYMBOL OPEN_PAR_SYMBOL server_option (COMMA_SYMBOL server_option)* CLOSE_PAR_SYMBOL
;

// Options for CREATE/ALTER SERVER, used for the federated storage engine.
server_option:
	HOST_SYMBOL string_literal
	| DATABASE_SYMBOL string_literal
	| USER_SYMBOL string_literal
	| PASSWORD_SYMBOL string_literal
	| SOCKET_SYMBOL string_literal
	| OWNER_SYMBOL string_literal
	| PORT_SYMBOL ulong_number
;

create_table: // For external use only. Don't reference this in the normal grammar.
	CREATE_SYMBOL create_table_tail SEMICOLON_SYMBOL? EOF
;

create_table_tail:
	TEMPORARY_SYMBOL? TABLE_SYMBOL if_not_exists? table_name
		(
			OPEN_PAR_SYMBOL
				(
					create_field_list CLOSE_PAR_SYMBOL create_table_options? partitioning? table_creation_source?
					| partitioning create_select CLOSE_PAR_SYMBOL union_clause?
				)
			| create_table_options? partitioning? table_creation_source
			| LIKE_SYMBOL table_ref
			| OPEN_PAR_SYMBOL LIKE_SYMBOL table_ref CLOSE_PAR_SYMBOL
		)
;

create_field_list:
	create_item (COMMA_SYMBOL create_item)*
;

create_item:
	create_field_list_item
;

create_field_list_item:
	column_definition
	| key_def
;

table_creation_source: // create3 in sql_yacc.yy
	(REPLACE_SYMBOL | IGNORE_SYMBOL)? AS_SYMBOL?
	(
		create_select union_clause?
		| OPEN_PAR_SYMBOL create_select CLOSE_PAR_SYMBOL union_or_order_by_or_limit?
	)
;

// The select statement allowed for CREATE TABLE (and certain others) differs from the standard select statement.
create_select:
	SELECT_SYMBOL ( options { greedy = true; }: select_option)* select_item_list table_expression
;

create_tablespace: // For external use only. Don't reference this in the normal grammar.
	CREATE_SYMBOL create_tablespace_tail SEMICOLON_SYMBOL? EOF
;

create_tablespace_tail:
	TABLESPACE_SYMBOL tablespace_name ADD_SYMBOL DATAFILE_SYMBOL string_literal
		(USE_SYMBOL LOGFILE_SYMBOL GROUP_SYMBOL logfile_group_ref)? tablespace_options?
;

tablespace_options:
	(tablespace_option (COMMA_SYMBOL? tablespace_option)*)
;

tablespace_option:
	INITIAL_SIZE_SYMBOL EQUAL_OPERATOR? size_number
	| AUTOEXTEND_SIZE_SYMBOL EQUAL_OPERATOR? size_number
	| MAX_SIZE_SYMBOL EQUAL_OPERATOR? size_number
	| EXTENT_SIZE_SYMBOL EQUAL_OPERATOR? size_number
	| NODEGROUP_SYMBOL EQUAL_OPERATOR? real_ulong_number
	| STORAGE_SYMBOL? ENGINE_SYMBOL EQUAL_OPERATOR? engine_ref
	| (WAIT_SYMBOL | NO_WAIT_SYMBOL)
	| COMMENT_SYMBOL EQUAL_OPERATOR? string_literal
	| {SERVER_VERSION >= 50707}? FILE_BLOCK_SIZE_SYMBOL EQUAL_OPERATOR? size_number
;

create_trigger: // For external use only. Don't reference this in the normal grammar.
	create_with_definer create_trigger_tail SEMICOLON_SYMBOL? EOF
;

create_trigger_tail:
	TRIGGER_SYMBOL trigger_name (BEFORE_SYMBOL | AFTER_SYMBOL) (INSERT_SYMBOL | UPDATE_SYMBOL | DELETE_SYMBOL)
		ON_SYMBOL table_ref FOR_SYMBOL EACH_SYMBOL ROW_SYMBOL trigger_follows_precedes_clause? compound_statement
;

trigger_follows_precedes_clause:
	{SERVER_VERSION >= 50700}? (FOLLOWS_SYMBOL | PRECEDES_SYMBOL) text_or_identifier // not a trigger reference!
;
	
create_view: // For external use only. Don't reference this in the normal grammar.
	CREATE_SYMBOL (OR_SYMBOL REPLACE_SYMBOL)? view_algorithm?  definer_clause? create_view_tail SEMICOLON_SYMBOL? EOF
;

create_view_tail:
	(SQL_SYMBOL SECURITY_SYMBOL (DEFINER_SYMBOL | INVOKER_SYMBOL))?
		VIEW_SYMBOL view_name identifier_list_with_parentheses?
		AS_SYMBOL select_statement
		(WITH_SYMBOL (CASCADED_SYMBOL | LOCAL_SYMBOL)? CHECK_SYMBOL OPTION_SYMBOL)?
;

view_replace_or_algorithm:
	OR_SYMBOL REPLACE_SYMBOL view_algorithm?
	| view_algorithm
;

view_algorithm:
	ALGORITHM_SYMBOL EQUAL_OPERATOR (UNDEFINED_SYMBOL | MERGE_SYMBOL | TEMPTABLE_SYMBOL)
;

//--------------------------------------------------------------------------------------------------

drop_statement:
	DROP_SYMBOL
	(
		DATABASE_SYMBOL if_exists? schema_ref
		| {SERVER_VERSION >= 50100}? EVENT_SYMBOL if_exists? event_ref
		|
			(
				FUNCTION_SYMBOL if_exists? function_ref // Including UDFs.
				| PROCEDURE_SYMBOL if_exists? procedure_ref
			)
		| online_option? INDEX_SYMBOL index_ref ON_SYMBOL table_ref index_lock_algorithm?
		| LOGFILE_SYMBOL GROUP_SYMBOL logfile_group_ref (drop_logfile_group_option (COMMA_SYMBOL? drop_logfile_group_option)*)?
		| SERVER_SYMBOL if_exists? server_ref
		| TEMPORARY_SYMBOL? (TABLE_SYMBOL | TABLES_SYMBOL) if_exists? table_ref_list (RESTRICT_SYMBOL | CASCADE_SYMBOL)?
		| TABLESPACE_SYMBOL tablespace_ref (drop_logfile_group_option (COMMA_SYMBOL? drop_logfile_group_option)*)?
		| TRIGGER_SYMBOL if_exists? trigger_ref
		| VIEW_SYMBOL if_exists? view_identifier_list (RESTRICT_SYMBOL | CASCADE_SYMBOL)?
	)
;

drop_logfile_group_option:
	(WAIT_SYMBOL | NO_WAIT_SYMBOL)
	| STORAGE_SYMBOL? ENGINE_SYMBOL EQUAL_OPERATOR? engine_ref;

//--------------------------------------------------------------------------------------------------

rename_table_statement:
	RENAME_SYMBOL (TABLE_SYMBOL | TABLES_SYMBOL)
		table_ref TO_SYMBOL table_name (COMMA_SYMBOL table_ref TO_SYMBOL table_name)*
;
	
//--------------------------------------------------------------------------------------------------

truncate_table_statement:
	TRUNCATE_SYMBOL TABLE_SYMBOL? table_ref
;

//--------------- DML statements -------------------------------------------------------------------

call_statement:
	CALL_SYMBOL procedure_ref optional_expression_list_with_parentheses?
;

delete_statement:
	DELETE_SYMBOL ( options { greedy = true; }: delete_option)*
		(
			FROM_SYMBOL
				// Both alternatives can start with identifier DOT.
				( options { k = 4; }:
					 table_ref_list_with_wildcard USING_SYMBOL join_table_list where_clause? // Multi table variant 1.
					| table_ref partition_delete? where_clause? order_by_clause? simple_limit_clause? // Single table delete.
				)
			|  table_ref_list_with_wildcard FROM_SYMBOL join_table_list where_clause? // Multi table variant 2.
		)
;

partition_delete:
	{SERVER_VERSION >= 50602}? => PARTITION_SYMBOL OPEN_PAR_SYMBOL identifier_list CLOSE_PAR_SYMBOL
;

delete_option:
	LOW_PRIORITY_SYMBOL | QUICK_SYMBOL | IGNORE_SYMBOL
;

do_statement:
	DO_SYMBOL 
	(
		{SERVER_VERSION < 50709}? => expression_list
		| {SERVER_VERSION >= 50709}? => select_item_list
	)
;

handler_statement:
	HANDLER_SYMBOL
	(  options { k = 4; }:
		table_ref OPEN_SYMBOL (AS_SYMBOL? identifier)?
		| table_ref_no_db
			(
				| CLOSE_SYMBOL
				| READ_SYMBOL handler_read_or_scan where_clause? limit_clause?
			)
	)
;

handler_read_or_scan:
	(FIRST_SYMBOL | NEXT_SYMBOL) // Scan function.
	| identifier
		(
			FIRST_SYMBOL
			| NEXT_SYMBOL
			| PREV_SYMBOL
			| LAST_SYMBOL
			| (EQUAL_OPERATOR | LESS_THAN_OPERATOR | GREATER_THAN_OPERATOR | LESS_OR_EQUAL_OPERATOR | GREATER_OR_EQUAL_OPERATOR)
				OPEN_PAR_SYMBOL values CLOSE_PAR_SYMBOL
		)
;

//--------------------------------------------------------------------------------------------------

insert_statement:
	INSERT_SYMBOL insert_lock_option? IGNORE_SYMBOL? INTO_SYMBOL? table_ref use_partition?
		insert_field_spec duplicate_key_update?
;

insert_lock_option:
	LOW_PRIORITY_SYMBOL
	| DELAYED_SYMBOL		// Only allowed if no select is used. Check in the semantic phase.
	| HIGH_PRIORITY_SYMBOL
;

insert_field_spec:
	(OPEN_PAR_SYMBOL fields? CLOSE_PAR_SYMBOL)? 
		(
			insert_values
			| insert_query_expression
		)
	| SET_SYMBOL column_assignment_list_with_default
;

fields:
	column_ref_with_wildcard (COMMA_SYMBOL column_ref_with_wildcard)*
;

insert_values:
	(VALUES_SYMBOL | VALUE_SYMBOL) insert_value_list
;

insert_query_expression:
	create_select union_clause?
	| OPEN_PAR_SYMBOL create_select CLOSE_PAR_SYMBOL union_or_order_by_or_limit?
;

insert_value_list:
	OPEN_PAR_SYMBOL values? CLOSE_PAR_SYMBOL (COMMA_SYMBOL OPEN_PAR_SYMBOL values? CLOSE_PAR_SYMBOL)*
;

values:
	(expression | DEFAULT_SYMBOL) (COMMA_SYMBOL (expression | DEFAULT_SYMBOL))*
;

duplicate_key_update:
	ON_SYMBOL DUPLICATE_SYMBOL KEY_SYMBOL UPDATE_SYMBOL column_assignment_list_with_default
;

//--------------------------------------------------------------------------------------------------

load_statement:
	LOAD_SYMBOL data_or_xml (LOW_PRIORITY_SYMBOL | CONCURRENT_SYMBOL)? LOCAL_SYMBOL? INFILE_SYMBOL string_literal
		(REPLACE_SYMBOL | IGNORE_SYMBOL)? INTO_SYMBOL TABLE_SYMBOL table_ref
		use_partition? charset_clause?
		xml_rows_identified_by?
		fields_clause? lines_clause?
		load_data_file_tail
;

data_or_xml:
	DATA_SYMBOL
	| {SERVER_VERSION >= 50500}? XML_SYMBOL
;

xml_rows_identified_by:
	{SERVER_VERSION >= 50500}? => ROWS_SYMBOL IDENTIFIED_SYMBOL BY_SYMBOL text_string
;

load_data_file_tail:
	(IGNORE_SYMBOL INT_NUMBER (LINES_SYMBOL | ROWS_SYMBOL))? load_data_file_target_list? (SET_SYMBOL column_assignment_list_with_default)?
;

load_data_file_target_list:
	OPEN_PAR_SYMBOL field_or_variable_list? CLOSE_PAR_SYMBOL
;

field_or_variable_list:
	(column_ref | user_variable) (COMMA_SYMBOL (column_ref | user_variable))*
;

//--------------------------------------------------------------------------------------------------

replace_statement:
	REPLACE_SYMBOL (LOW_PRIORITY_SYMBOL | DELAYED_SYMBOL)? INTO_SYMBOL? table_ref
		use_partition? insert_field_spec
;

//--------------------------------------------------------------------------------------------------

select_statement:
	SELECT_SYMBOL select_part2 union_clause?
	| OPEN_PAR_SYMBOL select_paren CLOSE_PAR_SYMBOL union_or_order_by_or_limit?
;

select_paren:
	SELECT_SYMBOL select_part2
	| OPEN_PAR_SYMBOL select_paren CLOSE_PAR_SYMBOL
;

select_from:
	from_clause where_clause? group_by_clause? having_clause? order_by_clause? limit_clause? procedure_analyse_clause?
;

select_part2:
	( options { greedy = true; }: select_option)* select_item_list
		(
			order_by_clause? limit_clause?
			| into_clause select_from?
			| select_from into_clause?
		)
		select_lock_type?
;

table_expression:
	from_clause?
	( options { greedy = true; }: where_clause)?
	( options { greedy = true; }: group_by_clause)?
	( options { greedy = true; }: having_clause)?
	( options { greedy = true; }: order_by_clause)?
	( options { greedy = true; }: limit_clause)?
	( options { greedy = true; }: procedure_analyse_clause)?
	( options { greedy = true; }: select_lock_type)?
;

subquery: // subselect in sql_yacc.yy, this is always wrapped by parentheses so I moved them into this rule.
	OPEN_PAR_SYMBOL query_expression_body CLOSE_PAR_SYMBOL
;

query_expression_body:
	query_specification (UNION_SYMBOL union_option query_specification)*
;

select_part2_derived: // select_part2 equivalent for sub queries.
	( options { greedy = true; }:
		query_spec_option
		| {SERVER_VERSION <= 50100}? (SQL_NO_CACHE_SYMBOL | SQL_CACHE_SYMBOL)
	)* select_item_list
;

select_option:
	query_spec_option
	| SQL_NO_CACHE_SYMBOL
	| SQL_CACHE_SYMBOL
	| {SERVER_VERSION >= 50704 && SERVER_VERSION < 50708}? MAX_STATEMENT_TIME_SYMBOL EQUAL_OPERATOR real_ulong_number
;

query_spec_option:
	(ALL_SYMBOL | DISTINCT_SYMBOL)
	| STRAIGHT_JOIN_SYMBOL
	| HIGH_PRIORITY_SYMBOL
	| SQL_SMALL_RESULT_SYMBOL
	| SQL_BIG_RESULT_SYMBOL
	| SQL_BUFFER_RESULT_SYMBOL
	| SQL_CALC_FOUND_ROWS_SYMBOL
;

select_item_list:
	(select_item | MULT_OPERATOR) ( options { greedy = true; }: COMMA_SYMBOL select_item)*
;

select_item:
	(table_wild) => table_wild
	| expression select_alias?
;

select_alias:
	AS_SYMBOL? (identifier | text_string )
;         

limit_clause:
	LIMIT_SYMBOL limit_options
;

simple_limit_clause:
	LIMIT_SYMBOL limit_option
;

limit_options:
	limit_option ( options { greedy = true; }: (COMMA_SYMBOL | OFFSET_SYMBOL) limit_option)?
;

limit_option:
	identifier
	| PARAM_MARKER
	| ULONGLONG_NUMBER
	| LONG_NUMBER
	| INT_NUMBER
;

into_clause:
	INTO_SYMBOL
	(
		OUTFILE_SYMBOL string_literal charset_clause? fields_clause? lines_clause?
		| DUMPFILE_SYMBOL string_literal
		| AT_SIGN_SYMBOL? (text_or_identifier | AT_TEXT_SUFFIX) (COMMA_SYMBOL AT_SIGN_SYMBOL? (text_or_identifier | AT_TEXT_SUFFIX))*
	)
;

procedure_analyse_clause:
	PROCEDURE_SYMBOL ANALYSE_SYMBOL OPEN_PAR_SYMBOL (INT_NUMBER (COMMA_SYMBOL INT_NUMBER)?)? CLOSE_PAR_SYMBOL
;

having_clause:
	HAVING_SYMBOL expression
;

group_by_clause:
	GROUP_SYMBOL BY_SYMBOL expression_list_with_direction olap_option?
;

olap_option:
	WITH_SYMBOL CUBE_SYMBOL
	| WITH_SYMBOL ROLLUP_SYMBOL
;

order_by_clause:
	ORDER_SYMBOL BY_SYMBOL expression_list_with_direction
;

order_by_or_limit:
	order_by_clause limit_clause?
	| limit_clause
;

direction:
	ASC_SYMBOL
	| DESC_SYMBOL
;

from_clause:
	FROM_SYMBOL table_reference_list
;

where_clause:
	WHERE_SYMBOL expression
;

table_reference_list:
	join_table_list
	| DUAL_SYMBOL
;

join_table_list: // join_table_list + derived_table_list in sql_yacc.yy.
	 escaped_table_reference ( options { greedy = true; }: COMMA_SYMBOL escaped_table_reference)*
;

// For the ODBC OJ syntax we do as the server does. This is what the server grammar says about it:
//   The ODBC escape syntax for Outer Join is: '{' OJ join_table '}'
//   The parser does not define OJ as a token, any ident is accepted
//   instead in $2 (ident). Also, all productions from table_ref can
//   be escaped, not only join_table. Both syntax extensions are safe
//   and are ignored.
escaped_table_reference:
	table_reference
	| OPEN_CURLY_SYMBOL identifier table_reference CLOSE_CURLY_SYMBOL
;

table_reference: // table_ref in sql_yacc.yy, we use table_ref here for a different rule.
	table_factor ( options { greedy = true; }: join)*
;

join:
	join_table
;

table_factor:
	SELECT_SYMBOL ( options { greedy = true; }: select_option)* select_item_list table_expression
	| OPEN_PAR_SYMBOL select_table_factor_union CLOSE_PAR_SYMBOL table_alias?
	| table_ref use_partition? table_alias? index_hint_list?
;

select_table_factor_union:
	(table_reference_list order_by_or_limit?) (UNION_SYMBOL union_option? query_specification)*
;

query_specification:
	SELECT_SYMBOL select_part2_derived table_expression
	| OPEN_PAR_SYMBOL select_paren_derived CLOSE_PAR_SYMBOL order_by_or_limit?
;

select_paren_derived:
	SELECT_SYMBOL select_part2_derived table_expression
	| OPEN_PAR_SYMBOL select_paren_derived CLOSE_PAR_SYMBOL
;

join_table: // Like the same named rule in sql_yacc.yy but with removed left recursion.
	(INNER_SYMBOL | CROSS_SYMBOL)? JOIN_SYMBOL table_reference
		( options { greedy = true; }:
			ON_SYMBOL expression
			| USING_SYMBOL identifier_list_with_parentheses
		)?
	| STRAIGHT_JOIN_SYMBOL table_factor ( options {greedy = true;}: ON_SYMBOL expression)?
	| (LEFT_SYMBOL | RIGHT_SYMBOL) OUTER_SYMBOL? JOIN_SYMBOL table_factor
		(
			join* ON_SYMBOL expression
			| USING_SYMBOL identifier_list_with_parentheses
		)
	| NATURAL_SYMBOL ((LEFT_SYMBOL | RIGHT_SYMBOL) OUTER_SYMBOL?)? JOIN_SYMBOL table_factor
;

join_condition:
	ON_SYMBOL expression
	| USING_SYMBOL identifier_list_with_parentheses
;

union_clause:
  UNION_SYMBOL union_option? select_statement
 ;

union_option:
	DISTINCT_SYMBOL
	| ALL_SYMBOL
;

union_or_order_by_or_limit: // union_opt in sql_yacc.yy
	union_clause
	| order_by_or_limit
;

select_lock_type:
	FOR_SYMBOL UPDATE_SYMBOL
	| LOCK_SYMBOL IN_SYMBOL SHARE_SYMBOL MODE_SYMBOL
;

table_alias:
	(AS_SYMBOL | EQUAL_OPERATOR)? identifier
;

index_hint_list:
	index_hint (COMMA_SYMBOL index_hint)*
;

index_hint:
	index_hint_type key_or_index index_hint_clause? OPEN_PAR_SYMBOL index_list CLOSE_PAR_SYMBOL
	| USE_SYMBOL key_or_index index_hint_clause? OPEN_PAR_SYMBOL index_list? CLOSE_PAR_SYMBOL
;

index_hint_type:
	FORCE_SYMBOL
	| IGNORE_SYMBOL
;

key_or_index:
	KEY_SYMBOL
	| INDEX_SYMBOL
;

index_hint_clause:
	FOR_SYMBOL (JOIN_SYMBOL | ORDER_SYMBOL BY_SYMBOL | GROUP_SYMBOL BY_SYMBOL)
;

index_list:
	index_list_element (COMMA_SYMBOL index_list_element)*
;

index_list_element:
	identifier
	| PRIMARY_SYMBOL
;

//--------------------------------------------------------------------------------------------------

update_statement:
	UPDATE_SYMBOL LOW_PRIORITY_SYMBOL? IGNORE_SYMBOL? join_table_list
		SET_SYMBOL column_assignment_list_with_default where_clause? order_by_clause? simple_limit_clause?
;

//--------------------------------------------------------------------------------------------------

transaction_or_locking_statement:
	transaction_statement
	| savepoint_statement
	| lock_statement
	| xa_statement
;

transaction_statement:
	START_SYMBOL TRANSACTION_SYMBOL transaction_characteristic*
	| COMMIT_SYMBOL WORK_SYMBOL? (AND_SYMBOL NO_SYMBOL? CHAIN_SYMBOL)? (NO_SYMBOL? RELEASE_SYMBOL)?
	| ROLLBACK_SYMBOL WORK_SYMBOL?
		(
			(AND_SYMBOL NO_SYMBOL? CHAIN_SYMBOL)? (NO_SYMBOL? RELEASE_SYMBOL)?
			| TO_SYMBOL SAVEPOINT_SYMBOL? identifier // Belongs to the savepoint_statement, but this way we don't need a predicate.
		)
	// In order to avoid needing a predicate to solve ambiquity between this and general SET statements with global/session variables the following
	// alternative is moved to the set_statement rule.
	//| SET_SYMBOL option_type? TRANSACTION_SYMBOL set_transaction_characteristic (COMMA_SYMBOL set_transaction_characteristic)*
;

// BEGIN WORK is separated from transactional statements as it must not appear as part of a stored program.
begin_work:
	BEGIN_SYMBOL WORK_SYMBOL?
;

transaction_characteristic:
	WITH_SYMBOL CONSISTENT_SYMBOL SNAPSHOT_SYMBOL
	| {SERVER_VERSION >= 50605}? READ_SYMBOL (WRITE_SYMBOL | ONLY_SYMBOL)
;

set_transaction_characteristic:
	ISOLATION_SYMBOL LEVEL_SYMBOL isolation_level
	| {SERVER_VERSION >= 50605}? READ_SYMBOL (WRITE_SYMBOL | ONLY_SYMBOL)
;

isolation_level:
	REPEATABLE_SYMBOL READ_SYMBOL
	| READ_SYMBOL (COMMITTED_SYMBOL | UNCOMMITTED_SYMBOL)
	| SERIALIZABLE_SYMBOL
;

savepoint_statement:
	SAVEPOINT_SYMBOL identifier
	| RELEASE_SYMBOL SAVEPOINT_SYMBOL identifier
;

lock_statement:
	LOCK_SYMBOL (TABLES_SYMBOL | TABLE_SYMBOL) lock_item (COMMA_SYMBOL lock_item)*
	| UNLOCK_SYMBOL (TABLES_SYMBOL | TABLE_SYMBOL)
;

lock_item:
	table_ref table_alias? lock_option
;

lock_option:
	READ_SYMBOL LOCAL_SYMBOL?
	| LOW_PRIORITY_SYMBOL? WRITE_SYMBOL // low priority deprecated since 5.7
;

xa_statement:
	XA_SYMBOL
		(
			(START_SYMBOL | BEGIN_SYMBOL) xid (JOIN_SYMBOL | RESUME_SYMBOL)?
			| END_SYMBOL xid (SUSPEND_SYMBOL (FOR_SYMBOL MIGRATE_SYMBOL)?)?
			| PREPARE_SYMBOL xid
			| COMMIT_SYMBOL xid (ONE_SYMBOL PHASE_SYMBOL)?
			| ROLLBACK_SYMBOL xid
			| RECOVER_SYMBOL xa_convert
		)
;

xa_convert:
	{SERVER_VERSION >= 50704}? (CONVERT_SYMBOL XID_SYMBOL)?
	| /* empty */
;

xid:
	text_string (COMMA_SYMBOL text_string (COMMA_SYMBOL ulong_number)?)?
;

//--------------------------------------------------------------------------------------------------

replication_statement:
	PURGE_SYMBOL (BINARY_SYMBOL | MASTER_SYMBOL) LOGS_SYMBOL (TO_SYMBOL string_literal | BEFORE_SYMBOL expression)
	| change_master
	| {SERVER_VERSION >= 50700}? change_replication
	/* Defined in the miscellaneous statement to avoid ambiguities.
	| RESET_SYMBOL MASTER_SYMBOL
	| RESET_SYMBOL SLAVE_SYMBOL ALL_SYMBOL
	*/
	| slave
	| {SERVER_VERSION < 50500}? replication_load
	| {SERVER_VERSION > 50706}? group_replication
;

replication_load:
	LOAD_SYMBOL (DATA_SYMBOL | TABLE_SYMBOL table_ref) FROM_SYMBOL MASTER_SYMBOL
;

change_master:
	CHANGE_SYMBOL MASTER_SYMBOL TO_SYMBOL change_master_options channel?
;

change_master_options:
	master_option (COMMA_SYMBOL master_option)*
;

master_option:
	MASTER_HOST_SYMBOL EQUAL_OPERATOR text_string_no_linebreak
	| MASTER_BIND_SYMBOL EQUAL_OPERATOR text_string_no_linebreak
	| MASTER_USER_SYMBOL EQUAL_OPERATOR text_string_no_linebreak
	| MASTER_PASSWORD_SYMBOL EQUAL_OPERATOR text_string_no_linebreak
	| MASTER_PORT_SYMBOL EQUAL_OPERATOR ulong_number
	| MASTER_CONNECT_RETRY_SYMBOL EQUAL_OPERATOR ulong_number
	| MASTER_RETRY_COUNT_SYMBOL EQUAL_OPERATOR ulong_number
	| MASTER_DELAY_SYMBOL EQUAL_OPERATOR ulong_number
	| MASTER_SSL_SYMBOL EQUAL_OPERATOR ulong_number
	| MASTER_SSL_CA_SYMBOL EQUAL_OPERATOR text_string_no_linebreak
	| MASTER_SSL_CAPATH_SYMBOL EQUAL_OPERATOR text_string_no_linebreak
	| MASTER_SSL_CERT_SYMBOL EQUAL_OPERATOR text_string_no_linebreak
	| MASTER_SSL_CIPHER_SYMBOL EQUAL_OPERATOR text_string_no_linebreak
	| MASTER_SSL_KEY_SYMBOL EQUAL_OPERATOR text_string_no_linebreak
	| MASTER_SSL_VERIFY_SERVER_CERT_SYMBOL EQUAL_OPERATOR ulong_number
	| MASTER_SSL_CRL_SYMBOL EQUAL_OPERATOR string_literal
	| MASTER_SSL_CRLPATH_SYMBOL EQUAL_OPERATOR text_string_no_linebreak
	| MASTER_HEARTBEAT_PERIOD_SYMBOL EQUAL_OPERATOR ulong_number
	| IGNORE_SERVER_IDS_SYMBOL EQUAL_OPERATOR server_id_list
	| MASTER_AUTO_POSITION_SYMBOL EQUAL_OPERATOR ulong_number
	| master_file_def
;

master_file_def:
	MASTER_LOG_FILE_SYMBOL EQUAL_OPERATOR text_string_no_linebreak
	| MASTER_LOG_POS_SYMBOL EQUAL_OPERATOR ulonglong_number
	| RELAY_LOG_FILE_SYMBOL EQUAL_OPERATOR text_string_no_linebreak
	| RELAY_LOG_POS_SYMBOL EQUAL_OPERATOR ulong_number
;

server_id_list:
	OPEN_PAR_SYMBOL (ulong_number (COMMA_SYMBOL ulong_number)*)? CLOSE_PAR_SYMBOL
;

change_replication:
	CHANGE_SYMBOL REPLICATION_SYMBOL FILTER_SYMBOL filter_definition (COMMA_SYMBOL filter_definition)*
;

filter_definition:
	REPLICATE_DO_DB_SYMBOL EQUAL_OPERATOR OPEN_PAR_SYMBOL filter_db_list? CLOSE_PAR_SYMBOL
	| REPLICATE_IGNORE_DB_SYMBOL EQUAL_OPERATOR OPEN_PAR_SYMBOL filter_db_list? CLOSE_PAR_SYMBOL
	| REPLICATE_DO_TABLE_SYMBOL EQUAL_OPERATOR OPEN_PAR_SYMBOL filter_table_list? CLOSE_PAR_SYMBOL
	| REPLICATE_IGNORE_TABLE_SYMBOL EQUAL_OPERATOR OPEN_PAR_SYMBOL filter_table_list? CLOSE_PAR_SYMBOL
	| REPLICATE_WILD_DO_TABLE_SYMBOL EQUAL_OPERATOR OPEN_PAR_SYMBOL filter_string_list? CLOSE_PAR_SYMBOL
	| REPLICATE_WILD_IGNORE_TABLE_SYMBOL EQUAL_OPERATOR OPEN_PAR_SYMBOL filter_string_list? CLOSE_PAR_SYMBOL
	| REPLICATE_REWRITE_DB_SYMBOL EQUAL_OPERATOR OPEN_PAR_SYMBOL filter_db_pair_list? CLOSE_PAR_SYMBOL
;

filter_db_list:
	schema_ref (COMMA_SYMBOL schema_ref)*
;

filter_table_list:
	filter_table_ref (COMMA_SYMBOL filter_table_ref)*
;

filter_string_list:
	filter_wild_db_table_string (COMMA_SYMBOL filter_wild_db_table_string)*
;

filter_wild_db_table_string:
	text_string_no_linebreak // sql_yacc.yy checks for the existance of at least one dot char in the string.
;

filter_db_pair_list:
	schema_identifier_pair (COMMA_SYMBOL schema_identifier_pair)*
;

slave:
	START_SYMBOL SLAVE_SYMBOL slave_thread_options? (UNTIL_SYMBOL slave_until_options)? slave_connection_options channel?
	| STOP_SYMBOL SLAVE_SYMBOL slave_thread_options? channel?
;

slave_until_options:
	(
		master_file_def
		| {SERVER_VERSION >= 50606}? (SQL_BEFORE_GTIDS_SYMBOL | SQL_AFTER_GTIDS_SYMBOL) EQUAL_OPERATOR text_string
		| {SERVER_VERSION >= 50606}? SQL_AFTER_MTS_GAPS_SYMBOL
	)
	(COMMA_SYMBOL master_file_def)*
;

slave_connection_options:
	{SERVER_VERSION >= 50604}? (USER_SYMBOL EQUAL_OPERATOR text_string)? (PASSWORD_SYMBOL EQUAL_OPERATOR text_string)?
		(DEFAULT_AUTH_SYMBOL EQUAL_OPERATOR text_string)? (PLUGIN_DIR_SYMBOL EQUAL_OPERATOR text_string)?
	| /* empty */
;

slave_thread_options:
	slave_thread_option (COMMA_SYMBOL slave_thread_option)*
;

slave_thread_option:
	RELAY_THREAD_SYMBOL | SQL_THREAD_SYMBOL
;

group_replication:
	(START_SYMBOL | STOP_SYMBOL) GROUP_REPLICATION_SYMBOL
;

//--------------------------------------------------------------------------------------------------

prepared_statement:
	PREPARE_SYMBOL identifier FROM_SYMBOL (string_literal | user_variable)
	| execute_statement
	| (DEALLOCATE_SYMBOL | DROP_SYMBOL) PREPARE_SYMBOL identifier
;

execute_statement:
	EXECUTE_SYMBOL identifier (USING_SYMBOL execute_var_list)?
;

execute_var_list:
	user_variable (COMMA_SYMBOL user_variable)*
;

//--------------------------------------------------------------------------------------------------

account_management_statement:
	{SERVER_VERSION >= 50606}? alter_user
	| create_user
	| drop_user
	| {SERVER_VERSION >= 50500}? grant_proxy
	| grant
	| rename_user
	| revoke_statement
	| set_password
;

alter_user:
	ALTER_SYMBOL USER_SYMBOL ({SERVER_VERSION >= 50706}? if_exists | /* empty */) alter_user_tail
;

alter_user_tail:
	grant_list create_user_tail
	| {SERVER_VERSION >= 50706}? USER_SYMBOL parentheses IDENTIFIED_SYMBOL BY_SYMBOL text_string
;

create_user:
	CREATE_SYMBOL USER_SYMBOL ({SERVER_VERSION >= 50706}? if_not_exists | /* empty */) grant_list create_user_tail
;

create_user_tail:
	{SERVER_VERSION >= 50706}? require_clause? connect_options? account_lock_password_expire_options?
	| /* empty */
;

require_clause:
	REQUIRE_SYMBOL (require_list | SSL_SYMBOL | X509_SYMBOL | NONE_SYMBOL)
;

connect_options:
	WITH_SYMBOL
	(
		MAX_QUERIES_PER_HOUR_SYMBOL ulong_number
		| MAX_UPDATES_PER_HOUR_SYMBOL ulong_number
		| MAX_CONNECTIONS_PER_HOUR_SYMBOL ulong_number
		| MAX_USER_CONNECTIONS_SYMBOL ulong_number
	)+
;

account_lock_password_expire_options:
	ACCOUNT_SYMBOL (LOCK_SYMBOL | UNLOCK_SYMBOL)
	| PASSWORD_SYMBOL EXPIRE_SYMBOL
		(
			INTERVAL_SYMBOL real_ulong_number DAY_SYMBOL
			| NEVER_SYMBOL
			| DEFAULT_SYMBOL
		)	
;

drop_user:
	DROP_SYMBOL USER_SYMBOL ({SERVER_VERSION >= 50706}? if_exists | /* empty */) user_list
;

parse_grant: // For external use only. Don't reference this in the normal grammar.
	grant EOF
;

grant:
	GRANT_SYMBOL grant_privileges privilege_target
		TO_SYMBOL grant_list require_clause? (WITH_SYMBOL grant_option+)?
;

grant_proxy:
	GRANT_SYMBOL PROXY_SYMBOL ON_SYMBOL grant_user TO_SYMBOL grant_user (COMMA_SYMBOL grant_user)*
		(WITH_SYMBOL GRANT_SYMBOL OPTION_SYMBOL)?
;

rename_user:
	RENAME_SYMBOL USER_SYMBOL user TO_SYMBOL user (COMMA_SYMBOL user TO_SYMBOL user)*
;

revoke_statement:
	REVOKE_SYMBOL
	(
		{LA(1) == ALL_SYMBOL}? => ALL_SYMBOL PRIVILEGES_SYMBOL? COMMA_SYMBOL GRANT_SYMBOL OPTION_SYMBOL FROM_SYMBOL user_list
		| grant_privileges privilege_target FROM_SYMBOL user_list
		| {SERVER_VERSION >= 50500}? PROXY_SYMBOL ON_SYMBOL user FROM_SYMBOL user_list
	)
;

privilege_target:
	ON_SYMBOL grant_object_type privilege_level
;

set_password:
	SET_SYMBOL PASSWORD_SYMBOL (FOR_SYMBOL user)? equal
	(
		PASSWORD_SYMBOL OPEN_PAR_SYMBOL text_string CLOSE_PAR_SYMBOL
		| {SERVER_VERSION < 50706}? OLD_PASSWORD_SYMBOL OPEN_PAR_SYMBOL text_string CLOSE_PAR_SYMBOL
		| text_string
	)
;

grant_object_type:
	(TABLE_SYMBOL? | FUNCTION_SYMBOL | PROCEDURE_SYMBOL)
;

grant_privileges:
	ALL_SYMBOL PRIVILEGES_SYMBOL?
	| privilege_type (COMMA_SYMBOL privilege_type)*
;

privilege_type:
	ALTER_SYMBOL ROUTINE_SYMBOL?
	| CREATE_SYMBOL (ROUTINE_SYMBOL | TABLESPACE_SYMBOL | TEMPORARY_SYMBOL TABLES_SYMBOL | USER_SYMBOL | VIEW_SYMBOL)?
	| DELETE_SYMBOL
	| DROP_SYMBOL
	| EVENT_SYMBOL
	| EXECUTE_SYMBOL
	| FILE_SYMBOL
	| GRANT_SYMBOL OPTION_SYMBOL
	| INDEX_SYMBOL
	| INSERT_SYMBOL identifier_list_with_parentheses?
	| LOCK_SYMBOL TABLES_SYMBOL
	| PROCESS_SYMBOL
	| PROXY_SYMBOL
	| REFERENCES_SYMBOL identifier_list_with_parentheses?
	| RELOAD_SYMBOL
	| REPLICATION_SYMBOL (CLIENT_SYMBOL | SLAVE_SYMBOL)
	| SELECT_SYMBOL identifier_list_with_parentheses?
	| SHOW_SYMBOL DATABASES_SYMBOL
	| SHOW_SYMBOL VIEW_SYMBOL
	| SHUTDOWN_SYMBOL
	| SUPER_SYMBOL
	| TRIGGER_SYMBOL
	| UPDATE_SYMBOL identifier_list_with_parentheses?
	| USAGE_SYMBOL
;

privilege_level:
	MULT_OPERATOR (DOT_SYMBOL MULT_OPERATOR)?
	| identifier (DOT_SYMBOL MULT_OPERATOR | dot_identifier)?
;

require_list:
	require_list_element (AND_SYMBOL? require_list_element)*
;

require_list_element:
	CIPHER_SYMBOL text_string
	| ISSUER_SYMBOL text_string
	| SUBJECT_SYMBOL text_string
;

grant_option:
	GRANT_SYMBOL OPTION_SYMBOL
	| MAX_QUERIES_PER_HOUR_SYMBOL ulong_number
	| MAX_UPDATES_PER_HOUR_SYMBOL ulong_number
	| MAX_CONNECTIONS_PER_HOUR_SYMBOL ulong_number
	| MAX_USER_CONNECTIONS_SYMBOL ulong_number
;

//--------------------------------------------------------------------------------------------------

table_administration_statement:
	ANALYZE_SYMBOL no_write_to_bin_log? TABLE_SYMBOL table_ref_list
	| CHECK_SYMBOL TABLE_SYMBOL table_ref_list check_option*
	| CHECKSUM_SYMBOL TABLE_SYMBOL table_ref_list (QUICK_SYMBOL | EXTENDED_SYMBOL)?
	| OPTIMIZE_SYMBOL no_write_to_bin_log? TABLE_SYMBOL table_ref_list
	| REPAIR_SYMBOL no_write_to_bin_log? TABLE_SYMBOL table_ref_list repair_option*
	| {SERVER_VERSION < 50500}? BACKUP_SYMBOL TABLE_SYMBOL table_ref_list TO_SYMBOL string_literal
	| {SERVER_VERSION < 50500}? RESTORE_SYMBOL TABLE_SYMBOL table_ref_list FROM_SYMBOL string_literal
;

check_option:
	FOR_SYMBOL UPGRADE_SYMBOL | QUICK_SYMBOL | FAST_SYMBOL | MEDIUM_SYMBOL | EXTENDED_SYMBOL | CHANGED_SYMBOL
;

repair_option:
	QUICK_SYMBOL | EXTENDED_SYMBOL | USE_FRM_SYMBOL
;

//--------------------------------------------------------------------------------------------------

install_uninstall_statment:
	INSTALL_SYMBOL PLUGIN_SYMBOL identifier SONAME_SYMBOL string_literal
	| UNINSTALL_SYMBOL PLUGIN_SYMBOL identifier
;

//--------------------------------------------------------------------------------------------------

set_statement:
	SET_SYMBOL
		( options { k = 3; }:
			 option_type? TRANSACTION_SYMBOL set_transaction_characteristic
			// ONE_SHOT is available only until 5.6. We don't need a predicate here, however. Handling it in the lexer is enough.
			| ONE_SHOT_SYMBOL? option_value_no_option_type (COMMA_SYMBOL option_value_list)?
			| option_type option_value_following_option_type (COMMA_SYMBOL option_value_list)?
			
			// SET PASSWORD is handled in an own rule.
		)
;

option_value_no_option_type:
	{LA(1) == NAMES_SYMBOL}? NAMES_SYMBOL
		(
			equal expression
			| charset_name_or_default (COLLATE_SYMBOL collation_name_or_default)?
		)
	| variable_name equal set_expression_or_default
	| user_variable equal expression
	| system_variable equal set_expression_or_default
	| charset_clause
;

option_value_following_option_type:
	variable_name equal set_expression_or_default
;
	
set_expression_or_default:
	expression
	| DEFAULT_SYMBOL
	| ON_SYMBOL
	| ALL_SYMBOL
	| BINARY_SYMBOL
;

option_value_list:
	option_value (COMMA_SYMBOL option_value)*
;

option_value:
	option_type variable_name equal set_expression_or_default
	| option_value_no_option_type
;

//--------------------------------------------------------------------------------------------------

show_statement:
	SHOW_SYMBOL
	(
		{SERVER_VERSION < 50700}? AUTHORS_SYMBOL
		| DATABASES_SYMBOL like_or_where?
		| FULL_SYMBOL? TABLES_SYMBOL in_db? like_or_where?
		| FULL_SYMBOL? TRIGGERS_SYMBOL in_db? like_or_where?
		| EVENTS_SYMBOL in_db? like_or_where?
		| TABLE_SYMBOL STATUS_SYMBOL in_db? like_or_where?
		| OPEN_SYMBOL TABLES_SYMBOL in_db? like_or_where?
		| {(SERVER_VERSION >= 50105) && (SERVER_VERSION < 50500)}? PLUGIN_SYMBOL // Supported between 5.1.5 and 5.5.0.
		| {SERVER_VERSION >= 50500}? PLUGINS_SYMBOL
		| ENGINE_SYMBOL engine_ref (STATUS_SYMBOL | MUTEX_SYMBOL)
		| FULL_SYMBOL? COLUMNS_SYMBOL (FROM_SYMBOL | IN_SYMBOL) table_ref in_db? like_or_where?
		| (BINARY_SYMBOL | MASTER_SYMBOL) LOGS_SYMBOL
		| SLAVE_SYMBOL
			(
				HOSTS_SYMBOL
				| STATUS_SYMBOL non_blocking channel?
			)
		| (BINLOG_SYMBOL | RELAYLOG_SYMBOL) EVENTS_SYMBOL (IN_SYMBOL text_string)? (FROM_SYMBOL ulonglong_number)? limit_clause? channel?
		| (INDEX_SYMBOL | INDEXES_SYMBOL | KEYS_SYMBOL) from_or_in table_ref in_db? where_clause?
		| STORAGE_SYMBOL? ENGINES_SYMBOL
		| PRIVILEGES_SYMBOL
		| COUNT_SYMBOL OPEN_PAR_SYMBOL MULT_OPERATOR CLOSE_PAR_SYMBOL (WARNINGS_SYMBOL | ERRORS_SYMBOL)
		| WARNINGS_SYMBOL limit_clause?
		| ERRORS_SYMBOL limit_clause?
		| PROFILES_SYMBOL
		| PROFILE_SYMBOL (profile_type (COMMA_SYMBOL profile_type)*)? (FOR_SYMBOL QUERY_SYMBOL INT_NUMBER)? limit_clause?
		| option_type? (STATUS_SYMBOL | VARIABLES_SYMBOL) like_or_where?
		| FULL_SYMBOL? PROCESSLIST_SYMBOL
		| charset like_or_where?
		| COLLATION_SYMBOL like_or_where?
		| {SERVER_VERSION < 50700}? CONTRIBUTORS_SYMBOL
		| GRANTS_SYMBOL (FOR_SYMBOL user)?
		| MASTER_SYMBOL STATUS_SYMBOL
		| CREATE_SYMBOL
			(
				DATABASE_SYMBOL if_not_exists? schema_ref
				| EVENT_SYMBOL event_ref
				| FUNCTION_SYMBOL function_ref
				| PROCEDURE_SYMBOL procedure_ref
				| TABLE_SYMBOL table_ref
				| TRIGGER_SYMBOL trigger_ref
				| VIEW_SYMBOL view_ref
				| {SERVER_VERSION >= 50704}? USER_SYMBOL user
			)
		| PROCEDURE_SYMBOL STATUS_SYMBOL like_or_where?
		| FUNCTION_SYMBOL STATUS_SYMBOL like_or_where?
		| PROCEDURE_SYMBOL CODE_SYMBOL procedure_ref
		| FUNCTION_SYMBOL CODE_SYMBOL function_ref
		| {SERVER_VERSION < 50500}? INNODB_SYMBOL STATUS_SYMBOL // Deprecated in 5.5.
	)
;

non_blocking:
	{SERVER_VERSION >= 50700 && SERVER_VERSION < 50706}? NONBLOCKING_SYMBOL?
	| /* empty */
;

from_or_in:
	FROM_SYMBOL | IN_SYMBOL
;

in_db:
	from_or_in identifier
;

profile_type:
	ALL_SYMBOL
	| BLOCK_SYMBOL IO_SYMBOL
	| CONTEXT_SYMBOL SWITCHES_SYMBOL
	| CPU_SYMBOL
	| IPC_SYMBOL
	| MEMORY_SYMBOL
	| PAGE_SYMBOL FAULTS_SYMBOL
	| SOURCE_SYMBOL
	| SWAPS_SYMBOL
;

//--------------------------------------------------------------------------------------------------

other_administrative_statement:
	BINLOG_SYMBOL string_literal
	| CACHE_SYMBOL INDEX_SYMBOL key_cache_list_or_parts IN_SYMBOL (identifier | DEFAULT_SYMBOL)
	| FLUSH_SYMBOL no_write_to_bin_log?
		(
			flush_tables
			| flush_option (COMMA_SYMBOL flush_option)*
		)
	| KILL_SYMBOL  (options { greedy = true; }: (CONNECTION_SYMBOL | QUERY_SYMBOL))? expression
	| LOAD_SYMBOL INDEX_SYMBOL INTO_SYMBOL CACHE_SYMBOL load_table_index_list
	| RESET_SYMBOL reset_option (COMMA_SYMBOL reset_option)*
	| {SERVER_VERSION >= 50709}? SHUTDOWN_SYMBOL
;

key_cache_list_or_parts options { k = 4; }:
	key_cache_list
	| assign_to_keycache_partition
;

key_cache_list:
	assign_to_keycache (COMMA_SYMBOL assign_to_keycache)*
;

assign_to_keycache:
	table_ref cache_keys_spec?
;

assign_to_keycache_partition:
	table_ref PARTITION_SYMBOL OPEN_PAR_SYMBOL (ALL_SYMBOL | identifier_list) CLOSE_PAR_SYMBOL cache_keys_spec?
;

cache_keys_spec:
	(KEY_SYMBOL | INDEX_SYMBOL) OPEN_PAR_SYMBOL (key_usage_element (COMMA_SYMBOL key_usage_element)*)? CLOSE_PAR_SYMBOL
;

key_usage_element:
	identifier
	| PRIMARY_SYMBOL
;

flush_option:
	DES_KEY_FILE_SYMBOL
	| HOSTS_SYMBOL
	| log_type? LOGS_SYMBOL
	| RELAY_SYMBOL LOGS_SYMBOL channel?
	| PRIVILEGES_SYMBOL
	| QUERY_SYMBOL CACHE_SYMBOL
	| STATUS_SYMBOL
	| USER_RESOURCES_SYMBOL
	| {SERVER_VERSION >= 50706}? OPTIMIZER_COSTS_SYMBOL
;

log_type:
	BINARY_SYMBOL
	| ENGINE_SYMBOL
	| ERROR_SYMBOL
	| GENERAL_SYMBOL
	| SLOW_SYMBOL
;

flush_tables:
	(TABLES_SYMBOL | TABLE_SYMBOL)
	(
		WITH_SYMBOL READ_SYMBOL LOCK_SYMBOL
		| identifier_list flush_tables_options?
	)?
;

flush_tables_options:
	{SERVER_VERSION >= 50606}? FOR_SYMBOL EXPORT_SYMBOL
	| WITH_SYMBOL READ_SYMBOL LOCK_SYMBOL
;

load_table_index_list:
	table_ref load_table_index_partion?
		((INDEX_SYMBOL | KEY_SYMBOL)? identifier_list_with_parentheses)? (IGNORE_SYMBOL LEAVES_SYMBOL)?
;

load_table_index_partion:
	{SERVER_VERSION >= 50500}? => (PARTITION_SYMBOL OPEN_PAR_SYMBOL (identifier_list | ALL_SYMBOL) CLOSE_PAR_SYMBOL)
;

reset_option:
	MASTER_SYMBOL
	| QUERY_SYMBOL CACHE_SYMBOL
	| SLAVE_SYMBOL ALL_SYMBOL? channel?
;

//--------------------------------------------------------------------------------------------------

utility_statement:
	describe_command
		(
			table_ref (text_string | identifier)?
			|
				(
					// The format specifier is defined here like in the server grammar but actually defined are only
					// traditional and json, anything else results in a server error.
					EXTENDED_SYMBOL // deprecated since 5.7
					| {SERVER_VERSION >= 50105}? PARTITIONS_SYMBOL // deprecated since 5.7
					| {SERVER_VERSION >= 50605}? FORMAT_SYMBOL EQUAL_OPERATOR text_or_identifier
				)? explainable_statement
		)
	| HELP_SYMBOL text_or_identifier
	| use_command
;

describe_command:
	DESCRIBE_SYMBOL | DESC_SYMBOL
;

// Before server version 5.6 only select statements were explainable.
explainable_statement:
	select_statement
	| {SERVER_VERSION >= 50603}?
		(
			delete_statement
			| insert_statement
			| replace_statement
			| update_statement
		)
	| {SERVER_VERSION >= 50700}? FOR_SYMBOL CONNECTION_SYMBOL real_ulong_number
;

use_command:
	USE_SYMBOL identifier
;

//----------------- Expression support -------------------------------------------------------------

// The expression grammar here is completely different to that of the server parser due to the way operator
// precedence is implemented there (by custom tree writing). We do it the classical way here.

expression:
	logical_or_expression
;

logical_or_expression:
	logical_xor_expression ( options { greedy = true; }: (LOGICAL_OR_OPERATOR | OR_SYMBOL) logical_xor_expression)*
;

logical_xor_expression:
	logical_and_expression ( options { greedy = true; }: XOR_SYMBOL logical_and_expression)*
;

logical_and_expression:
	logical_not_expression ( options { greedy = true; }: (LOGICAL_AND_OPERATOR | AND_SYMBOL) logical_not_expression)*
;

logical_not_expression:
	NOT_SYMBOL logical_not_expression
	| boolean_primary_expression
;

boolean_primary_expression:
	predicate
		( options { greedy = true; }:
			comparison_operator
			(
				{LA(2) == OPEN_PAR_SYMBOL}? (ALL_SYMBOL | ANY_SYMBOL) subquery
				| predicate
			)
	    	| ( options { greedy = true; }: IS_SYMBOL NOT_SYMBOL? ( null_literal | FALSE_SYMBOL | TRUE_SYMBOL | UNKNOWN_SYMBOL))+
		)*
;

predicate:
	bitwise_or_expression
		( options { greedy = true; }:
			NOT_SYMBOL?
				(
					BETWEEN_SYMBOL bitwise_or_expression AND_SYMBOL predicate
					| LIKE_SYMBOL unary_expression ( options { greedy = true; }: ESCAPE_SYMBOL primary)?
					| REGEXP_SYMBOL bitwise_or_expression
					| IN_SYMBOL predicate_in
	    		)
	    	| SOUNDS_SYMBOL LIKE_SYMBOL bitwise_or_expression
		)?
;

// One of the 2 rules were 2 sub rules with unlimited nesting come together (and we need a predicate).
predicate_in:
	{LA(1) == OPEN_PAR_SYMBOL && LA(2) == SELECT_SYMBOL}? => subquery
	| expression_list_with_parentheses
;

bitwise_or_expression:
	bitwise_and_expression ( options { greedy = true; }: BITWISE_OR_OPERATOR bitwise_and_expression)*
;

bitwise_and_expression:
	shift_expression ( options { greedy = true; }: BITWISE_AND_OPERATOR shift_expression)*
;

shift_expression:
	additive_expression ( options { greedy = true; }: (SHIFT_LEFT_OPERATOR | SHIFT_RIGHT_OPERATOR) additive_expression)*
;

additive_expression:
	multiplicative_expression ( options{ greedy = true; }: (PLUS_OPERATOR | MINUS_OPERATOR) multiplicative_expression)*
;

multiplicative_expression:
	bitwise_xor_expression ( options { greedy = true; }: multiplication_operator bitwise_xor_expression)*
;

bitwise_xor_expression:
	concat_expression ( options { greedy = true; }: BITWISE_XOR_OPERATOR concat_expression)*
;

// The concat expression is only active if SQL_MODE_PIPES_AS_CONCAT is set (then || switches semantic from logical or to concat pipes).
concat_expression:
	unary_expression ( options { greedy = true; }: CONCAT_PIPES_SYMBOL unary_expression)*
;

unary_expression:
	(PLUS_OPERATOR | MINUS_OPERATOR | BITWISE_NOT_OPERATOR) unary_expression
	| not_expression
;

not_expression:
	not2_rule? interval_expression
;

interval_expression:
	INTERVAL_SYMBOL
		(
			// There are two forms using INTERVAL as keyword. Both can use INTERVAL OPEN_PAR expression... .
			// We disambiguate like the server parser does: if there is a second expression after the first separated
			// by comma then it's a function call, otherwise a time span.
			(OPEN_PAR_SYMBOL expression COMMA_SYMBOL) => interval_function
			| interval_time_span
		)
	| primary
;

interval_function:
	OPEN_PAR_SYMBOL expression (COMMA_SYMBOL expression)+ CLOSE_PAR_SYMBOL
;

interval_time_span:
	interval
;

primary:
    ( options { k = 4; }:
		literal
		| function_call
		| runtime_function_call // Complete functions defined in the grammar.
		| column_ref jsonOperator?
		| PARAM_MARKER
		| variable
		| EXISTS_SYMBOL subquery
		| expression_with_nested_parentheses
		| ROW_SYMBOL OPEN_PAR_SYMBOL expression (COMMA_SYMBOL expression)+ CLOSE_PAR_SYMBOL
		| OPEN_CURLY_SYMBOL identifier expression CLOSE_CURLY_SYMBOL
		| match_expression
		| case_expression
		| cast_expression
	)
	// Consume any collation expression locally to avoid ambiguities with the recursive cast_expression.
	( options { greedy = true; }: COLLATE_SYMBOL collation_name)*
;

jsonOperator:
  {SERVER_VERSION >= 50708}? JSON_SEPARATOR_SYMBOL text_string
  | {SERVER_VERSION >= 50713}? JSON_UNQUOTED_SEPARATOR_SYMBOL text_string
;

// This part is tricky, because all alternatives can have an unlimited nesting within parentheses.
// Best results by using a custom semantic predicate.
expression_with_nested_parentheses:
	{LA(1) == OPEN_PAR_SYMBOL && LA(2) == SELECT_SYMBOL}? subquery
	| expression_list_with_parentheses
;

comparison_operator:
	EQUAL_OPERATOR
	| GREATER_OR_EQUAL_OPERATOR
	| GREATER_THAN_OPERATOR
	| LESS_OR_EQUAL_OPERATOR
	| LESS_THAN_OPERATOR
	| NOT_EQUAL_OPERATOR
	| NOT_EQUAL2_OPERATOR
	| NULL_SAFE_EQUAL_OPERATOR
;

multiplication_operator:
	MULT_OPERATOR | DIV_OPERATOR | DIV_SYMBOL | MOD_OPERATOR | MOD_SYMBOL
;

runtime_function_call:
	runtime_function_call_expression
;

runtime_function_call_expression:
	// Function names that are keywords.
	CHAR_SYMBOL OPEN_PAR_SYMBOL expression_list (USING_SYMBOL charset_name)? CLOSE_PAR_SYMBOL
	| CURRENT_USER_SYMBOL parentheses?
	| DATE_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
	| DAY_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
	| HOUR_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
	| INSERT_SYMBOL OPEN_PAR_SYMBOL expression COMMA_SYMBOL expression COMMA_SYMBOL expression COMMA_SYMBOL expression CLOSE_PAR_SYMBOL
	// | INTERVAL_SYMBOL OPEN_PAR_SYMBOL expression (COMMA_SYMBOL expression)+ CLOSE_PAR_SYMBOL => handled in interval_expression.
	| LEFT_SYMBOL OPEN_PAR_SYMBOL expression COMMA_SYMBOL expression CLOSE_PAR_SYMBOL
	| MINUTE_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
	| MONTH_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
	| RIGHT_SYMBOL OPEN_PAR_SYMBOL expression COMMA_SYMBOL expression CLOSE_PAR_SYMBOL
	| SECOND_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
	| TIME_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
	| TIMESTAMP_SYMBOL OPEN_PAR_SYMBOL expression (COMMA_SYMBOL expression)? CLOSE_PAR_SYMBOL
	| trim_function
	| USER_SYMBOL parentheses
	| VALUES_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
	| YEAR_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL

	// Function names that are not keywords.
	| (ADDDATE_SYMBOL | SUBDATE_SYMBOL) OPEN_PAR_SYMBOL expression COMMA_SYMBOL expression CLOSE_PAR_SYMBOL
	| CURDATE_SYMBOL parentheses?
	| CURTIME_SYMBOL time_function_parameters?
	| (DATE_ADD_SYMBOL | DATE_SUB_SYMBOL) OPEN_PAR_SYMBOL expression COMMA_SYMBOL INTERVAL_SYMBOL interval CLOSE_PAR_SYMBOL
	| EXTRACT_SYMBOL OPEN_PAR_SYMBOL interval_unit FROM_SYMBOL expression CLOSE_PAR_SYMBOL
	| GET_FORMAT_SYMBOL OPEN_PAR_SYMBOL date_time_type  COMMA_SYMBOL expression CLOSE_PAR_SYMBOL
	| NOW_SYMBOL time_function_parameters?
	| POSITION_SYMBOL OPEN_PAR_SYMBOL bitwise_or_expression IN_SYMBOL expression CLOSE_PAR_SYMBOL
	| substring_function
	| SYSDATE_SYMBOL time_function_parameters?
	| (TIMESTAMP_ADD_SYMBOL | TIMESTAMP_DIFF_SYMBOL) OPEN_PAR_SYMBOL interval_timestamp_unit COMMA_SYMBOL expression COMMA_SYMBOL expression CLOSE_PAR_SYMBOL
	| UTC_DATE_SYMBOL parentheses?
	| UTC_TIME_SYMBOL time_function_parameters?
	| UTC_TIMESTAMP_SYMBOL time_function_parameters?

	// Function calls with other conflicts.
	| ASCII_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
	| CHARSET_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
	| COALESCE_SYMBOL expression_list_with_parentheses
	| COLLATION_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
	| DATABASE_SYMBOL parentheses
	| IF_SYMBOL OPEN_PAR_SYMBOL expression COMMA_SYMBOL expression COMMA_SYMBOL expression CLOSE_PAR_SYMBOL
	| FORMAT_SYMBOL OPEN_PAR_SYMBOL expression COMMA_SYMBOL expression (COMMA_SYMBOL expression)? CLOSE_PAR_SYMBOL
	| MICROSECOND_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
	| MOD_SYMBOL OPEN_PAR_SYMBOL expression COMMA_SYMBOL expression CLOSE_PAR_SYMBOL
	| {SERVER_VERSION < 50607}? OLD_PASSWORD_SYMBOL OPEN_PAR_SYMBOL string_literal CLOSE_PAR_SYMBOL
	| PASSWORD_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
	| QUARTER_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
	| REPEAT_SYMBOL OPEN_PAR_SYMBOL expression COMMA_SYMBOL expression CLOSE_PAR_SYMBOL
	| REPLACE_SYMBOL OPEN_PAR_SYMBOL expression COMMA_SYMBOL expression COMMA_SYMBOL expression CLOSE_PAR_SYMBOL
	| REVERSE_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
	| ROW_COUNT_SYMBOL parentheses
	| TRUNCATE_SYMBOL OPEN_PAR_SYMBOL expression COMMA_SYMBOL expression CLOSE_PAR_SYMBOL
	| WEEK_SYMBOL OPEN_PAR_SYMBOL expression (COMMA_SYMBOL expression)? CLOSE_PAR_SYMBOL
	| {SERVER_VERSION >= 50600}? WEIGHT_STRING_SYMBOL OPEN_PAR_SYMBOL expression
		(
			(AS_SYMBOL CHAR_SYMBOL field_length)? weight_string_levels?
			| AS_SYMBOL BINARY_SYMBOL field_length
			| COMMA_SYMBOL ulong_number COMMA_SYMBOL ulong_number COMMA_SYMBOL ulong_number
		)
		CLOSE_PAR_SYMBOL
	| geometry_function
	| aggregate_function
;

time_function_parameters:
	OPEN_PAR_SYMBOL fractional_precision? CLOSE_PAR_SYMBOL
;

fractional_precision:
	{SERVER_VERSION >= 50604}? => INT_NUMBER
;

weight_string_levels:
	LEVEL_SYMBOL
	(
		real_ulong_number MINUS_OPERATOR real_ulong_number
		| weight_string_level_list_item (COMMA_SYMBOL weight_string_level_list_item)*
	)
;

weight_string_level_list_item:
	real_ulong_number
	(
		(ASC_SYMBOL	| DESC_SYMBOL) REVERSE_SYMBOL?
		| REVERSE_SYMBOL
	)?
;

date_time_type:
	DATE_SYMBOL
	| TIME_SYMBOL
	| DATETIME_SYMBOL
	| TIMESTAMP_SYMBOL
;

trim_function:
	TRIM_SYMBOL OPEN_PAR_SYMBOL
	(
		expression (FROM_SYMBOL expression)?
		| LEADING_SYMBOL expression? FROM_SYMBOL expression
		| TRAILING_SYMBOL expression? FROM_SYMBOL expression
		| BOTH_SYMBOL expression? FROM_SYMBOL expression
	)
	CLOSE_PAR_SYMBOL
;

substring_function:
	SUBSTRING_SYMBOL OPEN_PAR_SYMBOL expression
	(
		COMMA_SYMBOL expression (COMMA_SYMBOL expression)?
		| FROM_SYMBOL expression (FOR_SYMBOL expression)?
	)
	CLOSE_PAR_SYMBOL
;
	
geometry_function:
	GEOMETRYCOLLECTION_SYMBOL optional_expression_list_with_parentheses
	| LINESTRING_SYMBOL expression_list_with_parentheses
	| MULTILINESTRING_SYMBOL expression_list_with_parentheses
	| MULTIPOINT_SYMBOL expression_list_with_parentheses
	| MULTIPOLYGON_SYMBOL expression_list_with_parentheses
	| POINT_SYMBOL OPEN_PAR_SYMBOL expression COMMA_SYMBOL expression CLOSE_PAR_SYMBOL
	| POLYGON_SYMBOL expression_list_with_parentheses
	| {SERVER_VERSION < 50706}? CONTAINS_SYMBOL OPEN_PAR_SYMBOL expression COMMA_SYMBOL expression CLOSE_PAR_SYMBOL
;

aggregate_function:
	AVG_SYMBOL OPEN_PAR_SYMBOL DISTINCT_SYMBOL? in_aggregate_expression CLOSE_PAR_SYMBOL
	| BIT_AND_SYMBOL OPEN_PAR_SYMBOL in_aggregate_expression CLOSE_PAR_SYMBOL
	| BIT_OR_SYMBOL OPEN_PAR_SYMBOL in_aggregate_expression CLOSE_PAR_SYMBOL
	| BIT_XOR_SYMBOL OPEN_PAR_SYMBOL in_aggregate_expression CLOSE_PAR_SYMBOL
	| count_function
	| MIN_SYMBOL OPEN_PAR_SYMBOL DISTINCT_SYMBOL? in_aggregate_expression CLOSE_PAR_SYMBOL
	| MAX_SYMBOL OPEN_PAR_SYMBOL DISTINCT_SYMBOL? in_aggregate_expression CLOSE_PAR_SYMBOL
	| STD_SYMBOL OPEN_PAR_SYMBOL in_aggregate_expression CLOSE_PAR_SYMBOL
	| VARIANCE_SYMBOL OPEN_PAR_SYMBOL in_aggregate_expression CLOSE_PAR_SYMBOL
	| STDDEV_SAMP_SYMBOL OPEN_PAR_SYMBOL in_aggregate_expression CLOSE_PAR_SYMBOL
	| VAR_SAMP_SYMBOL OPEN_PAR_SYMBOL in_aggregate_expression CLOSE_PAR_SYMBOL
	| SUM_SYMBOL OPEN_PAR_SYMBOL DISTINCT_SYMBOL? in_aggregate_expression CLOSE_PAR_SYMBOL
	| GROUP_CONCAT_SYMBOL OPEN_PAR_SYMBOL DISTINCT_SYMBOL? expression_list order_by_clause? (SEPARATOR_SYMBOL text_string)? CLOSE_PAR_SYMBOL
;

in_aggregate_expression:
	ALL_SYMBOL? expression
;

count_function:
	COUNT_SYMBOL OPEN_PAR_SYMBOL
	(
		ALL_SYMBOL? MULT_OPERATOR
		| in_aggregate_expression
		| DISTINCT_SYMBOL expression_list
	)
	CLOSE_PAR_SYMBOL
;

function_call:
	function_call_expression
;

function_call_expression:
	pure_identifier
	(
		OPEN_PAR_SYMBOL aliased_expression_list? CLOSE_PAR_SYMBOL // For both UDF + other functions.
		| dot_identifier OPEN_PAR_SYMBOL expression_list? CLOSE_PAR_SYMBOL // Other functions only.
	)
;

aliased_expression_list:
	aliased_expression (COMMA_SYMBOL aliased_expression)*
;

aliased_expression:
	expression select_alias?
;

variable options { greedy = true; }:
	user_variable (ASSIGN_OPERATOR expression)?
	| system_variable
;

user_variable:
	(AT_SIGN_SYMBOL text_or_identifier)
	| AT_TEXT_SUFFIX
;

// System variables as used in expressions. SET has another variant of this (SET GLOBAL/LOCAL varname).
system_variable:
	AT_AT_SIGN_SYMBOL ( options { greedy = true; }: option_type DOT_SYMBOL)? variable_name
;

variable_name:
	identifier dot_identifier?    // Check in semantic phase that the first id is not global/local/session/default.
	| DEFAULT_SYMBOL dot_identifier
;

match_expression:
	MATCH_SYMBOL OPEN_PAR_SYMBOL field_name_list CLOSE_PAR_SYMBOL AGAINST_SYMBOL OPEN_PAR_SYMBOL bitwise_or_expression
		(
			IN_SYMBOL BOOLEAN_SYMBOL MODE_SYMBOL
			| (IN_SYMBOL NATURAL_SYMBOL LANGUAGE_SYMBOL MODE_SYMBOL)? (WITH_SYMBOL QUERY_SYMBOL EXPANSION_SYMBOL)?
		)
		CLOSE_PAR_SYMBOL
;

// CASE expression used in (primary) expressions. There's another form used for stored programs (case_statement).
case_expression:
	CASE_SYMBOL expression? (when_expression then_expression)+ else_expression? END_SYMBOL
;

when_expression:
	WHEN_SYMBOL expression
;

then_expression:
	THEN_SYMBOL expression
;

else_expression:
	ELSE_SYMBOL expression
;

cast_expression:
	BINARY_SYMBOL primary
	| CAST_SYMBOL OPEN_PAR_SYMBOL expression AS_SYMBOL cast_type CLOSE_PAR_SYMBOL
	| CONVERT_SYMBOL OPEN_PAR_SYMBOL expression	(COMMA_SYMBOL cast_type	| USING_SYMBOL charset_name) CLOSE_PAR_SYMBOL
;

cast_type:
	BINARY_SYMBOL field_length?
	| CHAR_SYMBOL field_length? encoding?
	| NCHAR_SYMBOL field_length?
	| SIGNED_SYMBOL INT_SYMBOL?
	| UNSIGNED_SYMBOL INT_SYMBOL?
	| DATE_SYMBOL
	| TIME_SYMBOL type_datetime_precision?
	| DATETIME_SYMBOL type_datetime_precision?
	| DECIMAL_SYMBOL float_options?
	| {SERVER_VERSION >= 50708}? JSON_SYMBOL
;

encoding:
	ASCII_SYMBOL BINARY_SYMBOL?
	| BINARY_SYMBOL (ASCII_SYMBOL | UNICODE_SYMBOL | charset charset_name)?
	| UNICODE_SYMBOL BINARY_SYMBOL?
	| BYTE_SYMBOL
	| charset charset_name BINARY_SYMBOL?
;

charset:
	CHAR_SYMBOL SET_SYMBOL | CHARSET_SYMBOL
;

not_rule:
	NOT_SYMBOL
	| NOT2_SYMBOL // A NOT with a different (higher) operator precedence. 
;

not2_rule:
	LOGICAL_NOT_OPERATOR
	| NOT2_SYMBOL
;

or_rule:
	OR_SYMBOL
	| LOGICAL_OR_OPERATOR
;

xor_rule:
	XOR_SYMBOL
	| BITWISE_XOR_OPERATOR
;

and_rule:
	AND_SYMBOL
	| LOGICAL_AND_OPERATOR
;

interval:
	 expression interval_unit
;

// None of the microsecond variants can be used in schedules (e.g. events).
interval_unit:
	interval_timestamp_unit
	| SECOND_MICROSECOND_SYMBOL
	| MINUTE_MICROSECOND_SYMBOL
	| MINUTE_SECOND_SYMBOL
	| HOUR_MICROSECOND_SYMBOL
	| HOUR_SECOND_SYMBOL
	| HOUR_MINUTE_SYMBOL
	| DAY_MICROSECOND_SYMBOL
	| DAY_SECOND_SYMBOL
	| DAY_MINUTE_SYMBOL
	| DAY_HOUR_SYMBOL
	| YEAR_MONTH_SYMBOL
;

// Support for SQL_TSI_* units is added by mapping those to tokens without SQL_TSI_ prefix.
interval_timestamp_unit:
	MICROSECOND_SYMBOL
	| FRAC_SECOND_SYMBOL // Conditionally set in the lexer.
	| SECOND_SYMBOL
	| MINUTE_SYMBOL
	| HOUR_SYMBOL
	| DAY_SYMBOL   
	| WEEK_SYMBOL
	| MONTH_SYMBOL
	| QUARTER_SYMBOL
	| YEAR_SYMBOL
;

expression_list_with_parentheses:
	OPEN_PAR_SYMBOL expression_list CLOSE_PAR_SYMBOL
;

optional_expression_list_with_parentheses:
	OPEN_PAR_SYMBOL expression_list? CLOSE_PAR_SYMBOL
;

expression_list:
	expression (COMMA_SYMBOL expression)*
;

expression_list_with_direction:
	expression direction? ( options { greedy = true; }: COMMA_SYMBOL expression direction?)*
;

channel:
	{SERVER_VERSION >= 50706}? => FOR_SYMBOL CHANNEL_SYMBOL text_string_no_linebreak
;

//----------------- Stored program rules -----------------------------------------------------------

// Compound syntax for stored procedures, stored functions, triggers and events.
// Both sp_proc_stmt and ev_sql_stmt_inner in the server grammar.
compound_statement options { k = 3; }:
	statement
	| return_statement
	| if_statement
	| case_statement
	| labeled_block
	| unlabeled_block
	| labeled_control
	| unlabeled_control
	| leave_statement
	| iterate_statement
	
	| cursor_open
	| cursor_fetch
	| cursor_close
;

return_statement:
	RETURN_SYMBOL expression
;

if_statement:
	IF_SYMBOL if_body END_SYMBOL IF_SYMBOL
;
	
if_body:
	expression then_statement (ELSEIF_SYMBOL if_body | ELSE_SYMBOL compound_statement_list)?
;

then_statement:
	THEN_SYMBOL compound_statement_list
;

compound_statement_list:
	(compound_statement SEMICOLON_SYMBOL)+
;

// CASE rule solely for stored programs. There's another variant (case_expression) used in (primary) expressions.
case_statement:
	CASE_SYMBOL expression? (when_expression then_statement)+ else_statement? END_SYMBOL CASE_SYMBOL
;

else_statement:
	ELSE_SYMBOL compound_statement_list
;

labeled_block:
	label begin_end_block label_identifier?
;
	
unlabeled_block:
	begin_end_block
;
	
label:
	// Block labels can only be up to 16 characters long.
	label_identifier COLON_SYMBOL
;

label_identifier:
	pure_identifier | keyword_sp
;

begin_end_block: 
	BEGIN_SYMBOL sp_declarations? compound_statement_list? END_SYMBOL
;

labeled_control:
	label unlabeled_control label_identifier?
;

unlabeled_control:
	loop_block
	| while_do_block
	| repeat_until_block
;

loop_block:
	LOOP_SYMBOL compound_statement_list END_SYMBOL LOOP_SYMBOL
;

while_do_block:
	WHILE_SYMBOL expression DO_SYMBOL compound_statement_list END_SYMBOL WHILE_SYMBOL
;

repeat_until_block:
	REPEAT_SYMBOL compound_statement_list UNTIL_SYMBOL expression END_SYMBOL REPEAT_SYMBOL
;

sp_declarations:
	(sp_declaration SEMICOLON_SYMBOL)+
;

sp_declaration:
	DECLARE_SYMBOL
	(
		variable_declaration
		| condition_declaration
		| handler_declaration
		| cursor_declaration
	) 
;

variable_declaration:
	identifier_list data_type (COLLATE_SYMBOL collation_name_or_default)? (DEFAULT_SYMBOL expression)?
;

condition_declaration:
	identifier CONDITION_SYMBOL FOR_SYMBOL sp_condition
;

sp_condition:
	ulong_number
	| sqlstate
;

sqlstate:
	SQLSTATE_SYMBOL VALUE_SYMBOL? string_literal
;

handler_declaration:
	(CONTINUE_SYMBOL | EXIT_SYMBOL | UNDO_SYMBOL) HANDLER_SYMBOL
		FOR_SYMBOL handler_condition (COMMA_SYMBOL handler_condition)* compound_statement
;

handler_condition:
	sp_condition
	| identifier
	| SQLWARNING_SYMBOL
	| not_rule FOUND_SYMBOL
	| SQLEXCEPTION_SYMBOL
;

cursor_declaration:
	identifier CURSOR_SYMBOL FOR_SYMBOL select_statement
;

iterate_statement:
	ITERATE_SYMBOL label_identifier
;

leave_statement:
	LEAVE_SYMBOL label_identifier
;

get_diagnostics:
	GET_SYMBOL
	(
		CURRENT_SYMBOL
		| {SERVER_VERSION >= 50700}? STACKED_SYMBOL
	)? DIAGNOSTICS_SYMBOL
	(
		statement_information_item (COMMA_SYMBOL statement_information_item)*
		| CONDITION_SYMBOL signal_allowed_expression condition_information_item (COMMA_SYMBOL condition_information_item)*
	)
;

// Only a limited subset of expression is allowed in SIGNAL/RESIGNAL/CONDITIONS.
signal_allowed_expression:
	literal
	| variable
	| qualified_identifier
;

statement_information_item:
	(variable | identifier) EQUAL_OPERATOR (NUMBER_SYMBOL | ROW_COUNT_SYMBOL)
;

condition_information_item:
	(variable | identifier) EQUAL_OPERATOR (signal_information_item_name | RETURNED_SQLSTATE_SYMBOL)
;

signal_information_item_name:
	CLASS_ORIGIN_SYMBOL
	| SUBCLASS_ORIGIN_SYMBOL
	| CONSTRAINT_CATALOG_SYMBOL
	| CONSTRAINT_SCHEMA_SYMBOL
	| CONSTRAINT_NAME_SYMBOL
	| CATALOG_NAME_SYMBOL
	| SCHEMA_NAME_SYMBOL
	| TABLE_NAME_SYMBOL
	| COLUMN_NAME_SYMBOL
	| CURSOR_NAME_SYMBOL
	| MESSAGE_TEXT_SYMBOL
	| MYSQL_ERRNO_SYMBOL
;

signal_statement:
	SIGNAL_SYMBOL (identifier | sqlstate) (SET_SYMBOL signal_information_item (COMMA_SYMBOL signal_information_item)*)?
;

resignal_statement:
	RESIGNAL_SYMBOL (SQLSTATE_SYMBOL VALUE_SYMBOL? text_or_identifier)?
		(SET_SYMBOL signal_information_item (COMMA_SYMBOL signal_information_item)*)?
;

signal_information_item:
	signal_information_item_name EQUAL_OPERATOR signal_allowed_expression
;

cursor_open:
	OPEN_SYMBOL identifier
;

cursor_close:
	CLOSE_SYMBOL identifier
;

cursor_fetch:
	FETCH_SYMBOL identifier INTO_SYMBOL identifier_list
;

//----------------- Supplemental rules -------------------------------------------------------------

// Schedules in CREATE/ALTER EVENT.
schedule:
	AT_SYMBOL expression
	| EVERY_SYMBOL interval (STARTS_SYMBOL expression)? (ENDS_SYMBOL expression)?
;

database_option:
	DEFAULT_SYMBOL?
		(
			charset EQUAL_OPERATOR? charset_name_or_default
			| COLLATE_SYMBOL EQUAL_OPERATOR? collation_name_or_default
		)
;

column_definition:
	field_spec (reference_definition | (CHECK_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL)?)
;

field_spec:
	column_name field_def
;

field_def:
	data_type field_def_tail
;

field_def_tail options { k = 3; }:
	attribute*
	| {SERVER_VERSION >= 50707}? (COLLATE_SYMBOL collation_name)? (GENERATED_SYMBOL ALWAYS_SYMBOL)? AS_SYMBOL
		OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL (VIRTUAL_SYMBOL | STORED_SYMBOL)? gcol_attribute*
;

attribute:
	NOT_SYMBOL? null_literal
	| DEFAULT_SYMBOL (signed_literal | NOW_SYMBOL time_function_parameters?)
	| ON_SYMBOL UPDATE_SYMBOL NOW_SYMBOL time_function_parameters?
	| AUTO_INCREMENT_SYMBOL
	| SERIAL_SYMBOL DEFAULT_SYMBOL VALUE_SYMBOL
	| {LA(2) != KEY_SYMBOL}? UNIQUE_SYMBOL
	| (PRIMARY_SYMBOL | UNIQUE_SYMBOL) KEY_SYMBOL
	| KEY_SYMBOL
	| COMMENT_SYMBOL string_literal
	| COLLATE_SYMBOL collation_name
	| COLUMN_FORMAT_SYMBOL (FIXED_SYMBOL | DYNAMIC_SYMBOL | DEFAULT_SYMBOL)
	| STORAGE_SYMBOL (DISK_SYMBOL | MEMORY_SYMBOL | DEFAULT_SYMBOL)
;

// gcol_attribute is used in a loop, so we have an ambiquity between the 2 KEY_SYMBOLs.
gcol_attribute:
	UNIQUE_SYMBOL (options { greedy = true; }: KEY_SYMBOL)?	
	| COMMENT_SYMBOL text_string
	| not_rule? NULL_SYMBOL
	| PRIMARY_SYMBOL? KEY_SYMBOL
;

/* Internal to server.
parse_gcol_expression:
	PARSE_GCOL_EXPR_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
;
*/

reference_definition:
	REFERENCES_SYMBOL table_ref index_columns?
		// The MATCH part (from the SQL standard) is discouraged to be used in MySQL.
		// Using it will also cause the ON DELETE/UPDATE part to be ignored.
		(MATCH_SYMBOL (FULL_SYMBOL | PARTIAL_SYMBOL | SIMPLE_SYMBOL))?
		(
			ON_SYMBOL DELETE_SYMBOL reference_option (ON_SYMBOL UPDATE_SYMBOL reference_option)?
			| ON_SYMBOL UPDATE_SYMBOL reference_option (ON_SYMBOL DELETE_SYMBOL reference_option)?
		)?
;

index_columns:
	OPEN_PAR_SYMBOL index_column (COMMA_SYMBOL index_column)* CLOSE_PAR_SYMBOL
;

index_column:
	identifier field_length? direction?
;

index_type:
	(USING_SYMBOL | TYPE_SYMBOL) (BTREE_SYMBOL | RTREE_SYMBOL | HASH_SYMBOL)
;

normal_index_option:
	index_type
	| all_key_option
;

fulltext_index_option:
	WITH_SYMBOL PARSER_SYMBOL identifier
	| all_key_option
;

spatial_index_option:
	all_key_option
;

all_key_option:
	KEY_BLOCK_SIZE_SYMBOL EQUAL_OPERATOR? ulong_number
	| {SERVER_VERSION >= 50600}? COMMENT_SYMBOL string_literal
;

reference_option:
	RESTRICT_SYMBOL | CASCADE_SYMBOL | SET_SYMBOL null_literal | NO_SYMBOL ACTION_SYMBOL
;

data_type:
	data_type_elements
;

data_type_definition: // For external use only. Don't reference this in the normal grammar.
	data_type_elements EOF
;

data_type_elements:
	integer_type field_length? field_options?

	| real_literal precision? field_options?
	| FLOAT_SYMBOL float_options? field_options?
	
	| BIT_SYMBOL field_length?
	| BOOL_SYMBOL
	| BOOLEAN_SYMBOL
	
	| CHAR_SYMBOL field_length? string_binary?
	| nchar_literal field_length? BINARY_SYMBOL?
	| BINARY_SYMBOL field_length?
	| varchar_literal field_length string_binary?
	| nvarchar_literal field_length BINARY_SYMBOL?

	| VARBINARY_SYMBOL field_length

	| YEAR_SYMBOL field_length? field_options?
	| DATE_SYMBOL
	| TIME_SYMBOL type_datetime_precision?
	| TIMESTAMP_SYMBOL type_datetime_precision?
	| DATETIME_SYMBOL type_datetime_precision?

	| TINYBLOB_SYMBOL
	| BLOB_SYMBOL field_length?
	| MEDIUMBLOB_SYMBOL
	| LONGBLOB_SYMBOL
	| LONG_SYMBOL VARBINARY_SYMBOL
	| LONG_SYMBOL varchar_literal? string_binary?

	| TINYTEXT_SYMBOL string_binary?
	| TEXT_SYMBOL field_length? string_binary?
	| MEDIUMTEXT_SYMBOL string_binary?
	| LONGTEXT_SYMBOL string_binary?
	
	| DECIMAL_SYMBOL float_options? field_options?
	| NUMERIC_SYMBOL float_options? field_options?
	| FIXED_SYMBOL float_options? field_options?
	
	| ENUM_SYMBOL string_list string_binary?
	| SET_SYMBOL string_list string_binary?
	| SERIAL_SYMBOL
	| {SERVER_VERSION >= 50707}? JSON_SYMBOL
	| spatial_type
;

field_length:
	OPEN_PAR_SYMBOL (real_ulonglong_number | DECIMAL_NUMBER) CLOSE_PAR_SYMBOL
;

field_options:
	(SIGNED_SYMBOL | UNSIGNED_SYMBOL | ZEROFILL_SYMBOL)+
;

string_binary:
	ascii
	| unicode
	| BYTE_SYMBOL
	| charset charset_name BINARY_SYMBOL?
	| BINARY_SYMBOL (charset charset_name)?
;

ascii:
	ASCII_SYMBOL BINARY_SYMBOL?
	| {SERVER_VERSION >= 50500}? BINARY_SYMBOL ASCII_SYMBOL
;

unicode:
	UNICODE_SYMBOL BINARY_SYMBOL?
	| {SERVER_VERSION >= 50500}? BINARY_SYMBOL UNICODE_SYMBOL
;

type_datetime_precision:
	{SERVER_VERSION >= 50600}? OPEN_PAR_SYMBOL INT_NUMBER CLOSE_PAR_SYMBOL
;

charset_name:
	text_or_identifier
	| BINARY_SYMBOL
;

charset_name_or_default:
	charset_name
	| DEFAULT_SYMBOL
;

collation_name:
	text_or_identifier
;

collation_name_or_default:
	collation_name
	| DEFAULT_SYMBOL
;

spatial_type:
	GEOMETRY_SYMBOL
	| GEOMETRYCOLLECTION_SYMBOL
	| POINT_SYMBOL
	| MULTIPOINT_SYMBOL
	| LINESTRING_SYMBOL
	| MULTILINESTRING_SYMBOL
	| POLYGON_SYMBOL
	| MULTIPOLYGON_SYMBOL
;

alter_table_options:
	create_table_option+
;

create_table_options:
	create_table_option (COMMA_SYMBOL? create_table_option)*
;

create_table_option: // In the order as they appear in the server grammar.
	(ENGINE_SYMBOL | {SERVER_VERSION < 50500}? TYPE_SYMBOL) EQUAL_OPERATOR? engine_ref
	| MAX_ROWS_SYMBOL EQUAL_OPERATOR? ulonglong_number
	| MIN_ROWS_SYMBOL EQUAL_OPERATOR? ulonglong_number
	| AVG_ROW_LENGTH_SYMBOL EQUAL_OPERATOR? ulong_number
	| PASSWORD_SYMBOL EQUAL_OPERATOR? text_string
	| COMMENT_SYMBOL EQUAL_OPERATOR? text_string
	| COMPRESSION_SYMBOL EQUAL_OPERATOR? text_string
	| AUTO_INCREMENT_SYMBOL EQUAL_OPERATOR? ulonglong_number
	| PACK_KEYS_SYMBOL EQUAL_OPERATOR? (ulong_number | DEFAULT_SYMBOL)
	| {SERVER_VERSION >= 50600}? (STATS_AUTO_RECALC_SYMBOL | STATS_PERSISTENT_SYMBOL | STATS_SAMPLE_PAGES_SYMBOL)
		EQUAL_OPERATOR? (ulong_number | DEFAULT_SYMBOL)
	| (CHECKSUM_SYMBOL | TABLE_CHECKSUM_SYMBOL) EQUAL_OPERATOR? ulong_number
	| DELAY_KEY_WRITE_SYMBOL EQUAL_OPERATOR? ulong_number
	| ROW_FORMAT_SYMBOL EQUAL_OPERATOR? (DEFAULT_SYMBOL | DYNAMIC_SYMBOL | FIXED_SYMBOL | COMPRESSED_SYMBOL | REDUNDANT_SYMBOL | COMPACT_SYMBOL)
	| UNION_SYMBOL EQUAL_OPERATOR? OPEN_PAR_SYMBOL table_ref_list CLOSE_PAR_SYMBOL
	| DEFAULT_SYMBOL?
		(
			COLLATE_SYMBOL EQUAL_OPERATOR? collation_name_or_default
			| charset EQUAL_OPERATOR? charset_name_or_default
		)

	| INSERT_METHOD_SYMBOL EQUAL_OPERATOR? (NO_SYMBOL | FIRST_SYMBOL | LAST_SYMBOL)
	| DATA_SYMBOL DIRECTORY_SYMBOL EQUAL_OPERATOR? text_string
	| INDEX_SYMBOL DIRECTORY_SYMBOL EQUAL_OPERATOR? text_string
	| TABLESPACE_SYMBOL ({SERVER_VERSION >= 50707}? EQUAL_OPERATOR? | /* empty */ ) identifier
	| STORAGE_SYMBOL (DISK_SYMBOL | MEMORY_SYMBOL)
	| CONNECTION_SYMBOL EQUAL_OPERATOR? text_string
	| KEY_BLOCK_SIZE_SYMBOL EQUAL_OPERATOR? ulong_number
;

// Partition rules for CREATE/ALTER TABLE.
partitioning:
	{SERVER_VERSION >= 50100}? => PARTITION_SYMBOL BY_SYMBOL
	(
		LINEAR_SYMBOL? HASH_SYMBOL OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
		| LINEAR_SYMBOL? KEY_SYMBOL partition_key_algorithm? identifier_list_with_parentheses
		| (RANGE_SYMBOL | LIST_SYMBOL)
			( 
				OPEN_PAR_SYMBOL expression CLOSE_PAR_SYMBOL
				| {SERVER_VERSION >= 50500}? COLUMNS_SYMBOL identifier_list_with_parentheses
			)
	)
	(PARTITIONS_SYMBOL real_ulong_number)?
	(
		SUBPARTITION_SYMBOL BY_SYMBOL LINEAR_SYMBOL?
		(
			(
				 HASH_SYMBOL OPEN_PAR_SYMBOL bitwise_or_expression CLOSE_PAR_SYMBOL
				| KEY_SYMBOL partition_key_algorithm? identifier_list_with_parentheses
			)
			(SUBPARTITIONS_SYMBOL real_ulong_number)?
		)?
    )?
    partition_definitions?
;

partition_key_algorithm: // Actually only 1 and 2 are allowed. Needs a semantic check.
	{SERVER_VERSION >= 50700}? ALGORITHM_SYMBOL EQUAL_OPERATOR real_ulong_number
;

partition_definitions:
	OPEN_PAR_SYMBOL partition_definition (COMMA_SYMBOL partition_definition)* CLOSE_PAR_SYMBOL
;

partition_definition:
	PARTITION_SYMBOL identifier
	(
		VALUES_SYMBOL
		(
			LESS_SYMBOL THAN_SYMBOL (partition_value_list | MAXVALUE_SYMBOL)
			| IN_SYMBOL partition_value_list
		)
	)?
	(partition_option)*
	(OPEN_PAR_SYMBOL subpartition_definition (COMMA_SYMBOL subpartition_definition)* CLOSE_PAR_SYMBOL)?
;

partition_option:
	TABLESPACE_SYMBOL EQUAL_OPERATOR? identifier
	| STORAGE_SYMBOL? ENGINE_SYMBOL EQUAL_OPERATOR? engine_ref
	| NODEGROUP_SYMBOL EQUAL_OPERATOR? real_ulong_number
	| (MAX_ROWS_SYMBOL | MIN_ROWS_SYMBOL) EQUAL_OPERATOR? real_ulong_number
	| (DATA_SYMBOL | INDEX_SYMBOL) DIRECTORY_SYMBOL EQUAL_OPERATOR? string_literal
	| COMMENT_SYMBOL EQUAL_OPERATOR? string_literal
;

subpartition_definition:
	SUBPARTITION_SYMBOL identifier (partition_option)*
;

partition_value_list:
	OPEN_PAR_SYMBOL (expression | MAXVALUE_SYMBOL) (COMMA_SYMBOL (expression | MAXVALUE_SYMBOL))* CLOSE_PAR_SYMBOL
;

definer_clause:
	DEFINER_SYMBOL EQUAL_OPERATOR user
;

if_exists:
	IF_SYMBOL EXISTS_SYMBOL
;

if_not_exists:
	IF_SYMBOL not_rule EXISTS_SYMBOL
;

procedure_parameter:
	(IN_SYMBOL | OUT_SYMBOL | INOUT_SYMBOL)? function_parameter
;

function_parameter:
	identifier data_type
;

schema_identifier_pair:
	OPEN_PAR_SYMBOL schema_ref COMMA_SYMBOL schema_ref CLOSE_PAR_SYMBOL
;

view_identifier_list:
	view_ref (COMMA_SYMBOL view_ref)*
;       

field_name_list:
	column_ref (COMMA_SYMBOL column_ref)*
;

column_assignment_list_with_default:
	column_assignment_with_default (COMMA_SYMBOL column_assignment_with_default)*
;

column_assignment_with_default:
	column_ref EQUAL_OPERATOR (expression | DEFAULT_SYMBOL)
;

charset_clause:
	charset (text_or_identifier | DEFAULT_SYMBOL | BINARY_SYMBOL)
;

fields_clause:
	COLUMNS_SYMBOL field_term+
;

field_term:
	TERMINATED_SYMBOL BY_SYMBOL text_string
	| OPTIONALLY_SYMBOL? ENCLOSED_SYMBOL BY_SYMBOL text_string
	| ESCAPED_SYMBOL BY_SYMBOL text_string
;

lines_clause:
	LINES_SYMBOL line_term+
;

line_term:
	(TERMINATED_SYMBOL | STARTING_SYMBOL) BY_SYMBOL text_string
;

user_list:
	user (COMMA_SYMBOL user)*
;

grant_list:
	grant_user (COMMA_SYMBOL grant_user)*
;

grant_user:
	user
	(
		IDENTIFIED_SYMBOL
		(
			BY_SYMBOL PASSWORD_SYMBOL? text_string
			| {SERVER_VERSION >= 50600}? WITH_SYMBOL text_or_identifier (
				(AS_SYMBOL | {SERVER_VERSION >= 50706}? BY_SYMBOL) text_string)?
		)
	)?
;

user:
	text_or_identifier (AT_SIGN_SYMBOL text_or_identifier | AT_TEXT_SUFFIX)?
	| CURRENT_USER_SYMBOL parentheses?
;

like_clause:
	LIKE_SYMBOL text_string
;

like_or_where:
	like_clause | where_clause
;

online_option:
	{SERVER_VERSION < 50600}? (ONLINE_SYMBOL | OFFLINE_SYMBOL)
;

no_write_to_bin_log:
	{LA(1) == LOCAL_SYMBOL}? => LOCAL_SYMBOL // Predicate needed to direct the parser (as LOCAL can also be an identifier).
	| NO_WRITE_TO_BINLOG_SYMBOL
;

use_partition:
	{SERVER_VERSION >= 50602}? => PARTITION_SYMBOL identifier_list_with_parentheses
;

//----------------- Object names and references ----------------------------------------------------

// For each object we have at least 2 rules here:
// 1) The name when creating that object.
// 2) The name when used to reference it from other rules.
//
// Sometimes we need additional reference rules with different form, depending on the place such a reference is used.

column_name:
	identifier
;

column_ref:
	column_ref_variants
;

// field_ident rule in the server parser.
column_ref_variants:
	dot_identifier
	| qualified_identifier (options { greedy = true; }: dot_identifier)?
;

column_internal_ref:
	column_ref_variants
;

column_ref_with_wildcard:
	column_ref_with_wildcard2
;

column_ref_with_wildcard2:
	identifier
	(
		{LA(2) ==  MULT_OPERATOR}? DOT_SYMBOL MULT_OPERATOR
		| dot_identifier
		(
			{LA(2) ==  MULT_OPERATOR}? DOT_SYMBOL MULT_OPERATOR
			| dot_identifier
		)?
	)?
;

index_name:
	identifier
;

index_ref:
	identifier
;

table_wild:
	identifier
	(
		{LA(2) ==  MULT_OPERATOR}? DOT_SYMBOL MULT_OPERATOR
		| dot_identifier DOT_SYMBOL MULT_OPERATOR
	)
;       

schema_name:
	identifier
;

schema_ref:
	identifier
;

procedure_name:
	qualified_identifier
;

procedure_ref:
	qualified_identifier
;

function_name:
	qualified_identifier
;

function_ref:
	qualified_identifier
;

trigger_name:
	qualified_identifier
;

trigger_ref:
	qualified_identifier
;

view_name:
	table_name_variants
;

view_ref:
	qualified_identifier
;

tablespace_name:
	identifier
;

tablespace_ref:
	identifier
;

logfile_group_name:
	identifier
;

logfile_group_ref:
	identifier
;

event_name:
	qualified_identifier
;

event_ref:
	qualified_identifier
;

udf_name: // UDFs are referenced at the same places as any other function. So, no dedicated *_ref here.
	identifier
;

server_name:
	text_or_identifier
;

server_ref:
	text_or_identifier
;

engine_ref:
	text_or_identifier
;

table_name:
	table_name_variants
;

filter_table_ref: // Always qualified.
	identifier dot_identifier
;

table_ref_with_wildcard:
	identifier
	(
		{LA(2) == MULT_OPERATOR}? DOT_SYMBOL MULT_OPERATOR
		| dot_identifier (DOT_SYMBOL MULT_OPERATOR)?
	)?
;

table_ref:
	table_name_variants
;

table_ref_no_db:
	identifier
;

table_name_variants:
	qualified_identifier
	| dot_identifier
;

table_ref_list:
	table_ref (COMMA_SYMBOL table_ref)*
;       

table_ref_list_with_wildcard:
	table_ref_with_wildcard (COMMA_SYMBOL table_ref_with_wildcard)*
;

//----------------- Common basic rules -------------------------------------------------------------

// Identifiers excluding keywords (except if they are quoted).
pure_identifier:
	IDENTIFIER
	| BACK_TICK_QUOTED_ID
	| {SQL_MODE_ACTIVE(SQL_MODE_ANSI_QUOTES)}? DOUBLE_QUOTED_TEXT
;

// Identifiers including a certain set of keywords, which are allowed also if not quoted.
identifier:
	pure_identifier
	| keyword
;

identifier_list:
	identifier ( options { greedy = true; }: COMMA_SYMBOL identifier)*
;

identifier_list_with_parentheses:
	OPEN_PAR_SYMBOL identifier_list CLOSE_PAR_SYMBOL
;

qualified_identifier:
	identifier (options { greedy = true; }: dot_identifier)?
;

// This rule mimics the behavior of the server parser to allow any identifier, including any keyword,
// unquoted when directly preceded by a dot in a qualified identifier.
dot_identifier:
	DOT_SYMBOL identifier // Dot and kw separated. Not all keywords are allowed then.
	//| DOT_IDENTIFIER    // Cannot appear due to our manual splitting.
;

ulong_number:
	INT_NUMBER
	| HEX_NUMBER
	| LONG_NUMBER
	| ULONGLONG_NUMBER
	| DECIMAL_NUMBER
	| FLOAT_NUMBER
;

real_ulong_number:
	INT_NUMBER
	| HEX_NUMBER
	| LONG_NUMBER
	| ULONGLONG_NUMBER
;

ulonglong_number:
	INT_NUMBER
	| ULONGLONG_NUMBER
	| LONG_NUMBER
	| DECIMAL_NUMBER
	| FLOAT_NUMBER
;

real_ulonglong_number:
	INT_NUMBER
	| ULONGLONG_NUMBER
	| LONG_NUMBER
;

string_list:
	OPEN_PAR_SYMBOL text_string (COMMA_SYMBOL text_string)* CLOSE_PAR_SYMBOL
;

text_string:
	SINGLE_QUOTED_TEXT
	| HEX_NUMBER
	| BIN_NUMBER
;

// A special variant of a text string that must not contain a linebreak (TEXT_STRING_sys_nonewline in sql_yacc.yy).
// Check validity in semantic phase.
text_string_no_linebreak:
	text_string
;

literal:
	string_literal
	| num_literal
   	// Date, time and timestamp can be both a temporal literal or a field name, so we need a predicate.
	| {LA(1) == DATE_SYMBOL || LA(1) == TIME_SYMBOL || LA(1) == TIMESTAMP_SYMBOL}? temporal_literal
	| null_literal
	| bool_literal
	| UNDERSCORE_CHARSET? HEX_NUMBER
	| {SERVER_VERSION >= 50003}? UNDERSCORE_CHARSET? BIN_NUMBER
	;

signed_literal:
	literal
	| PLUS_OPERATOR ulong_number
	| MINUS_OPERATOR ulong_number
;

// To ease post processing strings (for automatic concatenation) we use an own subtree for each string.
// Because of that already mentioned bug we need a separate rule when doing tree rewrite for a single alternative.
string_literal:
	string
;

string:
	NCHAR_TEXT
	| UNDERSCORE_CHARSET? ( options { greedy = true; }:	SINGLE_QUOTED_TEXT | {!SQL_MODE_ACTIVE(SQL_MODE_ANSI_QUOTES)}? DOUBLE_QUOTED_TEXT)+
;

num_literal:
	INT_NUMBER
	| LONG_NUMBER
	| ULONGLONG_NUMBER
	| DECIMAL_NUMBER
	| FLOAT_NUMBER
;

bool_literal:
	TRUE_SYMBOL
	| FALSE_SYMBOL
;

null_literal: // In sql_yacc.cc both 'NULL' and '\N' are mapped to NULL_SYM (which is our null_literal).
	NULL_SYMBOL
	| NULL2_SYMBOL
;

temporal_literal:
	DATE_SYMBOL SINGLE_QUOTED_TEXT
	| TIME_SYMBOL SINGLE_QUOTED_TEXT
	| TIMESTAMP_SYMBOL SINGLE_QUOTED_TEXT
;

// Support for INT1, INT2 etc. is added by mapping them to their explicitly named integer counter part.
// See lexer rules for INT1 etc.
integer_type:
	INT_SYMBOL
	| TINYINT_SYMBOL
	| SMALLINT_SYMBOL
	| MEDIUMINT_SYMBOL
	| BIGINT_SYMBOL
;

real_literal:
	REAL_SYMBOL | DOUBLE_SYMBOL PRECISION_SYMBOL?
;

float_options:
	// Another way to write (field_length | precision), but without ambiguity.
	OPEN_PAR_SYMBOL INT_NUMBER (COMMA_SYMBOL INT_NUMBER)? CLOSE_PAR_SYMBOL
;

precision:
	OPEN_PAR_SYMBOL INT_NUMBER COMMA_SYMBOL INT_NUMBER CLOSE_PAR_SYMBOL
;

nchar_literal:
	NCHAR_SYMBOL
	| {LA(3) != VARYING_SYMBOL}? NATIONAL_SYMBOL CHAR_SYMBOL // Predicate to solve ambiguity with nvarchar_literal.
;

varchar_literal:
	CHAR_SYMBOL VARYING_SYMBOL
	| VARCHAR_SYMBOL
;

nvarchar_literal:
	NATIONAL_SYMBOL VARCHAR_SYMBOL
	| NVARCHAR_SYMBOL
	| NCHAR_SYMBOL VARCHAR_SYMBOL
	| NATIONAL_SYMBOL CHAR_SYMBOL VARYING_SYMBOL
	| NCHAR_SYMBOL VARYING_SYMBOL
;

text_or_identifier:
	SINGLE_QUOTED_TEXT
	| identifier
	//| AT_TEXT_SUFFIX // LEX_HOSTNAME in the server grammar. Handled differently.
;

size_number:
	real_ulonglong_number
	| pure_identifier // Something like 10G. Semantic check needed for validity.
;

parentheses:
	OPEN_PAR_SYMBOL CLOSE_PAR_SYMBOL
;

equal:
	EQUAL_OPERATOR | ASSIGN_OPERATOR
;

option_type:
	GLOBAL_SYMBOL | LOCAL_SYMBOL | SESSION_SYMBOL
;

// Keyword that we allow for identifiers.
// Keywords defined only for specific server versions are handled at lexer level and so cannot match this rule
// if the current server version doesn't allow them. Hence we don't need predicates here for them.
keyword:
	keyword_sp
	| ( // Leave this list in parentheses, as this makes a difference of several MB in the resulting parser.
	    // (at least in the C target).
		ACCOUNT_SYMBOL // Conditionally set in the lexer.
		| ASCII_SYMBOL
		| ALWAYS_SYMBOL // Conditionally set in the lexer.
		| BACKUP_SYMBOL
		| BEGIN_SYMBOL
		| BYTE_SYMBOL
		| CACHE_SYMBOL
		| CHARSET_SYMBOL
		| CHECKSUM_SYMBOL
		| CLOSE_SYMBOL
		| COMMENT_SYMBOL
		| COMMIT_SYMBOL
		| CONTAINS_SYMBOL
		| DEALLOCATE_SYMBOL
		| DO_SYMBOL
		| END_SYMBOL
		| EXECUTE_SYMBOL
		| FLUSH_SYMBOL
		| FOLLOWS_SYMBOL
		| FORMAT_SYMBOL
		| GROUP_REPLICATION_SYMBOL // Conditionally set in the lexer.
		| HANDLER_SYMBOL
		| HELP_SYMBOL
		| HOST_SYMBOL
		| INSTALL_SYMBOL
		| LANGUAGE_SYMBOL
		| NO_SYMBOL
		| OPEN_SYMBOL
		| OPTIONS_SYMBOL
		| OWNER_SYMBOL
		| PARSER_SYMBOL
		| PARTITION_SYMBOL
		| PORT_SYMBOL
		| PRECEDES_SYMBOL
		| PREPARE_SYMBOL
		| REMOVE_SYMBOL
		| REPAIR_SYMBOL
		| RESET_SYMBOL
		| RESTORE_SYMBOL
		| ROLLBACK_SYMBOL
		| SAVEPOINT_SYMBOL
		| SECURITY_SYMBOL
		| SERVER_SYMBOL
		| /*{SERVER_VERSION >= 50709}? */ SHUTDOWN_SYMBOL // Moved here from keyword_sp.
			// Cannot make this alt using a sempred as ANTLR crashs on that (stack overflow).
		| SIGNED_SYMBOL
		| SLAVE_SYMBOL
		| SOCKET_SYMBOL
		| SONAME_SYMBOL
		| START_SYMBOL
		| STOP_SYMBOL
		| TRUNCATE_SYMBOL
		| UNICODE_SYMBOL
		| UNINSTALL_SYMBOL
		| UPGRADE_SYMBOL
		| WRAPPER_SYMBOL
		| XA_SYMBOL
	)
;

// Comment from server yacc grammar:
//   Keywords that we allow for labels in SPs. Anything that's the beginning of a statement
//   or characteristics must be in keyword above, otherwise we get (harmful) shift/reduce conflicts.
// Additionally:
//   The keywords are only roughly sorted to stay with the same order as in sql_yacc.yy (for simpler diff'ing).
keyword_sp:
	ACTION_SYMBOL
	| ADDDATE_SYMBOL
	| AFTER_SYMBOL
	| AGAINST_SYMBOL
	| AGGREGATE_SYMBOL
	| ALGORITHM_SYMBOL
	| ANALYZE_SYMBOL
	| ANY_SYMBOL
	| AT_SYMBOL
	| AUTHORS_SYMBOL
	| AUTO_INCREMENT_SYMBOL
	| AUTOEXTEND_SIZE_SYMBOL
	| AVG_ROW_LENGTH_SYMBOL
	| AVG_SYMBOL
	| BINLOG_SYMBOL
	| BIT_SYMBOL
	| BLOCK_SYMBOL
	| BOOL_SYMBOL
	| BOOLEAN_SYMBOL
	| BTREE_SYMBOL
	| CASCADED_SYMBOL
	| CATALOG_NAME_SYMBOL
	| CHAIN_SYMBOL
	| CHANGED_SYMBOL
	| CHANNEL_SYMBOL // Conditionally set in the lexer.
	| CIPHER_SYMBOL
	| CLIENT_SYMBOL
	| CLASS_ORIGIN_SYMBOL
	| COALESCE_SYMBOL
	| CODE_SYMBOL
	| COLLATION_SYMBOL
	| COLUMN_NAME_SYMBOL
	| COLUMN_FORMAT_SYMBOL
	| COLUMNS_SYMBOL
	| COMMITTED_SYMBOL
	| COMPACT_SYMBOL
	| COMPLETION_SYMBOL
	| COMPRESSED_SYMBOL
	| COMPRESSION_SYMBOL // Conditionally set in the lexer.
	| CONCURRENT_SYMBOL
	| CONNECTION_SYMBOL
	| CONSISTENT_SYMBOL
	| CONSTRAINT_CATALOG_SYMBOL
	| CONSTRAINT_SCHEMA_SYMBOL
	| CONSTRAINT_NAME_SYMBOL
	| CONTEXT_SYMBOL
	| CONTRIBUTORS_SYMBOL
	| CPU_SYMBOL
	| CUBE_SYMBOL
	| CURRENT_SYMBOL
	| CURSOR_NAME_SYMBOL
	| DATA_SYMBOL
	| DATAFILE_SYMBOL
	| DATETIME_SYMBOL
	| DATE_SYMBOL
	| DAY_SYMBOL
	| DEFAULT_AUTH_SYMBOL
	| DEFINER_SYMBOL
	| DELAY_KEY_WRITE_SYMBOL
	| DES_KEY_FILE_SYMBOL
	| DIAGNOSTICS_SYMBOL
	| DIRECTORY_SYMBOL
	| DISABLE_SYMBOL
	| DISCARD_SYMBOL
	| DISK_SYMBOL
	| DUMPFILE_SYMBOL
	| DUPLICATE_SYMBOL
	| DYNAMIC_SYMBOL
	| ENDS_SYMBOL
	| ENUM_SYMBOL
	| ENGINE_SYMBOL
	| ENGINES_SYMBOL
	| ERROR_SYMBOL
	| ERRORS_SYMBOL
	| ESCAPE_SYMBOL
	| EVENT_SYMBOL
	| EVENTS_SYMBOL
	| EVERY_SYMBOL
	| EXPANSION_SYMBOL
	| EXPORT_SYMBOL
	| EXTENDED_SYMBOL
	| EXTENT_SIZE_SYMBOL
	| FAULTS_SYMBOL
	| FAST_SYMBOL
	| FOUND_SYMBOL
	| ENABLE_SYMBOL
	| FULL_SYMBOL
	| FILE_SYMBOL
	| FILE_BLOCK_SIZE_SYMBOL // Conditionally set in the lexer.
	| FILTER_SYMBOL
	| FIRST_SYMBOL
	| FIXED_SYMBOL
	| GENERAL_SYMBOL
	| GEOMETRY_SYMBOL
	| GEOMETRYCOLLECTION_SYMBOL
	| GET_FORMAT_SYMBOL
	| GRANTS_SYMBOL
	| GLOBAL_SYMBOL
	| HASH_SYMBOL
	| HOSTS_SYMBOL
	| HOUR_SYMBOL
	| IDENTIFIED_SYMBOL
	| IGNORE_SERVER_IDS_SYMBOL
	| INVOKER_SYMBOL
	| IMPORT_SYMBOL
	| INDEXES_SYMBOL
	| INITIAL_SIZE_SYMBOL
	| INNODB_SYMBOL // Conditionally deprecated in the lexer.
	| IO_SYMBOL
	| IPC_SYMBOL
	| ISOLATION_SYMBOL
	| ISSUER_SYMBOL
	| INSERT_METHOD_SYMBOL
	| JSON_SYMBOL // Conditionally set in the lexer.
	| KEY_BLOCK_SIZE_SYMBOL
	| LAST_SYMBOL
	| LEAVES_SYMBOL
	| LESS_SYMBOL
	| LEVEL_SYMBOL
	| LINESTRING_SYMBOL
	| LIST_SYMBOL
	| LOCAL_SYMBOL
	| LOCKS_SYMBOL
	| LOGFILE_SYMBOL
	| LOGS_SYMBOL
	| MAX_ROWS_SYMBOL
	| MASTER_SYMBOL
	| MASTER_HEARTBEAT_PERIOD_SYMBOL
	| MASTER_HOST_SYMBOL
	| MASTER_PORT_SYMBOL
	| MASTER_LOG_FILE_SYMBOL
	| MASTER_LOG_POS_SYMBOL
	| MASTER_USER_SYMBOL
	| MASTER_PASSWORD_SYMBOL
	| MASTER_SERVER_ID_SYMBOL
	| MASTER_CONNECT_RETRY_SYMBOL
	| MASTER_RETRY_COUNT_SYMBOL
	| MASTER_DELAY_SYMBOL
	| MASTER_SSL_SYMBOL
	| MASTER_SSL_CA_SYMBOL
	| MASTER_SSL_CAPATH_SYMBOL
	| MASTER_SSL_CERT_SYMBOL
	| MASTER_SSL_CIPHER_SYMBOL
	| MASTER_SSL_CRL_SYMBOL
	| MASTER_SSL_CRLPATH_SYMBOL
	| MASTER_SSL_KEY_SYMBOL
	| MASTER_AUTO_POSITION_SYMBOL
	| MAX_CONNECTIONS_PER_HOUR_SYMBOL
	| MAX_QUERIES_PER_HOUR_SYMBOL
	| MAX_STATEMENT_TIME_SYMBOL // Conditionally deprecated in the lexer.
	| MAX_SIZE_SYMBOL
	| MAX_UPDATES_PER_HOUR_SYMBOL
	| MAX_USER_CONNECTIONS_SYMBOL
	| MEDIUM_SYMBOL
	| MEMORY_SYMBOL
	| MERGE_SYMBOL
	| MESSAGE_TEXT_SYMBOL
	| MICROSECOND_SYMBOL
	| MIGRATE_SYMBOL
	| MINUTE_SYMBOL
	| MIN_ROWS_SYMBOL
	| MODIFY_SYMBOL
	| MODE_SYMBOL
	| MONTH_SYMBOL
	| MULTILINESTRING_SYMBOL
	| MULTIPOINT_SYMBOL
	| MULTIPOLYGON_SYMBOL
	| MUTEX_SYMBOL
	| MYSQL_ERRNO_SYMBOL
	| NAME_SYMBOL
	| NAMES_SYMBOL
	| NATIONAL_SYMBOL
	| NCHAR_SYMBOL
	| NDBCLUSTER_SYMBOL
	| NEVER_SYMBOL
	| NEXT_SYMBOL
	| NEW_SYMBOL
	| NO_WAIT_SYMBOL
	| NODEGROUP_SYMBOL
	| NONE_SYMBOL
	| NUMBER_SYMBOL
	| NVARCHAR_SYMBOL
	| OFFSET_SYMBOL
	| OLD_PASSWORD_SYMBOL
	| ONE_SHOT_SYMBOL
	| ONE_SYMBOL
	| PACK_KEYS_SYMBOL
	| PAGE_SYMBOL
	| PARTIAL_SYMBOL
	| PARTITIONING_SYMBOL
	| PARTITIONS_SYMBOL
	| PASSWORD_SYMBOL
	| PHASE_SYMBOL
	| PLUGIN_DIR_SYMBOL
	| PLUGIN_SYMBOL
	| PLUGINS_SYMBOL
	| POINT_SYMBOL
	| POLYGON_SYMBOL
	| PRESERVE_SYMBOL
	| PREV_SYMBOL
	| PRIVILEGES_SYMBOL
	| PROCESS_SYMBOL
	| PROCESSLIST_SYMBOL
	| PROFILE_SYMBOL
	| PROFILES_SYMBOL
	| PROXY_SYMBOL
	| QUARTER_SYMBOL
	| QUERY_SYMBOL
	| QUICK_SYMBOL
	| READ_ONLY_SYMBOL
	| REBUILD_SYMBOL
	| RECOVER_SYMBOL
	| REDO_BUFFER_SIZE_SYMBOL
	| REDOFILE_SYMBOL
	| REDUNDANT_SYMBOL
	| RELAY_SYMBOL
	| RELAYLOG_SYMBOL
	| RELAY_LOG_FILE_SYMBOL
	| RELAY_LOG_POS_SYMBOL
	| RELAY_THREAD_SYMBOL
	| RELOAD_SYMBOL
	| REORGANIZE_SYMBOL
	| REPEATABLE_SYMBOL
	| REPLICATION_SYMBOL
	| REPLICATE_DO_DB_SYMBOL
	| REPLICATE_IGNORE_DB_SYMBOL
	| REPLICATE_DO_TABLE_SYMBOL
	| REPLICATE_IGNORE_TABLE_SYMBOL
	| REPLICATE_WILD_DO_TABLE_SYMBOL
	| REPLICATE_WILD_IGNORE_TABLE_SYMBOL
	| REPLICATE_REWRITE_DB_SYMBOL
	| RESUME_SYMBOL
	| RETURNED_SQLSTATE_SYMBOL
	| RETURNS_SYMBOL
	| REVERSE_SYMBOL
	| ROLLUP_SYMBOL
	| ROUTINE_SYMBOL
	| ROWS_SYMBOL
	| ROW_COUNT_SYMBOL
	| ROW_FORMAT_SYMBOL
	| ROW_SYMBOL
	| RTREE_SYMBOL
	| SCHEDULE_SYMBOL
	| SCHEMA_NAME_SYMBOL
	| SECOND_SYMBOL
	| SERIAL_SYMBOL
	| SERIALIZABLE_SYMBOL
	| SESSION_SYMBOL
	| SIMPLE_SYMBOL
	| SHARE_SYMBOL
	//| {SERVER_VERSION < 50709}? SHUTDOWN_SYMBOL // Moved to keyword rule. We cannot keep this here however, as ANTLR crashs on that.
	| SLOW_SYMBOL
	| SNAPSHOT_SYMBOL
	| SOUNDS_SYMBOL
	| SOURCE_SYMBOL
	| SQL_AFTER_GTIDS_SYMBOL
	| SQL_AFTER_MTS_GAPS_SYMBOL
	| SQL_BEFORE_GTIDS_SYMBOL
	| SQL_CACHE_SYMBOL
	| SQL_BUFFER_RESULT_SYMBOL
	| SQL_NO_CACHE_SYMBOL
	| SQL_THREAD_SYMBOL
	| STACKED_SYMBOL
	| STARTS_SYMBOL
	| STATS_AUTO_RECALC_SYMBOL
	| STATS_PERSISTENT_SYMBOL
	| STATS_SAMPLE_PAGES_SYMBOL
	| STATUS_SYMBOL
	| STORAGE_SYMBOL
	| STRING_SYMBOL
	| SUBCLASS_ORIGIN_SYMBOL
	| SUBDATE_SYMBOL
	| SUBJECT_SYMBOL
	| SUBPARTITION_SYMBOL
	| SUBPARTITIONS_SYMBOL
	| SUPER_SYMBOL
	| SUSPEND_SYMBOL
	| SWAPS_SYMBOL
	| SWITCHES_SYMBOL
	| TABLE_NAME_SYMBOL
	| TABLES_SYMBOL
	| TABLE_CHECKSUM_SYMBOL
	| TABLESPACE_SYMBOL
	| TEMPORARY_SYMBOL
	| TEMPTABLE_SYMBOL
	| TEXT_SYMBOL
	| THAN_SYMBOL
	| TRANSACTION_SYMBOL
	| TRIGGERS_SYMBOL
	| TIMESTAMP_SYMBOL
	| TIMESTAMP_ADD_SYMBOL
	| TIMESTAMP_DIFF_SYMBOL
	| TIME_SYMBOL
	| TYPES_SYMBOL
	| TYPE_SYMBOL
	| UDF_RETURNS_SYMBOL
	| FUNCTION_SYMBOL
	| UNCOMMITTED_SYMBOL
	| UNDEFINED_SYMBOL
	| UNDO_BUFFER_SIZE_SYMBOL
	| UNDOFILE_SYMBOL
	| UNKNOWN_SYMBOL
	| UNTIL_SYMBOL
	| USER_RESOURCES_SYMBOL
	| USER_SYMBOL
	| USE_FRM_SYMBOL
	| VARIABLES_SYMBOL
	| VIEW_SYMBOL
	| VALUE_SYMBOL
	| WARNINGS_SYMBOL
	| WAIT_SYMBOL
	| WEEK_SYMBOL
	| WORK_SYMBOL
	| WEIGHT_STRING_SYMBOL
	| X509_SYMBOL
	| XID_SYMBOL
	| XML_SYMBOL
	| YEAR_SYMBOL
;
