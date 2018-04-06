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

#include "db_alter_script_be.h"

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
#include "grtsqlparser/sql_schema_rename.h"
#include "interfaces/sqlgenerator.h"
#include "db_mysql_public_interface.h"

#include "db.mysql/src/module_db_mysql_shared_code.h"
#include "grtdb/charset_utils.h"
#include "grtdb/db_object_helpers.h"
#include "db_mysql_sql_export.h"
#include "diff_tree.h"
#include "base/log.h"

DEFAULT_LOG_DOMAIN("alter_script_be");

DbMySQLDiffAlter::DbMySQLDiffAlter() : _alter_list(grt::Initialized), _alter_object_list(grt::Initialized) {
}

DbMySQLDiffAlter::~DbMySQLDiffAlter() {
}

std::string DbMySQLDiffAlter::generate_alter() {
  SQLGeneratorInterfaceImpl *diffsql_module =
    dynamic_cast<SQLGeneratorInterfaceImpl *>(grt::GRT::get()->get_module("DbMySQL"));

  if (diffsql_module == NULL)
    throw std::runtime_error("Could not find module DbMySQL");

  std::vector<grt::ValueRef> vec;
  _diff_tree->get_object_list_for_script(vec);

  std::vector<std::string> schemata;
  std::vector<std::string> tables;
  std::vector<std::string> triggers;
  std::vector<std::string> views;
  std::vector<std::string> routines;

  for (std::vector<grt::ValueRef>::const_iterator e = vec.end(), it = vec.begin(); it != e; it++) {
    grt::ValueRef v = *it;
    if (!GrtNamedObjectRef::can_wrap(v))
      continue;

    std::string name(
      get_old_object_name_for_key(GrtNamedObjectRef::cast_from(v), get_db_options().get_int("CaseSensitive") != 0));

    if (db_mysql_SchemaRef::can_wrap(v)) {
      db_mysql_SchemaRef schema = db_mysql_SchemaRef::cast_from(v);
      // name.append(get_old_name_or_name(schema));
      schemata.push_back(name);
    } else if (db_mysql_TableRef::can_wrap(v)) {
      db_mysql_TableRef table = db_mysql_TableRef::cast_from(v);
      // name.append(get_old_name_or_name(GrtNamedObjectRef::cast_from(table->owner()))).append(".").append(get_old_name_or_name(table));
      tables.push_back(name);
    } else if (db_mysql_ViewRef::can_wrap(v)) {
      db_mysql_ViewRef view = db_mysql_ViewRef::cast_from(v);
      // name.append(get_old_name_or_name(GrtNamedObjectRef::cast_from(view->owner()))).append(".").append(get_old_name_or_name(view));
      views.push_back(name);
    } else if (db_mysql_RoutineRef::can_wrap(v)) {
      db_mysql_RoutineRef routine = db_mysql_RoutineRef::cast_from(v);
      // name.append(get_old_name_or_name(GrtNamedObjectRef::cast_from(routine->owner()))).append(".").append(get_old_name_or_name(routine));
      routines.push_back(name);
    } else if (db_mysql_TriggerRef::can_wrap(v)) {
      db_mysql_TriggerRef trigger = db_mysql_TriggerRef::cast_from(v);
      // name.append(get_old_name_or_name(GrtNamedObjectRef::cast_from(trigger->owner()->owner()))).append(".").append(get_old_name_or_name(trigger));
      triggers.push_back(name);
    }
  }

  grt::DictRef options(true);
  options.set("SchemaFilterList", convert_string_vector_to_grt_list(schemata));
  options.set("TableFilterList", convert_string_vector_to_grt_list(tables));
  options.set("ViewFilterList", convert_string_vector_to_grt_list(views));
  options.set("RoutineFilterList", convert_string_vector_to_grt_list(routines));
  options.set("TriggerFilterList", convert_string_vector_to_grt_list(triggers));
  options.set("KeepOrder", grt::IntegerRef(1));
  options.set("DBSettings", get_db_options());
  // enable this once the ALTER script generation code is able to properly generate USE statements
  // options.set("OmitSchemas", grt::IntegerRef(1));

  grt::StringListRef alter_list(grt::Initialized);
  grt::ListRef<GrtNamedObject> alter_object_list(true);
  options.set("OutputContainer", alter_list);
  options.set("OutputObjectContainer", alter_object_list);

  if (_alter_change) {
    diffsql_module->generateSQL(_left_cat_copy, options, _alter_change);
  }

  ssize_t res = diffsql_module->makeSQLSyncScript(_left_cat_copy, options, alter_list, alter_object_list);
  if (res != 0)
    return "";

  grt::StringRef script = grt::StringRef::cast_from(options.get("OutputScript"));

  return *script;
};

std::shared_ptr<DiffTreeBE> DbMySQLDiffAlter::init_diff_tree(const std::vector<std::string> &schemata,
                                                             const grt::ValueRef &left, const grt::ValueRef &right,
                                                             grt::StringListRef SchemaSkipList, grt::DictRef options) {

  db_mgmt_RdbmsRef rdbms = db_mgmt_RdbmsRef::cast_from(grt::GRT::get()->get("/wb/rdbmsMgmt/rdbms/0"));
  std::string default_engine_name;
  grt::ValueRef default_engine = bec::GRTManager::get()->get_app_option("db.mysql.Table:tableEngine");
  if (grt::StringRef::can_wrap(default_engine))
    default_engine_name = grt::StringRef::cast_from(default_engine);
  std::string err;

  db_mysql_CatalogRef right_cat_copy;

  _left_catalog = db_mysql_CatalogRef::cast_from(left);
  _left_cat_copy = db_mysql_CatalogRef::cast_from(grt::copy_object(_left_catalog));
  bec::apply_user_datatypes(_left_cat_copy, rdbms);
  bec::CatalogHelper::apply_defaults(_left_cat_copy, default_engine_name);

  CatalogMap left_catalog_map;
  build_catalog_map(_left_cat_copy, left_catalog_map);
  update_all_old_names(_left_cat_copy, true, left_catalog_map);

  _right_catalog = db_mysql_CatalogRef::cast_from(right);
  right_cat_copy = db_mysql_CatalogRef::cast_from(grt::copy_object(_right_catalog));
  bec::apply_user_datatypes(right_cat_copy, rdbms);
  bec::CatalogHelper::apply_defaults(right_cat_copy, default_engine_name);

  CatalogMap right_catalog_map;
  build_catalog_map(right_cat_copy, right_catalog_map);
  update_all_old_names(right_cat_copy, true, right_catalog_map);

  {
    SqlFacade *parser = SqlFacade::instance_for_rdbms_name("Mysql");
    // if the target schema does not have the same name as the original, make sure that the
    // target objects have references to the old schema name fixed in all code objects (triggers, views, SPs, functions)
    for (unsigned int i = 0; i < _left_cat_copy->schemata().count(); i++) {
      db_SchemaRef schema(_left_cat_copy->schemata()[i]);

      std::string orig_schema_name = schema->customData().get_string("db.mysql.synchronize:originalName", "");
      if (!orig_schema_name.empty() && schema->name() != orig_schema_name) {
        logInfo("Fix schema references of %s (from %s)\n", schema->name().c_str(), orig_schema_name.c_str());
        Sql_schema_rename::Ref renamer = parser->sqlSchemaRenamer();
        renamer->rename_schema_references(_left_cat_copy, orig_schema_name, schema->name());
      }
    }
  }

  for (size_t i = 0; i < _left_cat_copy->schemata().count(); i++) {
    db_SchemaRef schema(_left_cat_copy->schemata()[i]);

    // remove excluded object types from the copy of the left catalog
    if (options.get_int("SkipTriggers")) {
      logInfo("Remove triggers from copy of model schema %s\n", schema->name().c_str());
      for (size_t t = 0; t < schema->tables().count(); t++) {
        schema->tables()[t]->triggers().remove_all();
      }
    }
    if (options.get_int("SkipRoutines")) {
      logInfo("Remove routines from copy of model schema %s\n", schema->name().c_str());
      schema->routines().remove_all();
      schema->routineGroups().remove_all();
    }
  }
  for (size_t i = 0; i < right_cat_copy->schemata().count(); i++) {
    db_SchemaRef schema(right_cat_copy->schemata()[i]);

    // remove excluded object types from the copy of the right catalog
    if (options.get_int("SkipTriggers")) {
      logInfo("Remove triggers from copy of model schema %s\n", schema->name().c_str());
      for (size_t t = 0; t < schema->tables().count(); t++) {
        schema->tables()[t]->triggers().remove_all();
      }
    }
    if (options.get_int("SkipRoutines")) {
      logInfo("Remove routines from copy of model schema %s\n", schema->name().c_str());
      schema->routines().remove_all();
      schema->routineGroups().remove_all();
    }
  }

  // 2. diff with model

  grt::DbObjectMatchAlterOmf omf;
  omf.dontdiff_mask = 3;
  grt::NormalizedComparer comparer(get_db_options());
  comparer.init_omf(&omf);
  _alter_change = diff_make(right_cat_copy, _left_cat_copy, &omf);

  SQLGeneratorInterfaceImpl *diffsql_module =
    dynamic_cast<SQLGeneratorInterfaceImpl *>(grt::GRT::get()->get_module("DbMySQL"));
  if (diffsql_module == NULL)
    throw DbMySQLDiffAlterException("error loading module DbMySQL");

  grt::DictRef genoptions(true);
  genoptions.set("DBSettings", get_db_options());
  genoptions.set("OutputContainer", _alter_list);
  genoptions.set("OutputObjectContainer", _alter_object_list);
  genoptions.set("UseFilteredLists", grt::IntegerRef(0));

  if (_alter_change && diffsql_module) {
    diffsql_module->generateSQL(_right_catalog, genoptions, _alter_change);
  }

  // 3. build the tree
  std::map<DiffNode::ApplicationDirection, DiffNode::ApplicationDirection> apply_directions_map;
  apply_directions_map[DiffNode::ApplyToDb] = DiffNode::DontApply;
  apply_directions_map[DiffNode::DontApply] = DiffNode::ApplyToDb;
  return _diff_tree = std::shared_ptr<DiffTreeBE>(
           new ::DiffTreeBE(schemata, _left_cat_copy, right_cat_copy, _alter_change, apply_directions_map));
}

std::string DbMySQLDiffAlter::get_col_name(const size_t col_id) {
  switch (col_id) {
    case 0:
      return "Source";
      break;
    case 1:
      return "Update";
      break;
    case 2:
      return "Destination";
      break;
  }
  return "No Column Name Defined";
}

void DbMySQLDiffAlter::restore_overriden_names() {
  db_mysql_CatalogRef mod_cat = get_model_catalog();
  for (size_t i = 0; i < mod_cat->schemata().count(); i++) {
    db_SchemaRef schema(mod_cat->schemata()[i]);
    std::string original_name = schema->customData().get_string("db.mysql.synchronize:originalName", schema->name());
    std::string original_old_name =
      schema->customData().get_string("db.mysql.synchronize:originalOldName", schema->oldName());
    schema->customData().remove("db.mysql.synchronize:originalName");
    schema->customData().remove("db.mysql.synchronize:originalOldName");
    schema->name(original_name);
    schema->oldName(original_old_name);
  }
}
