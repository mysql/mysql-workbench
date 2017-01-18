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

#include "grt.h"
#include "interfaces/plugin.h"

#include "grtui/grt_wizard_plugin.h"

#include "grts/structs.db.mgmt.h"

#define MODULE_VERSION "1.0.0"

static grt::ListRef<app_Plugin> get_mysql_plugins_info();

class MySQLDbDiffReportingModuleImpl : public grt::ModuleImplBase, public PluginInterfaceImpl {
public:
  MySQLDbDiffReportingModuleImpl(grt::CPPModuleLoader *ldr) : grt::ModuleImplBase(ldr) {
  }

  DEFINE_INIT_MODULE(MODULE_VERSION, "Oracle and/or its affiliates", grt::ModuleImplBase,
                     DECLARE_MODULE_FUNCTION(MySQLDbDiffReportingModuleImpl::getPluginInfo),
                     DECLARE_MODULE_FUNCTION(MySQLDbDiffReportingModuleImpl::runWizard), NULL);

  int runWizard() {
    extern grtui::WizardPlugin *createWbPluginDiffReport(grt::Module * module);

    grtui::WizardPlugin *wizard = createWbPluginDiffReport(this);
    int rc = wizard->run_wizard();
    delete wizard;

    return rc;
  }

  virtual grt::ListRef<app_Plugin> getPluginInfo() {
    return get_mysql_plugins_info();
  }
};

static grt::ListRef<app_Plugin> get_mysql_plugins_info() {
  grt::ListRef<app_Plugin> plugins(true);
  app_PluginRef diff_sql_generator(grt::Initialized);

  {
    app_PluginRef plugin(grt::Initialized);

    plugin->pluginType("standalone");
    plugin->moduleName("MySQLDbDiffReportingModule");
    plugin->moduleFunctionName("runWizard");
    plugin->name("db.mysql.plugin.diff_report.catalog");
    plugin->caption("Generate Catalog Diff Report");
    plugin->groups().insert("database/Database");

    grt::StringListRef document_types(grt::Initialized);
    document_types.insert("workbench.Document");
    // plugin->documentStructNames(document_types);

    app_PluginObjectInputRef pdef(grt::Initialized);
    pdef->objectStructName("db.Catalog");
    plugin->inputValues().insert(pdef);

    plugins.insert(plugin);
  }

  return plugins;
}

GRT_MODULE_ENTRY_POINT(MySQLDbDiffReportingModuleImpl);
