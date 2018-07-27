/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#ifndef _DB_MYSQL_SQL_EXPORT_H_
#define _DB_MYSQL_SQL_EXPORT_H_

#include "db_mysql_public_interface.h"
#include "grt/grt_manager.h"
#include "grts/structs.db.mysql.h"
#include "grt/grt_string_list_model.h"
#include "db_mysql_validation_page.h"

class WBPLUGINDBMYSQLBE_PUBLIC_FUNC DbMySQLSQLExport : public DbMySQLValidationPage {
  db_mysql_CatalogRef _catalog;

  // options
  bool _gen_drops;
  bool _gen_schema_drops;
  bool _gen_warnings;
  bool _gen_create_index;
  bool _no_users_just_privileges;
  bool _no_view_placeholders;
  bool _gen_inserts;
  bool _no_FK_for_inserts;
  bool _triggers_after_inserts;
  std::string _output_filename;
  std::string _output_header;
  bool _tables_are_selected, _triggers_are_selected, _routines_are_selected, _views_are_selected, _users_are_selected;
  bool _omitSchemas;
  bool _generate_use;
  bool _skip_foreign_keys;
  bool _skip_fk_indexes;
  bool _case_sensitive;
  bool _gen_doc_props;
  bool _gen_attached_scripts;
  bool _sortTablesAlphabetically;

  std::shared_ptr<bec::GrtStringListModel> _users_model;
  std::shared_ptr<bec::GrtStringListModel> _users_exc_model;
  std::shared_ptr<bec::GrtStringListModel> _tables_model;
  std::shared_ptr<bec::GrtStringListModel> _tables_exc_model;
  std::shared_ptr<bec::GrtStringListModel> _views_model;
  std::shared_ptr<bec::GrtStringListModel> _views_exc_model;
  std::shared_ptr<bec::GrtStringListModel> _routines_model;
  std::shared_ptr<bec::GrtStringListModel> _routines_exc_model;
  std::shared_ptr<bec::GrtStringListModel> _triggers_model;
  std::shared_ptr<bec::GrtStringListModel> _triggers_exc_model;

  std::map<std::string, GrtNamedObjectRef> _users_map;
  std::map<std::string, GrtNamedObjectRef> _tables_map;
  std::map<std::string, GrtNamedObjectRef> _views_map;
  std::map<std::string, GrtNamedObjectRef> _routines_map;
  std::map<std::string, GrtNamedObjectRef> _triggers_map;
  grt::DictRef _db_options;

  std::string get_q_name(const char *part1, const char *part2) {
    return std::string(part1).append(".").append(part2);
  }

  db_mysql_CatalogRef filter_catalog();

protected:
  virtual db_mysql_CatalogRef get_model_catalog();
  virtual grt::DictRef get_options_as_dict();
  // bec::MessageListBE messages_list;
public:
  DbMySQLSQLExport(db_mysql_CatalogRef catalog = db_mysql_CatalogRef());
  virtual ~DbMySQLSQLExport(){};

  db_mysql_CatalogRef get_catalog() const {
    return _catalog;
  }

  std::string get_output_filename() const {
    return _output_filename;
  }

  void set_option(const std::string &name, bool value);
  void set_option(const std::string &name, const std::string &value);
  void set_db_options_for_version(const GrtVersionRef &version);
  void set_db_options(grt::DictRef &db_options);

  void start_export(bool wait_finish);
  // void run_validation();

  void export_finished(grt::ValueRef res);
  grt::ValueRef export_task(grt::StringRef);

  typedef std::function<int()> Task_finish_cb;
  void task_finish_cb(Task_finish_cb cb) {
    _task_finish_cb = cb;
  }

  void setup_grt_string_list_models_from_catalog(
    bec::GrtStringListModel **users_model, bec::GrtStringListModel **users_exc_model,
    bec::GrtStringListModel **tables_model, bec::GrtStringListModel **tables_exc_model,
    bec::GrtStringListModel **views_model, bec::GrtStringListModel **views_exc_model,
    bec::GrtStringListModel **routines_model, bec::GrtStringListModel **routines_exc_model,
    bec::GrtStringListModel **triggers_model, bec::GrtStringListModel **triggers_exc_model);

  std::string export_sql_script() {
    return _export_sql_script;
  }

private:
  // Validation_finished_cb _validation_finished_cb;
  // Validation_step_finished_cb _validation_step_finished_cb;
  Task_finish_cb _task_finish_cb;
  std::string _export_sql_script;
};

grt::StringListRef convert_string_vector_to_grt_list(const std::vector<std::string> &v);

#endif // _DB_MYSQL_SQL_EXPORT_H_
