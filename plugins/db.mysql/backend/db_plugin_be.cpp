/*
 * Copyright (c) 2009, 2020, Oracle and/or its affiliates.
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

#include <ostream>
#include <sstream>
#include <memory>

#include "db_plugin_be.h"
#include "grtsqlparser/sql_facade.h"
#include "grt/icon_manager.h"
#include "grts/structs.db.h"
#include "base/string_utilities.h"
#include "db.mysql/src/module_db_mysql.h"
#include "base/log.h"
#include "grtsqlparser/mysql_parser_services.h"
#include "base/util_functions.h"

#include <glib.h>

DEFAULT_LOG_DOMAIN("Db Plugin")

void Db_plugin::grtm(bool reveng) {
  _doc = workbench_DocumentRef::cast_from(grt::GRT::get()->get("/wb/doc"));

  db_mgmt_ManagementRef mgmt = workbench_WorkbenchRef::cast_from(_doc->owner())->rdbmsMgmt();
  // don't need schema box for reverse engineer, but need it for fwd/sync (in case OmitQualifiers is on)
  _db_conn = new DbConnection(mgmt, db_mgmt_DriverRef(), reveng ? true : false);

  _tables.icon_id(table_icon_id(bec::Icon16));
  _views.icon_id(view_icon_id(bec::Icon16));
  _routines.icon_id(routine_icon_id(bec::Icon16));
  _triggers.icon_id(trigger_icon_id(bec::Icon16));
  _users.icon_id(user_icon_id(bec::Icon16));

  _catalog = db_CatalogRef(grt::Initialized);
}

std::string Db_plugin::task_desc() {
  return _("Apply SQL script to server");
}

db_mgmt_RdbmsRef Db_plugin::selected_rdbms() {
  return db_mgmt_RdbmsRef::cast_from(_db_conn->get_connection()->driver()->owner());
}

db_CatalogRef Db_plugin::model_catalog() {
  db_mgmt_RdbmsRef rdbms = selected_rdbms();

  // find first appropriate model catalog for selected rdbms
  grt::ListRef<workbench_physical_Model> physicalModels = _doc->physicalModels();
  for (size_t n = 0, count = physicalModels.count(); n < count; ++n) {
    workbench_physical_ModelRef model = physicalModels.get(n);
    if (model->rdbms().id() == rdbms.id()) {
      _catalog = model->catalog();
      break;
    }
  }

  return _catalog;
}

Db_plugin::Db_objects_setup *Db_plugin::db_objects_setup_by_type(Db_object_type db_object_type) {
  switch (db_object_type) {
    case dbotTable:
      return &_tables;
    case dbotView:
      return &_views;
    case dbotRoutine:
      return &_routines;
    case dbotTrigger:
      return &_triggers;
    case dbotUser:
      return &_users;
    default:
      return NULL;
  }
}

const char *Db_plugin::db_objects_type_to_string(Db_object_type db_object_type) {
  switch (db_object_type) {
    case dbotTable:
      return "table";
    case dbotView:
      return "view";
    case dbotRoutine:
      return "routine";
    case dbotTrigger:
      return "trigger";
    case dbotUser:
      return "user";
    default:
      return NULL;
  }
}

std::string Db_plugin::db_objects_struct_name_by_type(Db_object_type db_object_type) {
  grt::ObjectRef obj = grt::GRT::get()->create_object<grt::internal::Object>(
    model_catalog().get_metaclass()->get_member_type("schemata").content.object_class);
  std::string attr_name = db_objects_type_to_string(db_object_type);
  attr_name.append("s"); // suffix denoting multiple objects
  if (attr_name.compare("triggers") == 0)
    obj = grt::GRT::get()->create_object<grt::internal::Object>(
      obj.get_metaclass()->get_member_type("tables").content.object_class);
  else if (attr_name.compare("users") == 0)
    obj = model_catalog();
  return obj.get_metaclass()->get_member_type(attr_name).content.object_class;
}

/**
 * Check if on the server we're connecting we can expirence some case sensitivity problems,
 * return -1 if it's impossible to check
 * return 0 if everything is ok
 * return 1 if there can be some problems
 */
int Db_plugin::check_case_sensitivity_problems() {
  sql::ConnectionWrapper dbc_conn = _db_conn->get_dbc_connection();
  std::unique_ptr<sql::Statement> statement(dbc_conn->createStatement());

  std::string compile_os;
  int lower_case_table_names = -1;
  {
    std::unique_ptr<sql::ResultSet> rs(statement->executeQuery("SELECT @@version_compile_os"));
    if (rs->next()) {
      compile_os = rs->getString(1);
    }
  }
  {
    std::unique_ptr<sql::ResultSet> rs(statement->executeQuery("SELECT @@lower_case_table_names"));
    if (rs->next()) {
      lower_case_table_names = rs->getInt(1);
    }
  }
  if (compile_os.empty() || lower_case_table_names == -1)
    return -1;

  if (lower_case_table_names == 0 && (base::hasPrefix(compile_os, "Win") || base::hasPrefix(compile_os, "osx")))
    return 1;

  if (lower_case_table_names == 2 && base::hasPrefix(compile_os, "Win"))
    return 1;

  return 0;
}

void Db_plugin::load_schemata(std::vector<std::string> &schemata) {
  _schemata.clear();
  _schemata_ddl.clear();

  sql::ConnectionWrapper dbc_conn = _db_conn->get_dbc_connection();
  sql::DatabaseMetaData *dbc_meta(dbc_conn->getMetaData());

  grt::GRT::get()->send_info(_("Fetching schema list."));
  grt::GRT::get()->send_progress(0.0, _("Fetching schema list..."));

  const unsigned int major = dbc_meta->getDatabaseMajorVersion();
  const unsigned int minor = dbc_meta->getDatabaseMinorVersion();
  const unsigned int revision = dbc_meta->getDatabasePatchVersion();

  DbMySQLImpl *diffsql_module = grt::GRT::get()->find_native_module<DbMySQLImpl>("DbMySQL");
  _db_options = diffsql_module->getTraitsForServerVersion(major, minor, revision);
  _db_options.set("CaseSensitive", grt::IntegerRef(dbc_meta->storesMixedCaseIdentifiers()));
  _db_options.set("MySQLVersion", grt::StringRef(base::strfmt("%d.%d.%d", major, minor, revision)));

  const std::unique_ptr<sql::Statement> statement(dbc_conn->createStatement());
  const std::unique_ptr<sql::ResultSet> rs(statement->executeQuery("SHOW SESSION VARIABLES LIKE 'sql_mode'"));
  if (rs->next()) {
    _db_options.set("SqlMode", grt::StringRef(rs->getString(2)));
  } else {
    _db_options.set("SqlMode", grt::StringRef());
  }


  std::unique_ptr<sql::ResultSet> rset(dbc_meta->getSchemaObjects("", "", "schema"));
  _schemata.reserve(rset->rowsCount());
  float total = (float)rset->rowsCount();
  int current = 0;
  while (rset->next()) {
    std::string name = rset->getString("NAME");
    if (name != "mysql" && name != "information_schema" && name != "performance_schema" && name != "sys") {
      _schemata.push_back(name);
      _schemata_ddl[name] = rset->getString("DDL");
    }
    grt::GRT::get()->send_progress(current++ / total, name, "");
  }

  grt::GRT::get()->send_progress(1.0, _("Fetch finished."));
  grt::GRT::get()->send_info("OK");

  schemata = _schemata;
}

//
// void Db_plugin::default_schemata_selection(std::vector<std::string> &selection)
//{
//  grt::ListRef<db_Schema> model_schemata= model_catalog()->schemata();
//
//  for (grt::ListRef<db_Schema>::const_iterator It= model_schemata.begin(); It != model_schemata.end(); ++It)
//  {
//    selection.push_back((*It)->name());
//  }
//  return;
///*
////select only schemas thet exists both in  DB and model
//  for (size_t n= 0, count= _schemata.size(); n < count; ++n)
//  {
//    db_SchemaRef model_schema= grt::find_named_object_in_list(model_schemata, _schemata[n].c_str());
//    if (model_schema.is_valid())
//      selection.push_back(_schemata[n]);
//  }
//  */
//}

void Db_plugin::schemata_selection(const std::vector<std::string> &selection, bool sel_none_means_sel_all) {
  _schemata_selection = selection;

  if (sel_none_means_sel_all && !_schemata_selection.size())
    _schemata_selection = _schemata;
}

void Db_plugin::load_db_objects(Db_object_type db_object_type) {
  Db_objects_setup *setup = db_objects_setup_by_type(db_object_type);
  setup->reset();

  grt::GRT::get()->send_info(
    std::string("Fetching ").append(db_objects_type_to_string(db_object_type)).append(" list."));

  grt::GRT::get()->send_progress(
    0.0, std::string("Fetching ").append(db_objects_type_to_string(db_object_type)).append(" list."));

  sql::ConnectionWrapper dbc_conn = _db_conn->get_dbc_connection();
  sql::DatabaseMetaData *dbc_meta(dbc_conn->getMetaData());
  std::string db_object_type_name = db_objects_type_to_string(db_object_type);
  std::list<Db_obj_handle> db_objects;
  std::list<std::string> db_obj_names;

  float total_schemas = (float)_schemata_selection.size();
  int current_schema = 0;

  for (std::vector<std::string>::const_iterator iter = _schemata_selection.begin(); iter != _schemata_selection.end();
       ++iter) {
    const std::string &schema_name = *iter;
    float total_objects;
    int count = 0;

    grt::GRT::get()->send_progress((current_schema / total_schemas),
                                   std::string("Fetch ")
                                     .append(db_objects_type_to_string(db_object_type))
                                     .append(" objects from ")
                                     .append(schema_name));

    if (!schema_name.empty()) {
      try {
        std::unique_ptr<sql::ResultSet> rset(dbc_meta->getSchemaObjects("", schema_name, db_object_type_name));
        total_objects = (float)rset->rowsCount();
        while (rset->next()) {
          Db_obj_handle db_obj;
          db_obj.schema = schema_name;
          db_obj.name = rset->getString("NAME");
          db_obj.ddl = rset->getString("DDL");
          setup->all.push_back(db_obj);

          // prefixed by schema name
          db_obj_names.push_back(std::string(schema_name).append(".").append(db_obj.name));

          grt::GRT::get()->send_progress((current_schema / total_schemas) + (count / total_objects) / total_schemas,
                                         db_obj_names.back());

          count++;
        }
      } catch (std::exception &e) {
        std::string msg = base::strfmt("Failed to fetch %s objects from %s: %s",
                                       db_objects_type_to_string(db_object_type), schema_name.c_str(), e.what());
        grt::GRT::get()->send_info(msg);
        logError("Failed to fetch %s objects from %s: %s", db_objects_type_to_string(db_object_type),
                 schema_name.c_str(), e.what());
      }
    }

    current_schema++;
    grt::GRT::get()->send_info(base::strfmt("    %i items from %s", count, schema_name.c_str()));
  }

  // copy from temp list (used for performance optimization)
  setup->all.reserve(db_objects.size());
  std::copy(db_objects.begin(), db_objects.end(), setup->all.begin());
  db_objects.clear();

  // initialize db obj selection
  setup->selection.reset(db_obj_names);
  db_obj_names.clear();

  grt::GRT::get()->send_progress(1.0, "Finished.");
  grt::GRT::get()->send_info("OK");
}

/** read_back_view_ddl()

 Load back VIEW code from the database for everything we created, so that we know how it was normalized.

 This will go through the whole catalog and read the SQL definition for each view. This is needed because
 the server stores the view definition in a canonical form, which is quite different from what the user
 types (not only formatting is stripped, but object names are expanded and expressions are rewritten).
 That makes it impossible for us to synchronize by comparing the definition in the model with the
 definition in the server. So we need to store the server version of the view right after we create it in
 there and a snapshot of the view that was used to create that view.

 During synchronization, we can detect changes in the model by comparing the model version of the snapshot with
 the model SQL definition and detect server changes by comparing the server version of the definition with
 the snapshot for the server. We need to do that separately for every target server that synchronization is
 used with (using the db.SyncProfile object).
 */
void Db_plugin::read_back_view_ddl() {
  Db_objects_setup *setup = db_objects_setup_by_type(dbotView);
  setup->reset();

  grt::GRT::get()->send_info(std::string("Fetching back view definitions in final form."));

  grt::GRT::get()->send_progress(0.0, std::string("Fetching back view definitions in final form."));

  sql::ConnectionWrapper dbc_conn = _db_conn->get_dbc_connection();
  sql::DatabaseMetaData *dbc_meta(dbc_conn->getMetaData());
  std::list<Db_obj_handle> db_objects;

  float total_views = 0;

  for (size_t sc = model_catalog()->schemata().count(), s = 0; s < sc; ++s) {
    db_SchemaRef schema(model_catalog()->schemata()[s]);
    total_views += schema->views().count();
  }
  if (total_views == 0) {
    grt::GRT::get()->send_progress(1.0, "Finished.");
    grt::GRT::get()->send_info("Nothing to fetch");
    return;
  }

  int current_view = 0;
  for (size_t sc = model_catalog()->schemata().count(), s = 0; s < sc; ++s) {
    db_SchemaRef schema(model_catalog()->schemata()[s]);
    for (size_t vc = schema->views().count(), v = 0; v < vc; ++v) {
      db_ViewRef view(schema->views()[v]);

      grt::GRT::get()->send_progress(
        (current_view / total_views),
        std::string("Fetch back database view code for ").append(schema->name()).append(".").append(view->name()));
      std::unique_ptr<sql::ResultSet> rset(dbc_meta->getSchemaObjects("", *schema->name(), "view", true, *view->name()));

      // take a snapshot of the server version of the SQL
      if (rset->next())
        view->oldServerSqlDefinition(grt::StringRef(rset->getString("DDL")));
      else
        grt::GRT::get()->send_info(
          base::strfmt("Could not get definition for %s.%s from server", schema->name().c_str(), view->name().c_str()));

      // take a snapshot of the model version of the SQL
      view->oldModelSqlDefinition(view->sqlDefinition());

      current_view++;
    }
  }
  grt::GRT::get()->send_progress(1.0, "Finished.");
  grt::GRT::get()->send_info(base::strfmt("%i views were read back.", current_view));
}

bool Db_plugin::validate_db_objects_selection(std::list<std::string> *messages) {
  // check if there are selected triggers without selected owner table
  Db_objects_setup *tables_setup = db_objects_setup_by_type(dbotTable);
  Db_objects_setup *triggers_setup = db_objects_setup_by_type(dbotTrigger);

  if (!triggers_setup->activated)
    return true;

  std::vector<std::string> triggers = _triggers.selection.items();
  std::vector<std::string> tables = _tables.selection.items();

  typedef std::vector<std::string>::iterator Iterator;
  for (Iterator tr_i = triggers.begin(), tr_i_end = triggers.end(); tr_i != tr_i_end; ++tr_i) {
    bool owner_table_selected = false;
    if (tables_setup->activated) {
      for (Iterator tbl_i = tables.begin(), tbl_i_end = tables.end(); tbl_i != tbl_i_end; ++tbl_i) {
        std::string table_dot = *tbl_i + ".";
        if (0 == tr_i->compare(0, table_dot.size(), table_dot, 0, std::string::npos)) {
          owner_table_selected = true;
          break;
        }
      }
    }
    if (!owner_table_selected) {
      if (messages) {
        std::string err_msg;
        err_msg = "Owner table for trigger `" + *tr_i + "` was not selected.";
        messages->push_back(err_msg);
        err_msg = "Please either select the table or deselect triggers owned by that table.";
        messages->push_back(err_msg);
      }
      return false;
    }
  }

  return true;
}

void Db_plugin::dump_ddl(Db_object_type db_object_type, std::string &sql_script) {
  std::string _non_std_sql_delimiter;
  {
    SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms(selected_rdbms());
    Sql_specifics::Ref sql_specifics = sql_facade->sqlSpecifics();
    _non_std_sql_delimiter = sql_specifics->non_std_sql_delimiter();
  }

  Db_objects_setup *setup = db_objects_setup_by_type(db_object_type);
  if (setup->activated) {
    bec::GrtStringListModel::Items_ids items_ids = setup->selection.items_ids();
    for (size_t n = 0, count = items_ids.size(); n < count; ++n) {
      Db_obj_handle &db_obj = setup->all[items_ids[n]];

      sql_script.append("USE `").append(db_obj.schema).append("`;\n");

      if (dbotRoutine == db_object_type || dbotTrigger == db_object_type)
        sql_script.append(base::strfmt("DELIMITER %s\n", _non_std_sql_delimiter.c_str()));

      if (g_utf8_validate(db_obj.ddl.c_str(), -1, NULL))
        sql_script.append(db_obj.ddl);
      else
        sql_script.append("CREATE ... ")
          .append(db_objects_struct_name_by_type(db_object_type))
          .append(" `")
          .append(db_obj.schema)
          .append("`.`")
          .append(db_obj.name)
          .append("`: DDL contains non-UTF symbol(s)");

      if (dbotRoutine == db_object_type || dbotTrigger == db_object_type)
        sql_script.append(base::strfmt(" %s\nDELIMITER ;\n", _non_std_sql_delimiter.c_str()));

      sql_script.append(";\n\n");
    }
  }
}

void Db_plugin::dump_ddl(std::string &sql_script) {
  /*
  alter/diff/sync module requirement: create selected schemata first.
  even if schema is empty the corresponding object must be created.
  */
  // process only selected objects
  for (std::vector<std::string>::const_iterator iter = _schemata_selection.begin(); iter != _schemata_selection.end();
       ++iter) {
    sql_script.append(_schemata_ddl[*iter]).append(";\n\n");
  }

  dump_ddl(dbotTable, sql_script);
  dump_ddl(dbotView, sql_script);
  dump_ddl(dbotRoutine, sql_script);
  dump_ddl(dbotTrigger, sql_script);
}

db_CatalogRef Db_plugin::db_catalog() {
  db_CatalogRef mod_cat = model_catalog();

  if (!mod_cat.is_valid())
    throw std::runtime_error(_("Internal error. Catalog is invalid"));

  workbench_physical_ModelRef pm = workbench_physical_ModelRef::cast_from(mod_cat->owner());

  std::string sql_input_script;
  dump_ddl(sql_input_script);

  db_mysql_CatalogRef catalog = grt::GRT::get()->create_object<db_mysql_Catalog>(mod_cat.get_metaclass()->name());


  grt::replace_contents(catalog->simpleDatatypes(), pm->rdbms()->simpleDatatypes());
  catalog->name("default");
  catalog->oldName(catalog->name());

  grt::DictRef parse_options(true);
  parse_options.set("case_sensitive_identifiers", _db_options.get("CaseSensitive", grt::IntegerRef(1)));

  grt::StringRef sqlMode = grt::StringRef::cast_from(_db_options.get("SqlMode", grt::StringRef("")));

  auto version = bec::parse_version(grt::StringRef::cast_from(_db_options.get("MySQLVersion", grt::StringRef(base::getVersion()))).c_str());
  catalog->version(version);

  auto services = parsers::MySQLParserServices::get();
  auto context = services->createParserContext(pm->rdbms()->characterSets(), version, sqlMode.c_str(),
    _db_options.get_int("CaseSensitive", 1) != 0);
  auto errorCount = services->parseSQLIntoCatalog(context, catalog, sql_input_script, parse_options);

  if (errorCount != 0) {
    logError("There was an error while parsing the DDL retrieved from the server.\n");
  }

  return catalog;
}

void Db_plugin::set_task_proc() {
  _task_proc_cb = std::bind(&Db_plugin::apply_script_to_db, this);
}

grt::StringRef Db_plugin::apply_script_to_db() {
  sql::ConnectionWrapper conn = db_conn()->get_dbc_connection();
  std::unique_ptr<sql::Statement> stmt(conn->createStatement());

  grt::GRT::get()->send_info(_("Executing SQL script in server"));

  std::list<std::string> statements;
  SqlFacade::Ref sql_splitter = SqlFacade::instance_for_rdbms(selected_rdbms());
  sql_splitter->splitSqlScript(_sql_script, statements);

  sql::SqlBatchExec sql_batch_exec;

  sql_batch_exec.error_cb(std::bind(&Db_plugin::process_sql_script_error, this, std::placeholders::_1,
                                    std::placeholders::_2, std::placeholders::_3));
  sql_batch_exec.batch_exec_progress_cb(
    std::bind(&Db_plugin::process_sql_script_progress, this, std::placeholders::_1));
  sql_batch_exec.batch_exec_stat_cb(
    std::bind(&Db_plugin::process_sql_script_statistics, this, std::placeholders::_1, std::placeholders::_2));

  sql_batch_exec(stmt.get(), statements);

  return grt::StringRef(_("The SQL script was successfully applied to server"));
}

int Db_plugin::process_sql_script_error(long long err_no, const std::string &err_msg, const std::string &statement) {
  std::ostringstream oss;
  std::string stmt = base::trim(statement, "\n");

  base::replaceStringInplace(stmt, "\n", "\n        ");
  stmt = "        " + stmt;

  oss << _("Error ") << err_no << ": " << err_msg << std::endl << _("SQL Code:") << std::endl << stmt << std::endl;
  grt::GRT::get()->send_error(oss.str());
  return 0;
}

int Db_plugin::process_sql_script_progress(float progress_state) {
  grt::GRT::get()->send_progress(progress_state, "");
  return 0;
}

int Db_plugin::process_sql_script_statistics(long success_count, long err_count) {
  std::ostringstream oss;
  oss << _("SQL script execution finished: statements: ") << success_count << _(" succeeded, ") << err_count
      << _(" failed") << std::endl;
  grt::GRT::get()->send_progress(1.f, "");
  grt::GRT::get()->send_info(oss.str());
  return 0;
}
