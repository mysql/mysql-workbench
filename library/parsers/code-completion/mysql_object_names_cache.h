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

#pragma once

#ifndef HAVE_PRECOMPILED_HEADERS
#include <functional>
#endif
#include <set>
#include <map>
#include <deque>

#include "parsers-common.h"
#include "base/string_utilities.h"
#include "base/threading.h"

// Callback used to have the owner of the cache run an sql query and return the first 2 columns
// of the resultset in pairs of the return vector (or an empty string for non-existing columns).
using ObjectQueryCallback = std::function<std::vector<std::pair<std::string, std::string>>(const std::string &query)>;

class PARSERS_PUBLIC_TYPE MySQLObjectNamesCache {
public:
  // Note: feedback can be called from the worker thread. Make the necessary arrangements.
  //       It comes with parameter true if the cache update is going on, otherwise false.
  MySQLObjectNamesCache(ObjectQueryCallback getValues, std::function<void(bool)> feedback, bool jsonSupport = false);
  ~MySQLObjectNamesCache();

  // Data retrieval functions.
  std::vector<std::string> getMatchingSchemaNames(const std::string &prefix = "");
  std::vector<std::string> getMatchingTableNames(const std::string &schema = "", const std::string &prefix = "");
  std::vector<std::string> getMatchingViewNames(const std::string &schema = "", const std::string &prefix = "");
  std::vector<std::string> getMatchingColumnNames(const std::string &schema, const std::string &table,
                                                  const std::string &prefix = "");
  std::vector<std::string> getMatchingProcedureNames(const std::string &schema = "", const std::string &prefix = "");
  std::vector<std::string> getMatchingFunctionNames(const std::string &schema = "", const std::string &prefix = "");
  std::vector<std::string> getMatchingTriggerNames(const std::string &schema = "", const std::string &table = "",
                                                   const std::string &prefix = "");

  std::vector<std::string> getMatchingUdfNames(const std::string &prefix = "");
  std::vector<std::string> getMatchingVariables(const std::string &prefix = ""); // System vars only.
  std::vector<std::string> getMatchingEngines(const std::string &prefix = "");

  std::vector<std::string> getMatchingLogfileGroups(const std::string &prefix = ""); // Only useful for NDB cluster.
  std::vector<std::string> getMatchingTablespaces(const std::string &prefix = "");   // Only useful for NDB cluster.

  std::vector<std::string> getMatchingCharsets(const std::string &prefix = "");
  std::vector<std::string> getMatchingCollations(const std::string &prefix = "");

  std::vector<std::string> getMatchingEvents(const std::string &schema = "", const std::string &prefix = "");
  std::vector<std::string> getMatchingCollections(const std::string &schema = "", const std::string &prefix = "");

  // Data refresh functions. Also can be called from outside when data objects are created or destroyed.
  bool loadSchemaObjectsIfNeeded(const std::string &schema);
  void refreshSchemaCache();
  void refreshTables(const std::string &schema);
  void refreshViews(const std::string &schema);
  void refreshColumns(const std::string &schema, const std::string &table);
  void refreshTriggers(const std::string &schema, const std::string &table);
  void refreshUdfs();
  void refreshTablespaces();   // Logfile groups and tablespaces are unqualified,
  void refreshLogfileGroups(); // event though they belong to a specific table.
  void refreshEvents();

  // Update functions that can also be called from outside.
  void updateSchemas(const std::set<std::string> &schemas);
  void updateTables(const std::string &schema, base::StringListPtr tables);
  void updateViews(const std::string &schema, base::StringListPtr tables);
  void updateProcedures(const std::string &schema, base::StringListPtr tables);
  void updateFunctions(const std::string &schema, base::StringListPtr tables);
  void updateEvents(const std::string &schema, base::StringListPtr events);
  void updateCollections(const std::string &schema, base::StringListPtr collections);

  void shutdown();

private:
  struct RefreshTask {
    enum RefreshType {
      RefreshSchemas,
      RefreshTables,
      RefreshViews,
      RefreshProcedures,
      RefreshFunctions,
      RefreshColumns,
      RefreshTriggers,
      RefreshUDFs,
      RefreshVariables,
      RefreshEngines,
      RefreshLogfileGroups,
      RefreshTableSpaces,
      RefreshCharsets,
      RefreshCollations,
      RefreshEvents,
      RefreshCollections
    } type;

    std::string schemaName;
    std::string tableName;

    RefreshTask() {
      type = RefreshSchemas;
    }

    RefreshTask(RefreshType type_, const std::string &schema, const std::string &table) {
      type = type_;
      schemaName = schema;
      tableName = table;
    }
  };

  enum RetrievalType { RetrieveWithNoQualifier, RetrieveWithSchemaQualifier, RetrieveWithFullQualifier };

  enum CacheObjectType {
    OtherCacheType,
    ColumnsCacheType,
    TriggersCacheType,
  };

  static void *refreshThreadFunction(void *);
  void refreshThread();
  void doRefreshSchemas();
  void doRefreshTables(const std::string &schema);
  void doRefreshViews(const std::string &schema);
  void doRefreshFunctions(const std::string &schema);
  void doRefreshProcedures(const std::string &schema);
  void doRefreshColumns(const std::string &schema, const std::string &table);
  void doRefreshTriggers(const std::string &schema, const std::string &table);

  void doRefreshUdfs();
  void doRefreshCharsets();
  void doRefreshCollations();
  void doRefreshVariables();
  void doRefreshEngines();
  void doRefreshLogfileGroups();
  void doRefreshTablespaces();
  void doRefreshEvents(const std::string &schema);
  void doRefreshCollections(const std::string &schema);

  void updateObjectNames(const std::string &cache, const std::set<std::string> &objects);
  void updateObjectNames(const std::string &cache, const std::string &schema, base::StringListPtr objects);
  void updateObjectNames(const std::string &context, const std::string &schema, const std::set<std::string> &objects,
                         CacheObjectType type);

  std::vector<std::string> getMatchingObjects(const std::string &cache, const std::string &schema,
                                              const std::string &table, const std::string &prefix, RetrievalType type);

  bool is_fetch_done(const std::string &cache, const std::string &schema);

  bool getPendingRefresh(RefreshTask &task);
  void addPendingRefresh(RefreshTask::RefreshType type, const std::string &schema = "", const std::string &table = "");
  void createWorkerThread();

  GThread *_refreshThread;
  base::Semaphore _cacheWworking; // Indicates if there is currently a worker thread doing updates.
  bool _jsonSupport;              // Whenever we can use getCollections

  base::RecMutex _pendingMutex; // Protects the pending tasks.
  std::list<RefreshTask> _pendingTasks;

  ObjectQueryCallback _getValues;
  std::function<void(bool)> _feedback;

  bool _shutdown;

  base::RecMutex _cacheLock; // Protects the caches.

  // Cache structure is split into multiple parts: top level objects (not bound to a schema),
  // objects from a schema and objects from a table.

  // Unbound objects (schemas, udfs, variables, engines, logfile_groups, tablespaces, charsets, collations).
  // Stored as: "object type": object names set.
  std::map<std::string, std::set<std::string>> _topLevelCache;

  // Schema specific objects (views, tables, functions, procedures, events).
  // A schema can be in the top level cache, but not in the objects cache (if not loaded yet).
  //
  // (schema, object type): object names set
  // e.g. (sakila, tables): actor, address, ...
  std::map<std::pair<std::string, std::string>, std::set<std::string>> _schemaObjectsCache;

  // Table specific objects (columns, triggers).
  // A schema or a table can be in the other caches but not here (if not loaded yet).
  // (schema, (table name, type)): column/trigger names set
  // e.g. (sakila, (actor, TriggersCacheType)): actor_id, ...
  // Note: did not use a tuple here as it doesn't easily work with a map.
  struct CacheObjectMap {
    std::map<CacheObjectType, std::set<std::string>> element;
  };
  struct TableObjectsMap {
    std::map<std::string, CacheObjectMap> element;
  };
  std::map<std::string, TableObjectsMap> _tableObjectsCache;

public:
  using TableObjectsCacheType =
    std::pair<std::string, std::map<std::string, std::map<CacheObjectType, std::set<std::string>>>>;
};
