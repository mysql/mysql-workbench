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

#include "db_mysql_diff_reporting.h"

#include "grtdb/db_object_helpers.h"

#include "grts/structs.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.db.mysql.h"
#include "grts/structs.workbench.h"

#include "grt.h"

using namespace grt;

#include "diff/diffchange.h"
#include "grtdb/diff_dbobjectmatch.h"

#include "grtsqlparser/sql_facade.h"
#include "interfaces/sqlgenerator.h"
#include "backend/db_mysql_public_interface.h"

#include "grtdb/charset_utils.h"
#include "grtdb/db_object_helpers.h"

DbMySQLDiffReporting::DbMySQLDiffReporting() {
}

DbMySQLDiffReporting::~DbMySQLDiffReporting() {
}

std::string DbMySQLDiffReporting::generate_report(const db_mysql_CatalogRef& left_cat,
                                                  const db_mysql_CatalogRef& right_cat) {
  std::string err;
  db_mysql_CatalogRef left_cat_copy, right_cat_copy;

  std::string default_engine_name;
  grt::ValueRef default_engine = bec::GRTManager::get()->get_app_option("db.mysql.Table:tableEngine");
  if (grt::StringRef::can_wrap(default_engine))
    default_engine_name = grt::StringRef::cast_from(default_engine);

  left_cat_copy = db_mysql_CatalogRef::cast_from(grt::copy_object(left_cat));
  bec::CatalogHelper::apply_defaults(left_cat_copy, default_engine_name);

  right_cat_copy = db_mysql_CatalogRef::cast_from(grt::copy_object(right_cat));
  bec::CatalogHelper::apply_defaults(right_cat_copy, default_engine_name);

  if (left_cat_copy->schemata().count() > 0 && right_cat->schemata().count() > 0)
    right_cat_copy->schemata()[0]->name(left_cat->schemata()[0]->name());

  CatalogMap left_catalog_map;
  build_catalog_map(left_cat_copy, left_catalog_map);
  update_all_old_names(left_cat_copy, true, left_catalog_map);

  CatalogMap right_catalog_map;
  build_catalog_map(right_cat_copy, right_catalog_map);
  update_all_old_names(right_cat_copy, true, right_catalog_map);

  db_mgmt_RdbmsRef rdbms = db_mgmt_RdbmsRef::cast_from(grt::GRT::get()->get("/wb/rdbmsMgmt/rdbms/0"));

  bec::apply_user_datatypes(right_cat_copy, rdbms);
  bec::apply_user_datatypes(left_cat_copy, rdbms);

  SQLGeneratorInterfaceImpl* diffsql_module =
    dynamic_cast<SQLGeneratorInterfaceImpl*>(grt::GRT::get()->get_module("DbMySQL"));

  if (diffsql_module == NULL)
    throw DbMySQLDiffReportingException("error loading module DbMySQL");

  std::string tpath;
  tpath.append("modules")
    .append(G_DIR_SEPARATOR_S)
    .append("data")
    .append(G_DIR_SEPARATOR_S)
    .append("db_mysql_catalog_reporting")
    .append(G_DIR_SEPARATOR_S)
    .append("Basic_Text.tpl")
    .append(G_DIR_SEPARATOR_S)
    .append("basic_text_report.txt.tpl");

  grt::DictRef options(true);
  options.set("OMFDontDiffMask", grt::IntegerRef(3));
  options.set("UseFilteredLists", grt::IntegerRef(0));
  options.set("KeepOrder", grt::IntegerRef(1));
  options.set("SeparateForeignKeys", grt::IntegerRef(0));
  options.set("TemplateFile", grt::StringRef(bec::GRTManager::get()->get_data_file_path(tpath).c_str()));

  grt::StringRef output_string(diffsql_module->generateReportForDifferences(left_cat_copy, right_cat_copy, options));

  return *output_string;
}
