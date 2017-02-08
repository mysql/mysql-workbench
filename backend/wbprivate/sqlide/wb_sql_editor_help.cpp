/*
 * Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "base/sqlstring.h"
#include "base/log.h"
#include "base/threading.h"

#include "workbench/wb_backend_public_interface.h"
#include "wb_sql_editor_help.h"

#include <antlr3.h>
#include "mysql-parser.h"
#include "mysql-scanner.h"
#include "MySQLLexer.h"

DEFAULT_LOG_DOMAIN("Context help")

//--------------------------------------------------------------------------------------------------

/**
 * See if there's a help topic for the proposed topic. Return the real topic title if so.
 */
std::string DbSqlEditorContextHelp::lookup_topic_for_string(const SqlEditorForm::Ref &form, std::string topic) {
  if (!topic.empty()) {
    // HELP supports wildcards. We don't use them but if a single % is looked up we get all topics as result.
    // Since we know there's a topic for '%' we just return it.
    if (topic != "%") {
      try {
        logDebug2("Validating topic: %s\n", topic.c_str());

        sql::Dbc_connection_handler::Ref conn;
        base::RecMutexLock aux_dbc_conn_mutex = form->ensure_valid_aux_connection(conn);

        base::sqlstring query = base::sqlstring("help ?", 0) << topic;
        std::auto_ptr<sql::ResultSet> rs(conn->ref->createStatement()->executeQuery(std::string(query)));
        if (rs->rowsCount() == 1) {
          rs->next();
          topic = rs->getString(1);
        } else
          topic = "";
      } catch (...) {
        // It's an exception but not relevant for the rest of the code. We just did not get a topic
        // (maybe the server was busy or not reachable).
        logDebug2("Exception caught while looking up topic\n");
      }
    }
  }

  return topic;
}

//--------------------------------------------------------------------------------------------------

static std::string query_type_to_help_topic[] = {
  "",                      // QtUnknown
  "",                      // QtAmbiguous
  "alter database",        // QtAlterDatabase
  "alter logfile group",   // QtAlterLogFileGroup - not there yet
  "alter function",        // QtAlterFunction
  "alter procedure",       // QtAlterProcedure
  "alter server",          // QtAlterServer
  "alter table",           // QtAlterTable
  "alter tablespace",      // QtAlterTableSpace - not there yet
  "alter event",           // QtAlterEvent
  "alter view",            // QtAlterView
  "create table",          // QtCreateTable
  "create index",          // QtCreateIndex
  "create database",       // QtCreateDatabase
  "create event",          // QtCreateEvent
  "create view",           // QtCreateView
  "create procedure",      // QtCreateRoutine
  "create procedure",      // QtCreateProcedure
  "create function",       // QtCreateFunction
  "create function udf",   // QtCreateUdf
  "create trigger",        // QtCreateTrigger
  "create logfile group",  // QtCreateLogFileGroup
  "create server",         // QtCreateServer
  "create tablespace",     // QtCreateTableSpace - not there yet
  "drop database",         // QtDropDatabase
  "drop event",            // QtDropEvent
  "drop function",         // QtDropFunction, including UDF
  "drop procedure",        // QtDropProcedure
  "drop index",            // QtDropIndex
  "drop logfile group",    // QtDropLogfileGroup - not there yet
  "drop server",           // QtDropServer
  "drop table",            // QtDropTable
  "drop tablespace",       // QtDropTablespace - not there yet
  "drop trigger",          // QtDropTrigger
  "drop view",             // QtDropView
  "rename table",          // QtRenameTable
  "truncate table",        // QtTruncateTable
  "call",                  // QtCall
  "delete",                // QtDelete
  "do",                    // QtDo
  "handler",               // QtHandler,
  "insert",                // QtInsert
  "load data",             // QtLoadData
  "load xml",              // QtLoadXML
  "replace",               // QtReplace
  "select",                // QtSelect
  "update",                // QtUpdate
  "partition",             // QtPartition (TODO: must be handled individually for each statement type it is used in)
  "start transaction",     // QtStartTransaction
  "start transaction",     // QtBeginWork,
  "commit",                // QtCommit (automatically detours to start transaction)
  "start transaction",     // QtRollbackWork,
  "start transaction",     // QtSetAutoCommit
  "start transaction",     // QtSetTransaction
  "savepoint",             // QtSavepoint
  "savepoint",             // QtReleaseSavepoint
  "savepoint",             // QtRollbackSavepoint
  "lock",                  // QtLock
  "unlock",                // QtUnlock
  "xa",                    // QtXA - not there yet
  "purge binary logs",     // QtPurge
  "change master to",      // QtChangeMaster
  "reset",                 // QtReset
  "reset master",          // QtResetMaster
  "reset slave",           // QtResetSlave
  "start slave",           // QtStartSlave
  "stop slave",            // QtStopSlave
  "load data master",      // QtLoadDataMaster - not there yet
  "load table master",     // QtLoadTableMaster - not there yet
  "prepare",               // QtPrepare
  "execute statement",     // QtExecute
  "deallocate prepare",    // QtDeallocate
  "alter user",            // QtAlterUser
  "create user",           // QtCreateUser
  "drop user",             // QtDropUser
  "grant",                 // QtGrantProxy
  "grant",                 // QtGrant
  "rename user",           // QtRenameUser
  "revoke",                // QtRevokeProxy
  "revoke",                // QtRevoke
  "analyze table",         // QtAnalyzeTable
  "check table",           // QtCheckTable
  "checksum table",        // QtChecksumTable
  "optimize table",        // QtOptimizeTable
  "repair table",          // QtRepairTable
  "backup table",          // QtBackUpTable
  "restore table",         // QtRestoreTable
  "install plugin",        // QtInstallPlugin
  "uninstall plugin",      // QtUninstallPlugin
  "set",                   // QtSet
  "set password",          // QtSetPassword
  "show",                  // QtShow
  "show authors",          // QtShowAuthors
  "show binary logs",      // QtShowBinaryLogs
  "show binlog events",    // QtShowBinlogEvents
  "show relaylog events",  // QtShowRelaylogEvents
  "show character set",    // QtShowCharset
  "show collation",        // QtShowCollation
  "show columns",          // QtShowColumns
  "show contributors",     // QtShowContributors
  "show create database",  // QtShowCreateDatabase
  "show create event",     // QtShowCreateEvent
  "show create function",  // QtShowCreateFunction
  "show create procedure", // QtShowCreateProcedure
  "show create table",     // QtShowCreateTable
  "show create trigger",   // QtShowCreateTrigger
  "show create view",      // QtShowCreateView
  "show databases",        // QtShowDatabases
  "show engine",           // QtShowEngineStatus
  "show engines",          // QtShowStorageEngines
  "show errors",           // QtShowErrors
  "show events",           // QtShowEvents
  "show function code",    // QtShowFunctionCode
  "show function status",  // QtShowFunctionStatus
  "show grants",           // QtShowGrants
  "show index",            // QtShowIndexes
  "show innodb status",    // QtShowInnoDBStatus
  "show master status",    // QtShowMasterStatus
  "show open tables",      // QtShowOpenTables
  "show plugins",          // QtShowPlugins
  "show procedure status", // QtShowProcedureStatus
  "show procedure code",   // QtShowProcedureCode
  "show privileges",       // QtShowPrivileges
  "show processlist",      // QtShowProcessList
  "show profile",          // QtShowProfile
  "show profiles",         // QtShowProfiles
  "show slave hosts",      // QtShowSlaveHosts
  "show slave status",     // QtShowSlaveStatus
  "show status",           // QtShowStatus
  "show variables",        // QtShowVariables
  "show table status",     // QtShowTableStatus
  "show tables",           // QtShowTables
  "show triggers",         // QtShowTriggers
  "show warnings",         // QtShowWarnings
  "cache index",           // QtCacheIndex
  "flush",                 // QtFlush
  "kill",                  // QtKill
  "load index",            // QtLoadIndex
  "explain",               // QtExplainTable
  "explain",               // QtExplainStatement
  "help command",          // QtHelp
  "use",                   // QtUse
};

//--------------------------------------------------------------------------------------------------

/**
 * Helper to skip over a single text or identifier. Actually for text there can be more than
 * one instance. Cohered string tokens are concatenated and hence to be handled as a single one.
 *
 * On entry we are on the text/id token. On exit we are on the following token.
 */
bool skip_text_or_identifier(MySQLScanner &scanner) {
  switch (scanner.token_type()) {
    case NCHAR_TEXT:
      scanner.next(true);
      return true;

    case UNDERSCORE_CHARSET:
    case SINGLE_QUOTED_TEXT:
    case DOUBLE_QUOTED_TEXT:
      do {
        scanner.next(true);
      } while (scanner.token_type() == SINGLE_QUOTED_TEXT || scanner.token_type() == DOUBLE_QUOTED_TEXT);

      return true;

    default:
      if (scanner.is_identifier()) {
        scanner.next(true);
        return true;
      }
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Determines the object type from the current position (after one of the DML keywords like
 * alter, create, drop). Default is table if we cannot determine the real one.
 */
std::string object_from_token(MySQLScanner &scanner) {
  // Not all object types support a definer clause, but we are flexible.
  // Skip over it if there's one, regardless.
  if (scanner.token_type() == DEFINER_SYMBOL) {
    scanner.next(true);
    if (scanner.token_type() != EQUAL_OPERATOR)
      return "table";

    // user:
    //   text_or_identifier (AT_SIGN_SYMBOL text_or_identifier | AT_TEXT_SUFFIX)?
    //   | CURRENT_USER_SYMBOL parentheses?
    scanner.next(true);

    if (scanner.token_type() == CURRENT_USER_SYMBOL) {
      scanner.next(true);
      if (scanner.token_type() == OPEN_PAR_SYMBOL) {
        scanner.next(true);
        if (scanner.token_type() == CLOSE_PAR_SYMBOL)
          scanner.next(true);
        else
          return "table";
      }
    } else {
      if (!skip_text_or_identifier(scanner))
        return "table";
      if (scanner.token_type() == AT_SIGN_SYMBOL) {
        scanner.next(true);
        if (!skip_text_or_identifier(scanner)) {
          if (scanner.token_type() != AT_TEXT_SUFFIX)
            return "table";
          scanner.next(true);
        }
      }
    }
  }

  switch (scanner.token_type()) {
    case DATABASE_SYMBOL:
      return "database";

    case LOGFILE_SYMBOL:
      return "logfile group";

    case FUNCTION_SYMBOL: {
      scanner.next(true);
      if (scanner.token_type() == IDENTIFIER)
        scanner.next(true);

      if (scanner.token_type() == RETURNS_SYMBOL)
        return "function udf";

      return "function";
    }

    case AGGREGATE_SYMBOL: // create aggregate function...
      return "function udf";

    case PROCEDURE_SYMBOL:
      return "procedure";

    case SERVER_SYMBOL:
      return "server";

    case TABLESPACE_SYMBOL:
      return "tablespace";

    case EVENT_SYMBOL:
      return "event";

    case VIEW_SYMBOL:
    case ALGORITHM_SYMBOL:
    case OR_SYMBOL:  // create or replace...
    case SQL_SYMBOL: // create sql security... view
      return "view";

    case TRIGGER_SYMBOL:
      return "trigger";

    case ONLINE_SYMBOL: // Old style online option.
    case OFFLINE_SYMBOL: {
      scanner.next(true);
      switch (scanner.token_type()) {
        case UNIQUE_SYMBOL:
        case FULLTEXT_SYMBOL:
        case SPATIAL_SYMBOL:
        case INDEX_SYMBOL:
          return "index";

        default:
          return "table";
      }
    }

    case TEMPORARY_SYMBOL:
      return "table";

    case USER_SYMBOL:
      return "user";

    default:
      return "table";
  }
}

//--------------------------------------------------------------------------------------------------

bool DbSqlEditorContextHelp::get_help_text(const SqlEditorForm::Ref &form, const std::string &topic, std::string &title,
                                           std::string &text) {
  logDebug2("Looking up help topic: %s\n", topic.c_str());
  if (!topic.empty()) {
    try {
      sql::Dbc_connection_handler::Ref conn;
      base::RecMutexLock aux_dbc_conn_mutex(form->ensure_valid_aux_connection(conn));

      // % is interpreted as a wildcard, so we have to escape it. However, we don't use wildcards
      // in any other topic (nor %), so a simple check is enough.
      base::sqlstring query = base::sqlstring("help ?", 0) << (topic == "%" ? "\\%" : topic);
      std::auto_ptr<sql::ResultSet> rs(conn->ref->createStatement()->executeQuery(std::string(query)));
      if (rs->rowsCount() > 0) {
        rs->next();
        title = rs->getString(1);
        text = rs->getString(2);
        return true;
      }
    } catch (...) {
      logDebug2("Exception caught while looking up help text\n");
    }
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Determines a help topic from the given query at the given position.
 */
std::string DbSqlEditorContextHelp::find_help_topic_from_position(const SqlEditorForm::Ref &form,
                                                                  const std::string &query,
                                                                  std::pair<ssize_t, ssize_t> caret) {
  logDebug2("Finding help topic\n");

  // Ensure our translation list has the same size as there are query types.
  g_assert((sizeof(query_type_to_help_topic) / sizeof(query_type_to_help_topic[0])) == QtSentinel);

  // The strategy here is this:
  //   1) Try to find help for the word at the caret position by a simple scan for the given position.
  //      Some cases need parser support already at this stage, so try the parser first for these cases
  //      and then do the local scan.
  //   2) Try to parse the query and if that works find the topic from the closest sub query or the top query.
  //   3) Scan the query from the beginning and try to find enough info to construct a topic.
  std::string topic;

  caret.second++; // Lines are one-based.

  // There are a few cases where we need a parser to decide. That might however fail due to
  // syntax errors. So we check first these special cases if we can solve them by
  // parsing. If not we continue with our normal strategy.
  MySQLRecognizer recognizer(form->server_version(), form->sql_mode(), form->valid_charsets());
  recognizer.parse(query.c_str(), query.length(), true, MySQLParseUnit::PuGeneric);
  MySQLRecognizerTreeWalker walker = recognizer.tree_walker();
  bool found_token = walker.advanceToPosition((int)caret.second, (int)caret.first);
  if (found_token && recognizer.has_errors()) {
    // We can only assume success if the first error is after our position. Otherwise
    // we cannot predict what's in the syntax tree.
    ParserErrorInfo error = recognizer.error_info().front();
    found_token =
      ((int)error.line > caret.second || ((int)error.line == caret.second && (int)error.charOffset > caret.first));
  }

  if (found_token) {
    std::string text = base::tolower(walker.tokenText());
    switch (walker.tokenType()) {
      case CHAR_SYMBOL:
        switch (walker.parentType()) {
          case FUNCTION_CALL_TOKEN:
            return "char function";

          case SHOW_SYMBOL:
            return "show character set";
        }

        if (walker.lookAhead(false) != OPEN_PAR_SYMBOL)
          return "char byte";
        return "char";

      case DISTINCT_SYMBOL:
        if (walker.up() && walker.tokenType() == FUNCTION_CALL_TOKEN) {
          if (walker.lookAhead(true) == COUNT_SYMBOL)
            return "count distinct";
        }
        break;

      case IDENTIFIER:
        if (text == "mrg_myisam") // Synonym for merge engine.
          return "merge";

        if (text == "merge") {
          if (walker.previousType() == EQUAL_OPERATOR) {
            if (!walker.previousSibling())
              break;
          }
          if (walker.previousType() == ENGINES_SYMBOL || walker.previousType() == TYPE_SYMBOL)
            return "merge";
        }
        break;

      case YEAR_SYMBOL:
        if (walker.parentType() == DATA_TYPE_TOKEN)
          return "year data type";
        else
          return "year";
        break;

      default:
        switch (walker.parentType()) {
          case LABEL_TOKEN:
            return "labels";
            break;
        }
        break;
    }
  }

  topic = topic_from_position(form, query, caret);

  // INSERT has a special topic for INSERT SELECT which we can only handle by a parser.
  if (!topic.empty() && topic != "INSERT")
    return topic;

  if (found_token) {
    MySQLQueryType type = walker.getCurrentQueryType();

    if (topic == "INSERT" || type == QtInsert) {
      if (type == QtInsert)
        walker.goToSubQueryStart(); // Go back to the start of this specific subquery.

      // Insert is a major keyword, so it has an own AST tree. Hence go down to the first child
      // for the next scan (which must be non-recursive).
      walker.next();

      // The current position should now be on the INSERT keyword.
      if (walker.advanceToType(SELECT_SYMBOL, false))
        return "insert select";
      return "insert";
    }

    // For flush we have a dedicated help topic (flush query cache), so we need an extra scan.
    if (type != QtUnknown && type != QtAmbiguous && type != QtFlush)
      return query_type_to_help_topic[type];
  } else {
    if (topic == "insert")
      return topic;
  }

  return topic_from_position(form, query, std::make_pair(0, 0));
}

//--------------------------------------------------------------------------------------------------

/**
 * Checks for tokens we know there is no help topic for.
 */
bool is_token_without_topic(unsigned type) {
  switch (type) {
    case DOT_SYMBOL:
    case COMMA_SYMBOL:
    case SEMICOLON_SYMBOL:
    case COLON_SYMBOL:
    case OPEN_PAR_SYMBOL:
    case CLOSE_PAR_SYMBOL:
    case AT_SIGN_SYMBOL:
    case AT_AT_SIGN_SYMBOL:
    case PARAM_MARKER:
    case SINGLE_QUOTED_TEXT:
    case BACK_TICK_QUOTED_ID:
    case DOUBLE_QUOTED_TEXT:
      return true;

    default:
      return false;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Start from the given position in the query and construct a topic if possible.
 * This includes all single word topics, so they don't need to be covered later again.
 */
std::string DbSqlEditorContextHelp::topic_from_position(const SqlEditorForm::Ref &form, const std::string &query,
                                                        std::pair<ssize_t, ssize_t> caret) {
  // TODO: switch to use a parser context instead of the form reference.
  logDebug2("Trying to get help topic at position <%li, %li>, from query: %s...\n", (long)caret.first,
            (long)caret.second, query.substr(0, 300).c_str());

  // First collect all tokens up to the caret position.
  MySQLScanner scanner(query.c_str(), query.length(), true, form->server_version(), form->sql_mode(),
                       form->valid_charsets());

  scanner.seek((int)caret.second, (int)caret.first);

  // If we end up in a whitespace or certain other symbols we jump to the previous token instead.
  if (scanner.token_channel() != 0)
    scanner.previous(true);
  while (scanner.position() > 0 && (scanner.is_number() || is_token_without_topic(scanner.token_type())))
    scanner.previous(true);

  if (scanner.token_type() == ANTLR3_TOKEN_EOF)
    return "";

  // There are special single token topics (or multi tokens which have a single token topic too)
  // which require some extra processing.
  std::string topic = topic_with_single_topic_equivalent(scanner);
  topic = lookup_topic_for_string(form, topic);
  if (!topic.empty())
    return topic;

  // The token at the caret could be part of a multi word topic, so check the special cases.
  switch (scanner.token_type()) {
    case ALTER_SYMBOL:
      scanner.next(true);
      topic = "alter " + object_from_token(scanner);
      break;

    case CREATE_SYMBOL:
      scanner.next(true);
      topic = "create " + object_from_token(scanner);
      break;

    case DROP_SYMBOL:
      scanner.next(true);
      topic = "drop " + object_from_token(scanner);
      break;

    case TRUNCATE_SYMBOL:
      topic = "truncate table";
      break;

    case ANALYZE_SYMBOL: // TODO: analyze and analyse are subject for merge in 5.7.
      topic = "analyze table";
      break;

    case ANALYSE_SYMBOL:
      topic = "procedure analyse";
      break;

    case BEGIN_SYMBOL:
      scanner.next(true);
      switch (scanner.token_type()) {
        case WORK_SYMBOL:
        case ANTLR3_TOKEN_EOF:
          topic = "begin work"; // Transaction statement. Topic does not exist.
          break;

        case STRING_TOKEN:
          topic = "xa start"; // Topic does not exist.
          break;

        default:
          topic = "begin end";
      }
      break;

    case END_SYMBOL:
      scanner.next(true);
      switch (scanner.token_type()) {
        case LOOP_SYMBOL:
          topic = "loop";
          break;

        case REPEAT_SYMBOL:
          topic = "repeat loop";
          break;

        case WHILE_SYMBOL:
          topic = "while loop";
          break;

        case CASE_SYMBOL: // Case statement.
          topic = "case statement";
          break;

        case IF_SYMBOL:
          topic = "if statement";
          break;

        default:
          topic = "begin end"; // Could also be a case expression but it's difficult to determine.
      }
      break;

    case CACHE_SYMBOL:
      scanner.next(true);
      if (scanner.token_type() == INDEX_SYMBOL)
        topic = "cache index";
      break;

    case CHANGE_SYMBOL:
      scanner.next(true);
      if (scanner.token_type() == MASTER_SYMBOL)
        topic = "change master to";
      break;

    case CHECK_SYMBOL:
      topic = "check table";
      break;

    case CHECKSUM_SYMBOL:
      topic = "checksum table";
      break;

    case CURRENT_SYMBOL:
      scanner.previous(true);
      if (scanner.token_type() == GET_SYMBOL)
        topic = "get diagnostics";
      else {
        scanner.previous(true);
        scanner.previous(true);
        if (scanner.token_type() == DIAGNOSTICS_SYMBOL)
          topic = "get diagnostics";
      }
      break;

    case DEALLOCATE_SYMBOL:
      topic = "deallocate prepare";
      break;

    case DECLARE_SYMBOL:
      // There must be an identifier before the next keyword, but we also
      // handle cases where this was not (yet) written.
      scanner.next(true);

      switch (scanner.token_type()) {
        case COMMA_SYMBOL:
        case CONDITION_SYMBOL:
        case CURSOR_SYMBOL:
        case CONTINUE_SYMBOL:
        case EXISTS_SYMBOL:
        case UNDO_SYMBOL:
        case HANDLER_SYMBOL:
          break; // Do nothing.

        default:
          scanner.next(true);
      }

      switch (scanner.token_type()) {
        case CONDITION_SYMBOL:
          topic = "declare condition";
          break;

        case CURSOR_SYMBOL:
          topic = "declare cursor";
          break;

        case CONTINUE_SYMBOL:
        case EXISTS_SYMBOL:
        case UNDO_SYMBOL:
        case HANDLER_SYMBOL:
          topic = "declare handler";
          break;

        default:
          topic = "declare variable";
          break;
      }

      break;

    case DIAGNOSTICS_SYMBOL:
      topic = "get diagnostics";
      break;

    case EXECUTE_SYMBOL:
      topic = "execute statement";
      break;

    case EVENT_SYMBOL:
      topic = ""; // Event is complicated. Get topic from start.
      break;

    case FOREIGN_SYMBOL:
    case KEY_SYMBOL:
    case REFERENCES_SYMBOL:
      topic = "constraint";
      break;

    case GET_SYMBOL:
      scanner.next(true);
      if (scanner.token_type() == CURRENT_SYMBOL)
        scanner.next(true);
      scanner.next(true);
      if (scanner.token_type() == DIAGNOSTICS_SYMBOL)
        topic = "get diagnostics";
      break;

    case INSTALL_SYMBOL:
      topic = "install plugin";
      break;

    case LOAD_SYMBOL:
      scanner.next(true);
      switch (scanner.token_type()) {
        case DATA_SYMBOL:
          topic = "load data";
          break;

        case XML_SYMBOL:
          topic = "load xml";
          break;

        case INDEX_SYMBOL:
          topic = "load index";
          break;
      }

      break;

    case NOT_SYMBOL:
      scanner.next(true);
      switch (scanner.token_type()) {
        case BETWEEN_SYMBOL:
          topic = "not between";
          break;

        case IN_SYMBOL:
          topic = "not in";
          break;

        case LIKE_SYMBOL:
          topic = "not like";
          break;

        case REGEXP_SYMBOL:
          topic = "not regexp";
          break;
      }

      break;

    case ON_SYMBOL:
      scanner.next(true);
      switch (scanner.token_type()) {
        case UPDATE_SYMBOL:
        case DELETE_SYMBOL:
          topic = "constraint";
          break;
      }
      break;

    case OPTIMIZE_SYMBOL:
      scanner.next(true);
      switch (scanner.token_type()) {
        case LOCAL_SYMBOL:
        case NO_WRITE_TO_BINLOG_SYMBOL:
        case TABLE_SYMBOL:
          topic = "optimize table";
          break;

        default: // optimize partition... (which is part of a larger statement).
          topic = "";
          break;
      }
      break;

    case PROCEDURE_SYMBOL:
      scanner.previous(true);
      if (scanner.token_type() == ANTLR3_TOKEN_INVALID) // Is this the first token?
        topic = "procedure analyse";
      else
        topic = ""; // Anything else with another leading token. Look them up for topic construction.
      break;

    case PURGE_SYMBOL:
      topic = "purge binary logs";
      break;

    case RENAME_SYMBOL:
      scanner.next(true);
      switch (scanner.token_type()) {
        case TABLE_SYMBOL:
        case TABLES_SYMBOL:
          topic = "rename table";
          break;

        case USER_SYMBOL:
          topic = "rename user";
          break;
      }

      break;

    case REPAIR_SYMBOL:
      topic = "repair table";
      break;

    case SOUNDS_SYMBOL:
      topic = "sounds like";
      break;

    case START_SYMBOL:
      topic = "start ";
      scanner.next(true);
      topic += scanner.token_text();

      break;

    case UNINSTALL_SYMBOL:
      topic = "uninstall plugin";
      break;

    case MATCH_SYMBOL:
      scanner.next(true);
      if (scanner.token_type() == OPEN_PAR_SYMBOL)
        topic = "match against";

      break;

    case AGAINST_SYMBOL:
      topic = "match against";
      break;

    case DELAYED_SYMBOL:
      scanner.previous(true);
      switch (scanner.token_type()) {
        case INSERT_SYMBOL:
          topic = "insert delayed";
          break;

        case REPLACE_SYMBOL:
          topic = "replace";
          break;

        default:
          topic = "";
      }
      break;

    case SESSION_SYMBOL:
      scanner.previous(true);
      switch (scanner.token_type()) {
        case SHOW_SYMBOL:
          topic = "show";
          break;

        case SET_SYMBOL:
          topic = "set";
          break;
      }
      break;
  }

  return topic; // Could be empty. This is checked at the caller level.
}

//--------------------------------------------------------------------------------------------------

/**
 * Handles the search for a single word topic, a topic which requires text transformation or
 * a topic which has several variants and one of them is a single word topic.
 */
std::string DbSqlEditorContextHelp::topic_with_single_topic_equivalent(MySQLScanner &scanner) {
  logDebug2("Trying single word topics\n");

  std::string topic = base::tolower(scanner.token_text());

  // First some text transformations, which are not based on (mysql) tokens.
  if (topic == "mbr") // MBR is not a syntax element, but just a topic part.
    return "mbr definitions";
  if (topic == "wkt") // Same for wkt.
    return "wkt definition";

  // Synonyms.
  if (topic == "from_base64")
    return "from_base64()";
  if (topic == "to_base64")
    return "to_base64()";
  if (topic == "geometryfromtext")
    return "geomfromtext";
  if (topic == "geometryfromwkb")
    return "geomfromwkb";

  switch (scanner.token_type()) {
    case MINUS_OPERATOR:
      scanner.next(false);
      if (scanner.token_channel() != 0 || scanner.is_operator()) // whitespace or operator follows
        topic = "- binary";
      else
        topic = "- unary";
      break;

    case ASSIGN_OPERATOR:
      topic = "assign-value"; // "assign-equal" contains nearly the same help text, not that for =, as one would expect.
      break;

    case BETWEEN_SYMBOL:
      scanner.previous(true);
      if (scanner.token_type() == NOT_SYMBOL)
        topic = "not between";
      else
        topic = "between and";
      break;

    case BINARY_SYMBOL:
      scanner.previous(true);
      if (scanner.token_type() == SHOW_SYMBOL) {
        topic = "show binary logs";
        break;
      }

      scanner.next(true);
      scanner.next(true);
      if (scanner.token_type() == OPEN_PAR_SYMBOL)
        topic = "binary";
      else
        topic = "binary operator";
      break;

    case BINLOG_SYMBOL:
      scanner.previous(true);
      if (scanner.token_type() == SHOW_SYMBOL)
        topic = "show binlog events";
      break;

    case CASE_SYMBOL:
      topic = "case statement";
      break;

    case NATIONAL_SYMBOL:
      topic = "char";
      break;

    case CHAR_SYMBOL:
      scanner.previous(true);
      if (scanner.token_type() == SHOW_SYMBOL) {
        topic = "show character set";
        break;
      }
      scanner.next(true);
      scanner.next(true);
      if (scanner.token_type() != OPEN_PAR_SYMBOL)
        topic = "char byte";
      else
        topic = ""; // We need a parser to check for "char function" vs "char()"
      break;

    case COLLATION_SYMBOL:
      scanner.previous(true);
      if (scanner.token_type() == SHOW_SYMBOL)
        topic = "show collation";
      break;

    case COUNT_SYMBOL:
      scanner.previous(true);
      if (scanner.token_type() == SHOW_SYMBOL) // show count(*) ...
      {
        topic = ""; // Let the parser or scan from start do the actual work.
        break;
      }

      scanner.next(true);
      scanner.next(true);
      if (scanner.token_type() == OPEN_PAR_SYMBOL) {
        scanner.next(true);
        if (scanner.token_type() == DISTINCT_SYMBOL)
          topic = "count distinct";
        else
          topic = "count";
      } else
        topic = "count";
      break;

    case DATABASE_SYMBOL:
      scanner.next(true);
      if (scanner.token_type() == OPEN_PAR_SYMBOL)
        topic = "database"; // The function database().
      else
        topic = ""; // create database, drop database etc. Need to parse.
      break;

    case DATE_SYMBOL:
      scanner.next(true);
      if (scanner.token_type() == OPEN_PAR_SYMBOL)
        topic = "date function";
      else
        topic = "date";
      break;

    case DELETE_SYMBOL:
      scanner.previous(true);
      if (scanner.token_type() == ON_SYMBOL)
        topic = "constraint"; // Part of a foreign key definition (on update/delete);
      break;

    case DOUBLE_SYMBOL:
      scanner.next(true);
      if (scanner.token_type() == PRECISION_SYMBOL)
        topic = "double precision";
      else
        topic = "double";
      break;

    case PRECISION_SYMBOL:
      topic = "double precision";
      break;

    case REAL_SYMBOL:
      topic = "double";
      break;

    case FLUSH_SYMBOL:
      scanner.next(true);
      if (scanner.token_type() == LOCAL_SYMBOL || scanner.token_type() == NO_WRITE_TO_BINLOG_SYMBOL)
        scanner.next(true);
      if (scanner.token_type() == QUERY_SYMBOL)
        topic = "flush query cache";
      else
        topic = "flush";
      break;

    case GEOMETRY_SYMBOL:
      topic = "geometry hierarchy"; // There's also a "geometry" topic, but that doesn't contain much.
      break;

    case HELP_SYMBOL:
      topic = "help command"; // There's also "help statement" which tells essentially the same but shorter.
      break;

    case IF_SYMBOL:
      scanner.next(true);
      if (scanner.token_type() == OPEN_PAR_SYMBOL)
        topic = "if function";
      else
        topic = "if statement";
      break;

    case IS_SYMBOL:
      scanner.next(true);
      switch (scanner.token_type()) {
        case NOT_SYMBOL:
          scanner.next(true);
          if (scanner.token_type() == NULL_SYMBOL)
            topic = "is not null";
          else
            topic = "is not";
          break;

        case NULL_SYMBOL:
          topic = "is null";
          break;

        default:
          topic = "is";
          break;
      }
      break;

    case IN_SYMBOL:
      scanner.previous(true);
      if (scanner.token_type() == NOT_SYMBOL)
        topic = "not in";
      break;

    case LIKE_SYMBOL:
      scanner.previous(true);
      switch (scanner.token_type()) {
        case NOT_SYMBOL:
          topic = "not like";
          break;

        case SOUNDS_SYMBOL:
          topic = "sounds like";
          break;
      }
      break;

    case OPEN_SYMBOL:
      scanner.previous(true);
      if (scanner.token_type() == SHOW_SYMBOL)
        topic = "show open tables";
      break;

    case PASSWORD_SYMBOL:
      scanner.previous(true);
      if (scanner.token_type() == SET_SYMBOL)
        topic = "set password";
      break;

    case REPEAT_SYMBOL:
      scanner.next(true);
      if (scanner.token_type() == OPEN_PAR_SYMBOL)
        topic = "repeat function";
      else
        topic = "repeat loop";

      break;

    case REPLACE_SYMBOL:
      scanner.next(true);
      if (scanner.token_type() == OPEN_PAR_SYMBOL)
        topic = "replace function";
      else
        topic = "replace";
      break;

    case RESET_SYMBOL:
      scanner.next(true);
      switch (scanner.token_type()) {
        case MASTER_SYMBOL:
          topic = "reset master";
          break;

        case SLAVE_SYMBOL:
          topic = "reset slave";
          break;

        default:
          topic = "reset";
      }
      break;

    case SET_SYMBOL:
      scanner.next(true);
      if (scanner.token_type() == GLOBAL_SYMBOL)
        scanner.next(true);

      switch (scanner.token_type()) {
        case OPEN_PAR_SYMBOL:
          topic = "set data type";
          break;

        case PASSWORD_SYMBOL:
          topic = "set password";
          break;

        case IDENTIFIER:
          if (base::tolower(scanner.token_text()) == "sql_slave_skip_counter")
            topic = "set global sql_slave_skip_counter";
          else
            topic += " " + scanner.token_text();
          break;

        default:
          topic = "set";
      }
      break;

    case SHOW_SYMBOL:
      scanner.next(true);

      // Not all queries allow these tokens, but to be flexible we skip them anyway.
      if (scanner.token_type() == FULL_SYMBOL || scanner.token_type() == GLOBAL_SYMBOL ||
          scanner.token_type() == LOCAL_SYMBOL || scanner.token_type() == SESSION_SYMBOL)
        scanner.next(true);

      switch (scanner.token_type()) {
        case AUTHORS_SYMBOL:
        case COLLATION_SYMBOL:
        case COLUMNS_SYMBOL:
        case CONTRIBUTORS_SYMBOL:
        case DATABASES_SYMBOL:
        case ENGINE_SYMBOL:
        case ENGINES_SYMBOL:
        case ERRORS_SYMBOL:
        case EVENTS_SYMBOL:
        case GRANTS_SYMBOL:
        case PRIVILEGES_SYMBOL:
        case PROCESSLIST_SYMBOL:
        case PROFILE_SYMBOL:
        case PROFILES_SYMBOL:
        case PLUGINS_SYMBOL:
        case WARNINGS_SYMBOL:
        case STATUS_SYMBOL:
        case VARIABLES_SYMBOL:
        case TABLES_SYMBOL:
        case TRIGGERS_SYMBOL:
          topic += " " + scanner.token_text();
          break;

        case BINARY_SYMBOL:
          topic += " binary logs";
          break;

        case RELAYLOG_SYMBOL:
          topic += " relaylog events";
          break;

        case BINLOG_SYMBOL:
          topic += " binlog events";
          break;

        case CHAR_SYMBOL:
          topic += " character set";
          break;

        case CREATE_SYMBOL:
          scanner.next(true);
          topic += " create " + scanner.token_text();
          break;

        case FUNCTION_SYMBOL:
          scanner.next(true);
          topic += " function " + scanner.token_text();
          break;

        case INDEX_SYMBOL:
        case INDEXES_SYMBOL:
        case KEYS_SYMBOL:
          topic += " index";
          break;

        case IN_SYMBOL:
          scanner.previous(true);
          if (scanner.token_type() == NOT_SYMBOL)
            topic = "not in";
          break;

        case INNODB_SYMBOL:
          topic += " innodb status";
          break;

        case MASTER_SYMBOL:
          topic += " master status";
          break;

        case OPEN_SYMBOL:
          topic += " open tables";
          break;

        case PLUGIN_SYMBOL:
          topic += " plugins";
          break;

        case PROCEDURE_SYMBOL:
          scanner.next(true);
          topic += " procedure " + scanner.token_text();
          break;

        case REGEXP_SYMBOL:
          scanner.previous(true);
          if (scanner.token_type() == NOT_SYMBOL)
            topic = "not regexp";
          break;

        case STORAGE_SYMBOL:
          topic += " engines";
          break;

        case SLAVE_SYMBOL:
          scanner.next(true);
          topic += " slave " + scanner.token_text();
          break;

        case TABLE_SYMBOL:
          topic += " table status";
          break;

        case COUNT_SYMBOL: // show count(*) ..., use show warnings as default.
          topic += " warnings";
          scanner.next(true);
          if (scanner.token_type() != OPEN_PAR_SYMBOL)
            break;
          scanner.next(true);
          if (scanner.token_type() != MULT_OPERATOR)
            break;
          scanner.next(true);
          if (scanner.token_type() != CLOSE_PAR_SYMBOL)
            break;

          scanner.next(true);
          if (scanner.token_type() == ERRORS_SYMBOL)
            topic += " errors";
          break;
      }
      break;

    case TIME_SYMBOL:
      scanner.next(true);
      if (scanner.token_type() == OPEN_PAR_SYMBOL)
        topic = "time function";
      else
        topic = "time"; // This is not entirely correct. Starting with 5.6 TIME can have a precision value
      // also specified within parentheses. This makes differentiating between
      // time function and time value impossible without outer context
      // (which is only available if we can parse the query).
      break;

    case TIMESTAMP_SYMBOL:
      scanner.next(true);
      if (scanner.token_type() == OPEN_PAR_SYMBOL)
        topic = "timestamp function";
      else
        topic = "timestamp"; // The same applies here as for time.
      break;

    case TRUE_SYMBOL:
    case FALSE_SYMBOL:
      topic = "true false";
      break;

    case TRUNCATE_SYMBOL:
      scanner.next(true);
      switch (scanner.token_type()) {
        case OPEN_PAR_SYMBOL:
          // topic = "truncate";
          break;

        case TABLE_SYMBOL:
          topic += " table";
          break;

        case PARTITION_SYMBOL:
          topic += " partition";
          break;
      }
      break;

    case YEAR_SYMBOL:
      scanner.next(true);
      if (scanner.token_type() == OPEN_PAR_SYMBOL)
        topic = "year"; // Similar to time and timestamp this is not entirely correct. See TIME for more info.
      else
        topic = "year data type";
      break;

    case INSERT_SYMBOL:
      scanner.next(true);
      switch (scanner.token_type()) {
        case DELAYED_SYMBOL:
          topic = "insert delayed";
          break;

        case OPEN_PAR_SYMBOL: // Insert function or grant insert (...).
          scanner.reset();
          if (scanner.token_channel() != ANTLR3_TOKEN_DEFAULT_CHANNEL)
            scanner.next(true);
          if (scanner.token_type() == GRANTS_SYMBOL) // Could be wrong if this statement is part of a compound statement
                                                     // (e.g. in triggers or events).
            topic = "grant";
          else
            topic = "insert function";
          break;

        case ON_SYMBOL:
          topic = ""; // Probably part of a trigger definition. Check somewhere else.
          break;

        default:
          topic = "insert";
          break;
      }

      break;

    case UPDATE_SYMBOL:
      scanner.previous(true);
      if (scanner.token_type() == ON_SYMBOL)
        topic = "constraint"; // Part of a foreign key definition (on update/delete);
      break;

    case USER_SYMBOL:
      scanner.next(true);
      if (scanner.token_type() != OPEN_PAR_SYMBOL)
        topic = ""; // If not the user() function then more parsing is required.
      break;

    case PREPARE_SYMBOL:
      scanner.previous(true);
      switch (scanner.token_type()) {
        case DEALLOCATE_SYMBOL:
        case DROP_SYMBOL:
          topic = "deallocate prepare";
          break;

        case XA_SYMBOL:
          topic = "xa prepare"; // This topic doesn't exist (yet).
          break;
      }

      // Otherwise the set topic is ok.
      break;

    case HANDLER_SYMBOL:
      scanner.previous(true);
      switch (scanner.token_type()) {
        case CONDITION_SYMBOL:
        case EXIT_SYMBOL:
        case UNDO_SYMBOL:
        case DECLARE_SYMBOL:
          topic = ""; // Handling below.
          break;
      }

      break;

    case REGEXP_SYMBOL:
      scanner.previous(true);
      if (scanner.token_type() == NOT_SYMBOL)
        topic = "not regexp";
      break;

    case THEN_SYMBOL:
    case ELSE_SYMBOL: // ELSE and THEN can also be part of a CASE expression/statement.
                      // However this is too complicated to figure out here, so we assume IF
                      // and the user has to set the caret on the CASE keyword to get its help.
    case ELSEIF_SYMBOL:
      topic = "if statement";
      break;

    case IDENTIFIER:
      if (topic == "sql_log_bin") {
        scanner.previous(true);
        if (scanner.token_type() == SET_SYMBOL)
          topic = "set sql_log_bin";
        else
          topic = "";
        break;
      }

      scanner.next(true);
      if (scanner.token_type() == COLON_SYMBOL) {
        topic = "labels";
        break;
      }

      if (topic == "x" || topic == "y") // X() and Y() check.
      {
        if (scanner.token_type() != OPEN_PAR_SYMBOL)
          topic = "";
        break;
      }

      break;
  }

  return topic;
}

//--------------------------------------------------------------------------------------------------
