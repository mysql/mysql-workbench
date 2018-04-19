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

#include "grt.h"
#include "interfaces/plugin.h"

#include "grts/structs.db.mgmt.h"

#define MODULE_VERSION "1.0.0"

#ifdef _MSC_VER
#define FRONTEND_LIBNAME(obj, windows_dll, linux_so, osx_dylib) obj->moduleName(windows_dll)
#elif defined(__APPLE__)
#define FRONTEND_LIBNAME(obj, windows_dll, linux_so, osx_dylib) obj->moduleName(osx_dylib)
#else
#define FRONTEND_LIBNAME(obj, windows_dll, linux_so, osx_dylib) obj->moduleName(linux_so)
#endif

static grt::ListRef<app_Plugin> get_mysql_plugins_info();

class MySQLEditorsModuleImpl : public grt::ModuleImplBase, public PluginInterfaceImpl {
public:
  MySQLEditorsModuleImpl(grt::CPPModuleLoader *ldr) : grt::ModuleImplBase(ldr) {
  }

  DEFINE_INIT_MODULE(MODULE_VERSION, "Oracle and/or its affiliates", grt::ModuleImplBase,
                     DECLARE_MODULE_FUNCTION(MySQLEditorsModuleImpl::getPluginInfo), NULL);

  virtual grt::ListRef<app_Plugin> getPluginInfo() override {
    return get_mysql_plugins_info();
  }
};

static void set_object_argument(app_PluginRef &plugin, const std::string &struct_name) {
  app_PluginObjectInputRef pdef(grt::Initialized);

  pdef->objectStructName(struct_name);
  pdef->owner(plugin);

  plugin->inputValues().insert(pdef);
}

static grt::ListRef<app_Plugin> get_mysql_plugins_info() {
  grt::ListRef<app_Plugin> editors(grt::Initialized);

  app_PluginRef schema_editor(grt::Initialized);
  app_PluginRef table_editor(grt::Initialized);
  app_PluginRef view_editor(grt::Initialized);
  app_PluginRef routine_group_editor(grt::Initialized);
  app_PluginRef routine_editor(grt::Initialized);
  app_PluginRef user_editor(grt::Initialized);
  app_PluginRef role_editor(grt::Initialized);
  app_PluginRef relationship_editor(grt::Initialized);

  FRONTEND_LIBNAME(schema_editor, ".\\db.mysql.editors.wbp.fe.dll", "db.mysql.editors.wbp.so",
                   "db.mysql.editors.mwbplugin");
  schema_editor->pluginType("gui");
  schema_editor->moduleFunctionName("DbMysqlSchemaEditor");
  set_object_argument(schema_editor, "db.mysql.Schema");
  schema_editor->rating(10);
  schema_editor->caption("Edit Schema");
  schema_editor->name("db.mysql.plugin.edit.schema");
  schema_editor->groups().insert("catalog/Editors");
  editors.insert(schema_editor);

  FRONTEND_LIBNAME(table_editor, ".\\db.mysql.editors.wbp.fe.dll", "db.mysql.editors.wbp.so",
                   "db.mysql.editors.mwbplugin");
  table_editor->pluginType("gui");
  table_editor->moduleFunctionName("DbMysqlTableEditor");
  set_object_argument(table_editor, "db.mysql.Table");
  table_editor->caption("Edit Table");
  table_editor->rating(10);
  table_editor->name("db.mysql.plugin.edit.table");
  table_editor->groups().insert("catalog/Editors");
  editors.insert(table_editor);

  FRONTEND_LIBNAME(view_editor, ".\\db.mysql.editors.wbp.fe.dll", "db.mysql.editors.wbp.so",
                   "db.mysql.editors.mwbplugin");
  view_editor->pluginType("gui");
  view_editor->moduleFunctionName("DbMysqlViewEditor");
  set_object_argument(view_editor, "db.mysql.View");
  view_editor->caption("Edit View");
  view_editor->rating(10);
  view_editor->name("db.mysql.plugin.edit.view");
  view_editor->groups().insert("catalog/Editors");
  editors.insert(view_editor);

  FRONTEND_LIBNAME(routine_group_editor, ".\\db.mysql.editors.wbp.fe.dll", "db.mysql.editors.wbp.so",
                   "db.mysql.editors.mwbplugin");
  routine_group_editor->pluginType("gui");
  routine_group_editor->moduleFunctionName("DbMysqlRoutineGroupEditor");
  set_object_argument(routine_group_editor, "db.mysql.RoutineGroup");
  routine_group_editor->caption("Edit Routine Group");
  routine_group_editor->rating(10);
  routine_group_editor->name("db.mysql.plugin.edit.routineGroup");
  routine_group_editor->groups().insert("catalog/Editors");
  editors.insert(routine_group_editor);

  FRONTEND_LIBNAME(routine_editor, ".\\db.mysql.editors.wbp.fe.dll", "db.mysql.editors.wbp.so",
                   "db.mysql.editors.mwbplugin");
  routine_editor->pluginType("gui");
  routine_editor->moduleFunctionName("DbMysqlRoutineEditor");
  set_object_argument(routine_editor, "db.mysql.Routine");
  routine_editor->caption("Edit Routine");
  routine_editor->rating(10);
  routine_editor->name("db.mysql.plugin.edit.routine");
  routine_editor->groups().insert("catalog/Editors");
  editors.insert(routine_editor);

  // generic
  FRONTEND_LIBNAME(user_editor, ".\\db.mysql.editors.wbp.fe.dll", "db.mysql.editors.wbp.so",
                   "db.mysql.editors.mwbplugin");
  user_editor->pluginType("gui");
  user_editor->moduleFunctionName("DbMysqlUserEditor");
  set_object_argument(user_editor, "db.User");
  user_editor->caption("Edit User");
  user_editor->rating(10);
  user_editor->name("db.mysql.plugin.edit.user");
  user_editor->groups().insert("catalog/Editors");
  editors.insert(user_editor);

  FRONTEND_LIBNAME(role_editor, ".\\db.mysql.editors.wbp.fe.dll", "db.mysql.editors.wbp.so",
                   "db.mysql.editors.mwbplugin");
  role_editor->pluginType("gui");
  role_editor->moduleFunctionName("DbMysqlRoleEditor");
  set_object_argument(role_editor, "db.Role");
  role_editor->caption("Edit Role");
  role_editor->rating(10);
  role_editor->name("db.mysql.plugin.edit.role");
  role_editor->groups().insert("catalog/Editors");
  editors.insert(role_editor);

  FRONTEND_LIBNAME(relationship_editor, ".\\db.mysql.editors.wbp.fe.dll", "db.mysql.editors.wbp.so",
                   "db.mysql.editors.mwbplugin");
  relationship_editor->pluginType("gui");
  relationship_editor->moduleFunctionName("DbMysqlRelationshipEditor");
  set_object_argument(relationship_editor, "workbench.physical.Connection");
  relationship_editor->caption("Edit Relationship");
  relationship_editor->rating(10);
  relationship_editor->name("db.mysql.plugin.edit.relationship");
  relationship_editor->groups().insert("catalog/Editors");
  editors.insert(relationship_editor);

  return editors;
}

GRT_MODULE_ENTRY_POINT(MySQLEditorsModuleImpl);
