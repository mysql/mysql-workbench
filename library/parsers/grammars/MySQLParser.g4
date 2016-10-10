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
    ((statement | beginWork) SEMICOLON_SYMBOL?)? EOF
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
        (COMMENT_SYMBOL textLiteral)?
        (DO_SYMBOL compoundStatement)?
;

alterLogfileGroup:
    LOGFILE_SYMBOL GROUP_SYMBOL logfileGroupRef ADD_SYMBOL UNDOFILE_SYMBOL textLiteral
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
        (ADD_SYMBOL | DROP_SYMBOL) DATAFILE_SYMBOL textLiteral (alterTablespaceOption (COMMA_SYMBOL? alterTablespaceOption)*)?
        // The alternatives listed below are not documented but appear in the server grammar file.
        | CHANGE_SYMBOL DATAFILE_SYMBOL textLiteral (changeTablespaceOption (COMMA_SYMBOL? changeTablespaceOption)*)?
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

changeTablespaceOption:
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
        (COMMENT_SYMBOL textLiteral)?
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
      type = (STRING_SYMBOL | INT_SYMBOL | REAL_SYMBOL | DECIMAL_SYMBOL) SONAME_SYMBOL textLiteral
;

routineCreateOption:
    routineOption
    | NOT_SYMBOL? DETERMINISTIC_SYMBOL
;

routineAlterOptions:
    routineCreateOption+
;

routineOption:
    option = COMMENT_SYMBOL textLiteral
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
        ADD_SYMBOL (UNDOFILE_SYMBOL | REDOFILE_SYMBOL) textLiteral
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
    | option = COMMENT_SYMBOL EQUAL_OPERATOR? textLiteral
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
    option = HOST_SYMBOL textLiteral
    | option = DATABASE_SYMBOL textLiteral
    | option = USER_SYMBOL textLiteral
    | option = PASSWORD_SYMBOL textLiteral
    | option = SOCKET_SYMBOL textLiteral
    | option = OWNER_SYMBOL textLiteral
    | option = PORT_SYMBOL ulong_number
;

createTablespace:
    CREATE_SYMBOL TABLESPACE_SYMBOL tablespaceName ADD_SYMBOL DATAFILE_SYMBOL textLiteral
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
    | option = COMMENT_SYMBOL EQUAL_OPERATOR? textLiteral
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
    CALL_SYMBOL procedureRef (OPEN_PAR_SYMBOL expressionList? CLOSE_PAR_SYMBOL)?
;

deleteStatement:
    DELETE_SYMBOL deleteStatementOption*
        (
            FROM_SYMBOL
                (
                     tableRefListWithWildcard USING_SYMBOL joinTableList whereClause? // Multi table variant 1.
                    | tableRef partition_delete? whereClause? orderClause? simpleLimitClause? // Single table delete.
                )
            |  tableRefListWithWildcard FROM_SYMBOL joinTableList whereClause? // Multi table variant 2.
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
        {serverVersion < 50709}? expressionList
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
                | READ_SYMBOL handlerReadOrScan whereClause? limitClause?
            )
    )
;

handlerReadOrScan:
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
    INSERT_SYMBOL insertLockOption? IGNORE_SYMBOL? INTO_SYMBOL? tableRef usePartition?
        insertFieldSpec duplicateKeyUpdate?
;

insertLockOption:
    LOW_PRIORITY_SYMBOL
    | DELAYED_SYMBOL        // Only allowed if no select is used. Check in the semantic phase.
    | HIGH_PRIORITY_SYMBOL
;

insertFieldSpec:
    (OPEN_PAR_SYMBOL fields? CLOSE_PAR_SYMBOL)?
        (
            insertValues
            | insertQueryExpression
        )
    | SET_SYMBOL columnAssignmentListWithDefault
;

fields:
    columnRefWithWildcard (COMMA_SYMBOL columnRefWithWildcard)*
;

insertValues:
    (VALUES_SYMBOL | VALUE_SYMBOL) insertValueList
;

insertQueryExpression:
    createSelect unionClause?
    | OPEN_PAR_SYMBOL createSelect CLOSE_PAR_SYMBOL unionOpt?;

insertValueList:
    OPEN_PAR_SYMBOL values? CLOSE_PAR_SYMBOL (COMMA_SYMBOL OPEN_PAR_SYMBOL values? CLOSE_PAR_SYMBOL)*
;

values:
    (expr | DEFAULT_SYMBOL) (COMMA_SYMBOL (expr | DEFAULT_SYMBOL))*
;

duplicateKeyUpdate:
    ON_SYMBOL DUPLICATE_SYMBOL KEY_SYMBOL UPDATE_SYMBOL columnAssignmentListWithDefault
;

//--------------------------------------------------------------------------------------------------

loadStatement:
    LOAD_SYMBOL dataOrXml (LOW_PRIORITY_SYMBOL | CONCURRENT_SYMBOL)? LOCAL_SYMBOL? INFILE_SYMBOL textLiteral
        (REPLACE_SYMBOL | IGNORE_SYMBOL)? INTO_SYMBOL TABLE_SYMBOL tableRef
        usePartition? charsetClause?
        xmlRowsIdentifiedBy?
        fieldsClause? linesClause?
        loadDataFileTail
;

dataOrXml:
    DATA_SYMBOL
    | {serverVersion >= 50500}? XML_SYMBOL
;

xmlRowsIdentifiedBy:
    {serverVersion >= 50500}? ROWS_SYMBOL IDENTIFIED_SYMBOL BY_SYMBOL textString
;

loadDataFileTail:
    (IGNORE_SYMBOL INT_NUMBER (LINES_SYMBOL | ROWS_SYMBOL))? loadDataFileTargetList? (SET_SYMBOL columnAssignmentListWithDefault)?
;

loadDataFileTargetList:
    OPEN_PAR_SYMBOL fieldOrVariableList? CLOSE_PAR_SYMBOL
;

fieldOrVariableList:
    (columnRef | userVariable) (COMMA_SYMBOL (columnRef | userVariable))*
;

//--------------------------------------------------------------------------------------------------

replaceStatement:
    REPLACE_SYMBOL (LOW_PRIORITY_SYMBOL | DELAYED_SYMBOL)? INTO_SYMBOL? tableRef
        usePartition? insertFieldSpec
;

//--------------------------------------------------------------------------------------------------

selectStatement:
    SELECT_SYMBOL selectPart2 unionClause?
    | OPEN_PAR_SYMBOL selectParen CLOSE_PAR_SYMBOL unionOpt?;

selectParen:
    SELECT_SYMBOL selectPart2
    | OPEN_PAR_SYMBOL selectParen CLOSE_PAR_SYMBOL
;

selectFrom:
    fromClause whereClause? groupByClause? havingClause? orderClause? limitClause? procedureAnalyseClause?
;

selectPart2:
    selectOption* selectItemList
        (
            orderClause? limitClause?
            | intoClause selectFrom?
            | selectFrom intoClause?
        )
        selectLockType?
;

tableExpression:
    fromClause?
    whereClause?
    groupByClause?
    havingClause?
    orderClause?
    limitClause?
    procedureAnalyseClause?
    selectLockType?
;

subselect: // both subselect and query_expression_body in sql_yacc.yy.
    querySpecification (UNION_SYMBOL unionOption querySpecification)*
;

selectPart2Derived: // selectPart2 equivalent for sub queries.
    (
        querySpecOption
        | {serverVersion <= 50100}? (SQL_NO_CACHE_SYMBOL | SQL_CACHE_SYMBOL)
    )* selectItemList
;

selectOption:
    querySpecOption
    | (SQL_NO_CACHE_SYMBOL | SQL_CACHE_SYMBOL)
    | {serverVersion >= 50704 && serverVersion < 50708}? MAX_STATEMENT_TIME_SYMBOL EQUAL_OPERATOR real_ulong_number
;

querySpecOption:
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
    (selectItem | MULT_OPERATOR) (COMMA_SYMBOL selectItem)*
;

selectItem:
    tableWild
    | expr selectAlias?
;

selectAlias:
    AS_SYMBOL? (identifier | textString )
;

limitClause:
    LIMIT_SYMBOL limitOptions
;

simpleLimitClause:
    LIMIT_SYMBOL limitOption
;

limitOptions:
    limitOption ((COMMA_SYMBOL | OFFSET_SYMBOL) limitOption)?
;

limitOption:
    identifier
    | (PARAM_MARKER | ULONGLONG_NUMBER | LONG_NUMBER | INT_NUMBER)
;

intoClause:
    INTO_SYMBOL
    (
        OUTFILE_SYMBOL textLiteral charsetClause? fieldsClause? linesClause?
        | DUMPFILE_SYMBOL textLiteral
        | AT_SIGN_SYMBOL? (textOrIdentifier | AT_TEXT_SUFFIX) (COMMA_SYMBOL AT_SIGN_SYMBOL? (textOrIdentifier | AT_TEXT_SUFFIX))*
    )
;

procedureAnalyseClause:
    PROCEDURE_SYMBOL ANALYSE_SYMBOL OPEN_PAR_SYMBOL (INT_NUMBER (COMMA_SYMBOL INT_NUMBER)?)? CLOSE_PAR_SYMBOL
;

havingClause:
    HAVING_SYMBOL expr
;

groupByClause:
    GROUP_SYMBOL BY_SYMBOL orderList olapOption?
;

olapOption:
    WITH_SYMBOL CUBE_SYMBOL
    | WITH_SYMBOL ROLLUP_SYMBOL
;

orderClause:
    ORDER_SYMBOL BY_SYMBOL orderList
;

orderByOrLimit:
    orderClause limitClause?
    | limitClause
;

direction:
    ASC_SYMBOL
    | DESC_SYMBOL
;

fromClause:
    FROM_SYMBOL tablekeyList
;

whereClause:
    WHERE_SYMBOL expr
;

tablekeyList:
    joinTableList
    | DUAL_SYMBOL
;

joinTableList: // joinTableList + derived_table_list in sql_yacc.yy.
     escapedTableReference (COMMA_SYMBOL escapedTableReference)*
;

// For the ODBC OJ syntax we do as the server does. This is what the server grammar says about it:
//   The ODBC escape syntax for Outer Join is: '{' OJ joinTable '}'
//   The parser does not define OJ as a token, any ident is accepted
//   instead in $2 (ident). Also, all productions from tableRef can
//   be escaped, not only joinTable. Both syntax extensions are safe
//   and are ignored.
escapedTableReference:
    tableReference
    | OPEN_CURLY_SYMBOL identifier tableReference CLOSE_CURLY_SYMBOL
;

tableReference: // table_ref in sql_yacc.yy, we use tableRef here for a different rule.
    tableFactor joinTable*
;

tableFactor:
    SELECT_SYMBOL selectOption* selectItemList tableExpression
    | OPEN_PAR_SYMBOL selectTableFactorUnion CLOSE_PAR_SYMBOL tableAlias?
    | tableRef usePartition? tableAlias? indexHintList?
;

selectTableFactorUnion:
    (tablekeyList orderByOrLimit?) (UNION_SYMBOL unionOption? querySpecification)*
;

querySpecification:
    SELECT_SYMBOL selectPart2Derived tableExpression
    | OPEN_PAR_SYMBOL selectParenDerived CLOSE_PAR_SYMBOL orderByOrLimit?
;

selectParenDerived:
    SELECT_SYMBOL selectPart2Derived tableExpression
    | OPEN_PAR_SYMBOL selectParenDerived CLOSE_PAR_SYMBOL
;

joinTable: // join_table in sql_yacc.yy, with removed indirect left recursion + some optimizations.
    normalJoin tableReference (ON_SYMBOL expr | USING_SYMBOL identifierListWithParentheses)?
    | STRAIGHT_JOIN_SYMBOL tableFactor (ON_SYMBOL expr)?
    | NATURAL_SYMBOL ((LEFT_SYMBOL | RIGHT_SYMBOL) OUTER_SYMBOL?)? JOIN_SYMBOL tableFactor
    | (LEFT_SYMBOL | RIGHT_SYMBOL) OUTER_SYMBOL? JOIN_SYMBOL (tableReference ON_SYMBOL expr | tableFactor USING_SYMBOL identifierListWithParentheses)
;

normalJoin: JOIN_SYMBOL | INNER_SYMBOL JOIN_SYMBOL | CROSS_SYMBOL JOIN_SYMBOL;

unionClause:
  UNION_SYMBOL unionOption? selectStatement
 ;

unionOption:
    DISTINCT_SYMBOL
    | ALL_SYMBOL
;

unionOpt:
    unionClause
    | orderByOrLimit
;

selectLockType:
    FOR_SYMBOL UPDATE_SYMBOL
    | LOCK_SYMBOL IN_SYMBOL SHARE_SYMBOL MODE_SYMBOL
;

tableAlias:
    (AS_SYMBOL | EQUAL_OPERATOR)? identifier
;

indexHintList:
    indexHint (COMMA_SYMBOL indexHint)*
;

indexHint:
    indexHintType keyOrIndex indexHintClause? OPEN_PAR_SYMBOL indexList CLOSE_PAR_SYMBOL
    | USE_SYMBOL keyOrIndex indexHintClause? OPEN_PAR_SYMBOL indexList? CLOSE_PAR_SYMBOL
;

indexHintType:
    FORCE_SYMBOL
    | IGNORE_SYMBOL
;

keyOrIndex:
    KEY_SYMBOL
    | INDEX_SYMBOL
;

indexHintClause:
    FOR_SYMBOL (JOIN_SYMBOL | ORDER_SYMBOL BY_SYMBOL | GROUP_SYMBOL BY_SYMBOL)
;

indexList:
    indexListElement (COMMA_SYMBOL indexListElement)*
;

indexListElement:
    identifier
    | PRIMARY_SYMBOL
;

//--------------------------------------------------------------------------------------------------

updateStatement:
    UPDATE_SYMBOL LOW_PRIORITY_SYMBOL? IGNORE_SYMBOL? joinTableList
        SET_SYMBOL columnAssignmentListWithDefault whereClause? orderClause? simpleLimitClause?
;

//--------------------------------------------------------------------------------------------------

transactionOrLockingStatement:
    transactionStatement
    | savepointStatement
    | lockStatement
    | xaStatement
;

transactionStatement:
    START_SYMBOL TRANSACTION_SYMBOL transactionCharacteristic*
    | COMMIT_SYMBOL WORK_SYMBOL? (AND_SYMBOL NO_SYMBOL? CHAIN_SYMBOL)? (NO_SYMBOL? RELEASE_SYMBOL)?
    // SET TRANSACTION is part of setStatement.
;

// BEGIN WORK is separated from transactional statements as it must not appear as part of a stored program.
beginWork:
    BEGIN_SYMBOL WORK_SYMBOL?
;

transactionCharacteristic:
    WITH_SYMBOL CONSISTENT_SYMBOL SNAPSHOT_SYMBOL
    | {serverVersion >= 50605}? READ_SYMBOL (WRITE_SYMBOL | ONLY_SYMBOL)
;

setTransactionCharacteristic:
    ISOLATION_SYMBOL LEVEL_SYMBOL isolationLevel
    | {serverVersion >= 50605}? READ_SYMBOL (WRITE_SYMBOL | ONLY_SYMBOL)
;

isolationLevel:
    REPEATABLE_SYMBOL READ_SYMBOL
    | READ_SYMBOL (COMMITTED_SYMBOL | UNCOMMITTED_SYMBOL)
    | SERIALIZABLE_SYMBOL
;

savepointStatement:
    SAVEPOINT_SYMBOL identifier
    | ROLLBACK_SYMBOL WORK_SYMBOL?
        (
            TO_SYMBOL SAVEPOINT_SYMBOL? identifier
            | (AND_SYMBOL NO_SYMBOL? CHAIN_SYMBOL)? (NO_SYMBOL? RELEASE_SYMBOL)?
        )
    | RELEASE_SYMBOL SAVEPOINT_SYMBOL identifier
;

lockStatement:
    LOCK_SYMBOL (TABLES_SYMBOL | TABLE_SYMBOL) lockItem (COMMA_SYMBOL lockItem)*
    | UNLOCK_SYMBOL (TABLES_SYMBOL | TABLE_SYMBOL)
;

lockItem:
    tableRef tableAlias? lockOption
;

lockOption:
    READ_SYMBOL LOCAL_SYMBOL?
    | LOW_PRIORITY_SYMBOL? WRITE_SYMBOL // low priority deprecated since 5.7
;

xaStatement:
    XA_SYMBOL
        (
            (START_SYMBOL | BEGIN_SYMBOL) xid (JOIN_SYMBOL | RESUME_SYMBOL)?
            | END_SYMBOL xid (SUSPEND_SYMBOL (FOR_SYMBOL MIGRATE_SYMBOL)?)?
            | PREPARE_SYMBOL xid
            | COMMIT_SYMBOL xid (ONE_SYMBOL PHASE_SYMBOL)?
            | ROLLBACK_SYMBOL xid
            | RECOVER_SYMBOL xaConvert
        )
;

xaConvert:
    {serverVersion >= 50704}? (CONVERT_SYMBOL XID_SYMBOL)?
    | /* empty */
;

xid:
    textString (COMMA_SYMBOL textString (COMMA_SYMBOL ulong_number)?)?
;

//--------------------------------------------------------------------------------------------------

replicationStatement:
    PURGE_SYMBOL (BINARY_SYMBOL | MASTER_SYMBOL) LOGS_SYMBOL (TO_SYMBOL textLiteral | BEFORE_SYMBOL expr)
    | changeMaster
    | RESET_SYMBOL resetOption (COMMA_SYMBOL resetOption)*
    | slave
    | {serverVersion >= 50700}? changeReplication
    | {serverVersion < 50500}? replicationLoad
    | {serverVersion > 50706}? groupReplication
;

resetOption:
    option = MASTER_SYMBOL
    | option = QUERY_SYMBOL CACHE_SYMBOL
    | option = SLAVE_SYMBOL ALL_SYMBOL? channel?
;

replicationLoad:
    LOAD_SYMBOL (DATA_SYMBOL | TABLE_SYMBOL tableRef) FROM_SYMBOL MASTER_SYMBOL
;

changeMaster:
    CHANGE_SYMBOL MASTER_SYMBOL TO_SYMBOL changeMasterOptions channel?
;

changeMasterOptions:
    masterOption (COMMA_SYMBOL masterOption)*
;

masterOption:
    MASTER_HOST_SYMBOL EQUAL_OPERATOR textStringNoLinebreak
    | MASTER_BIND_SYMBOL EQUAL_OPERATOR textStringNoLinebreak
    | MASTER_USER_SYMBOL EQUAL_OPERATOR textStringNoLinebreak
    | MASTER_PASSWORD_SYMBOL EQUAL_OPERATOR textStringNoLinebreak
    | MASTER_PORT_SYMBOL EQUAL_OPERATOR ulong_number
    | MASTER_CONNECT_RETRY_SYMBOL EQUAL_OPERATOR ulong_number
    | MASTER_RETRY_COUNT_SYMBOL EQUAL_OPERATOR ulong_number
    | MASTER_DELAY_SYMBOL EQUAL_OPERATOR ulong_number
    | MASTER_SSL_SYMBOL EQUAL_OPERATOR ulong_number
    | MASTER_SSL_CA_SYMBOL EQUAL_OPERATOR textStringNoLinebreak
    | MASTER_TLS_VERSION_SYMBOL EQUAL_OPERATOR textStringNoLinebreak
    | MASTER_SSL_CAPATH_SYMBOL EQUAL_OPERATOR textStringNoLinebreak
    | MASTER_SSL_CERT_SYMBOL EQUAL_OPERATOR textStringNoLinebreak
    | MASTER_SSL_CIPHER_SYMBOL EQUAL_OPERATOR textStringNoLinebreak
    | MASTER_SSL_KEY_SYMBOL EQUAL_OPERATOR textStringNoLinebreak
    | MASTER_SSL_VERIFY_SERVER_CERT_SYMBOL EQUAL_OPERATOR ulong_number
    | MASTER_SSL_CRL_SYMBOL EQUAL_OPERATOR textLiteral
    | MASTER_SSL_CRLPATH_SYMBOL EQUAL_OPERATOR textStringNoLinebreak
    | MASTER_HEARTBEAT_PERIOD_SYMBOL EQUAL_OPERATOR ulong_number
    | IGNORE_SERVER_IDS_SYMBOL EQUAL_OPERATOR serverIdList
    | MASTER_AUTO_POSITION_SYMBOL EQUAL_OPERATOR ulong_number
    | masterFileDef
;

masterFileDef:
    MASTER_LOG_FILE_SYMBOL EQUAL_OPERATOR textStringNoLinebreak
    | MASTER_LOG_POS_SYMBOL EQUAL_OPERATOR ulonglong_number
    | RELAY_LOG_FILE_SYMBOL EQUAL_OPERATOR textStringNoLinebreak
    | RELAY_LOG_POS_SYMBOL EQUAL_OPERATOR ulong_number
;

serverIdList:
    OPEN_PAR_SYMBOL (ulong_number (COMMA_SYMBOL ulong_number)*)? CLOSE_PAR_SYMBOL
;

changeReplication:
    CHANGE_SYMBOL REPLICATION_SYMBOL FILTER_SYMBOL filterDefinition (COMMA_SYMBOL filterDefinition)*
;

filterDefinition:
    REPLICATE_DO_DB_SYMBOL EQUAL_OPERATOR OPEN_PAR_SYMBOL filterDbList? CLOSE_PAR_SYMBOL
    | REPLICATE_IGNORE_DB_SYMBOL EQUAL_OPERATOR OPEN_PAR_SYMBOL filterDbList? CLOSE_PAR_SYMBOL
    | REPLICATE_DO_TABLE_SYMBOL EQUAL_OPERATOR OPEN_PAR_SYMBOL filterTableList? CLOSE_PAR_SYMBOL
    | REPLICATE_IGNORE_TABLE_SYMBOL EQUAL_OPERATOR OPEN_PAR_SYMBOL filterTableList? CLOSE_PAR_SYMBOL
    | REPLICATE_WILD_DO_TABLE_SYMBOL EQUAL_OPERATOR OPEN_PAR_SYMBOL filterStringList? CLOSE_PAR_SYMBOL
    | REPLICATE_WILD_IGNORE_TABLE_SYMBOL EQUAL_OPERATOR OPEN_PAR_SYMBOL filterStringList? CLOSE_PAR_SYMBOL
    | REPLICATE_REWRITE_DB_SYMBOL EQUAL_OPERATOR OPEN_PAR_SYMBOL filterDbPairList? CLOSE_PAR_SYMBOL
;

filterDbList:
    schemaRef (COMMA_SYMBOL schemaRef)*
;

filterTableList:
    filterTableRef (COMMA_SYMBOL filterTableRef)*
;

filterStringList:
    filterWildDbTableString (COMMA_SYMBOL filterWildDbTableString)*
;

filterWildDbTableString:
    textStringNoLinebreak // sql_yacc.yy checks for the existance of at least one dot char in the string.
;

filterDbPairList:
    schema_identifier_pair (COMMA_SYMBOL schema_identifier_pair)*
;

slave:
    START_SYMBOL SLAVE_SYMBOL slaveThreadOptions? (UNTIL_SYMBOL slaveUntilOptions)? slaveConnectionOptions channel?
    | STOP_SYMBOL SLAVE_SYMBOL slaveThreadOptions? channel?
;

slaveUntilOptions:
    (
        masterFileDef
        | {serverVersion >= 50606}? (SQL_BEFORE_GTIDS_SYMBOL | SQL_AFTER_GTIDS_SYMBOL) EQUAL_OPERATOR textString
        | {serverVersion >= 50606}? SQL_AFTER_MTS_GAPS_SYMBOL
    )
    (COMMA_SYMBOL masterFileDef)*
;

slaveConnectionOptions:
    {serverVersion >= 50604}? (USER_SYMBOL EQUAL_OPERATOR textString)? (PASSWORD_SYMBOL EQUAL_OPERATOR textString)?
        (DEFAULT_AUTH_SYMBOL EQUAL_OPERATOR textString)? (PLUGIN_DIR_SYMBOL EQUAL_OPERATOR textString)?
    | /* empty */
;

slaveThreadOptions:
    slaveThreadOption (COMMA_SYMBOL slaveThreadOption)*
;

slaveThreadOption:
    RELAY_THREAD_SYMBOL | SQL_THREAD_SYMBOL
;

groupReplication:
    (START_SYMBOL | STOP_SYMBOL) GROUP_REPLICATION_SYMBOL
;

//--------------------------------------------------------------------------------------------------

preparedStatement:
    type = PREPARE_SYMBOL identifier FROM_SYMBOL (textLiteral | userVariable)
    | executeStatement
    | type = (DEALLOCATE_SYMBOL | DROP_SYMBOL) PREPARE_SYMBOL identifier
;

executeStatement:
    EXECUTE_SYMBOL identifier (USING_SYMBOL executeVarList)?
;

executeVarList:
    userVariable (COMMA_SYMBOL userVariable)*
;

//--------------------------------------------------------------------------------------------------

accountManagementStatement:
    {serverVersion >= 50606}? alterUser
    | createUser
    | dropUser
    | {serverVersion >= 50500}? grantProxy
    | grant
    | renameUser
    | revokeStatement
    | setPassword
;

alterUser:
    ALTER_SYMBOL USER_SYMBOL ({serverVersion >= 50706}? ifExists | /* empty */) alterUserTail
;

alterUserTail:
    grantList createUserTail
    | {serverVersion >= 50706}? USER_SYMBOL parentheses IDENTIFIED_SYMBOL BY_SYMBOL textString
;

createUser:
    CREATE_SYMBOL USER_SYMBOL ({serverVersion >= 50706}? ifNotExists | /* empty */) grantList createUserTail
;

createUserTail:
    {serverVersion >= 50706}? requireClause? connectOptions? accountLockPasswordExpireOptions?
    | /* empty */
;

requireClause:
    REQUIRE_SYMBOL (requireList | option = (SSL_SYMBOL | X509_SYMBOL | NONE_SYMBOL))
;

connectOptions:
    WITH_SYMBOL
    (
        MAX_QUERIES_PER_HOUR_SYMBOL ulong_number
        | MAX_UPDATES_PER_HOUR_SYMBOL ulong_number
        | MAX_CONNECTIONS_PER_HOUR_SYMBOL ulong_number
        | MAX_USER_CONNECTIONS_SYMBOL ulong_number
    )+
;

accountLockPasswordExpireOptions:
    ACCOUNT_SYMBOL (LOCK_SYMBOL | UNLOCK_SYMBOL)
    | PASSWORD_SYMBOL EXPIRE_SYMBOL
        (
            INTERVAL_SYMBOL real_ulong_number DAY_SYMBOL
            | (NEVER_SYMBOL | DEFAULT_SYMBOL)
        )?
;

dropUser:
    DROP_SYMBOL USER_SYMBOL ({serverVersion >= 50706}? ifExists | /* empty */) userList
;

grant:
    GRANT_SYMBOL grantPrivileges privilegeTarget
        TO_SYMBOL grantList requireClause? (WITH_SYMBOL grantOption+)?
;

grantProxy:
    GRANT_SYMBOL PROXY_SYMBOL ON_SYMBOL grantUser TO_SYMBOL grantUser (COMMA_SYMBOL grantUser)*
        (WITH_SYMBOL GRANT_SYMBOL OPTION_SYMBOL)?
;

renameUser:
    RENAME_SYMBOL USER_SYMBOL user TO_SYMBOL user (COMMA_SYMBOL user TO_SYMBOL user)*
;

revokeStatement:
    REVOKE_SYMBOL
    (
        ALL_SYMBOL PRIVILEGES_SYMBOL? COMMA_SYMBOL GRANT_SYMBOL OPTION_SYMBOL FROM_SYMBOL userList
        | grantPrivileges privilegeTarget FROM_SYMBOL userList
        | {serverVersion >= 50500}? PROXY_SYMBOL ON_SYMBOL user FROM_SYMBOL userList
    )
;

privilegeTarget:
    ON_SYMBOL grantObjectType privilegeLevel
;

setPassword:
    SET_SYMBOL PASSWORD_SYMBOL (FOR_SYMBOL user)? equal
    (
        PASSWORD_SYMBOL OPEN_PAR_SYMBOL textString CLOSE_PAR_SYMBOL
        | {serverVersion < 50706}? OLD_PASSWORD_SYMBOL OPEN_PAR_SYMBOL textString CLOSE_PAR_SYMBOL
        | textString
    )
;

grantObjectType:
    TABLE_SYMBOL?
    | (FUNCTION_SYMBOL | PROCEDURE_SYMBOL)
;

grantPrivileges:
    ALL_SYMBOL PRIVILEGES_SYMBOL?
    | privilegeType (COMMA_SYMBOL privilegeType)*
;

privilegeType:
    ALTER_SYMBOL ROUTINE_SYMBOL?
    | CREATE_SYMBOL (TEMPORARY_SYMBOL object = TABLES_SYMBOL | object = (ROUTINE_SYMBOL | TABLESPACE_SYMBOL | USER_SYMBOL | VIEW_SYMBOL))?
    | GRANT_SYMBOL OPTION_SYMBOL
    | INSERT_SYMBOL identifierListWithParentheses?
    | LOCK_SYMBOL TABLES_SYMBOL
    | REFERENCES_SYMBOL identifierListWithParentheses?
    | REPLICATION_SYMBOL object = (CLIENT_SYMBOL | SLAVE_SYMBOL)
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

privilegeLevel:
    MULT_OPERATOR (DOT_SYMBOL MULT_OPERATOR)?
    | identifier (DOT_SYMBOL MULT_OPERATOR | dotIdentifier)?
;

requireList:
    requireListElement (AND_SYMBOL? requireListElement)*
;

requireListElement:
    element = CIPHER_SYMBOL textString
    | element = ISSUER_SYMBOL textString
    | element = SUBJECT_SYMBOL textString
;

grantOption:
    option = GRANT_SYMBOL OPTION_SYMBOL
    | option = MAX_QUERIES_PER_HOUR_SYMBOL ulong_number
    | option = MAX_UPDATES_PER_HOUR_SYMBOL ulong_number
    | option = MAX_CONNECTIONS_PER_HOUR_SYMBOL ulong_number
    | option = MAX_USER_CONNECTIONS_SYMBOL ulong_number
;

//--------------------------------------------------------------------------------------------------

tableAdministrationStatement:
    type = ANALYZE_SYMBOL noWriteToBinLog? TABLE_SYMBOL tableRefList
    | type = CHECK_SYMBOL TABLE_SYMBOL tableRefList checkOption*
    | type = CHECKSUM_SYMBOL TABLE_SYMBOL tableRefList (QUICK_SYMBOL | EXTENDED_SYMBOL)?
    | type = OPTIMIZE_SYMBOL noWriteToBinLog? TABLE_SYMBOL tableRefList
    | type = REPAIR_SYMBOL noWriteToBinLog? TABLE_SYMBOL tableRefList repairOption*
    | {serverVersion < 50500}? type = BACKUP_SYMBOL TABLE_SYMBOL tableRefList TO_SYMBOL textLiteral
    | {serverVersion < 50500}? type = RESTORE_SYMBOL TABLE_SYMBOL tableRefList FROM_SYMBOL textLiteral
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
    action = INSTALL_SYMBOL PLUGIN_SYMBOL identifier SONAME_SYMBOL textLiteral
    | action = UNINSTALL_SYMBOL PLUGIN_SYMBOL identifier
;

//--------------------------------------------------------------------------------------------------

setStatement:
    SET_SYMBOL
        (
            optionType? TRANSACTION_SYMBOL setTransactionCharacteristic
            // ONE_SHOT is available only until 5.6. We don't need a predicate here, however. Handling it in the lexer is enough.
            | ONE_SHOT_SYMBOL? optionValueNoOptionType (COMMA_SYMBOL optionValueList)?
            | optionType optionValueFollowingOptionType (COMMA_SYMBOL optionValueList)?

            // SET PASSWORD is handled in an own rule.
        )
;

optionValueNoOptionType:
    NAMES_SYMBOL
        (
            equal expr
            | charsetNameOrDefault (COLLATE_SYMBOL collationNameOrDefault)?
        )
    | variableName equal setExpressionOrDefault
    | userVariable equal expr
    | systemVariable equal setExpressionOrDefault
    | charsetClause
;

optionValueFollowingOptionType:
    variableName equal setExpressionOrDefault
;

setExpressionOrDefault:
    expr
    | (DEFAULT_SYMBOL | ON_SYMBOL | ALL_SYMBOL | BINARY_SYMBOL)
;

optionValueList:
    optionValue (COMMA_SYMBOL optionValue)*
;

optionValue:
    optionType variableName equal setExpressionOrDefault
    | optionValueNoOptionType
;

//--------------------------------------------------------------------------------------------------

showStatement:
    SHOW_SYMBOL
    (
        {serverVersion < 50700}? value = AUTHORS_SYMBOL
        | value = DATABASES_SYMBOL likeOrWhere?
        | FULL_SYMBOL? value = TABLES_SYMBOL inDb? likeOrWhere?
        | FULL_SYMBOL? value = TRIGGERS_SYMBOL inDb? likeOrWhere?
        | value = EVENTS_SYMBOL inDb? likeOrWhere?
        | value = TABLE_SYMBOL STATUS_SYMBOL inDb? likeOrWhere?
        | value = OPEN_SYMBOL TABLES_SYMBOL inDb? likeOrWhere?
        | {(serverVersion >= 50105) && (serverVersion < 50500)}? value = PLUGIN_SYMBOL // Supported between 5.1.5 and 5.5.0.
        | {serverVersion >= 50500}? value = PLUGINS_SYMBOL
        | value = ENGINE_SYMBOL engineRef (STATUS_SYMBOL | MUTEX_SYMBOL)
        | FULL_SYMBOL? value = COLUMNS_SYMBOL (FROM_SYMBOL | IN_SYMBOL) tableRef inDb? likeOrWhere?
        | (BINARY_SYMBOL | MASTER_SYMBOL) value = LOGS_SYMBOL
        | value = SLAVE_SYMBOL
            (
                HOSTS_SYMBOL
                | STATUS_SYMBOL nonBlocking channel?
            )
        | value = (BINLOG_SYMBOL | RELAYLOG_SYMBOL) EVENTS_SYMBOL (IN_SYMBOL textString)? (FROM_SYMBOL ulonglong_number)? limitClause? channel?
        | value = (INDEX_SYMBOL | INDEXES_SYMBOL | KEYS_SYMBOL) fromOrIn tableRef inDb? whereClause?
        | STORAGE_SYMBOL? value = ENGINES_SYMBOL
        | value = PRIVILEGES_SYMBOL
        | COUNT_SYMBOL OPEN_PAR_SYMBOL MULT_OPERATOR CLOSE_PAR_SYMBOL value = (WARNINGS_SYMBOL | ERRORS_SYMBOL)
        | value = WARNINGS_SYMBOL limitClause?
        | value = ERRORS_SYMBOL limitClause?
        | value = PROFILES_SYMBOL
        | value = PROFILE_SYMBOL (profileType (COMMA_SYMBOL profileType)*)? (FOR_SYMBOL QUERY_SYMBOL INT_NUMBER)? limitClause?
        | optionType? value = (STATUS_SYMBOL | VARIABLES_SYMBOL) likeOrWhere?
        | FULL_SYMBOL? value = PROCESSLIST_SYMBOL
        | charset likeOrWhere?
        | value = COLLATION_SYMBOL likeOrWhere?
        | {serverVersion < 50700}? value = CONTRIBUTORS_SYMBOL
        | value = GRANTS_SYMBOL (FOR_SYMBOL user)?
        | value = MASTER_SYMBOL STATUS_SYMBOL
        | value = CREATE_SYMBOL
            (
                object = DATABASE_SYMBOL ifNotExists? schemaRef
                | object = EVENT_SYMBOL eventRef
                | object = FUNCTION_SYMBOL functionRef
                | object = PROCEDURE_SYMBOL procedureRef
                | object = TABLE_SYMBOL tableRef
                | object = TRIGGER_SYMBOL triggerRef
                | object = VIEW_SYMBOL viewRef
                | {serverVersion >= 50704}? object = USER_SYMBOL user
            )
        | value = PROCEDURE_SYMBOL STATUS_SYMBOL likeOrWhere?
        | value = FUNCTION_SYMBOL STATUS_SYMBOL likeOrWhere?
        | value = PROCEDURE_SYMBOL CODE_SYMBOL procedureRef
        | value = FUNCTION_SYMBOL CODE_SYMBOL functionRef
        | {serverVersion < 50500}? value = INNODB_SYMBOL STATUS_SYMBOL // Deprecated in 5.5.
    )
;

nonBlocking:
    {serverVersion >= 50700 && serverVersion < 50706}? NONBLOCKING_SYMBOL?
    | /* empty */
;

fromOrIn:
    FROM_SYMBOL | IN_SYMBOL
;

inDb:
    fromOrIn identifier
;

profileType:
    BLOCK_SYMBOL IO_SYMBOL
    | CONTEXT_SYMBOL SWITCHES_SYMBOL
    | PAGE_SYMBOL FAULTS_SYMBOL
    | (ALL_SYMBOL | CPU_SYMBOL | IPC_SYMBOL | MEMORY_SYMBOL | SOURCE_SYMBOL | SWAPS_SYMBOL)
;

//--------------------------------------------------------------------------------------------------

otherAdministrativeStatement:
    type = BINLOG_SYMBOL textLiteral
    | type = CACHE_SYMBOL INDEX_SYMBOL keyCacheListOrParts IN_SYMBOL (identifier | DEFAULT_SYMBOL)
    | type = FLUSH_SYMBOL noWriteToBinLog?
        (
            flushTables
            | flushOption (COMMA_SYMBOL flushOption)*
        )
    | type = KILL_SYMBOL  (CONNECTION_SYMBOL | QUERY_SYMBOL)? expr
    | type = LOAD_SYMBOL INDEX_SYMBOL INTO_SYMBOL CACHE_SYMBOL loadTableIndexList
    | {serverVersion >= 50709}? type = SHUTDOWN_SYMBOL
;

keyCacheListOrParts:
    keyCacheList
    | assignToKeycachePartition
;

keyCacheList:
    assignToKeycache (COMMA_SYMBOL assignToKeycache)*
;

assignToKeycache:
    tableRef cacheKeysSpec?
;

assignToKeycachePartition:
    tableRef PARTITION_SYMBOL OPEN_PAR_SYMBOL (ALL_SYMBOL | identifierList) CLOSE_PAR_SYMBOL cacheKeysSpec?
;

cacheKeysSpec:
    (KEY_SYMBOL | INDEX_SYMBOL) OPEN_PAR_SYMBOL (keyUsageElement (COMMA_SYMBOL keyUsageElement)*)? CLOSE_PAR_SYMBOL
;

keyUsageElement:
    identifier
    | PRIMARY_SYMBOL
;

flushOption:
    option = (DES_KEY_FILE_SYMBOL | HOSTS_SYMBOL | PRIVILEGES_SYMBOL | STATUS_SYMBOL | USER_RESOURCES_SYMBOL)
    | logType? option = LOGS_SYMBOL
    | option = RELAY_SYMBOL LOGS_SYMBOL channel?
    | option = QUERY_SYMBOL CACHE_SYMBOL
    | {serverVersion >= 50706}? option = OPTIMIZER_COSTS_SYMBOL
;

logType:
    BINARY_SYMBOL
    | ENGINE_SYMBOL
    | ERROR_SYMBOL
    | GENERAL_SYMBOL
    | SLOW_SYMBOL
;

flushTables:
    (TABLES_SYMBOL | TABLE_SYMBOL)
    (
        WITH_SYMBOL READ_SYMBOL LOCK_SYMBOL
        | identifierList flushTablesOptions?
    )?
;

flushTablesOptions:
    {serverVersion >= 50606}? FOR_SYMBOL EXPORT_SYMBOL
    | WITH_SYMBOL READ_SYMBOL LOCK_SYMBOL
;

loadTableIndexList:
    tableRef loadTableIndexPartion?
        ((INDEX_SYMBOL | KEY_SYMBOL)? identifierListWithParentheses)? (IGNORE_SYMBOL LEAVES_SYMBOL)?
;

loadTableIndexPartion:
    {serverVersion >= 50500}? (PARTITION_SYMBOL OPEN_PAR_SYMBOL (identifierList | ALL_SYMBOL) CLOSE_PAR_SYMBOL)
;

//--------------------------------------------------------------------------------------------------

utilityStatement:
    describeCommand
    | helpCommand
    | useCommand
;

describeCommand:
    (DESCRIBE_SYMBOL | DESC_SYMBOL) (
        tableRef (textString | identifier)?
        | // Format must be "traditional" or "json".
            (
                EXTENDED_SYMBOL // deprecated since 5.7
                | {serverVersion >= 50105}? PARTITIONS_SYMBOL // deprecated since 5.7
                | {serverVersion >= 50605}? FORMAT_SYMBOL EQUAL_OPERATOR textOrIdentifier
            )? explainableStatement
    )
;

// Before server version 5.6 only select statements were explainable.
explainableStatement:
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

helpCommand: HELP_SYMBOL textOrIdentifier;

useCommand:
    USE_SYMBOL identifier
;

//----------------- Expression support -------------------------------------------------------------

expr:
    boolPri (IS_SYMBOL notRule? type = (TRUE_SYMBOL | FALSE_SYMBOL | UNKNOWN_SYMBOL))?  # exprIs
    | NOT_SYMBOL expr                                                                   # exprNot
    | expr op = (AND_SYMBOL | LOGICAL_AND_OPERATOR) expr                                # exprAnd
    | expr XOR_SYMBOL expr                                                              # exprXor
    | expr op = (OR_SYMBOL | LOGICAL_OR_OPERATOR) expr                                  # exprOr
;

boolPri:
    predicate                                                                             # primaryExprPredicate
    | boolPri IS_SYMBOL notRule? NULL_SYMBOL                                              # primaryExprIsNull
    | boolPri compOp predicate                                                            # primaryExprCompare
    | boolPri compOp (ALL_SYMBOL | ANY_SYMBOL) OPEN_PAR_SYMBOL subselect CLOSE_PAR_SYMBOL # primaryExprAllAny
;

compOp:
    EQUAL_OPERATOR
    | NULL_SAFE_EQUAL_OPERATOR
    | GREATER_OR_EQUAL_OPERATOR
    | GREATER_THAN_OPERATOR
    | LESS_OR_EQUAL_OPERATOR
    | LESS_THAN_OPERATOR
    | NOT_EQUAL_OPERATOR
;

predicate:
    bitExpr (notRule? predicateOperations)?     # predicateExprOperations
    | bitExpr SOUNDS_SYMBOL LIKE_SYMBOL bitExpr # predicateExprSoundsLike
;

predicateOperations:
    IN_SYMBOL OPEN_PAR_SYMBOL (subselect | expressionList) CLOSE_PAR_SYMBOL # predicateExprIn
    | BETWEEN_SYMBOL bitExpr AND_SYMBOL predicate                           # predicateExprBetween
    | LIKE_SYMBOL simpleExpr (ESCAPE_SYMBOL simpleExpr)?                    # predicateExprLike
    | REGEXP_SYMBOL bitExpr                                                 # predicateExprRegex
;

bitExpr:
    simpleExpr
    | bitExpr op = BITWISE_XOR_OPERATOR bitExpr
    | bitExpr op = (
        MULT_OPERATOR | DIV_OPERATOR | MOD_OPERATOR | DIV_SYMBOL | MOD_SYMBOL
    ) bitExpr
    | bitExpr op = (PLUS_OPERATOR | MINUS_OPERATOR) bitExpr
    | bitExpr op = (PLUS_OPERATOR | MINUS_OPERATOR) INTERVAL_SYMBOL expr interval
    | bitExpr op = (SHIFT_LEFT_OPERATOR | SHIFT_RIGHT_OPERATOR) bitExpr
    | bitExpr op = BITWISE_AND_OPERATOR bitExpr
    | bitExpr op = BITWISE_OR_OPERATOR bitExpr
;

simpleExpr:
    variable                                                                         # simpleExprVariable
    | simpleIdentifier jsonOperator?                                                 # simpleExprIdentifier
    | runtimeFunctionCall                                                            # simpleExprRuntimeFunction
    | functionCall                                                                   # simpleExprFunction
    | simpleExpr COLLATE_SYMBOL textOrIdentifier                                     # simpleExprCollate
    | literal                                                                        # simpleExprLiteral
    | PARAM_MARKER                                                                   # simpleExprParamMarker
    | sumExpr                                                                        # simpleExprSum
    | simpleExpr CONCAT_PIPES_SYMBOL simpleExpr                                      # simpleExprConcat
    | op = (PLUS_OPERATOR | MINUS_OPERATOR | BITWISE_NOT_OPERATOR) simpleExpr        # simpleExprUnary
    | not2Rule simpleExpr                                                            # simpleExprNot
    | OPEN_PAR_SYMBOL subselect CLOSE_PAR_SYMBOL                                     # simpleExprSubselect
    | ROW_SYMBOL? OPEN_PAR_SYMBOL expressionList CLOSE_PAR_SYMBOL                    # simpleExprList
    | EXISTS_SYMBOL OPEN_PAR_SYMBOL subselect CLOSE_PAR_SYMBOL                       # simpleExprExists
    | OPEN_CURLY_SYMBOL identifier expr CLOSE_CURLY_SYMBOL                           # simpleExprOdbc
    | MATCH_SYMBOL identListArg AGAINST_SYMBOL
        OPEN_PAR_SYMBOL bitExpr fulltext_options CLOSE_PAR_SYMBOL                    # simpleExprMatch
    | BINARY_SYMBOL simpleExpr                                                       # simpleExprBinary
    | CAST_SYMBOL OPEN_PAR_SYMBOL expr AS_SYMBOL castType CLOSE_PAR_SYMBOL           # simpleExprCast
    | CASE_SYMBOL expr? (whenExpression thenExpression)+ elseExpression? END_SYMBOL  # simpleExprCase
    | CONVERT_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL castType CLOSE_PAR_SYMBOL     # simpleExprConvert
    | CONVERT_SYMBOL OPEN_PAR_SYMBOL expr USING_SYMBOL charsetName CLOSE_PAR_SYMBOL  # simpleExprConvertUsing
    | DEFAULT_SYMBOL OPEN_PAR_SYMBOL simpleIdentifier CLOSE_PAR_SYMBOL               # simpleExprDefault
    | VALUES_SYMBOL OPEN_PAR_SYMBOL simpleIdentifier CLOSE_PAR_SYMBOL                # simpleExprValues
    | INTERVAL_SYMBOL expr interval PLUS_OPERATOR expr                               # simpleExprInterval
;

jsonOperator:
  {serverVersion >= 50708}? JSON_SEPARATOR_SYMBOL textStringLiteral
  | {serverVersion >= 50713}? JSON_UNQUOTED_SEPARATOR_SYMBOL textStringLiteral
;

sumExpr:
    name = AVG_SYMBOL OPEN_PAR_SYMBOL DISTINCT_SYMBOL? inSumExpr CLOSE_PAR_SYMBOL
    | name = (BIT_AND_SYMBOL | BIT_OR_SYMBOL | BIT_XOR_SYMBOL) OPEN_PAR_SYMBOL inSumExpr CLOSE_PAR_SYMBOL
    | name = COUNT_SYMBOL OPEN_PAR_SYMBOL ALL_SYMBOL? MULT_OPERATOR CLOSE_PAR_SYMBOL
    | name = COUNT_SYMBOL OPEN_PAR_SYMBOL (inSumExpr | DISTINCT_SYMBOL expressionList) CLOSE_PAR_SYMBOL
    | name = MIN_SYMBOL OPEN_PAR_SYMBOL DISTINCT_SYMBOL? inSumExpr CLOSE_PAR_SYMBOL
    | name = MAX_SYMBOL OPEN_PAR_SYMBOL DISTINCT_SYMBOL? inSumExpr CLOSE_PAR_SYMBOL
    | name = STD_SYMBOL OPEN_PAR_SYMBOL inSumExpr CLOSE_PAR_SYMBOL
    | name = VARIANCE_SYMBOL OPEN_PAR_SYMBOL inSumExpr CLOSE_PAR_SYMBOL
    | name = STDDEV_SAMP_SYMBOL OPEN_PAR_SYMBOL inSumExpr CLOSE_PAR_SYMBOL
    | name = VAR_SAMP_SYMBOL OPEN_PAR_SYMBOL inSumExpr CLOSE_PAR_SYMBOL
    | name = SUM_SYMBOL OPEN_PAR_SYMBOL DISTINCT_SYMBOL? inSumExpr CLOSE_PAR_SYMBOL
    | name = GROUP_CONCAT_SYMBOL OPEN_PAR_SYMBOL DISTINCT_SYMBOL? expressionList orderClause?
        (SEPARATOR_SYMBOL textString)? CLOSE_PAR_SYMBOL
;

inSumExpr:
    ALL_SYMBOL? expr
;

identListArg:
    identList
    | OPEN_PAR_SYMBOL identList CLOSE_PAR_SYMBOL
;

identList:
    simpleIdentifier (COMMA_SYMBOL simpleIdentifier)*
;

fulltext_options:
    IN_SYMBOL BOOLEAN_SYMBOL MODE_SYMBOL
    | (IN_SYMBOL NATURAL_SYMBOL LANGUAGE_SYMBOL MODE_SYMBOL)? (WITH_SYMBOL QUERY_SYMBOL EXPANSION_SYMBOL)?
;

runtimeFunctionCall:
    // Function names that are keywords.
    name = CHAR_SYMBOL OPEN_PAR_SYMBOL expressionList (USING_SYMBOL charsetName)? CLOSE_PAR_SYMBOL
    | name = CURRENT_USER_SYMBOL parentheses?
    | name = DATE_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = DAY_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = HOUR_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = INSERT_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr COMMA_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = INTERVAL_SYMBOL OPEN_PAR_SYMBOL expr (COMMA_SYMBOL expr)+ CLOSE_PAR_SYMBOL
    | name = LEFT_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = MINUTE_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = MONTH_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = RIGHT_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = SECOND_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = TIME_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = TIMESTAMP_SYMBOL OPEN_PAR_SYMBOL expr (COMMA_SYMBOL expr)? CLOSE_PAR_SYMBOL
    | trimFunction
    | name = USER_SYMBOL parentheses
    | name = VALUES_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = YEAR_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL

    // Function names that are not keywords.
    | name = (ADDDATE_SYMBOL | SUBDATE_SYMBOL) OPEN_PAR_SYMBOL expr COMMA_SYMBOL (expr | INTERVAL_SYMBOL expr interval) CLOSE_PAR_SYMBOL
    | name = CURDATE_SYMBOL parentheses?
    | name = CURTIME_SYMBOL timeFunctionParameters?
    | name = (DATE_ADD_SYMBOL | DATE_SUB_SYMBOL) OPEN_PAR_SYMBOL expr COMMA_SYMBOL INTERVAL_SYMBOL expr interval CLOSE_PAR_SYMBOL
    | name = EXTRACT_SYMBOL OPEN_PAR_SYMBOL interval FROM_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = GET_FORMAT_SYMBOL OPEN_PAR_SYMBOL dateTimeTtype  COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = NOW_SYMBOL timeFunctionParameters?
    | name = POSITION_SYMBOL OPEN_PAR_SYMBOL bitExpr IN_SYMBOL expr CLOSE_PAR_SYMBOL
    | substringFunction
    | name = SYSDATE_SYMBOL timeFunctionParameters?
    | name = (TIMESTAMP_ADD_SYMBOL | TIMESTAMP_DIFF_SYMBOL) OPEN_PAR_SYMBOL intervalTimeStamp COMMA_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = UTC_DATE_SYMBOL parentheses?
    | name = UTC_TIME_SYMBOL timeFunctionParameters?
    | name = UTC_TIMESTAMP_SYMBOL timeFunctionParameters?

    // Function calls with other conflicts.
    | name = ASCII_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = CHARSET_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = COALESCE_SYMBOL expressionListWithParentheses
    | name = COLLATION_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = DATABASE_SYMBOL parentheses
    | name = IF_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = FORMAT_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr (COMMA_SYMBOL expr)? CLOSE_PAR_SYMBOL
    | name = MICROSECOND_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = MOD_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | {serverVersion < 50607}? name = OLD_PASSWORD_SYMBOL OPEN_PAR_SYMBOL textLiteral CLOSE_PAR_SYMBOL
    | name = PASSWORD_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = QUARTER_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = REPEAT_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = REPLACE_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = REVERSE_SYMBOL OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = ROW_COUNT_SYMBOL parentheses
    | name = TRUNCATE_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = WEEK_SYMBOL OPEN_PAR_SYMBOL expr (COMMA_SYMBOL expr)? CLOSE_PAR_SYMBOL
    | {serverVersion >= 50600}? name = WEIGHT_STRING_SYMBOL OPEN_PAR_SYMBOL expr
        (
            (AS_SYMBOL CHAR_SYMBOL fieldLength)? weightStringLevels?
            | AS_SYMBOL BINARY_SYMBOL fieldLength
            | COMMA_SYMBOL ulong_number COMMA_SYMBOL ulong_number COMMA_SYMBOL ulong_number
        )
        CLOSE_PAR_SYMBOL
    | geometryFunction
;

geometryFunction:
    name = GEOMETRYCOLLECTION_SYMBOL OPEN_PAR_SYMBOL expressionList? CLOSE_PAR_SYMBOL
    | name = LINESTRING_SYMBOL expressionListWithParentheses
    | name = MULTILINESTRING_SYMBOL expressionListWithParentheses
    | name = MULTIPOINT_SYMBOL expressionListWithParentheses
    | name = MULTIPOLYGON_SYMBOL expressionListWithParentheses
    | name = POINT_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
    | name = POLYGON_SYMBOL expressionListWithParentheses
    | {serverVersion < 50706}? name = CONTAINS_SYMBOL OPEN_PAR_SYMBOL expr COMMA_SYMBOL expr CLOSE_PAR_SYMBOL
;

timeFunctionParameters:
    OPEN_PAR_SYMBOL fractionalPrecision? CLOSE_PAR_SYMBOL
;

fractionalPrecision:
    {serverVersion >= 50604}? INT_NUMBER
;

weightStringLevels:
    LEVEL_SYMBOL
    (
        real_ulong_number MINUS_OPERATOR real_ulong_number
        | weightStringLevelListItem (COMMA_SYMBOL weightStringLevelListItem)*
    )
;

weightStringLevelListItem:
    real_ulong_number
    (
        (ASC_SYMBOL | DESC_SYMBOL) REVERSE_SYMBOL?
        | REVERSE_SYMBOL
    )?
;

dateTimeTtype:
    DATE_SYMBOL
    | TIME_SYMBOL
    | DATETIME_SYMBOL
    | TIMESTAMP_SYMBOL
;

trimFunction:
    TRIM_SYMBOL OPEN_PAR_SYMBOL
    (
        expr (FROM_SYMBOL expr)?
        | LEADING_SYMBOL expr? FROM_SYMBOL expr
        | TRAILING_SYMBOL expr? FROM_SYMBOL expr
        | BOTH_SYMBOL expr? FROM_SYMBOL expr
    )
    CLOSE_PAR_SYMBOL
;

substringFunction:
    SUBSTRING_SYMBOL OPEN_PAR_SYMBOL expr
    (
        COMMA_SYMBOL expr (COMMA_SYMBOL expr)?
        | FROM_SYMBOL expr (FOR_SYMBOL expr)?
    )
    CLOSE_PAR_SYMBOL
;

functionCall:
    pureIdentifier OPEN_PAR_SYMBOL aliasedExpressionList? CLOSE_PAR_SYMBOL // For both UDF + other functions.
    | qualifiedIdentifier OPEN_PAR_SYMBOL expressionList? CLOSE_PAR_SYMBOL // Other functions only.
;

aliasedExpressionList:
    aliasedExpression (COMMA_SYMBOL aliasedExpression)*
;

aliasedExpression:
    expr selectAlias?
;

variable:
    userVariable (ASSIGN_OPERATOR expr)?
    | systemVariable
;

userVariable:
    (AT_SIGN_SYMBOL textOrIdentifier)
    | AT_TEXT_SUFFIX
;

// System variables as used in exprs. SET has another variant of this (SET GLOBAL/LOCAL varname).
systemVariable:
    AT_AT_SIGN_SYMBOL (optionType DOT_SYMBOL)? variableName
;

variableName:
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

castType:
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

notRule:
    NOT_SYMBOL
    | NOT2_SYMBOL // A NOT with a different (higher) operator precedence.
;

not2Rule:
    LOGICAL_NOT_OPERATOR
    | NOT2_SYMBOL
;

xorRule:
    XOR_SYMBOL
    | BITWISE_XOR_OPERATOR
;

// None of the microsecond variants can be used in schedules (e.g. events).
interval:
    intervalTimeStamp
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
intervalTimeStamp:
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

expressionListWithParentheses:
    OPEN_PAR_SYMBOL expressionList CLOSE_PAR_SYMBOL
;

expressionList:
    expr (COMMA_SYMBOL expr)*
;

orderList:
    expr direction? (COMMA_SYMBOL expr direction?)*
;

channel:
    {serverVersion >= 50706}? FOR_SYMBOL CHANNEL_SYMBOL textStringNoLinebreak
;

//----------------- Stored program rules -----------------------------------------------------------

// Compound syntax for stored procedures, stored functions, triggers and events.
// Both sp_proc_stmt and ev_sql_stmt_inner in the server grammar.
compoundStatement:
    statement
    | returnStatement
    | ifStatement
    | caseStatement
    | labeledBlock
    | unlabeledBlock
    | labeledControl
    | unlabeledControl
    | leaveStatement
    | iterateStatement

    | cursorOpen
    | cursorFetch
    | cursorClose
;

returnStatement:
    RETURN_SYMBOL expr
;

ifStatement:
    IF_SYMBOL ifBody END_SYMBOL IF_SYMBOL
;

ifBody:
    expr thenStatement (ELSEIF_SYMBOL ifBody | ELSE_SYMBOL compoundStatementList)?
;

thenStatement:
    THEN_SYMBOL compoundStatementList
;

compoundStatementList:
    (compoundStatement SEMICOLON_SYMBOL)+
;

caseStatement:
    CASE_SYMBOL expr? (whenExpression thenStatement)+ elseStatement? END_SYMBOL CASE_SYMBOL
;

elseStatement:
    ELSE_SYMBOL compoundStatementList
;

labeledBlock:
    label beginEndBlock labelIdentifier?
;

unlabeledBlock:
    beginEndBlock
;

label:
    // Block labels can only be up to 16 characters long.
    labelIdentifier COLON_SYMBOL
;

labelIdentifier:
    pureIdentifier | keywordSp
;

beginEndBlock:
    BEGIN_SYMBOL spDeclarations? compoundStatementList? END_SYMBOL
;

labeledControl:
    label unlabeledControl labelIdentifier?
;

unlabeledControl:
    loopBlock
    | whileDoBlock
    | repeatUntilBlock
;

loopBlock:
    LOOP_SYMBOL compoundStatementList END_SYMBOL LOOP_SYMBOL
;

whileDoBlock:
    WHILE_SYMBOL expr DO_SYMBOL compoundStatementList END_SYMBOL WHILE_SYMBOL
;

repeatUntilBlock:
    REPEAT_SYMBOL compoundStatementList UNTIL_SYMBOL expr END_SYMBOL REPEAT_SYMBOL
;

spDeclarations:
    (spDeclaration SEMICOLON_SYMBOL)+
;

spDeclaration:
    variableDeclaration
    | conditionDeclaration
    | handlerDeclaration
    | cursorDeclaration
;

variableDeclaration:
    DECLARE_SYMBOL identifierList dataType (COLLATE_SYMBOL collationNameOrDefault)? (DEFAULT_SYMBOL expr)?
;

conditionDeclaration:
    DECLARE_SYMBOL identifier CONDITION_SYMBOL FOR_SYMBOL spCondition
;

spCondition:
    ulong_number
    | sqlstate
;

sqlstate:
    SQLSTATE_SYMBOL VALUE_SYMBOL? textLiteral
;

handlerDeclaration:
    DECLARE_SYMBOL (CONTINUE_SYMBOL | EXIT_SYMBOL | UNDO_SYMBOL) HANDLER_SYMBOL
        FOR_SYMBOL handlerCondition (COMMA_SYMBOL handlerCondition)* compoundStatement
;

handlerCondition:
    spCondition
    | identifier
    | SQLWARNING_SYMBOL
    | notRule FOUND_SYMBOL
    | SQLEXCEPTION_SYMBOL
;

cursorDeclaration:
    DECLARE_SYMBOL identifier CURSOR_SYMBOL FOR_SYMBOL selectStatement
;

iterateStatement:
    ITERATE_SYMBOL labelIdentifier
;

leaveStatement:
    LEAVE_SYMBOL labelIdentifier
;

getDiagnostics:
    GET_SYMBOL
    (
        CURRENT_SYMBOL
        | {serverVersion >= 50700}? STACKED_SYMBOL
    )? DIAGNOSTICS_SYMBOL
    (
        statementInformationItem (COMMA_SYMBOL statementInformationItem)*
        | CONDITION_SYMBOL signalAllowedExpr conditionInformationItem (COMMA_SYMBOL conditionInformationItem)*
    )
;

// Only a limited subset of expr is allowed in SIGNAL/RESIGNAL/CONDITIONS.
signalAllowedExpr:
    literal
    | variable
    | qualifiedIdentifier
;

statementInformationItem:
    (variable | identifier) EQUAL_OPERATOR (NUMBER_SYMBOL | ROW_COUNT_SYMBOL)
;

conditionInformationItem:
    (variable | identifier) EQUAL_OPERATOR (signalInformationItemName | RETURNED_SQLSTATE_SYMBOL)
;

signalInformationItemName:
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
    SIGNAL_SYMBOL (identifier | sqlstate) (SET_SYMBOL signalInformationItem (COMMA_SYMBOL signalInformationItem)*)?
;

resignalStatement:
    RESIGNAL_SYMBOL (SQLSTATE_SYMBOL VALUE_SYMBOL? textOrIdentifier)?
        (SET_SYMBOL signalInformationItem (COMMA_SYMBOL signalInformationItem)*)?
;

signalInformationItem:
    signalInformationItemName EQUAL_OPERATOR signalAllowedExpr
;

cursorOpen:
    OPEN_SYMBOL identifier
;

cursorClose:
    CLOSE_SYMBOL identifier
;

cursorFetch:
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
        OPEN_PAR_SYMBOL expr CLOSE_PAR_SYMBOL (VIRTUAL_SYMBOL | STORED_SYMBOL)? gcolAttribute*
;

attribute:
    NOT_SYMBOL? nullLiteral
    | value = DEFAULT_SYMBOL (signedLiteral | NOW_SYMBOL timeFunctionParameters?)
    | value = ON_SYMBOL UPDATE_SYMBOL NOW_SYMBOL timeFunctionParameters?
    | value = AUTO_INCREMENT_SYMBOL
    | value = SERIAL_SYMBOL DEFAULT_SYMBOL VALUE_SYMBOL
    | value = UNIQUE_SYMBOL KEY_SYMBOL?
    | PRIMARY_SYMBOL? value = KEY_SYMBOL
    | value = COMMENT_SYMBOL textLiteral
    | value = COLLATE_SYMBOL collationName
    | value = COLUMN_FORMAT_SYMBOL (FIXED_SYMBOL | DYNAMIC_SYMBOL | DEFAULT_SYMBOL)
    | value = STORAGE_SYMBOL (DISK_SYMBOL | MEMORY_SYMBOL | DEFAULT_SYMBOL)
;

gcolAttribute:
    UNIQUE_SYMBOL KEY_SYMBOL?
    | COMMENT_SYMBOL textString
    | notRule? NULL_SYMBOL
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
    | {serverVersion >= 50600}? COMMENT_SYMBOL textLiteral
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
    | LINEAR_SYMBOL? HASH_SYMBOL OPEN_PAR_SYMBOL bitExpr CLOSE_PAR_SYMBOL                              # partitionDefHash
    | (RANGE_SYMBOL | LIST_SYMBOL)
        (
            OPEN_PAR_SYMBOL bitExpr CLOSE_PAR_SYMBOL
            | {serverVersion >= 50500}? COLUMNS_SYMBOL OPEN_PAR_SYMBOL identifierList? CLOSE_PAR_SYMBOL
        )                                                                                               # partitionDefRangeList
;

subPartitions:
    SUBPARTITION_SYMBOL BY_SYMBOL LINEAR_SYMBOL?
    (
        HASH_SYMBOL OPEN_PAR_SYMBOL bitExpr CLOSE_PAR_SYMBOL
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
    | option = (DATA_SYMBOL | INDEX_SYMBOL) DIRECTORY_SYMBOL EQUAL_OPERATOR? textLiteral
    | option = COMMENT_SYMBOL EQUAL_OPERATOR? textLiteral
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
    IF_SYMBOL notRule EXISTS_SYMBOL
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

columnAssignmentListWithDefault:
    columnAssignmentWithDefault (COMMA_SYMBOL columnAssignmentWithDefault)*
;

columnAssignmentWithDefault:
    columnRef EQUAL_OPERATOR (expr | DEFAULT_SYMBOL)
;

charsetClause:
    charset (textOrIdentifier | (DEFAULT_SYMBOL | BINARY_SYMBOL))
;

fieldsClause:
    COLUMNS_SYMBOL fieldTerm+
;

fieldTerm:
    TERMINATED_SYMBOL BY_SYMBOL textString
    | OPTIONALLY_SYMBOL? ENCLOSED_SYMBOL BY_SYMBOL textString
    | ESCAPED_SYMBOL BY_SYMBOL textString
;

linesClause:
    LINES_SYMBOL lineTerm+
;

lineTerm:
    (TERMINATED_SYMBOL | STARTING_SYMBOL) BY_SYMBOL textString
;

userList:
    user (COMMA_SYMBOL user)*
;

grantList:
    grantUser (COMMA_SYMBOL grantUser)*
;

grantUser:
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

likeClause:
    LIKE_SYMBOL textString
;

likeOrWhere:
    likeClause | whereClause
;

onlineOption:
    {serverVersion < 50600}? (ONLINE_SYMBOL | OFFLINE_SYMBOL)
;

noWriteToBinLog:
    LOCAL_SYMBOL // Predicate needed to direct the parser (as LOCAL can also be an identifier).
    | NO_WRITE_TO_BINLOG_SYMBOL
;

usePartition:
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
    | {isSqlModeActive(AnsiQuotes)}? DOUBLE_QUOTED_TEXT
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

literal:
    textLiteral
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

stringList:
    OPEN_PAR_SYMBOL textString (COMMA_SYMBOL textString)* CLOSE_PAR_SYMBOL
;

textStringLiteral: // TEXT_STRING_sys + TEXT_STRING_literal + TEXT_STRING_filesystem + TEXT_STRING in sql_yacc.yy.
    value = SINGLE_QUOTED_TEXT | {!isSqlModeActive(AnsiQuotes)}? value = DOUBLE_QUOTED_TEXT
;

textString:
    textStringLiteral | HEX_NUMBER | BIN_NUMBER
;

textLiteral:
    (UNDERSCORE_CHARSET? textStringLiteral | NCHAR_TEXT) textStringLiteral*
;

// A special variant of a text string that must not contain a linebreak (TEXT_STRING_sys_nonewline in sql_yacc.yy).
// Check validity in semantic phase.
textStringNoLinebreak:
    textStringLiteral {!containsLinebreak($text)}?
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

optionType:
    GLOBAL_SYMBOL | LOCAL_SYMBOL | SESSION_SYMBOL
;

// Keyword that we allow for identifiers.
// Keywords defined only for specific server versions are handled at lexer level and so cannot match this rule
// if the current server version doesn't allow them. Hence we don't need predicates here for them.
keyword:
    keywordSp
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
    | {serverVersion >= 50709}? SHUTDOWN_SYMBOL // Moved here from keywordSp in version 5.7.9 .
;

// Comment from server yacc grammar:
//   Keywords that we allow for labels in SPs. Anything that's the beginning of a statement
//   or characteristics must be in keyword above, otherwise we get (harmful) shift/reduce conflicts.
// Additionally:
//   The keywords are only roughly sorted to stay with the same order as in sql_yacc.yy (for simpler diff'ing).
keywordSp:
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
