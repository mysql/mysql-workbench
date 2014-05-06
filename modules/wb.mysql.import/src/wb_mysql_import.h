/*
* Copyright (c) 2004, 2014, Oracle and/or its affiliates. All rights reserved.
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
