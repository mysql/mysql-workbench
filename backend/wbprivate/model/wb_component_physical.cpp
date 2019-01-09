/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "base/wb_iterators.h"
#include "base/file_utilities.h"
#include "base/string_utilities.h"
#include "base/util_functions.h"

#include "wb_component_physical.h"
#include "wb_model_diagram_form.h"

#include "workbench/wb_context.h"
#include "workbench/wb_context_ui.h"

#include "model/wb_context_model.h"
#include "wb_overview_physical.h"

#include "grt/clipboard.h"

#include "grts/structs.workbench.physical.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.db.mysql.h"

#include "grtdb/db_helpers.h"
#include "grtdb/db_object_helpers.h"
#include "grtui/db_conn_be.h"

#include "mdc.h"

#include "wbcanvas/workbench_physical_model_impl.h"
#include "wbcanvas/workbench_physical_diagram_impl.h"
#include "wbcanvas/workbench_physical_tablefigure_impl.h"
#include "wbcanvas/workbench_physical_viewfigure_impl.h"
#include "wbcanvas/workbench_physical_routinegroupfigure_impl.h"
#include "wbcanvas/workbench_physical_connection_impl.h"

#include "base/log.h"
DEFAULT_LOG_DOMAIN("component_physical")

#define FILE_CONNECTION_LIST "connections.xml"

#define MAX_COMMENT_LENGTH_FOR_TOOLTIP 1024
#define MAX_COMMENT_LINE_LENGTH_FOR_TOOLTIP 60

using namespace std;

using namespace grt;
using namespace bec;
using namespace wb;
using namespace base;

template <class C>
grt::Ref<C> get_parent_for_object(const GrtObjectRef &object) {
  GrtObjectRef obj = object;
  while (obj.is_valid() && !obj.is_instance(C::static_class_name()))
    obj = obj->owner();
  return grt::Ref<C>::cast_from(obj);
}

WBComponentPhysical::WBComponentPhysical(WBContext *wb) : WBComponent(wb) {
}

WBComponentPhysical::~WBComponentPhysical() {
  close_document();
}

void WBComponentPhysical::load_app_options(bool update) {
  if (!update) {
    app_ToolbarRef toolbar;
    toolbar = app_ToolbarRef::cast_from(
      grt::GRT::get()->unserialize(base::makePath(_wb->get_datadir(), "data/model_option_toolbar_physical_table.xml")));
    _toolbars[toolbar->name()] = toolbar;

    toolbar = app_ToolbarRef::cast_from(
      grt::GRT::get()->unserialize(base::makePath(_wb->get_datadir(), "data/model_option_toolbar_physical_view.xml")));
    _toolbars[toolbar->name()] = toolbar;

    toolbar = app_ToolbarRef::cast_from(grt::GRT::get()->unserialize(
      base::makePath(_wb->get_datadir(), "data/model_option_toolbar_physical_routinegroup.xml")));
    _toolbars[toolbar->name()] = toolbar;

    toolbar = app_ToolbarRef::cast_from(grt::GRT::get()->unserialize(
      base::makePath(_wb->get_datadir(), "data/model_option_toolbar_physical_relationship.xml")));
    _toolbars["main/" WB_TOOL_PREL11_NOID] = toolbar;
    _toolbars["main/" WB_TOOL_PREL1n_NOID] = toolbar;
    _toolbars["main/" WB_TOOL_PREL11] = toolbar;
    _toolbars["main/" WB_TOOL_PREL1n] = toolbar;
    _toolbars["main/" WB_TOOL_PRELnm] = toolbar;
    _toolbars["main/" WB_TOOL_PREL_PICK] = toolbar;

    _shortcuts = grt::ListRef<app_ShortcutItem>::cast_from(
      grt::GRT::get()->unserialize(base::makePath(_wb->get_datadir(), "data/shortcuts_physical.xml")));
  }

  // this needs to be loaded after drivers list has been loaded
  db_mgmt_ManagementRef mgmt = _wb->get_root()->rdbmsMgmt();
  std::string conn_list_xml = base::makePath(_wb->get_user_datadir(), FILE_CONNECTION_LIST);
  if (g_file_test(conn_list_xml.c_str(), G_FILE_TEST_EXISTS)) {
    try {
      grt::ListRef<db_mgmt_Connection> list(
        grt::ListRef<db_mgmt_Connection>::cast_from(grt::GRT::get()->unserialize(conn_list_xml)));

      if (list.is_valid()) {
        logDebug("Loaded connection list, %i connections found.\n", (int)list.count());
        bool changed = false;
        while (mgmt->storedConns().count() > 0)
          mgmt->storedConns().remove(0);
        for (std::size_t c = list.count(), i = 0; i < c; i++) {
          db_mgmt_ConnectionRef conn(list.get(i));

          conn->owner(mgmt);

          // starting from 5.2.16 we do not store passwords for MySQL or SSH in the connections file anymore
          // so we strip the password, update the hostIdentifier field and store the password in the keychain
          if (*conn->hostIdentifier() == "") {
            conn->hostIdentifier(bec::get_host_identifier_for_connection(conn));

            // save the MySQL password if its set
            if (conn->parameterValues().get_string("password") != "") {
              try {
                mforms::Utilities::store_password(conn->hostIdentifier(),
                                                  conn->parameterValues().get_string("userName"),
                                                  conn->parameterValues().get_string("password"));
              } catch (std::exception &exc) {
                logWarning("Could not store password for %s: %s\n", conn->hostIdentifier().c_str(), exc.what());
              }
              conn->parameterValues().gset("password", "");
              changed = true;
            }

            // save the SSH tunnel password if its set
            if (conn->parameterValues().get_string("sshPassword") != "") {
              if (conn->parameterValues().get_string("sshHost") != "") {
                std::string service = strfmt("ssh@%s", conn->parameterValues().get_string("sshHost").c_str());
                try {
                  mforms::Utilities::store_password(service, conn->parameterValues().get_string("sshUserName"),
                                                    conn->parameterValues().get_string("sshPassword"));
                } catch (std::exception &exc) {
                  logWarning("Could not store password for %s: %s\n", service.c_str(), exc.what());
                }
              }
              conn->parameterValues().gset("sshPassword", "");
              changed = true;
            }
          }

          mgmt->storedConns().insert(conn);
        }
        if (changed)
          save_app_options();
      }
    } catch (std::exception &exc) {
      grt::GRT::get()->send_warning(strfmt("Error loading '%s': %s", conn_list_xml.c_str(), exc.what()));
    }
  }
}

void WBComponentPhysical::setup_context_grt(WBOptions *options) {
  std::string engines;
  // fill engine types list
  grt::Module *module = grt::GRT::get()->get_module("DbMySQL");
  if (module) {
    grt::ListRef<db_mysql_StorageEngine> engines_ret(grt::ListRef<db_mysql_StorageEngine>::cast_from(
      module->call_function("getKnownEngines", grt::BaseListRef(grt::Initialized))));

    for (std::size_t c = engines_ret.count(), i = 0; i < c; i++) {
      engines.append(",").append(engines_ret[i]->name());
    }
    engines = engines.substr(1);
    // this is also used by WBA
    _wb->get_wb_options().gset("@db.mysql.Table:tableEngine/Items", engines.c_str());
  }
  // fill fk types
  _wb->get_wb_options().gset("@db.ForeignKey:updateRule/Items", "NO ACTION,CASCADE,SET NULL,RESTRICT");
  _wb->get_wb_options().gset("@db.ForeignKey:deleteRule/Items", "NO ACTION,CASCADE,SET NULL,RESTRICT");
}

void WBComponentPhysical::init_catalog_grt(const db_mgmt_RdbmsRef &rdbms, const std::string &db_version,
                                           workbench_physical_ModelRef &model) {
  std::string db_package = rdbms->databaseObjectPackage();

  // assemble struct name for catalog and schema for the requested db type

  std::string catalog_struct = db_package + ".Catalog";
  std::string schema_struct = db_package + ".Schema";

  if (!grt::GRT::get()->get_metaclass(catalog_struct) || !grt::GRT::get()->get_metaclass(schema_struct)) {
    // Struct definition for '%s' and/or '%s' cannot be found
    throw grt_runtime_error(
      "Support for RDBMS " + db_package + " not found.",
      "Struct definition for " + catalog_struct + " and/or " + schema_struct + " could not be found");
  }

  db_CatalogRef catalog(grt::GRT::get()->create_object<db_Catalog>(catalog_struct));

  catalog->name("default");
  catalog->owner(model);

  // catalog.oldName(catalog.name());

  // set version
  GrtVersionRef version = parse_version(db_version);
  version->name("Version");
  version->owner(catalog);
  catalog->version(version);

  append_contents(catalog->simpleDatatypes(), rdbms->simpleDatatypes());
  append_contents(catalog->characterSets(), rdbms->characterSets());

  model->catalog(catalog);

  // ml: the user datatypes created by this call are not really user datatypes but explicit
  //     types for implicit aliases in the server, which is not correct. From a parsing standpoint
  //     all accepted server datatypes, alias or not, should go through the normal parsing + handling
  //     process.
  //     User datatypes are datatypes defined by a user.
  //     Commented but not removed as this can have a deeper impact. If it turns out to be ok
  //     then remove entirely (including the create function code). 2015-04-17
  // replace_contents(catalog->userDatatypes(), create_builtin_user_datatypes(catalog, rdbms));

  // add listener for any operation on the schema list
  reset_document();

  // create default roles
  {
    static struct RoleDefinition {
      const char *name;
      const char *object_type;
      const char *object_name;
      const char *privileges[8];
    } default_roles[] = {{"owner", "SCHEMA", "**.*", {"ALL", NULL}},
                         {"table.readonly", "TABLE", "**.*", {"SELECT", NULL}},
                         {"table.insert", "TABLE", "**.*", {"SELECT", "INSERT", "TRIGGER", NULL}},
                         {"table.modify", "TABLE", "**.*", {"SELECT", "INSERT", "TRIGGER", "UPDATE", "DELETE", NULL}},
                         {"routine.execute", "ROUTINE", "**.*", {"EXECUTE", NULL}},
                         {NULL, NULL, NULL, {NULL}}};

    for (int i = 0; default_roles[i].name; i++) {
      db_RoleRef role(grt::Initialized);

      role->name(default_roles[i].name);
      role->owner(catalog);
      catalog->roles().insert(role);

      db_RolePrivilegeRef priv(grt::Initialized);
      priv->owner(role);
      priv->databaseObjectType(default_roles[i].object_type);
      priv->databaseObjectName(default_roles[i].object_name);
      for (int j = 0; default_roles[i].privileges[j]; j++)
        priv->privileges().ginsert(grt::StringRef(default_roles[i].privileges[j]));
      role->privileges().insert(priv);
    }
  }

  // add standard tag categories
  {
    GrtObjectRef category(grt::Initialized);

    category->name("Business Rule");
    category->owner(model);

    model->tagCategories().insert(category);
  }
#if 1
  // create an initial schema
  db_SchemaRef schema(grt::GRT::get()->create_object<db_Schema>(schema_struct));

  schema->name(StringRef("mydb"));
  schema->owner(catalog);
  schema->defaultCharacterSetName("utf8");
  schema->defaultCollationName("utf8_general_ci");

  // schema->oldName(schema->name());

  catalog->schemata().insert(schema);
#endif
}

grt::ListRef<db_UserDatatype> WBComponentPhysical::create_builtin_user_datatypes(const db_CatalogRef &catalog,
                                                                                 const db_mgmt_RdbmsRef &rdbms) {
  grt::Module *module = grt::GRT::get()->get_module("DbMySQL");
  if (module) {
    grt::BaseListRef args(true);
    args.ginsert(rdbms);
    grt::ListRef<db_UserDatatype> user_types(
      grt::ListRef<db_UserDatatype>::cast_from(module->call_function("getDefaultUserDatatypes", args)));

    if (user_types.is_valid()) {
      GRTLIST_FOREACH(db_UserDatatype, user_types, ut) {
        (*ut)->owner(catalog);
      }
    }
    return user_types;
  }

  return grt::ListRef<db_UserDatatype>();
}

void WBComponentPhysical::setup_physical_model(workbench_DocumentRef &doc, const std::string &rdbms_name,
                                               const std::string &rdbms_version) {
  // init physical model
  workbench_physical_ModelRef pmodel(grt::Initialized);
  pmodel->owner(doc);

  pmodel->connectionNotation(_wb->get_wb_options().get_string("DefaultConnectionNotation"));
  pmodel->figureNotation(_wb->get_wb_options().get_string("DefaultFigureNotation"));

  doc->physicalModels().insert(pmodel);

  db_mgmt_ManagementRef mgmt = db_mgmt_ManagementRef::cast_from(grt::GRT::get()->get("/wb/rdbmsMgmt"));

  // find the rdbms module for the db we want
  db_mgmt_RdbmsRef rdbms;

  rdbms = grt::find_named_object_in_list<db_mgmt_Rdbms>(mgmt->rdbms(), rdbms_name);
  if (!rdbms.is_valid()) {
    throw grt_runtime_error(
      "Could not locate RDBMS support object for " + rdbms_name,
      "new_physical(): There is no RDBMS object with the name " + rdbms_name + " in the /rdbmsMgmt/rdbms list.");
  }
  pmodel->rdbms(rdbms);

  init_catalog_grt(rdbms, rdbms_version, pmodel);
}

//--------------------------------------------------------------------------------
// Model Management

db_SchemaRef WBComponentPhysical::add_new_db_schema(const workbench_physical_ModelRef &model) {
  db_SchemaRef schema;
  std::string name;
  std::string class_name;

  grt::AutoUndo undo;

  class_name = *model->rdbms()->databaseObjectPackage() + ".Schema";

  name =
    grt::get_name_suggestion_for_list_object(grt::ObjectListRef::cast_from(model->catalog()->schemata()), "new_schema");

  schema = grt::GRT::get()->create_object<db_Schema>(class_name);
  schema->owner(model->catalog());
  schema->name(name);

  schema->createDate(base::fmttime(0, DATETIME_FMT));
  schema->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  model->catalog()->schemata().insert(schema);

  undo.end(_("Create New Schema"));

  _wb->_frontendCallbacks->show_status_text(strfmt(_("Schema '%s' created."), schema->name().c_str()));

  return schema;
}

grt::DictRef WBComponentPhysical::delete_db_schema(const db_SchemaRef &schema, bool check_empty) {
  if (check_empty && (schema->tables().count() > 0 || schema->views().count() > 0 || schema->routines().count() > 0)) {
    grt::DictRef dict(true);

    dict.gset("name", schema->name());
    dict.gset("tables", (long)schema->tables().count());
    dict.gset("views", (long)schema->views().count());
    dict.gset("routines", (long)schema->routines().count());

    return dict;
  }

  workbench_physical_ModelRef model(get_parent_for_object<workbench_physical_Model>(schema));
  if (model.is_valid()) {
    workbench_physical_DiagramRef view;

    if (model->catalog()->schemata().get_index(schema) == grt::BaseListRef::npos)
      return grt::DictRef();

    grt::AutoUndo undo;

    for (std::size_t vc = model->diagrams().count(), vi = 0; vi < vc; vi++) {
      view = model->diagrams().get(vi);
      std::list<model_FigureRef> figures;

      // remove canvas objects for schema contents
      for (std::size_t c = schema->tables().count(), i = 0; i < c; i++) {
        db_TableRef table = schema->tables().get(i);
        model_FigureRef figure = view->getFigureForDBObject(table);
        if (figure.is_valid())
          figures.push_back(figure);
      }
      for (std::size_t c = schema->views().count(), i = 0; i < c; i++) {
        db_ViewRef v = schema->views().get(i);
        model_FigureRef figure = view->getFigureForDBObject(v);
        if (figure.is_valid())
          figures.push_back(figure);
      }
      for (std::size_t c = schema->routineGroups().count(), i = 0; i < c; i++) {
        db_RoutineGroupRef rgroup = schema->routineGroups().get(i);
        model_FigureRef figure = view->getFigureForDBObject(rgroup);
        if (figure.is_valid())
          figures.push_back(figure);
      }

      for (std::list<model_FigureRef>::const_iterator f = figures.begin(); f != figures.end(); ++f)
        delete_model_object(*f, true);
    }
    // remove schema
    model->catalog()->schemata().remove_value(schema);

    undo.end(_("Delete Schema"));
  }

  return grt::DictRef();
}

void WBComponentPhysical::delete_db_schema(const db_SchemaRef &schema) {
  grt::DictRef info;

  _wb->_frontendCallbacks->show_status_text(_("Deleting schema..."));

  info = delete_db_schema(schema, true);

  if (info.is_valid()) {
    int res;
    std::string objects;

    if (info.get_int("tables") > 0)
      objects += strfmt("%li tables, ", (long)info.get_int("tables"));

    if (info.get_int("views") > 0)
      objects += strfmt("%li views, ", (long)info.get_int("views"));

    if (info.get_int("routines") > 0)
      objects += strfmt("%li routines, ", (long)info.get_int("routines"));

    if (!objects.empty())
      objects = objects.substr(0, objects.length() - 2);

    res =
      mforms::Utilities::show_message(_("Delete Schema"), strfmt(_("The schema '%s' contains objects (%s).\n"
                                                                   "Do you want to delete it with all its contents?"),
                                                                 info.get_string("name").c_str(), objects.c_str()),
                                      _("Delete"), _("Cancel"));
    if (res != mforms::ResultOk) {
      _wb->_frontendCallbacks->show_status_text(_("Delete schema cancelled."));
      return;
    }

    info = delete_db_schema(schema, false);
  }

  if (!info.is_valid())
    _wb->_frontendCallbacks->show_status_text(_("Schema deleted."));
  else
    _wb->_frontendCallbacks->show_status_text(_("Could not delete schema."));
}

#include "grts/structs.meta.h"

db_DatabaseObjectRef WBComponentPhysical::add_new_db_table(const db_SchemaRef &schema,
                                                           const std::string &template_name) {
  grt::AutoUndo undo;
  db_TableRef table;

  if (!template_name.empty()) {
    grt::BaseListRef templates =
      grt::BaseListRef::cast_from(_wb->get_root()->options()->options().get("TableTemplates"));
    for (std::size_t c = templates.count(), i = 0; i < c; i++) {
      db_TableRef templ(db_TableRef::cast_from(templates.get(i)));
      if (templ->name() == template_name) {
        grt::CopyContext context;
        table = db_TableRef::cast_from(clone_db_object_to_schema(schema, templ, context));
        context.finish();
        break;
      }
    }
  }
  if (!table.is_valid())
    table =
      schema->addNewTable(*get_parent_for_object<workbench_physical_Model>(schema)->rdbms()->databaseObjectPackage());

  if (table.has_member("tableEngine"))
    table.set_member("tableEngine", bec::GRTManager::get()->get_app_option("db.mysql.Table:tableEngine"));

  undo.end(_("Create Table"));

  if (table.is_valid()) {
    _wb->_frontendCallbacks->show_status_text(
      strfmt(_("Table '%s' created in schema '%s'"), table->name().c_str(), table->owner()->name().c_str()));
  } else
    _wb->_frontendCallbacks->show_status_text(_("Could not create new table."));

  return table;
}

db_DatabaseObjectRef WBComponentPhysical::add_new_db_view(const db_SchemaRef &schema) {
  grt::AutoUndo undo;

  db_ViewRef view =
    schema->addNewView(*get_parent_for_object<workbench_physical_Model>(schema)->rdbms()->databaseObjectPackage());

  undo.end(_("Create View"));
  if (view.is_valid()) {
    _wb->_frontendCallbacks->show_status_text(
      strfmt(_("View '%s' created in schema '%s'"), view->name().c_str(), view->owner()->name().c_str()));
  } else
    _wb->_frontendCallbacks->show_status_text(_("Could not create new view"));

  return view;
}

db_DatabaseObjectRef WBComponentPhysical::add_new_db_routine_group(const db_SchemaRef &schema) {
  grt::AutoUndo undo;

  db_RoutineGroupRef rgroup = schema->addNewRoutineGroup(
    *get_parent_for_object<workbench_physical_Model>(schema)->rdbms()->databaseObjectPackage());

  undo.end(_("Create Routine Group"));
  if (rgroup.is_valid()) {
    _wb->_frontendCallbacks->show_status_text(
      strfmt(_("Routine group '%s' created in schema '%s'"), rgroup->name().c_str(), rgroup->owner()->name().c_str()));
  } else
    _wb->_frontendCallbacks->show_status_text(_("Could not create new routine group"));

  return rgroup;
}

db_DatabaseObjectRef WBComponentPhysical::add_new_db_routine(const db_SchemaRef &schema) {
  grt::AutoUndo undo;

  db_RoutineRef routine =
    schema->addNewRoutine(*get_parent_for_object<workbench_physical_Model>(schema)->rdbms()->databaseObjectPackage());

  undo.end(_("Create Routine"));
  if (routine.is_valid()) {
    _wb->_frontendCallbacks->show_status_text(
      strfmt(_("Routine '%s' created in schema '%s'"), routine->name().c_str(), routine->owner()->name().c_str()));
  } else
    _wb->_frontendCallbacks->show_status_text(_("Could not create new routine"));

  return routine;
}

db_ScriptRef WBComponentPhysical::add_new_stored_script(const workbench_physical_ModelRef &model,
                                                        const std::string &path) {
  db_ScriptRef script(grt::Initialized);
  std::string name = "script";
  if (!path.empty())
    name = base::basename(path);
  script->owner(model);
  script->name(grt::get_name_suggestion_for_list_object(grt::ObjectListRef::cast_from(model->scripts()), name, false));
  script->createDate(base::fmttime(0, DATETIME_FMT));
  script->lastChangeDate(base::fmttime(0, DATETIME_FMT));
  script->filename(_wb->create_attached_file("script", path));

  grt::AutoUndo undo;
  // insertion will trigger the creation of the file
  model->scripts().insert(script);
  if (path.empty())
    undo.end(_("Add SQL Script"));
  else
    undo.end(strfmt(_("Add Script File '%s'"), name.c_str()));

  return script;
}

GrtStoredNoteRef WBComponentPhysical::add_new_stored_note(const workbench_physical_ModelRef &model,
                                                          const std::string &path) {
  GrtStoredNoteRef note(grt::Initialized);
  std::string name = _("New Note");
  if (!path.empty())
    name = base::basename(path);
  note->owner(model);
  note->name(grt::get_name_suggestion_for_list_object(grt::ObjectListRef::cast_from(model->notes()), name, false));
  note->createDate(base::fmttime(0, DATETIME_FMT));
  note->lastChangeDate(base::fmttime(0, DATETIME_FMT));
  note->filename(_wb->create_attached_file("note", path));

  grt::AutoUndo undo;
  // insertion will trigger the creation of the file
  model->notes().insert(note);
  if (path.empty())
    undo.end(_("Add Text Note"));
  else
    undo.end(strfmt(_("Add Note File '%s'"), name.c_str()));

  return note;
}

db_DatabaseObjectRef WBComponentPhysical::clone_db_object_to_schema(const db_SchemaRef &schema,
                                                                    const db_DatabaseObjectRef &object,
                                                                    grt::CopyContext &context) {
  grt::AutoUndo undo;

  if (object.is_instance(db_Table::static_class_name())) {
    db_TableRef dbtable(db_TableRef::cast_from(context.copy(object)));

    if (grt::find_named_object_in_list(schema->tables(), dbtable->name()).is_valid())
      dbtable->name(grt::get_name_suggestion_for_list_object(schema->tables(), *dbtable->name() + "_copy"));

    dbtable->owner(schema);
    dbtable->oldName("");
    schema->tables().insert(dbtable);
    undo.end(strfmt(_("Duplicate '%s'"), dbtable->name().c_str()));

    return dbtable;
  } else if (object.is_instance(db_View::static_class_name())) {
    db_ViewRef dbview(db_ViewRef::cast_from(context.copy(object)));

    if (grt::find_named_object_in_list(schema->views(), dbview->name()).is_valid())
      dbview->name(grt::get_name_suggestion_for_list_object(schema->views(), *dbview->name() + "_copy"));

    dbview->owner(schema);
    dbview->oldName("");
    schema->views().insert(dbview);
    undo.end(strfmt(_("Duplicate '%s'"), dbview->name().c_str()));

    return dbview;
  } else if (object.is_instance(db_Routine::static_class_name())) {
    db_RoutineRef dbroutine(db_RoutineRef::cast_from(context.copy(object)));

    if (grt::find_named_object_in_list(schema->routines(), dbroutine->name()).is_valid())
      dbroutine->name(grt::get_name_suggestion_for_list_object(schema->routines(), *dbroutine->name() + "_copy"));

    dbroutine->owner(schema);
    dbroutine->oldName("");
    schema->routines().insert(dbroutine);
    undo.end(strfmt(_("Duplicate '%s'"), dbroutine->name().c_str()));

    return dbroutine;
  } else if (object.is_instance(db_RoutineGroup::static_class_name())) {
    db_RoutineGroupRef dbroutineGroup(db_RoutineGroupRef::cast_from(context.copy(object)));

    if (grt::find_named_object_in_list(schema->routineGroups(), dbroutineGroup->name()).is_valid())
      dbroutineGroup->name(
        grt::get_name_suggestion_for_list_object(schema->routineGroups(), *dbroutineGroup->name() + "_copy"));

    dbroutineGroup->owner(schema);
    dbroutineGroup->oldName("");
    schema->routineGroups().insert(dbroutineGroup);
    undo.end(strfmt(_("Duplicate '%s'"), dbroutineGroup->name().c_str()));

    return dbroutineGroup;
  }
  return db_DatabaseObjectRef();
}

//--------------------------------------------------------------------------------
// Canvas Object Management

model_FigureRef WBComponentPhysical::place_db_object(ModelDiagramForm *view, const Point &pos,
                                                     const db_DatabaseObjectRef &object, bool select_figure) {
  model_FigureRef figure;
  try {
    workbench_physical_DiagramRef pview(workbench_physical_DiagramRef::cast_from(view->get_model_diagram()));
    std::string object_member;

    if (object.is_instance(db_Table::static_class_name())) {
      figure = pview->placeTable(db_TableRef::cast_from(object), pos.x, pos.y);
      object_member = "table";
    } else if (object.is_instance(db_View::static_class_name())) {
      figure = pview->placeView(db_ViewRef::cast_from(object), pos.x, pos.y);
      object_member = "view";
    } else if (object.is_instance(db_RoutineGroup::static_class_name())) {
      figure = pview->placeRoutineGroup(db_RoutineGroupRef::cast_from(object), pos.x, pos.y);
      object_member = "routineGroup";
    } else
      throw std::invalid_argument("trying to place invalid object on view");

    grt::AutoUndo undo;

    if ((*figure->color()).empty()) {
      if (!view->get_tool_argument(figure.class_name() + ":Color").empty())
        figure->color(grt::StringRef(view->get_tool_argument(figure.class_name() + ":Color")));
      else
        figure->color(_wb->get_wb_options().get_string(figure.class_name() + ":Color", ""));
    }

    if (view->get_model_options().get_int("workbench.physical.ObjectFigure:Expanded",
                                          _wb->get_wb_options().get_int("workbench.physical.ObjectFigure:Expanded")))
      figure->expanded(1);
    else
      figure->expanded(0);

    if (select_figure) {
      pview->unselectAll();
      pview->selectObject(figure);
    }
    undo.end(strfmt(_("Place '%s'"), object->name().c_str()));
  } catch (std::invalid_argument &) {
    _wb->_frontendCallbacks->show_status_text(_("Cannot place object."));
    return model_FigureRef();
  } catch (grt::grt_runtime_error &exc) {
    _wb->show_exception(_("Place Object on Canvas"), exc);
    return model_FigureRef();
  }

  if (figure.is_valid())
    _wb->_frontendCallbacks->show_status_text(strfmt(_("Placed %s"), figure->name().c_str()));
  else
    _wb->_frontendCallbacks->show_status_text(_("Failed placing db object."));

  return figure;
}

//--------------------------------------------------------------------------------------------------

bool WBComponentPhysical::accepts_drop(ModelDiagramForm *view, int x, int y, const std::string &type,
                                       const std::list<GrtObjectRef> &objects) {
  if (objects.empty())
    return false;

  if (type == WB_DBOBJECT_DRAG_TYPE) {
    for (std::list<GrtObjectRef>::const_iterator iter = objects.begin(); iter != objects.end(); ++iter) {
      if (!(*iter).is_instance(db_DatabaseObject::static_class_name()))
        return false;
    }
    return true;
  }
  return false;
}

bool WBComponentPhysical::perform_drop(ModelDiagramForm *view, int x, int y, const std::string &type,
                                       const std::list<GrtObjectRef> &objects) {
  if (objects.empty())
    return false;

  if (type == WB_DBOBJECT_DRAG_TYPE) {
    std::list<db_DatabaseObjectRef> dbobjects;

    for (std::list<GrtObjectRef>::const_iterator iter = objects.begin(); iter != objects.end(); ++iter)
      dbobjects.push_back(db_DatabaseObjectRef::cast_from((*iter)));

    interactive_place_db_objects(view, x, y, dbobjects);
    return true;
  }

  return false;
}

bool WBComponentPhysical::perform_drop(ModelDiagramForm *view, int x, int y, const std::string &type,
                                       const std::string &data) {
  if (data.empty())
    return false;

  if (type == WB_DBOBJECT_DRAG_TYPE) {
    std::list<db_DatabaseObjectRef> dbobjects;
    db_CatalogRef catalog = workbench_physical_ModelRef::cast_from(view->get_model_diagram()->owner())->catalog();

    dbobjects = bec::CatalogHelper::dragdata_to_dbobject_list(catalog, data);

    interactive_place_db_objects(view, x, y, dbobjects);

    return true;
  }

  return false;
}

std::list<model_FigureRef> WBComponentPhysical::interactive_place_db_objects(
  ModelDiagramForm *vform, int x, int y, const std::list<db_DatabaseObjectRef> &objects) {
  grt::CopyContext copy_context;
  std::list<model_FigureRef> result = interactive_place_db_objects(vform, x, y, objects, copy_context);
  copy_context.finish();
  return result;
}

std::list<model_FigureRef> WBComponentPhysical::interactive_place_db_objects(
  ModelDiagramForm *vform, int x, int y, const std::list<db_DatabaseObjectRef> &objects,
  grt::CopyContext &copy_context) {
  int copied = 0;
  std::list<model_FigureRef> created_figures;
  //  std::vector<db_TableRef> tables;

  if (objects.empty()) {
    mforms::Utilities::show_message(_("Cannot Place Object"), _("The object cannot be placed in the diagram."),
                                    _("Close"));
    return created_figures;
  }

  grt::AutoUndo undo;

  Point op, p = vform->get_view()->window_to_canvas(x, y);
  op = p;
  Size view_size(vform->get_view()->get_total_view_size());

  vform->get_model_diagram()->unselectAll();

  for (std::list<db_DatabaseObjectRef>::const_iterator iter = objects.begin(); iter != objects.end(); ++iter) {
    created_figures.push_back(model_FigureRef());

    db_DatabaseObjectRef obj = *iter;
    if (has_figure_for_object_in_active_view(obj, vform)) {
      int r = mforms::Utilities::show_message(
        _("Place Object in Diagram"),
        base::strfmt(
          _("'%s' is already in this diagram. Would you like to duplicate the schema object and place a copy?"),
          obj->name().c_str()),
        _("Duplicate"), _("Cancel"), objects.size() > 1 ? _("Ignore") : "");
      if (r == mforms::ResultOk) {
        // duplicate the object and place it
        obj = db_DatabaseObjectRef::cast_from(_wb->get_model_context()->duplicate_object(obj, copy_context));
      } else if (r == mforms::ResultCancel)
        break;
      else
        continue;
    }

    copied++;
    {
      model_FigureRef figure = place_db_object(vform, p, obj, false);

      created_figures.back() = figure;

      if (figure.is_valid())
        vform->get_model_diagram()->selectObject(figure);

      p.x += 20;
      p.y += 20;
      if (p.x + 100 > view_size.width) {
        op.y += 20;
        p = op;
      } else if (p.y + 100 > view_size.height) {
        op.y += 20;
        p = op;
      }

      if (p.x + 100 > view_size.width || p.y + 100 > view_size.height)
        p = op;

      //      if (obj->is_instance(db_Table::static_class_name()))
      //        tables.push_back(db_TableRef::cast_from(obj));
    }
  }

  if (copied > 0) {
    undo.end(_("Place object(s) on canvas"));
  }
  return created_figures;
}

void WBComponentPhysical::place_new_db_object(ModelDiagramForm *vform, const Point &pos, wb::ObjectType type) {
  std::string object_struct_name;
  db_SchemaRef target_schema;
  std::string schema_name;
  std::string template_name;

  grt::AutoUndo undo;

  model_DiagramRef view(vform->get_model_diagram());
  workbench_physical_ModelRef model(get_parent_for_object<workbench_physical_Model>(view));

  switch (type) {
    case ObjectTable:
      object_struct_name = workbench_physical_TableFigure::static_class_name();
      template_name = vform->get_tool_argument(object_struct_name + std::string(":Template"));
      if (template_name == "*None*")
        template_name = "";
      break;
    case ObjectView:
      object_struct_name = workbench_physical_ViewFigure::static_class_name();
      break;
    case ObjectRoutineGroup:
      object_struct_name = workbench_physical_RoutineGroupFigure::static_class_name();
      break;
    default:
      throw std::logic_error("place_db_object() called with invalid tool");
  }

  schema_name = vform->get_tool_argument(object_struct_name + std::string(":Schema"));

  if (!schema_name.empty()) {
    db_SchemaRef schema(grt::find_named_object_in_list(model->catalog()->schemata(), schema_name));
    if (schema.is_valid())
      target_schema = schema;
  }

  // pick a default schema..
  if (!target_schema.is_valid()) {
    if (model->catalog()->schemata().count() == 0) {
      // if there are no schemas, create a default one
      add_new_db_schema(model);
    }
    target_schema = model->catalog()->schemata().get(0);
  }

  db_DatabaseObjectRef object;

  switch (type) {
    case ObjectTable:
      object = add_new_db_table(target_schema, template_name);
      break;
    case ObjectView:
      object = add_new_db_view(target_schema);
      break;
    case ObjectRoutineGroup:
      object = add_new_db_routine_group(target_schema);
      break;
    default:
      throw std::logic_error("place_db_object() called with invalid tool");
  }

  std::string collation = vform->get_tool_argument(object_struct_name + ":Collation");

  if (collation != "" && collation[0] != '*') {
    if (object.has_member("defaultCollationName"))
      object.set_member("defaultCollationName", grt::StringRef(collation));

    std::string charset = base::split(collation, "_", 1)[0];
    if (object.has_member("defaultCharacterSetName"))
      object.set_member("defaultCharacterSetName", grt::StringRef(charset));
  }

  std::string engine = vform->get_tool_argument(object_struct_name + ":Engine");
  if (!engine.empty() && engine[0] != '*') {
    if (object.has_member("tableEngine"))
      object.set_member("tableEngine", grt::StringRef(engine));
  }

  place_db_object(vform, pos, object, true);

  undo.end(strfmt(_("Place '%s'"), object->name().c_str()));
}

bool WBComponentPhysical::create_nm_relationship(ModelDiagramForm *view, workbench_physical_TableFigureRef table1,
                                                 workbench_physical_TableFigureRef table2, bool imandatory,
                                                 bool fmandatory) {
  grt::AutoUndo undo;
  // create the associative table for a n:m relationship
  db_TableRef atable = bec::TableHelper::create_associative_table(
    db_SchemaRef::cast_from(table1->table()->owner()), table1->table(), table2->table(), imandatory, fmandatory,
    workbench_physical_ModelRef::cast_from(view->get_model_diagram()->owner())->rdbms(), _wb->get_wb_options(),
    view->get_model_diagram()->owner()->options());

  if (!atable.is_valid())
    return false;

  // place the assoc table in the view
  Point pos;
  Point p1(table1->left(), table1->top());
  Point p2(table2->left(), table2->top());

  if (table1->layer() != table1->owner()->rootLayer()) {
    p1.x += table1->layer()->left();
    p1.y += table1->layer()->top();
  }
  if (table2->layer() != table2->owner()->rootLayer()) {
    p2.x += table2->layer()->left();
    p2.y += table2->layer()->top();
  }
  pos.x = (p1.x + p2.x) / 2;
  pos.y = (p1.y + p2.y) / 2;

  place_db_object(view, pos, atable, true);

  undo.end(_("Create n:m Relationship"));

  return true;
}

WBComponentPhysical::RelationshipToolContext *WBComponentPhysical::start_relationship(ModelDiagramForm *view,
                                                                                      const Point &pos,
                                                                                      RelationshipType type) {
  RelationshipToolContext *rctx = new RelationshipToolContext(this, view, type);

  return rctx;
}

void WBComponentPhysical::cancel_relationship(ModelDiagramForm *view, RelationshipToolContext *rctx) {
  if (rctx) {
    rctx->cancel();

    delete rctx;
  }
}

void WBComponentPhysical::delete_db_object(const db_DatabaseObjectRef &object) {
  db_SchemaRef schema(db_SchemaRef::cast_from(object->owner()));
  workbench_physical_ModelRef model(get_parent_for_object<workbench_physical_Model>(schema));

  // XXX need to look for refs by other objects and show them to user
  // and confirm that they should be removed or cancel

  if (object.is_instance(db_Table::static_class_name())) {
    grt::AutoUndo undo;

    schema->tables().remove_value(db_TableRef::cast_from(object));

    if (model.is_valid()) {
      for (grt::ListRef<workbench_physical_Diagram>::const_iterator view = model->diagrams().begin();
           view != model->diagrams().end(); ++view) {
        grt::ListRef<model_Figure> figures((*view)->figures());
        for (std::size_t f = figures.count(); f > 0; --f) {
          model_FigureRef figure = figures[f - 1];
          if (figure.is_instance(workbench_physical_TableFigure::static_class_name()) &&
              workbench_physical_TableFigureRef::cast_from(figure)->table() == object) {
            // do not delete db object from model object deleter, since that would
            // mean deleting it twice and adding 2 entries in the undo stack
            delete_model_object(figure, true);
          }
        }
      }
    }

    // remove referencing foreign keys
    {
      db_TableRef table = db_TableRef::cast_from(object);
      grt::ListRef<db_ForeignKey> foreignKeys(
        db_SchemaRef::cast_from(table->owner())->getForeignKeysReferencingTable(table));
      if (0 < foreignKeys.count()) {
        for (grt::ListRef<db_ForeignKey>::const_iterator iter = foreignKeys.begin(); iter != foreignKeys.end();
             ++iter) {
          db_ForeignKeyRef fk(*iter);
          db_TableRef ref_table = db_TableRef::cast_from(fk->owner());

          // remove corresponding index
          /*
          grt::ListRef<db_Index> indices= ref_table->indices();
          for (size_t count= indices.count(), i= 0; i < count; ++i)
          {
            db_IndexRef index= indices.get(i);
            if (0 == strcmp(index->indexType().c_str(), "FOREIGN"))
            {
              grt::ListRef<db_IndexColumn> index_columns= index->columns();
              grt::ListRef<db_Column> fk_columns= fk->columns();
              if (index_columns.count() == fk_columns.count())
              {
                bool equal= true;
                for (size_t count= index_columns.count(), c= 0; c < count; ++c)
                {
                  db_ColumnRef index_column= index_columns.get(c)->referencedColumn();
                  if (index_column.is_valid())
                  {
                  db_ColumnRef fk_column= find_object_in_list(fk_columns, index_column.id());
                  if (!fk_column.is_valid())
                  {
                    equal= false;
                    break;
                  }
                }
                }
                if (equal)
                {
                  ref_table->indices().remove_value(index);
                  break;
                }
              }
            }
          }
           */
          if (fk->index().is_valid())
            ref_table->indices().remove_value(fk->index());
          ref_table->foreignKeys().remove_value(fk);
        }
      }
    }

    remove_references_to_object(object);

    undo.end(_("Delete Table"));
  } else if (object.is_instance(db_View::static_class_name())) {
    grt::AutoUndo undo;

    schema->views().remove_value(db_ViewRef::cast_from(object));

    if (model.is_valid()) {
      for (grt::ListRef<workbench_physical_Diagram>::const_iterator view = model->diagrams().begin();
           view != model->diagrams().end(); ++view) {
        for (grt::ListRef<model_Figure>::const_reverse_iterator figure = (*view)->figures().rbegin();
             figure != (*view)->figures().rend(); ++figure) {
          if ((*figure).is_instance(workbench_physical_ViewFigure::static_class_name()) &&
              workbench_physical_ViewFigureRef::cast_from(*figure)->view() == object) {
            delete_model_object(*figure, false);
            break; // We have deleted figure, figure is invalid from now and may not be incremented
          }
        }
      }
    }

    remove_references_to_object(object);

    undo.end(_("Delete View"));
  } else if (object.is_instance(db_RoutineGroup::static_class_name())) {
    std::set<db_RoutineRef> ungrouped_routines;
    db_RoutineGroupRef routine_group(db_RoutineGroupRef::cast_from(object));

    // first check if the routines in the rg are in any other routine groups
    // if they are in none, check if they should be totally deleted

    GRTLIST_FOREACH(db_Routine, routine_group->routines(), routine) {
      ungrouped_routines.insert(*routine);
    }

    if (model.is_valid()) {
      GRTLIST_FOREACH(db_RoutineGroup, schema->routineGroups(), rgroup) {
        if (*rgroup != routine_group) {
          GRTLIST_FOREACH(db_Routine, (*rgroup)->routines(), routine) {
            std::set<db_RoutineRef>::iterator iter;

            // remove the routines that are in other groups
            if ((iter = ungrouped_routines.find(*routine)) != ungrouped_routines.end())
              ungrouped_routines.erase(iter);

            if (ungrouped_routines.empty())
              break;
          }
        }
        if (ungrouped_routines.empty())
          break;
      }
    }

    grt::AutoUndo undo;

    if (!ungrouped_routines.empty()) {
      int result;
      if (routine_group->routines().count() == ungrouped_routines.size())
        result = mforms::Utilities::show_message(
          _("Delete Routines"),
          strfmt(_("Would you like to delete the routines contained in group '%s'?"), routine_group->name().c_str()),
          _("Delete"), _("Cancel"), _("Keep"));
      else {
        result = mforms::Utilities::show_message(
          _("Delete Routines"),
          strfmt(_("There are %lu routines in '%s' that are not in any other group, would you like to delete them?"),
                 (unsigned long)ungrouped_routines.size(), routine_group->name().c_str()),
          _("Delete"), _("Cancel"), _("Keep"));
      }

      if (result == mforms::ResultCancel) {
        undo.cancel();
        return;
      } else if (result == mforms::ResultOk) {
        // delete all routines
        for (base::const_range<std::set<db_RoutineRef> > r(ungrouped_routines); r; ++r) {
          std::size_t i = schema->routines().get_index(*r);
          if (i != grt::BaseListRef::npos)
            schema->routines().remove(i);
        }
      }
    }

    schema->routineGroups().remove_value(routine_group);

    if (model.is_valid()) {
      for (grt::ListRef<workbench_physical_Diagram>::const_iterator view = model->diagrams().begin();
           view != model->diagrams().end(); ++view) {
        for (grt::ListRef<model_Figure>::const_reverse_iterator figure = (*view)->figures().rbegin();
             figure != (*view)->figures().rend(); ++figure) {
          if ((*figure).is_instance(workbench_physical_RoutineGroupFigure::static_class_name()) &&
              workbench_physical_RoutineGroupFigureRef::cast_from(*figure)->routineGroup() == object) {
            delete_model_object(*figure, false);
            break; // We have deleted figure, figure is invalid from now and may not be incremented
          }
        }
      }
    }

    remove_references_to_object(object);

    undo.end(_("Delete Routine Group"));
  } else if (object.is_instance(db_Routine::static_class_name())) {
    grt::AutoUndo undo;
    db_RoutineRef routine(db_RoutineRef::cast_from(object));

    schema->routines().remove_value(routine);
    // remove from routine groups
    GRTLIST_FOREACH(db_RoutineGroup, schema->routineGroups(), rg) {
      std::size_t i;
      while ((i = (*rg)->routines().get_index(routine)) != grt::BaseListRef::npos) {
        (*rg)->routines().remove(i);
      }
    }

    remove_references_to_object(object);

    undo.end(_("Delete Routine"));
  }
}

bool WBComponentPhysical::delete_model_object(const model_ObjectRef &object, bool figure_only) {
  if (object.is_instance(workbench_physical_Connection::static_class_name())) {
    if (!figure_only) {
      workbench_physical_ConnectionRef conn(workbench_physical_ConnectionRef::cast_from(object));
      db_ForeignKeyRef fk(conn->foreignKey());
      db_TableRef table(db_TableRef::cast_from(fk->owner()));
      int result;

      // Check if the fk still belongs to the table.
      // If the connection is being deleted together with the table that it points to,
      // it will be auto-removed. Depending on the order things get executed and by the
      // way this place is reached, the FK might already be gone.
      if (table->foreignKeys().get_index(fk) == grt::BaseListRef::npos)
        return false;

      result = mforms::Utilities::show_message(
        _("Delete Foreign Key Columns"), _("Please confirm whether columns used by the foreign key should be deleted "
                                           "too.\nColumns used by other foreign keys will be left untouched."),
        _("Delete"), _("Cancel"), _("Keep"));

      if (result == mforms::ResultCancel)
        return false;

      grt::AutoUndo undo;
      /*
          model_DiagramRef view(conn->owner());
          // remove connection
          view->removeConnection(conn);
      */
      table->removeForeignKey(fk, result == mforms::ResultOk ? true : false);

      undo.end(_("Delete Relationship"));
    }
  } else if (object.is_instance(model_Figure::static_class_name())) {
    model_FigureRef figure(model_FigureRef::cast_from(object));

    grt::AutoUndo undo;
    // if the figure is a DB object, the DB object is also deleted
    if (figure.is_instance(workbench_physical_TableFigure::static_class_name())) {
      db_TableRef dbtable(workbench_physical_TableFigureRef::cast_from(figure)->table());

      workbench_physical_DiagramRef::cast_from(figure->owner())->deleteConnectionsForTable(dbtable);

      workbench_physical_TableFigureRef::cast_from(figure)->table(db_TableRef());

      // this must be called after the figure->table field is invalidated, otherwise
      // delete_db_object() will try to delete the figure again and we get into a loop
      if (!figure_only)
        delete_db_object(dbtable);
    } else if (figure.is_instance(workbench_physical_ViewFigure::static_class_name())) {
      db_ViewRef view(workbench_physical_ViewFigureRef::cast_from(figure)->view());

      workbench_physical_ViewFigureRef::cast_from(figure)->view(db_ViewRef());

      if (!figure_only)
        delete_db_object(view);
    } else if (figure.is_instance(workbench_physical_RoutineGroupFigure::static_class_name())) {
      db_RoutineGroupRef rg(workbench_physical_RoutineGroupFigureRef::cast_from(figure)->routineGroup());

      workbench_physical_RoutineGroupFigureRef::cast_from(figure)->routineGroup(db_RoutineGroupRef());

      if (!figure_only)
        delete_db_object(rg);
    } else
      return false;

    // removeFigure() will remove anything that depends on the figure automatically, ie connections
    workbench_physical_DiagramRef::cast_from(figure->owner())->removeFigure(figure);

    if (figure_only)
      undo.end(strfmt(_("Remove Figure '%s'"), figure.get_metaclass()->get_attribute("caption").c_str()));
    else
      undo.end(strfmt(_("Delete '%s'"), figure.get_metaclass()->get_attribute("caption").c_str()));
  }
  return true;
}

bool WBComponentPhysical::handles_figure(const model_ObjectRef &figure) {
  if (figure.is_instance(workbench_physical_TableFigure::static_class_name()) ||
      figure.is_instance(workbench_physical_ViewFigure::static_class_name()) ||
      figure.is_instance(workbench_physical_RoutineGroupFigure::static_class_name()) ||
      figure.is_instance(workbench_physical_Connection::static_class_name()))
    return true;
  return false;
}

void WBComponentPhysical::copy_object_to_clipboard(const grt::ObjectRef &object, grt::CopyContext &copy_context) {
  std::set<std::string> skip;
  skip.insert("oldName");

  // copy table
  grt::ObjectRef copy = copy_context.copy(object, skip);

  bec::Clipboard *clip = get_wb()->get_clipboard();
  clip->append_data(copy);
}

/* unused
model_ObjectRef WBComponentPhysical::clone_object(const model_ObjectRef &object, const model_LayerRef &destlayer,
                                                  grt::CopyContext &copy_context)
{
  std::set<std::string> skip;
  skip.insert("oldName");

  if (object.is_instance(workbench_physical_TableFigure::static_class_name()))
  {
    workbench_physical_TableFigureRef table(workbench_physical_TableFigureRef::cast_from(object));

    grt::AutoUndo undo;

    // copy table
    db_TableRef dbtable(db_TableRef::cast_from(_wb->get_model_context()->duplicate_object(table->table(),
copy_context)));

    // copy figure
    skip.insert("table");
    workbench_physical_TableFigureRef copy(workbench_physical_TableFigureRef::cast_from(copy_context.copy(table,
skip)));
    copy->table(dbtable);

    copy_context.update_references();

    if (destlayer.is_valid())
    {
      copy->owner(destlayer->owner());
      copy->layer(destlayer);

      copy->name(dbtable->name());

      destlayer->owner()->addFigure(copy);
      workbench_physical_DiagramRef::cast_from(destlayer->owner())->createConnectionsForTable(copy->table());

      undo.end(strfmt(_("Duplicate Table '%s'"), copy->name().c_str()));
    }
    else
    {
      copy->owner(model_DiagramRef());
      copy->layer(model_LayerRef());
    }
    return copy;
  }
  else if (object.is_instance(workbench_physical_ViewFigure::static_class_name()))
  {
    workbench_physical_ViewFigureRef view(workbench_physical_ViewFigureRef::cast_from(object));

    grt::AutoUndo undo;

    // copy view
    db_ViewRef dbview(db_ViewRef::cast_from(_wb->get_model_context()->duplicate_object(view->view(), copy_context)));

    // copy figure
    skip.insert("view");
    workbench_physical_ViewFigureRef copy(workbench_physical_ViewFigureRef::cast_from(copy_context.copy(view)));
    copy->view(dbview);

    copy_context.update_references();

    if (destlayer.is_valid())
    {
      copy->owner(destlayer->owner());
      copy->layer(destlayer);

      copy->name(dbview->name());

      destlayer->owner()->addFigure(copy);

      undo.end(strfmt(_("Duplicate View '%s'"), copy->name().c_str()));
    }
    else
    {
      copy->owner(model_DiagramRef());
      copy->layer(model_LayerRef());
    }
    return copy;
  }
  else if (object.is_instance(workbench_physical_RoutineGroupFigure::static_class_name()))
  {
    workbench_physical_RoutineGroupFigureRef routineGroup(workbench_physical_RoutineGroupFigureRef::cast_from(object));

    grt::AutoUndo undo;

    // copy routineGroup
    db_RoutineGroupRef
dbroutineGroup(db_RoutineGroupRef::cast_from(_wb->get_model_context()->duplicate_object(routineGroup->routineGroup(),
copy_context)));

    // copy figure
    skip.insert("routineGroup");
    workbench_physical_RoutineGroupFigureRef
copy(workbench_physical_RoutineGroupFigureRef::cast_from(copy_context.copy(routineGroup)));
    copy->routineGroup(dbroutineGroup);

    copy_context.update_references();

    if (destlayer.is_valid())
    {
      copy->owner(destlayer->owner());
      copy->layer(destlayer);

      copy->name(dbroutineGroup->name());

      destlayer->owner()->addFigure(copy);

      undo.end(strfmt(_("Duplicate Routine Group '%s'"), copy->name().c_str()));
    }
    else
    {
      copy->owner(model_DiagramRef());
      copy->layer(model_LayerRef());
    }
    return copy;
  }

  return model_ObjectRef();
}
*/

bool WBComponentPhysical::can_paste_object(const grt::ObjectRef &object) {
  if (object.is_instance(db_Table::static_class_name()) || object.is_instance(db_View::static_class_name()) ||
      object.is_instance(db_RoutineGroup::static_class_name()) ||
      object.is_instance(workbench_physical_TableFigure::static_class_name()) ||
      object.is_instance(workbench_physical_ViewFigure::static_class_name()) ||
      object.is_instance(workbench_physical_RoutineGroupFigure::static_class_name()) ||
      object.is_instance(workbench_physical_Connection::static_class_name()))
    return true;
  return false;
}

static void updateConnectionState(workbench_physical_TableFigureRef src, workbench_physical_TableFigureRef dst) {
  workbench_physical_DiagramRef dstView = workbench_physical_DiagramRef::cast_from(dst->owner());
  workbench_physical_DiagramRef srcView = workbench_physical_DiagramRef::cast_from(src->owner());

  grt::ListRef<db_ForeignKey> dstKeys = dst->table()->foreignKeys();
  grt::ListRef<db_ForeignKey> srcKeys = src->table()->foreignKeys();
  for (grt::ListRef<db_ForeignKey>::const_iterator dstIt = dstKeys.begin(); dstIt != dstKeys.end(); ++dstIt) {
    workbench_physical_ConnectionRef dstConn = dstView->getConnectionForForeignKey(*dstIt);
    if (dstConn.is_valid()) // If there's a connection we need to find out state of the src conn and copy it.
    {
      for (grt::ListRef<db_ForeignKey>::const_iterator srcIt = srcKeys.begin(); srcIt != srcKeys.end(); ++srcIt) {
        if (*srcIt == *dstIt) {
          workbench_physical_ConnectionRef srcConn = srcView->getConnectionForForeignKey(*srcIt);
          if (srcConn.is_valid()) {
            dstConn->visible(srcConn->visible());
            dstConn->drawSplit(srcConn->drawSplit());
          }
        }
      }
    }
  }
}

model_ObjectRef WBComponentPhysical::paste_object(ModelDiagramForm *view, const grt::ObjectRef &object,
                                                  grt::CopyContext &copy_context) {
  db_DatabaseObjectRef obj;
  if (model_ObjectRef::can_wrap(object))
    obj = db_DatabaseObjectRef::cast_from(get_object_for_figure(model_ObjectRef::cast_from(object)));
  else
    obj = db_DatabaseObjectRef::cast_from(object);

  db_SchemaRef schema(db_SchemaRef::cast_from(obj->owner()));
  std::list<db_DatabaseObjectRef> objects;
  if (obj.is_instance(db_Table::static_class_name()))
    objects.push_back(grt::find_named_object_in_list(schema->tables(), *obj->name()));
  else if (obj.is_instance(db_View::static_class_name()))
    objects.push_back(grt::find_named_object_in_list(schema->views(), *obj->name()));
  else if (obj.is_instance(db_RoutineGroup::static_class_name()))
    objects.push_back(grt::find_named_object_in_list(schema->routineGroups(), *obj->name()));
  else
    return model_ObjectRef();

  if (objects.front().is_valid()) {
    std::list<model_FigureRef> figures = interactive_place_db_objects(view, 10, 10, objects, copy_context);

    if (model_FigureRef::can_wrap(object)) {
      model_FigureRef figure(figures.back());

      if (figure.is_valid()) {
        model_FigureRef original(model_FigureRef::cast_from(object));

        figure->color(original->color());
        figure->top(original->top());
        figure->left(original->left());
        figure->expanded(original->expanded());
        figure->width(original->width());
        figure->height(original->height());

        // We need to try to cast is to workbench_physical_TableFigureRef, so we can copy additional properties.
        if (workbench_physical_TableFigureRef::can_wrap(original)) {
          workbench_physical_TableFigureRef src(workbench_physical_TableFigureRef::cast_from(original));
          workbench_physical_TableFigureRef dst(workbench_physical_TableFigureRef::cast_from(figure));
          dst->indicesExpanded(src->indicesExpanded());
          dst->triggersExpanded(src->triggersExpanded());
          dst->foreignKeysExpanded(src->foreignKeysExpanded());
          dst->height(src->height()); // Height needs to be recopied after we changed indicesExpanded property.

          updateConnectionState(src, dst);
        }

        return figure;
      }
    }
  }
  return model_ObjectRef();
}

inline const char *find_prev_space(const char *begin, const char *pos) {
  const char *p = pos;
  while (p > begin) {
    if (g_unichar_isspace(g_utf8_get_char_validated(p, -1)))
      return p;
    p = g_utf8_find_prev_char(begin, p);
  }
  return pos;
}

std::string WBComponentPhysical::get_object_tooltip(const model_ObjectRef &object, mdc::CanvasItem *item) {
  if (workbench_physical_TableFigureRef::can_wrap(object)) {
    workbench_physical_TableFigureRef table_figure(workbench_physical_TableFigureRef::cast_from(object));
    db_TableRef table(table_figure->table());

    if (table.is_valid()) {
      workbench_physical_TableFigure::ImplData *tfig = table_figure->get_data();
      if (tfig) {
        db_ColumnRef column(tfig->get_column_at(item));
        db_IndexRef index;

        if (!column.is_valid())
          index = tfig->get_index_at(item);

        if (column.is_valid()) {
          std::string text;
          bool isfk = false;

          text.append(*column->name());
          if (table->isPrimaryKeyColumn(column))
            text.append(" (PK)");
          else if (table->isForeignKeyColumn(column)) {
            isfk = true;
            text.append(" (FK)");
          }
          text.append("\n");

          if (isfk) {
            text.append("References: ");

            for (std::size_t c = table->foreignKeys().count(), i = 0; i < c; i++) {
              db_ForeignKeyRef fk(table->foreignKeys()[i]);
              // ssize_t idx; // get_index returns size_t, so comparing it by >= 0 is always true!!!

              const std::size_t idx = fk->columns().get_index(column);
              if (idx != BaseListRef::npos) {
                if (fk->referencedTable().is_valid() && fk->referencedColumns().get(idx).is_valid()) {
                  text.append(fk->referencedTable()->name());
                  text.append(".");
                  text.append(fk->referencedColumns().get(idx)->name());
                } else
                  text.append("INVALID");
                break;
              }
            }
            text.append("\n");
          }

          text.append(column->formattedRawType());
          text.append("\n");
          {
            std::string flags;
            if (*column->isNotNull())
              flags.append("NOT NULL"); // XXX add a method to db_Column to return a formatted string of all flags
            for (std::size_t c = column->flags().count(), i = 0; i < c; i++) {
              if (i > 0 || *column->isNotNull())
                flags.append(", ");
              flags.append(column->flags().get(i).c_str());
            }
            if (!flags.empty())
              text.append("Flags: ").append(flags);
          }
          if (*column->defaultValue() != "")
            text.append("\nDEFAULT ").append(column->defaultValue().c_str());

          if (*column->comment().c_str()) {
            std::string comment = column->comment();
            text.append("\n");
            comment = truncate_text(comment, MAX_COMMENT_LENGTH_FOR_TOOLTIP);
            try {
              comment = base::reflow_text(comment, MAX_COMMENT_LINE_LENGTH_FOR_TOOLTIP, "  ");
            } catch (std::invalid_argument &e) {
              logWarning("base::reflow_text throw an exception: %s\n", e.what());
              comment = "??Invalid text??";
            } catch (std::logic_error &e) {
              logWarning("base::reflow_text throw an exception: %s\n", e.what());
              comment = "??Invalid text result??";
            }
            comment.append("\n");
            text.append(comment);
          }

          return text;
        } else if (index.is_valid()) {
          std::string text;

          text.append(*index->name()).append("  (").append(index->indexType().c_str()).append(")\n");

          for (std::size_t c = index->columns().count(), i = 0; i < c; i++) {
            db_IndexColumnRef column(index->columns()[i]);

            text.append("  - ").append(column->referencedColumn()->name().c_str()).append("\n");
          }

          return text;
        } else if (table.is_valid()) // table
        {
          std::string text;

          text.append(table->owner()->name());
          text.append(".");
          text.append(table->name());
          text.append("\n");

          if (*table->comment().c_str()) {
            text.append("Comments:\n");
            std::string comment = table->comment();
            comment = truncate_text(comment, MAX_COMMENT_LENGTH_FOR_TOOLTIP);
            try {
              comment = base::reflow_text(comment, MAX_COMMENT_LINE_LENGTH_FOR_TOOLTIP, "  ");
            } catch (std::invalid_argument &e) {
              logWarning("base::reflow_text throw an exception: %s\n", e.what());
              comment = "??Invalid text??";
            } catch (std::logic_error &e) {
              logWarning("base::reflow_text throw an exception: %s\n", e.what());
              comment = "??Invalid text result??";
            }
            comment.append("\n");
            text.append(comment);
          }

          text.append("Columns:\n");
          for (std::size_t c = table->columns().count(), i = 0; i < c; i++) {
            db_ColumnRef column(table->columns()[i]);
            std::string comment = column->comment();
            const std::string column_name(column->name().c_str());
            if (!comment.empty()) {
              comment = "  " + column_name + ": " + comment;
              try {
                comment = base::reflow_text(comment, MAX_COMMENT_LINE_LENGTH_FOR_TOOLTIP, "    ", false, 3);
              } catch (std::invalid_argument &e) {
                logWarning("base::reflow_text throw an exception: %s\n", e.what());
                comment = "??Invalid text??";
              } catch (std::logic_error &e) {
                logWarning("base::reflow_text throw an exception: %s\n", e.what());
                comment = "??Invalid text result??";
              }
              comment.append("\n");
              text.append(comment);
            } else
              text.append("  " + column_name + "\n");
          }

          if (table->foreignKeys().count() > 0) {
            text.append("References:\n");
            for (std::size_t c = table->foreignKeys().count(), i = 0; i < c; i++) {
              db_ForeignKeyRef fk(table->foreignKeys()[i]);
              if (fk->referencedTable().is_valid()) {
                text.append("  (");
                for (std::size_t d = fk->columns().count(), j = 0; j < d; j++) {
                  if (j > 0)
                    text.append(",");
                  text.append(fk->columns()[j]->name());
                }
                text.append(") TO ");
                if (fk->referencedTable()->owner() != table->owner()) {
                  text.append(fk->referencedTable()->owner()->name());
                  text.append(".");
                }
                text.append(fk->referencedTable()->name());
                text.append("(");
                for (std::size_t d = fk->referencedColumns().count(), j = 0; j < d; j++) {
                  if (j > 0)
                    text.append(",");
                  if (!fk->referencedColumns()[j].is_valid())
                    text.append("INVALID");
                  else
                    text.append(fk->referencedColumns()[j]->name());
                }
                text.append(")\n");
              } else
                text.append("  INVALID\n");
            }
          }

          grt::ListRef<db_ForeignKey> foreignKeys(
            db_SchemaRef::cast_from(table->owner())->getForeignKeysReferencingTable(table));
          if (foreignKeys.is_valid() && foreignKeys.count() > 0) {
            text.append("Referenced By:\n");
            for (grt::ListRef<db_ForeignKey>::const_iterator iter = foreignKeys.begin(); iter != foreignKeys.end();
                 ++iter) {
              db_ForeignKeyRef fk(*iter);
              text.append("  ");
              if (fk->owner()->owner() != table->owner()) {
                text.append(fk->owner()->owner()->name());
                text.append(".");
              }
              text.append(fk->owner()->name());
              text.append(" (");
              for (std::size_t d = fk->columns().count(), j = 0; j < d; j++) {
                if (j > 0)
                  text.append(",");
                if (fk->columns()[j].is_valid())
                  text.append(fk->columns()[j]->name());
                else
                  text.append("INVALiD");
              }
              text.append(") TO ");

              text.append("(");
              for (std::size_t d = fk->referencedColumns().count(), j = 0; j < d; j++) {
                if (j > 0)
                  text.append(",");
                if (fk->referencedColumns()[j].is_valid())
                  text.append(fk->referencedColumns()[j]->name());
                else
                  text.append("INVALID");
              }
              text.append(")\n");
            }
          }

          return text;
        }
      }
    }
    return "";
  } else if (workbench_physical_ViewFigureRef::can_wrap(object)) {
    workbench_physical_ViewFigureRef figure(workbench_physical_ViewFigureRef::cast_from(object));
    if (figure->view().is_valid())
      return figure->view()->comment();
  } else if (workbench_physical_RoutineGroupFigureRef::can_wrap(object)) {
    workbench_physical_RoutineGroupFigureRef figure(workbench_physical_RoutineGroupFigureRef::cast_from(object));
    if (figure->routineGroup().is_valid())
      return figure->routineGroup()->comment();
  } else if (workbench_physical_ConnectionRef::can_wrap(object)) {
    workbench_physical_ConnectionRef connection(workbench_physical_ConnectionRef::cast_from(object));
    std::string text;
    db_ForeignKeyRef fk(connection->foreignKey());
    if (fk.is_valid()) {
      if (fk->owner().is_valid()) {
        text.append(fk->owner()->name()).append("\n");
        for (std::size_t c = fk->columns().count(), i = 0; i < c; i++) {
          if (fk->columns()[i].is_valid())
            text.append("    ").append(fk->columns()[i]->name()).append("\n");
          else
            text.append("???\n");
        }
      }

      text.append("<references>\n");

      if (fk->referencedTable().is_valid()) {
        text.append(fk->referencedTable()->name()).append("\n");
        for (std::size_t c = fk->referencedColumns().count(), i = 0; i < c; i++) {
          if (fk->referencedColumns()[i].is_valid())
            text.append("    ").append(fk->referencedColumns()[i]->name()).append("\n");
          else
            text.append("???\n");
        }
      }
      text.append("on update: ").append(fk->updateRule()).append("\n");
      text.append("on delete: ").append(fk->deleteRule());
    }
    return text;
  }

  return "";
}

GrtObjectRef WBComponentPhysical::get_object_for_figure(const model_ObjectRef &object) {
  if (workbench_physical_TableFigureRef::can_wrap(object))
    return workbench_physical_TableFigureRef::cast_from(object)->table();

  else if (workbench_physical_ViewFigureRef::can_wrap(object))
    return workbench_physical_ViewFigureRef::cast_from(object)->view();

  else if (workbench_physical_RoutineGroupFigureRef::can_wrap(object))
    return workbench_physical_RoutineGroupFigureRef::cast_from(object)->routineGroup();

  return GrtObjectRef();
}

void WBComponentPhysical::activate_canvas_object(const model_ObjectRef &figure, bool newwindow) {
  GrtObjectRef object(get_object_for_figure(figure));

  if (object.is_valid())
    bec::GRTManager::get()->open_object_editor(object, newwindow ? bec::ForceNewWindowFlag : bec::NoFlags);

  else if (workbench_physical_ConnectionRef::can_wrap(figure))
    bec::GRTManager::get()->open_object_editor(figure, newwindow ? bec::ForceNewWindowFlag : bec::NoFlags);
}

// TODO sigc _blockable_listeners seened never being filled and thus there is no sence to iterate there
void WBComponentPhysical::block_model_notifications() {
  /*
  //for (std::list<sigc::connection>::iterator iter= _blockable_listeners.begin();
       iter != _blockable_listeners.end(); ++iter)
    iter->block();
    */
}

void WBComponentPhysical::unblock_model_notifications() {
  /*
  //for (std::list<sigc::connection>::iterator iter= _blockable_listeners.begin();
       iter != _blockable_listeners.end(); ++iter)
    iter->unblock();
    */
}

//--------------------------------------------------------------------------------

app_ToolbarRef WBComponentPhysical::get_tools_toolbar() {
  return app_ToolbarRef::cast_from(
    grt::GRT::get()->unserialize(base::makePath(_wb->get_datadir(), "data/tools_toolbar_physical.xml")));
}

app_ToolbarRef WBComponentPhysical::get_tool_options(const std::string &tool) {
  if (_toolbars.find("options/" + tool) != _toolbars.end())
    return _toolbars["options/" + tool];
  return app_ToolbarRef();
}

grt::ListRef<app_ShortcutItem> WBComponentPhysical::get_shortcut_items() {
  return _shortcuts;
}

std::vector<std::string> WBComponentPhysical::get_command_dropdown_items(const std::string &option) {
  std::vector<std::string> items;
  ModelDiagramForm *form = dynamic_cast<ModelDiagramForm *>(_wb->get_active_main_form());

  if (base::hasPrefix(option, "workbench.physical.")) {
    if (base::hasSuffix(option, ":Color")) {
      std::string colors = _wb->get_wb_options().get_string("workbench.model.ObjectFigure:ColorList");
      std::vector<std::string> colorList;

      colorList = base::split(colors, "\n");

      if (!colorList.empty()) {
        for (std::size_t c = colorList.size(), i = 0; i < c; i++) {
          if (!colorList[i].empty() && colorList[i][0] == '#')
            items.push_back(colorList[i]);
        }
      } else {
        items.push_back("#98BFDA");
        items.push_back("#FEDE58");
        items.push_back("#98D8A5");

        items.push_back("#FE9898");
        items.push_back("#FE98FE");

        items.push_back("#FFFFFF");
      }

      std::string selected = form->get_tool_argument(option);
      if (selected.empty())
        selected = _wb->get_wb_options().get_string(option);
      if (selected.empty())
        selected = items[0];
      if (!selected.empty() && std::find(items.begin(), items.end(), selected) == items.end())
        items.push_back(selected);

      form->set_tool_argument(option, selected);
    } else if (base::hasSuffix(option, ":Template")) {
      grt::BaseListRef templates =
        grt::BaseListRef::cast_from(_wb->get_root()->options()->options().get("TableTemplates"));

      items.push_back("*None*");

      for (std::size_t i = 0; i < templates.count(); i++)
        items.push_back(db_TableRef::cast_from(templates[i])->name());

      form->set_tool_argument(option, "None");
    } else if (base::hasSuffix(option, ":Schema")) {
      workbench_physical_ModelRef model(get_parent_for_object<workbench_physical_Model>(form->get_model_diagram()));

      if (model.is_valid()) {
        for (std::size_t c = model->catalog()->schemata().count(), i = 0; i < c; i++)
          items.push_back(*model->catalog()->schemata().get(i)->name());
        std::sort(items.begin(), items.end());
      }

      std::string selected = form->get_tool_argument(option);
      if (!items.empty() && (selected.empty() || std::find(items.begin(), items.end(), selected) == items.end()))
        selected = items[0];

      form->set_tool_argument(option, selected);
    } else if (base::hasSuffix(option, ":Engine")) {
      items.push_back("*Default Engine*");

      grt::Module *module = grt::GRT::get()->get_module("DbMySQL");
      if (module) {
        grt::ListRef<db_mysql_StorageEngine> engines_ret(grt::ListRef<db_mysql_StorageEngine>::cast_from(
          module->call_function("getKnownEngines", grt::BaseListRef(true))));

        for (std::size_t c = engines_ret.count(), i = 0; i < c; i++)
          items.push_back(engines_ret[i]->name());
      }

      // Table engine might be model specific.
      workbench_physical_ModelRef model(get_parent_for_object<workbench_physical_Model>(form->get_model_diagram()));
      std::string selected;

      wb::WBContextUI::get()->get_wb_options_value(model.id(), "db.mysql.Table:tableEngine", selected);
      if (selected.empty())
        selected = "*Default Engine*";

      form->set_tool_argument(option, selected);
    } else if (base::hasSuffix(option, ":Collation")) {
      workbench_physical_ModelRef model(get_parent_for_object<workbench_physical_Model>(form->get_model_diagram()));

      if (_collation_list.empty()) {
        items.push_back("*Default Collation*");
        if (model.is_valid()) {
          for (std::size_t c = model->catalog()->characterSets().count(), i = 0; i < c; i++) {
            db_CharacterSetRef charset(model->catalog()->characterSets().get(i));

            for (std::size_t d = charset->collations().count(), j = 0; j < d; j++)
              items.push_back(*charset->collations().get(j));
          }
          std::sort(items.begin(), items.end());
        }
        _collation_list = items;
      } else
        items = _collation_list;

      std::string selected = form->get_tool_argument(option);
      if (selected.empty())
        selected = _wb->get_wb_options().get_string(option);
      if (selected.empty())
        selected = "*Default Collation*";

      form->set_tool_argument(option, selected);
    }
  } else
    throw std::logic_error("Unknown option " + option);

  return items;
}

//--------------------------------------------------------------------------------
// Object Listeners

void WBComponentPhysical::add_schema_listeners(const db_SchemaRef &schema) {
  std::map<std::string, boost::signals2::connection>::iterator it = _object_listeners.find(schema.id());
  if (it == _object_listeners.end()) {
    // listener for changes in schema itself
    _object_listeners[schema.id()] = schema->signal_changed()->connect(std::bind(
      &WBComponentPhysical::schema_member_changed, this, std::placeholders::_1, std::placeholders::_2, schema));

    _schema_content_listeners[schema.id()] = schema->signal_refreshDisplay()->connect(
      std::bind(&WBComponentPhysical::schema_content_object_changed, this, std::placeholders::_1));

    // for changes in table, view, SP/function, routine (and other) lists
    _schema_list_listeners[schema.id()] = schema->signal_list_changed()->connect(
      std::bind(&WBComponentPhysical::schema_object_list_changed, this, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, schema));
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Removes all previously set listeners.
 */
void WBComponentPhysical::close_document() {
  // Model listeners.
  _catalog_object_list_listener.disconnect();
  _model_list_listener.disconnect();

  // Object listeners.
  for (std::map<std::string, boost::signals2::connection>::iterator iter = _object_listeners.begin();
       iter != _object_listeners.end(); ++iter)
    iter->second.disconnect();
  _object_listeners.clear();

  // Schema listeners.
  for (std::map<std::string, boost::signals2::connection>::iterator iter = _schema_list_listeners.begin();
       iter != _schema_list_listeners.end(); ++iter)
    iter->second.disconnect();
  _schema_list_listeners.clear();

  // Figure list listeners.
  for (std::map<std::string, boost::signals2::connection>::iterator iter = _figure_list_listeners.begin();
       iter != _figure_list_listeners.end(); ++iter)
    iter->second.disconnect();
  _figure_list_listeners.clear();
}

//--------------------------------------------------------------------------------------------------

/**
 * Refreshes internal state for a document change.
 * This will add listeners for a newly created or opened document.
 *
 */
void WBComponentPhysical::reset_document() {
  workbench_DocumentRef doc(_wb->get_document());

  // add listener to all schemas, views etc
  for (std::size_t c = doc->physicalModels().count(), i = 0; i < c; i++) {
    workbench_physical_ModelRef model(doc->physicalModels()[i]);
    if (model.is_valid()) {
      db_CatalogRef catalog(model->catalog());

      if (catalog.is_valid())
        _catalog_object_list_listener = catalog->signal_list_changed()->connect(
          std::bind(&WBComponentPhysical::catalog_object_list_changed, this, std::placeholders::_1,
                    std::placeholders::_2, std::placeholders::_3, catalog));

      for (std::size_t sc = catalog->schemata().count(), si = 0; si < sc; si++) {
        db_SchemaRef schema(catalog->schemata().get(si));

        add_schema_listeners(schema);

        // currently there are only listeners for tables
        grt::ListRef<db_Table> tables(schema->tables());
        for (std::size_t tc = tables.count(), ti = 0; ti < tc; ti++) {
          add_schema_object_listeners(tables[ti]);
        }
      }

      for (std::size_t vi = 0, vc = model->diagrams().count(); vi < vc; vi++) {
        model_DiagramRef view(model->diagrams().get(i));
        _figure_list_listeners[view.id()] = view->signal_list_changed()->connect(
          std::bind(&WBComponentPhysical::view_object_list_changed, this, std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3, view));
      }

      _model_list_listener = model->signal_list_changed()->connect(
        std::bind(&WBComponentPhysical::model_object_list_changed, this, std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3));
    }
  }

  ((PhysicalOverviewBE *)wb::WBContextUI::get()->get_physical_overview())->send_refresh_scripts();
  ((PhysicalOverviewBE *)wb::WBContextUI::get()->get_physical_overview())->send_refresh_notes();
}

/** Called when a document is loaded

 - Update the userDatatypes list in the document with what we have now
 */
void WBComponentPhysical::document_loaded() {
  grt::ListRef<workbench_physical_Model> models(_wb->get_document()->physicalModels());

  for (grt::ListRef<workbench_physical_Model>::const_iterator pmodel = models.begin(); pmodel != models.end();
       ++pmodel) {
    db_CatalogRef catalog((*pmodel)->catalog());
    db_mgmt_RdbmsRef rdbms((*pmodel)->rdbms());

    // ml: see comment for other place with that code in this file.
    // grt::ListRef<db_UserDatatype> userTypes(create_builtin_user_datatypes(catalog, rdbms));

    // merge in built-in UDTs with the one from the model, replacing stuff from the model with ours
    // grt::merge_contents_by_id(catalog->userDatatypes(), userTypes, true);

    // Merge simple datatypes and charsets, in case some new type was added to WB since the model was created
    grt::merge_contents_by_id(catalog->simpleDatatypes(), rdbms->simpleDatatypes(), false);
    grt::merge_contents_by_id(catalog->characterSets(), rdbms->characterSets(), false);
  }
}

/** Listener for changes in the list of schemas
 *
 * Used for attaching listeners to new schemas and content lists.
 */
void WBComponentPhysical::catalog_object_list_changed(grt::internal::OwnedList *list, bool added,
                                                      const grt::ValueRef &value, const db_CatalogRef &catalog) {
  if (grt::BaseListRef(list) == catalog->schemata()) {
    // we're called in the GRT thread, so just mark the refresh request
    // as pending. This has the bonus that multiple requests will be
    // collapsed into 1.
    // The requests are actually sent in flush_idle_tasks()

    // Send refresh messages for various UI parts.
    _wb->request_refresh(RefreshOverviewNodeChildren, "", 0); // Non-recursive refresh for the overview page.

    // A refresh for the schema list specifically.
    ((PhysicalOverviewBE *)wb::WBContextUI::get()->get_physical_overview())->send_refresh_schema_list();

    if (added) {
      // added new schema
      add_schema_listeners(db_SchemaRef::cast_from(value));
      _wb->get_model_context()->notify_catalog_tree_view(NodeAddUpdate, value);
    } else {
      db_SchemaRef schema(db_SchemaRef::cast_from(value));

      _wb->request_refresh(RefreshCloseEditor, schema.id());

      // remove listeners for the schema
      _object_listeners[schema.id()].disconnect();
      _schema_list_listeners[schema.id()].disconnect();

      _object_listeners.erase(schema.id());
      _schema_list_listeners.erase(schema.id());
      _wb->get_model_context()->notify_catalog_tree_view(NodeDelete, schema);
    }
  } else
    privilege_list_changed(list, added, value, catalog);
}

void WBComponentPhysical::schema_member_changed(const std::string &member, const grt::ValueRef &ovalue,
                                                const db_SchemaRef &schema) {
  if (wb::WBContextUI::get()->get_physical_overview())
    ((PhysicalOverviewBE *)wb::WBContextUI::get()->get_physical_overview())->send_refresh_for_schema(schema, true);

  _wb->get_model_context()->notify_catalog_tree_view(NodeAddUpdate, schema);
}

void WBComponentPhysical::add_schema_object_listeners(const grt::ObjectRef &object) {
  if (object.is_instance(db_Table::static_class_name())) {
    if (_object_listeners.find(object.id()) != _object_listeners.end())
      _object_listeners[object.id()].disconnect();
    _object_listeners[object.id()] = db_TableRef::cast_from(object)->signal_foreignKeyChanged()->connect(
      std::bind(&WBComponentPhysical::foreign_key_changed, this, std::placeholders::_1));
  }
}

void WBComponentPhysical::schema_object_list_changed(grt::internal::OwnedList *list, bool added,
                                                     const grt::ValueRef &value, const db_SchemaRef &schema) {
  grt::ObjectRef object(grt::ObjectRef::cast_from(value));

  if (added)
    add_schema_object_listeners(object);
  else {
    _wb->get_model_context()->notify_catalog_tree_view(NodeDelete, value);
    // remove old listeners
    if (object.is_instance(db_Table::static_class_name())) {
      _object_listeners[object.id()].disconnect();
      _object_listeners.erase(object.id());
    }
    _wb->request_refresh(RefreshCloseEditor, object.id());
  }

  if (wb::WBContextUI::get()->get_physical_overview())
    ((PhysicalOverviewBE *)wb::WBContextUI::get()->get_physical_overview())
      ->send_refresh_for_schema_object(GrtObjectRef::cast_from(value), false);
}

void WBComponentPhysical::view_object_list_changed(grt::internal::OwnedList *list, bool added,
                                                   const grt::ValueRef &value, const model_DiagramRef &view) {
  if (list == view->figures().valueptr()) {
    if (handles_figure(model_ObjectRef::cast_from(value))) {
      if (added) {
#if 0
        // if not undoing/redoing, then auto-create the connection for the table
        if (!get_grt()->get_undo_manager()->is_undoing() && !get_grt()->get_undo_manager()->is_redoing())
        {
          if (value.is_instance(workbench_physical_TableFigure::static_class_name()))
          {
            workbench_physical_TableFigureRef table(workbench_physical_TableFigureRef::cast_from(value));
            
            if (table.table().is_valid())
            {
              std::vector<db_TableRef> tbls;
              tbls.push_back(table.table());
              
              update_connections_for_new_tables(view, tbls);
            }
            else
             grt::GRT::get()->send_warning("Table figure added to view has no table db object bound to it.");
          }
        }
#endif
      } else {
      }
    } else {
      // make sure editor for notes and images are closed
      // this should be in wb_component_basic.cpp, but since there's no need for anything
      // else over there, just put here
      if (!added)
        _wb->request_refresh(RefreshCloseEditor, grt::ObjectRef::cast_from(value).id());
    }
  } else if (list == view->layers().valueptr() || list == view->connections().valueptr()) {
    if (!added)
      _wb->request_refresh(RefreshCloseEditor, grt::ObjectRef::cast_from(value).id());
  }
}

// static void convert_utf16_to_utf8_if_needed(char *&data, size_t &size)
//{
//  gchar *output;
//  glong iread, iwritten;
//
//  // check if utf16
//  if (size >= 2 && (guchar)data[0] == 0xff && (guchar)data[1] == 0xfe)
//  {
//    output= g_utf16_to_utf8((const gunichar2*)data, (glong) size, &iread, &iwritten, NULL);
//    if (output)
//    {
//      g_free(data);
//      data= output;
//      size= iwritten;
//    }
//  }
//}

void WBComponentPhysical::model_object_list_changed(grt::internal::OwnedList *list, bool added,
                                                    const grt::ValueRef &avalue) {
  if (avalue.type() == grt::ObjectType) {
    if (added) {
      grt::ObjectRef value(grt::ObjectRef::cast_from(avalue));
      std::string group, tmpl;

      if (value.is_instance(db_Script::static_class_name())) {
        group = "@sqlscripts";
        tmpl = "script$.sql";
      } else if (value.is_instance(GrtStoredNote::static_class_name())) {
        group = "@notes";
        tmpl = "note$.txt";
      } else if (value.is_instance(model_Diagram::static_class_name())) {
        model_DiagramRef view(model_DiagramRef::cast_from(value));

        _figure_list_listeners[view.id()] = view->signal_list_changed()->connect(
          std::bind(&WBComponentPhysical::view_object_list_changed, this, std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3, view));

        wb::WBContextUI::get()->get_physical_overview()->send_refresh_diagram(model_DiagramRef());
        return;
      } else
        return;

      // check if the filename is in the temp dir, if so, just readd it with old contents
      GrtStoredNoteRef object(GrtStoredNoteRef::cast_from(value));

#if 0
      if (object->filename() != "")
      {
        if (object->filename().c_str()[0] == '@')
        {
          gchar *data;
          gsize length;
          std::string path= base::makePath(bec::GRTManager::get()->get_tmp_dir(), object->filename());
          
          if (g_file_get_contents(path.c_str(), &data, &length, NULL))
          {
            // we're undoing, load the file and add it back to the model file
            object->filename(_wb->recreate_attached_file(object->filename(), data));
            g_free(data);
          }
          else
           grt::GRT::get()->send_error(strfmt(_("Error undoing file delete: can't read contents of temporary file '%s'"), path.c_str()));
        }
        else
        {
          // load file from scratch
          gchar *data;
          gsize length;
          GError *error= NULL;
          
          if (g_file_get_contents(object->filename().c_str(), &data, &length, &error))
          {
            convert_utf16_to_utf8_if_needed(data, length);
            
            object->filename(_wb->create_attached_file(group, object->filename()));
            _wb->save_attached_file_contents(object->filename(), data, length);
            g_free(data);
          }
          else
          {
           grt::GRT::get()->send_error(strfmt(_("Error adding file: can't read contents of file '%s': %s"), 
                                      object->filename().c_str(), error->message));
            g_error_free(error);
          }
        }
      }
      else
        object->filename(_wb->create_attached_file(group, tmpl));
#endif
      // request refresh of overview
      if (value.is_instance(db_Script::static_class_name()))
        ((PhysicalOverviewBE *)wb::WBContextUI::get()->get_physical_overview())->send_refresh_scripts();
      else if (value.is_instance(GrtStoredNote::static_class_name()))
        ((PhysicalOverviewBE *)wb::WBContextUI::get()->get_physical_overview())->send_refresh_notes();
    } else // !added
    {
      grt::ObjectRef value(grt::ObjectRef::cast_from(avalue));

      _wb->request_refresh(RefreshCloseEditor, value.id());

      if (value.is_instance(GrtStoredNote::static_class_name())) {
        GrtStoredNoteRef object(GrtStoredNoteRef::cast_from(value));
#if 0
        if (object->filename() != "")
        {
          std::string path= base::makePath(bec::GRTManager::get()->get_tmp_dir(), object->filename());
          std::string tmp= base::dirname(path.c_str());
          g_mkdir_with_parents(tmp.c_str(), 0700);
          
          // save the file in the temp dir in case an undo delete is requested
          std::string data= _wb->get_attached_file_contents(object->filename());
          
          if (!g_file_set_contents(path.c_str(), data.data(), (gssize) data.length(), NULL))
          {
           grt::GRT::get()->send_error(strfmt(_("Could not save temporary file '%s'"), path.c_str()));
          }
          
          _wb->delete_attached_file(object->filename());
        }
#endif
        // request refresh of overview
        if (value.is_instance(db_Script::static_class_name()))
          ((PhysicalOverviewBE *)wb::WBContextUI::get()->get_physical_overview())->send_refresh_scripts();
        else
          ((PhysicalOverviewBE *)wb::WBContextUI::get()->get_physical_overview())->send_refresh_notes();
      } else if (value.is_instance(model_Diagram::static_class_name())) {
        std::string id = grt::ObjectRef::cast_from(value).id();

        _figure_list_listeners[id].disconnect();
        _figure_list_listeners.erase(id);

        wb::WBContextUI::get()->get_physical_overview()->send_refresh_diagram(model_DiagramRef());
      }
    }
  }
}

void WBComponentPhysical::foreign_key_changed(const db_ForeignKeyRef &fk) {
  // don't auto-create/remove stuff when undoing/redoing
  if (grt::GRT::get()->get_undo_manager()->is_undoing() || grt::GRT::get()->get_undo_manager()->is_redoing())
    return;

  if (!_wb->get_document().is_valid())
    return;

  bool valid = fk->checkCompleteness() != 0;
  grt::ListRef<workbench_physical_Diagram> views(_wb->get_document()->physicalModels()[0]->diagrams());

  // go through all views and create/destroy connections if needed
  for (grt::ListRef<workbench_physical_Diagram>::const_iterator iter = views.begin(); iter != views.end(); ++iter) {
    workbench_physical_DiagramRef view(*iter);
    workbench_physical_ConnectionRef conn(view->getConnectionForForeignKey(fk));

    if (conn.is_valid() != valid) {
      if (!conn.is_valid())
        view->createConnectionForForeignKey(fk);
      else
        view->removeConnection(conn);
    } else {
      if (conn.is_valid())
        view->removeConnection(conn);
      view->createConnectionForForeignKey(fk);
    }
  }
}

void WBComponentPhysical::schema_content_object_changed(const db_DatabaseObjectRef &object) {
  refresh_ui_for_object(object);
}

//--------------------------------------------------------------------------------------------------

/**
 * Called if a property for an object is changed. Doesn't usually require a full refresh in the UI
 * but rather only updates for displays of properties of this object.
 * TODO: this function is called multiple times for a single change. Optimized this situation!
 */
void WBComponentPhysical::refresh_ui_for_object(const GrtObjectRef &object) {
  if (object.is_valid() && object->owner().is_valid()) {
    workbench_physical_ModelRef model(get_parent_for_object<workbench_physical_Model>(object));
    PhysicalOverviewBE *overview =
      (PhysicalOverviewBE *)((PhysicalOverviewBE *)wb::WBContextUI::get()->get_physical_overview());

    if (overview->get_model() != model)
      throw std::logic_error("code is outdated");

    overview->send_refresh_for_schema_object(object, true);

    // When adding a new object this call is triggered in addition to a full refresh of the catalog
    // tree due to the change of the owners child list and hence basically useless in this case.
    // Less impact if we have updates for individual nodes one day.
    //_catalog_tree->refresh_node(object.owner().id()); Requires a node id.
    _wb->get_model_context()->notify_catalog_tree_view(NodeAddUpdate, object);
  }
}

/**
 ****************************************************************************
 * @brief Add/remove connections for added/removed table foreign keys
 *
 * Updates the connections from a table figure corresponding to the given
 * foreign key. This can be called when the list of FKs for a table is
 * changed or when a pre-existing table with a fk is added to a view.
 * It will also update existing connections in case the referenced table
 * of a fk changes.
 *
 * @param object the db object
 * @param figure the model figure that was assigned to the object
 * @param attach whether the figure was attached or detached to the object
 *
 * @return true if a change has happened (conn added/remove), false otherwise
 ****************************************************************************
 */
bool WBComponentPhysical::update_table_fk_connection(const db_TableRef &table, const db_ForeignKeyRef &fk, bool added) {
  workbench_physical_ModelRef model(get_parent_for_object<workbench_physical_Model>(table));

  if (!model.is_valid() || !model->diagrams().is_valid())
    return false;

  if (!fk.is_valid()) {
    if (!added) {
      // all FKs from table removed
    }
  } else {
    if (!fk->referencedTable().is_valid() || fk->owner() != table)
      return false;

    // kind of a hack, this is needed because copy_object() will generate
    // spurious notifications. eg: in a sync of a model with a relationship,
    // the rel will be recreated (because of the copy)
    if (added && table->foreignKeys().get_index(fk) == BaseListRef::npos)
      return false;

    grt::ListRef<workbench_physical_Diagram> views(model->diagrams());
    if (added) {
      // we have to go through all views in the model and find all table figures
      // that correspond to the FK for creating the relationship in all these views

      for (std::size_t c = views.count(), i = 0; i < c; i++) {
        workbench_physical_DiagramRef view(views[i]);

        workbench_physical_TableFigureRef table1(
          workbench_physical_TableFigureRef::cast_from(view->getFigureForDBObject(table)));
        workbench_physical_TableFigureRef table2(
          workbench_physical_TableFigureRef::cast_from(view->getFigureForDBObject(fk->referencedTable())));

        if (table1.is_valid() &&
            table2.is_valid()) { // both tables in the relationship are in this view, so create the connection

          // but 1st check if it already exists

          grt::ListRef<model_Connection> connections(view->connections());
          workbench_physical_ConnectionRef found;

          for (std::size_t c = connections.count(), i = 0; i < c; i++) {
            model_ConnectionRef conn(connections[i]);
            if (conn.is_instance(workbench_physical_Connection::static_class_name())) {
              workbench_physical_ConnectionRef pconn(workbench_physical_ConnectionRef::cast_from(conn));

              if (pconn->foreignKey() == fk) {
                found = pconn;
                break;
              }
            }
          }

          // connection doesnt exist yet, create it
          if (!found.is_valid()) {
            workbench_physical_ConnectionRef conn(grt::Initialized);

            conn->owner(view);
            conn->startFigure(table1);
            conn->endFigure(table2);
            conn->caption(fk->name());
            conn->foreignKey(fk);

            conn->name(grt::get_name_suggestion_for_list_object(view->connections(), "connection"));

            grt::AutoUndo undo;
            view->connections().insert(conn);
            undo.end(_("Create Relationship"));

            return true;
          } else {
            // connection exists, check if its correct
            if (workbench_physical_TableFigureRef::cast_from(found->endFigure())->table() !=
                found->foreignKey()->referencedTable()) {
              if (!found->foreignKey()->referencedTable().is_valid()) {
                // referencedTable was unset, so we need to remove the connection

              } else {
                // update the connection with the new end figure
                grt::AutoUndo undo;
                found->endFigure(view->getFigureForDBObject(fk->referencedTable()));
                undo.end(_("Update Relationship"));
              }
              return true;
            }
          }
        }
      }
    } else {
      // remove all connections that correspond to the FK in all views

      for (grt::ListRef<workbench_physical_Diagram>::const_iterator view = views.begin(); view != views.end(); ++view) {
        workbench_physical_TableFigureRef table1(
          workbench_physical_TableFigureRef::cast_from((*view)->getFigureForDBObject(table)));
        grt::ListRef<model_Connection>::const_reverse_iterator end = (*view)->connections().rend();

        for (grt::ListRef<model_Connection>::const_reverse_iterator conn = (*view)->connections().rbegin(); conn != end;
             ++conn) {
          if ((*conn).is_instance(workbench_physical_Connection::static_class_name())) {
            workbench_physical_ConnectionRef pconn(workbench_physical_ConnectionRef::cast_from(*conn));

            if (pconn->foreignKey() == fk) {
              grt::AutoUndo undo;
              (*view)->connections().remove_value(*conn);
              undo.end(_("Remove Relationship"));

              return true;
            }
          }
        }
      }
    }
  }
  return false;
}

bool WBComponentPhysical::has_figure_for_object_in_active_view(const GrtObjectRef &object, ModelDiagramForm *vform) {
  if (!vform)
    vform = dynamic_cast<ModelDiagramForm *>(_wb->get_active_main_form());

  if (vform) {
    workbench_physical_DiagramRef view(workbench_physical_DiagramRef::cast_from(vform->get_model_diagram()));

    if (view->getFigureForDBObject(db_DatabaseObjectRef::cast_from(object)).is_valid())
      return true;
  }
  return false;
}

void WBComponentPhysical::setup_canvas_tool(ModelDiagramForm *view, const std::string &tool) {
  void *data = 0;
  bool relationship = false;

  if (tool == WB_TOOL_PTABLE) {
    if (mforms::App::get()->isDarkModeActive()) {
      view->set_cursor("table_dark");
    } else {
      view->set_cursor("table");
    }
    _wb->_frontendCallbacks->show_status_text(_("Select location for new table."));
  } else if (tool == WB_TOOL_PVIEW) {
    if (mforms::App::get()->isDarkModeActive()) {
      view->set_cursor("view_dark");
    } else {
      view->set_cursor("view");
    }
    _wb->_frontendCallbacks->show_status_text(_("Select location for new view."));
  } else if (tool == WB_TOOL_PROUTINEGROUP) {
    if (mforms::App::get()->isDarkModeActive()) {
      view->set_cursor("routine_dark");
    } else {
      view->set_cursor("routine");
    }
    _wb->_frontendCallbacks->show_status_text(_("Select location for new routine group."));
  } else if (tool == WB_TOOL_PREL11) {
    if (mforms::App::get()->isDarkModeActive()) {
      view->set_cursor("rel11_dark");
    } else {
      view->set_cursor("rel11");
    }
    data = start_relationship(view, Point(), Relationship11Id);
    relationship = true;
  } else if (tool == WB_TOOL_PREL1n) {
    if (mforms::App::get()->isDarkModeActive()) {
      view->set_cursor("rel1n_dark");
    } else {
      view->set_cursor("rel1n");
    }
    data = start_relationship(view, Point(), Relationship1nId);
    relationship = true;
  } else if (tool == WB_TOOL_PRELnm) {
    if (mforms::App::get()->isDarkModeActive()) {
      view->set_cursor("relnm_dark");
    } else {
      view->set_cursor("relnm");
    }
    data = start_relationship(view, Point(), RelationshipnmId);
    relationship = true;
  } else if (tool == WB_TOOL_PREL11_NOID) {
    if (mforms::App::get()->isDarkModeActive()) {
      view->set_cursor("rel11_dark");
    } else {
      view->set_cursor("rel11");
    }
    data = start_relationship(view, Point(), Relationship11NonId);
    relationship = true;
  } else if (tool == WB_TOOL_PREL1n_NOID) {
    if (mforms::App::get()->isDarkModeActive()) {
      view->set_cursor("rel1n_dark");
    } else {
      view->set_cursor("rel1n");
    }
    data = start_relationship(view, Point(), Relationship1nNonId);
    relationship = true;
  } else if (tool == WB_TOOL_PREL_PICK) {
    if (mforms::App::get()->isDarkModeActive()) {
      view->set_cursor("rel1n_dark");
    } else {
      view->set_cursor("rel1n");
    }
    data = start_relationship(view, Point(), RelationshipPick);
    relationship = true;
  } else {
    _wb->_frontendCallbacks->show_status_text("Invalid tool " + tool);
    return;
  }
  view->set_button_callback(std::bind(&WBComponentPhysical::handle_button_event, this, std::placeholders::_1,
                                      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
                                      std::placeholders::_5, data));
  if (relationship)
    view->set_reset_tool_callback(std::bind(&WBComponentPhysical::cancel_relationship, this, std::placeholders::_1,
                                            reinterpret_cast<RelationshipToolContext *>(data)));
}

bool WBComponentPhysical::handle_button_event(ModelDiagramForm *view, mdc::MouseButton button, bool press, Point pos,
                                              mdc::EventState, void *data) {
  std::string tool = view->get_tool();

  if (button != mdc::ButtonLeft)
    return false;

  mdc::CanvasItem *item;
  item = view->get_view()->get_item_at(pos);
  if (item && item->get_layer() != view->get_view()->get_current_layer())
    return false;

  if (tool == WB_TOOL_PTABLE) {
    if (press) {
      place_new_db_object(view, pos, ObjectTable);
      view->reset_tool(true);
      return true;
    }
  } else if (tool == WB_TOOL_PROUTINEGROUP) {
    if (press) {
      place_new_db_object(view, pos, ObjectRoutineGroup);
      view->reset_tool(true);
      return true;
    }
  } else if (tool == WB_TOOL_PVIEW) {
    if (press) {
      place_new_db_object(view, pos, ObjectView);
      view->reset_tool(true);
      return true;
    }
  } else if (tool == WB_TOOL_PREL11 || tool == WB_TOOL_PREL1n || tool == WB_TOOL_PRELnm ||
             tool == WB_TOOL_PREL11_NOID || tool == WB_TOOL_PREL1n_NOID || tool == WB_TOOL_PREL_PICK) {
    if (press) {
      RelationshipToolContext *rctx = reinterpret_cast<RelationshipToolContext *>(data);

      if (rctx->button_press(view, pos))
        view->reset_tool(true);
      return true;
    }
  }

  return false;
}

//===================================================================================================
// Privilege Stuff
//===================================================================================================

// static void refresh_privileges(PhysicalOverviewBE *overview)
//{
//  overview->send_refresh_users();
//  overview->send_refresh_roles();
//}

void WBComponentPhysical::privilege_list_changed(grt::internal::OwnedList *list, bool added, const grt::ValueRef &value,
                                                 const db_CatalogRef &catalog) {
  if (grt::BaseListRef(list) == catalog->users())
    ((PhysicalOverviewBE *)wb::WBContextUI::get()->get_physical_overview())->send_refresh_users();
  else if (grt::BaseListRef(list) == catalog->roles())
    ((PhysicalOverviewBE *)wb::WBContextUI::get()->get_physical_overview())->send_refresh_roles();
  if (!added) {
    grt::ObjectRef object(grt::ObjectRef::cast_from(value));
    _wb->request_refresh(RefreshCloseEditor, object.id());
  }
}

//--------------------------------------------------------------------------------
// Privilege Management

db_UserRef WBComponentPhysical::add_new_user(const workbench_physical_ModelRef &model) {
  db_CatalogRef catalog;
  db_UserRef user;

  catalog = model->catalog();

  std::string name = grt::get_name_suggestion_for_list_object(grt::ObjectListRef::cast_from(catalog->users()), "user");

  user = db_UserRef(grt::Initialized);
  user->owner(catalog);
  user->name(name);

  grt::AutoUndo undo;
  catalog->users().insert(user);

  undo.end(strfmt(_("Create User '%s'"), user->name().c_str()));

  _wb->_frontendCallbacks->show_status_text(strfmt(_("User '%s' created"), user->name().c_str()));

  return user;
}

void WBComponentPhysical::remove_user(const db_UserRef &user) {
  db_CatalogRef catalog(db_CatalogRef::cast_from(user->owner()));

  grt::AutoUndo undo;
  catalog->users().remove_value(user);
  undo.end(strfmt(_("Remove User '%s'"), user->name().c_str()));

  _wb->_frontendCallbacks->show_status_text(strfmt(_("Removed user '%s'"), user->name().c_str()));
}

db_RoleRef WBComponentPhysical::add_new_role(const workbench_physical_ModelRef &model) {
  db_CatalogRef catalog;
  db_RoleRef role;

  catalog = model->catalog();

  std::string name = grt::get_name_suggestion_for_list_object(grt::ObjectListRef::cast_from(catalog->roles()), "role");

  role = db_RoleRef(grt::Initialized);
  role->owner(catalog);
  role->name(name);

  grt::AutoUndo undo;
  catalog->roles().insert(role);
  undo.end(strfmt(_("Create Role '%s'"), role->name().c_str()));

  _wb->_frontendCallbacks->show_status_text(strfmt(_("Role '%s' created"), role->name().c_str()));

  return role;
}

void WBComponentPhysical::remove_role(const db_RoleRef &role) {
  db_CatalogRef catalog(db_CatalogRef::cast_from(role->owner()));

  for (std::size_t index = 0; index < catalog->users().count(); ++index)
      catalog->users()[index]->roles().remove_value(role);
  
  for (std::size_t index = 0; index < catalog->roles().count(); ++index) {
      db_RoleRef r = catalog->roles()[index];
      r->childRoles().remove_value(role);
      
      if (r->parentRole().is_valid() && r->parentRole()->name() == role->name())
          r->parentRole(db_RoleRef());
  }
  
  grt::AutoUndo undo;
  catalog->roles().remove_value(role);
  ((PhysicalOverviewBE *)wb::WBContextUI::get()->get_physical_overview())->send_refresh_roles();
  undo.end(strfmt(_("Remove Role '%s'"), role->name().c_str()));

  _wb->_frontendCallbacks->show_status_text(strfmt(_("Removed role '%s'"), role->name().c_str()));
}

void WBComponentPhysical::remove_references_to_object(const db_DatabaseObjectRef &object) {
  // this is called when a object is removed from the schema.

  // remove all role privileges assigned to the object
  db_CatalogRef catalog(get_parent_for_object<db_Catalog>(object));

  grt::ListRef<db_Role> roles(catalog->roles());
  {
    grt::AutoUndo undo;

    for (std::size_t c = roles.count(), i = 0; i < c; i++) {
      db_RoleRef role(roles[i]);

      grt::ListRef<db_RolePrivilege> privs(role->privileges());
      std::list<db_RolePrivilegeRef> privs_to_remove;
      for (grt::ListRef<db_RolePrivilege>::const_reverse_iterator priv = privs.rbegin(); priv != privs.rend(); ++priv) {
        if ((*priv)->databaseObject() == object) {
          privs_to_remove.push_back(*priv);
        }
      }
      for (std::list<db_RolePrivilegeRef>::const_iterator It = privs_to_remove.begin(); It != privs_to_remove.end();
           ++It)
        privs.remove_value(*It);
    }
    undo.end_or_cancel_if_empty(_("Remove Object Privileges"));
  }

  workbench_physical_ModelRef model(get_parent_for_object<workbench_physical_Model>(object));
  // remove any tags that reference this object
  if (model.is_valid()) {
    grt::AutoUndo undo;

    for (grt::ListRef<meta_Tag>::const_iterator end = model->tags().end(), tag = model->tags().begin(); tag != end;
         ++tag) {
      std::list<meta_TaggedObjectRef> tags_to_remove;
      for (grt::ListRef<meta_TaggedObject>::const_reverse_iterator rend = (*tag)->objects().rend(),
                                                                   to = (*tag)->objects().rbegin();
           to != rend; ++to) {
        if ((*to)->object() == object) {
          tags_to_remove.push_back(*to);
        }
      }
      for (std::list<meta_TaggedObjectRef>::const_iterator It = tags_to_remove.begin(); It != tags_to_remove.end();
           ++It)
        (*tag)->objects().remove_value(*It);
    }

    undo.end_or_cancel_if_empty(_("Remove Tags for Object"));
  }
}
