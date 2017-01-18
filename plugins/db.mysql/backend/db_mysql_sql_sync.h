#ifndef _DB_MYSQL_SQL_SYNC_H_
#define _DB_MYSQL_SQL_SYNC_H_

#include "db_mysql_public_interface.h"
#include "db_plugin_be.h"
#include "grts/structs.db.mysql.h"
#include "../../db.mysql/backend/db_mysql_validation_page.h"

class WBPLUGINDBMYSQLBE_PUBLIC_FUNC DbMySQLSync : public Db_plugin, public DbMySQLValidationPage {
private:
  // options
  std::string _input_filename;
  std::string _output_filename;
  std::string _script_to_apply;

public:
  DbMySQLSync();
  void set_option(const std::string& name, const std::string& value);
  void start_apply_script_to_db();
};

#endif // _DB_MYSQL_SQL_SYNC_H_
