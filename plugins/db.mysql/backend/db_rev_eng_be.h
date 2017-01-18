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

#ifndef _DB_REV_ENG_BE_H_
#define _DB_REV_ENG_BE_H_

#include "sql_import_be.h"

#include "db_mysql_public_interface.h"
#include "db_plugin_be.h"

class WBPLUGINDBMYSQLBE_PUBLIC_FUNC Db_rev_eng : public Db_plugin, public Sql_import {
private:
  std::string task_desc();
  void parse_sql_script(parser::MySQLParserServices::Ref sql_parser, parser::MySQLParserContext::Ref context,
                        db_CatalogRef &catalog, const std::string &sql_scrtipt, grt::DictRef &options);
  db_CatalogRef target_catalog();

public:
  std::string sql_script();
  void sql_script(const std::string &sql_script) {
    Db_plugin::sql_script(sql_script);
  }

public:
  Db_rev_eng() : Db_plugin(), Sql_import() {
  }
  void grtm();
  GrtVersionRef getVersion();
};

#endif /* _DB_REV_ENG_BE_H_ */
