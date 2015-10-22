/* 
 * Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
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

#include <string>
#include <stdint.h>

// Generally used types by the recognizers/scanners, as well as their consumers.

#define	INVALID_TOKEN	0

struct MySQLToken
{
  uint32_t type;    // The type as defined in the grammar.
  uint32_t line;    // One-based line number of this token.
  int32_t position; // Zero-based position in the line.
  uint64_t index;   // The index of the token in the input.
  uint32_t channel; // 0 for normally visible tokens. 99  for the hidden channel (whitespaces, comments).

  char *line_start;      // Pointer into the input to the beginning of the line where this token is located.
  char *start;           // Points to the start of the token in the input.
  char *stop;            // Points to the last character of the token.

  std::string text;      // The text of the token.

  MySQLToken()
  {
    type = INVALID_TOKEN;
    line = 0;
    position = 0;
    index = -1;
    channel = 0;
    line_start = NULL;
    start = NULL;
    stop = NULL;
  }
};

// Determines the sub parts of a query that can be parsed individually.
enum MySQLParseUnit {
  PuGeneric,
  PuCreateTable,
  PuCreateTrigger,
  PuCreateView,
  PuCreateRoutine,
  PuCreateEvent,
  PuCreateIndex,
  PuGrant,
  PuDataType,
  PuCreateLogfileGroup,
  PuCreateServer,
  PuCreateTablespace,
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
  uint32_t token_type;
  size_t charOffset;   // Offset (in bytes) from the beginning of the input to the error position.
  size_t line;         // Error line.
  uint32_t offset;     // Byte offset in the error line to the error start position.
  size_t length;
};

namespace parser {
  typedef std::pair<std::string, std::string> Identifier;
  typedef struct { std::string schema; std::string table; std::string column; } ColumnIdentifier;
}