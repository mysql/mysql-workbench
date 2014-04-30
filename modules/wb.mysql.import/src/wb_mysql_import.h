#ifndef _WB_MYSQL_IMPORT_H_
#define _WB_MYSQL_IMPORT_H_


#include "wb_mysql_import_public_interface.h"
#include "grtpp_module_cpp.h"
#include "grtdb/db_object_helpers.h"
#include "grts/structs.db.mysql.h"
#include "grts/structs.workbench.physical.h"
#include "interfaces/plugin.h"


#define WbMysqlImport_VERSION "1.0"


class WB_MYSQL_IMPORT_WBM_PUBLIC_FUNC WbMysqlImportImpl
:
public PluginInterfaceImpl,
public grt::ModuleImplBase
{
public:
  WbMysqlImportImpl(grt::CPPModuleLoader *ldr) : grt::ModuleImplBase(ldr) {}

  DEFINE_INIT_MODULE(WbMysqlImport_VERSION, "MySQL AB", grt::ModuleImplBase,
    DECLARE_MODULE_FUNCTION(WbMysqlImportImpl::getPluginInfo),
    DECLARE_MODULE_FUNCTION(WbMysqlImportImpl::importDBD4),
    DECLARE_MODULE_FUNCTION(WbMysqlImportImpl::importDBD4Ex),
    DECLARE_MODULE_FUNCTION(WbMysqlImportImpl::parseSqlScriptFile),
    DECLARE_MODULE_FUNCTION(WbMysqlImportImpl::parseSqlScriptFileEx));
  virtual grt::ListRef<app_Plugin> getPluginInfo();

  int importDBD4(workbench_physical_ModelRef model, const std::string file_name);
  int importDBD4Ex(workbench_physical_ModelRef model, const std::string file_name, const grt::DictRef options);
  int parseSqlScriptFile(db_CatalogRef catalog, const std::string sql_script_filename);
  int parseSqlScriptFileEx(db_CatalogRef catalog, const std::string sql_script_filename, const grt::DictRef options);
};


#endif // _WB_MYSQL_IMPORT_H_
