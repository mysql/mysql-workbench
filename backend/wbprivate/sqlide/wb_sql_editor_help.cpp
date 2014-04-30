/* 
 * Copyright (c) 2012, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "stdafx.h"

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
std::string DbSqlEditorContextHelp::lookup_topic_for_string(const SqlEditorForm::Ref &form,
  std::string topic)
{
  if (!topic.empty())
  {
    // HELP supports wildcards. We don't use them but if a single % is looked up we get all topics as result.
    // Since we know there's a topic for '%' we just return it.
    if (topic != "%")
    {
      try
      {
        log_debug2("Validating topic: %s\n", topic.c_str());

        sql::Dbc_connection_handler::Ref conn;
        base::RecMutexLock aux_dbc_conn_mutex = form->ensure_valid_aux_connection(conn);

        base::sqlstring query = base::sqlstring("help ?", 0) << topic;
        std::auto_ptr<sql::ResultSet> rs(conn->ref->createStatement()->executeQuery(std::string(query)));
        if (rs->rowsCount() == 1)
        {
          rs->next();
          topic = rs->getString(1);
        }
        else
          topic = "";
      }
      catch (...)
      {
        // It's an exception but not relevant for the rest of the code. We just did not get a topic
        // (maybe the server was busy or not reachable).
        log_debug2("Exception caught while looking up topic\n");
      }
    }
  }

  return topic;
}

//--------------------------------------------------------------------------------------------------

static std::string query_type_to_help_topic[] = {
  "",                     // QtUnknown
  "",                     // QtAmbiguous
  "alter database",       // QtAlterDatabase
  "alter logfile group",  // QtAlterLogFileGroup - not there yet
  "alter function",       // QtAlterFunction
  "alter procedure",      // QtAlterProcedure
  "alter server",         // QtAlterServer
  "alter table",          // QtAlterTable
  "alter tablespace",     // QtAlterTableSpace - not there yet
  "alter event",          // QtAlterEvent
  "alter view",           // QtAlterView
  "create table",         // QtCreateTable
  "create index",         // QtCreateIndex
  "create database",      // QtCreateDatabase
  "create event",         // QtCreateEvent
  "create view",          // QtCreateView
  "create procedure",     // QtCreateProcedure
  "create function",      // QtCreateFunction
  "create function udf",  // QtCreateUdf
  "create trigger",       // QtCreateTrigger
  "create logfile group", // QtCreateLogFileGroup
  "create server",        // QtCreateServer
  "create tablespace",    // QtCreateTableSpace - not there yet
  "drop database",        // QtDropDatabase
  "drop event",           // QtDropEvent
  "drop function",        // QtDropFunction, including UDF
  "drop procedure",       // QtDropProcedure
  "drop index",           // QtDropIndex
  "drop logfile group",   // QtDropLogfileGroup - not there yet
  "drop server",          // QtDropServer
  "drop table",           // QtDropTable
  "drop tablespace",      // QtDropTablespace - not there yet
  "drop trigger",         // QtDropTrigger
  "drop view",            // QtDropView
  "rename table",         // QtRenameTable
  "truncate table",       // QtTruncateTable
  "call",                 // QtCall
  "delete",               // QtDelete
  "do",                   // QtDo
  "handler",              // QtHandler,
  "insert",               // QtInsert
  "load data",            // QtLoadData
  "load xml",             // QtLoadXML
  "replace",              // QtReplace
  "select",               // QtSelect
  "update",               // QtUpdate
  "partition",            // QtPartition (TODO: must be handled individually for each statement type it is used in)
  "start transaction",    // QtStartTransaction
  "start transaction",    // QtBeginWork,
  "commit",               // QtCommit (automatically detours to start transaction)
  "start transaction",    // QtRollbackWork,
  "start transaction",    // QtSetAutoCommit
  "start transaction",    // QtSetTransaction
  "savepoint",            // QtSavepoint
  "savepoint",            // QtReleaseSavepoint
  "savepoint",            // QtRollbackSavepoint
  "lock",                 // QtLock
  "unlock",               // QtUnlock
  "xa",                   // QtXA - not there yet
  "purge binary logs",    // QtPurge
  "change master to",     // QtChangeMaster
  "reset",                // QtReset
  "reset master",         // QtResetMaster
  "reset slave",          // QtResetSlave
  "start slave",          // QtStartSlave
  "stop slave",           // QtStopSlave
  "load data master",     // QtLoadDataMaster - not there yet
  "load table master",    // QtLoadTableMaster - not there yet
  "prepare",              // QtPrepare
  "execute statement",    // QtExecute
  "deallocate prepare",   // QtDeallocate
  "alter user",           // QtAlterUser
  "create user",          // QtCreateUser
  "drop user",            // QtDropUser
  "grant",                // QtGrantProxy
  "grant",                // QtGrant
  "rename user",          // QtRenameUser
  "revoke",               // QtRevokeProxy
  "revoke",               // QtRevoke
  "analyze table",        // QtAnalyzeTable
  "check table",          // QtCheckTable
  "checksum table",       // QtChecksumTable
  "optimze table",        // QtOptimizeTable
  "repair table",         // QtRepairTable
  "backup table",         // QtBackUpTable
  "restore table",        // QtRestoreTable
  "install plugin",       // QtInstallPlugin
  "uninstall plugin",     // QtUninstallPlugin
  "set",                  // QtSet
  "set password",         // QtSetPassword
  "show",                 // QtShow
  "show authors",         // QtShowAuthors
  "show binary logs",     // QtShowBinaryLogs
  "show binlog events",   // QtShowBinlogEvents
  "show relaylog events", // QtShowRelaylogEvents
  "show character set",   // QtShowCharset
  "show collation",       // QtShowCollation
  "show columns",         // QtShowColumns
  "show contributors",    // QtShowContributors
  "show create database", // QtShowCreateDatabase
  "show create event",    // QtShowCreateEvent
  "show create function",  // QtShowCreateFunction
  "show create procedure", // QtShowCreateProcedure
  "show create table",    // QtShowCreateTable
  "show create trigger",  // QtShowCreateTrigger
  "show create view",     // QtShowCreateView
  "show databases",       // QtShowDatabases
  "show engine",          // QtShowEngineStatus
  "show engines",         // QtShowStorageEngines
  "show errors",          // QtShowErrors
  "show events",          // QtShowEvents
  "show function code",   // QtShowFunctionCode
  "show function status", // QtShowFunctionStatus
  "show grants",          // QtShowGrants
  "show index",           // QtShowIndexes
  "show innodb status",   // QtShowInnoDBStatus
  "show master status",   // QtShowMasterStatus
  "show open tables",     // QtShowOpenTables
  "show plugins",         // QtShowPlugins
  "show procedure status", // QtShowProcedureStatus
  "show procedure code",  // QtShowProcedureCode
  "show privileges",      // QtShowPrivileges
  "show processlist",    // QtShowProcessList
  "show profile",         // QtShowProfile
  "show profiles",        // QtShowProfiles
  "show slave hosts",     // QtShowSlaveHosts
  "show slave status",    // QtShowSlaveStatus
  "show status",          // QtShowStatus
  "show variables",       // QtShowVariables
  "show table status",    // QtShowTableStatus
  "show tables",          // QtShowTables
  "show triggers",        // QtShowTriggers
  "show warnings",        // QtShowWarnings
  "cache index",          // QtCacheIndex
  "flush",                // QtFlush
  "kill",                 // QtKill
  "load index",           // QtLoadIndex
  "explain",              // QtExplainTable
  "explain",              // QtExplainStatement
  "help command",         // QtHelp
  "use",                  // QtUse
};

//--------------------------------------------------------------------------------------------------

/**
 * Returns the first of already scanned tokens on the default channel.
 * It does not try to fill new tokens if there aren't any.
 */
MySQLToken get_first_real_token(std::vector<MySQLToken> &tokens)
{
  for (size_t i = 0; i < tokens.size(); i++)
  {
    if (tokens[i].type == ANTLR3_TOKEN_EOF || tokens[i].channel == 0)
      return tokens[i];
  }

  MySQLToken result;
  result.type = ANTLR3_TOKEN_INVALID;
  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the previous token (before index) that is on the default channel.
 */
MySQLToken get_previous_real_token(std::vector<MySQLToken> &tokens, size_t &index)
{
  if (index > 1)
  {
    --index; // On enter it points to the position after the current token.
    while (index-- > 0)
    {
      if (tokens[index].channel == 0)
        return tokens[index++];
    }
  }

  MySQLToken result;
  result.type = ANTLR3_TOKEN_INVALID;
  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the next token (given by index) in the input. If the token list has not enough entries
 * for the given index then more tokens are scanned by the given scanner and stored in the token list
 * until there are enough (or nothing more is available).
 */
MySQLToken get_next_real_token(MySQLScanner &scanner, std::vector<MySQLToken> &tokens, size_t &index)
{
  // On enter index already points to the next token (or the end of the list).
  if (index < tokens.size() && tokens[index].channel == 0)
    return tokens[index++];
  
  MySQLToken result;
  result.type = ANTLR3_TOKEN_INVALID;
  if (tokens.empty())
    return result;

  do
  {
    if (index == tokens.size())
    {
      result = scanner.next_token();
      tokens.push_back(result);
    }
    result = tokens[index++];
  } while (result.type != ANTLR3_TOKEN_EOF && result.channel != 0);

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper to skip over a single text or identifier. Actually for text there can be more than
 * one instance. Cohered string tokens are concatenated and hence to be handled as a single one.
 * 
 * On entry we are on the text/id token. On exit we are on the following token.
 */
bool skip_text_or_identifier(std::vector<MySQLToken> &tokens, MySQLScanner &scanner, size_t &index)
{
  MySQLToken token = tokens[index - 1];
  switch (token.type)
  {
  case NCHAR_TEXT:
    get_next_real_token(scanner, tokens, index);
    return true;

  case UNDERSCORE_CHARSET:
  case SINGLE_QUOTED_TEXT:
  case DOUBLE_QUOTED_TEXT:
    do 
    {
      token = get_next_real_token(scanner, tokens, index);
    } while (token.type == SINGLE_QUOTED_TEXT || token.type == DOUBLE_QUOTED_TEXT);

    return true;

  default:
    if (scanner.is_identifier(token.type))
    {
      get_next_real_token(scanner, tokens, index);
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
std::string object_from_token(std::vector<MySQLToken> &tokens, MySQLScanner &scanner, size_t &index)
{
  // Not all object types support a definer clause, but we are flexible.
  // Skip over it if there's one, regardless.
  MySQLToken token = tokens[index - 1];
  if (token.type == DEFINER_SYMBOL)
  {
    token = get_next_real_token(scanner, tokens, index);
    if (token.type != EQUAL_OPERATOR)
      return "table";

    // user:
    //   text_or_identifier (AT_SIGN_SYMBOL text_or_identifier)?
    //   | CURRENT_USER_SYMBOL parentheses?
    //   
    token = get_next_real_token(scanner, tokens, index);

    if (token.type == CURRENT_USER_SYMBOL)
    {
      token = get_next_real_token(scanner, tokens, index);
      if (token.type == OPEN_PAR_SYMBOL)
      {
        token = get_next_real_token(scanner, tokens, index);
        if (token.type == CLOSE_PAR_SYMBOL)
          token = get_next_real_token(scanner, tokens, index);
        else
          return "table";
      }
    }
    else
    {
      if (!skip_text_or_identifier(tokens, scanner, index))
        return "table";
      token = tokens[index - 1];
      if (token.type == AT_SIGN_SYMBOL)
      {
        token = get_next_real_token(scanner, tokens, index);
        if (!skip_text_or_identifier(tokens, scanner, index))
          return "table";
      }
    }
    token = tokens[index - 1];
  }

  switch (token.type)
  {
  case DATABASE_SYMBOL:
    return "database";

  case LOGFILE_SYMBOL:
    return "logfile group";

  case FUNCTION_SYMBOL:
    {
      MySQLToken token = get_next_real_token(scanner, tokens, index);
      if (token.type == IDENTIFIER)
        token = get_next_real_token(scanner, tokens, index);

      if (token.type == RETURNS_SYMBOL)
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
  case OR_SYMBOL:      // create or replace...
  case SQL_SYMBOL:     // create sql security... view
    return "view";

  case TRIGGER_SYMBOL:
    return "trigger";

  case ONLINE_SYMBOL:  // Old style online option.
  case OFFLINE_SYMBOL:
    {
      MySQLToken token = get_next_real_token(scanner, tokens, index);
      switch (token.type)
      {
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
  std::string &text)
{
  log_debug2("Looking up help topic: %s\n", topic.c_str());
  if (!topic.empty())
  {
    try
    {
      sql::Dbc_connection_handler::Ref conn;
      base::RecMutexLock aux_dbc_conn_mutex(form->ensure_valid_aux_connection(conn));

      // % is interpreted as a wildcard, so we have to escape it. However, we don't use wildcards
      // in any other topic (nor %), so a simple check is enough.
      base::sqlstring query = base::sqlstring("help ?", 0) << (topic == "%" ? "\\%" : topic);
      std::auto_ptr<sql::ResultSet> rs(conn->ref->createStatement()->executeQuery(std::string(query)));
      if (rs->rowsCount() > 0)
      {
        rs->next();
        title = rs->getString(1);
        text = rs->getString(2);
        return true;
      }
    }
    catch (...)
    {
      log_debug2("Exception caught while looking up topic\n");
    }
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Determines a help topic from the given query at the given position.
 */
std::string DbSqlEditorContextHelp::find_help_topic_from_position(const SqlEditorForm::Ref &form,
  const std::string &query, std::pair<int, int> caret)
{
  log_debug2("Finding help topic\n");
  
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
  MySQLRecognizer recognizer(query.c_str(), query.length(), true, form->server_version(),
    form->sql_mode(), form->valid_charsets());
  MySQLRecognizerTreeWalker walker = recognizer.tree_walker();
  bool found_token = walker.advance_to_position(caret.second, caret.first);
  if (found_token && recognizer.has_errors())
  {
    // We can only assume success if the first error is after our position. Otherwise
    // we cannot predict what's in the syntax tree.
    MySQLParserErrorInfo error = recognizer.error_info().front();
    found_token = ((int)error.line > caret.second ||
      (int)error.line == caret.second && (int)error.offset > caret.first);
  }

  if (found_token)
  {
    std::string text = base::tolower(walker.token_text());
    switch (walker.token_type())
    {
      case CHAR_SYMBOL:
        switch (walker.parent_type())
        {
        case FUNCTION_CALL_TOKEN:
          return "char function";

        case SHOW_SYMBOL:
          return "show character set";
        }

        if (walker.look_ahead(false) != OPEN_PAR_SYMBOL)
          return "char byte";
        return "char";

      case DISTINCT_SYMBOL:
        if (walker.up() && walker.token_type() == FUNCTION_CALL_TOKEN)
        {
          if (walker.look_ahead(true) == COUNT_SYMBOL)
            return "count distinct";
        }
        break;

      case IDENTIFIER:
        if (text == "mrg_myisam") // Synonym for merge engine.
          return "merge";

        if (text == "merge")
        {
          if (walker.previous_type() == EQUAL_OPERATOR)
          {
            if (!walker.previous_sibling())
              break;
          }
          if (walker.previous_type() == ENGINES_SYMBOL || walker.previous_type() == TYPE_SYMBOL)
            return "merge";
        }
        break;

      case YEAR_SYMBOL:
        if (walker.parent_type() == DATA_TYPE_TOKEN)
          return "year data type";
        else
          return "year";
        break;

      default:
        switch (walker.parent_type())
        {
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

  if (found_token)
  {
    MySQLQueryType type = walker.get_current_query_type();

    if (topic == "INSERT" || type == QtInsert)
    {
      if (type == QtInsert)
        walker.go_to_subquery_start(); // Go back to the start of this specific subquery.

      // Insert is a major keyword, so it has an own AST tree. Hence go down to the first child
      // for the next scan (which must be non-recursive).
      walker.next();
        
      // The current position should now be on the INSERT keyword.
      if (walker.advance_to_type(SELECT_SYMBOL, false))
        return "insert select";
      return "insert";
    }

    // For flush we have a dedicated help topic (flush query cache), so we need an extra scan.
    if (type != QtUnknown && type != QtAmbiguous && type != QtFlush)
      return query_type_to_help_topic[type];
  }
  else
  {
    if (topic == "insert")
      return topic;
  }

  return topic_from_position(form, query, std::make_pair(0, 0));
}

//--------------------------------------------------------------------------------------------------

/**
 * A stripped down variant as used in the scanner to determine if the given type is an operator
 * for which no help topic exists. We don't do topic lookup though, but use a fixed list as this
 * won't change.
 */
bool is_operator_without_topic(unsigned type)
{
  switch (type)
  {
    case DOT_SYMBOL:
    case COMMA_SYMBOL:
    case SEMICOLON_SYMBOL:
    case COLON_SYMBOL:
    case OPEN_PAR_SYMBOL:
    case CLOSE_PAR_SYMBOL:
    case AT_SIGN_SYMBOL:
    case AT_AT_SIGN_SYMBOL:
    case PARAM_MARKER:
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
std::string DbSqlEditorContextHelp::topic_from_position(const SqlEditorForm::Ref &form,
  const std::string &query, std::pair<int, int> caret)
{
  // Don't log the entire query. That can really take a moment with large queries.
  log_debug2("Trying to get help topic from a position\n");
  
  // First collect all tokens up to the caret position.
  MySQLScanner scanner(query.c_str(), query.length(), true, form->server_version(), form->sql_mode(),
    form->valid_charsets());
  std::vector<MySQLToken> tokens;

  MySQLToken token;
  do 
  {
    token = scanner.next_token();
    tokens.push_back(token);
    if (token.type == ANTLR3_TOKEN_EOF)
      break;
    if ((int)token.line > caret.second ||
      (int)token.line == caret.second && (int)(token.position + token.text.size()) > caret.first)
      break;
  } while (true);

  // If we end up in a whitespace or certain other symbols we jump to the previous token instead.
  size_t index = tokens.size() - 1;
  while (index > 0)
  {
    if (token.channel != 0 || scanner.is_number(token.type) || is_operator_without_topic(token.type))
    {
      token = tokens[--index];
      continue;
    }
    break;
  }

  if (token.type == ANTLR3_TOKEN_EOF)
    return "";

  // Increase the index to point to the next token or the end of the list as this is what
  // we act upon if we need another token.
  index++;

  // There are special single token topics (or multi tokens which have a single token topic too)
  // which require some extra processing.
  std::string topic = topic_with_single_topic_equivalent(token, scanner, tokens, index);
  topic = lookup_topic_for_string(form, topic);
  if (!topic.empty())
    return topic;

  // The token at the caret could be part of a multi word topic, so check the special cases.
  switch (token.type)
  {
  case ALTER_SYMBOL:
    get_next_real_token(scanner, tokens, index);
    topic = "alter " + object_from_token(tokens, scanner, index);
    break;

  case CREATE_SYMBOL:
    get_next_real_token(scanner, tokens, index);
    topic = "create " + object_from_token(tokens, scanner, index);
    break;

  case DROP_SYMBOL:
    get_next_real_token(scanner, tokens, index);
    topic = "drop " + object_from_token(tokens, scanner, index);
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
    token = get_next_real_token(scanner, tokens, index);
    switch (token.type)
    {
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
    token = get_next_real_token(scanner, tokens, index);
    switch (token.type)
    {
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
    token = get_next_real_token(scanner, tokens, index);
    if (token.type == INDEX_SYMBOL)
      topic = "cache index";
    break;

  case CHANGE_SYMBOL:
    token = get_next_real_token(scanner, tokens, index);
    if (token.type == MASTER_SYMBOL)
      topic = "change master to";
    break;

  case CHECK_SYMBOL:
    topic = "check table";
    break;

  case CHECKSUM_SYMBOL:
    topic = "checksum table";
    break;

  case CURRENT_SYMBOL:
    token = get_previous_real_token(tokens, index);
    if (token.type == GET_SYMBOL)
      topic = "get diagnostics";
    else
    {
      token = get_previous_real_token(tokens, index);
      token = get_previous_real_token(tokens, index);
      if (token.type == DIAGNOSTICS_SYMBOL)
        topic = "get diagnostics";
    }
    break;

  case DEALLOCATE_SYMBOL:
    topic = "deallocate prepare";
    break;

  case DECLARE_SYMBOL:
    // There must be an identifier before the next keyword, but we also
    // handle cases where this was not (yet) written.
    token = get_next_real_token(scanner, tokens, index);

    switch (token.type)
    {
    case COMMA_SYMBOL:
    case CONDITION_SYMBOL:
    case CURSOR_SYMBOL:
    case CONTINUE_SYMBOL:
    case EXISTS_SYMBOL:
    case UNDO_SYMBOL:
    case HANDLER_SYMBOL:
      break; // Do nothing.

    default:
      token = get_next_real_token(scanner, tokens, index);
    }

    switch (token.type)
    {
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
    token = get_next_real_token(scanner, tokens, index);
    if (token.type == CURRENT_SYMBOL)
      get_next_real_token(scanner, tokens, index);
    token = get_next_real_token(scanner, tokens, index);
    if (token.type == DIAGNOSTICS_SYMBOL)
      topic = "get diagnostics";
    break;

  case INSTALL_SYMBOL:
    topic = "install plugin";
    break;

  case LOAD_SYMBOL:
    token = get_next_real_token(scanner, tokens, index);
    switch (token.type)
    {
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
    token = get_next_real_token(scanner, tokens, index);
    switch (token.type)
    {
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
    token = get_next_real_token(scanner, tokens, index);
    switch (token.type)
    {
    case UPDATE_SYMBOL:
    case DELETE_SYMBOL:
      topic = "constraint";
      break;
    }
    break;

  case OPTIMIZE_SYMBOL:
    token = get_next_real_token(scanner, tokens, index);
    switch (token.type)
    {
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
    token = get_previous_real_token(tokens, index);
    if (token.type == ANTLR3_TOKEN_INVALID) // Is this the first token?
      topic = "procedure analyse";
    else
      topic = ""; // Anything else with another leading token. Look them up for topic construction.
    break;

  case PURGE_SYMBOL:
    topic = "purge binary logs";
    break;

  case RENAME_SYMBOL:
    token = get_next_real_token(scanner, tokens, index);
    switch (token.type)
    {
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
    token = get_next_real_token(scanner, tokens, index);
    topic += token.text;

    break;

  case UNINSTALL_SYMBOL:
    topic = "uninstall plugin";
    break;

  case MATCH_SYMBOL:
    token = get_next_real_token(scanner, tokens, index);
    if (token.type == OPEN_PAR_SYMBOL)
      topic = "match against";

    break;

  case AGAINST_SYMBOL:
    topic = "match against";
    break;

  case DELAYED_SYMBOL:
    token = get_previous_real_token(tokens, index);
    switch (token.type)
    {
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

  }

  return topic; // Could be empty. This is checked at the caller level.
}

//--------------------------------------------------------------------------------------------------

/**
 * Handles the search for a single word topic, a topic which requires text transformation or
 * a topic which has several variants and one of them is a single word topic.
 */
std::string DbSqlEditorContextHelp::topic_with_single_topic_equivalent(MySQLToken token,
  MySQLScanner &scanner, std::vector<MySQLToken> &tokens, size_t &index) 
{
  log_debug2("Trying single word topics\n");

  std::string topic = base::tolower(token.text);

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

  switch (token.type)
  {
  case MINUS_OPERATOR:
    token = scanner.next_token();
    if (token.channel != 0 || MySQLRecognizer::is_operator(token.type)) // whitespace or operator follows
      topic = "- binary";
    else
      topic = "- unary";
    break;

  case ASSIGN_OPERATOR:
    topic = "assign-value"; // "assign-equal" contains nearly the same help text, not that for =, as one would expect.
    break;

  case BETWEEN_SYMBOL:
    token = get_previous_real_token(tokens, index);
    if (token.type == NOT_SYMBOL)
      topic = "not between";
    else
      topic = "between and";
    break;

  case BINARY_SYMBOL:
    token = get_previous_real_token(tokens, index);
    if (token.type == SHOW_SYMBOL)
    {
      topic = "show binary logs";
      break;
    }

    get_next_real_token(scanner, tokens, index);
    token = get_next_real_token(scanner, tokens, index);
    if (token.type == OPEN_PAR_SYMBOL)
      topic = "binary";
    else
      topic = "binary operator";
    break;

  case BINLOG_SYMBOL:
    token = get_previous_real_token(tokens, index);
    if (token.type == SHOW_SYMBOL)
      topic = "show binlog events";
    break;

  case CASE_SYMBOL:
    topic = "case statement";
    break;

  case NATIONAL_SYMBOL:
    topic = "char";
    break;

  case CHAR_SYMBOL:
    token = get_previous_real_token(tokens, index);
    if (token.type == SHOW_SYMBOL)
    {
      topic = "show character set";
      break;
    }
    get_next_real_token(scanner, tokens, index);
    token = get_next_real_token(scanner, tokens, index);
    if (token.type != OPEN_PAR_SYMBOL)
      topic = "char byte";
    else
      topic = ""; // We need a parser to check for "char function" vs "char()"
    break;
    
  case COLLATION_SYMBOL:
    token = get_previous_real_token(tokens, index);
    if (token.type == SHOW_SYMBOL)
      topic = "show collation";
    break;

  case COUNT_SYMBOL:
    token = get_previous_real_token(tokens, index);
    if (token.type == SHOW_SYMBOL) // show count(*) ...
    {
      topic = ""; // Let the parser or scan from start do the actual work.
      break;
    }

    get_next_real_token(scanner, tokens, index);
    token = get_next_real_token(scanner, tokens, index);
    if (token.type == OPEN_PAR_SYMBOL)
    {
      token = get_next_real_token(scanner, tokens, index);
      if (token.type == DISTINCT_SYMBOL)
        topic = "count distinct";
      else
        topic = "count";
    }
    else
      topic = "count";
    break;

  case DATABASE_SYMBOL:
    token = get_next_real_token(scanner, tokens, index);
    if (token.type == OPEN_PAR_SYMBOL)
      topic = "database"; // The function database().
    else
      topic = ""; // create database, drop database etc. Need to parse.
    break;

  case DATE_SYMBOL:
    token = get_next_real_token(scanner, tokens, index);
    if (token.type == OPEN_PAR_SYMBOL)
      topic = "date function";
    else
      topic = "date";
    break;

  case DELETE_SYMBOL:
    token = get_previous_real_token(tokens, index);
    if (token.type == ON_SYMBOL)
      topic = "constraint"; // Part of a foreign key definition (on update/delete);
    break;

  case DOUBLE_SYMBOL:
    token = get_next_real_token(scanner, tokens, index);
    if (token.type == PRECISION_SYMBOL)
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
    token = get_next_real_token(scanner, tokens, index);
    if (token.type == LOCAL_SYMBOL || token.type == NO_WRITE_TO_BINLOG_SYMBOL)
      token = get_next_real_token(scanner, tokens, index);
    if (token.type == QUERY_SYMBOL)
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
    token = get_next_real_token(scanner, tokens, index);
    if (token.type == OPEN_PAR_SYMBOL)
      topic = "if function";
    else
      topic = "if statement";
    break;

  case IS_SYMBOL:
    token = get_next_real_token(scanner, tokens, index);
    switch (token.type)
    {
    case NOT_SYMBOL:
      token = get_next_real_token(scanner, tokens, index);
      if (token.type == NULL_SYMBOL)
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
    token = get_previous_real_token(tokens, index);
    if (token.type == NOT_SYMBOL)
      topic = "not in";
    break;

  case LIKE_SYMBOL:
    token = get_previous_real_token(tokens, index);
    switch (token.type)
    {
    case NOT_SYMBOL:
      topic = "not like";
      break;

    case SOUNDS_SYMBOL:
      topic = "sounds like";
      break;
    }
    break;

  case OPEN_SYMBOL:
    token = get_previous_real_token(tokens, index);
    if (token.type == SHOW_SYMBOL)
      topic = "show open tables";
    break;

  case PASSWORD_SYMBOL:
    token = get_previous_real_token(tokens, index);
    if (token.type == SET_SYMBOL)
      topic = "set password";
    break;

  case REPEAT_SYMBOL:
    token = get_next_real_token(scanner, tokens, index);
    if (token.type == OPEN_PAR_SYMBOL)
      topic = "repeat function";
    else
      topic = "repeat loop";

    break;

  case REPLACE_SYMBOL:
    token = get_next_real_token(scanner, tokens, index);
    if (token.type == OPEN_PAR_SYMBOL)
      topic = "replace function";
    else
      topic = "replace";
    break;

  case RESET_SYMBOL:
    token = get_next_real_token(scanner, tokens, index);
    switch (token.type)
    { 
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
    token = get_next_real_token(scanner, tokens, index);
    if (token.type == GLOBAL_SYMBOL)
      token = get_next_real_token(scanner, tokens, index);

    switch (token.type)
    { 
    case OPEN_PAR_SYMBOL:
      topic = "set data type";
      break;

    case PASSWORD_SYMBOL:
      topic = "set password";
      break;

    case IDENTIFIER:
      if (base::tolower(token.text) == "sql_slave_skip_counter")
        topic = "set global sql_slave_skip_counter";
      else
        topic += " " + token.text;
      break;

    default:
      topic = "set";
    }
    break;

  case SHOW_SYMBOL:
    token = get_next_real_token(scanner, tokens, index);

    // Not all queries allow these tokens, but to be flexible we skip them anyway.
    if (token.type == FULL_SYMBOL || token.type == GLOBAL_SYMBOL || token.type == LOCAL_SYMBOL
      || token.type == SESSION_SYMBOL)
      token = get_next_real_token(scanner, tokens, index);

    switch (token.type)
    { 
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
      topic += " " + token.text;
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
      token = get_next_real_token(scanner, tokens, index);
      topic += " create " + token.text;
      break;

    case FUNCTION_SYMBOL:
      token = get_next_real_token(scanner, tokens, index);
      topic += " function " + token.text;
      break;

    case INDEX_SYMBOL:
    case INDEXES_SYMBOL:
    case KEYS_SYMBOL:
      topic += " index";
      break;

    case IN_SYMBOL:
      token = get_previous_real_token(tokens, index);
      if (token.type == NOT_SYMBOL)
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
      token = get_next_real_token(scanner, tokens, index);
      topic += " procedure " + token.text;
      break;

    case REGEXP_SYMBOL:
      token = get_previous_real_token(tokens, index);
      if (token.type == NOT_SYMBOL)
        topic = "not regexp";
      break;

    case STORAGE_SYMBOL:
      topic += " engines";
      break;

    case SLAVE_SYMBOL:
      token = get_next_real_token(scanner, tokens, index);
      topic += " slave " + token.text;
      break;

    case TABLE_SYMBOL:
      topic += " table status";
      break;

    case COUNT_SYMBOL: // show count(*) ..., use show warnings as default.
      topic += " warnings";
      token = get_next_real_token(scanner, tokens, index);
      if (token.type != OPEN_PAR_SYMBOL)
        break;
      token = get_next_real_token(scanner, tokens, index);
      if (token.type != MULT_OPERATOR)
        break;
      token = get_next_real_token(scanner, tokens, index);
      if (token.type != CLOSE_PAR_SYMBOL)
        break;

      token = get_next_real_token(scanner, tokens, index);
      if (token.type == ERRORS_SYMBOL)
        topic += " errors";
      break;
    }
    break;

  case TIME_SYMBOL:
    token = get_next_real_token(scanner, tokens, index);
    if (token.type == OPEN_PAR_SYMBOL)
      topic = "time function";
    else
      topic = "time"; // This is not entirely correct. Starting with 5.6 TIME can have a precision value
    // also specified within parentheses. This makes differentiating between
    // time function and time value impossible without outer context
    // (which is only available if we can parse the query).
    break;

  case TIMESTAMP_SYMBOL:
    token = get_next_real_token(scanner, tokens, index);
    if (token.type == OPEN_PAR_SYMBOL)
      topic = "timestamp function";
    else
      topic = "timestamp"; // The same applies here as for time.
    break;

  case TRUE_SYMBOL:
  case FALSE_SYMBOL:
    topic = "true false";
    break;

  case TRUNCATE_SYMBOL:
    token = get_next_real_token(scanner, tokens, index);
    switch (token.type)
    {
    case OPEN_PAR_SYMBOL:
      //topic = "truncate";
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
    token = get_next_real_token(scanner, tokens, index);
    if (token.type == OPEN_PAR_SYMBOL)
      topic = "year"; // Similar to time and timestamp this is not entirely correct. See TIME for more info.
    else
      topic = "year data type";
    break;

  case INSERT_SYMBOL:
    token = get_next_real_token(scanner, tokens, index);
    switch (token.type)
    {
    case DELAYED_SYMBOL:
      topic = "insert delayed";
      break;

    case OPEN_PAR_SYMBOL: // Insert function or grant insert (...).
      token = get_first_real_token(tokens);
      if (token.type == GRANTS_SYMBOL) // Could be wrong if this statement is part of a compound statement (e.g. in triggers or events).
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
    token = get_previous_real_token(tokens, index);
    if (token.type == ON_SYMBOL)
      topic = "constraint"; // Part of a foreign key definition (on update/delete);
    break;

  case USER_SYMBOL:
    token = get_next_real_token(scanner, tokens, index);
    if (token.type != OPEN_PAR_SYMBOL)
      topic = ""; // If not the user() function then more parsing is required.
    break;

  case PREPARE_SYMBOL:
    token = get_previous_real_token(tokens, index);
    switch (token.type)
    {
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
    token = get_previous_real_token(tokens, index);
    switch (token.type)
    {
    case CONDITION_SYMBOL:
    case EXIT_SYMBOL:
    case UNDO_SYMBOL:
    case DECLARE_SYMBOL:
      topic = ""; // Handling below.
      break;
    }

    break;

  case REGEXP_SYMBOL:
    token = get_previous_real_token(tokens, index);
    if (token.type == NOT_SYMBOL)
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
    if (topic == "sql_log_bin")
    {
      token = get_previous_real_token(tokens, index);
      if (token.type == SET_SYMBOL)
        topic = "set sql_log_bin";
      else
        topic = "";
      break;
    }

    token = get_next_real_token(scanner, tokens, index);
    if (token.type == COLON_SYMBOL)
    {
      topic = "labels";
      break;
    }

    if (topic == "x" || topic == "y") // X() and Y() check.
    {
      if (token.type != OPEN_PAR_SYMBOL)
        topic = "";
      break;
    }

    break;

  }

  return topic;
}

//--------------------------------------------------------------------------------------------------
