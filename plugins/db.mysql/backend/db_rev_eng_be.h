#ifndef _DB_REV_ENG_BE_H_
#define _DB_REV_ENG_BE_H_

#include "sql_import_be.h"

#include "db_mysql_public_interface.h"
#include "db_plugin_be.h"


class WBPLUGINDBMYSQLBE_PUBLIC_FUNC Db_rev_eng : public Db_plugin, public Sql_import
{
private:
  std::string task_desc();
  void parse_sql_script(SqlFacade::Ref sql_parser, db_CatalogRef &catalog, const std::string &sql_scrtipt, grt::DictRef &options);
  db_CatalogRef target_catalog();

public:
  std::string sql_script();
  void sql_script(const std::string &sql_script) { Db_plugin::sql_script(sql_script); }

public:
  Db_rev_eng() : Db_plugin(), Sql_import() {}
  void grtm(bec::GRTManager *grtm);
};


#endif /* _DB_REV_ENG_BE_H_ */
