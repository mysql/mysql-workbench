/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
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
  virtual db_mgmt_SSHConnectionRef sshConnection() const = 0;
  virtual grt::IntegerRef getSSHTunnelPort() const = 0;
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
