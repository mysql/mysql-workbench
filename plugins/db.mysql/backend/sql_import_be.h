/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grts/structs.db.h"
#include "grts/structs.workbench.h"

#include "grt/grt_manager.h"
#include "grtpp_undo_manager.h"
#include "db_mysql_public_interface.h"
#include "grtsqlparser/mysql_parser_services.h"
#include "grtdb/db_helpers.h"

class WBPLUGINDBMYSQLBE_PUBLIC_FUNC Sql_import {
public:
  virtual ~Sql_import(){};
  void grtm();

  std::function<grt::ValueRef()> get_task_slot();

  std::function<grt::ValueRef()> get_autoplace_task_slot();

private:
  grt::StringRef parse_sql_script(db_CatalogRef catalog, const std::string &sql_script);
  virtual void parse_sql_script(parsers::MySQLParserServices::Ref sql_parser, parsers::MySQLParserContext::Ref context,
                                db_CatalogRef &catalog, const std::string &sql_script, grt::DictRef &options);
  virtual db_CatalogRef target_catalog();

public:
  virtual std::string sql_script() {
    return _sqlScript;
  }

  virtual void sql_script(const std::string &sql) {
    _sqlScript = sql;
  }

  virtual void encoding(const std::string &value) {
    _encoding = value;
  }

  virtual void sqlMode(const std::string &mode) {
    _sqlMode = mode;
  }

  grt::ListRef<GrtObject> get_created_objects();

protected:
  grt::ValueRef autoplace_grt();

  grt::DictRef _options;

  workbench_DocumentRef _doc;
  std::string _sqlScript;
  std::string _encoding;
  std::string _sqlMode;
};

