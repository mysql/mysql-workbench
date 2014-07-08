/* 
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#ifdef _WIN32
  #ifdef MYSQL_PARSER_EXPORTS
    #define MYSQL_PARSER_PUBLIC_FUNC __declspec(dllexport)
  #else
    #define MYSQL_PARSER_PUBLIC_FUNC __declspec(dllimport)
  #endif
#else
  #define MYSQL_PARSER_PUBLIC_FUNC
#endif

#include <set>
#include <antlr3.h>
#include <string>
#include <vector>

extern "C" {
  ANTLR3_UINT32 check_charset(void *payload, pANTLR3_STRING text);
  ANTLR3_UINT32 check_null(pANTLR3_STRING text);
}

struct MySQLToken
{
  ANTLR3_UINT32 type;    // The type as defined in the grammar.
  ANTLR3_UINT32 line;    // One-based line number of this token.
  ANTLR3_INT32 position; // Zero-based position in the line.
  ANTLR3_MARKER index;   // The index of the token in the input.
  ANTLR3_UINT32 channel; // 0 for normally visible tokens. 99  for the hidden channel (whitespaces, comments).

  char *line_start;      // Pointer into the input to the beginning of the line where this token is located.
  char *start;           // Points to the start of the token in the input.
  char *stop;            // Points to the last character of the token.

  std::string text;      // The text of the token.

  MySQLToken()
  {
    type = ANTLR3_TOKEN_INVALID;
    line = 0;
    position = 0;
    index = -1;
    channel = 0;
    line_start = NULL;
    start = NULL;
    stop = NULL;
  }
};

// Describes the type of a given query.
enum MySQLQueryType
{
  QtUnknown,
  QtAmbiguous,

  // DDL
  QtAlterDatabase,
  QtAlterLogFileGroup,
  QtAlterFunction,
  QtAlterProcedure,
  QtAlterServer,
  QtAlterTable,
  QtAlterTableSpace,
  QtAlterEvent,
  QtAlterView,

  QtCreateTable,
  QtCreateIndex,
  QtCreateDatabase,
  QtCreateEvent,
  QtCreateView,
  QtCreateRoutine,    // All of procedure, function, UDF. Used for parse type.
  QtCreateProcedure,
  QtCreateFunction,
  QtCreateUdf,
  QtCreateTrigger,
  QtCreateLogFileGroup,
  QtCreateServer,
  QtCreateTableSpace,

  QtDropDatabase,
  QtDropEvent,
  QtDropFunction, // Includes UDF.
  QtDropProcedure,
  QtDropIndex,
  QtDropLogfileGroup,
  QtDropServer,
  QtDropTable,
  QtDropTablespace,
  QtDropTrigger,
  QtDropView,

  QtRenameTable,
  QtTruncateTable,

  // DML
  QtCall,
  QtDelete,
  QtDo,

  QtHandler, // Do we need Handler open/close etc.?

  QtInsert,
  QtLoadData,
  QtLoadXML,
  QtReplace,
  QtSelect,
  QtUpdate,

  QtPartition, // Cannot be used standalone.

  QtStartTransaction,
  QtBeginWork,
  QtCommit,
  QtRollbackWork,
  QtSetAutoCommit, // "set autocommit" is especially mentioned in transaction help, so identify this too.
  QtSetTransaction,

  QtSavepoint,
  QtReleaseSavepoint,
  QtRollbackSavepoint,

  QtLock,
  QtUnlock,

  QtXA,    // Do we need xa start, xa end etc.?

  QtPurge,
  QtChangeMaster,
  QtReset,
  QtResetMaster,
  QtResetSlave,
  QtStartSlave,
  QtStopSlave,
  QtLoadDataMaster,
  QtLoadTableMaster,

  QtPrepare,
  QtExecute,
  QtDeallocate,

  // Database administration
  QtAlterUser,
  QtCreateUser,
  QtDropUser,
  QtGrantProxy,
  QtGrant,
  QtRenameUser,
  QtRevokeProxy,
  QtRevoke,

  QtAnalyzeTable,
  QtCheckTable,
  QtChecksumTable,
  QtOptimizeTable,
  QtRepairTable,
  QtBackUpTable,
  QtRestoreTable,

  QtInstallPlugin,
  QtUninstallPlugin,

  QtSet,   // Any variable assignment.
  QtSetPassword,

  QtShow,
  QtShowAuthors,
  QtShowBinaryLogs,
  QtShowBinlogEvents,
  QtShowRelaylogEvents,
  QtShowCharset,
  QtShowCollation,
  QtShowColumns,
  QtShowContributors,
  QtShowCreateDatabase,
  QtShowCreateEvent,
  QtShowCreateFunction,
  QtShowCreateProcedure,
  QtShowCreateTable,
  QtShowCreateTrigger,
  QtShowCreateView,
  QtShowDatabases,
  QtShowEngineStatus,
  QtShowStorageEngines,
  QtShowErrors,
  QtShowEvents,
  QtShowFunctionCode,
  QtShowFunctionStatus,
  QtShowGrants,
  QtShowIndexes, // Index, Indexes, Keys
  QtShowInnoDBStatus,
  QtShowMasterStatus,
  QtShowOpenTables,
  QtShowPlugins,
  QtShowProcedureStatus,
  QtShowProcedureCode,
  QtShowPrivileges,
  QtShowProcessList,
  QtShowProfile,
  QtShowProfiles,
  QtShowSlaveHosts,
  QtShowSlaveStatus,
  QtShowStatus,
  QtShowVariables,
  QtShowTableStatus,
  QtShowTables,
  QtShowTriggers,
  QtShowWarnings,

  QtCacheIndex,
  QtFlush,
  QtKill,   // Connection, Query
  QtLoadIndex,

  QtExplainTable,
  QtExplainStatement,
  QtHelp,
  QtUse,

  QtSentinel
};

struct MySQLParserErrorInfo
{
  std::string message;
  ANTLR3_UINT32 token_type;
  size_t charOffset;        // Offset (in bytes) from the beginning of the input to the error position.
  size_t line;              // Error line.
  ANTLR3_UINT32 offset;     // Byte offset in the error line to the error start position.
  size_t length;
};

class MYSQL_PARSER_PUBLIC_FUNC MySQLRecognitionBase
{
public:
  MySQLRecognitionBase(const std::set<std::string> &charsets);
  virtual ~MySQLRecognitionBase() {};

  // Internal function called by static callback.
  void add_error(const std::string &message, ANTLR3_UINT32 token, ANTLR3_MARKER token_start,
    ANTLR3_UINT32 line, ANTLR3_UINT32 offset_in_line, ANTLR3_MARKER length);

  const std::vector<MySQLParserErrorInfo> &error_info();
  bool has_errors();
  unsigned sql_mode();
  virtual void set_sql_mode(const std::string &sql_mode);
  virtual const char* text() = 0;

  bool is_charset(const std::string &s);
  bool is_identifier(ANTLR3_UINT32 type);

  size_t get_keyword_token(const std::string &keyword);

  static bool is_keyword(ANTLR3_UINT32 type);
  static bool is_relation(ANTLR3_UINT32 type);
  static bool is_number(ANTLR3_UINT32 type);
  static bool is_operator(ANTLR3_UINT32 type);
  static bool is_subtree(struct ANTLR3_BASE_TREE_struct *tree);

protected:
  virtual void reset();

private:
  class Private;
  Private *d;
};
