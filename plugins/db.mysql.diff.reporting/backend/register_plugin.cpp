#include "stdafx.h"

#include "grtpp.h"
#include "interfaces/plugin.h"
//#include "interfaces/rdbms_info.h"

#include "grtui/grt_wizard_plugin.h"

#include "grts/structs.db.mgmt.h"


#define MODULE_VERSION "1.0.0"



static grt::ListRef<app_Plugin> get_mysql_plugins_info(grt::GRT *grt);


class MySQLDbDiffReportingModuleImpl : public grt::ModuleImplBase, public PluginInterfaceImpl
{
public:
  MySQLDbDiffReportingModuleImpl(grt::CPPModuleLoader *ldr)
  : grt::ModuleImplBase(ldr)
  {
  }

  DEFINE_INIT_MODULE(MODULE_VERSION, "MySQL AB", grt::ModuleImplBase,
                     DECLARE_MODULE_FUNCTION(MySQLDbDiffReportingModuleImpl::getPluginInfo), 
                     DECLARE_MODULE_FUNCTION(MySQLDbDiffReportingModuleImpl::runWizard),
                     NULL);

  int runWizard()
  {
    extern grtui::WizardPlugin *createWbPluginDiffReport(grt::Module *module);
    
    grtui::WizardPlugin *wizard= createWbPluginDiffReport(this);
    int rc= wizard->run_wizard();
    delete wizard;
    
    return rc;
  }
  
  virtual grt::ListRef<app_Plugin> getPluginInfo()
  {
    return get_mysql_plugins_info(get_grt());
  }
};



static grt::ListRef<app_Plugin> get_mysql_plugins_info(grt::GRT *grt)
{
  grt::ListRef<app_Plugin> plugins(grt);
  app_PluginRef diff_sql_generator(grt);

  {
    app_PluginRef plugin(grt);

    plugin->pluginType("standalone");
    plugin->moduleName("MySQLDbDiffReportingModule");
    plugin->moduleFunctionName("runWizard");
    plugin->name("db.mysql.plugin.diff_report.catalog");
    plugin->caption("Generate Catalog Diff Report");
    plugin->groups().insert("database/Database");

    grt::StringListRef document_types(grt);
    document_types.insert("workbench.Document");
    //plugin->documentStructNames(document_types);

    app_PluginObjectInputRef pdef(grt);
    pdef->objectStructName("db.Catalog");
    plugin->inputValues().insert(pdef);

    plugins.insert(plugin);
  }

  return plugins;
}


GRT_MODULE_ENTRY_POINT(MySQLDbDiffReportingModuleImpl);
