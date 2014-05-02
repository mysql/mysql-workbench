#ifndef _DB_MYSQL_SQLIDE_H_
#define _DB_MYSQL_SQLIDE_H_


#include "db_mysql_sqlide_public_interface.h"
#include "sqlide/sqlide.h"
#include "wbpublic_public_interface.h"
//#include "interfaces/plugin.h"


#define MysqlSql_VERSION "1.0"


class DB_MYSQL_SQLIDE_PUBLIC_FUNC MysqlSqlImpl
  : public Sql, /*public PluginInterfaceImpl, */public grt::ModuleImplBase
{
public:
  MysqlSqlImpl(grt::CPPModuleLoader *ldr) : grt::ModuleImplBase(ldr) {}

  DEFINE_INIT_MODULE(MysqlSql_VERSION, "MySQL AB", grt::ModuleImplBase,
                     NULL,
              //    DECLARE_MODULE_FUNCTION(MysqlSqlImpl::getPluginInfo),
                     NULL);
//  virtual grt::ListRef<app_Plugin> getPluginInfo();

  virtual Sql_editor::Ref getSqlEditor(db_mgmt_RdbmsRef rdbms, GrtVersionRef version);
};


#endif /* _DB_MYSQL_SQLIDE_H_ */
