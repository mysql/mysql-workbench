/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grts/structs.workbench.h"
#include "grts/structs.db.mgmt.h"
#include "grtui/grt_wizard_plugin.h"
#include "merge_model.h"

#define MODULE_VERSION "1.0.0"

static grt::ListRef<app_Plugin> get_mysql_plugins_info();

class MySQLModelSnippetsModuleImpl : public grt::ModuleImplBase, public PluginInterfaceImpl {
public:
  MySQLModelSnippetsModuleImpl(grt::CPPModuleLoader *ldr) : grt::ModuleImplBase(ldr) {
  }

  DEFINE_INIT_MODULE(MODULE_VERSION, "Oracle and/or its affiliates", grt::ModuleImplBase,
                     DECLARE_MODULE_FUNCTION(MySQLModelSnippetsModuleImpl::getPluginInfo),
                     DECLARE_MODULE_FUNCTION(MySQLModelSnippetsModuleImpl::includeModel), NULL);

  virtual grt::ListRef<app_Plugin> getPluginInfo() override {
    return get_mysql_plugins_info();
  }

  virtual grt::IntegerRef includeModel(const std::string &path) {
    grt::Module *module = grt::GRT::get()->get_module("Workbench");
    if (!module)
      throw std::runtime_error("Workbench module not found");

    grt::BaseListRef args(true);

    args.ginsert(grt::StringRef(path));

    workbench_DocumentRef doc(workbench_DocumentRef::cast_from(module->call_function("openModelFile", args)));
    if (!doc.is_valid())
      return grt::IntegerRef(0);

    // Merge catalog
    db_CatalogRef source_catalog = doc->physicalModels()[0]->catalog();
    db_CatalogRef target_catalog = db_CatalogRef::cast_from(grt::GRT::get()->get("/wb/doc/physicalModels/0/catalog"));
    merge_catalog(this, target_catalog, source_catalog);

    // Merge diagrams
    grt::ListRef<workbench_physical_Diagram> source_diagrams = doc->physicalModels()[0]->diagrams();
    grt::ListRef<workbench_physical_Diagram> target_diagrams =
      grt::ListRef<workbench_physical_Diagram>::cast_from(grt::GRT::get()->get("/wb/doc/physicalModels/0/diagrams"));
    workbench_physical_ModelRef dst_owner =
      workbench_physical_ModelRef::cast_from(grt::GRT::get()->get("/wb/doc/physicalModels/0"));
    merge_diagrams(target_diagrams, source_diagrams, dst_owner);
    //    dst_owner->signal_changed().emit("", grt::ValueRef());
    args.clear();
    module->call_function("closeModelFile", args);
    return grt::IntegerRef(0);
  }

  virtual grt::IntegerRef includeModelObjects(const workbench_DocumentRef &document,
                                              grt::StringListRef objectNameList) {
    return grt::IntegerRef(0);
  }
};

//
// static void set_object_argument(app_PluginRef &plugin, const std::string &struct_name)
//{
//  app_PluginObjectInputRef pdef(plugin.get_grt());
//
//  pdef->objectStructName(struct_name);
//  pdef->owner(plugin);
//
//  plugin->inputValues().insert(pdef);
//}
//

static grt::ListRef<app_Plugin> get_mysql_plugins_info() {
  grt::ListRef<app_Plugin> plugins(true);
  {
    app_PluginRef plugin(grt::Initialized);

    plugin->pluginType("standalone");
    plugin->moduleName("MySQLModelSnippetsModule");
    plugin->moduleFunctionName("includeModel");
    plugin->name("wb.mysql.includeModel");
    plugin->caption("Include Objects from a Model File");
    plugin->groups().insert("model/Model");

    app_PluginFileInputRef pdef(grt::Initialized);
    pdef->owner(plugin);
    pdef->dialogTitle(_("Include Model"));
    pdef->dialogType("open");
    pdef->fileExtensions("mwb");
    plugin->inputValues().insert(pdef);

    plugins.insert(plugin);
  }

  return plugins;
}

GRT_MODULE_ENTRY_POINT(MySQLModelSnippetsModuleImpl);
