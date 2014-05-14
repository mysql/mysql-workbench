/*
* Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "grtpp_module_cpp.h"

#include "grts/structs.db.mgmt.h"


#define DbUtils_VERSION "1.0.0"

class DbUtilsImpl : public grt::ModuleImplBase
{
public:
  DbUtilsImpl(grt::CPPModuleLoader *loader) : grt::ModuleImplBase(loader) {}

  DEFINE_INIT_MODULE(DbUtils_VERSION, "MySQL AB", grt::ModuleImplBase,
                  DECLARE_MODULE_FUNCTION(DbUtilsImpl::loadRdbmsInfo), NULL);

  int mergeCatalogs(db_CatalogRef sourceCatalog, db_CatalogRef targetCatalog)
  {
    return 0;
  }

  int removeIgnoredObjectsFromCatalog(db_CatalogRef catalog,
                                      grt::StringListRef ignoreList)
  {
    return 0;
  }

  int removeEmptySchemataFromCatalog(db_CatalogRef catalog)
  {
    return 0;
  }

  std::string getRoutineName(const std::string &sql)
  {
    return "";
  }

  std::string getRoutineType(const std::string &sql)
  {
    return "";
  }

  std::string getTriggerStatement(const std::string &sql)
  {
    return "";
  }

  std::string getTriggerEvent(const std::string &sql)
  {
    return "";
  }

  std::string getTriggerTiming(const std::string &sql)
  {
    return "";
  }

  int copyColumnType(db_ColumnRef sourceColumn, db_ColumnRef destColumn)
  {
    return 0;
  }

  db_mgmt_RdbmsRef loadRdbmsInfo(db_mgmt_ManagementRef owner, const std::string &path)
  {
    db_mgmt_RdbmsRef rdbms= db_mgmt_RdbmsRef::cast_from(get_grt()->unserialize(path));

    rdbms->owner(owner);

    return rdbms;
  }
};



GRT_MODULE_ENTRY_POINT(DbUtilsImpl);
