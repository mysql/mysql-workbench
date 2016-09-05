parser grammar MySQLParser;

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
 * Merged in all changes up to mysql-5.7 git revision [f84d8da] (26 April 2016).
 *
 * MySQL grammar for ANTLR 4.5 with language features from MySQL 4.0 up to MySQL 5.7.13 (except for
 * internal function names which were reduced significantly in 5.1, we only use the reduced set).
 * The server version in the generated parser can be switched at runtime, making it so possible
 * to switch the supported feature set dynamically.
 *
 * This grammar is a port of the ANTLR v3 variant for v4.
 * The coverage of the MySQL language should be 100%, but there might still be bugs or omissions.
 *
 * To use this grammar you will need a few support classes (which should be included in the package).
 * See the demo project for further details.
 *
 * Written by Mike Lischke. Direct all bug reports, omissions etc. to mike.lischke@oracle.com.
 */

//-------------------------------------------------------------------------------------------------

options {
    superClass = MySQLBaseRecognizer;
    tokenVocab = MySQLLexer;
}

//-------------------------------------------------------------------------------------------------

@header {/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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
}

@postinclude {
#include "MySQLBaseRecognizer.h"
}

//-------------------------------------------------------------------------------------------------

query:
    ((statement | begin_work) SEMICOLON_SYMBOL?)? EOF
;

statement:
    // DDL
    alterStatement
    | createStatement
    | dropStatement
    | renameTableStatement
    | truncateTableStatement

    // DML
    | callStatement
    | deleteStatement
    | doStatement
    | handlerStatement
    | insertStatement
    | loadStatement
    | replaceStatement
    | selectStatement
    | updateStatement

    | transactionOrLockingStatement

    | replicationStatement

    | preparedStatement

    // Database administration
    | accountManagementStatement
    | tableAdministrationStatement
    | installUninstallStatment
    | setStatement // SET PASSWORD is handled in accountManagementStatement.
    | showStatement
    | otherAdministrativeStatement

    // MySQL utilitity statements
    | utilityStatement

    | {serverVersion >= 50604}? getDiagnostics
    | {serverVersion >= 50500}? signalStatement
    | {serverVersion >= 50500}? resignalStatement
;

//----------------- DDL statements -----------------------------------------------------------------

alterStatement:
  ALTER_SYMBOL
  (
    alterTable
    | alterDatabase
    | PROCEDURE_SYMBOL procedureRef routineAlterOptions?
    | FUNCTION_SYMBOL functionRef routineAlterOptions?
    | alterView
    | {serverVersion >= 50100}? alterEvent
    | alterTablespace
    | alterLogfileGroup
    | alterServer
    // ALTER USER is part of the user management rule.
    | {serverVersion >= 50713}? INSTANCE_SYMBOL ROTATE_SYMBOL textOrIdentifier MASTER_SYMBOL KEY_SYMBOL
  )
;

alterDatabase:
    DATABASE_SYMBOL schemaRef
    (
         databaseOption+
        | UPGRADE_SYMBOL DATA_SYMBOL DIRECTORY_SYMBOL NAME_SYMBOL
    )
;

alterEvent:
    definerClause?
        EVENT_SYMBOL eventRef
        (ON_SYMBOL SCHEDULE_SYMBOL schedule)?
        (ON_SYMBOL COMPLETION_SYMBOL NOT_SYMBOL? PRESERVE_SYMBOL)?
        (RENAME_SYMBOL TO_SYMBOL identifier)?
        (ENABLE_SYMBOL | DISABLE_SYMBOL (ON_SYMBOL SLAVE_SYMBOL)?)?
        (COMMENT_SYMBOL stringLiteral)?
        (DO_SYMBOL compoundStatement)?
;

alterLogfileGroup:
    LOGFILE_SYMBOL GROUP_SYMBOL logfileGroupRef ADD_SYMBOL UNDOFILE_SYMBOL stringLiteral
        (INITIAL_SIZE_SYMBOL EQUAL_OPERATOR? sizeNumber)? WAIT_SYMBOL? ENGINE_SYMBOL EQUAL_OPERATOR? engineRef
        ;

alterServer:
    SERVER_SYMBOL serverRef serverOptions
;

alterTable:
    onlineOption? ({serverVersion < 50700}? IGNORE_SYMBOL)? TABLE_SYMBOL tableRef alterCommands?
;

alterCommands:
    alterCommandList
    | alterCommandList? (partitioning | removePartitioning)
    | (alterCommandsModifier+ COMMA_SYMBOL)? standaloneAlterCommands
;

alterCommandList:
    alterCommandsModifier+
    | (alterCommandsModifier+ COMMA_SYMBOL)? alterList
;

standaloneAlterCommands:
    DISCARD_SYMBOL TABLESPACE_SYMBOL
    | IMPORT_SYMBOL TABLESPACE_SYMBOL
    | {serverVersion >= 50100}? alterPartition
;

alterPartition:
    ADD_SYMBOL PARTITION_SYMBOL noWriteToBinLog?
        (
            OPEN_PAR_SYMBOL partitionDefinition CLOSE_PAR_SYMBOL
            | PARTITIONS_SYMBOL real_ulong_number
        )
    | DROP_SYMBOL PARTITION_SYMBOL identifierList
    | REBUILD_SYMBOL PARTITION_SYMBOL noWriteToBinLog? allOrPartitionNameList
    | OPTIMIZE_SYMBOL PARTITION_SYMBOL noWriteToBinLog? allOrPartitionNameList noWriteToBinLog? // yes, twice "no write to bin log".
    | ANALYZE_SYMBOL PARTITION_SYMBOL noWriteToBinLog? allOrPartitionNameList
    | CHECK_SYMBOL PARTITION_SYMBOL allOrPartitionNameList checkOption*
    | REPAIR_SYMBOL PARTITION_SYMBOL noWriteToBinLog? allOrPartitionNameList repairOption*
    | COALESCE_SYMBOL PARTITION_SYMBOL noWriteToBinLog? real_ulong_number
    | {serverVersion >= 50500}? TRUNCATE_SYMBOL PARTITION_SYMBOL allOrPartitionNameList
    | EXCHANGE_SYMBOL PARTITION_SYMBOL identifier WITH_SYMBOL TABLE_SYMBOL tableRef validation?
    | REORGANIZE_SYMBOL PARTITION_SYMBOL noWriteToBinLog? (identifierList INTO_SYMBOL partitionDefinitions)?
    | {serverVersion >= 50704}? DISCARD_SYMBOL PARTITION_SYMBOL allOrPartitionNameList TABLESPACE_SYMBOL
    | {serverVersion >= 50704}? IMPORT_SYMBOL PARTITION_SYMBOL allOrPartitionNameList TABLESPACE_SYMBOL
;

alterList:
    alterListItem (COMMA_SYMBOL (alterListItem | alterCommandsModifier))*
;

alterCommandsModifier:
    {serverVersion >= 50600}? alterAlgorithmOption
    | {serverVersion >= 50600}? alterLockOption
    | validation
;

alterListItem:
    createTableOption+
    | ADD_SYMBOL COLUMN_SYMBOL?
        (
            columnDefinition (FIRST_SYMBOL | AFTER_SYMBOL identifier)?
            | OPEN_PAR_SYMBOL columnDefinition (COMMA_SYMBOL columnDefinition)* CLOSE_PAR_SYMBOL
        )
    | ADD_SYMBOL keyDefinition
    | ALTER_SYMBOL COLUMN_SYMBOL? columnInternalRef (SET_SYMBOL DEFAULT_SYMBOL signedLiteral | DROP_SYMBOL DEFAULT_SYMBOL)
    | CHANGE_SYMBOL COLUMN_SYMBOL? fieldIdentifier fieldSpec (FIRST_SYMBOL | AFTER_SYMBOL identifier)?
    | MODIFY_SYMBOL COLUMN_SYMBOL? fieldIdentifier dataType attribute* (FIRST_SYMBOL | AFTER_SYMBOL identifier)?
    | DROP_SYMBOL
        (
            (INDEX_SYMBOL | KEY_SYMBOL) columnRef
            | COLUMN_SYMBOL? columnInternalRef
            | PRIMARY_SYMBOL fieldIdentifier
            | FOREIGN_SYMBOL KEY_SYMBOL
                (
                    // This part is no longer optional starting with 5.7.
                    {serverVersion >= 50700}? columnRef
                    | {serverVersion < 50700}? columnRef?
                )
        )
    | DISABLE_SYMBOL KEYS_SYMBOL
    | ENABLE_SYMBOL KEYS_SYMBOL
    | RENAME_SYMBOL (TO_SYMBOL | AS_SYMBOL)? tableRef
    | {serverVersion >= 50700}? RENAME_SYMBOL (INDEX_SYMBOL | KEY_SYMBOL) columnRef TO_SYMBOL columnRef
    | CONVERT_SYMBOL TO_SYMBOL charset charsetNameOrDefault (COLLATE_SYMBOL collationNameOrDefault)?
    | FORCE_SYMBOL
    | alterOrderClause
    | {serverVersion >= 50708}? UPGRADE_SYMBOL PARTITIONING_SYMBOL
;

keyDefinition:
    type = (INDEX_SYMBOL | KEY_SYMBOL) fieldIdentifier? keyAlgorithm? keyList normalKeyOption*
    | type = FULLTEXT_SYMBOL (INDEX_SYMBOL | KEY_SYMBOL)? fieldIdentifier? keyList fulltextKeyOption*
    | type = SPATIAL_SYMBOL (INDEX_SYMBOL | KEY_SYMBOL)? fieldIdentifier? keyList spatialKeyOption*
    | (CONSTRAINT_SYMBOL constraintName?)?
        (
            type = PRIMARY_SYMBOL KEY_SYMBOL fieldIdentifier? keyAlgorithm? keyList normalKeyOption*
            | type = UNIQUE_SYMBOL (INDEX_SYMBOL | KEY_SYMBOL)? fieldIdentifier? keyAlgorithm? keyList normalKeyOption*
            | type = FOREIGN_SYMBOL KEY_SYMBOL fieldIdentifier? keyList references
            | type = CHECK_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
        )
;

constraintName:
    fieldIdentifier
;

alterOrderClause:
    ORDER_SYMBOL BY_SYMBOL identifier direction (COMMA_SYMBOL identifier direction)*
;

alterAlgorithmOption:
    ALGORITHM_SYMBOL EQUAL_OPERATOR? ( DEFAULT_SYMBOL | identifier )
;

alterLockOption:
    LOCK_SYMBOL EQUAL_OPERATOR? (DEFAULT_SYMBOL | identifier)
;

indexLockAlgorithm:
    {serverVersion >= 50600}?
        (
            alterAlgorithmOption alterLockOption?
            | alterLockOption alterAlgorithmOption?
        )
;

validation:
    {serverVersion >= 50706}? (WITH_SYMBOL | WITHOUT_SYMBOL) VALIDATION_SYMBOL
;

removePartitioning:
    {serverVersion >= 50100}? REMOVE_SYMBOL PARTITIONING_SYMBOL
;

allOrPartitionNameList:
    ALL_SYMBOL
    | identifierList
;

alterTablespace:
    TABLESPACE_SYMBOL tablespaceRef
    (
        (ADD_SYMBOL | DROP_SYMBOL) DATAFILE_SYMBOL stringLiteral (alterTablespaceOption (COMMA_SYMBOL? alterTablespaceOption)*)?
        // The alternatives listed below are not documented but appear in the server grammar file.
        | CHANGE_SYMBOL DATAFILE_SYMBOL stringLiteral (change_tablespaceOption (COMMA_SYMBOL? change_tablespaceOption)*)?
        | (READ_ONLY_SYMBOL | READ_WRITE_SYMBOL)
        | NOT_SYMBOL ACCESSIBLE_SYMBOL
    )
;

alterTablespaceOption:
    INITIAL_SIZE_SYMBOL EQUAL_OPERATOR? sizeNumber
    | AUTOEXTEND_SIZE_SYMBOL EQUAL_OPERATOR? sizeNumber
    | MAX_SIZE_SYMBOL EQUAL_OPERATOR? sizeNumber
    | STORAGE_SYMBOL? ENGINE_SYMBOL EQUAL_OPERATOR? engineRef
    | (WAIT_SYMBOL | NO_WAIT_SYMBOL)
;

change_tablespaceOption:
    INITIAL_SIZE_SYMBOL EQUAL_OPERATOR? sizeNumber
    | AUTOEXTEND_SIZE_SYMBOL EQUAL_OPERATOR? sizeNumber
    | MAX_SIZE_SYMBOL EQUAL_OPERATOR? sizeNumber
;

alterView:
    (ALGORITHM_SYMBOL EQUAL_OPERATOR (UNDEFINED_SYMBOL | MERGE_SYMBOL | TEMPTABLE_SYMBOL))?
        definerClause?
        (SQL_SYMBOL SECURITY_SYMBOL (DEFINER_SYMBOL | INVOKER_SYMBOL))?
        VIEW_SYMBOL viewRef viewList? AS_SYMBOL selectStatement
        (WITH_SYMBOL (CASCADED_SYMBOL | LOCAL_SYMBOL)? CHECK_SYMBOL OPTION_SYMBOL)?
;

viewList:
    OPEN_PAR_SYMBOL identifierList CLOSE_PAR_SYMBOL
;

//--------------------------------------------------------------------------------------------------

createStatement:
    createDatabase
    | createTable
    | createFunction
    | createProcedure
    | createUdf
    | createLogfileGroup
    | createView
    | createTrigger
    | createIndex
    | createServer
    | createTablespace
    | {serverVersion >= 50100}? createEvent
;

createDatabase:
    CREATE_SYMBOL DATABASE_SYMBOL ifNotExists? schemaName databaseOption*
;

createTable:
    CREATE_SYMBOL TEMPORARY_SYMBOL? TABLE_SYMBOL ifNotExists? tableName
    (
        OPEN_PAR_SYMBOL createFieldList CLOSE_PAR_SYMBOL createTableOptions? partitioning? tableCreationSource?
        | OPEN_PAR_SYMBOL partitioning createSelect CLOSE_PAR_SYMBOL unionClause?
        | createTableOptions? partitioning? tableCreationSource
        | (LIKE_SYMBOL tableRef | OPEN_PAR_SYMBOL LIKE_SYMBOL tableRef CLOSE_PAR_SYMBOL)
    )
;

createFieldList:
    createItem (COMMA_SYMBOL createItem)*
;

createItem:
    columnDefinition
    | keyDefinition
;

tableCreationSource: // create3 in sql_yacc.yy
    (REPLACE_SYMBOL | IGNORE_SYMBOL)? AS_SYMBOL?
    (
        createSelect unionClause?
        | OPEN_PAR_SYMBOL createSelect CLOSE_PAR_SYMBOL unionOpt?
    )
;

// The select statement allowed for CREATE TABLE (and certain others) differs from the standard select statement.
createSelect:
    SELECT_SYMBOL selectOption* selectItemList tableExpression
;

createWithDefiner:
    CREATE_SYMBOL definerClause?
;

createEvent:
    createWithDefiner EVENT_SYMBOL ifNotExists? eventName ON_SYMBOL SCHEDULE_SYMBOL schedule
        (ON_SYMBOL COMPLETION_SYMBOL NOT_SYMBOL? PRESERVE_SYMBOL)?
        (ENABLE_SYMBOL | DISABLE_SYMBOL (ON_SYMBOL SLAVE_SYMBOL)?)?
        (COMMENT_SYMBOL stringLiteral)?
        DO_SYMBOL compoundStatement
;

createRoutine: // Rule for external use only.
    (createProcedure | createFunction | createUdf) SEMICOLON_SYMBOL? EOF
;

createProcedure:
    createWithDefiner PROCEDURE_SYMBOL procedureName
        OPEN_PAR_SYMBOL (procedureParameter (COMMA_SYMBOL procedureParameter)*)? CLOSE_PAR_SYMBOL
        routineCreateOption* compoundStatement
;

createFunction:
    createWithDefiner FUNCTION_SYMBOL functionName
        OPEN_PAR_SYMBOL (functionParameter (COMMA_SYMBOL functionParameter)*)? CLOSE_PAR_SYMBOL
        RETURNS_SYMBOL typeWithOptCollate routineCreateOption* compoundStatement
;

createUdf:
    CREATE_SYMBOL AGGREGATE_SYMBOL? FUNCTION_SYMBOL udfName RETURNS_SYMBOL
      type = (STRING_SYMBOL | INT_SYMBOL | REAL_SYMBOL | DECIMAL_SYMBOL) SONAME_SYMBOL stringLiteral
;

routineCreateOption:
    routineOption
    | NOT_SYMBOL? DETERMINISTIC_SYMBOL
;

routineAlterOptions:
    routineCreateOption+
;

routineOption:
    option = COMMENT_SYMBOL stringLiteral
    | option = LANGUAGE_SYMBOL SQL_SYMBOL
    | option = NO_SYMBOL SQL_SYMBOL
    | option = CONTAINS_SYMBOL SQL_SYMBOL
    | option = READS_SYMBOL SQL_SYMBOL DATA_SYMBOL
    | option = MODIFIES_SYMBOL SQL_SYMBOL DATA_SYMBOL
    | option = SQL_SYMBOL SECURITY_SYMBOL security = (DEFINER_SYMBOL | INVOKER_SYMBOL)
;

createIndex:
    CREATE_SYMBOL onlineOption?
    (
        UNIQUE_SYMBOL? type = INDEX_SYMBOL indexName keyAlgorithm? createIndexTarget normalKeyOption* indexLockAlgorithm?
        | type = FULLTEXT_SYMBOL INDEX_SYMBOL indexName createIndexTarget fulltextKeyOption* indexLockAlgorithm?
        | type = SPATIAL_SYMBOL INDEX_SYMBOL indexName createIndexTarget spatialKeyOption* indexLockAlgorithm?
    )
;

createIndexTarget:
    ON_SYMBOL tableRef keyList
;

createLogfileGroup:
    CREATE_SYMBOL LOGFILE_SYMBOL GROUP_SYMBOL logfileGroupName
        ADD_SYMBOL (UNDOFILE_SYMBOL | REDOFILE_SYMBOL) stringLiteral
        logfileGroupOptions?
;

logfileGroupOptions:
    logfileGroupOption (COMMA_SYMBOL? logfileGroupOption)*
;

logfileGroupOption:
    option = INITIAL_SIZE_SYMBOL EQUAL_OPERATOR? sizeNumber
    | option = (UNDO_BUFFER_SIZE_SYMBOL | REDO_BUFFER_SIZE_SYMBOL) EQUAL_OPERATOR? sizeNumber
    | option = NODEGROUP_SYMBOL EQUAL_OPERATOR? real_ulong_number
    | option = (WAIT_SYMBOL | NO_WAIT_SYMBOL)
    | option = COMMENT_SYMBOL EQUAL_OPERATOR? stringLiteral
    | STORAGE_SYMBOL? option = ENGINE_SYMBOL EQUAL_OPERATOR? engineRef
;

createServer:
    CREATE_SYMBOL SERVER_SYMBOL serverName
        FOREIGN_SYMBOL DATA_SYMBOL WRAPPER_SYMBOL textOrIdentifier serverOptions
;

serverOptions:
    OPTIONS_SYMBOL OPEN_PAR_SYMBOL serverOption (COMMA_SYMBOL serverOption)* CLOSE_PAR_SYMBOL
;

// Options for CREATE/ALTER SERVER, used for the federated storage engine.
serverOption:
    option = HOST_SYMBOL stringLiteral
    | option = DATABASE_SYMBOL stringLiteral
    | option = USER_SYMBOL stringLiteral
    | option = PASSWORD_SYMBOL stringLiteral
    | option = SOCKET_SYMBOL stringLiteral
    | option = OWNER_SYMBOL stringLiteral
    | option = PORT_SYMBOL ulong_number
;

createTablespace:
    CREATE_SYMBOL TABLESPACE_SYMBOL tablespaceName ADD_SYMBOL DATAFILE_SYMBOL stringLiteral
        (USE_SYMBOL LOGFILE_SYMBOL GROUP_SYMBOL logfileGroupRef)? tablespaceOptions?
;

tablespaceOptions:
    tablespaceOption (COMMA_SYMBOL? tablespaceOption)*
;

tablespaceOption:
    option = INITIAL_SIZE_SYMBOL EQUAL_OPERATOR? sizeNumber
    | option = AUTOEXTEND_SIZE_SYMBOL EQUAL_OPERATOR? sizeNumber
    | option = MAX_SIZE_SYMBOL EQUAL_OPERATOR? sizeNumber
    | option = EXTENT_SIZE_SYMBOL EQUAL_OPERATOR? sizeNumber
    | option = NODEGROUP_SYMBOL EQUAL_OPERATOR? real_ulong_number
    | STORAGE_SYMBOL? option = ENGINE_SYMBOL EQUAL_OPERATOR? engineRef
    | option = (WAIT_SYMBOL | NO_WAIT_SYMBOL)
    | option = COMMENT_SYMBOL EQUAL_OPERATOR? stringLiteral
    | {serverVersion >= 50707}? option = FILE_BLOCK_SIZE_SYMBOL EQUAL_OPERATOR? sizeNumber
;

createTrigger:
    createWithDefiner TRIGGER_SYMBOL triggerName
        timing = (BEFORE_SYMBOL | AFTER_SYMBOL) event = (INSERT_SYMBOL | UPDATE_SYMBOL | DELETE_SYMBOL)
        ON_SYMBOL tableRef FOR_SYMBOL EACH_SYMBOL ROW_SYMBOL triggerFollowsPrecedesClause?
        compoundStatement
;

triggerFollowsPrecedesClause:
    {serverVersion >= 50700}? ordering = (FOLLOWS_SYMBOL | PRECEDES_SYMBOL) textOrIdentifier // not a trigger reference!
;

createView:
    CREATE_SYMBOL viewReplaceOrAlgorithm?  definerClause?
        (SQL_SYMBOL SECURITY_SYMBOL (DEFINER_SYMBOL | INVOKER_SYMBOL))?
        VIEW_SYMBOL viewName viewList?
        AS_SYMBOL selectStatement
        (WITH_SYMBOL check = (CASCADED_SYMBOL | LOCAL_SYMBOL)? CHECK_SYMBOL OPTION_SYMBOL)?
;

viewReplaceOrAlgorithm:
    OR_SYMBOL REPLACE_SYMBOL viewAlgorithm?
    | viewAlgorithm
;

viewAlgorithm:
    ALGORITHM_SYMBOL EQUAL_OPERATOR algorithm = (UNDEFINED_SYMBOL | MERGE_SYMBOL | TEMPTABLE_SYMBOL)
;

//--------------------------------------------------------------------------------------------------

dropStatement:
    DROP_SYMBOL
    (
        type = DATABASE_SYMBOL ifExists? schemaRef
        | {serverVersion >= 50100}? type = EVENT_SYMBOL ifExists? eventRef
        | type = FUNCTION_SYMBOL ifExists? functionRef // Including UDFs.
        | type = PROCEDURE_SYMBOL ifExists? procedureRef
        | onlineOption? type = INDEX_SYMBOL indexRef ON_SYMBOL tableRef indexLockAlgorithm?
        | type = LOGFILE_SYMBOL GROUP_SYMBOL logfileGroupRef (dropLogfileGroupOption (COMMA_SYMBOL? dropLogfileGroupOption)*)?
        | type = SERVER_SYMBOL ifExists? serverRef
        | TEMPORARY_SYMBOL? type = (TABLE_SYMBOL | TABLES_SYMBOL) ifExists? tableRefList (RESTRICT_SYMBOL | CASCADE_SYMBOL)?
        | type = TABLESPACE_SYMBOL tablespaceRef (dropLogfileGroupOption (COMMA_SYMBOL? dropLogfileGroupOption)*)?
        | type = TRIGGER_SYMBOL ifExists? triggerRef
        | type = VIEW_SYMBOL ifExists? viewRefList (RESTRICT_SYMBOL | CASCADE_SYMBOL)?
    )
;

dropLogfileGroupOption:
    (WAIT_SYMBOL | NO_WAIT_SYMBOL) // Currently ignored by the server.
    | STORAGE_SYMBOL? ENGINE_SYMBOL EQUAL_OPERATOR? engineRef;

//--------------------------------------------------------------------------------------------------

renameTableStatement:
    RENAME_SYMBOL (TABLE_SYMBOL | TABLES_SYMBOL) renamePair (COMMA_SYMBOL renamePair)*
;

renamePair:
    tableRef TO_SYMBOL tableName
;

//--------------------------------------------------------------------------------------------------

truncateTableStatement:
    TRUNCATE_SYMBOL TABLE_SYMBOL? tableRef
;

//--------------- DML statements -------------------------------------------------------------------

callStatement:
    CALL_SYMBOL procedureRef (OPEN_PAR_SYMBOL expression_list? CLOSE_PAR_SYMBOL)?
;

deleteStatement:
    DELETE_SYMBOL deleteStatementOption*
        (
            FROM_SYMBOL
                (
                     tableRefListWithWildcard USING_SYMBOL join_table_list where_clause? // Multi table variant 1.
                    | tableRef partition_delete? where_clause? order_clause? simple_limit_clause? // Single table delete.
                )
            |  tableRefListWithWildcard FROM_SYMBOL join_table_list where_clause? // Multi table variant 2.
        )
;

partition_delete:
    {serverVersion >= 50602}? PARTITION_SYMBOL OPEN_PAR_SYMBOL identifierList CLOSE_PAR_SYMBOL
;

deleteStatementOption: // opt_delete_option in sql_yacc.yy, but the name collides with another rule (delete_options).
    QUICK_SYMBOL | LOW_PRIORITY_SYMBOL | QUICK_SYMBOL | IGNORE_SYMBOL
;

doStatement:
    DO_SYMBOL
    (
        {serverVersion < 50709}? expression_list
        | {serverVersion >= 50709}? selectItemList
    )
;

handlerStatement:
    HANDLER_SYMBOL
    (
        tableRef OPEN_SYMBOL (AS_SYMBOL? identifier)?
        | tableRefNoDb
            (
                CLOSE_SYMBOL
                | READ_SYMBOL handler_read_or_scan where_clause? limit_clause?
            )
    )
;

handler_read_or_scan:
    (FIRST_SYMBOL | NEXT_SYMBOL) // Scan function.
    | identifier
        (
            (FIRST_SYMBOL | NEXT_SYMBOL | PREV_SYMBOL | LAST_SYMBOL)
            | (EQUAL_OPERATOR | LESS_THAN_OPERATOR | GREATER_THAN_OPERATOR | LESS_OR_EQUAL_OPERATOR | GREATER_OR_EQUAL_OPERATOR)
                OPEN_PAR_SYMBOL values CLOSE_PAR_SYMBOL
        )
;

//--------------------------------------------------------------------------------------------------

insertStatement:
    INSERT_SYMBOL insert_lock_option? IGNORE_SYMBOL? INTO_SYMBOL? tableRef use_partition?
        insert_fieldSpec duplicate_key_update?
;

insert_lock_option:
    LOW_PRIORITY_SYMBOL
    | DELAYED_SYMBOL        // Only allowed if no select is used. Check in the semantic phase.
    | HIGH_PRIORITY_SYMBOL
;

insert_fieldSpec:
    (OPEN_PAR_SYMBOL fields? CLOSE_PAR_SYMBOL)?
        (
            insert_values
            | insertQueryExpression
        )
    | SET_SYMBOL column_assignment_list_with_default
;

fields:
    columnRefWithWildcard (COMMA_SYMBOL columnRefWithWildcard)*
;

insert_values:
    (VALUES_SYMBOL | VALUE_SYMBOL) insert_value_list
;

insertQueryExpression:
    createSelect unionClause?
    | OPEN_PAR_SYMBOL createSelect CLOSE_PAR_SYMBOL unionOpt?;

insert_value_list:
    OPEN_PAR_SYMBOL values? CLOSE_PAR_SYMBOL (COMMA_SYMBOL OPEN_PAR_SYMBOL values? CLOSE_PAR_SYMBOL)*
;

values:
    (expr | DEFAULT_SYMBOL) (COMMA_SYMBOL (expr | DEFAULT_SYMBOL))*
;

duplicate_key_update:
    ON_SYMBOL DUPLICATE_SYMBOL KEY_SYMBOL UPDATE_SYMBOL column_assignment_list_with_default
;

//--------------------------------------------------------------------------------------------------

loadStatement:
    LOAD_SYMBOL data_or_xml (LOW_PRIORITY_SYMBOL | CONCURRENT_SYMBOL)? LOCAL_SYMBOL? INFILE_SYMBOL stringLiteral
        (REPLACE_SYMBOL | IGNORE_SYMBOL)? INTO_SYMBOL TABLE_SYMBOL tableRef
        use_partition? charset_clause?
        xml_rows_identified_by?
        fields_clause? lines_clause?
        load_data_file_tail
;

data_or_xml:
    DATA_SYMBOL
    | {serverVersion >= 50500}? XML_SYMBOL
;

xml_rows_identified_by:
    {serverVersion >= 50500}? ROWS_SYMBOL IDENTIFIED_SYMBOL BY_SYMBOL textString
;

load_data_file_tail:
    (IGNORE_SYMBOL INT_NUMBER (LINES_SYMBOL | ROWS_SYMBOL))? load_data_file_target_list? (SET_SYMBOL column_assignment_list_with_default)?
;

load_data_file_target_list:
    OPEN_PAR_SYMBOL field_or_variable_list? CLOSE_PAR_SYMBOL
;

field_or_variable_list:
    (columnRef | user_variable) (COMMA_SYMBOL (columnRef | user_variable))*
;

//--------------------------------------------------------------------------------------------------

replaceStatement:
    REPLACE_SYMBOL (LOW_PRIORITY_SYMBOL | DELAYED_SYMBOL)? INTO_SYMBOL? tableRef
        use_partition? insert_fieldSpec
;

//--------------------------------------------------------------------------------------------------

selectStatement:
    SELECT_SYMBOL select_part2 unionClause?
    | OPEN_PAR_SYMBOL select_paren CLOSE_PAR_SYMBOL unionOpt?;

select_paren:
    SELECT_SYMBOL select_part2
    | OPEN_PAR_SYMBOL select_paren CLOSE_PAR_SYMBOL
;

select_from:
    from_clause where_clause? group_by_clause? having_clause? order_clause? limit_clause? procedure_analyse_clause?
;

select_part2:
    selectOption* selectItemList
        (
            order_clause? limit_clause?
            | into_clause select_from?
            | select_from into_clause?
        )
        select_lock_type?
;

tableExpression:
    from_clause?
    where_clause?
    group_by_clause?
    having_clause?
    order_clause?
    limit_clause?
    procedure_analyse_clause?
    select_lock_type?
;

subselect: // both subselect and query_expression_body in sql_yacc.yy.
    query_specification (UNION_SYMBOL union_option query_specification)*
;

select_part2_derived: // select_part2 equivalent for sub queries.
    (
        query_spec_option
        | {serverVersion <= 50100}? (SQL_NO_CACHE_SYMBOL | SQL_CACHE_SYMBOL)
    )* selectItemList
;

selectOption:
    query_spec_option
    | (SQL_NO_CACHE_SYMBOL | SQL_CACHE_SYMBOL)
    | {serverVersion >= 50704 && serverVersion < 50708}? MAX_STATEMENT_TIME_SYMBOL EQUAL_OPERATOR real_ulong_number
;

query_spec_option:
    ALL_SYMBOL
    | DISTINCT_SYMBOL
    | STRAIGHT_JOIN_SYMBOL
    | HIGH_PRIORITY_SYMBOL
    | SQL_SMALL_RESULT_SYMBOL
    | SQL_BIG_RESULT_SYMBOL
    | SQL_BUFFER_RESULT_SYMBOL
    | SQL_CALC_FOUND_ROWS_SYMBOL
;

selectItemList:
    (select_item | MULT_OPERATOR) (COMMA_SYMBOL select_item)*
;

select_item:
    tableWild
    | expr select_alias?
;

select_alias:
    AS_SYMBOL? (identifier | textString )
;

limit_clause:
    LIMIT_SYMBOL limit_options
;

simple_limit_clause:
    LIMIT_SYMBOL limit_option
;

limit_options:
    limit_option ((COMMA_SYMBOL | OFFSET_SYMBOL) limit_option)?
;

limit_option:
    identifier
    | (PARAM_MARKER | ULONGLONG_NUMBER | LONG_NUMBER | INT_NUMBER)
;

into_clause:
    INTO_SYMBOL
    (
        OUTFILE_SYMBOL stringLiteral charset_clause? fields_clause? lines_clause?
        | DUMPFILE_SYMBOL stringLiteral
        | AT_SIGN_SYMBOL? (textOrIdentifier | AT_TEXT_SUFFIX) (COMMA_SYMBOL AT_SIGN_SYMBOL? (textOrIdentifier | AT_TEXT_SUFFIX))*
    )
;

procedure_analyse_clause:
    PROCEDURE_SYMBOL ANALYSE_SYMBOL OPEN_PAR_SYMBOL (INT_NUMBER (COMMA_SYMBOL INT_NUMBER)?)? CLOSE_PAR_SYMBOL
;

having_clause:
    HAVING_SYMBOL expr
;

group_by_clause:
    GROUP_SYMBOL BY_SYMBOL order_list olap_option?
;

olap_option:
    WITH_SYMBOL CUBE_SYMBOL
    | WITH_SYMBOL ROLLUP_SYMBOL
;

order_clause:
    ORDER_SYMBOL BY_SYMBOL order_list
;

order_by_or_limit:
    order_clause limit_clause?
    | limit_clause
;

direction:
    ASC_SYMBOL
    | DESC_SYMBOL
;

from_clause:
    FROM_SYMBOL tablekeyList
;

where_clause:
    WHERE_SYMBOL expr
;

tablekeyList:
    join_table_list
    | DUAL_SYMBOL
;

join_table_list: // join_table_list + derived_table_list in sql_yacc.yy.
     escaped_tableReference (COMMA_SYMBOL escaped_tableReference)*
;

// For the ODBC OJ syntax we do as the server does. This is what the server grammar says about it:
//   The ODBC escape syntax for Outer Join is: '{' OJ join_table '}'
//   The parser does not define OJ as a token, any ident is accepted
//   instead in $2 (ident). Also, all productions from tableRef can
//   be escaped, not only join_table. Both syntax extensions are safe
//   and are ignored.
escaped_tableReference:
    tableReference
    | OPEN_CURLY_SYMBOL identifier tableReference CLOSE_CURLY_SYMBOL
;

tableReference: // tableRef in sql_yacc.yy, we use tableRef here for a different rule.
    table_factor join*
;

join:
    join_table
;

table_factor:
    SELECT_SYMBOL selectOption* selectItemList tableExpression
    | OPEN_PAR_SYMBOL select_table_factor_union CLOSE_PAR_SYMBOL table_alias?
    | tableRef use_partition? table_alias? index_hint_list?
;

select_table_factor_union:
    (tablekeyList order_by_or_limit?) (UNION_SYMBOL union_option? query_specification)*
;

query_specification:
    SELECT_SYMBOL select_part2_derived tableExpression
    | OPEN_PAR_SYMBOL select_paren_derived CLOSE_PAR_SYMBOL order_by_or_limit?
;

select_paren_derived:
    SELECT_SYMBOL select_part2_derived tableExpression
    | OPEN_PAR_SYMBOL select_paren_derived CLOSE_PAR_SYMBOL
;

join_table: // Like the same named rule in sql_yacc.yy but with removed left recursion.
    (INNER_SYMBOL | CROSS_SYMBOL)? JOIN_SYMBOL tableReference
        (
            ON_SYMBOL expr
            | USING_SYMBOL identifierListWithParentheses
        )?
    | STRAIGHT_JOIN_SYMBOL table_factor (ON_SYMBOL expr)?
    | (LEFT_SYMBOL | RIGHT_SYMBOL) OUTER_SYMBOL? JOIN_SYMBOL table_factor
        (
            join* ON_SYMBOL expr
            | USING_SYMBOL identifierListWithParentheses
        )
    | NATURAL_SYMBOL ((LEFT_SYMBOL | RIGHT_SYMBOL) OUTER_SYMBOL?)? JOIN_SYMBOL table_factor
;

unionClause:
  UNION_SYMBOL union_option? selectStatement
 ;

union_option:
    DISTINCT_SYMBOL
    | ALL_SYMBOL
;

unionOpt:
    unionClause
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

updateStatement:
    UPDATE_SYMBOL LOW_PRIORITY_SYMBOL? IGNORE_SYMBOL? join_table_list
        SET_SYMBOL column_assignment_list_with_default where_clause? order_clause? simple_limit_clause?
;

//--------------------------------------------------------------------------------------------------

transactionOrLockingStatement:
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
    // alternative is moved to the setStatement rule.
    //| SET_SYMBOL option_type? TRANSACTION_SYMBOL set_transaction_characteristic (COMMA_SYMBOL set_transaction_characteristic)*
;

// BEGIN WORK is separated from transactional statements as it must not appear as part of a stored program.
begin_work:
    BEGIN_SYMBOL WORK_SYMBOL?
;

transaction_characteristic:
    WITH_SYMBOL CONSISTENT_SYMBOL SNAPSHOT_SYMBOL
    | {serverVersion >= 50605}? READ_SYMBOL (WRITE_SYMBOL | ONLY_SYMBOL)
;

set_transaction_characteristic:
    ISOLATION_SYMBOL LEVEL_SYMBOL isolation_level
    | {serverVersion >= 50605}? READ_SYMBOL (WRITE_SYMBOL | ONLY_SYMBOL)
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
    tableRef table_alias? lock_option
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
    {serverVersion >= 50704}? (CONVERT_SYMBOL XID_SYMBOL)?
    | /* empty */
;

xid:
    textString (COMMA_SYMBOL textString (COMMA_SYMBOL ulong_number)?)?
;

//--------------------------------------------------------------------------------------------------

replicationStatement:
    PURGE_SYMBOL (BINARY_SYMBOL | MASTER_SYMBOL) LOGS_SYMBOL (TO_SYMBOL stringLiteral | BEFORE_SYMBOL expr)
    | change_master
    | {serverVersion >= 50700}? change_replication
    /* Defined in the miscellaneous statement to avoid ambiguities.
    | RESET_SYMBOL MASTER_SYMBOL
    | RESET_SYMBOL SLAVE_SYMBOL ALL_SYMBOL
    */
    | slave
    | {serverVersion < 50500}? replication_load
    | {serverVersion > 50706}? group_replication
;

replication_load:
    LOAD_SYMBOL (DATA_SYMBOL | TABLE_SYMBOL tableRef) FROM_SYMBOL MASTER_SYMBOL
;

change_master:
    CHANGE_SYMBOL MASTER_SYMBOL TO_SYMBOL change_master_options channel?
;

change_master_options:
    master_option (COMMA_SYMBOL master_option)*
;

master_option:
    MASTER_HOST_SYMBOL EQUAL_OPERATOR textString_no_linebreak
    | MASTER_BIND_SYMBOL EQUAL_OPERATOR textString_no_linebreak
    | MASTER_USER_SYMBOL EQUAL_OPERATOR textString_no_linebreak
    | MASTER_PASSWORD_SYMBOL EQUAL_OPERATOR textString_no_linebreak
    | MASTER_PORT_SYMBOL EQUAL_OPERATOR ulong_number
    | MASTER_CONNECT_RETRY_SYMBOL EQUAL_OPERATOR ulong_number
    | MASTER_RETRY_COUNT_SYMBOL EQUAL_OPERATOR ulong_number
    | MASTER_DELAY_SYMBOL EQUAL_OPERATOR ulong_number
    | MASTER_SSL_SYMBOL EQUAL_OPERATOR ulong_number
    | MASTER_SSL_CA_SYMBOL EQUAL_OPERATOR textString_no_linebreak
    | MASTER_TLS_VERSION_SYMBOL EQUAL_OPERATOR textString_no_linebreak
    | MASTER_SSL_CAPATH_SYMBOL EQUAL_OPERATOR textString_no_linebreak
    | MASTER_SSL_CERT_SYMBOL EQUAL_OPERATOR textString_no_linebreak
    | MASTER_SSL_CIPHER_SYMBOL EQUAL_OPERATOR textString_no_linebreak
    | MASTER_SSL_KEY_SYMBOL EQUAL_OPERATOR textString_no_linebreak
    | MASTER_SSL_VERIFY_SERVER_CERT_SYMBOL EQUAL_OPERATOR ulong_number
    | MASTER_SSL_CRL_SYMBOL EQUAL_OPERATOR stringLiteral
    | MASTER_SSL_CRLPATH_SYMBOL EQUAL_OPERATOR textString_no_linebreak
    | MASTER_HEARTBEAT_PERIOD_SYMBOL EQUAL_OPERATOR ulong_number
    | IGNORE_SERVER_IDS_SYMBOL EQUAL_OPERATOR server_id_list
    | MASTER_AUTO_POSITION_SYMBOL EQUAL_OPERATOR ulong_number
    | master_file_def
;

master_file_def:
    MASTER_LOG_FILE_SYMBOL EQUAL_OPERATOR textString_no_linebreak
    | MASTER_LOG_POS_SYMBOL EQUAL_OPERATOR ulonglong_number
    | RELAY_LOG_FILE_SYMBOL EQUAL_OPERATOR textString_no_linebreak
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
    | REPLICATE_WILD_DO_TABLE_SYMBOL EQUAL_OPERATOR OPEN_PAR_SYMBOL filter_stringList? CLOSE_PAR_SYMBOL
    | REPLICATE_WILD_IGNORE_TABLE_SYMBOL EQUAL_OPERATOR OPEN_PAR_SYMBOL filter_stringList? CLOSE_PAR_SYMBOL
    | REPLICATE_REWRITE_DB_SYMBOL EQUAL_OPERATOR OPEN_PAR_SYMBOL filter_db_pair_list? CLOSE_PAR_SYMBOL
;

filter_db_list:
    schemaRef (COMMA_SYMBOL schemaRef)*
;

filter_table_list:
    filterTableRef (COMMA_SYMBOL filterTableRef)*
;

filter_stringList:
    filter_wild_db_table_string (COMMA_SYMBOL filter_wild_db_table_string)*
;

filter_wild_db_table_string:
    textString_no_linebreak // sql_yacc.yy checks for the existance of at least one dot char in the string.
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
        | {serverVersion >= 50606}? (SQL_BEFORE_GTIDS_SYMBOL | SQL_AFTER_GTIDS_SYMBOL) EQUAL_OPERATOR textString
        | {serverVersion >= 50606}? SQL_AFTER_MTS_GAPS_SYMBOL
    )
    (COMMA_SYMBOL master_file_def)*
;

slave_connection_options:
    {serverVersion >= 50604}? (USER_SYMBOL EQUAL_OPERATOR textString)? (PASSWORD_SYMBOL EQUAL_OPERATOR textString)?
        (DEFAULT_AUTH_SYMBOL EQUAL_OPERATOR textString)? (PLUGIN_DIR_SYMBOL EQUAL_OPERATOR textString)?
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

preparedStatement:
    PREPARE_SYMBOL identifier FROM_SYMBOL (stringLiteral | user_variable)
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

accountManagementStatement:
    {serverVersion >= 50606}? alter_user
    | create_user
    | drop_user
    | {serverVersion >= 50500}? grant_proxy
    | grant
    | rename_user
    | revoke_statement
    | set_password
;

alter_user:
    ALTER_SYMBOL USER_SYMBOL ({serverVersion >= 50706}? ifExists | /* empty */) alter_user_tail
;

alter_user_tail:
    grant_list create_user_tail
    | {serverVersion >= 50706}? USER_SYMBOL parentheses IDENTIFIED_SYMBOL BY_SYMBOL textString
;

create_user:
    CREATE_SYMBOL USER_SYMBOL ({serverVersion >= 50706}? ifNotExists | /* empty */) grant_list create_user_tail
;

create_user_tail:
    {serverVersion >= 50706}? require_clause? connect_options? account_lock_password_expire_options?
    | /* empty */
;

require_clause:
    REQUIRE_SYMBOL (require_list | (SSL_SYMBOL | X509_SYMBOL | NONE_SYMBOL))
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
            | (NEVER_SYMBOL | DEFAULT_SYMBOL)
        )?
;

drop_user:
    DROP_SYMBOL USER_SYMBOL ({serverVersion >= 50706}? ifExists | /* empty */) user_list
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
        ALL_SYMBOL PRIVILEGES_SYMBOL? COMMA_SYMBOL GRANT_SYMBOL OPTION_SYMBOL FROM_SYMBOL user_list
        | grant_privileges privilege_target FROM_SYMBOL user_list
        | {serverVersion >= 50500}? PROXY_SYMBOL ON_SYMBOL user FROM_SYMBOL user_list
    )
;

privilege_target:
    ON_SYMBOL grant_object_type privilege_level
;

set_password:
    SET_SYMBOL PASSWORD_SYMBOL (FOR_SYMBOL user)? equal
    (
        PASSWORD_SYMBOL OPEN_PAR_SYMBOL textString CLOSE_PAR_SYMBOL
        | {serverVersion < 50706}? OLD_PASSWORD_SYMBOL OPEN_PAR_SYMBOL textString CLOSE_PAR_SYMBOL
        | textString
    )
;

grant_object_type:
    TABLE_SYMBOL?
    | (FUNCTION_SYMBOL | PROCEDURE_SYMBOL)
;

grant_privileges:
    ALL_SYMBOL PRIVILEGES_SYMBOL?
    | privilege_type (COMMA_SYMBOL privilege_type)*
;

privilege_type:
    ALTER_SYMBOL ROUTINE_SYMBOL?
    | CREATE_SYMBOL (TEMPORARY_SYMBOL TABLES_SYMBOL | (ROUTINE_SYMBOL | TABLESPACE_SYMBOL | USER_SYMBOL | VIEW_SYMBOL))?
    | GRANT_SYMBOL OPTION_SYMBOL
    | INSERT_SYMBOL identifierListWithParentheses?
    | LOCK_SYMBOL TABLES_SYMBOL
    | REFERENCES_SYMBOL identifierListWithParentheses?
    | REPLICATION_SYMBOL (CLIENT_SYMBOL | SLAVE_SYMBOL)
    | SELECT_SYMBOL identifierListWithParentheses?
    | SHOW_SYMBOL DATABASES_SYMBOL
    | SHOW_SYMBOL VIEW_SYMBOL
    | UPDATE_SYMBOL identifierListWithParentheses?
    | (
        DELETE_SYMBOL
        | DROP_SYMBOL
        | EVENT_SYMBOL
        | EXECUTE_SYMBOL
        | FILE_SYMBOL
        | INDEX_SYMBOL
        | PROCESS_SYMBOL
        | PROXY_SYMBOL
        | RELOAD_SYMBOL
        | SHUTDOWN_SYMBOL
        | SUPER_SYMBOL
        | TRIGGER_SYMBOL
        | USAGE_SYMBOL
      )
;

privilege_level:
    MULT_OPERATOR (DOT_SYMBOL MULT_OPERATOR)?
    | identifier (DOT_SYMBOL MULT_OPERATOR | dotIdentifier)?
;

require_list:
    require_list_element (AND_SYMBOL? require_list_element)*
;

require_list_element:
    CIPHER_SYMBOL textString
    | ISSUER_SYMBOL textString
    | SUBJECT_SYMBOL textString
;

grant_option:
    GRANT_SYMBOL OPTION_SYMBOL
    | MAX_QUERIES_PER_HOUR_SYMBOL ulong_number
    | MAX_UPDATES_PER_HOUR_SYMBOL ulong_number
    | MAX_CONNECTIONS_PER_HOUR_SYMBOL ulong_number
    | MAX_USER_CONNECTIONS_SYMBOL ulong_number
;

//--------------------------------------------------------------------------------------------------

tableAdministrationStatement:
    ANALYZE_SYMBOL noWriteToBinLog? TABLE_SYMBOL tableRefList
    | CHECK_SYMBOL TABLE_SYMBOL tableRefList checkOption*
    | CHECKSUM_SYMBOL TABLE_SYMBOL tableRefList (QUICK_SYMBOL | EXTENDED_SYMBOL)?
    | OPTIMIZE_SYMBOL noWriteToBinLog? TABLE_SYMBOL tableRefList
    | REPAIR_SYMBOL noWriteToBinLog? TABLE_SYMBOL tableRefList repairOption*
    | {serverVersion < 50500}? BACKUP_SYMBOL TABLE_SYMBOL tableRefList TO_SYMBOL stringLiteral
    | {serverVersion < 50500}? RESTORE_SYMBOL TABLE_SYMBOL tableRefList FROM_SYMBOL stringLiteral
;

checkOption:
    FOR_SYMBOL UPGRADE_SYMBOL
    | (QUICK_SYMBOL | FAST_SYMBOL | MEDIUM_SYMBOL | EXTENDED_SYMBOL | CHANGED_SYMBOL)
;

repairOption:
    QUICK_SYMBOL | EXTENDED_SYMBOL | USE_FRM_SYMBOL
;

//--------------------------------------------------------------------------------------------------

installUninstallStatment:
    INSTALL_SYMBOL PLUGIN_SYMBOL identifier SONAME_SYMBOL stringLiteral
    | UNINSTALL_SYMBOL PLUGIN_SYMBOL identifier
;

//--------------------------------------------------------------------------------------------------

setStatement:
    SET_SYMBOL
        (
             option_type? TRANSACTION_SYMBOL set_transaction_characteristic
            // ONE_SHOT is available only until 5.6. We don't need a predicate here, however. Handling it in the lexer is enough.
            | ONE_SHOT_SYMBOL? option_value_no_option_type (COMMA_SYMBOL option_value_list)?
            | option_type option_value_following_option_type (COMMA_SYMBOL option_value_list)?

            // SET PASSWORD is handled in an own rule.
        )
;

option_value_no_option_type:
    NAMES_SYMBOL
        (
            equal expr
            | charsetNameOrDefault (COLLATE_SYMBOL collationNameOrDefault)?
        )
    | variable_name equal setExpressionOrDefault
    | user_variable equal expr
    | system_variable equal setExpressionOrDefault
    | charset_clause
;

option_value_following_option_type:
    variable_name equal setExpressionOrDefault
;

setExpressionOrDefault:
    expr
    | (DEFAULT_SYMBOL | ON_SYMBOL | ALL_SYMBOL | BINARY_SYMBOL)
;

option_value_list:
    option_value (COMMA_SYMBOL option_value)*
;

option_value:
    option_type variable_name equal setExpressionOrDefault
    | option_value_no_option_type
;

//--------------------------------------------------------------------------------------------------

showStatement:
    SHOW_SYMBOL
    (
        {serverVersion < 50700}? AUTHORS_SYMBOL
        | DATABASES_SYMBOL like_or_where?
        | FULL_SYMBOL? TABLES_SYMBOL in_db? like_or_where?
        | FULL_SYMBOL? TRIGGERS_SYMBOL in_db? like_or_where?
        | EVENTS_SYMBOL in_db? like_or_where?
        | TABLE_SYMBOL STATUS_SYMBOL in_db? like_or_where?
        | OPEN_SYMBOL TABLES_SYMBOL in_db? like_or_where?
        | {(serverVersion >= 50105) && (serverVersion < 50500)}? PLUGIN_SYMBOL // Supported between 5.1.5 and 5.5.0.
        | {serverVersion >= 50500}? PLUGINS_SYMBOL
        | ENGINE_SYMBOL engineRef (STATUS_SYMBOL | MUTEX_SYMBOL)
        | FULL_SYMBOL? COLUMNS_SYMBOL (FROM_SYMBOL | IN_SYMBOL) tableRef in_db? like_or_where?
        | (BINARY_SYMBOL | MASTER_SYMBOL) LOGS_SYMBOL
        | SLAVE_SYMBOL
            (
                HOSTS_SYMBOL
                | STATUS_SYMBOL non_blocking channel?
            )
        | (BINLOG_SYMBOL | RELAYLOG_SYMBOL) EVENTS_SYMBOL (IN_SYMBOL textString)? (FROM_SYMBOL ulonglong_number)? limit_clause? channel?
        | (INDEX_SYMBOL | INDEXES_SYMBOL | KEYS_SYMBOL) from_or_in tableRef in_db? where_clause?
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
        | {serverVersion < 50700}? CONTRIBUTORS_SYMBOL
        | GRANTS_SYMBOL (FOR_SYMBOL user)?
        | MASTER_SYMBOL STATUS_SYMBOL
        | CREATE_SYMBOL
            (
                DATABASE_SYMBOL ifNotExists? schemaRef
                | EVENT_SYMBOL eventRef
                | FUNCTION_SYMBOL functionRef
                | PROCEDURE_SYMBOL procedureRef
                | TABLE_SYMBOL tableRef
                | TRIGGER_SYMBOL triggerRef
                | VIEW_SYMBOL viewRef
                | {serverVersion >= 50704}? USER_SYMBOL user
            )
        | PROCEDURE_SYMBOL STATUS_SYMBOL like_or_where?
        | FUNCTION_SYMBOL STATUS_SYMBOL like_or_where?
        | PROCEDURE_SYMBOL CODE_SYMBOL procedureRef
        | FUNCTION_SYMBOL CODE_SYMBOL functionRef
        | {serverVersion < 50500}? INNODB_SYMBOL STATUS_SYMBOL // Deprecated in 5.5.
    )
;

non_blocking:
    {serverVersion >= 50700 && serverVersion < 50706}? NONBLOCKING_SYMBOL?
    | /* empty */
;

from_or_in:
    FROM_SYMBOL | IN_SYMBOL
;

in_db:
    from_or_in identifier
;

profile_type:
    BLOCK_SYMBOL IO_SYMBOL
    | CONTEXT_SYMBOL SWITCHES_SYMBOL
    | PAGE_SYMBOL FAULTS_SYMBOL
    | (ALL_SYMBOL | CPU_SYMBOL | IPC_SYMBOL | MEMORY_SYMBOL | SOURCE_SYMBOL | SWAPS_SYMBOL)
;

//--------------------------------------------------------------------------------------------------

otherAdministrativeStatement:
    BINLOG_SYMBOL stringLiteral
    | CACHE_SYMBOL INDEX_SYMBOL key_cache_list_or_parts IN_SYMBOL (identifier | DEFAULT_SYMBOL)
    | FLUSH_SYMBOL noWriteToBinLog?
        (
            flush_tables
            | flush_option (COMMA_SYMBOL flush_option)*
        )
    | KILL_SYMBOL  (CONNECTION_SYMBOL | QUERY_SYMBOL)? expr
    | LOAD_SYMBOL INDEX_SYMBOL INTO_SYMBOL CACHE_SYMBOL load_table_index_list
    | RESET_SYMBOL reset_option (COMMA_SYMBOL reset_option)*
    | {serverVersion >= 50709}? SHUTDOWN_SYMBOL
;

key_cache_list_or_parts:
    key_cache_list
    | assign_to_keycache_partition
;

key_cache_list:
    assign_to_keycache (COMMA_SYMBOL assign_to_keycache)*
;

assign_to_keycache:
    tableRef cache_keys_spec?
;

assign_to_keycache_partition:
    tableRef PARTITION_SYMBOL OPEN_PAR_SYMBOL (ALL_SYMBOL | identifierList) CLOSE_PAR_SYMBOL cache_keys_spec?
;

cache_keys_spec:
    (KEY_SYMBOL | INDEX_SYMBOL) OPEN_PAR_SYMBOL (key_usage_element (COMMA_SYMBOL key_usage_element)*)? CLOSE_PAR_SYMBOL
;

key_usage_element:
    identifier
    | PRIMARY_SYMBOL
;

flush_option:
    (DES_KEY_FILE_SYMBOL | HOSTS_SYMBOL | PRIVILEGES_SYMBOL | STATUS_SYMBOL | USER_RESOURCES_SYMBOL)
    | log_type? LOGS_SYMBOL
    | RELAY_SYMBOL LOGS_SYMBOL channel?
    | QUERY_SYMBOL CACHE_SYMBOL
    | {serverVersion >= 50706}? OPTIMIZER_COSTS_SYMBOL
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
        | identifierList flush_tables_options?
    )?
;

flush_tables_options:
    {serverVersion >= 50606}? FOR_SYMBOL EXPORT_SYMBOL
    | WITH_SYMBOL READ_SYMBOL LOCK_SYMBOL
;

load_table_index_list:
    tableRef load_table_index_partion?
        ((INDEX_SYMBOL | KEY_SYMBOL)? identifierListWithParentheses)? (IGNORE_SYMBOL LEAVES_SYMBOL)?
;

load_table_index_partion:
    {serverVersion >= 50500}? (PARTITION_SYMBOL OPEN_PAR_SYMBOL (identifierList | ALL_SYMBOL) CLOSE_PAR_SYMBOL)
;

reset_option:
    MASTER_SYMBOL
    | QUERY_SYMBOL CACHE_SYMBOL
    | SLAVE_SYMBOL ALL_SYMBOL? channel?
;

//--------------------------------------------------------------------------------------------------

utilityStatement:
    describe_command
        (
            tableRef (textString | identifier)?
            |
                (
                    // The format specifier is defined here like in the server grammar but actually defined are only
                    // traditional and json, anything else results in a server error.
                    EXTENDED_SYMBOL // deprecated since 5.7
                    | {serverVersion >= 50105}? PARTITIONS_SYMBOL // deprecated since 5.7
                    | {serverVersion >= 50605}? FORMAT_SYMBOL EQUAL_OPERATOR textOrIdentifier
                )? explainable_statement
        )
    | HELP_SYMBOL textOrIdentifier
    | useCommand
;

describe_command:
    DESCRIBE_SYMBOL | DESC_SYMBOL
;

// Before server version 5.6 only select statements were explainable.
explainable_statement:
    selectStatement
    | {serverVersion >= 50603}?
        (
            deleteStatement
            | insertStatement
            | replaceStatement
            | updateStatement
        )
    | {serverVersion >= 50700}? FOR_SYMBOL CONNECTION_SYMBOL real_ulong_number
;

useCommand:
    USE_SYMBOL identifier
;

//----------------- Expression support -------------------------------------------------------------

expr:
    bool_pri (IS_SYMBOL not_rule? type = (TRUE_SYMBOL | FALSE_SYMBOL | UNKNOWN_SYMBOL))? # exprIs
    | NOT_SYMBOL expr                                                                    # exprNot
    | expr op = (AND_SYMBOL | LOGICAL_AND_OPERATOR) expr                                 # exprAnd
    | expr XOR_SYMBOL expr                                                               # exprXor
    | expr op = (OR_SYMBOL | LOGICAL_OR_OPERATOR) expr                                   # exprOr
;

bool_pri:
    predicate                                                                               # primaryExprPredicate
    | bool_pri IS_SYMBOL not_rule? NULL_SYMBOL                                              # primaryExprIsNull
    | bool_pri comp_op predicate                                                            # primaryExprCompare
    | bool_pri comp_op (ALL_SYMBOL | ANY_SYMBOL) OPEN_PAR_SYMBOL subselect CLOSE_PAR_SYMBOL # primaryExprAllAny
;

comp_op:
    EQUAL_OPERATOR
    | NULL_SAFE_EQUAL_OPERATOR
    | GREATER_OR_EQUAL_OPERATOR
    | GREATER_THAN_OPERATOR
    | LESS_OR_EQUAL_OPERATOR
    | LESS_THAN_OPERATOR
    | NOT_EQUAL_OPERATOR
;

predicate:
    bit_expr (not_rule? predicateOperations)?     # predicateExprOperations
    | bit_expr SOUNDS_SYMBOL LIKE_SYMBOL bit_expr # predicateExprSoundsLike
;

predicateOperations:
    IN_SYMBOL OPEN_PAR_SYMBOL (subselect | expression_list) CLOSE_PAR_SYMBOL # predicateExprIn
    | BETWEEN_SYMBOL bit_expr AND_SYMBOL predicate                           # predicateExprBetween
    | LIKE_SYMBOL simple_expr (ESCAPE_SYMBOL simple_expr)?                   # predicateExprLike
    | REGEXP_SYMBOL bit_expr                                                 # predicateExprRegex
;

bit_expr:
    simple_expr
    | bit_expr op = BITWISE_XOR_OPERATOR bit_expr
    | bit_expr op = (
        MULT_OPERATOR | DIV_OPERATOR | MOD_OPERATOR | DIV_SYMBOL | MOD_SYMBOL
    ) bit_expr
    | bit_expr op = (PLUS_OPERATOR | MINUS_OPERATOR) bit_expr
    | bit_expr op = (PLUS_OPERATOR | MINUS_OPERATOR) INTERVAL_SYMBOL expr interval
    | bit_expr op = (SHIFT_LEFT_OPERATOR | SHIFT_RIGHT_OPERATOR) bit_expr
    | bit_expr op = BITWISE_AND_OPERATOR bit_expr
    | bit_expr op = BITWISE_OR_OPERATOR bit_expr
;

simple_expr:
    variable                                                                            # simpleExprVariable
    | simpleIdentifier json_operator?                                                   # simpleExprIdentifier
    | runtime_function_call                                                             # simpleExprRuntimeFunction
    | function_call                                                                     # simpleExprFunction
    | simple_expr COLLATE_SYMBOL textOrIdentifier                                       # simpleExprCollate
    | literal                                                                           # simpleExprLiteral
    | PARAM_MARKER                                                                      # simpleExprParamMarker
    | sum_expr                                                                          # simpleExprSum
    | simple_expr CONCAT_PIPES_SYMBOL simple_expr                                       # simpleExprConcat
    | op = (PLUS_OPERATOR | MINUS_OPERATOR | BITWISE_NOT_OPERATOR) simple_expr          # simpleExprUnary
    | not2_rule simple_expr                                                             # simpleExprNot
    | OPEN_PAR_SYMBOL subselect CLOSE_PAR_SYMBOL                                        # simpleExprSubselect
    | ROW_SYMBOL? OPEN_PAR_SYMBOL expression_list CLOSE_PAR_SYMBOL                      # simpleExprList
    | EXISTS_SYMBOL OPEN_PAR_SYMBOL subselect CLOSE_PAR_SYMBOL                          # simpleExprExists
    | OPEN_CURLY_SYMBOL identifier expr CLOSE_CURLY_SYMBOL                              # simpleExprOdbc
    | MATCH_SYMBOL ident_list_arg AGAINST_SYMBOL
        OPEN_PAR_SYMBOL bit_expr fulltext_options CLOSE_PAR_SYMBOL                      # simpleExprMatch
    | BINARY_SYMBOL simple_expr                                                         # simpleExprBinary
    | CAST_SYMBOL OPEN_PAR_SYMBOL expr AS_SYMBOL cast_type CLOSE_PAR_SYMBOL             # simpleExprCast
    | CASE_SYMBOL expr? (whenExpression thenExpression)+ elseExpression? END_SYMBOL     # simpleExprCase
    | CONVERT_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL cast_type CLOSE_PAR_SYMBOL       # simpleExprConvert
    | CONVERT_SYMBOL OPEN_PAR_SYMBOL expr USING_SYMBOL charsetName CLOSE_PAR_SYMBOL     # simpleExprConvertUsing
    | DEFAULT_SYMBOL OPEN_PAR_SYMBOL simpleIdentifier CLOSE_PAR_SYMBOL                  # simpleExprDefault
    | VALUES_SYMBOL OPEN_PAR_SYMBOL simpleIdentifier CLOSE_PAR_SYMBOL                   # simpleExprValues
    | INTERVAL_SYMBOL expr interval PLUS_OPERATOR expr                                  # simpleExprInterval
;

json_operator:
  {serverVersion >= 50708}? JSON_SEPARATOR_SYMBOL textString
  | {serverVersion >= 50713}? JSON_UNQUOTED_SEPARATOR_SYMBOL textString
;

sum_expr:
    AVG_SYMBOL OPEN_PAR_SYMBOL in_sum_expr CLOSE_PAR_SYMBOL
    | AVG_SYMBOL OPEN_PAR_SYMBOL DISTINCT_SYMBOL in_sum_expr CLOSE_PAR_SYMBOL
    | BITWISE_AND_OPERATOR OPEN_PAR_SYMBOL in_sum_expr CLOSE_PAR_SYMBOL
    | BITWISE_OR_OPERATOR OPEN_PAR_SYMBOL in_sum_expr CLOSE_PAR_SYMBOL
    | BITWISE_XOR_OPERATOR OPEN_PAR_SYMBOL in_sum_expr CLOSE_PAR_SYMBOL
    | COUNT_SYMBOL OPEN_PAR_SYMBOL ALL_SYMBOL? MULT_OPERATOR CLOSE_PAR_SYMBOL
    | COUNT_SYMBOL OPEN_PAR_SYMBOL in_sum_expr CLOSE_PAR_SYMBOL
    | COUNT_SYMBOL OPEN_PAR_SYMBOL DISTINCT_SYMBOL expression_list CLOSE_PAR_SYMBOL
    | MIN_SYMBOL OPEN_PAR_SYMBOL in_sum_expr CLOSE_PAR_SYMBOL
    | MIN_SYMBOL OPEN_PAR_SYMBOL DISTINCT_SYMBOL in_sum_expr CLOSE_PAR_SYMBOL
    | MAX_SYMBOL OPEN_PAR_SYMBOL in_sum_expr CLOSE_PAR_SYMBOL
    | MAX_SYMBOL OPEN_PAR_SYMBOL DISTINCT_SYMBOL in_sum_expr CLOSE_PAR_SYMBOL
    | STD_SYMBOL OPEN_PAR_SYMBOL in_sum_expr CLOSE_PAR_SYMBOL
    | VARIANCE_SYMBOL OPEN_PAR_SYMBOL in_sum_expr CLOSE_PAR_SYMBOL
    | STDDEV_SAMP_SYMBOL OPEN_PAR_SYMBOL in_sum_expr CLOSE_PAR_SYMBOL
    | VAR_SAMP_SYMBOL OPEN_PAR_SYMBOL in_sum_expr CLOSE_PAR_SYMBOL
    | SUM_SYMBOL OPEN_PAR_SYMBOL in_sum_expr CLOSE_PAR_SYMBOL
    | SUM_SYMBOL OPEN_PAR_SYMBOL DISTINCT_SYMBOL in_sum_expr CLOSE_PAR_SYMBOL
    | GROUP_CONCAT_SYMBOL OPEN_PAR_SYMBOL DISTINCT_SYMBOL? expression_list order_clause?
        (SEPARATOR_SYMBOL textString)? CLOSE_PAR_SYMBOL
;

in_sum_expr:
    ALL_SYMBOL? expr
;

ident_list_arg:
    ident_list
    | OPEN_PAR_SYMBOL ident_list CLOSE_PAR_SYMBOL
;

ident_list:
    simpleIdentifier (COMMA_SYMBOL simpleIdentifier)*
;

fulltext_options:
    IN_SYMBOL BOOLEAN_SYMBOL MODE_SYMBOL
    | (IN_SYMBOL NATURAL_SYMBOL LANGUAGE_SYMBOL MODE_SYMBOL)? (WITH_SYMBOL QUERY_SYMBOL EXPANSION_SYMBOL)?
;

runtime_function_call:
    // Function names that are keywords.
    CHAR_SYMBOL OPEN_PAR_SYMBOL expression_list (USING_SYMBOL charsetName)? CLOSE_PAR_SYMBOL
    | CURRENT_USER_SYMBOL parentheses?
    | DATE_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | DAY_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | HOUR_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | INSERT_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr COMMA_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | INTERVAL_SYMBOL OPEN_PAR_SYMBOL expr (COMMA_SYMBOL expr)+ CLOSE_PAR_SYMBOL
    | LEFT_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | MINUTE_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | MONTH_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | RIGHT_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | SECOND_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | TIME_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | TIMESTAMP_SYMBOL OPEN_PAR_SYMBOL expr (COMMA_SYMBOL expr)? CLOSE_PAR_SYMBOL
    | trim_function
    | USER_SYMBOL parentheses
    | VALUES_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | YEAR_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL

    // Function names that are not keywords.
    | (ADDDATE_SYMBOL | SUBDATE_SYMBOL) OPEN_PAR_SYMBOL expr COMMA_SYMBOL (expr | INTERVAL_SYMBOL expr interval) CLOSE_PAR_SYMBOL
    | CURDATE_SYMBOL parentheses?
    | CURTIME_SYMBOL timeFunctionParameters?
    | (DATE_ADD_SYMBOL | DATE_SUB_SYMBOL) OPEN_PAR_SYMBOL expr COMMA_SYMBOL INTERVAL_SYMBOL expr interval CLOSE_PAR_SYMBOL
    | EXTRACT_SYMBOL OPEN_PAR_SYMBOL interval FROM_SYMBOL expr CLOSE_PAR_SYMBOL
    | GET_FORMAT_SYMBOL OPEN_PAR_SYMBOL date_time_type  COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | NOW_SYMBOL timeFunctionParameters?
    | POSITION_SYMBOL OPEN_PAR_SYMBOL bit_expr IN_SYMBOL expr CLOSE_PAR_SYMBOL
    | substring_function
    | SYSDATE_SYMBOL timeFunctionParameters?
    | (TIMESTAMP_ADD_SYMBOL | TIMESTAMP_DIFF_SYMBOL) OPEN_PAR_SYMBOL interval_time_stamp COMMA_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | UTC_DATE_SYMBOL parentheses?
    | UTC_TIME_SYMBOL timeFunctionParameters?
    | UTC_TIMESTAMP_SYMBOL timeFunctionParameters?

    // Function calls with other conflicts.
    | ASCII_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | CHARSET_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | COALESCE_SYMBOL expression_list_with_parentheses
    | COLLATION_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | DATABASE_SYMBOL parentheses
    | IF_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | FORMAT_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr (COMMA_SYMBOL expr)? CLOSE_PAR_SYMBOL
    | MICROSECOND_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | MOD_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | {serverVersion < 50607}? OLD_PASSWORD_SYMBOL OPEN_PAR_SYMBOL stringLiteral CLOSE_PAR_SYMBOL
    | PASSWORD_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | QUARTER_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | REPEAT_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | REPLACE_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | REVERSE_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | ROW_COUNT_SYMBOL parentheses
    | TRUNCATE_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | WEEK_SYMBOL OPEN_PAR_SYMBOL expr (COMMA_SYMBOL expr)? CLOSE_PAR_SYMBOL
    | {serverVersion >= 50600}? WEIGHT_STRING_SYMBOL OPEN_PAR_SYMBOL expr
        (
            (AS_SYMBOL CHAR_SYMBOL fieldLength)? weight_string_levels?
            | AS_SYMBOL BINARY_SYMBOL fieldLength
            | COMMA_SYMBOL ulong_number COMMA_SYMBOL ulong_number COMMA_SYMBOL ulong_number
        )
        CLOSE_PAR_SYMBOL
    | geometry_function
;

geometry_function:
    GEOMETRYCOLLECTION_SYMBOL OPEN_PAR_SYMBOL expression_list? CLOSE_PAR_SYMBOL
    | LINESTRING_SYMBOL expression_list_with_parentheses
    | MULTILINESTRING_SYMBOL expression_list_with_parentheses
    | MULTIPOINT_SYMBOL expression_list_with_parentheses
    | MULTIPOLYGON_SYMBOL expression_list_with_parentheses
    | POINT_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | POLYGON_SYMBOL expression_list_with_parentheses
    | {serverVersion < 50706}? CONTAINS_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
;

timeFunctionParameters:
    OPEN_PAR_SYMBOL fractionalPrecision? CLOSE_PAR_SYMBOL
;

fractionalPrecision:
    {serverVersion >= 50604}? INT_NUMBER
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
        (ASC_SYMBOL | DESC_SYMBOL) REVERSE_SYMBOL?
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
        expr (FROM_SYMBOL expr)?
        | LEADING_SYMBOL expr? FROM_SYMBOL expr
        | TRAILING_SYMBOL expr? FROM_SYMBOL expr
        | BOTH_SYMBOL expr? FROM_SYMBOL expr
    )
    CLOSE_PAR_SYMBOL
;

substring_function:
    SUBSTRING_SYMBOL OPEN_PAR_SYMBOL expr
    (
        COMMA_SYMBOL expr (COMMA_SYMBOL expr)?
        | FROM_SYMBOL expr (FOR_SYMBOL expr)?
    )
    CLOSE_PAR_SYMBOL
;

function_call:
    pureIdentifier OPEN_PAR_SYMBOL aliased_expression_list? CLOSE_PAR_SYMBOL // For both UDF + other functions.
    | qualifiedIdentifier OPEN_PAR_SYMBOL expression_list? CLOSE_PAR_SYMBOL // Other functions only.
;

aliased_expression_list:
    aliasedExpression (COMMA_SYMBOL aliasedExpression)*
;

aliasedExpression:
    expr select_alias?
;

variable:
    user_variable (ASSIGN_OPERATOR expr)?
    | system_variable
;

user_variable:
    (AT_SIGN_SYMBOL textOrIdentifier)
    | AT_TEXT_SUFFIX
;

// System variables as used in exprs. SET has another variant of this (SET GLOBAL/LOCAL varname).
system_variable:
    AT_AT_SIGN_SYMBOL (option_type DOT_SYMBOL)? variable_name
;

variable_name:
    identifier dotIdentifier?    // Check in semantic phase that the first id is not global/local/session/default.
    | DEFAULT_SYMBOL dotIdentifier
;

whenExpression:
    WHEN_SYMBOL expr
;

thenExpression:
    THEN_SYMBOL expr
;

elseExpression:
   ELSE_SYMBOL expr
;

cast_type:
    BINARY_SYMBOL fieldLength?
    | CHAR_SYMBOL fieldLength? encoding?
    | NCHAR_SYMBOL fieldLength?
    | SIGNED_SYMBOL INT_SYMBOL?
    | UNSIGNED_SYMBOL INT_SYMBOL?
    | DATE_SYMBOL
    | TIME_SYMBOL typeDatetimePrecision?
    | DATETIME_SYMBOL typeDatetimePrecision?
    | DECIMAL_SYMBOL floatOptions?
    | {serverVersion >= 50708}? JSON_SYMBOL
;

encoding:
    ASCII_SYMBOL BINARY_SYMBOL?
    | BINARY_SYMBOL ((ASCII_SYMBOL | UNICODE_SYMBOL) | charset charsetName)?
    | UNICODE_SYMBOL BINARY_SYMBOL?
    | BYTE_SYMBOL
    | charset charsetName BINARY_SYMBOL?
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

xor_rule:
    XOR_SYMBOL
    | BITWISE_XOR_OPERATOR
;

// None of the microsecond variants can be used in schedules (e.g. events).
interval:
    interval_time_stamp
    | (
        SECOND_MICROSECOND_SYMBOL
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
      )
;

// Support for SQL_TSI_* units is added by mapping those to tokens without SQL_TSI_ prefix.
interval_time_stamp:
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

expression_list:
    expr (COMMA_SYMBOL expr)*
;

order_list:
    expr direction? (COMMA_SYMBOL expr direction?)*
;

channel:
    {serverVersion >= 50706}? FOR_SYMBOL CHANNEL_SYMBOL textString_no_linebreak
;

//----------------- Stored program rules -----------------------------------------------------------

// Compound syntax for stored procedures, stored functions, triggers and events.
// Both sp_proc_stmt and ev_sql_stmt_inner in the server grammar.
compoundStatement:
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
    RETURN_SYMBOL expr
;

if_statement:
    IF_SYMBOL if_body END_SYMBOL IF_SYMBOL
;

if_body:
    expr thenStatement (ELSEIF_SYMBOL if_body | ELSE_SYMBOL compoundStatement_list)?
;

thenStatement:
    THEN_SYMBOL compoundStatement_list
;

compoundStatement_list:
    (compoundStatement SEMICOLON_SYMBOL)+
;

case_statement:
    CASE_SYMBOL expr? (whenExpression thenStatement)+ else_statement? END_SYMBOL CASE_SYMBOL
;

else_statement:
    ELSE_SYMBOL compoundStatement_list
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
    pureIdentifier | keyword_sp
;

begin_end_block:
    BEGIN_SYMBOL sp_declarations? compoundStatement_list? END_SYMBOL
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
    LOOP_SYMBOL compoundStatement_list END_SYMBOL LOOP_SYMBOL
;

while_do_block:
    WHILE_SYMBOL expr DO_SYMBOL compoundStatement_list END_SYMBOL WHILE_SYMBOL
;

repeat_until_block:
    REPEAT_SYMBOL compoundStatement_list UNTIL_SYMBOL expr END_SYMBOL REPEAT_SYMBOL
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
    identifierList dataType (COLLATE_SYMBOL collationNameOrDefault)? (DEFAULT_SYMBOL expr)?
;

condition_declaration:
    identifier CONDITION_SYMBOL FOR_SYMBOL sp_condition
;

sp_condition:
    ulong_number
    | sqlstate
;

sqlstate:
    SQLSTATE_SYMBOL VALUE_SYMBOL? stringLiteral
;

handler_declaration:
    (CONTINUE_SYMBOL | EXIT_SYMBOL | UNDO_SYMBOL) HANDLER_SYMBOL
        FOR_SYMBOL handler_condition (COMMA_SYMBOL handler_condition)* compoundStatement
;

handler_condition:
    sp_condition
    | identifier
    | SQLWARNING_SYMBOL
    | not_rule FOUND_SYMBOL
    | SQLEXCEPTION_SYMBOL
;

cursor_declaration:
    identifier CURSOR_SYMBOL FOR_SYMBOL selectStatement
;

iterate_statement:
    ITERATE_SYMBOL label_identifier
;

leave_statement:
    LEAVE_SYMBOL label_identifier
;

getDiagnostics:
    GET_SYMBOL
    (
        CURRENT_SYMBOL
        | {serverVersion >= 50700}? STACKED_SYMBOL
    )? DIAGNOSTICS_SYMBOL
    (
        statement_information_item (COMMA_SYMBOL statement_information_item)*
        | CONDITION_SYMBOL signal_allowed_expr condition_information_item (COMMA_SYMBOL condition_information_item)*
    )
;

// Only a limited subset of expr is allowed in SIGNAL/RESIGNAL/CONDITIONS.
signal_allowed_expr:
    literal
    | variable
    | qualifiedIdentifier
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

signalStatement:
    SIGNAL_SYMBOL (identifier | sqlstate) (SET_SYMBOL signal_information_item (COMMA_SYMBOL signal_information_item)*)?
;

resignalStatement:
    RESIGNAL_SYMBOL (SQLSTATE_SYMBOL VALUE_SYMBOL? textOrIdentifier)?
        (SET_SYMBOL signal_information_item (COMMA_SYMBOL signal_information_item)*)?
;

signal_information_item:
    signal_information_item_name EQUAL_OPERATOR signal_allowed_expr
;

cursor_open:
    OPEN_SYMBOL identifier
;

cursor_close:
    CLOSE_SYMBOL identifier
;

cursor_fetch:
    FETCH_SYMBOL identifier INTO_SYMBOL identifierList
;

//----------------- Supplemental rules -------------------------------------------------------------

// Schedules in CREATE/ALTER EVENT.
schedule:
    AT_SYMBOL expr
    | EVERY_SYMBOL expr interval (STARTS_SYMBOL expr)? (ENDS_SYMBOL expr)?
;

databaseOption:
    DEFAULT_SYMBOL?
        (
            charset EQUAL_OPERATOR? charsetNameOrDefault
            | COLLATE_SYMBOL EQUAL_OPERATOR? collationNameOrDefault
        )
;

columnDefinition:
    fieldSpec (references | (CHECK_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL)?)
;

fieldSpec:
    fieldIdentifier dataType fieldDefinition
;

fieldDefinition:
    attribute*
    | {serverVersion >= 50707}? (COLLATE_SYMBOL collationName)? (GENERATED_SYMBOL ALWAYS_SYMBOL)? AS_SYMBOL
        OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL (VIRTUAL_SYMBOL | STORED_SYMBOL)? gcol_attribute*
;

attribute:
    NOT_SYMBOL? nullLiteral
    | value = DEFAULT_SYMBOL (signedLiteral | NOW_SYMBOL timeFunctionParameters?)
    | value = ON_SYMBOL UPDATE_SYMBOL NOW_SYMBOL timeFunctionParameters?
    | value = AUTO_INCREMENT_SYMBOL
    | value = SERIAL_SYMBOL DEFAULT_SYMBOL VALUE_SYMBOL
    | value = UNIQUE_SYMBOL KEY_SYMBOL?
    | PRIMARY_SYMBOL? value = KEY_SYMBOL
    | value = COMMENT_SYMBOL stringLiteral
    | value = COLLATE_SYMBOL collationName
    | value = COLUMN_FORMAT_SYMBOL (FIXED_SYMBOL | DYNAMIC_SYMBOL | DEFAULT_SYMBOL)
    | value = STORAGE_SYMBOL (DISK_SYMBOL | MEMORY_SYMBOL | DEFAULT_SYMBOL)
;

gcol_attribute:
    UNIQUE_SYMBOL KEY_SYMBOL?
    | COMMENT_SYMBOL textString
    | not_rule? NULL_SYMBOL
    | PRIMARY_SYMBOL? KEY_SYMBOL
;

/* Internal to server.
parse_gcol_expr:
    PARSE_GCOL_expr_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
;
*/

references:
    REFERENCES_SYMBOL tableRef identifierListWithParentheses? (MATCH_SYMBOL match = (FULL_SYMBOL | PARTIAL_SYMBOL | SIMPLE_SYMBOL))?
        (
            ON_SYMBOL option = UPDATE_SYMBOL deleteOption (ON_SYMBOL DELETE_SYMBOL deleteOption)?
            | ON_SYMBOL option = DELETE_SYMBOL deleteOption (ON_SYMBOL UPDATE_SYMBOL deleteOption)?
        )?
;

deleteOption:
    (RESTRICT_SYMBOL | CASCADE_SYMBOL) | SET_SYMBOL nullLiteral | NO_SYMBOL ACTION_SYMBOL
;

keyList:
    OPEN_PAR_SYMBOL keyPart (COMMA_SYMBOL keyPart)* CLOSE_PAR_SYMBOL
;

keyPart:
    identifier fieldLength? direction?
;

keyAlgorithm:
    (USING_SYMBOL | TYPE_SYMBOL) algorithm = (BTREE_SYMBOL | RTREE_SYMBOL | HASH_SYMBOL)
;

normalKeyOption:
    keyAlgorithm
    | allKeyOption
;

fulltextKeyOption:
    WITH_SYMBOL PARSER_SYMBOL identifier
    | allKeyOption
;

spatialKeyOption:
    allKeyOption
;

allKeyOption:
    KEY_BLOCK_SIZE_SYMBOL EQUAL_OPERATOR? ulong_number
    | {serverVersion >= 50600}? COMMENT_SYMBOL stringLiteral
;

dataTypeDefinition: // For external use only. Don't reference this in the normal grammar.
    dataType EOF
;

dataType:
    type = (INT_SYMBOL | TINYINT_SYMBOL | SMALLINT_SYMBOL | MEDIUMINT_SYMBOL | BIGINT_SYMBOL) fieldLength? fieldOptions?

    | (type = REAL_SYMBOL | type = DOUBLE_SYMBOL PRECISION_SYMBOL?) precision? fieldOptions?
    | type = FLOAT_SYMBOL floatOptions? fieldOptions?

    | type = BIT_SYMBOL fieldLength?
    | type = (BOOL_SYMBOL | BOOLEAN_SYMBOL)

    | type = CHAR_SYMBOL fieldLength? stringBinary?
    | (type = NCHAR_SYMBOL | type = NATIONAL_SYMBOL CHAR_SYMBOL) fieldLength? BINARY_SYMBOL?
    | type = BINARY_SYMBOL fieldLength?
    | (type = CHAR_SYMBOL VARYING_SYMBOL | type = VARCHAR_SYMBOL) fieldLength stringBinary?
    | (
        type = NATIONAL_SYMBOL VARCHAR_SYMBOL
        | type = NVARCHAR_SYMBOL
        | type = NCHAR_SYMBOL VARCHAR_SYMBOL
        | type = NATIONAL_SYMBOL CHAR_SYMBOL VARYING_SYMBOL
        | type = NCHAR_SYMBOL VARYING_SYMBOL
    ) fieldLength BINARY_SYMBOL?

    | type = VARBINARY_SYMBOL fieldLength

    | type = YEAR_SYMBOL fieldLength? fieldOptions?
    | type = DATE_SYMBOL
    | type = TIME_SYMBOL typeDatetimePrecision?
    | type = TIMESTAMP_SYMBOL typeDatetimePrecision?
    | type = DATETIME_SYMBOL typeDatetimePrecision?

    | type = TINYBLOB_SYMBOL
    | type = BLOB_SYMBOL fieldLength?
    | type = (MEDIUMBLOB_SYMBOL | LONGBLOB_SYMBOL)
    | type = LONG_SYMBOL VARBINARY_SYMBOL
    | type = LONG_SYMBOL (CHAR_SYMBOL VARYING_SYMBOL | VARCHAR_SYMBOL)? stringBinary?

    | type = TINYTEXT_SYMBOL stringBinary?
    | type = TEXT_SYMBOL fieldLength? stringBinary?
    | type = MEDIUMTEXT_SYMBOL stringBinary?
    | type = LONGTEXT_SYMBOL stringBinary?

    | type = DECIMAL_SYMBOL floatOptions? fieldOptions?
    | type = NUMERIC_SYMBOL floatOptions? fieldOptions?
    | type = FIXED_SYMBOL floatOptions? fieldOptions?

    | type = ENUM_SYMBOL stringList stringBinary?
    | type = SET_SYMBOL stringList stringBinary?
    | type = SERIAL_SYMBOL
    | {serverVersion >= 50707}? type = JSON_SYMBOL
    | type = (
        GEOMETRY_SYMBOL
        | GEOMETRYCOLLECTION_SYMBOL
        | POINT_SYMBOL
        | MULTIPOINT_SYMBOL
        | LINESTRING_SYMBOL
        | MULTILINESTRING_SYMBOL
        | POLYGON_SYMBOL
        | MULTIPOLYGON_SYMBOL
    )
;

fieldLength:
    OPEN_PAR_SYMBOL (real_ulonglong_number | DECIMAL_NUMBER) CLOSE_PAR_SYMBOL
;

fieldOptions:
    (SIGNED_SYMBOL | UNSIGNED_SYMBOL | ZEROFILL_SYMBOL)+
;

stringBinary:
    ascii
    | unicode
    | BYTE_SYMBOL
    | charset charsetName BINARY_SYMBOL?
    | BINARY_SYMBOL (charset charsetName)?
;

ascii:
    ASCII_SYMBOL BINARY_SYMBOL?
    | {serverVersion >= 50500}? BINARY_SYMBOL ASCII_SYMBOL
;

unicode:
    UNICODE_SYMBOL BINARY_SYMBOL?
    | {serverVersion >= 50500}? BINARY_SYMBOL UNICODE_SYMBOL
;

typeDatetimePrecision:
    {serverVersion >= 50600}? OPEN_PAR_SYMBOL INT_NUMBER CLOSE_PAR_SYMBOL
;

charsetName:
    textOrIdentifier
    | BINARY_SYMBOL
;

charsetNameOrDefault:
    charsetName
    | DEFAULT_SYMBOL
;

collationName:
    textOrIdentifier
;

collationNameOrDefault:
    collationName
    | DEFAULT_SYMBOL
;

createTableOptions:
    createTableOption (COMMA_SYMBOL? createTableOption)*
;

createTableOption: // In the order as they appear in the server grammar.
    option = ENGINE_SYMBOL EQUAL_OPERATOR? engineRef
    | {serverVersion < 50500}? option = TYPE_SYMBOL EQUAL_OPERATOR? engineRef
    | option = MAX_ROWS_SYMBOL EQUAL_OPERATOR? ulonglong_number
    | option = MIN_ROWS_SYMBOL EQUAL_OPERATOR? ulonglong_number
    | option = AVG_ROW_LENGTH_SYMBOL EQUAL_OPERATOR? ulong_number
    | option = PASSWORD_SYMBOL EQUAL_OPERATOR? textString
    | option = COMMENT_SYMBOL EQUAL_OPERATOR? textString
    | {serverVersion >= 50708}? option = COMPRESSION_SYMBOL EQUAL_OPERATOR? textString
    | {serverVersion >= 50700}? option = ENCRYPTION_SYMBOL EQUAL_OPERATOR? textString
    | option = AUTO_INCREMENT_SYMBOL EQUAL_OPERATOR? ulonglong_number
    | option = PACK_KEYS_SYMBOL EQUAL_OPERATOR? (ulong_number | DEFAULT_SYMBOL)
    | {serverVersion >= 50600}? option = (STATS_AUTO_RECALC_SYMBOL | STATS_PERSISTENT_SYMBOL | STATS_SAMPLE_PAGES_SYMBOL)
        EQUAL_OPERATOR? (ulong_number | DEFAULT_SYMBOL)
    | option = (CHECKSUM_SYMBOL | TABLE_CHECKSUM_SYMBOL) EQUAL_OPERATOR? ulong_number
    | option = DELAY_KEY_WRITE_SYMBOL EQUAL_OPERATOR? ulong_number
    | option = ROW_FORMAT_SYMBOL EQUAL_OPERATOR?
        format = (DEFAULT_SYMBOL | DYNAMIC_SYMBOL | FIXED_SYMBOL | COMPRESSED_SYMBOL | REDUNDANT_SYMBOL | COMPACT_SYMBOL)
    | option = UNION_SYMBOL EQUAL_OPERATOR? OPEN_PAR_SYMBOL tableRefList CLOSE_PAR_SYMBOL
    | DEFAULT_SYMBOL?
        (
            COLLATE_SYMBOL EQUAL_OPERATOR? collationNameOrDefault
            | charset EQUAL_OPERATOR? charsetNameOrDefault
        )

    | option = INSERT_METHOD_SYMBOL EQUAL_OPERATOR? method = (NO_SYMBOL | FIRST_SYMBOL | LAST_SYMBOL)
    | option = DATA_SYMBOL DIRECTORY_SYMBOL EQUAL_OPERATOR? textString
    | option = INDEX_SYMBOL DIRECTORY_SYMBOL EQUAL_OPERATOR? textString
    | option = TABLESPACE_SYMBOL ({serverVersion >= 50707}? EQUAL_OPERATOR? | /* empty */ ) identifier
    | option = STORAGE_SYMBOL (DISK_SYMBOL | MEMORY_SYMBOL)
    | option = CONNECTION_SYMBOL EQUAL_OPERATOR? textString
    | option = KEY_BLOCK_SIZE_SYMBOL EQUAL_OPERATOR? ulong_number
;

// Partition rules for CREATE/ALTER TABLE.
partitioning:
    {serverVersion >= 50100}? PARTITION_SYMBOL partition
;

partition:
    BY_SYMBOL partitionTypeDef (PARTITIONS_SYMBOL real_ulong_number)? subPartitions? partitionDefinitions?
;

partitionTypeDef:
    LINEAR_SYMBOL? KEY_SYMBOL partitionKeyAlgorithm? OPEN_PAR_SYMBOL identifierList? CLOSE_PAR_SYMBOL   # partitionDefKey
    | LINEAR_SYMBOL? HASH_SYMBOL OPEN_PAR_SYMBOL bit_expr CLOSE_PAR_SYMBOL                              # partitionDefHash
    | (RANGE_SYMBOL | LIST_SYMBOL)
        (
            OPEN_PAR_SYMBOL bit_expr CLOSE_PAR_SYMBOL
            | {serverVersion >= 50500}? COLUMNS_SYMBOL OPEN_PAR_SYMBOL identifierList? CLOSE_PAR_SYMBOL
        )                                                                                               # partitionDefRangeList
;

subPartitions:
    SUBPARTITION_SYMBOL BY_SYMBOL LINEAR_SYMBOL?
    (
        HASH_SYMBOL OPEN_PAR_SYMBOL bit_expr CLOSE_PAR_SYMBOL
        | KEY_SYMBOL partitionKeyAlgorithm? identifierListWithParentheses
    )
    (SUBPARTITIONS_SYMBOL real_ulong_number)?
;

partitionKeyAlgorithm: // Actually only 1 and 2 are allowed. Needs a semantic check.
    {serverVersion >= 50700}? ALGORITHM_SYMBOL EQUAL_OPERATOR real_ulong_number
;

partitionDefinitions:
    OPEN_PAR_SYMBOL partitionDefinition (COMMA_SYMBOL partitionDefinition)* CLOSE_PAR_SYMBOL
;

partitionDefinition:
    PARTITION_SYMBOL identifier
    (
        VALUES_SYMBOL
        (
            LESS_SYMBOL THAN_SYMBOL (partitionValueList | MAXVALUE_SYMBOL)
            | IN_SYMBOL partitionValueList
        )
    )?
    partitionOption*
    (OPEN_PAR_SYMBOL subpartitionDefinition (COMMA_SYMBOL subpartitionDefinition)* CLOSE_PAR_SYMBOL)?
;

partitionOption:
    option = TABLESPACE_SYMBOL EQUAL_OPERATOR? identifier
    | option = STORAGE_SYMBOL? ENGINE_SYMBOL EQUAL_OPERATOR? engineRef
    | option = NODEGROUP_SYMBOL EQUAL_OPERATOR? real_ulong_number
    | option = (MAX_ROWS_SYMBOL | MIN_ROWS_SYMBOL) EQUAL_OPERATOR? real_ulong_number
    | option = (DATA_SYMBOL | INDEX_SYMBOL) DIRECTORY_SYMBOL EQUAL_OPERATOR? stringLiteral
    | option = COMMENT_SYMBOL EQUAL_OPERATOR? stringLiteral
;

subpartitionDefinition:
    SUBPARTITION_SYMBOL identifier partitionOption*
;

partitionValueList:
    OPEN_PAR_SYMBOL partitionValue (COMMA_SYMBOL partitionValue)* CLOSE_PAR_SYMBOL
;

partitionValue:
    expr | MAXVALUE_SYMBOL
;

definerClause:
    DEFINER_SYMBOL EQUAL_OPERATOR user
;

ifExists:
    IF_SYMBOL EXISTS_SYMBOL
;

ifNotExists:
    IF_SYMBOL not_rule EXISTS_SYMBOL
;

procedureParameter:
    type = (IN_SYMBOL | OUT_SYMBOL | INOUT_SYMBOL)? functionParameter
;

functionParameter:
    identifier typeWithOptCollate
;

typeWithOptCollate:
    dataType (COLLATE_SYMBOL collationNameOrDefault)?
;

schema_identifier_pair:
    OPEN_PAR_SYMBOL schemaRef COMMA_SYMBOL schemaRef CLOSE_PAR_SYMBOL
;

viewRefList:
    viewRef (COMMA_SYMBOL viewRef)*
;

field_name_list:
    columnRef (COMMA_SYMBOL columnRef)*
;

column_assignment_list_with_default:
    column_assignment_with_default (COMMA_SYMBOL column_assignment_with_default)*
;

column_assignment_with_default:
    columnRef EQUAL_OPERATOR (expr | DEFAULT_SYMBOL)
;

charset_clause:
    charset (textOrIdentifier | (DEFAULT_SYMBOL | BINARY_SYMBOL))
;

fields_clause:
    COLUMNS_SYMBOL field_term+
;

field_term:
    TERMINATED_SYMBOL BY_SYMBOL textString
    | OPTIONALLY_SYMBOL? ENCLOSED_SYMBOL BY_SYMBOL textString
    | ESCAPED_SYMBOL BY_SYMBOL textString
;

lines_clause:
    LINES_SYMBOL line_term+
;

line_term:
    (TERMINATED_SYMBOL | STARTING_SYMBOL) BY_SYMBOL textString
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
            BY_SYMBOL PASSWORD_SYMBOL? textString
            | {serverVersion >= 50600}? WITH_SYMBOL textOrIdentifier (
                (AS_SYMBOL | {serverVersion >= 50706}? BY_SYMBOL) textString)?
        )
    )?
;

user:
    textOrIdentifier (AT_SIGN_SYMBOL textOrIdentifier | AT_TEXT_SUFFIX)?
    | CURRENT_USER_SYMBOL parentheses?
;

like_clause:
    LIKE_SYMBOL textString
;

like_or_where:
    like_clause | where_clause
;

onlineOption:
    {serverVersion < 50600}? (ONLINE_SYMBOL | OFFLINE_SYMBOL)
;

noWriteToBinLog:
    LOCAL_SYMBOL // Predicate needed to direct the parser (as LOCAL can also be an identifier).
    | NO_WRITE_TO_BINLOG_SYMBOL
;

use_partition:
    {serverVersion >= 50602}? PARTITION_SYMBOL identifierListWithParentheses
;

//----------------- Object names and references ----------------------------------------------------

// For each object we have at least 2 rules here:
// 1) The name when creating that object.
// 2) The name when used to reference it from other rules.
//
// Sometimes we need additional reference rules with different form, depending on the place such a reference is used.

// A name for a field (column). Can be qualified with the current schema + table (although it's not a reference).
fieldIdentifier:
    dotIdentifier
    | qualifiedIdentifier dotIdentifier?
;

// Same as a field identifier, but with own name to indicate it can only be a column from the table this is used in
// (not a column reference to a different table).
columnInternalRef:
    fieldIdentifier
;

columnRef: // A field identifier that can reference any schema/table.
    fieldIdentifier
;

columnRefWithWildcard:
    identifier
    (
        DOT_SYMBOL MULT_OPERATOR
        | dotIdentifier
        (
            DOT_SYMBOL MULT_OPERATOR
            | dotIdentifier
        )?
    )?
;

indexName:
    identifier
;

indexRef:
    identifier
;

tableWild:
    identifier
    (
        DOT_SYMBOL MULT_OPERATOR
        | dotIdentifier DOT_SYMBOL MULT_OPERATOR
    )
;

schemaName:
    identifier
;

schemaRef:
    identifier
;

procedureName:
    qualifiedIdentifier
;

procedureRef:
    qualifiedIdentifier
;

functionName:
    qualifiedIdentifier
;

functionRef:
    qualifiedIdentifier
;

triggerName:
    qualifiedIdentifier
;

triggerRef:
    qualifiedIdentifier
;

viewName:
    tableNameVariants
;

viewRef:
    qualifiedIdentifier
;

tablespaceName:
    identifier
;

tablespaceRef:
    identifier
;

logfileGroupName:
    identifier
;

logfileGroupRef:
    identifier
;

eventName:
    qualifiedIdentifier
;

eventRef:
    qualifiedIdentifier
;

udfName: // UDFs are referenced at the same places as any other function. So, no dedicated *_ref here.
    identifier
;

serverName:
    textOrIdentifier
;

serverRef:
    textOrIdentifier
;

engineRef:
    textOrIdentifier
;

tableName:
    tableNameVariants
;

filterTableRef: // Always qualified.
    identifier dotIdentifier
;

tableRefWithWildcard:
    identifier
    (
        DOT_SYMBOL MULT_OPERATOR
        | dotIdentifier (DOT_SYMBOL MULT_OPERATOR)?
    )?
;

tableRef:
    tableNameVariants
;

tableRefNoDb:
    identifier
;

tableNameVariants:
    qualifiedIdentifier
    | dotIdentifier
;

tableRefList:
    tableRef (COMMA_SYMBOL tableRef)*
;

tableRefListWithWildcard:
    tableRefWithWildcard (COMMA_SYMBOL tableRefWithWildcard)*
;

//----------------- Common basic rules -------------------------------------------------------------

// Identifiers excluding keywords (except if they are quoted).
pureIdentifier:
    (IDENTIFIER | BACK_TICK_QUOTED_ID)
    | DOUBLE_QUOTED_TEXT {isSqlModeActive(AnsiQuotes)}?
;

// Identifiers including a certain set of keywords, which are allowed also if not quoted.
// ident in sql_yacc.yy
identifier:
    pureIdentifier
    | keyword
;

identifierList:
    identifier (COMMA_SYMBOL identifier)*
;

identifierListWithParentheses:
    OPEN_PAR_SYMBOL identifierList CLOSE_PAR_SYMBOL
;

qualifiedIdentifier:
    identifier dotIdentifier?
;

simpleIdentifier: // simple_ident + simple_ident_q
    identifier (dotIdentifier dotIdentifier?)?
    | dotIdentifier dotIdentifier
;

// This rule mimics the behavior of the server parser to allow any identifier, including any keyword,
// unquoted when directly preceded by a dot in a qualified identifier.
dotIdentifier:
    DOT_SYMBOL identifier // Dot and kw separated. Not all keywords are allowed then.
    | DOT_IDENTIFIER
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

stringList:
    OPEN_PAR_SYMBOL textString (COMMA_SYMBOL textString)* CLOSE_PAR_SYMBOL
;

textString:
    SINGLE_QUOTED_TEXT | HEX_NUMBER | BIN_NUMBER
;

// A special variant of a text string that must not contain a linebreak (TEXTSTRING_sys_nonewline in sql_yacc.yy).
// Check validity in semantic phase.
textString_no_linebreak:
    SINGLE_QUOTED_TEXT {!containsLinebreak($SINGLE_QUOTED_TEXT.text)}?
;

literal:
    stringLiteral
    | numLiteral
    | temporalLiteral
    | nullLiteral
    | boolLiteral
    | UNDERSCORE_CHARSET? HEX_NUMBER
    | {serverVersion >= 50003}? UNDERSCORE_CHARSET? BIN_NUMBER
;

signedLiteral:
    literal
    | PLUS_OPERATOR ulong_number
    | MINUS_OPERATOR ulong_number
;

stringLiteral:
    NCHAR_TEXT
    | UNDERSCORE_CHARSET? (textList += SINGLE_QUOTED_TEXT | {!isSqlModeActive(AnsiQuotes)}? textList += DOUBLE_QUOTED_TEXT)+
;

numLiteral:
    INT_NUMBER
    | LONG_NUMBER
    | ULONGLONG_NUMBER
    | DECIMAL_NUMBER
    | FLOAT_NUMBER
;

boolLiteral:
    TRUE_SYMBOL
    | FALSE_SYMBOL
;

nullLiteral: // In sql_yacc.cc both 'NULL' and '\N' are mapped to NULL_SYM (which is our nullLiteral).
    NULL_SYMBOL
    | NULL2_SYMBOL
;

temporalLiteral:
    DATE_SYMBOL SINGLE_QUOTED_TEXT
    | TIME_SYMBOL SINGLE_QUOTED_TEXT
    | TIMESTAMP_SYMBOL SINGLE_QUOTED_TEXT
;

floatOptions:
    fieldLength | precision
;

precision:
    OPEN_PAR_SYMBOL INT_NUMBER COMMA_SYMBOL INT_NUMBER CLOSE_PAR_SYMBOL
;

textOrIdentifier:
    SINGLE_QUOTED_TEXT
    | identifier
    //| AT_TEXT_SUFFIX // LEX_HOSTNAME in the server grammar. Handled differently.
;

sizeNumber:
    real_ulonglong_number
    | pureIdentifier // Something like 10G. Semantic check needed for validity.
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
    | (
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
    | {serverVersion >= 50709}? SHUTDOWN_SYMBOL // Moved here from keyword_sp in version 5.7.9 .
;

// Comment from server yacc grammar:
//   Keywords that we allow for labels in SPs. Anything that's the beginning of a statement
//   or characteristics must be in keyword above, otherwise we get (harmful) shift/reduce conflicts.
// Additionally:
//   The keywords are only roughly sorted to stay with the same order as in sql_yacc.yy (for simpler diff'ing).
keyword_sp:
    (
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
        | ENCRYPTION_SYMBOL  // Conditionally set in the lexer.
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
        | INSTANCE_SYMBOL // Conditionally deprecated in the lexer.
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
        | MASTER_TLS_VERSION_SYMBOL // Conditionally deprecated in the lexer.
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
        | ROTATE_SYMBOL // Conditionally deprecated in the lexer.
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
    )
    | {serverVersion < 50709}? SHUTDOWN_SYMBOL // Moved to keyword rule in version 5.7.9.
;
