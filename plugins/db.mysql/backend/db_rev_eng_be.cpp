/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "db_rev_eng_be.h"
#include "base/string_utilities.h"

void Db_rev_eng::grtm() {
  Db_plugin::grtm(true);
  Sql_import::grtm();
}

std::string Db_rev_eng::task_desc() {
  return "Reverse engineer database";
}

db_CatalogRef Db_rev_eng::target_catalog() {
  return Db_plugin::model_catalog();
}

std::string Db_rev_eng::sql_script() {
  std::string sql_script;
  Db_plugin::dump_ddl(sql_script);
  return sql_script;
}

void Db_rev_eng::parse_sql_script(parsers::MySQLParserServices::Ref sql_parser, parsers::MySQLParserContext::Ref context,
                                  db_CatalogRef &catalog, const std::string &sql_script, grt::DictRef &options) {
  grt::AutoUndo undo;
  sql_parser->parseSQLIntoCatalog(context, db_mysql_CatalogRef::cast_from(catalog), sql_script, options);
  undo.end(_("Reverse Engineer Database"));
}

GrtVersionRef Db_rev_eng::getVersion() {
  std::string value;
  sql::ConnectionWrapper dbc_conn = Db_plugin::_db_conn->get_dbc_connection();
  std::unique_ptr<sql::Statement> stmt(dbc_conn->createStatement());
  std::unique_ptr<sql::ResultSet> result(stmt->executeQuery("SELECT version()"));
  if (result->next())
    value = result->getString(1);
  GrtVersionRef _version = bec::parse_version(value);
  return _version;
}
