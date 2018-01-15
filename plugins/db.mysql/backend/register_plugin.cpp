/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include "grtui/grt_wizard_plugin.h"

#define MODULE_VERSION "1.0.0"

static grt::ListRef<app_Plugin> get_mysql_plugins_info();

class MySQLDbModuleImpl : public grt::ModuleImplBase, public PluginInterfaceImpl {
public:
  MySQLDbModuleImpl(grt::CPPModuleLoader *ldr) : grt::ModuleImplBase(ldr) {
  }

  DEFINE_INIT_MODULE(MODULE_VERSION, "Oracle and/or its affiliates", grt::ModuleImplBase,
                     DECLARE_MODULE_FUNCTION(MySQLDbModuleImpl::getPluginInfo),
                     DECLARE_MODULE_FUNCTION(MySQLDbModuleImpl::runExportCREATEScriptWizard),
                     DECLARE_MODULE_FUNCTION(MySQLDbModuleImpl::runImportScriptWizard),
                     DECLARE_MODULE_FUNCTION(MySQLDbModuleImpl::runDbSynchronizeWizard),
                     DECLARE_MODULE_FUNCTION(MySQLDbModuleImpl::runDbImportWizard),
                     DECLARE_MODULE_FUNCTION(MySQLDbModuleImpl::runDbExportWizard),
                     DECLARE_MODULE_FUNCTION(MySQLDbModuleImpl::runDiffAlterWizard), NULL);

  int runExportCREATEScriptWizard(db_CatalogRef catalog) {
    extern grtui::WizardPlugin *createExportCREATEScriptWizard(grt::Module * module, db_CatalogRef catalog);
    extern void deleteExportCREATEScriptWizard(grtui::WizardPlugin * plugin);

    grtui::WizardPlugin *wizard = createExportCREATEScriptWizard(this, catalog);
    int rc = wizard->run_wizard();
    deleteExportCREATEScriptWizard(wizard);

    return rc;
  }

  int runImportScriptWizard(db_CatalogRef catalog) {
    extern grtui::WizardPlugin *createImportScriptWizard(grt::Module * module, db_CatalogRef catalog);
    extern void deleteImportScriptWizard(grtui::WizardPlugin * plugin);

    grtui::WizardPlugin *wizard = createImportScriptWizard(this, catalog);
    int rc = wizard->run_wizard();
    deleteImportScriptWizard(wizard);

    return rc;
  }

  int runDbSynchronizeWizard(db_CatalogRef catalog) {
    extern grtui::WizardPlugin *createDbSynchronizeWizard(grt::Module * module, db_CatalogRef catalog);
    extern void deleteDbSynchronizeWizard(grtui::WizardPlugin * plugin);
    grtui::WizardPlugin *wizard = createDbSynchronizeWizard(this, catalog);
    int rc = wizard->run_wizard();
    deleteDbSynchronizeWizard(wizard);

    return rc;
  }

  int runDbImportWizard(db_CatalogRef catalog) {
    extern grtui::WizardPlugin *createDbImportWizard(grt::Module * module, db_CatalogRef catalog);
    extern void deleteDbImportWizard(grtui::WizardPlugin * plugin);

    grtui::WizardPlugin *wizard = createDbImportWizard(this, catalog);
    int rc = wizard->run_wizard();
    deleteDbImportWizard(wizard);

    return rc;
  }

  int runDiffAlterWizard(db_CatalogRef catalog) {
    extern grtui::WizardPlugin *createWbSynchronizeAnyWizard(grt::Module * module, db_CatalogRef catalog);
    extern void deleteWbSynchronizeAnyWizard(grtui::WizardPlugin * plugin);

    grtui::WizardPlugin *wizard = createWbSynchronizeAnyWizard(this, catalog);
    int rc = wizard->run_wizard();
    deleteWbSynchronizeAnyWizard(wizard);

    return rc;
  }

  int runDbExportWizard(db_CatalogRef catalog) {
    extern grtui::WizardPlugin *createDbExportWizard(grt::Module * module, db_CatalogRef catalog);
    extern void deleteDbExportWizard(grtui::WizardPlugin * plugin);

    grtui::WizardPlugin *wizard = createDbExportWizard(this, catalog);
    int rc = wizard->run_wizard();
    deleteDbExportWizard(wizard);

    return rc;
  }
  
  virtual grt::ListRef<app_Plugin> getPluginInfo() override {
    return get_mysql_plugins_info();
  }
};

static grt::ListRef<app_Plugin> get_mysql_plugins_info() {
  grt::ListRef<app_Plugin> plugins(true);
  app_PluginRef diff_sql_generator(grt::Initialized);

  {
    app_PluginRef plugin(grt::Initialized);

    plugin->pluginType("standalone");
    plugin->moduleName("MySQLDbModule");
    plugin->moduleFunctionName("runExportCREATEScriptWizard");
    plugin->name("db.mysql.plugin.export.sql");
    plugin->caption("Export MySQL SQL Script");
    plugin->groups().insert("database/Database");

    grt::StringListRef document_types(grt::Initialized);
    document_types.insert("workbench.Document");
    // plugin->documentStructNames(document_types);

    app_PluginObjectInputRef pdef(grt::Initialized);
    pdef->name("activeCatalog");
    pdef->objectStructName("db.Catalog");
    plugin->inputValues().insert(pdef);

    plugins.insert(plugin);
  }

  {
    app_PluginRef plugin(grt::Initialized);

    plugin->pluginType("standalone");
    plugin->moduleName("MySQLDbModule");
    plugin->moduleFunctionName("runImportScriptWizard");
    plugin->name("db.mysql.plugin.import.sql");
    plugin->caption("Import from SQL Script");
    plugin->groups().insert("database/Database");

    grt::StringListRef document_types(grt::Initialized);
    document_types.insert("workbench.Document");
    // plugin->documentStructNames(document_types);

    app_PluginObjectInputRef pdef(grt::Initialized);
    pdef->name("activeCatalog");
    pdef->objectStructName("db.Catalog");
    plugin->inputValues().insert(pdef);

    plugins.insert(plugin);
  }

  {
    app_PluginRef plugin(grt::Initialized);

    plugin->pluginType("standalone");
    plugin->moduleName("MySQLDbModule");
    plugin->moduleFunctionName("runDbSynchronizeWizard");
    plugin->name("db.mysql.plugin.sync.db");
    plugin->caption("Synchronize with Database");
    plugin->groups().insert("database/Database");

    grt::StringListRef document_types(grt::Initialized);
    document_types.insert("workbench.Document");
    // plugin->documentStructNames(document_types);

    app_PluginObjectInputRef pdef(grt::Initialized);
    pdef->name("activeCatalog");
    pdef->objectStructName("db.Catalog");
    plugin->inputValues().insert(pdef);

    plugins.insert(plugin);
  }

  {
    app_PluginRef plugin(grt::Initialized);

    plugin->pluginType("standalone");
    plugin->moduleName("MySQLDbModule");
    plugin->moduleFunctionName("runDbImportWizard");
    plugin->name("db.plugin.database.rev_eng");
    plugin->caption("Reverse Engineer from Database");
    plugin->groups().insert("database/Database");

    grt::StringListRef document_types(grt::Initialized);
    document_types.insert("workbench.Document");
    // plugin->documentStructNames(document_types);

    app_PluginObjectInputRef pdef(grt::Initialized);
    pdef->name("activeCatalog");
    pdef->objectStructName("db.Catalog");
    plugin->inputValues().insert(pdef);

    plugins.insert(plugin);
  }

  {
    app_PluginRef plugin(grt::Initialized);

    plugin->pluginType("standalone");
    plugin->moduleName("MySQLDbModule");
    plugin->moduleFunctionName("runDbExportWizard");
    plugin->name("db.plugin.database.frw_eng");
    plugin->caption("Forward Engineer to Database");
    plugin->groups().insert("database/Database");

    grt::StringListRef document_types(grt::Initialized);
    document_types.insert("workbench.Document");
    // plugin->documentStructNames(document_types);

    app_PluginObjectInputRef pdef(grt::Initialized);
    pdef->name("activeCatalog");
    pdef->objectStructName("db.Catalog");
    plugin->inputValues().insert(pdef);

    plugins.insert(plugin);
  }

  {
    app_PluginRef plugin(grt::Initialized);

    plugin->pluginType("standalone");
    plugin->moduleName("MySQLDbModule");
    plugin->moduleFunctionName("runDiffAlterWizard");
    plugin->name("db.plugin.database.create_alter");
    plugin->caption("Create Alter script");
    plugin->groups().insert("database/Database");

    grt::StringListRef document_types(grt::Initialized);
    document_types.insert("workbench.Document");
    // plugin->documentStructNames(document_types);

    app_PluginObjectInputRef pdef(grt::Initialized);
    pdef->name("activeCatalog");
    pdef->objectStructName("db.Catalog");
    plugin->inputValues().insert(pdef);

    plugins.insert(plugin);
  }

  return plugins;
}

GRT_MODULE_ENTRY_POINT(MySQLDbModuleImpl);
