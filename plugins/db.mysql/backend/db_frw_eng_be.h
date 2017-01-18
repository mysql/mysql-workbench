#ifndef _DB_FRW_ENG_BE_H_
#define _DB_FRW_ENG_BE_H_

#include "db_mysql_public_interface.h"
#include "db_plugin_be.h"

// if this plugin is for generic db this shouldn't be here! -alfredo
#include "grts/structs.db.mysql.h"
#include "db_mysql_sql_export.h"
#include "db_mysql_validation_page.h"

class WBPLUGINDBMYSQLBE_PUBLIC_FUNC Db_frw_eng : public Db_plugin, public DbMySQLValidationPage {
public:
  Db_frw_eng();

  void set_option(const std::string &name, bool value) {
    _export.set_option(name, value);
  }
  void set_option(const std::string &name, const std::string &value) {
    _export.set_option(name, value);
  }
  void set_up_dboptions() {
    _export.set_db_options(_db_options);
  }

  void start_export() {
    _export.start_export(false);
  }
  std::string export_sql_script() {
    return _export.export_sql_script();
  }
  void start_apply_script_to_db();

  void export_task_finish_cb(Task_finish_cb cb) {
    _export.task_finish_cb(cb);
  }

  void setup_grt_string_list_models_from_catalog(
    bec::GrtStringListModel **users_model, bec::GrtStringListModel **users_exc_model,
    bec::GrtStringListModel **tables_model, bec::GrtStringListModel **tables_exc_model,
    bec::GrtStringListModel **views_model, bec::GrtStringListModel **views_exc_model,
    bec::GrtStringListModel **routines_model, bec::GrtStringListModel **routines_exc_model,
    bec::GrtStringListModel **triggers_model, bec::GrtStringListModel **triggers_exc_model);

private:
  DbMySQLSQLExport _export;
};

#endif // _DB_FRW_ENG_BE_H_
