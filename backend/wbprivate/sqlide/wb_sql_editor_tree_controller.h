/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "workbench/wb_backend_public_interface.h"
#include "sqlide/wb_live_schema_tree.h"
#include "sqlide/db_sql_editor_log.h" // for RowId
#include "grt/grt_threaded_task.h"

#include "grts/structs.db.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.db.mysql.h"

#include "grtpp_notifications.h"

class SqlEditorForm;

namespace bec {
  class GRTManager;
  class DBObjectEditorBE;
};

namespace mforms {
  class View;
  class Box;
  class Splitter;
  class TabView;
  class HyperText;
  class MenuItem;
};

namespace wb {
  class SimpleSidebar;
  class AdvancedSidebar;
}

class MYSQLWBBACKEND_PUBLIC_FUNC SqlEditorTreeController :
  public base::trackable,
  public grt::GRTObserver,
  public wb::LiveSchemaTree::FetchDelegate,
  public wb::LiveSchemaTree::Delegate,
  public std::enable_shared_from_this<SqlEditorTreeController> {
#if defined(ENABLE_TESTING)
  friend class EditorFormTester;
  friend class LocalEditorFormTester;
#endif
  friend class db_query_EditorConcreteImplData;

public:
  static std::shared_ptr<SqlEditorTreeController> create(SqlEditorForm *owner);
  virtual ~SqlEditorTreeController();

  void finish_init();
  void prepare_close();

private:
  SqlEditorTreeController(SqlEditorForm *owner);

  SqlEditorForm *_owner;

  wb::AdvancedSidebar *_schema_side_bar;
  wb::SimpleSidebar *_admin_side_bar;
  mforms::TabView *_task_tabview;
  mforms::Box *_taskbar_box;

  wb::LiveSchemaTree *_schema_tree;
  wb::LiveSchemaTree _base_schema_tree;
  wb::LiveSchemaTree _filtered_schema_tree;
  base::Mutex _schema_contents_mutex;
  GrtThreadedTask::Ref live_schema_fetch_task;
  GrtThreadedTask::Ref live_schemata_refresh_task;
  bool _is_refreshing_schema_tree;

  bool _use_show_procedure;

  mforms::Splitter *_side_splitter;
  mforms::TabView *_info_tabview;
  mforms::HyperText *_object_info;
  mforms::HyperText *_session_info;

  boost::signals2::scoped_connection _splitter_connection;

  // Observer
  virtual void handle_grt_notification(const std::string &name, grt::ObjectRef sender, grt::DictRef info);

  void updateColors();

  // LiveSchemaTree::FetchDelegate
  virtual std::vector<std::string> fetch_schema_list();
  virtual bool fetch_data_for_filter(const std::string &schema_filter, const std::string &object_filter,
                                     const wb::LiveSchemaTree::NewSchemaContentArrivedSlot &arrived_slot);
  virtual bool fetch_schema_contents(const std::string &schema_name,
                                     const wb::LiveSchemaTree::NewSchemaContentArrivedSlot &arrived_slot);
  virtual bool fetch_object_details(const std::string &schema_name, const std::string &object_name,
                                    wb::LiveSchemaTree::ObjectType type, short flags,
                                    const wb::LiveSchemaTree::NodeChildrenUpdaterSlot &);
  virtual bool fetch_routine_details(const std::string &schema_name, const std::string &obj_name,
                                     wb::LiveSchemaTree::ObjectType type);
  // LiveSchemaTree::Delegate
  virtual void tree_refresh();
  virtual bool sidebar_action(const std::string &);
  virtual void tree_activate_objects(const std::string &, const std::vector<wb::LiveSchemaTree::ChangeRecord> &changes);

public:
  virtual void tree_create_object(wb::LiveSchemaTree::ObjectType type, const std::string &schema_name,
                                  const std::string &obj_name);

  std::string generate_alter_script(const db_mgmt_RdbmsRef &rdbms, db_DatabaseObjectRef db_object,
                                    std::string algorithm, std::string lock);

private:
  grt::StringRef do_fetch_live_schema_contents(std::weak_ptr<SqlEditorTreeController> self_ptr,
                                               const std::string &schema_name,
                                               wb::LiveSchemaTree::NewSchemaContentArrivedSlot arrived_slot);
  wb::LiveSchemaTree::ObjectType fetch_object_type(const std::string &schema_name, const std::string &obj_name);
  void fetch_column_data(const std::string &schema_name, const std::string &obj_name,
                         wb::LiveSchemaTree::ObjectType type,
                         const wb::LiveSchemaTree::NodeChildrenUpdaterSlot &updater_slot);
  void fetch_trigger_data(const std::string &schema_name, const std::string &obj_name,
                          wb::LiveSchemaTree::ObjectType type,
                          const wb::LiveSchemaTree::NodeChildrenUpdaterSlot &updater_slot);
  void fetch_index_data(const std::string &schema_name, const std::string &obj_name,
                        wb::LiveSchemaTree::ObjectType type,
                        const wb::LiveSchemaTree::NodeChildrenUpdaterSlot &updater_slot);
  void fetch_foreign_key_data(const std::string &schema_name, const std::string &obj_name,
                              wb::LiveSchemaTree::ObjectType type,
                              const wb::LiveSchemaTree::NodeChildrenUpdaterSlot &updater_slot);

  grt::StringRef do_fetch_data_for_filter(std::weak_ptr<SqlEditorTreeController> self_ptr,
                                          const std::string &schema_filter, const std::string &object_filter,
                                          wb::LiveSchemaTree::NewSchemaContentArrivedSlot arrived_slot);

  void schema_row_selected();
  void side_bar_filter_changed(const std::string &filter);
  void sidebar_splitter_changed();

  void context_menu_will_show(mforms::MenuItem *parent_item);

public:
  void refresh_live_object_in_editor(bec::DBObjectEditorBE *obj_editor, bool using_old_name);
  void refresh_live_object_in_overview(wb::LiveSchemaTree::ObjectType type, const std::string schema_name,
                                       const std::string old_obj_name, const std::string new_obj_name);

  void on_active_schema_change(const std::string &schema);
  void mark_busy(bool busy);

  void open_alter_object_editor(db_DatabaseObjectRef object, db_CatalogRef server_state_catalog);

private:
  grt::StringRef do_refresh_schema_tree_safe(std::weak_ptr<SqlEditorForm> self_ptr);

  int insert_text_to_active_editor(const std::string &str);

private:
  void do_alter_live_object(wb::LiveSchemaTree::ObjectType type, const std::string &schema_name,
                            const std::string &obj_name);
  std::string run_execute_routine_wizard(wb::LiveSchemaTree::ObjectType type, const std::string &schema_name,
                                         const std::string &obj_name);

  std::string get_object_ddl_script(wb::LiveSchemaTree::ObjectType type, const std::string &schema_name,
                                    const std::string &obj_name);
  std::pair<std::string, std::string> get_object_create_script(wb::LiveSchemaTree::ObjectType type,
                                                               const std::string &schema_name,
                                                               const std::string &obj_name);
  std::vector<std::string> get_trigger_sql_for_table(const std::string &schema_name, const std::string &table_name);

  bool parse_ddl_into_catalog(db_mysql_CatalogRef catalog, const std::string &objectDescription, const std::string &sql,
                              std::string sqlMode, const std::string &schema);

public:
  void schema_object_activated(const std::string &action, wb::LiveSchemaTree::ObjectType type,
                               const std::string &schema, const std::string &name);
  bool apply_changes_to_object(bec::DBObjectEditorBE *obj_editor, bool dry_run);

  mforms::View *get_sidebar();

  wb::LiveSchemaTree *get_schema_tree();
  void request_refresh_schema_tree();

public:
  void create_live_table_stubs(bec::DBObjectEditorBE *table_editor);
  bool expand_live_table_stub(bec::DBObjectEditorBE *table_editor, const std::string &schema_name,
                              const std::string &obj_name);

public:
  bool activate_live_object(GrtObjectRef object);

private:
  db_SchemaRef create_new_schema(db_CatalogRef owner);
  db_TableRef create_new_table(db_SchemaRef owner);
  db_ViewRef create_new_view(db_SchemaRef owner);
  db_RoutineRef create_new_routine(db_SchemaRef owner, wb::LiveSchemaTree::ObjectType type);
};
