
#include "stdafx.h"

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
