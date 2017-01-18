/*
 * Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _DB_QUERY_EDITOR_H_
#define _DB_QUERY_EDITOR_H_

#include <grts/structs.db.query.h>

#include "wbpublic_public_interface.h"

class MySQLEditor;

// Use an abstract class here because db_query_Editor.cpp is in wbpublic but
// actual query editor object is in wbprivate. So wbprivate must subclass this
// and assign instances to the grt object
class WBPUBLICBACKEND_PUBLIC_FUNC db_query_Editor::ImplData {
public:
  ImplData();
  virtual ~ImplData() {
  }
  virtual db_mgmt_ConnectionRef connection() const = 0;
  virtual grt::IntegerRef isConnected() const = 0;
  virtual db_query_QueryEditorRef addQueryEditor() = 0;
  virtual grt::IntegerRef addToOutput(const std::string &text, long bringToFront) = 0;
  virtual grt::ListRef<db_query_Resultset> executeScript(const std::string &sql) = 0;
  virtual grt::IntegerRef executeScriptAndOutputToGrid(const std::string &sql) = 0;
  virtual db_query_EditableResultsetRef createTableEditResultset(const std::string &schema, const std::string &table,
                                                                 const std::string &where, bool showGrid) = 0;

  virtual void activeSchema(const std::string &schema) = 0;
  virtual std::string activeSchema() = 0;
  virtual db_query_QueryEditorRef activeQueryEditor() = 0;
  virtual grt::ListRef<db_query_LiveDBObject> schemaTreeSelection() const = 0;
  virtual void editLiveObject(const grt::Ref<db_DatabaseObject> &object, const db_CatalogRef &catalog) = 0;
  virtual void alterLiveObject(const std::string &type, const std::string &schemaName,
                               const std::string &objectName) = 0;

  virtual db_query_ResultsetRef executeQuery(const std::string &sql, bool log) = 0;
  virtual void executeCommand(const std::string &sql, bool log, bool background) = 0;

  virtual db_query_ResultsetRef executeManagementQuery(const std::string &sql, bool log) = 0;
  virtual void executeManagementCommand(const std::string &sql, bool log) = 0;
};

#endif
