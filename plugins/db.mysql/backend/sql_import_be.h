/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _SQL_IMPORT_BE_H_
#define _SQL_IMPORT_BE_H_

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
  virtual void parse_sql_script(parser::MySQLParserServices::Ref sql_parser, parser::MySQLParserContext::Ref context,
                                db_CatalogRef &catalog, const std::string &sql_script, grt::DictRef &options);
  virtual db_CatalogRef target_catalog();
  virtual GrtVersionRef getVersion();

public:
  virtual std::string sql_script() {
    return _sql_script;
  }
  virtual void sql_script(const std::string &sql_script) {
    _sql_script = sql_script;
  }
  virtual void sql_script_codeset(const std::string &value) {
    _sql_script_codeset = value;
  }

  grt::ListRef<GrtObject> get_created_objects();

protected:
  grt::ValueRef autoplace_grt();

  grt::DictRef _options;

  workbench_DocumentRef _doc;
  std::string _sql_script;
  std::string _sql_script_codeset;
};

#endif /* _SQL_IMPORT_BE_H_ */
