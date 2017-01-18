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

#include <sstream>
#include <algorithm>
#include <string>
#include <stack>
#include <set>
#include <map>

#include <antlr3.h>
#include <glib.h>

#include "MySQLLexer.h"  // The generated lexer.
#include "MySQLParser.h" // The generated parser.

#include "base/log.h"
#include "base/string_utilities.h"

#include "mysql-parser.h"
#include "mysql-scanner.h"

DEFAULT_LOG_DOMAIN("MySQL parsing")

//--------------------------------------------------------------------------------------------------

std::string get_token_name(pANTLR3_UINT8 *tokenNames, ANTLR3_UINT32 token) {
  // Transform a selection of tokens to nice strings. All others just take the token name.
  switch (token) {
    case ANTLR3_TOKEN_EOF:
      return "end of statement";
    case 1:
    case 2:
    case 3:
    case 4:
      return "<invalid token>";
    case OPEN_PAR_SYMBOL:
      return "opening parenthesis";
    case CLOSE_PAR_SYMBOL:
      return "closing parenthesis";
    case OPEN_CURLY_SYMBOL:
      return "opening curly brace";
    case CLOSE_CURLY_SYMBOL:
      return "closing curly brace";
    case NULL2_SYMBOL:
      return "null escape sequence";
    case PARAM_MARKER:
      return "parameter placeholder";

    default:
      std::string result = base::tolower((char *)tokenNames[token]);
      std::string::size_type position = result.find("_symbol");
      if (position != std::string::npos)
        result = result.substr(0, position);

      base::replaceStringInplace(result, "_", " ");
      return result;
  }
}

//--------------------------------------------------------------------------------------------------

std::string formatVersion(long version) {
  long major = version / 10000, minor = (version / 100) % 100, release = version % 100;
  return base::strfmt("%ld.%ld.%ld", major, minor, release);
}

//--------------------------------------------------------------------------------------------------

/**
 * Parses a server version predicate to get out the version + a word for the error message
 * that describes the version relationship.
 */
std::string handleServerVersion(const std::vector<std::string> parts, bool withPrefix) {
  bool includesEquality = parts[1].size() == 2;
  std::string version = formatVersion(atoi(parts[2].c_str()));
  switch (parts[1][0]) {
    case '<': // A max version.
      if (!includesEquality)
        return withPrefix ? "server versions before " + version : "before " + version;
      return withPrefix ? "server versions up to " + version : "up to " + version;
    case '=': // An exact version.
      return "the server version " + version;
    case '>': // A min version.
      if (!includesEquality)
        withPrefix ? "server versions after " + version : "after " + version;
      return withPrefix ? "server versions starting with " + version : "starting with " + version;

    default:
      return "specific versions"; // Unexpected operator found. Return a generic message part.
  }
}

//------------------------------------------------------------------------------------------------

/**
 * Parses the given predicate and constructs an error message that tells why there is that error.
 */
std::string createErrorFromPredicate(std::string predicate, long version) {
  // Parsable predicates have one of these forms:
  // - "SERVER_VERSION >= 50100"
  // - "(SERVER_VERSION >= 50105) && (SERVER_VERSION < 50500)"
  // - "SQL_MODE_ACTIVE(SQL_MODE_ANSI_QUOTES)"
  // - "!SQL_MODE_ACTIVE(SQL_MODE_ANSI_QUOTES)"
  //
  // We don't do full expression parsing here. Only what is given above.
  predicate = base::trim(predicate);
  std::vector<std::string> parts = base::split(predicate, "&&");
  std::string message = "\nThis syntax is only allowed for %s. The current version is " + formatVersion(version);
  switch (parts.size()) {
    case 2: {
      // Min and max values for server versions.
      std::string messagePart = "";
      std::string expression = base::trim(parts[0]);
      if (base::hasPrefix(expression, "(") && base::hasSuffix(expression, ")"))
        expression = expression.substr(1, expression.size() - 2);
      std::vector<std::string> expressionParts = base::split(expression, " ");
      if ((expressionParts[0] == "SERVER_VERSION") && (expressionParts.size() == 3))
        messagePart = handleServerVersion(expressionParts, true);

      expression = base::trim(parts[1]);
      if (base::hasPrefix(expression, "(") && base::hasSuffix(expression, ")"))
        expression = expression.substr(1, expression.size() - 2);
      expressionParts = base::split(expression, " ");
      if ((expressionParts[0] == "SERVER_VERSION") && (expressionParts.size() == 3))
        messagePart += " and " + handleServerVersion(expressionParts, false);

      if (messagePart.empty())
        return "";

      return base::strfmt(message.c_str(), messagePart.c_str());
    }

    case 1: {
      // A single expression.
      std::string messagePart = "";
      std::vector<std::string> expressionParts = base::split(predicate, " ");
      if (expressionParts.size() == 1) {
        message = "\nThis syntax is only allowed if ";
        if (base::hasPrefix(predicate, "SQL_MODE_ACTIVE("))
          message += predicate.substr(16, predicate.size() - 17) + " is active.";
        else if (base::hasPrefix(predicate, "!SQL_MODE_ACTIVE("))
          message += predicate.substr(17, predicate.size() - 18) + " is not active.";
      } else {
        if ((expressionParts[0] == "SERVER_VERSION") && (expressionParts.size() == 3))
          messagePart = handleServerVersion(expressionParts, true);
      }

      if (messagePart.empty())
        return "";

      return base::strfmt(message.c_str(), messagePart.c_str());
    }
    default:
      return "";
  }
}

//--------------------------------------------------------------------------------------------------

bool handleLexerError(pANTLR3_BASE_RECOGNIZER recognizer, pANTLR3_EXCEPTION exception, ANTLR3_MARKER &start,
                      ANTLR3_MARKER &length, std::string &message) {
  std::ostringstream error;
  pANTLR3_LEXER lexer = (pANTLR3_LEXER)(recognizer->super);
  start = recognizer->state->tokenStartCharIndex;

  length = exception->index - start;

  std::string tokenText((char *)start, length);
  switch (exception->type) {
    case ANTLR3_RECOGNITION_EXCEPTION: {
      switch (tokenText[0]) {
        case '/':
          error << "unfinished multiline comment";
          break;
        case 'x':
        case 'X':
          error << "unfinished hex string literal";
          break;
        case 'b':
        case 'B':
          error << "unfinished binary string literal";
          break;
        default:
          error << "unexpected input";
          break;
      }
      break;
    }

    case ANTLR3_NO_VIABLE_ALT_EXCEPTION: {
      switch (recognizer->state->type) {
        case DOUBLE_QUOTE:
          error << "unfinished double quote string";
          break;

        case SINGLE_QUOTE:
          error << "unfinished single quote string";
          break;

        case BACK_TICK:
          error << "unfinished back tick quote string";
          break;

        default:
          error << "unexpected input";
          break;
      }
      break;
    }

    case ANTLR3_FAILED_PREDICATE_EXCEPTION: {
      // One of the semantic predicates failed. Since most of those are our version check predicates
      // we can use that to give the user a hint about this.
      std::string predicate = (const char *)exception->message;
      RecognitionContext *context = (RecognitionContext *)recognizer->state->userp;
      error << "'" << tokenText << "' is not a valid keyword" << createErrorFromPredicate(predicate, context->version);
      break;
    }

    default:
      return false;
  }

  message = error.str();
  return true;
}

//--------------------------------------------------------------------------------------------------

bool handleParserError(pANTLR3_BASE_RECOGNIZER recognizer, pANTLR3_EXCEPTION exception, pANTLR3_UINT8 *tokenNames,
                       ANTLR3_MARKER &start, ANTLR3_MARKER &length, std::string &message) {
  std::ostringstream error;

  pANTLR3_PARSER parser = (pANTLR3_PARSER)(recognizer->super);
  pANTLR3_COMMON_TOKEN error_token = (pANTLR3_COMMON_TOKEN)(exception->token);

  std::string token_text = (char *)error_token->getText(error_token)->chars;
  if (token_text[0] != '"' && token_text[0] != '\'' && token_text[0] != '`')
    token_text = "'" + token_text + "'";

  std::string token_name;

  bool eoi = error_token->type == ANTLR3_TOKEN_EOF;
  if (eoi) {
    // We are at the end of the input. Seek back one token to have a meaningful error indicator.
    // If we cannot get a previous token then issue a generic eoi error.
    pANTLR3_COMMON_TOKEN previous_token = parser->tstream->_LT(parser->tstream, -1);
    if (previous_token != NULL)
      error_token = previous_token;
  } else
    token_name = get_token_name(tokenNames, error_token->type);

  start = error_token->start;
  switch (exception->type) {
    case ANTLR3_RECOGNITION_EXCEPTION:
      // Unpredicted input.
      if (error_token->type == INVALID_INPUT) // Our catch all rule for any char not allowed in MySQL.
        error << token_text << " is not allowed at all";
      else
        error << token_text << " (" << token_name << ") is not valid input at this position";
      break;

    case ANTLR3_MISMATCHED_TOKEN_EXCEPTION:
      // We were expecting to see one thing and got another. This is the
      // most common error if we could not detect a missing or unwanted token.
      if (exception->expecting == ANTLR3_TOKEN_EOF)
        error << "expected end of statement but found " << token_text << " (" << token_name << ")";
      else
        error << "expected '" << tokenNames[exception->expecting] << "' but found " << token_text << " (" << token_name
              << ")";
      break;

    case ANTLR3_NO_VIABLE_ALT_EXCEPTION:
      // No alternative to choose from here.
      if (eoi)
        error << "unexpected end of input";
      else
        error << "unexpected " << token_text << " (" << token_name << ")";

      break;

    case ANTLR3_MISMATCHED_SET_EXCEPTION: {
      // One out of a set of tokens was expected but hasn't been found.
      pANTLR3_BITSET errBits = antlr3BitsetLoad(exception->expectingSet);
      ANTLR3_UINT32 numbits = errBits->numBits(errBits);
      ANTLR3_UINT32 size = errBits->size(errBits);

      if (size == 0) {
        // No information about expected tokens available.
        error << "unexpected " << token_text << " (" << token_name << ")";
      } else {
        // I'd expect only a few set members here, but this case is hard to test. So
        // just to be sure not to show a huge list of expected tokens we limit the number here.
        // TODO: find a query that triggers this error branch.
        error << "wrong input, expected one of: ";
        for (ANTLR3_UINT32 bit = 1; bit < numbits && bit <= 20 && bit < size; ++bit) {
          if (errBits->isMember(errBits, bit))
            error << (bit > 1 ? ", " : "") << get_token_name(tokenNames, bit);
        }
      }
    } break;

    case ANTLR3_EARLY_EXIT_EXCEPTION:
      // We entered a loop requiring a number of token sequences but found a token that ended that
      // sequence earlier than we should have done.
      // Unfortunately, there's no expecting set for this exception which would have made the message
      // very useful.
      error << "missing sub clause or other elements before " << token_text << " (" << token_name << ")";
      break;

    case ANTLR3_FAILED_PREDICATE_EXCEPTION: {
      // One of the semantic predicates failed. Since most of those are our version check predicates
      // we can use that to give the user a hint about this.
      std::string predicate = (const char *)exception->message;
      RecognitionContext *context = (RecognitionContext *)recognizer->state->userp;
      error << token_text << " (" << token_name << ") is not valid input here."
            << createErrorFromPredicate(predicate, context->version);
      break;
    }

    case ANTLR3_MISMATCHED_TREE_NODE_EXCEPTION:
      // This is very likely a tree parser error and hence not relevant here (no info in ANTLR docs).
      error << "unexpected parser error type (" << exception->type << "), please file a bug report!";
      break;

    case ANTLR3_REWRITE_EARLY_EXCEPTION:
      // ANTLR docs say: No elements within a (...)+ in a rewrite rule
      // so this seems to be an error only raised if there was a grammar bug -> internal error.
      error << "internal parser error type (" << exception->type << "), please file a bug report!";
      break;

    case ANTLR3_UNWANTED_TOKEN_EXCEPTION:
      // Indicates that the recognizer was fed a token which seems to be spurious input. We can detect
      // this when the token that follows this unwanted token would normally be part of the
      // syntactically correct stream.
      if (exception->expecting == ANTLR3_TOKEN_EOF)
        error << "extraneous input found - expected end of input";
      else
        error << "extraneous input found - expected '" << get_token_name(tokenNames, exception->expecting) << "'";
      break;

    case ANTLR3_MISSING_TOKEN_EXCEPTION: {
      // Indicates that the recognizer detected that the token we just
      // hit would be valid syntactically if preceded by a particular
      // token. Perhaps a missing ';' at line end or a missing ',' in an
      // expression list, and such like.
      if (tokenNames == NULL)
        error << "missing token " << exception->expecting; // Will very likely never occur.
      else {
        if (exception->expecting == ANTLR3_TOKEN_EOF)
          // Will probably not occur since ANTLR3_UNWANTED_TOKEN_EXCEPTION will kick in instead.
          error << "expected end of input";
        else
          error << "missing '" << get_token_name(tokenNames, exception->expecting) << "'";
      }

      // The error token for a missing token does not contain much information to display.
      // If we reach this error case then the token after the missing token has been consumed already
      // (which is how the parser found out about the missing one), so by going back to that token
      // we can get good start and length information (showing so the error at this following token instead).
      error_token = parser->tstream->_LT(parser->tstream, -1);
      if (error_token == NULL)
        return false;

      start = error_token->start;
      break;
    }

    default:
      error << "unexpected parser error type (" << exception->type << "), please file a bug report!";
      break;
  }

  if (length == 0) {
    if (error_token != NULL)
      length = (int)error_token->stop - (int)error_token->start + 1;
    else
      length = 1;
  }

  message = error.str();
  return true;
}

//--------------------------------------------------------------------------------------------------

extern "C" {

/**
 * Error report function which is set in the parser (see MySQL.g where this is done).
 */
void onMySQLParseError(pANTLR3_BASE_RECOGNIZER recognizer, pANTLR3_UINT8 *tokenNames) {
  pANTLR3_EXCEPTION exception = recognizer->state->exception;

  // Only the take the current exception into account. There's a linked list of all exceptions we could walk
  // but that only contains what we've seen anyway.
  if (exception != NULL) {
    // Token position and length for error marker.
    ANTLR3_MARKER length = 0;
    ANTLR3_MARKER start = 0;
    std::string message;
    switch (recognizer->type) {
      case ANTLR3_TYPE_LEXER:
        if (!handleLexerError(recognizer, exception, start, length, message))
          return;
        break;

      case ANTLR3_TYPE_PARSER:
        if (!handleParserError(recognizer, exception, tokenNames, start, length, message))
          return;
        break;
    }

    MySQLRecognitionBase *our_recognizer =
      (MySQLRecognitionBase *)((RecognitionContext *)recognizer->state->userp)->payload;
    our_recognizer->add_error("Syntax error: " + message, recognizer->state->type, start, exception->line,
                              exception->charPositionInLine, length);
  }
}

} // extern "C"

//----------------- MySQLTreeWalker ----------------------------------------------------------------

MySQLRecognizerTreeWalker::MySQLRecognizerTreeWalker(MySQLRecognizer *recognizer, pANTLR3_BASE_TREE tree)
  : RecognizerTreeWalker(recognizer, tree) {
}

//--------------------------------------------------------------------------------------------------

MySQLRecognizer *MySQLRecognizerTreeWalker::recognizer() {
  return dynamic_cast<MySQLRecognizer *>(_recognizer);
}

//--------------------------------------------------------------------------------------------------

/**
 * Steps back to the start of the current subquery (or the top level query if we are not in a subquery).
 * On exit the current walker position is on the first query token.
 */
void MySQLRecognizerTreeWalker::goToSubQueryStart() {
  do {
    switch (tokenType()) {
      case ANALYZE_SYMBOL:
      case ALTER_SYMBOL:
      case BACKUP_SYMBOL:
      case BINLOG_SYMBOL:
      case CACHE_SYMBOL:
      case CALL_SYMBOL:
      case CHANGE_SYMBOL:
      case CHECK_SYMBOL:
      case CHECKSUM_SYMBOL:
      case COMMIT_SYMBOL:
      case CREATE_SYMBOL:
      case DEALLOCATE_SYMBOL:
      case DELETE_SYMBOL:
      case DESC_SYMBOL:
      case DESCRIBE_SYMBOL:
      case DO_SYMBOL:
      case DROP_SYMBOL:
      case EXECUTE_SYMBOL:
      case EXPLAIN_SYMBOL:
      case FLUSH_SYMBOL:
      case GRANT_SYMBOL:
      case HANDLER_SYMBOL:
      case HELP_SYMBOL:
      case INSERT_SYMBOL:
      case INSTALL_SYMBOL:
      case KILL_SYMBOL:
      case LOAD_SYMBOL:
      case LOCK_SYMBOL:
      case OPTIMIZE_SYMBOL:
      case PARTITION_SYMBOL:
      case PREPARE_SYMBOL:
      case PURGE_SYMBOL:
      case RELEASE_SYMBOL:
      case REMOVE_SYMBOL:
      case RENAME_SYMBOL:
      case REPAIR_SYMBOL:
      case REPLACE_SYMBOL:
      case RESET_SYMBOL:
      case RESTORE_SYMBOL:
      case REVOKE_SYMBOL:
      case ROLLBACK_SYMBOL:
      case SAVEPOINT_SYMBOL:
      case SELECT_SYMBOL:
      case SET_SYMBOL:
      case SHOW_SYMBOL:
      case START_SYMBOL:
      case STOP_SYMBOL:
      case TRUNCATE_SYMBOL:
      case UNINSTALL_SYMBOL:
      case UNLOCK_SYMBOL:
      case UPDATE_SYMBOL:
      case USE_SYMBOL:
      case XA_SYMBOL:
        return;

      default:
        if (!up()) {
          // Advance to first child.
          next();
          return;
        }
        break;
    }
  } while (true);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the query type of the (sub)query the waker is in currently.
 * We cannot use the MySQLQueryIdentifier here as it doesn't allow to start at an arbitrary
 * position.
 */
MySQLQueryType MySQLRecognizerTreeWalker::getCurrentQueryType() {
  // Walk up the parent chain until we find either a major keyword or the top of the tree.
  push();
  bool done = false;
  do {
    switch (tokenType()) {
      case ANALYZE_SYMBOL:
      case ALTER_SYMBOL:
      case BACKUP_SYMBOL:
      // case BEGIN_SYMBOL:
      case BINLOG_SYMBOL:
      case CACHE_SYMBOL:
      case CALL_SYMBOL:
      case CHANGE_SYMBOL:
      case CHECK_SYMBOL:
      case CHECKSUM_SYMBOL:
      case COMMIT_SYMBOL:
      case CREATE_SYMBOL:
      case DEALLOCATE_SYMBOL:
      case DELETE_SYMBOL:
      case DESC_SYMBOL:
      case DESCRIBE_SYMBOL:
      case DO_SYMBOL:
      case DROP_SYMBOL:
      case EXECUTE_SYMBOL:
      case EXPLAIN_SYMBOL:
      case FLUSH_SYMBOL:
      case GRANT_SYMBOL:
      case HANDLER_SYMBOL:
      case HELP_SYMBOL:
      case INSERT_SYMBOL:
      case INSTALL_SYMBOL:
      case KILL_SYMBOL:
      case LOAD_SYMBOL:
      case LOCK_SYMBOL:
      case OPTIMIZE_SYMBOL:
      case PARTITION_SYMBOL:
      case PREPARE_SYMBOL:
      case PURGE_SYMBOL:
      case RELEASE_SYMBOL:
      case REMOVE_SYMBOL:
      case RENAME_SYMBOL:
      case REPAIR_SYMBOL:
      case REPLACE_SYMBOL:
      case RESET_SYMBOL:
      case RESTORE_SYMBOL:
      case REVOKE_SYMBOL:
      case ROLLBACK_SYMBOL:
      case SAVEPOINT_SYMBOL:
      case SELECT_SYMBOL:
      case SET_SYMBOL:
      case SHOW_SYMBOL:
      case START_SYMBOL:
      case STOP_SYMBOL:
      case TRUNCATE_SYMBOL:
      case UNINSTALL_SYMBOL:
      case UNLOCK_SYMBOL:
      case UPDATE_SYMBOL:
      case USE_SYMBOL:
      case XA_SYMBOL:
        if (isSubtree())
          done = true;
        else if (!up())
          done = true;
        break;
      default:
        if (!up())
          done = true;
        break;
    }
  } while (!done);

  // Second step. Actual determintation.
  MySQLQueryType result = queryType();

  pop();

  return result;
}

//--------------------------------------------------------------------------------------------------

MySQLQueryType MySQLRecognizerTreeWalker::queryType() {
  // TODO: this duplicates code in MySQLQueryIdentifier.
  //       Find a way to handle all that in a single place.

  switch (tokenType()) {
    case ALTER_SYMBOL:
      if (!next())
        return QtAmbiguous;

      switch (tokenType()) {
        case DATABASE_SYMBOL:
          return QtAlterDatabase;

        case LOGFILE_SYMBOL:
          return QtAlterLogFileGroup;

        case FUNCTION_SYMBOL:
          return QtAlterFunction;

        case PROCEDURE_SYMBOL:
          return QtAlterProcedure;

        case SERVER_SYMBOL:
          return QtAlterServer;

        case TABLE_SYMBOL:
        case ONLINE_SYMBOL:  // Optional part of ALTER TABLE.
        case OFFLINE_SYMBOL: // ditto
        case IGNORE_SYMBOL:
          return QtAlterTable;

        case TABLESPACE_SYMBOL:
          return QtAlterTableSpace;

        case EVENT_SYMBOL:
          return QtAlterEvent;

        case VIEW_SYMBOL:
          return QtAlterView;

        case DEFINER_SYMBOL:  // Can be both event or view.
          if (!nextSibling()) // DEFINER has an own subtree so we can just jump over the details.
            return QtAmbiguous;

          switch (tokenType()) {
            case EVENT_SYMBOL:
              return QtAlterEvent;

            case SQL_SYMBOL:
            case VIEW_SYMBOL:
              return QtAlterView;
          }
          break;

        case ALGORITHM_SYMBOL: // Optional part of CREATE VIEW.
          return QtAlterView;

        case USER_SYMBOL:
          return QtAlterUser;
      }
      break;

    case CREATE_SYMBOL:
      if (!next())
        return QtAmbiguous;

      switch (tokenType()) {
        case TEMPORARY_SYMBOL: // Optional part of CREATE TABLE.
        case TABLE_SYMBOL:
          return QtCreateTable;

        case ONLINE_SYMBOL:
        case OFFLINE_SYMBOL:
        case INDEX_SYMBOL:
        case UNIQUE_SYMBOL:
        case FULLTEXT_SYMBOL:
        case SPATIAL_SYMBOL:
          return QtCreateIndex;

        case DATABASE_SYMBOL:
          return QtCreateDatabase;

        case TRIGGER_SYMBOL:
          return QtCreateTrigger;

        case DEFINER_SYMBOL: // Can be event, view, procedure, function, UDF, trigger.
        {
          if (!nextSibling())
            return QtAmbiguous;

          switch (tokenType()) {
            case EVENT_SYMBOL:
              return QtCreateEvent;

            case VIEW_SYMBOL:
            case SQL_SYMBOL:
              return QtCreateView;

            case PROCEDURE_SYMBOL:
              return QtCreateProcedure;

            case FUNCTION_SYMBOL: {
              if (!next())
                return QtAmbiguous;

              if (is(UDF_NAME_TOKEN))
                return QtCreateUdf;
              return QtCreateFunction;
            }

            case AGGREGATE_SYMBOL:
              return QtCreateUdf;

            case TRIGGER_SYMBOL:
              return QtCreateTrigger;
          }
        }

        case VIEW_SYMBOL:
        case OR_SYMBOL:        // CREATE OR REPLACE ... VIEW
        case ALGORITHM_SYMBOL: // CREATE ALGORITHM ... VIEW
          return QtCreateView;

        case EVENT_SYMBOL:
          return QtCreateEvent;

        case FUNCTION_SYMBOL:
          return QtCreateFunction;

        case AGGREGATE_SYMBOL:
          return QtCreateUdf;

        case PROCEDURE_SYMBOL:
          return QtCreateProcedure;

        case LOGFILE_SYMBOL:
          return QtCreateLogFileGroup;

        case SERVER_SYMBOL:
          return QtCreateServer;

        case TABLESPACE_SYMBOL:
          return QtCreateTableSpace;

        case USER_SYMBOL:
          return QtCreateUser;
      }
      break;

    case DROP_SYMBOL: {
      if (!next())
        return QtAmbiguous;

      switch (tokenType()) {
        case DATABASE_SYMBOL:
          return QtDropDatabase;

        case EVENT_SYMBOL:
          return QtDropEvent;

        case PROCEDURE_SYMBOL:
          return QtDropProcedure;

        case FUNCTION_SYMBOL:
          return QtDropFunction;

        case ONLINE_SYMBOL:
        case OFFLINE_SYMBOL:
        case INDEX_SYMBOL:
          return QtDropIndex;

        case LOGFILE_SYMBOL:
          return QtDropLogfileGroup;

        case SERVER_SYMBOL:
          return QtDropServer;

        case TEMPORARY_SYMBOL:
        case TABLE_SYMBOL:
        case TABLES_SYMBOL:
          return QtDropTable;

        case TABLESPACE_SYMBOL:
          return QtDropTablespace;

        case TRIGGER_SYMBOL:
          return QtDropTrigger;

        case VIEW_SYMBOL:
          return QtDropView;

        case PREPARE_SYMBOL:
          return QtDeallocate;

        case USER_SYMBOL:
          return QtDropUser;
      }
    }

    case TRUNCATE_SYMBOL:
      return QtTruncateTable;

    case CALL_SYMBOL:
      return QtCall;

    case DELETE_SYMBOL:
      return QtDelete;

    case DO_SYMBOL:
      return QtDo;

    case HANDLER_SYMBOL:
      return QtHandler;

    case INSERT_SYMBOL:
      return QtInsert;

    case LOAD_SYMBOL: {
      if (!next())
        return QtAmbiguous;

      switch (tokenType()) {
        case DATA_SYMBOL: {
          if (!next())
            return QtAmbiguous;

          if (tokenType() == FROM_SYMBOL)
            return QtLoadDataMaster;
          return QtLoadData;
        }
        case XML_SYMBOL:
          return QtLoadXML;

        case TABLE_SYMBOL:
          return QtLoadTableMaster;

        case INDEX_SYMBOL:
          return QtLoadIndex;
      }
    }

    case REPLACE_SYMBOL:
      return QtReplace;

    case SELECT_SYMBOL:
      return QtSelect;

    case UPDATE_SYMBOL:
      return QtUpdate;

    case OPEN_PAR_SYMBOL: // Either (((select ..))) or (partition...)
    {
      while (tokenType() == OPEN_PAR_SYMBOL) {
        if (!next())
          return QtAmbiguous;
      }
      if (tokenType() == SELECT_SYMBOL)
        return QtSelect;
      return QtPartition;
    }

    case PARTITION_SYMBOL:
    case PARTITIONS_SYMBOL:
      return QtPartition;

    case START_SYMBOL: {
      if (!next())
        return QtAmbiguous;

      if (tokenType() == TRANSACTION_SYMBOL)
        return QtStartTransaction;
      return QtStartSlave;
    }

    case BEGIN_SYMBOL: // Begin directly at the start of the query must be a transaction start.
      return QtBeginWork;

    case COMMIT_SYMBOL:
      return QtCommit;

    case ROLLBACK_SYMBOL: {
      // We assume a transaction statement here unless we exactly know it's about a savepoint.
      if (!next())
        return QtRollbackWork;
      if (tokenType() == WORK_SYMBOL) {
        if (!next())
          return QtRollbackWork;
      }

      if (tokenType() == TO_SYMBOL)
        return QtRollbackSavepoint;
      return QtRollbackWork;
    }

    case SET_SYMBOL: {
      if (!next())
        return QtSet;

      switch (tokenType()) {
        case PASSWORD_SYMBOL:
          return QtSetPassword;

        case GLOBAL_SYMBOL:
        case LOCAL_SYMBOL:
        case SESSION_SYMBOL:
          if (!next())
            return QtSet;
          break;

        case IDENTIFIER:
          if (base::tolower(tokenText()) == "autocommit")
            return QtSetAutoCommit;
          break;
      }

      if (tokenType() == TRANSACTION_SYMBOL)
        return QtSetTransaction;
      return QtSet;
    }

    case SAVEPOINT_SYMBOL:
      return QtSavepoint;

    case RELEASE_SYMBOL: // Release at the start of the query, obviously.
      return QtReleaseSavepoint;

    case LOCK_SYMBOL:
      return QtLock;

    case UNLOCK_SYMBOL:
      return QtUnlock;

    case XA_SYMBOL:
      return QtXA;

    case PURGE_SYMBOL:
      return QtPurge;

    case CHANGE_SYMBOL:
      return QtChangeMaster;

    case RESET_SYMBOL: {
      if (!next())
        return QtReset;

      switch (tokenType()) {
        case SERVER_SYMBOL:
          return QtResetMaster;
        case SLAVE_SYMBOL:
          return QtResetSlave;
        default:
          return QtReset;
      }
    }

    case STOP_SYMBOL:
      return QtStopSlave;

    case PREPARE_SYMBOL:
      return QtPrepare;

    case EXECUTE_SYMBOL:
      return QtExecute;

    case DEALLOCATE_SYMBOL:
      return QtDeallocate;

    case GRANT_SYMBOL: {
      if (!next())
        return QtAmbiguous;

      if (tokenType() == PROXY_SYMBOL)
        return QtGrantProxy;
      return QtGrant;
    }

    case RENAME_SYMBOL: {
      if (!next())
        return QtAmbiguous;

      if (tokenType() == USER_SYMBOL)
        return QtRenameUser;
      return QtRenameTable;
    }

    case REVOKE_SYMBOL: {
      if (!next())
        return QtAmbiguous;

      if (tokenType() == PROXY_SYMBOL)
        return QtRevokeProxy;
      return QtRevoke;
    }

    case ANALYZE_SYMBOL:
      return QtAnalyzeTable;

    case CHECK_SYMBOL:
      return QtCheckTable;

    case CHECKSUM_SYMBOL:
      return QtChecksumTable;

    case OPTIMIZE_SYMBOL:
      return QtOptimizeTable;

    case REPAIR_SYMBOL:
      return QtRepairTable;

    case BACKUP_SYMBOL:
      return QtBackUpTable;

    case RESTORE_SYMBOL:
      return QtRestoreTable;

    case INSTALL_SYMBOL:
      return QtInstallPlugin;

    case UNINSTALL_SYMBOL:
      return QtUninstallPlugin;

    case SHOW_SYMBOL: {
      if (!next())
        return QtShow;

      if (tokenType() == FULL_SYMBOL) {
        // Not all SHOW cases allow an optional FULL keyword, but this is not about checking for
        // a valid query but to find the most likely type.
        if (!next())
          return QtShow;
      }

      switch (tokenType()) {
        case GLOBAL_SYMBOL:
        case LOCK_SYMBOL:
        case SESSION_SYMBOL: {
          if (!next())
            return QtShow;

          if (tokenType() == STATUS_SYMBOL)
            return QtShowStatus;
          return QtShowVariables;
        }

        case AUTHORS_SYMBOL:
          return QtShowAuthors;

        case BINARY_SYMBOL:
          return QtShowBinaryLogs;

        case BINLOG_SYMBOL:
          return QtShowBinlogEvents;

        case RELAYLOG_SYMBOL:
          return QtShowRelaylogEvents;

        case CHAR_SYMBOL:
          return QtShowCharset;

        case COLLATION_SYMBOL:
          return QtShowCollation;

        case COLUMNS_SYMBOL:
          return QtShowColumns;

        case CONTRIBUTORS_SYMBOL:
          return QtShowContributors;

        case COUNT_SYMBOL: {
          if (!next() || tokenType() != OPEN_PAR_SYMBOL)
            return QtShow;
          if (!next() || tokenType() != MULT_OPERATOR)
            return QtShow;
          if (!next() || tokenType() != CLOSE_PAR_SYMBOL)
            return QtShow;

          if (!next())
            return QtShow;

          switch (tokenType()) {
            case WARNINGS_SYMBOL:
              return QtShowWarnings;

            case ERRORS_SYMBOL:
              return QtShowErrors;
          }

          return QtShow;
        }

        case CREATE_SYMBOL: {
          if (!next())
            return QtShow;

          switch (tokenType()) {
            case DATABASE_SYMBOL:
              return QtShowCreateDatabase;

            case EVENT_SYMBOL:
              return QtShowCreateEvent;

            case FUNCTION_SYMBOL:
              return QtShowCreateFunction;

            case PROCEDURE_SYMBOL:
              return QtShowCreateProcedure;

            case TABLE_SYMBOL:
              return QtShowCreateTable;

            case TRIGGER_SYMBOL:
              return QtShowCreateTrigger;

            case VIEW_SYMBOL:
              return QtShowCreateView;
          }

          return QtShow;
        }

        case DATABASES_SYMBOL:
          return QtShowDatabases;

        case ENGINE_SYMBOL:
          return QtShowEngineStatus;

        case STORAGE_SYMBOL:
        case ENGINES_SYMBOL:
          return QtShowStorageEngines;

        case ERRORS_SYMBOL:
          return QtShowErrors;

        case EVENTS_SYMBOL:
          return QtShowEvents;

        case FUNCTION_SYMBOL: {
          if (!next())
            return QtAmbiguous;

          if (tokenType() == CODE_SYMBOL)
            return QtShowFunctionCode;
          return QtShowFunctionStatus;
        }

        case GRANT_SYMBOL:
          return QtShowGrants;

        case INDEX_SYMBOL:
        case INDEXES_SYMBOL:
        case KEY_SYMBOL:
          return QtShowIndexes;

        case INNODB_SYMBOL:
          return QtShowInnoDBStatus;

        case MASTER_SYMBOL:
          return QtShowMasterStatus;

        case OPEN_SYMBOL:
          return QtShowOpenTables;

        case PLUGIN_SYMBOL:
        case PLUGINS_SYMBOL:
          return QtShowPlugins;

        case PROCEDURE_SYMBOL: {
          if (!next())
            return QtShow;

          if (tokenType() == STATUS_SYMBOL)
            return QtShowProcedureStatus;
          return QtShowProcedureCode;
        }

        case PRIVILEGES_SYMBOL:
          return QtShowPrivileges;

        case PROCESSLIST_SYMBOL:
          return QtShowProcessList;

        case PROFILE_SYMBOL:
          return QtShowProfile;

        case PROFILES_SYMBOL:
          return QtShowProfiles;

        case SLAVE_SYMBOL: {
          if (!next())
            return QtAmbiguous;

          if (tokenType() == HOSTS_SYMBOL)
            return QtShowSlaveHosts;
          return QtShowSlaveStatus;
        }

        case STATUS_SYMBOL:
          return QtShowStatus;

        case VARIABLES_SYMBOL:
          return QtShowVariables;

        case TABLE_SYMBOL:
          return QtShowTableStatus;

        case TABLES_SYMBOL:
          return QtShowTables;

        case TRIGGERS_SYMBOL:
          return QtShowTriggers;

        case WARNINGS_SYMBOL:
          return QtShowWarnings;
      }

      return QtShow;
    }

    case CACHE_SYMBOL:
      return QtCacheIndex;

    case FLUSH_SYMBOL:
      return QtFlush;

    case KILL_SYMBOL:
      return QtKill;

    case DESCRIBE_SYMBOL: // EXPLAIN is converted to DESCRIBE in the lexer.
    case DESC_SYMBOL: {
      if (!next())
        return QtAmbiguous;

      if (_recognizer->isIdentifier(tokenType()) || tokenType() == DOT_SYMBOL)
        return QtExplainTable;

      // EXTENDED is a bit special as it can be both, a table identifier or the keyword.
      if (tokenType() == EXTENDED_SYMBOL) {
        if (!next())
          return QtExplainTable;

        switch (tokenType()) {
          case DELETE_SYMBOL:
          case INSERT_SYMBOL:
          case REPLACE_SYMBOL:
          case UPDATE_SYMBOL:
            return QtExplainStatement;
          default:
            return QtExplainTable;
        }
      }
      return QtExplainStatement;
    }

    case HELP_SYMBOL:
      return QtHelp;

    case USE_SYMBOL:
      return QtUse;
  }

  return QtUnknown;
}

//----------------- MySQLRecognizer ----------------------------------------------------------------

class MySQLRecognizer::Private {
public:
  const char *_text;
  size_t _text_length;
  int _input_encoding;
  RecognitionContext _context;

  pANTLR3_INPUT_STREAM _input;
  pMySQLLexer _lexer;
  pANTLR3_COMMON_TOKEN_STREAM _tokens;
  pMySQLParser _parser;
  pANTLR3_BASE_TREE _ast;
};

//--------------------------------------------------------------------------------------------------

MySQLRecognizer::MySQLRecognizer(long server_version, const std::string &sql_mode,
                                 const std::set<std::string> &charsets)
  : MySQLRecognitionBase(charsets) {
  d = new Private();
  d->_context.version = server_version;
  d->_context.payload = this;
  set_sql_mode(sql_mode);

  d->_input = NULL;
  d->_lexer = NULL;
  d->_tokens = NULL;
  d->_parser = NULL;
}

//--------------------------------------------------------------------------------------------------

MySQLRecognizer::~MySQLRecognizer() {
  if (d->_parser != NULL)
    d->_parser->free(d->_parser);
  if (d->_tokens != NULL)
    d->_tokens->free(d->_tokens);
  if (d->_lexer != NULL)
    d->_lexer->free(d->_lexer);
  if (d->_input != NULL)
    d->_input->close(d->_input);

  delete d;
}

//--------------------------------------------------------------------------------------------------

/**
 * Starts parsing with new input but keeps everything else in place. This is expected to be more
 * efficient than creating a new parser over and over again for many statements (e.g. for error checking).
 *
 * @param text The text to parse.
 * @param length The length of the text.
 * @param is_utf8 True if text is utf-8 encoded. If false we assume ASCII encoding.
 * @param parse_unit used to restrict parsing to a particular query type.
 *                   Note: only a few types are supported, everything else is just parsed as a query.
 */
void MySQLRecognizer::parse(const char *text, size_t length, bool is_utf8, MySQLParseUnit parse_unit) {
  // If the text is not using utf-8 (which it should) then we interpret as 8bit encoding
  // (everything requiring only one byte per char as Latin1, ASCII and similar).
  d->_input_encoding = is_utf8 ? ANTLR3_ENC_UTF8 : ANTLR3_ENC_8BIT;

  d->_text = text;
  d->_text_length = length;

  // Logging adds significant time to parsing which especially shows with large scripts
  // (thousands of rather small queries to error check). And it adds not much benefit, so leave it off.
  // log_debug3("Start parsing\n");

  reset();

  if (d->_input == NULL) {
    // Input and depending structures are only created once. If there's no input stream yet we need the full setup.
    d->_input = antlr3StringStreamNew((pANTLR3_UINT8)d->_text, d->_input_encoding, (ANTLR3_UINT32)d->_text_length,
                                      (pANTLR3_UINT8) "");
    d->_input->setUcaseLA(
      d->_input, ANTLR3_TRUE); // Make input case-insensitive. String literals must all be upper case in the grammar!
    d->_lexer = MySQLLexerNew(d->_input);
    d->_lexer->pLexer->rec->state->userp = &d->_context;

    d->_tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(d->_lexer));

    d->_parser = MySQLParserNew(d->_tokens);
    d->_parser->pParser->rec->state->userp = &d->_context;
  } else {
    d->_input->reuse(d->_input, (pANTLR3_UINT8)d->_text, (ANTLR3_UINT32)d->_text_length, (pANTLR3_UINT8) "");
    d->_tokens->reset(d->_tokens);
    d->_lexer->reset(d->_lexer);
    d->_parser->reset(d->_parser);
  }

  switch (parse_unit) {
    case MySQLParseUnit::PuCreateTable:
      d->_ast = d->_parser->create_table(d->_parser).tree;
      break;
    case MySQLParseUnit::PuCreateTrigger:
      d->_ast = d->_parser->create_trigger(d->_parser).tree;
      break;
    case MySQLParseUnit::PuCreateView:
      d->_ast = d->_parser->create_view(d->_parser).tree;
      break;
    case MySQLParseUnit::PuCreateRoutine:
      d->_ast = d->_parser->create_routine(d->_parser).tree;
      break;
    case MySQLParseUnit::PuCreateEvent:
      d->_ast = d->_parser->create_event(d->_parser).tree;
      break;
    case MySQLParseUnit::PuCreateIndex:
      d->_ast = d->_parser->create_index(d->_parser).tree;
      break;
    case MySQLParseUnit::PuGrant:
      d->_ast = d->_parser->parse_grant(d->_parser).tree;
      break;
    case MySQLParseUnit::PuDataType:
      d->_ast = d->_parser->data_type_definition(d->_parser).tree;
      break;
    case MySQLParseUnit::PuCreateLogfileGroup:
      d->_ast = d->_parser->create_logfile_group(d->_parser).tree;
      break;
    case MySQLParseUnit::PuCreateServer:
      d->_ast = d->_parser->create_server(d->_parser).tree;
      break;
    case MySQLParseUnit::PuCreateTablespace:
      d->_ast = d->_parser->create_tablespace(d->_parser).tree;
      break;
    default:
      d->_ast = d->_parser->query(d->_parser).tree;
      break;
  }
}

//--------------------------------------------------------------------------------------------------

std::string MySQLRecognizer::dump_tree() {
  logDebug2("Generating parse tree\n");

  return dump_tree(d->_ast, "");
}

//--------------------------------------------------------------------------------------------------

std::string MySQLRecognizer::dump_tree(pANTLR3_BASE_TREE tree, const std::string &indentation) {
  std::string result;

  pANTLR3_RECOGNIZER_SHARED_STATE state = d->_parser->pParser->rec->state;
  ANTLR3_UINT32 char_pos = tree->getCharPositionInLine(tree);
  ANTLR3_UINT32 line = tree->getLine(tree);
  pANTLR3_STRING token_text = tree->getText(tree);

  pANTLR3_COMMON_TOKEN token = tree->getToken(tree);
  const char *utf8 = (const char *)token_text->chars;
  if (token != NULL) {
    ANTLR3_UINT32 token_type = token->getType(token);

    pANTLR3_UINT8 token_name;
    if (token_type == EOF)
      token_name = (pANTLR3_UINT8) "EOF";
    else
      token_name = state->tokenNames[token_type];

#ifdef ANTLR3_USE_64BIT
    result = base::strfmt("%s(line: %i, offset: %i, length: %" PRId64 ", index: %" PRId64 ", %s[%i])    %s\n",
                          indentation.c_str(), line, char_pos, token->stop - token->start + 1, token->index, token_name,
                          token_type, utf8);
#else
    result = base::strfmt("%s(line: %i, offset: %i, length: %i, index: %i, %s[%i])    %s\n", indentation.c_str(), line,
                          char_pos, token->stop - token->start + 1, token->index, token_name, token_type, utf8);
#endif

  } else {
    result = base::strfmt("%s(line: %i, offset: %i, nil)    %s\n", indentation.c_str(), line, char_pos, utf8);
  }

  for (ANTLR3_UINT32 index = 0; index < tree->getChildCount(tree); index++) {
    pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)tree->getChild(tree, index);
    std::string child_text = dump_tree(child, indentation + "\t");
    result += child_text;
  }
  return result;
}

//--------------------------------------------------------------------------------------------------

std::string MySQLRecognizer::text() const {
  return std::string(d->_text, d->_text_length);
}

//--------------------------------------------------------------------------------------------------

const char *MySQLRecognizer::lineStart() const {
  return d->_text;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns a tree walker for the current AST.
 */
MySQLRecognizerTreeWalker MySQLRecognizer::tree_walker() {
  return MySQLRecognizerTreeWalker(this, d->_ast);
}

//--------------------------------------------------------------------------------------------------

void MySQLRecognizer::set_sql_mode(const std::string &new_mode) {
  MySQLRecognitionBase::set_sql_mode(new_mode);
  d->_context.sqlMode = sql_mode(); // Parsed SQL mode.
}

//--------------------------------------------------------------------------------------------------

void MySQLRecognizer::set_server_version(long new_version) {
  d->_context.version = new_version;
}

//--------------------------------------------------------------------------------------------------

long MySQLRecognizer::server_version() {
  return d->_context.version;
}

//--------------------------------------------------------------------------------------------------

/**
 *	Returns the token start of this token or it's first real child/grandchild etc.
 */
static ANTLR3_MARKER getRealTokenStart(pANTLR3_BASE_TREE node) {
  if (node->getChildCount(node) == 0) {
    pANTLR3_COMMON_TOKEN token = node->getToken(node);
    return token->start;
  }

  return getRealTokenStart((pANTLR3_BASE_TREE)node->getChild(node, 0));
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the information for the token at the given index in the input stream. This includes
 * every possible token, including those on a hidden channel (e.g. comments and whitespaces).
 * Before calling this function the parser must have parsed the input to have the values available.
 * The result's type member can be used to find out if token information is not yet available or
 * the given index is out of the available range (ANTLR3_TOKEN_INVALID).
 */
ParserToken MySQLRecognizer::token_at_index(ANTLR3_MARKER index) {
  ParserToken result;

  pANTLR3_COMMON_TOKEN token = d->_tokens->tstream->get(d->_tokens->tstream, (ANTLR3_UINT32)index);
  if (token != NULL) {
    result.type = token->type;
    result.line = token->line;
    result.position = token->charPosition;
    result.index = token->index;
    result.channel = token->channel;
    result.line_start = (char *)token->lineStart;
    result.start = reinterpret_cast<char *>(token->start);
    result.stop = reinterpret_cast<char *>(token->stop);

    // If necessary the following part can be optimized to not always create a copy of the input.
    pANTLR3_STRING text = token->getText(token);
    result.text = (const char *)text->chars;
  }

  return result;
}

//--------------------------------------------------------------------------------------------------
