/*
 * Copyright (c) 2016, 2021, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation. The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "CommonToken.h"

#include "MySQLLexer.h"
#include "MySQLBaseLexer.h"

#include "base/symbol-info.h"

using namespace base;
using namespace antlr4;
using namespace parsers;

//----------------------------------------------------------------------------------------------------------------------

MySQLBaseLexer::MySQLBaseLexer(CharStream *input) : Lexer(input) {
  serverVersion = 0;
  sqlMode = NoMode;
  inVersionComment = false;
}

//----------------------------------------------------------------------------------------------------------------------

void MySQLBaseLexer::reset() {
  inVersionComment = false;
  Lexer::reset();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns true if the given token is an identifier. This includes all those keywords that are
 * allowed as identifiers when unquoted (non-reserved keywords).
 */
bool MySQLBaseLexer::isIdentifier(size_t type) const {
  if (type == MySQLLexer::EOF)
    return false;

  if ((type == MySQLLexer::IDENTIFIER) || (type == MySQLLexer::BACK_TICK_QUOTED_ID))
    return true;

  // Double quoted text represents identifiers only if the ANSI QUOTES sql mode is active.
  if (((sqlMode & AnsiQuotes) != 0) && (type == MySQLLexer::DOUBLE_QUOTED_TEXT))
    return true;

  std::string symbol = getVocabulary().getSymbolicName(type).data();
  if (!symbol.empty() && !MySQLSymbolInfo::isReservedKeyword(symbol, MySQLSymbolInfo::numberToVersion(serverVersion)))
    return true;

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

size_t MySQLBaseLexer::keywordFromText(std::string const& name) {
  // (My)SQL only uses ASCII chars for keywords so we can do a simple downcase here for comparison.
  std::string transformed;
  std::transform(name.begin(), name.end(), std::back_inserter(transformed), ::tolower);

  if (!MySQLSymbolInfo::isKeyword(transformed, MySQLSymbolInfo::numberToVersion(serverVersion)))
    return INVALID_INDEX - 1; // INVALID_INDEX alone can be interpreted as EOF.

  // Generate string -> enum value map, if not yet done.
  if (_symbols.empty()) {
    auto &vocabulary = getVocabulary();
    size_t max = vocabulary.getMaxTokenType();
    for (size_t i = 0; i <= max; ++i)
      _symbols[vocabulary.getSymbolicName(i).data()] = i;
  }

  // Here we know for sure we got a keyword.
  auto symbol = _symbols.find(transformed);
  if (symbol == _symbols.end())
    return INVALID_INDEX - 1;
  return symbol->second;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 *  Helper for the query type determination.
 */
std::unique_ptr<antlr4::Token> MySQLBaseLexer::nextDefaultChannelToken() {
  do {
    std::unique_ptr<Token> token = nextToken();
    if (token->getChannel() == ParserToken::DEFAULT_CHANNEL)
      return token;
  } while (true);
}

//--------------------------------------------------------------------------------------------------

/**
 *  Skips over a definer clause if possible. Returns true if it was successful and points to the
 *  token after the last definer part.
 *  On entry the DEFINER symbol has been already consumed.
 *  If the syntax is wrong false is returned and the token source state is undetermined.
 */
bool MySQLBaseLexer::skipDefiner(std::unique_ptr<antlr4::Token> &token) {
  token = nextDefaultChannelToken();
  if (token->getType() != MySQLLexer::EQUAL_OPERATOR)
    return false;

  token = nextDefaultChannelToken();
  if (token->getType() == MySQLLexer::CURRENT_USER_SYMBOL) {
    token = nextDefaultChannelToken();
    if (token->getType() == MySQLLexer::OPEN_PAR_SYMBOL) {
      token = nextDefaultChannelToken();
      if (token->getType() != MySQLLexer::CLOSE_PAR_SYMBOL)
        return false;
      token = nextDefaultChannelToken();
      if (token->getType() == Token::EOF)
        return false;
    }
    return true;
  }

  if (token->getType() == MySQLLexer::SINGLE_QUOTED_TEXT || isIdentifier(token->getType())) {
    // First part of the user definition (mandatory).
    token = nextDefaultChannelToken();
    if (token->getType() == MySQLLexer::AT_SIGN_SYMBOL || token->getType() == MySQLLexer::AT_TEXT_SUFFIX) {
      // Second part of the user definition (optional).
      bool needIdentifier = token->getType() == MySQLLexer::AT_SIGN_SYMBOL;
      token = nextDefaultChannelToken();
      if (needIdentifier) {
        if (!isIdentifier(token->getType()) && token->getType() != MySQLLexer::SINGLE_QUOTED_TEXT)
          return false;
        token = nextDefaultChannelToken();
        if (token->getType() == Token::EOF)
          return false;
      }
    }

    return true;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

MySQLQueryType MySQLBaseLexer::determineQueryType() {
  std::unique_ptr<Token> token = nextDefaultChannelToken();
  if (token->getType() == Token::EOF)
    return QtUnknown;

  switch (token->getType()) {
    case MySQLLexer::ALTER_SYMBOL:
      token = nextDefaultChannelToken();
      if (token->getType() == Token::EOF)
        return QtAmbiguous;

      switch (token->getType()) {
        case MySQLLexer::DATABASE_SYMBOL:
          return QtAlterDatabase;

        case MySQLLexer::LOGFILE_SYMBOL:
          return QtAlterLogFileGroup;

        case MySQLLexer::FUNCTION_SYMBOL:
          return QtAlterFunction;

        case MySQLLexer::PROCEDURE_SYMBOL:
          return QtAlterProcedure;

        case MySQLLexer::SERVER_SYMBOL:
          return QtAlterServer;

        case MySQLLexer::TABLE_SYMBOL:
        case MySQLLexer::ONLINE_SYMBOL:  // Optional part of ALTER TABLE.
        case MySQLLexer::OFFLINE_SYMBOL: // ditto
        case MySQLLexer::IGNORE_SYMBOL:
          return QtAlterTable;

        case MySQLLexer::TABLESPACE_SYMBOL:
          return QtAlterTableSpace;

        case MySQLLexer::EVENT_SYMBOL:
          return QtAlterEvent;

        case MySQLLexer::VIEW_SYMBOL:
          return QtAlterView;

        case MySQLLexer::DEFINER_SYMBOL: // Can be both event or view.
          if (!skipDefiner(token))
            return QtAmbiguous;

          token = nextDefaultChannelToken();

          switch (token->getType()) {
            case MySQLLexer::EVENT_SYMBOL:
              return QtAlterEvent;

            case MySQLLexer::SQL_SYMBOL:
            case MySQLLexer::VIEW_SYMBOL:
              return QtAlterView;
          }
          break;

        case MySQLLexer::ALGORITHM_SYMBOL: // Optional part of CREATE VIEW.
          return QtAlterView;

        case MySQLLexer::USER_SYMBOL:
          return QtAlterUser;
      }
      break;

    case MySQLLexer::CREATE_SYMBOL: {
      token = nextDefaultChannelToken();
      if (token->getType() == Token::EOF)
        return QtAmbiguous;

      switch (token->getType()) {
        case MySQLLexer::TEMPORARY_SYMBOL: // Optional part of CREATE TABLE.
        case MySQLLexer::TABLE_SYMBOL:
          return QtCreateTable;

        case MySQLLexer::ONLINE_SYMBOL:
        case MySQLLexer::OFFLINE_SYMBOL:
        case MySQLLexer::INDEX_SYMBOL:
        case MySQLLexer::UNIQUE_SYMBOL:
        case MySQLLexer::FULLTEXT_SYMBOL:
        case MySQLLexer::SPATIAL_SYMBOL:
          return QtCreateIndex;

        case MySQLLexer::DATABASE_SYMBOL:
          return QtCreateDatabase;

        case MySQLLexer::TRIGGER_SYMBOL:
          return QtCreateTrigger;

        case MySQLLexer::DEFINER_SYMBOL: // Can be event, view, procedure, function, UDF, trigger.
        {
          if (!skipDefiner(token))
            return QtAmbiguous;

          switch (token->getType()) {
            case MySQLLexer::EVENT_SYMBOL:
              return QtCreateEvent;

            case MySQLLexer::VIEW_SYMBOL:
            case MySQLLexer::SQL_SYMBOL:
              return QtCreateView;

            case MySQLLexer::PROCEDURE_SYMBOL:
              return QtCreateProcedure;

            case MySQLLexer::FUNCTION_SYMBOL: {
              token = nextDefaultChannelToken();
              if (token->getType() == Token::EOF)
                return QtAmbiguous;

              if (!isIdentifier(token->getType()))
                return QtAmbiguous;

              token = nextDefaultChannelToken();
              if (token->getType() == MySQLLexer::RETURNS_SYMBOL)
                return QtCreateUdf;

              return QtCreateFunction;
            }

            case MySQLLexer::AGGREGATE_SYMBOL:
              return QtCreateUdf;

            case MySQLLexer::TRIGGER_SYMBOL:
              return QtCreateTrigger;
          }
        }

        case MySQLLexer::VIEW_SYMBOL:
        case MySQLLexer::OR_SYMBOL:        // CREATE OR REPLACE ... VIEW
        case MySQLLexer::ALGORITHM_SYMBOL: // CREATE ALGORITHM ... VIEW
          return QtCreateView;

        case MySQLLexer::EVENT_SYMBOL:
          return QtCreateEvent;

        case MySQLLexer::FUNCTION_SYMBOL:
          return QtCreateFunction;

        case MySQLLexer::AGGREGATE_SYMBOL:
          return QtCreateUdf;

        case MySQLLexer::PROCEDURE_SYMBOL:
          return QtCreateProcedure;

        case MySQLLexer::LOGFILE_SYMBOL:
          return QtCreateLogFileGroup;

        case MySQLLexer::SERVER_SYMBOL:
          return QtCreateServer;

        case MySQLLexer::TABLESPACE_SYMBOL:
          return QtCreateTableSpace;

        case MySQLLexer::USER_SYMBOL:
          return QtCreateUser;
      }
      break;
    }

    case MySQLLexer::DROP_SYMBOL: {
      token = nextDefaultChannelToken();
      if (token->getType() == Token::EOF)
        return QtAmbiguous;

      switch (token->getType()) {
        case MySQLLexer::DATABASE_SYMBOL:
          return QtDropDatabase;

        case MySQLLexer::EVENT_SYMBOL:
          return QtDropEvent;

        case MySQLLexer::PROCEDURE_SYMBOL:
          return QtDropProcedure;

        case MySQLLexer::FUNCTION_SYMBOL:
          return QtDropFunction;

        case MySQLLexer::ONLINE_SYMBOL:
        case MySQLLexer::OFFLINE_SYMBOL:
        case MySQLLexer::INDEX_SYMBOL:
          return QtDropIndex;

        case MySQLLexer::LOGFILE_SYMBOL:
          return QtDropLogfileGroup;

        case MySQLLexer::SERVER_SYMBOL:
          return QtDropServer;

        case MySQLLexer::TEMPORARY_SYMBOL:
        case MySQLLexer::TABLE_SYMBOL:
        case MySQLLexer::TABLES_SYMBOL:
          return QtDropTable;

        case MySQLLexer::TABLESPACE_SYMBOL:
          return QtDropTablespace;

        case MySQLLexer::TRIGGER_SYMBOL:
          return QtDropTrigger;

        case MySQLLexer::VIEW_SYMBOL:
          return QtDropView;

        case MySQLLexer::PREPARE_SYMBOL:
          return QtDeallocate;

        case MySQLLexer::USER_SYMBOL:
          return QtDropUser;
      }
    }

    case MySQLLexer::TRUNCATE_SYMBOL:
      return QtTruncateTable;

    case MySQLLexer::CALL_SYMBOL:
      return QtCall;

    case MySQLLexer::DELETE_SYMBOL:
      return QtDelete;

    case MySQLLexer::DO_SYMBOL:
      return QtDo;

    case MySQLLexer::HANDLER_SYMBOL:
      return QtHandler;

    case MySQLLexer::INSERT_SYMBOL:
      return QtInsert;

    case MySQLLexer::LOAD_SYMBOL: {
      token = nextDefaultChannelToken();
      if (token->getType() == Token::EOF)
        return QtAmbiguous;

      switch (token->getType()) {
        case MySQLLexer::DATA_SYMBOL: {
          token = nextDefaultChannelToken();
          if (token->getType() == Token::EOF)
            return QtAmbiguous;

          if (token->getType() == MySQLLexer::FROM_SYMBOL)
            return QtLoadDataMaster;
          return QtLoadData;
        }
        case MySQLLexer::XML_SYMBOL:
          return QtLoadXML;

        case MySQLLexer::TABLE_SYMBOL:
          return QtLoadTableMaster;

        case MySQLLexer::INDEX_SYMBOL:
          return QtLoadIndex;
      }
    }

    case MySQLLexer::REPLACE_SYMBOL:
      return QtReplace;

    case MySQLLexer::SELECT_SYMBOL:
      return QtSelect;

    case MySQLLexer::TABLE_SYMBOL:
      return QtTable;

    case MySQLLexer::VALUES_SYMBOL:
      return QtValues;

    case MySQLLexer::UPDATE_SYMBOL:
      return QtUpdate;

    case MySQLLexer::OPEN_PAR_SYMBOL: // Either (((select ..))) or (partition...)
    {
      while (token->getType() == MySQLLexer::OPEN_PAR_SYMBOL) {
        token = nextDefaultChannelToken();
        if (token->getType() == Token::EOF)
          return QtAmbiguous;
      }
      if (token->getType() == MySQLLexer::SELECT_SYMBOL)
        return QtSelect;
      return QtPartition;
    }

    case MySQLLexer::PARTITION_SYMBOL:
    case MySQLLexer::PARTITIONS_SYMBOL:
      return QtPartition;

    case MySQLLexer::START_SYMBOL: {
      token = nextDefaultChannelToken();
      if (token->getType() == Token::EOF)
        return QtAmbiguous;

      if (token->getType() == MySQLLexer::TRANSACTION_SYMBOL)
        return QtStartTransaction;
      return QtStartSlave;
    }

    case MySQLLexer::BEGIN_SYMBOL: // Begin directly at the start of the query must be a transaction start.
      return QtBeginWork;

    case MySQLLexer::COMMIT_SYMBOL:
      return QtCommit;

    case MySQLLexer::ROLLBACK_SYMBOL: {
      // We assume a transaction statement here unless we exactly know it's about a savepoint.
      token = nextDefaultChannelToken();
      if (token->getType() == Token::EOF)
        return QtRollbackWork;
      if (token->getType() == MySQLLexer::WORK_SYMBOL) {
        token = nextDefaultChannelToken();
        if (token->getType() == Token::EOF)
          return QtRollbackWork;
      }

      if (token->getType() == MySQLLexer::TO_SYMBOL)
        return QtRollbackSavepoint;
      return QtRollbackWork;
    }

    case MySQLLexer::SET_SYMBOL: {
      token = nextDefaultChannelToken();
      if (token->getType() == Token::EOF)
        return QtSet;

      switch (token->getType()) {
        case MySQLLexer::PASSWORD_SYMBOL:
          return QtSetPassword;

        case MySQLLexer::GLOBAL_SYMBOL:
        case MySQLLexer::LOCAL_SYMBOL:
        case MySQLLexer::SESSION_SYMBOL:
          token = nextDefaultChannelToken();
          if (token->getType() == Token::EOF)
            return QtSet;
          break;

        case MySQLLexer::IDENTIFIER: {
          std::string text = token->getText();
          std::transform(text.begin(), text.end(), text.begin(), ::tolower);
          if (text == "autocommit")
            return QtSetAutoCommit;
          break;
        }
      }

      if (token->getType() == MySQLLexer::TRANSACTION_SYMBOL)
        return QtSetTransaction;
      return QtSet;
    }

    case MySQLLexer::SAVEPOINT_SYMBOL:
      return QtSavepoint;

    case MySQLLexer::RELEASE_SYMBOL: // Release at the start of the query, obviously.
      return QtReleaseSavepoint;

    case MySQLLexer::LOCK_SYMBOL:
      return QtLock;

    case MySQLLexer::UNLOCK_SYMBOL:
      return QtUnlock;

    case MySQLLexer::XA_SYMBOL:
      return QtXA;

    case MySQLLexer::PURGE_SYMBOL:
      return QtPurge;

    case MySQLLexer::CHANGE_SYMBOL:
      return QtChangeMaster;

    case MySQLLexer::RESET_SYMBOL: {
      token = nextDefaultChannelToken();
      if (token->getType() == Token::EOF)
        return QtReset;

      switch (token->getType()) {
        case MySQLLexer::SERVER_SYMBOL:
          return QtResetMaster;
        case MySQLLexer::SLAVE_SYMBOL:
          return QtResetSlave;
        default:
          return QtReset;
      }
    }

    case MySQLLexer::STOP_SYMBOL:
      return QtStopSlave;

    case MySQLLexer::PREPARE_SYMBOL:
      return QtPrepare;

    case MySQLLexer::EXECUTE_SYMBOL:
      return QtExecute;

    case MySQLLexer::DEALLOCATE_SYMBOL:
      return QtDeallocate;

    case MySQLLexer::GRANT_SYMBOL: {
      token = nextDefaultChannelToken();
      if (token->getType() == Token::EOF)
        return QtAmbiguous;

      if (token->getType() == MySQLLexer::PROXY_SYMBOL)
        return QtGrantProxy;
      return QtGrant;
    }

    case MySQLLexer::RENAME_SYMBOL: {
      token = nextDefaultChannelToken();
      if (token->getType() == Token::EOF)
        return QtAmbiguous;

      if (token->getType() == MySQLLexer::USER_SYMBOL)
        return QtRenameUser;
      return QtRenameTable;
    }

    case MySQLLexer::REVOKE_SYMBOL: {
      token = nextDefaultChannelToken();
      if (token->getType() == Token::EOF)
        return QtAmbiguous;

      if (token->getType() == MySQLLexer::PROXY_SYMBOL)
        return QtRevokeProxy;
      return QtRevoke;
    }

    case MySQLLexer::ANALYZE_SYMBOL:
      return QtAnalyzeTable;

    case MySQLLexer::CHECK_SYMBOL:
      return QtCheckTable;

    case MySQLLexer::CHECKSUM_SYMBOL:
      return QtChecksumTable;

    case MySQLLexer::OPTIMIZE_SYMBOL:
      return QtOptimizeTable;

    case MySQLLexer::REPAIR_SYMBOL:
      return QtRepairTable;

    case MySQLLexer::BACKUP_SYMBOL:
      return QtBackUpTable;

    case MySQLLexer::RESTORE_SYMBOL:
      return QtRestoreTable;

    case MySQLLexer::INSTALL_SYMBOL:
      return QtInstallPlugin;

    case MySQLLexer::UNINSTALL_SYMBOL:
      return QtUninstallPlugin;

    case MySQLLexer::SHOW_SYMBOL: {
      token = nextDefaultChannelToken();
      if (token->getType() == Token::EOF)
        return QtShow;

      if (token->getType() == MySQLLexer::FULL_SYMBOL) {
        // Not all SHOW cases allow an optional FULL keyword, but this is not about checking for
        // a valid query but to find the most likely type.
        token = nextDefaultChannelToken();
        if (token->getType() == Token::EOF)
          return QtShow;
      }

      switch (token->getType()) {
        case MySQLLexer::GLOBAL_SYMBOL:
        case MySQLLexer::LOCK_SYMBOL:
        case MySQLLexer::SESSION_SYMBOL: {
          token = nextDefaultChannelToken();
          if (token->getType() == Token::EOF)
            return QtShow;

          if (token->getType() == MySQLLexer::STATUS_SYMBOL)
            return QtShowStatus;
          return QtShowVariables;
        }

        case MySQLLexer::AUTHORS_SYMBOL:
          return QtShowAuthors;

        case MySQLLexer::BINARY_SYMBOL:
          return QtShowBinaryLogs;

        case MySQLLexer::BINLOG_SYMBOL:
          return QtShowBinlogEvents;

        case MySQLLexer::RELAYLOG_SYMBOL:
          return QtShowRelaylogEvents;

        case MySQLLexer::CHAR_SYMBOL:
          return QtShowCharset;

        case MySQLLexer::COLLATION_SYMBOL:
          return QtShowCollation;

        case MySQLLexer::COLUMNS_SYMBOL:
          return QtShowColumns;

        case MySQLLexer::CONTRIBUTORS_SYMBOL:
          return QtShowContributors;

        case MySQLLexer::COUNT_SYMBOL: {
          token = nextDefaultChannelToken();
          if (token->getType() != MySQLLexer::OPEN_PAR_SYMBOL)
            return QtShow;
          token = nextDefaultChannelToken();
          if (token->getType() != MySQLLexer::MULT_OPERATOR)
            return QtShow;
          token = nextDefaultChannelToken();
          if (token->getType() != MySQLLexer::CLOSE_PAR_SYMBOL)
            return QtShow;

          token = nextDefaultChannelToken();
          if (token->getType() == Token::EOF)
            return QtShow;

          switch (token->getType()) {
            case MySQLLexer::WARNINGS_SYMBOL:
              return QtShowWarnings;

            case MySQLLexer::ERRORS_SYMBOL:
              return QtShowErrors;
          }

          return QtShow;
        }

        case MySQLLexer::CREATE_SYMBOL: {
          token = nextDefaultChannelToken();
          if (token->getType() == Token::EOF)
            return QtShow;

          switch (token->getType()) {
            case MySQLLexer::DATABASE_SYMBOL:
              return QtShowCreateDatabase;

            case MySQLLexer::EVENT_SYMBOL:
              return QtShowCreateEvent;

            case MySQLLexer::FUNCTION_SYMBOL:
              return QtShowCreateFunction;

            case MySQLLexer::PROCEDURE_SYMBOL:
              return QtShowCreateProcedure;

            case MySQLLexer::TABLE_SYMBOL:
              return QtShowCreateTable;

            case MySQLLexer::TRIGGER_SYMBOL:
              return QtShowCreateTrigger;

            case MySQLLexer::VIEW_SYMBOL:
              return QtShowCreateView;
          }

          return QtShow;
        }

        case MySQLLexer::DATABASES_SYMBOL:
          return QtShowDatabases;

        case MySQLLexer::ENGINE_SYMBOL:
          return QtShowEngineStatus;

        case MySQLLexer::STORAGE_SYMBOL:
        case MySQLLexer::ENGINES_SYMBOL:
          return QtShowStorageEngines;

        case MySQLLexer::ERRORS_SYMBOL:
          return QtShowErrors;

        case MySQLLexer::EVENTS_SYMBOL:
          return QtShowEvents;

        case MySQLLexer::FUNCTION_SYMBOL: {
          token = nextDefaultChannelToken();
          if (token->getType() == Token::EOF)
            return QtAmbiguous;

          if (token->getType() == MySQLLexer::CODE_SYMBOL)
            return QtShowFunctionCode;
          return QtShowFunctionStatus;
        }

        case MySQLLexer::GRANT_SYMBOL:
          return QtShowGrants;

        case MySQLLexer::INDEX_SYMBOL:
        case MySQLLexer::INDEXES_SYMBOL:
        case MySQLLexer::KEY_SYMBOL:
          return QtShowIndexes;

        case MySQLLexer::MASTER_SYMBOL:
          return QtShowMasterStatus;

        case MySQLLexer::OPEN_SYMBOL:
          return QtShowOpenTables;

        case MySQLLexer::PLUGIN_SYMBOL:
        case MySQLLexer::PLUGINS_SYMBOL:
          return QtShowPlugins;

        case MySQLLexer::PROCEDURE_SYMBOL: {
          token = nextDefaultChannelToken();
          if (token->getType() == Token::EOF)
            return QtShow;

          if (token->getType() == MySQLLexer::STATUS_SYMBOL)
            return QtShowProcedureStatus;
          return QtShowProcedureCode;
        }

        case MySQLLexer::PRIVILEGES_SYMBOL:
          return QtShowPrivileges;

        case MySQLLexer::PROCESSLIST_SYMBOL:
          return QtShowProcessList;

        case MySQLLexer::PROFILE_SYMBOL:
          return QtShowProfile;

        case MySQLLexer::PROFILES_SYMBOL:
          return QtShowProfiles;

        case MySQLLexer::SLAVE_SYMBOL: {
          token = nextDefaultChannelToken();
          if (token->getType() == Token::EOF)
            return QtAmbiguous;

          if (token->getType() == MySQLLexer::HOSTS_SYMBOL)
            return QtShowSlaveHosts;
          return QtShowSlaveStatus;
        }

        case MySQLLexer::STATUS_SYMBOL:
          return QtShowStatus;

        case MySQLLexer::VARIABLES_SYMBOL:
          return QtShowVariables;

        case MySQLLexer::TABLE_SYMBOL:
          return QtShowTableStatus;

        case MySQLLexer::TABLES_SYMBOL:
          return QtShowTables;

        case MySQLLexer::TRIGGERS_SYMBOL:
          return QtShowTriggers;

        case MySQLLexer::WARNINGS_SYMBOL:
          return QtShowWarnings;
      }

      return QtShow;
    }

    case MySQLLexer::CACHE_SYMBOL:
      return QtCacheIndex;

    case MySQLLexer::FLUSH_SYMBOL:
      return QtFlush;

    case MySQLLexer::KILL_SYMBOL:
      return QtKill;

    case MySQLLexer::DESCRIBE_SYMBOL: // EXPLAIN is converted to DESCRIBE in the lexer.
    case MySQLLexer::DESC_SYMBOL: {
      token = nextDefaultChannelToken();
      if (token->getType() == Token::EOF)
        return QtAmbiguous;

      if (isIdentifier(token->getType()) || token->getType() == MySQLLexer::DOT_SYMBOL)
        return QtExplainTable;

      // EXTENDED is a bit special as it can be both, a table identifier or the keyword.
      if (token->getType() == MySQLLexer::EXTENDED_SYMBOL) {
        token = nextDefaultChannelToken();
        if (token->getType() == Token::EOF)
          return QtExplainTable;

        switch (token->getType()) {
          case MySQLLexer::DELETE_SYMBOL:
          case MySQLLexer::INSERT_SYMBOL:
          case MySQLLexer::REPLACE_SYMBOL:
          case MySQLLexer::UPDATE_SYMBOL:
            return QtExplainStatement;
          default:
            return QtExplainTable;
        }
      }
      return QtExplainStatement;
    }

    case MySQLLexer::HELP_SYMBOL:
      return QtHelp;

    case MySQLLexer::USE_SYMBOL:
      return QtUse;
  }

  return QtUnknown;
}

//----------------------------------------------------------------------------------------------------------------------

bool MySQLBaseLexer::isRelation(size_t type) {
  switch (type) {
    case MySQLLexer::EQUAL_OPERATOR:
    case MySQLLexer::ASSIGN_OPERATOR:
    case MySQLLexer::NULL_SAFE_EQUAL_OPERATOR:
    case MySQLLexer::GREATER_OR_EQUAL_OPERATOR:
    case MySQLLexer::GREATER_THAN_OPERATOR:
    case MySQLLexer::LESS_OR_EQUAL_OPERATOR:
    case MySQLLexer::LESS_THAN_OPERATOR:
    case MySQLLexer::NOT_EQUAL_OPERATOR:
    case MySQLLexer::NOT_EQUAL2_OPERATOR:
    case MySQLLexer::PLUS_OPERATOR:
    case MySQLLexer::MINUS_OPERATOR:
    case MySQLLexer::MULT_OPERATOR:
    case MySQLLexer::DIV_OPERATOR:
    case MySQLLexer::MOD_OPERATOR:
    case MySQLLexer::LOGICAL_NOT_OPERATOR:
    case MySQLLexer::BITWISE_NOT_OPERATOR:
    case MySQLLexer::SHIFT_LEFT_OPERATOR:
    case MySQLLexer::SHIFT_RIGHT_OPERATOR:
    case MySQLLexer::LOGICAL_AND_OPERATOR:
    case MySQLLexer::BITWISE_AND_OPERATOR:
    case MySQLLexer::BITWISE_XOR_OPERATOR:
    case MySQLLexer::LOGICAL_OR_OPERATOR:
    case MySQLLexer::BITWISE_OR_OPERATOR:

    case MySQLLexer::OR_SYMBOL:
    case MySQLLexer::XOR_SYMBOL:
    case MySQLLexer::AND_SYMBOL:
    case MySQLLexer::IS_SYMBOL:
    case MySQLLexer::BETWEEN_SYMBOL:
    case MySQLLexer::LIKE_SYMBOL:
    case MySQLLexer::REGEXP_SYMBOL:
    case MySQLLexer::IN_SYMBOL:
    case MySQLLexer::SOUNDS_SYMBOL:
    case MySQLLexer::NOT_SYMBOL:
      return true;

    default:
      return false;
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool MySQLBaseLexer::isNumber(size_t type) {
  switch (type) {
    case MySQLLexer::INT_NUMBER:
    case MySQLLexer::LONG_NUMBER:
    case MySQLLexer::ULONGLONG_NUMBER:
    case MySQLLexer::FLOAT_NUMBER:
    case MySQLLexer::HEX_NUMBER:
    case MySQLLexer::BIN_NUMBER:
    case MySQLLexer::DECIMAL_NUMBER:
      return true;

    default:
      return false;
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool MySQLBaseLexer::isOperator(size_t type) {
  switch (type) {
    case MySQLLexer::EQUAL_OPERATOR:
    case MySQLLexer::ASSIGN_OPERATOR:
    case MySQLLexer::NULL_SAFE_EQUAL_OPERATOR:
    case MySQLLexer::GREATER_OR_EQUAL_OPERATOR:
    case MySQLLexer::GREATER_THAN_OPERATOR:
    case MySQLLexer::LESS_OR_EQUAL_OPERATOR:
    case MySQLLexer::LESS_THAN_OPERATOR:
    case MySQLLexer::NOT_EQUAL_OPERATOR:
    case MySQLLexer::NOT_EQUAL2_OPERATOR:
    case MySQLLexer::PLUS_OPERATOR:
    case MySQLLexer::MINUS_OPERATOR:
    case MySQLLexer::MULT_OPERATOR:
    case MySQLLexer::DIV_OPERATOR:
    case MySQLLexer::MOD_OPERATOR:
    case MySQLLexer::LOGICAL_NOT_OPERATOR:
    case MySQLLexer::BITWISE_NOT_OPERATOR:
    case MySQLLexer::SHIFT_LEFT_OPERATOR:
    case MySQLLexer::SHIFT_RIGHT_OPERATOR:
    case MySQLLexer::LOGICAL_AND_OPERATOR:
    case MySQLLexer::BITWISE_AND_OPERATOR:
    case MySQLLexer::BITWISE_XOR_OPERATOR:
    case MySQLLexer::LOGICAL_OR_OPERATOR:
    case MySQLLexer::BITWISE_OR_OPERATOR:

    case MySQLLexer::DOT_SYMBOL:
    case MySQLLexer::COMMA_SYMBOL:
    case MySQLLexer::SEMICOLON_SYMBOL:
    case MySQLLexer::COLON_SYMBOL:
    case MySQLLexer::OPEN_PAR_SYMBOL:
    case MySQLLexer::CLOSE_PAR_SYMBOL:
    case MySQLLexer::AT_SIGN_SYMBOL:
    case MySQLLexer::AT_AT_SIGN_SYMBOL:
    case MySQLLexer::PARAM_MARKER:
      return true;

    default:
      return false;
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Allow a grammar rule to emit as many tokens as it needs.
 */
std::unique_ptr<antlr4::Token> MySQLBaseLexer::nextToken() {
  // First respond with pending tokens to the next token request, if there are any.
  if (!_pendingTokens.empty()) {
    auto pending = std::move(_pendingTokens.front());
    _pendingTokens.pop_front();
    return pending;
  }

  // Let the main lexer class run the next token recognition.
  // This might create additional tokens again.
  auto next = Lexer::nextToken();
  if (!_pendingTokens.empty()) {
    auto pending = std::move(_pendingTokens.front());
    _pendingTokens.pop_front();
    _pendingTokens.push_back(std::move(next));
    return pending;
  }
  return next;
}

//----------------------------------------------------------------------------------------------------------------------

bool MySQLBaseLexer::checkVersion(const std::string &text) {
  if (text.size() < 8) // Minimum is: /*!12345
    return false;

  // Skip version comment introducer.
  long version = std::stoul(text.c_str() + 3, NULL, 10);
  if (version <= serverVersion) {
    inVersionComment = true;
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

size_t MySQLBaseLexer::determineFunction(size_t proposed) {
  // Skip any whitespace character if the sql mode says they should be ignored,
  // before actually trying to match the open parenthesis.
  if (isSqlModeActive(IgnoreSpace)) {
    size_t input = _input->LA(1);
    while (input == ' ' || input == '\t' || input == '\r' || input == '\n') {
      getInterpreter<atn::LexerATNSimulator>()->consume(_input);
      channel = HIDDEN;
      type = MySQLLexer::WHITESPACE;
      input = _input->LA(1);
    }
  }

  return _input->LA(1) == '(' ? proposed : MySQLLexer::IDENTIFIER;
}

//----------------------------------------------------------------------------------------------------------------------

size_t MySQLBaseLexer::determineNumericType(const std::string &text) {
  static const char *long_str = "2147483647";
  static const unsigned long_len = 10;
  static const char *signed_long_str = "-2147483648";
  static const char *longlong_str = "9223372036854775807";
  static const unsigned longlong_len = 19;
  static const char *signed_longlong_str = "-9223372036854775808";
  static const unsigned signed_longlong_len = 19;
  static const char *unsigned_longlong_str = "18446744073709551615";
  static const unsigned unsigned_longlong_len = 20;

  // The original code checks for leading +/- but actually that can never happen, neither in the
  // server parser (as a digit is used to trigger processing in the lexer) nor in our parser
  // as our rules are defined without signs. But we do it anyway for maximum compatibility.
  unsigned length = (unsigned)text.size() - 1;
  const char *str = text.c_str();
  if (length < long_len) // quick normal case
    return MySQLLexer::INT_NUMBER;
  unsigned negative = 0;

  if (*str == '+') // Remove sign and pre-zeros
  {
    str++;
    length--;
  } else if (*str == '-') {
    str++;
    length--;
    negative = 1;
  }

  while (*str == '0' && length) {
    str++;
    length--;
  }

  if (length < long_len)
    return MySQLLexer::INT_NUMBER;

  unsigned smaller, bigger;
  const char *cmp;
  if (negative) {
    if (length == long_len) {
      cmp = signed_long_str + 1;
      smaller = MySQLLexer::INT_NUMBER; // If <= signed_long_str
      bigger = MySQLLexer::LONG_NUMBER; // If >= signed_long_str
    } else if (length < signed_longlong_len)
      return MySQLLexer::LONG_NUMBER;
    else if (length > signed_longlong_len)
      return MySQLLexer::DECIMAL_NUMBER;
    else {
      cmp = signed_longlong_str + 1;
      smaller = MySQLLexer::LONG_NUMBER; // If <= signed_longlong_str
      bigger = MySQLLexer::DECIMAL_NUMBER;
    }
  } else {
    if (length == long_len) {
      cmp = long_str;
      smaller = MySQLLexer::INT_NUMBER;
      bigger = MySQLLexer::LONG_NUMBER;
    } else if (length < longlong_len)
      return MySQLLexer::LONG_NUMBER;
    else if (length > longlong_len) {
      if (length > unsigned_longlong_len)
        return MySQLLexer::DECIMAL_NUMBER;
      cmp = unsigned_longlong_str;
      smaller = MySQLLexer::ULONGLONG_NUMBER;
      bigger = MySQLLexer::DECIMAL_NUMBER;
    } else {
      cmp = longlong_str;
      smaller = MySQLLexer::LONG_NUMBER;
      bigger = MySQLLexer::ULONGLONG_NUMBER;
    }
  }

  while (*cmp && *cmp++ == *str++)
    ;

  return ((unsigned char)str[-1] <= (unsigned char)cmp[-1]) ? smaller : bigger;
}

//----------------------------------------------------------------------------------------------------------------------

size_t MySQLBaseLexer::checkCharset(const std::string &text) {
  return charsets.count(text) > 0 ? MySQLLexer::UNDERSCORE_CHARSET : MySQLLexer::IDENTIFIER;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Puts a DOT token onto the pending token list.
 */
void MySQLBaseLexer::emitDot() {
  _pendingTokens.emplace_back(_factory->create({this, _input}, MySQLLexer::DOT_SYMBOL, _text, channel,
                                               tokenStartCharIndex, tokenStartCharIndex, tokenStartLine,
                                               tokenStartCharPositionInLine));
  ++tokenStartCharIndex;
}

//----------------------------------------------------------------------------------------------------------------------
