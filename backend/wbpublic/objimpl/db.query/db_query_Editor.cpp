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

#include <grts/structs.db.query.h>

#include "db_query_Editor.h"
#include <grtpp_util.h>

db_query_Editor::ImplData::ImplData() {
}

//================================================================================
// db_query_Editor

void db_query_Editor::init() {
  // _data must be set with set_data() by WBContextSQLIDE
  // if (!_data) _data= new db_query_Editor::ImplData(this);
}

db_query_Editor::~db_query_Editor() {
  delete _data;
}

void db_query_Editor::set_data(ImplData *data) {
  _data = data;
}

db_mgmt_ConnectionRef db_query_Editor::connection() const {
  if (_data)
    return _data->connection();
  return db_mgmt_ConnectionRef();
}

grt::IntegerRef db_query_Editor::getSSHTunnelPort() const {
  if (_data)
    return _data->getSSHTunnelPort();
  return -1;
}

db_mgmt_SSHConnectionRef db_query_Editor::sshConnection() const {
  if (_data)
    return _data->sshConnection();
  return db_mgmt_SSHConnectionRef();
}

grt::IntegerRef db_query_Editor::isConnected() const {
  if (_data)
    return _data->isConnected();
  return grt::IntegerRef(0);
}

db_query_QueryEditorRef db_query_Editor::activeQueryEditor() const {
  if (_data)
    return _data->activeQueryEditor();
  return db_query_QueryEditorRef();
}

grt::ListRef<db_query_LiveDBObject> db_query_Editor::schemaTreeSelection() const {
  return _data->schemaTreeSelection();
}

grt::StringRef db_query_Editor::defaultSchema() const {
  if (_data)
    return _data->activeSchema();
  return grt::StringRef();
}

void db_query_Editor::defaultSchema(const grt::StringRef &value) {
  if (_data)
    _data->activeSchema(*value);
}

db_query_QueryEditorRef db_query_Editor::addQueryEditor() {
  if (_data)
    return _data->addQueryEditor();
  return db_query_QueryEditorRef();
}

grt::IntegerRef db_query_Editor::addToOutput(const std::string &text, ssize_t bringToFront) {
  if (_data)
    return _data->addToOutput(text, (long)bringToFront);
  return grt::IntegerRef(0);
}

db_query_EditableResultsetRef db_query_Editor::createTableEditResultset(const std::string &schema,
                                                                        const std::string &table,
                                                                        const std::string &where, ssize_t showGrid) {
  if (_data)
    return _data->createTableEditResultset(schema, table, where, showGrid != 0);
  return db_query_EditableResultsetRef();
}

void db_query_Editor::editLiveObject(const grt::Ref<db_DatabaseObject> &object, const db_CatalogRef &catalog) {
  if (_data)
    _data->editLiveObject(object, catalog);
}

void db_query_Editor::alterLiveObject(const std::string &type, const std::string &schemaName,
                                      const std::string &objectName) {
  if (_data)
    _data->alterLiveObject(type, schemaName, objectName);
}

grt::ListRef<db_query_Resultset> db_query_Editor::executeScript(const std::string &sql) {
  if (_data)
    return _data->executeScript(sql);
  return grt::ListRef<db_query_Resultset>();
}

grt::IntegerRef db_query_Editor::executeScriptAndOutputToGrid(const std::string &sql) {
  if (_data)
    return _data->executeScriptAndOutputToGrid(sql);
  return grt::IntegerRef(0);
}

db_query_ResultsetRef db_query_Editor::executeManagementQuery(const std::string &sql, ssize_t log) {
  if (_data)
    return _data->executeManagementQuery(sql, log != 0);
  return db_query_ResultsetRef();
}

void db_query_Editor::executeManagementCommand(const std::string &sql, ssize_t log) {
  if (_data)
    _data->executeManagementCommand(sql, log != 0);
}

db_query_ResultsetRef db_query_Editor::executeQuery(const std::string &sql, ssize_t log) {
  if (_data)
    return _data->executeQuery(sql, log != 0);
  return db_query_ResultsetRef();
}

void db_query_Editor::executeCommand(const std::string &sql, ssize_t log, ssize_t background) {
  if (_data)
    _data->executeCommand(sql, log != 0, background != 0);
}
