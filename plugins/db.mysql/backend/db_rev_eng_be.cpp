/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
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

#include "stdafx.h"

#include "db_rev_eng_be.h"
#include "base/string_utilities.h"

void Db_rev_eng::grtm(bec::GRTManager *grtm)
{
  Db_plugin::grtm(grtm, true);
  Sql_import::grtm(grtm);
}


std::string Db_rev_eng::task_desc()
{
  return "Reverse engineer database";
}


db_CatalogRef Db_rev_eng::target_catalog()
{
  return Db_plugin::model_catalog();
}


std::string Db_rev_eng::sql_script()
{
  std::string sql_script;
  Db_plugin::dump_ddl(sql_script);
  return sql_script;
}


void Db_rev_eng::parse_sql_script(SqlFacade::Ref sql_parser, db_CatalogRef &catalog, const std::string &sql_script, grt::DictRef &options)
{
  grt::AutoUndo undo(_grtm->get_grt());
  sql_parser->parseSqlScriptStringEx(catalog, sql_script, options);
  undo.end(_("Reverse Engineer Database"));
}


