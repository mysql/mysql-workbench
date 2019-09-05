/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "grts/structs.db.mysql.h"

#include "workbench/wb_context.h"
#include "workbench/wb_context_ui.h"

#include "model/wb_context_model.h"
#include "model/wb_component_physical.h"
#include "model/wb_model_diagram_form.h"
#include "model/wb_overview_physical.h"

#include "cppdbc.h"

#define MYSQL_VERSION_LOWER 0
#define MYSQL_VERSION_HIGHER 99999
#define MYSQL_VERSION_5_0    50000
#define MYSQL_VERSION_5_1    50100
#define MYSQL_VERSION_5_2    50200
#define MYSQL_VERSION_5_3    50300
#define MYSQL_VERSION_5_4    50400
#define MYSQL_VERSION_5_5    50500
#define MYSQL_VERSION_5_5_3  50503
#define MYSQL_VERSION_5_5_36 50536
#define MYSQL_VERSION_5_6    50600
#define MYSQL_VERSION_5_7    50700
#define MYSQL_VERSION_5_7_6  50706
#define MYSQL_VERSION_8_0_0  80000

#define INT_METACLASS_COUNT 183UL

#ifndef UPPER_BOUND
#define UPPER_BOUND(x) ((x == NULL) ? 0 : sizeof(x) / sizeof(x[0]))
#endif

class WorkbenchTester {
private:
  wb::WBFrontendCallbacks _wbcallbacks;
  std::list<std::string> fileDialogInput; // paths to use when show_file_dialog is called

public:
  static void reinitGRT();
  
  wb::WBContext *wb;
  std::shared_ptr<wb::WBContextUI> wbui; // Need to reference here to avoid it to be freed prematurely.

  wb::WBOptions *wboptions;

  mdc::CanvasView *lastView;

  WorkbenchTester(bool initPython = false, const base::Size &apage_size = base::Size(800, 600),
                  const wb::WBFrontendCallbacks &callbacks = wb::WBFrontendCallbacks());
  ~WorkbenchTester();

  void initializeRuntime();

  void executeScript(sql::Statement *stmt, const std::string& script);
  void createNewDocument();
  bool closeDocument();
  bool renewDocument();

  void addFileForFileDialog(const std::string &path);
  void activateOverview();

  workbench_physical_ModelRef getPmodel();
  db_mgmt_RdbmsRef getRdbms();
  workbench_physical_DiagramRef getPview();
  db_CatalogRef getCatalog();
  db_SchemaRef getSchema();

  void addView();
  void syncView();
  db_mysql_TableRef addTableFigure(const std::string &name, int x, int y);

  void openAllDiagrams();
  void interactivePlaceDbObjects(int x, int y, std::list<db_DatabaseObjectRef> &objects);
  void flushUntil(float timeout);
  void flushUntil(float timeout, std::function<bool()> condition);
  void flushWhile(float timeout, std::function<bool()> condition);
  void flushUntil(float timeout, std::function<size_t()> condition, size_t value);

  void exportPNG(const std::string &path);

  db_mysql_CatalogRef reverseEngineerSchemas(const std::list<std::string> &schema_names);

private:
  base::Size _pageSize;
  bool _guiLock;
};

db_mysql_CatalogRef createEmptyCatalog();
