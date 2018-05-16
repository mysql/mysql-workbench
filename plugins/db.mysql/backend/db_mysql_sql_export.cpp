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

#include "grtdb/db_object_helpers.h"

#include "grts/structs.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.db.mysql.h"

#include "grt.h"
#include "grt/grt_manager.h"
#include "grt/grt_reporter.h"
#include "grtdb/diff_dbobjectmatch.h"
#include "base/log.h"

using namespace grt;

#include "interfaces/sqlgenerator.h"

#include "db_mysql_sql_export.h"
#include "diff_tree.h"

#include "grtdb/catalog_templates.h"
#include "db.mysql/src/module_db_mysql_shared_code.h"
#include "grtsqlparser/sql_facade.h"

DEFAULT_LOG_DOMAIN("DbMySQLSQLExport");

DbMySQLSQLExport::DbMySQLSQLExport(db_mysql_CatalogRef catalog) : DbMySQLValidationPage() {
  _tables_are_selected = true;
  _triggers_are_selected = true;
  _routines_are_selected = true;
  _views_are_selected = true;
  _users_are_selected = true;
  _catalog = catalog;
  _case_sensitive = true;
  _gen_doc_props = false;
  _gen_attached_scripts = false;
  _sortTablesAlphabetically = false;

  if (!_catalog.is_valid())
    _catalog = get_model_catalog(); // call own version

  _users_model = std::shared_ptr<bec::GrtStringListModel>(new bec::GrtStringListModel());
  _users_exc_model = std::shared_ptr<bec::GrtStringListModel>(new bec::GrtStringListModel());

  _tables_model = std::shared_ptr<bec::GrtStringListModel>(new bec::GrtStringListModel());
  _tables_exc_model = std::shared_ptr<bec::GrtStringListModel>(new bec::GrtStringListModel());
  _views_model = std::shared_ptr<bec::GrtStringListModel>(new bec::GrtStringListModel());
  _views_exc_model = std::shared_ptr<bec::GrtStringListModel>(new bec::GrtStringListModel());
  _routines_model = std::shared_ptr<bec::GrtStringListModel>(new bec::GrtStringListModel());
  _routines_exc_model = std::shared_ptr<bec::GrtStringListModel>(new bec::GrtStringListModel());
  _triggers_model = std::shared_ptr<bec::GrtStringListModel>(new bec::GrtStringListModel());
  _triggers_exc_model = std::shared_ptr<bec::GrtStringListModel>(new bec::GrtStringListModel());
}

db_mysql_CatalogRef DbMySQLSQLExport::get_model_catalog() {
  return db_mysql_CatalogRef::cast_from(grt::GRT::get()->get("/wb/doc/physicalModels/0/catalog"));
}

void DbMySQLSQLExport::set_option(const std::string &name, bool value) {
  if (name.compare("GenerateDrops") == 0)
    _gen_drops = value;
  else if (name.compare("GenerateSchemaDrops") == 0)
    _gen_schema_drops = value;
  else if (name.compare("GenerateWarnings") == 0)
    _gen_warnings = value;
  else if (name.compare("GenerateCreateIndex") == 0)
    _gen_create_index = value;
  else if (name.compare("NoViewPlaceholders") == 0)
    _no_view_placeholders = value;
  else if (name.compare("NoUsersJustPrivileges") == 0)
    _no_users_just_privileges = value;
  else if (name.compare("GenerateInserts") == 0)
    _gen_inserts = value;
  else if (name.compare("NoFKForInserts") == 0)
    _no_FK_for_inserts = value;
  else if (name.compare("TriggersAfterInserts") == 0)
    _triggers_after_inserts = value;
  else if (name.compare("TablesAreSelected") == 0)
    _tables_are_selected = value;
  else if (name.compare("TriggersAreSelected") == 0)
    _triggers_are_selected = value;
  else if (name.compare("RoutinesAreSelected") == 0)
    _routines_are_selected = value;
  else if (name.compare("ViewsAreSelected") == 0)
    _views_are_selected = value;
  else if (name.compare("UsersAreSelected") == 0)
    _users_are_selected = value;
  else if (name.compare("OmitSchemata") == 0)
    _omitSchemas = value;
  else if (name.compare("GenerateUse") == 0)
    _generate_use = value;
  else if (name.compare("SkipForeignKeys") == 0)
    _skip_foreign_keys = value;
  else if (name.compare("SkipFKIndexes") == 0)
    _skip_fk_indexes = value;
  else if (name.compare("GenerateDocumentProperties") == 0)
    _gen_doc_props = value;
  else if (name.compare("GenerateAttachedScripts") == 0)
    _gen_attached_scripts = value;
  else if (name.compare("SortTablesAlphabetically") == 0)
    _sortTablesAlphabetically = value;
}

void DbMySQLSQLExport::set_option(const std::string &name, const std::string &value) {
  if (name.compare("OutputFileName") == 0)
    _output_filename = value;
  else if (name.compare("OutputScriptHeader") == 0)
    _output_header = value;
}

void DbMySQLSQLExport::set_db_options_for_version(const GrtVersionRef &version) {
  SQLGeneratorInterfaceImpl *diffsql_module =
    dynamic_cast<SQLGeneratorInterfaceImpl *>(grt::GRT::get()->get_module("DbMySQL"));
  if (diffsql_module != NULL)
    _db_options = diffsql_module->getTraitsForServerVersion((int)version->majorNumber(), (int)version->minorNumber(),
                                                            (int)version->releaseNumber());
}

void DbMySQLSQLExport::set_db_options(grt::DictRef &db_options) {
  _db_options = db_options;
}

grt::StringListRef convert_string_vector_to_grt_list(const std::vector<std::string> &v) {
  grt::StringListRef grt_list(grt::Initialized);
  for (std::vector<std::string>::const_iterator e = v.end(), it = v.begin(); it != e; it++) {
    grt_list.insert(grt::StringRef(*it));
  }
  return grt_list;
}

// static void parse_string_from_filter(const std::string& source,
//                                     std::string& out_prefix,
//                                     std::string& out_suffix)
//{
//  std::string::size_type period= source.find('.');
//  out_prefix.assign(source.substr(0, period));
//  if(period != std::string::npos)
//    out_suffix.assign(source.substr(period+1));
//}

typedef std::map<std::string, std::list<std::string> > StringListMap;

std::vector<std::string> get_names(const bec::GrtStringListModel *list,
                                   const std::map<std::string, GrtNamedObjectRef> &obj_map,
                                   std::set<db_mysql_SchemaRef> &schemas, const bool case_sensitive) {
  std::vector<std::string> work_vector;
  const std::vector<std::string> &list_items = list->items();
  for (std::vector<std::string>::const_iterator It = list_items.begin(); It != list_items.end(); ++It) {
    std::map<std::string, GrtNamedObjectRef>::const_iterator ItTableName = obj_map.find(*It);
    if (ItTableName != obj_map.end()) {
      work_vector.push_back(get_old_object_name_for_key(ItTableName->second, case_sensitive));
      if (db_mysql_TriggerRef::can_wrap(ItTableName->second))
        schemas.insert(db_mysql_SchemaRef::cast_from(ItTableName->second->owner()->owner()));
      else if (db_mysql_SchemaRef::can_wrap(ItTableName->second->owner()))
        schemas.insert(db_mysql_SchemaRef::cast_from(ItTableName->second->owner()));
    }
  }
  return work_vector;
};

grt::DictRef DbMySQLSQLExport::get_options_as_dict() {
  grt::DictRef options(true);

  // general options
  options.set("GenerateDrops", grt::IntegerRef(_gen_drops ? 1 : 0));
  options.set("GenerateSchemaDrops", grt::IntegerRef(_gen_schema_drops ? 1 : 0));
  options.set("GenerateWarnings", grt::IntegerRef(_gen_warnings ? 1 : 0));
  options.set("GenerateCreateIndex", grt::IntegerRef(_gen_create_index ? 1 : 0));
  options.set("NoUsersJustPrivileges", grt::IntegerRef(_no_users_just_privileges ? 1 : 0));
  options.set("NoViewPlaceholders", grt::IntegerRef(_no_view_placeholders ? 1 : 0));
  options.set("GenerateInserts", grt::IntegerRef(_gen_inserts ? 1 : 0));
  options.set("NoFKForInserts", grt::IntegerRef(_no_FK_for_inserts ? 1 : 0));
  options.set("TriggersAfterInserts", grt::IntegerRef(_triggers_after_inserts ? 1 : 0));
  options.set("OmitSchemas", grt::IntegerRef(_omitSchemas ? 1 : 0));
  options.set("GenerateUse", grt::IntegerRef(_generate_use ? 1 : 0));
  options.set("SkipForeignKeys", grt::IntegerRef(_skip_foreign_keys ? 1 : 0));
  options.set("SkipFKIndexes", grt::IntegerRef(_skip_fk_indexes ? 1 : 0));
  options.set("GenerateDocumentProperties", grt::IntegerRef(_gen_doc_props ? 1 : 0));
  options.set("GenerateAttachedScripts", grt::IntegerRef(_gen_attached_scripts ? 1 : 0));
  options.set("SortTablesAlphabetically", grt::IntegerRef(_sortTablesAlphabetically ? 1 : 0));

  options.set("OutputScriptHeader", grt::StringRef(_output_header));

  std::vector<std::string> schemata_names;
  db_mysql_CatalogRef cat(get_model_catalog());

  for (size_t i = 0; i < cat->schemata().count(); i++)
    schemata_names.push_back(get_old_object_name_for_key(cat->schemata().get(i), _case_sensitive));

  std::set<db_mysql_SchemaRef> schemas;

  //  options.set("SchemaFilterList", convert_string_vector_to_grt_list(schemata_names));

  // filtering options
  options.set("TableFilterList", _tables_are_selected ? convert_string_vector_to_grt_list(get_names(
                                                          _tables_model.get(), _tables_map, schemas, _case_sensitive))
                                                      : grt::StringListRef());

  options.set("ViewFilterList", _views_are_selected ? convert_string_vector_to_grt_list(get_names(
                                                        _views_model.get(), _views_map, schemas, _case_sensitive))
                                                    : grt::StringListRef());

  options.set("RoutineFilterList", _routines_are_selected
                                     ? convert_string_vector_to_grt_list(
                                         get_names(_routines_model.get(), _routines_map, schemas, _case_sensitive))
                                     : grt::StringListRef());

  options.set("TriggerFilterList", _triggers_are_selected
                                     ? convert_string_vector_to_grt_list(
                                         get_names(_triggers_model.get(), _triggers_map, schemas, _case_sensitive))
                                     : grt::StringListRef());

  options.set("UserFilterList", _users_are_selected ? convert_string_vector_to_grt_list(get_names(
                                                        _users_model.get(), _users_map, schemas, _case_sensitive))
                                                    : grt::StringListRef());

  grt::StringListRef schema_names_list(grt::Initialized);
  for (std::set<db_mysql_SchemaRef>::const_iterator It = schemas.begin(); It != schemas.end(); ++It)
    schema_names_list.insert(get_old_object_name_for_key(*It, _case_sensitive));

  options.set("SchemaFilterList", schema_names_list);

  options.set("CaseSensitive", grt::IntegerRef(_case_sensitive));

  return options;
}

//--------------------------------------------------------------------------------------------------

void DbMySQLSQLExport::start_export(bool wait_finish) {
  bec::GRTTask::Ref task = bec::GRTTask::create_task("SQL export", bec::GRTManager::get()->get_dispatcher(),
                                                     std::bind(&DbMySQLSQLExport::export_task, this, grt::StringRef()));

  scoped_connect(task->signal_finished(), std::bind(&DbMySQLSQLExport::export_finished, this, std::placeholders::_1));

  if (wait_finish)
    bec::GRTManager::get()->get_dispatcher()->add_task_and_wait(task);
  else
    bec::GRTManager::get()->get_dispatcher()->add_task(task);
}

//--------------------------------------------------------------------------------------------------

void DbMySQLSQLExport::export_finished(grt::ValueRef res) {
  CatalogMap cmap;
  update_all_old_names(get_model_catalog(), false, cmap);
  logInfo("%s\n", grt::StringRef::cast_from(res).c_str());
  if (_task_finish_cb)
    _task_finish_cb();
}

ValueRef DbMySQLSQLExport::export_task(grt::StringRef) {
  bec::Reporter rep;

  try {
    SQLGeneratorInterfaceImpl *diffsql_module =
      dynamic_cast<SQLGeneratorInterfaceImpl *>(grt::GRT::get()->get_module("DbMySQL"));

    if (diffsql_module == NULL)
      return grt::StringRef("\nSQL Script Export Error: Not able to load 'DbMySQL' module");

    DictRef create_map;
    DictRef drop_map;

    grt::DictRef options = get_options_as_dict();

    options.set("SQL_MODE", bec::GRTManager::get()->get_app_option("SqlGenerator.Mysql:SQL_MODE"));
    options.gset("UseFilteredLists", 1);

    if (_db_options.is_valid()) {
      if (_db_options.count() == 0)
        logError("internal error: Supplied dboptions is empty!?\n");

      _db_options.set("CaseSensitive", grt::IntegerRef(1));
      options.set("DBSettings", _db_options);
    } else {
      grt::DictRef dboptions = diffsql_module->getDefaultTraits();
      dboptions.set("CaseSensitive", grt::IntegerRef(1));
      options.set("DBSettings", dboptions);
    }
    // grt::DictRef::cast_from(options.get("DBSettings", getDefaultTraits())),
    create_map = diffsql_module->generateSQLForDifferences(GrtNamedObjectRef(), _catalog, options);

    if (_gen_drops)
      drop_map = diffsql_module->generateSQLForDifferences(_catalog, GrtNamedObjectRef(), options);
    if (!drop_map.is_valid())
      drop_map = grt::DictRef(true);

    grt::StringListRef strlist = grt::StringListRef::cast_from(options.get("ViewFilterList"));

    // generateSQLForDifferences() will set DiffCaseSensitiveness to the used value
    _case_sensitive = options.get_int("DiffCaseSensitiveness", _case_sensitive) != 0;
    options.set("CaseSensitive", grt::IntegerRef(_case_sensitive));
    if (_db_options.is_valid())
      _db_options.set("CaseSensitive", grt::IntegerRef(_case_sensitive));

    options.set("SortTablesAlphabetically", grt::IntegerRef(_sortTablesAlphabetically ? 1 : 0));

    if (diffsql_module->makeSQLExportScript(_catalog, options, create_map, drop_map)) {
      return grt::StringRef("\nSQL Script Export Error: SQL Script Export Module Returned Error");
    }

    _export_sql_script = options.get_string("OutputScriptHeader") + options.get_string("OutputScript");

    if (!_output_filename.empty()) {
      g_file_set_contents(_output_filename.c_str(), _export_sql_script.c_str(), (gssize)_export_sql_script.size(),
                          NULL);
    }
    return StringRef("\nSQL Script Export Completed");
  } catch (std::exception &ex) {
    if (ex.what()) {
      return grt::StringRef(std::string("\nSQL Script Export Error: ").append(ex.what()).c_str());
    } else
      return grt::StringRef("\nUnknown SQL Script Export Error");
  }
}

void DbMySQLSQLExport::setup_grt_string_list_models_from_catalog(
  bec::GrtStringListModel **users_model, bec::GrtStringListModel **users_exc_model,
  bec::GrtStringListModel **tables_model, bec::GrtStringListModel **tables_exc_model,
  bec::GrtStringListModel **views_model, bec::GrtStringListModel **views_exc_model,
  bec::GrtStringListModel **routines_model, bec::GrtStringListModel **routines_exc_model,
  bec::GrtStringListModel **triggers_model, bec::GrtStringListModel **triggers_exc_model) {
  std::list<std::string> empty_list, users_list, tables_list, views_list, routines_list, triggers_list;

  grt::ListRef<db_User> users = _catalog->users();
  for (size_t i = 0, count0 = users.count(); i < count0; i++) {
    db_UserRef user = users.get(i);
    if (!user->modelOnly()) {
      users_list.push_back(user->name().c_str());
      _users_map[users_list.back()] = user;
    }
  }

  grt::ListRef<db_mysql_Schema> schemata = _catalog->schemata();
  for (size_t i = 0, count1 = schemata.count(); i < count1; i++) {
    db_mysql_SchemaRef schema = schemata.get(i);

    // tables
    grt::ListRef<db_mysql_Table> tables = schema->tables();
    for (size_t j = 0, count2 = tables.count(); j < count2; j++) {
      db_mysql_TableRef table = tables.get(j);
      if (!table->modelOnly()) {
        tables_list.push_back(get_q_name(table->owner()->name().c_str(), table->name().c_str()));
        _tables_map[tables_list.back()] = table;
      }

      // triigers
      grt::ListRef<db_mysql_Trigger> triggers = table->triggers();
      for (size_t k = 0, count3 = triggers.count(); k < count3; k++) {
        db_mysql_TriggerRef trigger = triggers.get(k);
        if (!trigger->modelOnly()) {
          triggers_list.push_back(get_q_name(trigger->owner()->owner()->name().c_str(), trigger->name().c_str()));
          _triggers_map[triggers_list.back()] = trigger;
        }
      }
    }

    // views
    grt::ListRef<db_mysql_View> views = schema->views();
    for (size_t j = 0, count2 = views.count(); j < count2; j++) {
      db_mysql_ViewRef view = views.get(j);
      if (!view->modelOnly()) {
        views_list.push_back(get_q_name(view->owner()->name().c_str(), view->name().c_str()));
        _views_map[views_list.back()] = view;
      }
    }

    // routines
    grt::ListRef<db_mysql_Routine> routines = schema->routines();
    for (size_t j = 0, count2 = routines.count(); j < count2; j++) {
      db_mysql_RoutineRef routine = routines.get(j);
      if (!routine->modelOnly()) {
        routines_list.push_back(get_q_name(routine->owner()->name().c_str(), routine->name().c_str()));
        _routines_map[routines_list.back()] = routine;
      }
    }
  }

  _users_exc_model->reset(empty_list);
  _tables_exc_model->reset(empty_list);
  _views_exc_model->reset(empty_list);
  _routines_exc_model->reset(empty_list);
  _triggers_exc_model->reset(empty_list);

  _users_model->items_val_masks(_users_exc_model.get());
  _tables_model->items_val_masks(_tables_exc_model.get());
  _views_model->items_val_masks(_views_exc_model.get());
  _routines_model->items_val_masks(_routines_exc_model.get());
  _triggers_model->items_val_masks(_triggers_exc_model.get());

  _users_model->reset(users_list);
  _tables_model->reset(tables_list);
  _views_model->reset(views_list);
  _routines_model->reset(routines_list);
  _triggers_model->reset(triggers_list);

  *users_model = _users_model.get();
  *users_exc_model = _users_exc_model.get();
  *tables_model = _tables_model.get();
  *tables_exc_model = _tables_exc_model.get();
  *views_model = _views_model.get();
  *views_exc_model = _views_exc_model.get();
  *routines_model = _routines_model.get();
  *routines_exc_model = _routines_exc_model.get();
  *triggers_model = _triggers_model.get();
  *triggers_exc_model = _triggers_exc_model.get();
}

// void DbMySQLSQLExport::validation_finished(grt::ValueRef res)
//{
//  _validation_finished_cb();
//}
//
// void DbMySQLSQLExport::validation_messages(MYX_GRT_MSGS *msgs)
//{
//  for(size_t i= 0; i < msgs->msgs_num; i++)
//    messages_list.handle_message(
//      msgs->msgs[i].msg_type,
//      msgs->msgs[i].timestamp,
//      std::string(msgs->msgs[i].msg),
//      fmtstringlist(msgs->msgs[i].msg_detail));
//}
//
// ValueRef DbMySQLSQLExport::validation_task(grt::GRT* grt, grt::StringRef)
//{
//  try
//  {
//    WbValidationInterfaceModule *validation_module=
//      static_cast<WbValidationInterfaceModule *>(
//      grt::GRT::get()->get_module("WbModuleValidation"));
//
//    if(validation_module == NULL)
//      return grt::StringRef("\nSQL Script Export Error: Not able to load 'WbModuleValidation' module");
//
//    grt::GRT::get()->send_info("Starting general validation");
//
//    int validation_res= validation_module->validateAll(
//      GrtObjectRef::cast_from(grt::GRT::get()->get("/wb/doc/physicalModels/0/catalog")));
//
//    _manager->get_dispatcher()->call_from_main_thread<int>(
//      std::bind(_validation_step_finished_cb, validation_res), true);
//
//    WbValidationMySQLInterfaceModule *mysql_validation_module=
//      static_cast<WbValidationMySQLInterfaceModule *>(
//      grt::GRT::get()->get_module("WbModuleValidationMySQL"));
//
//    if(validation_module == NULL)
//      return grt::StringRef("\nSQL Script Export Error: Not able to load 'WbModuleValidationMySQL' module");
//
//    grt::GRT::get()->send_info("Starting MySQL-specific validation");
//
//    validation_res= mysql_validation_module->validateAll(
//      GrtObjectRef::cast_from(grt::GRT::get()->get("/wb/doc/physicalModels/0/catalog")));
//
//    _manager->get_dispatcher()->call_from_main_thread<int>(
//      std::bind(_validation_step_finished_cb, validation_res), true);
//  }
//  catch(std::exception& ex)
//  {
//    if(ex.what())
//      return grt::StringRef(std::string("\nCatalog Validation Error: ").append(ex.what()).c_str());
//    else
//      return grt::StringRef("\nUnknown Catalog Validation Error");
//  }
//
//  return grt::StringRef("");
//}
