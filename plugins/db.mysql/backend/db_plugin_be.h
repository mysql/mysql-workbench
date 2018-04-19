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

#ifndef _DB_PLUGIN_BE_H_
#define _DB_PLUGIN_BE_H_

#include "db_mysql_public_interface.h"
#include "wb_plugin_be.h"
#include "grtui/db_conn_be.h"
#include "grt/grt_manager.h"
#include "grt/grt_string_list_model.h"
#include "grts/structs.workbench.h"
#include "grtdb/diff_dbobjectmatch.h"

class Db_plugin;
#ifdef _MSC_VER
#pragma make_public(Db_plugin)
#endif

class WBPLUGINDBMYSQLBE_PUBLIC_FUNC Db_plugin : virtual public Wb_plugin {
public:
  Db_plugin() : _db_conn(0) {
  }
  virtual ~Db_plugin() {
    delete _db_conn;
  }
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
  void grtm(bool reveng);
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

  grt::StringRef apply_script_to_db();

private:
  db_mgmt_RdbmsRef selected_rdbms();
  std::string task_desc();

protected:
  void set_task_proc();

public:
  enum Db_object_type { dbotSchema, dbotTable, dbotView, dbotRoutine, dbotTrigger, dbotUser };

protected:
  struct Db_obj_handle {
    std::string schema;
    std::string name;
    std::string ddl;
  };
  typedef std::vector<Db_obj_handle> Db_objects;

  struct Db_objects_setup {
    Db_objects all;
    bec::GrtStringListModel selection;
    bec::GrtStringListModel exclusion;
    bool activated; // consider this type of db objects during operations
    Db_objects_setup() {
      activated = true;
    }
    void reset() {
      all.clear();
      selection.reset();
      exclusion.reset();
      selection.items_val_masks(&exclusion);
    }
    void icon_id(bec::IconId icon_id) {
      selection.icon_id(icon_id);
      exclusion.icon_id(icon_id);
    }
  };

  workbench_DocumentRef _doc;
  DbConnection *_db_conn;
  db_CatalogRef _catalog;

  std::vector<std::string> _schemata;
  std::map<std::string, std::string> _schemata_ddl;

  std::map<std::string, std::string> _view_db_code;

  std::vector<std::string> _schemata_selection;

protected:
  Db_objects_setup _tables;
  Db_objects_setup _views;
  Db_objects_setup _routines;
  Db_objects_setup _triggers;
  Db_objects_setup _users;

  Db_objects_setup *db_objects_setup_by_type(Db_object_type db_object_type);
  const char *db_objects_type_to_string(Db_object_type db_object_type);

  void dump_ddl(Db_object_type db_object_type, std::string &sql_script);

  int process_sql_script_error(long long err_no, const std::string &err_msg, const std::string &statement);
  int process_sql_script_progress(float progress_state);
  int process_sql_script_statistics(long success_count, long err_count);

  std::string _sql_script;
  grt::DictRef _db_options;

public:
  bec::IconId schema_icon_id(bec::IconSize icon_size) {
    return bec::IconManager::get_instance()->get_icon_id(grt::GRT::get()->get_metaclass("db.Schema"), icon_size);
  }
  bec::IconId table_icon_id(bec::IconSize icon_size) {
    return bec::IconManager::get_instance()->get_icon_id(grt::GRT::get()->get_metaclass("db.Table"), icon_size);
  }
  bec::IconId view_icon_id(bec::IconSize icon_size) {
    return bec::IconManager::get_instance()->get_icon_id(grt::GRT::get()->get_metaclass("db.View"), icon_size);
  }
  bec::IconId routine_icon_id(bec::IconSize icon_size) {
    return bec::IconManager::get_instance()->get_icon_id(grt::GRT::get()->get_metaclass("db.Routine"), icon_size);
  }
  bec::IconId trigger_icon_id(bec::IconSize icon_size) {
    return bec::IconManager::get_instance()->get_icon_id(grt::GRT::get()->get_metaclass("db.Trigger"), icon_size);
  }
  bec::IconId user_icon_id(bec::IconSize icon_size) {
    return bec::IconManager::get_instance()->get_icon_id(grt::GRT::get()->get_metaclass("db.User"), icon_size);
  }

  DbConnection *db_conn() {
    return _db_conn;
  }

  db_CatalogRef db_catalog();
  db_CatalogRef model_catalog();

  std::string db_objects_struct_name_by_type(Db_object_type db_object_type);

  /**
   * Check if on the server we're connecting we can expirence some case sensitivity problems,
   * return -1 if it's impossible to check
   * return 0 if everything is ok
   * return 1 if there can be some problems
   */
  int check_case_sensitivity_problems();

  void load_schemata(std::vector<std::string> &schemata);
  grt::DictRef load_db_options() {
    return _db_options.is_valid() ? _db_options : grt::DictRef(grt::Initialized);
  };
  //  void default_schemata_selection(std::vector<std::string> &selection);
  void schemata_selection(const std::vector<std::string> &selection, bool sel_none_means_sel_all);
  void load_db_objects(Db_object_type db_object_type);
  void db_objects_activated(Db_object_type db_object_type, bool activated) {
    db_objects_setup_by_type(db_object_type)->activated = activated;
  }
  bool db_objects_activated(Db_object_type db_object_type) {
    return db_objects_setup_by_type(db_object_type)->activated;
  }
  bec::GrtStringListModel *db_objects_selection_model(Db_object_type db_object_type) {
    return &db_objects_setup_by_type(db_object_type)->selection;
  }
  bec::GrtStringListModel *db_objects_exclusion_model(Db_object_type db_object_type) {
    return &db_objects_setup_by_type(db_object_type)->exclusion;
  }
  bool *db_objects_enabled_flag(Db_object_type db_object_type) {
    return &db_objects_setup_by_type(db_object_type)->activated;
  }
  bool validate_db_objects_selection(std::list<std::string> *messages);

  void dump_ddl(std::string &sql_script);
  void read_back_view_ddl();

  std::string sql_script() const {
    return _sql_script;
  }
  void sql_script(const std::string &sql_script) {
    _sql_script = sql_script;
  }

  std::vector<std::string> get_schemata() {
    return _schemata;
  }
};

#endif /* _DB_PLUGIN_BE_H_ */
