#include "stdafx.h"

#include "wb_mysql_import.h"
#include "wb_mysql_import_dbd4.h"
#include "grtsqlparser/sql_facade.h"


using namespace bec;


GRT_MODULE_ENTRY_POINT(WbMysqlImportImpl);


grt::ListRef<app_Plugin> WbMysqlImportImpl::getPluginInfo()
{
  grt::ListRef<app_Plugin> list(get_grt());

  app_PluginRef plugin(get_grt());

  plugin->name("db.mysql.import.dbd4");
  plugin->caption("Import DBDesigner4 Model");
  plugin->description("Import a database model created by DBDesigner4");
  plugin->moduleName("WbMysqlImport");
  plugin->moduleFunctionName("importDBD4");
  plugin->pluginType("standalone");
  plugin->showProgress(1);

  app_PluginObjectInputRef obj_arg(get_grt());
  obj_arg->name("activeModel");
  obj_arg->objectStructName(workbench_physical_Model::static_class_name());
  plugin->inputValues().insert(obj_arg);

  app_PluginFileInputRef file_arg(get_grt());
  file_arg->name("filename");
  file_arg->dialogTitle(("Import DBDesigner4 Model"));
  file_arg->dialogType("open");
  file_arg->fileExtensions("DBDesigner4 Model (*.xml)|*.xml");
  plugin->inputValues().insert(file_arg);

  list.insert(plugin);

  /*delme this seems to be duplicated in plugins/db
  plugin= app_PluginRef(get_grt());
  plugin->name("db.mysql.import.sql");
  plugin->caption("Import MySQL Create Script");
  plugin->description("Import a MySQL Script File");
  plugin->moduleName("WbMysqlImport");
  plugin->moduleFunctionName("parseSqlScriptFile");
  plugin->pluginType("normal");
  plugin->showProgress(1);

  obj_arg= app_PluginObjectInputRef(get_grt());
  obj_arg->name("catalog");
  obj_arg->objectStructName(db_Catalog::static_class_name());
  plugin->inputValues().insert(obj_arg);

  file_arg= app_PluginFileInputRef(get_grt());
  file_arg->name("filename");
  file_arg->dialogTitle(("Import MySQL SQL Script"));
  file_arg->dialogType("open");
  file_arg->fileExtensions("SQL Script File (*.sql)|*.sql,SQL Script File (*.txt)|*.txt");
  plugin->inputValues().insert(file_arg);

  list.insert(plugin);
  */
  return list;
}


int WbMysqlImportImpl::importDBD4(workbench_physical_ModelRef model, const std::string file_name)
{
  return Wb_mysql_import_DBD4().import_DBD4(model, file_name.c_str(), DictRef());
}


int WbMysqlImportImpl::importDBD4Ex(workbench_physical_ModelRef model, const std::string file_name, const grt::DictRef options)
{
  return Wb_mysql_import_DBD4().import_DBD4(model, file_name.c_str(), options);
}


int WbMysqlImportImpl::parseSqlScriptFile(db_CatalogRef catalog, const std::string sql_script_filename)
{
  return parseSqlScriptFileEx(catalog, sql_script_filename, DictRef());
}


int WbMysqlImportImpl::parseSqlScriptFileEx(db_CatalogRef catalog, const std::string sql_script_filename, const grt::DictRef options)
{
  SqlFacade::Ref sqlFacade= SqlFacade::instance_for_rdbms(
    db_mgmt_RdbmsRef::cast_from(catalog->owner().get_member("rdbms")));
  return sqlFacade->parseSqlScriptFileEx(
    db_mysql_CatalogRef::cast_from(catalog), sql_script_filename.c_str(), options);
}
