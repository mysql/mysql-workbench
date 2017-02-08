/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <string>
#include <set>

#include <antlr3.h>
#include <glib.h>

#include "base/log.h"
#include "base/string_utilities.h"

#include "MySQLLexer.h"

#include "mysql-scanner.h"

DEFAULT_LOG_DOMAIN("MySQL parsing")

#ifdef _WIN32
#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif
#endif

extern "C" {

/**
 * Error report function placeholder.
 */
void lexer_error(struct ANTLR3_BASE_lexer_struct *lexer, pANTLR3_UINT8 *tokenNames) {
}

} // extern "C"

//----------------- MySQLScanner -------------------------------------------------------------------

class MySQLScanner::Private {
public:
  const char *_text;
  size_t _text_length;
  int _input_encoding;
  RecognitionContext _context;

  pANTLR3_INPUT_STREAM _input;
  pMySQLLexer _lexer;
  pANTLR3_TOKEN_SOURCE _token_source;

  // In order to support arbitrary back tracking we cache all tokens from the input stream here.
  size_t _token_index;
  std::vector<pANTLR3_COMMON_TOKEN> _tokens;
};

MySQLScanner::MySQLScanner(const char *text, size_t length, bool is_utf8, long server_version,
                           const std::string &sql_mode_string, const std::set<std::string> &charsets)
  : MySQLRecognitionBase(charsets) {
  d = new Private();

  d->_text = text;
  d->_text_length = length;
  d->_context.version = server_version;
  d->_context.payload = this;
  set_sql_mode(sql_mode_string);

  // If the text is not using utf-8 (which it should) then we interpret as 8bit encoding
  // (everything requiring only one byte per char as Latin1, ASCII and similar).
  d->_input_encoding = is_utf8 ? ANTLR3_ENC_UTF8 : ANTLR3_ENC_8BIT;
  setup();

  // Cache the tokens. There's always at least one token: the EOF token.
  // It might seem counter productive to load all tokens upfront, but this makes many
  // things a lot simpler or even possible. The token stream used by a parser does exactly the same.
  d->_token_index = 0;
  while (true) {
    pANTLR3_COMMON_TOKEN token = d->_token_source->nextToken(d->_token_source);
    d->_tokens.push_back(token);
    if (token->type == ANTLR3_TOKEN_EOF)
      break;
  }
}

//--------------------------------------------------------------------------------------------------

MySQLScanner::~MySQLScanner() {
  d->_lexer->free(d->_lexer);
  d->_input->close(d->_input);

  delete d;
}

//--------------------------------------------------------------------------------------------------

void MySQLScanner::reset() {
  d->_token_index = 0;
}

//--------------------------------------------------------------------------------------------------

/**
 * All token information in one call.
 */
ParserToken MySQLScanner::token() {
  pANTLR3_COMMON_TOKEN token = d->_tokens[d->_token_index];

  ParserToken result;
  if (token != NULL) {
    result.type = token->type;
    result.line = token->line;
    result.position = token->charPosition;
    result.index = token->index;
    result.channel = token->channel;
    result.line_start = (char *)token->lineStart;
    result.start = reinterpret_cast<char *>(token->start);
    result.stop = reinterpret_cast<char *>(token->stop);

    pANTLR3_STRING text = token->getText(token);
    result.text = (const char *)text->chars;
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

uint32_t MySQLScanner::token_type() {
  pANTLR3_COMMON_TOKEN token = d->_tokens[d->_token_index];
  return token->type;
}

//--------------------------------------------------------------------------------------------------

uint32_t MySQLScanner::token_line() {
  pANTLR3_COMMON_TOKEN token = d->_tokens[d->_token_index];
  return token->line;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the byte index of the start of token in the input string.
 */
size_t MySQLScanner::token_start() {
  pANTLR3_COMMON_TOKEN token = d->_tokens[d->_token_index];
  return token->charPosition;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the byte index directly following the last character of the token in the input string.
 */
size_t MySQLScanner::token_end() {
  pANTLR3_COMMON_TOKEN token = d->_tokens[d->_token_index];
  return token->charPosition + (token->stop - token->start) + 1;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLScanner::token_channel() {
  pANTLR3_COMMON_TOKEN token = d->_tokens[d->_token_index];
  return token->channel;
}

//--------------------------------------------------------------------------------------------------

std::string MySQLScanner::token_text() {
  pANTLR3_COMMON_TOKEN token = d->_tokens[d->_token_index];
  pANTLR3_STRING text = token->getText(token);
  return (const char *)text->chars;
}

//--------------------------------------------------------------------------------------------------

void MySQLScanner::next(bool skip_hidden) {
  while (d->_token_index < d->_tokens.size() - 1) {
    ++d->_token_index;
    if (d->_tokens[d->_token_index]->channel == 0 || !skip_hidden)
      break;
  }
}

//--------------------------------------------------------------------------------------------------

void MySQLScanner::previous(bool skip_hidden) {
  pANTLR3_COMMON_TOKEN token;
  while (d->_token_index > 0) {
    --d->_token_index;
    if (d->_tokens[d->_token_index]->channel == 0 || !skip_hidden)
      break;
  }

  // May set the index to a node on another than the default channel, if the very first token is hidden.
}

//--------------------------------------------------------------------------------------------------

/**
 * Advances to the next token if the current is of the given type. Returns true if that is the case.
 */
bool MySQLScanner::skipIf(uint32_t token) {
  if (d->_tokens[d->_token_index]->type == token) {
    next();
    return true;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the position in the token vector.
 */
size_t MySQLScanner::position() {
  return d->_token_index;
}

//--------------------------------------------------------------------------------------------------

/**
 * Sets the position in the token vector to the given index.
 */
void MySQLScanner::seek(size_t position) {
  d->_token_index = position;
  if (position >= d->_tokens.size())
    d->_token_index = d->_tokens.size() - 1;
}

//--------------------------------------------------------------------------------------------------

/**
 * Scans the token list from the beginning and moves to the token that covers the given
 * caret position. Line is one-based and offset is the zero-based offset in that line.
 */
void MySQLScanner::seek(size_t line, size_t offset) {
  // When scanning keep in mind a token can span more than one line (think of multi-line comments).
  // A simple pos + length computation doesn't cut it.
  d->_token_index = 0;
  if (d->_tokens[d->_token_index]->type == ANTLR3_TOKEN_EOF)
    return;

  while (true) {
    // When we reach the input end check if the current token can alone separate from the next token.
    // Some tokens require a whitespace for separation, some do not.
    pANTLR3_COMMON_TOKEN token = d->_tokens[d->_token_index + 1];
    if (token->type == ANTLR3_TOKEN_EOF) {
      // At the end of the input. We have no more tokens to make the lookahead work (nothing after EOF).
      // Instead we define: if the last good token is a separator and the caret is at its end
      // we consider the EOF as the current token (a separator is always only 1 char long).
      if (is_separator() && (size_t)d->_tokens[d->_token_index]->charPosition < offset)
        ++d->_token_index;
      break;
    }

    if (token->line > line)
      break;

    if (token->line == line && (size_t)token->charPosition > offset)
      break;

    ++d->_token_index;
  };
}

//--------------------------------------------------------------------------------------------------

/**
 * Looks forward (offset > 0) or backward (offset < 0) and returns the token at the given position.
 */
uint32_t MySQLScanner::look_around(int offset, bool ignore_hidden) {
  if (offset == 0)
    return d->_tokens[d->_token_index]->type;

  ssize_t index = (ssize_t)d->_token_index;
  if (index + offset < 0 || index + offset >= (ssize_t)d->_tokens.size())
    return ANTLR3_TOKEN_INVALID;

  pANTLR3_COMMON_TOKEN token;
  if (offset < 0) {
    while (index > 0 && offset < 0) {
      ++offset;
      if (ignore_hidden) {
        while (--index >= 0 && d->_tokens[index]->channel != 0)
          ;
      } else
        --index;
    }

    if (offset < 0) // Not enough non-hidden entries.
      return ANTLR3_TOKEN_INVALID;
    return d->_tokens[index]->type;
  } else {
    ssize_t count = (ssize_t)d->_tokens.size() - 1; // -1 because the last node is always EOF.
    while (index < count && offset > 0) {
      --offset;
      if (ignore_hidden) {
        while (++index < count && d->_tokens[index]->channel != 0)
          ;
      } else
        ++index;
    }

    if (offset > 0) // Not enough non-hidden entries.
      return ANTLR3_TOKEN_INVALID;
    return d->_tokens[index]->type;
  }
}

//--------------------------------------------------------------------------------------------------

bool MySQLScanner::is(uint32_t type) {
  return d->_tokens[d->_token_index]->type == type;
}

//--------------------------------------------------------------------------------------------------

bool MySQLScanner::is_keyword() {
  return isKeyword(d->_tokens[d->_token_index]->type);
}

//--------------------------------------------------------------------------------------------------

bool MySQLScanner::is_relation() {
  return MySQLRecognitionBase::is_relation(d->_tokens[d->_token_index]->type);
}

//--------------------------------------------------------------------------------------------------

bool MySQLScanner::is_number() {
  return MySQLRecognitionBase::is_number(d->_tokens[d->_token_index]->type);
}

//--------------------------------------------------------------------------------------------------

bool MySQLScanner::is_operator() {
  return MySQLRecognitionBase::is_operator(d->_tokens[d->_token_index]->type);
}

//--------------------------------------------------------------------------------------------------

bool MySQLScanner::is_identifier() {
  return isIdentifier(d->_tokens[d->_token_index]->type);
}

//--------------------------------------------------------------------------------------------------

/**
 * Determines if the current token is alone a separator, that is, a character that can separate
 * tokens without whitespaces. Typical tokens are operators (comma, semicolon, parentheses etc.).
 */
bool MySQLScanner::is_separator() {
  uint32_t type = d->_tokens[d->_token_index]->type;
  switch (type) {
    case EQUAL_OPERATOR:
    case ASSIGN_OPERATOR:
    case NULL_SAFE_EQUAL_OPERATOR:
    case GREATER_OR_EQUAL_OPERATOR:
    case GREATER_THAN_OPERATOR:
    case LESS_OR_EQUAL_OPERATOR:
    case LESS_THAN_OPERATOR:
    case NOT_EQUAL_OPERATOR:
    case NOT_EQUAL2_OPERATOR:
    case PLUS_OPERATOR:
    case MINUS_OPERATOR:
    case MULT_OPERATOR:
    case DIV_OPERATOR:
    case MOD_OPERATOR:
    case LOGICAL_NOT_OPERATOR:
    case BITWISE_NOT_OPERATOR:
    case SHIFT_LEFT_OPERATOR:
    case SHIFT_RIGHT_OPERATOR:
    case LOGICAL_AND_OPERATOR:
    case BITWISE_AND_OPERATOR:
    case BITWISE_XOR_OPERATOR:
    case LOGICAL_OR_OPERATOR:
    case BITWISE_OR_OPERATOR:
    case DOT_SYMBOL:
    case COMMA_SYMBOL:
    case SEMICOLON_SYMBOL:
    case COLON_SYMBOL:
    case OPEN_PAR_SYMBOL:
    case CLOSE_PAR_SYMBOL:
    case OPEN_CURLY_SYMBOL:
    case CLOSE_CURLY_SYMBOL:
    case PARAM_MARKER:
      return true;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

void MySQLScanner::setup() {
  logDebug2("Lexer setup\n");

  d->_input = antlr3StringStreamNew((pANTLR3_UINT8)d->_text, d->_input_encoding, (ANTLR3_UINT32)d->_text_length,
                                    (pANTLR3_UINT8) "mysql-script");
  d->_input->setUcaseLA(
    d->_input, ANTLR3_TRUE); // Make input case-insensitive. String literals must all be upper case in the grammar!

  d->_lexer = MySQLLexerNew(d->_input);
  d->_lexer->pLexer->rec->state->userp = &d->_context;
  d->_token_source = TOKENSOURCE(d->_lexer);

  logDebug2("Lexer setup ended\n");
}

//--------------------------------------------------------------------------------------------------

std::string MySQLScanner::text() const {
  return std::string(d->_text, d->_text_length);
}

//--------------------------------------------------------------------------------------------------

const char *MySQLScanner::lineStart() const {
  return d->_text;
}

//--------------------------------------------------------------------------------------------------

void MySQLScanner::set_server_version(long version) {
  d->_context.version = version;
}

//--------------------------------------------------------------------------------------------------

long MySQLScanner::get_server_version() const {
  return d->_context.version;
}

//--------------------------------------------------------------------------------------------------

void MySQLScanner::set_sql_mode(const std::string &new_mode) {
  MySQLRecognitionBase::set_sql_mode(new_mode);
  d->_context.sqlMode = sql_mode(); // Parsed SQL mode.
}

//--------------------------------------------------------------------------------------------------

unsigned int MySQLScanner::get_sql_mode_flags() const {
  return d->_context.sqlMode;
}

//------------------ MySQLQueryIdentifier ----------------------------------------------------------

class MySQLQueryIdentifier::Private {
public:
  RecognitionContext _context;
};

MySQLQueryIdentifier::MySQLQueryIdentifier(long serverVersion, const std::string &sqlModeString,
                                           const std::set<std::string> &charsets)
  : MySQLRecognitionBase(charsets) {
  d = new Private();

  d->_context.version = serverVersion;
  d->_context.payload = this;
  set_sql_mode(sqlModeString);
}

//--------------------------------------------------------------------------------------------------

MySQLQueryIdentifier::MySQLQueryIdentifier(long serverVersion, unsigned sqlMode, const std::set<std::string> &charsets)
  : MySQLRecognitionBase(charsets) {
  d = new Private();

  d->_context.version = serverVersion;
  d->_context.payload = this;
  set_sql_mode(sqlMode);
}

//--------------------------------------------------------------------------------------------------

MySQLQueryIdentifier::~MySQLQueryIdentifier() {
  delete d;
}

//--------------------------------------------------------------------------------------------------

/**
*	Helper for the query type determination.
*/
static bool nextToken(pANTLR3_TOKEN_SOURCE tokenSource, pANTLR3_COMMON_TOKEN &token) {
  do {
    token = tokenSource->nextToken(tokenSource);
    if (token == NULL)
      return false;

    if (token->type == ANTLR3_TOKEN_EOF) {
      token = NULL;
      return false;
    }
  } while (token->channel != 0); // Skip hidden tokens.

  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 *	Skips over a definer clause if possible. Returns true if it was successful and points to the
 *	token after the last definer part.
 *	On entry the DEFINER symbol has been already consumed.
 *	If the syntax is wrong false is returned and the token source state is undetermined.
 */
bool MySQLQueryIdentifier::skipDefiner(pANTLR3_TOKEN_SOURCE tokenSource, pANTLR3_COMMON_TOKEN &token) {
  if (!nextToken(tokenSource, token))
    return false;

  if (token->type != EQUAL_OPERATOR)
    return false;

  if (!nextToken(tokenSource, token))
    return false;

  if (token->type == CURRENT_USER_SYMBOL) {
    if (!nextToken(tokenSource, token))
      return false;
    if (token->type == OPEN_PAR_SYMBOL) {
      if (!nextToken(tokenSource, token))
        return false;
      if (token->type != CLOSE_PAR_SYMBOL)
        return false;
      if (!nextToken(tokenSource, token))
        return false;
    }
    return true;
  }

  if (token->type == SINGLE_QUOTED_TEXT || isIdentifier(token->type)) {
    // First part of the user definition (mandatory).
    if (!nextToken(tokenSource, token))
      return false;

    if (token->type == AT_SIGN_SYMBOL || token->type == AT_TEXT_SUFFIX) {
      // Second part of the user definition (optional).
      bool needIdentifier = token->type == AT_SIGN_SYMBOL;
      if (!nextToken(tokenSource, token))
        return false;

      if (needIdentifier) {
        if (!isIdentifier(token->type) && token->type != SINGLE_QUOTED_TEXT)
          return false;
        if (!nextToken(tokenSource, token))
          return false;
      }
    }

    return true;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

/**
*	Helper for the query type determination.
*/
static bool isTokenText(pANTLR3_COMMON_TOKEN token, const std::string &expected) {
  pANTLR3_STRING text = token->getText(token);
  if (text == nullptr)
    return false;

  return base::tolower((const char *)text->chars) == expected;
}

//--------------------------------------------------------------------------------------------------

MySQLQueryType MySQLQueryIdentifier::determineQueryType(pANTLR3_TOKEN_SOURCE tokenSource) {
  pANTLR3_COMMON_TOKEN token;
  if (!nextToken(tokenSource, token))
    return QtUnknown;

  switch (token->type) {
    case ALTER_SYMBOL:
      if (!nextToken(tokenSource, token))
        return QtAmbiguous;

      switch (token->type) {
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

        case DEFINER_SYMBOL: // Can be both event or view.
          if (!skipDefiner(tokenSource, token))
            return QtAmbiguous;

          switch (token->type) {
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
      if (!nextToken(tokenSource, token))
        return QtAmbiguous;

      switch (token->type) {
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
          if (!skipDefiner(tokenSource, token))
            return QtAmbiguous;

          switch (token->type) {
            case EVENT_SYMBOL:
              return QtCreateEvent;

            case VIEW_SYMBOL:
            case SQL_SYMBOL:
              return QtCreateView;

            case PROCEDURE_SYMBOL:
              return QtCreateProcedure;

            case FUNCTION_SYMBOL: {
              if (!nextToken(tokenSource, token))
                return QtAmbiguous;

              if (token->type == UDF_NAME_TOKEN)
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
      if (!nextToken(tokenSource, token))
        return QtAmbiguous;

      switch (token->type) {
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
      if (!nextToken(tokenSource, token))
        return QtAmbiguous;

      switch (token->type) {
        case DATA_SYMBOL: {
          if (!nextToken(tokenSource, token))
            return QtAmbiguous;

          if (token->type == FROM_SYMBOL)
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
      while (token->type == OPEN_PAR_SYMBOL) {
        if (!nextToken(tokenSource, token))
          return QtAmbiguous;
      }
      if (token->type == SELECT_SYMBOL)
        return QtSelect;
      return QtPartition;
    }

    case PARTITION_SYMBOL:
    case PARTITIONS_SYMBOL:
      return QtPartition;

    case START_SYMBOL: {
      if (!nextToken(tokenSource, token))
        return QtAmbiguous;

      if (token->type == TRANSACTION_SYMBOL)
        return QtStartTransaction;
      return QtStartSlave;
    }

    case BEGIN_SYMBOL: // Begin directly at the start of the query must be a transaction start.
      return QtBeginWork;

    case COMMIT_SYMBOL:
      return QtCommit;

    case ROLLBACK_SYMBOL: {
      // We assume a transaction statement here unless we exactly know it's about a savepoint.
      if (!nextToken(tokenSource, token))
        return QtRollbackWork;
      if (token->type == WORK_SYMBOL) {
        if (!nextToken(tokenSource, token))
          return QtRollbackWork;
      }

      if (token->type == TO_SYMBOL)
        return QtRollbackSavepoint;
      return QtRollbackWork;
    }

    case SET_SYMBOL: {
      if (!nextToken(tokenSource, token))
        return QtSet;

      switch (token->type) {
        case PASSWORD_SYMBOL:
          return QtSetPassword;

        case GLOBAL_SYMBOL:
        case LOCAL_SYMBOL:
        case SESSION_SYMBOL:
          if (!nextToken(tokenSource, token))
            return QtSet;
          break;

        case IDENTIFIER:
          if (isTokenText(token, "autocommit"))
            return QtSetAutoCommit;
          break;
      }

      if (token->type == TRANSACTION_SYMBOL)
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
      if (!nextToken(tokenSource, token))
        return QtReset;

      switch (token->type) {
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
      if (!nextToken(tokenSource, token))
        return QtAmbiguous;

      if (token->type == PROXY_SYMBOL)
        return QtGrantProxy;
      return QtGrant;
    }

    case RENAME_SYMBOL: {
      if (!nextToken(tokenSource, token))
        return QtAmbiguous;

      if (token->type == USER_SYMBOL)
        return QtRenameUser;
      return QtRenameTable;
    }

    case REVOKE_SYMBOL: {
      if (!nextToken(tokenSource, token))
        return QtAmbiguous;

      if (token->type == PROXY_SYMBOL)
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
      if (!nextToken(tokenSource, token))
        return QtShow;

      if (token->type == FULL_SYMBOL) {
        // Not all SHOW cases allow an optional FULL keyword, but this is not about checking for
        // a valid query but to find the most likely type.
        if (!nextToken(tokenSource, token))
          return QtShow;
      }

      switch (token->type) {
        case GLOBAL_SYMBOL:
        case LOCK_SYMBOL:
        case SESSION_SYMBOL: {
          if (!nextToken(tokenSource, token))
            return QtShow;

          if (token->type == STATUS_SYMBOL)
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
          if (!nextToken(tokenSource, token) || token->type != OPEN_PAR_SYMBOL)
            return QtShow;
          if (!nextToken(tokenSource, token) || token->type != MULT_OPERATOR)
            return QtShow;
          if (!nextToken(tokenSource, token) || token->type != CLOSE_PAR_SYMBOL)
            return QtShow;

          if (!nextToken(tokenSource, token))
            return QtShow;

          switch (token->type) {
            case WARNINGS_SYMBOL:
              return QtShowWarnings;

            case ERRORS_SYMBOL:
              return QtShowErrors;
          }

          return QtShow;
        }

        case CREATE_SYMBOL: {
          if (!nextToken(tokenSource, token))
            return QtShow;

          switch (token->type) {
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
          if (!nextToken(tokenSource, token))
            return QtAmbiguous;

          if (token->type == CODE_SYMBOL)
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
          if (!nextToken(tokenSource, token))
            return QtShow;

          if (token->type == STATUS_SYMBOL)
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
          if (!nextToken(tokenSource, token))
            return QtAmbiguous;

          if (token->type == HOSTS_SYMBOL)
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
      if (!nextToken(tokenSource, token))
        return QtAmbiguous;

      if (isIdentifier(token->type) || token->type == DOT_SYMBOL)
        return QtExplainTable;

      // EXTENDED is a bit special as it can be both, a table identifier or the keyword.
      if (token->type == EXTENDED_SYMBOL) {
        if (!nextToken(tokenSource, token))
          return QtExplainTable;

        switch (token->type) {
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

//--------------------------------------------------------------------------------------------------

/**
 *	A lightweight function to determine the type of the given query by scanning only the absolute
 *	minimum text to make a funded decision.
 */
MySQLQueryType MySQLQueryIdentifier::getQueryType(const char *text, size_t length, bool is_utf8) {
  logDebug2("Starting query type determination\n");

  pANTLR3_INPUT_STREAM input = antlr3StringStreamNew((pANTLR3_UINT8)text, is_utf8 ? ANTLR3_ENC_UTF8 : ANTLR3_ENC_8BIT,
                                                     (ANTLR3_UINT32)length, (pANTLR3_UINT8) "type-check");
  input->setUcaseLA(input, ANTLR3_TRUE);
  pMySQLLexer lexer = MySQLLexerNew(input);

  // Reset temp vars used during lexing. We may not have scanned the tokens that reset those
  // as we do only a minimum number of token retrievals.
  d->_context.inVersionComment = false;
  d->_context.versionMatched = false;
  lexer->pLexer->rec->state->userp = &d->_context;

  MySQLQueryType result = determineQueryType(TOKENSOURCE(lexer));

  lexer->free(lexer);
  input->close(input);

  logDebug2("Query type determination done\n");
  return result;
}

//--------------------------------------------------------------------------------------------------
