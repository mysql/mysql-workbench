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

#pragma once

#include "workbench/wb_backend_public_interface.h"

#include "base/file_utilities.h"
#include "base/ui_form.h"
#include "base/threaded_timer.h"

#include "grts/structs.workbench.h"
#include "grts/structs.db.mgmt.h"
#include "grtpp_notifications.h"

#include "sqlide/recordset_be.h"
#include "sqlide/sql_editor_be.h"
#include "sqlide/db_sql_editor_log.h"
#include "sqlide/db_sql_editor_history_be.h"
#include "sqlide/wb_context_sqlide.h"
#include "sqlide/wb_live_schema_tree.h"

#include "cppdbc.h"

#include "mforms/view.h"

#include "SymbolTable.h"

namespace mforms {
  class ToolBar;
  class AppView;
  class View;
  class MenuItem;
  class DockingPoint;
};

namespace bec {
  class DBObjectEditorBE;
}

#define MAIN_DOCKING_POINT "db.query.Editor:main"
#define RESULT_DOCKING_POINT "db.Query.QueryEditor:result"

class QuerySidePalette;
class SqlEditorTreeController;
class ColumnWidthCache;
class SqlEditorPanel;
class SqlEditorResult;

namespace wb {
  class SSHTunnel;
}

typedef std::vector<Recordset::Ref> Recordsets;
typedef std::shared_ptr<Recordsets> RecordsetsRef;

db_mgmt_ServerInstanceRef getServerInstance(const db_mgmt_ConnectionRef &connection);

class MYSQLWBBACKEND_PUBLIC_FUNC SqlEditorForm :
  public bec::UIForm,
  grt::GRTObserver,
  public std::enable_shared_from_this<SqlEditorForm>,
  mforms::DropDelegate {
public:
#if defined(ENABLE_TESTING)
  friend class EditorFormTester;
  friend class LocalEditorFormTester;
#endif

  enum ServerState { UnknownState, RunningState, PossiblyStoppedState, OfflineState };

  struct PSStage {
    std::string name;
    double wait_time;
  };

  struct PSWait {
    std::string name;
    double wait_time;
  };

  class RecordsetData : public Recordset::ClientData {
  public:
    SqlEditorResult *result_panel;
    std::string generator_query;

    double duration;
    std::string ps_stat_error;
    std::map<std::string, std::int64_t> ps_stat_info;
    std::vector<PSStage> ps_stage_info;
    std::vector<PSWait> ps_wait_info;
  };

public:
  typedef std::shared_ptr<SqlEditorForm> Ref;
  typedef std::weak_ptr<SqlEditorForm> Ptr;
  static SqlEditorForm::Ref create(wb::WBContextSQLIDE *wbsql, const db_mgmt_ConnectionRef &conn);
  static void report_connection_failure(const std::string &error, const db_mgmt_ConnectionRef &target);
  static void report_connection_failure(const grt::server_denied &info, const db_mgmt_ConnectionRef &target);

  void set_tab_dock(mforms::DockingPoint *dp);

  /* Callback must be set by frontend to show a busy indicator on the tab with the given index. -1 means remove it from
   * all */
  std::function<void(int)> set_busy_tab;

  parsers::SymbolTable *databaseSymbols() { return &_databaseSymbols; }

protected:
  SqlEditorForm(wb::WBContextSQLIDE *wbsql);

  void update_menu_and_toolbar();
  void update_toolbar_icons();

  void save_workspace_order(const std::string &prefix);
  std::string find_workspace_state(const std::string &workspace_name, std::unique_ptr<base::LockFile> &lock_file);

public:
  virtual ~SqlEditorForm();

  void cancel_connect();
  virtual void close();
  virtual bool is_main_form() {
    return true;
  }
  virtual std::string get_form_context_name() const;

  virtual mforms::MenuBar *get_menubar();
  virtual mforms::ToolBar *get_toolbar();
  std::string get_session_name();

  void auto_save();
  void save_workspace(const std::string &workspace_name, bool is_autosave);
  bool load_workspace(const std::string &workspace_name);

  void restore_last_workspace();

public:
  wb::WBContextSQLIDE *wbsql() const {
    return _wbsql;
  }

  db_query_EditorRef grtobj();

  void validate_menubar();

  void handle_tab_menu_action(const std::string &action, int tab_index);
  void handle_history_action(const std::string &action, const std::string &sql);

public:
  // do NOT use rdbms->version().. it's not specific for this connection
  db_mgmt_RdbmsRef rdbms();
  GrtVersionRef rdbms_version() const;

  std::string get_connection_info() const {
    return _connectionInfo;
  }

public:
  SqlEditorPanel *active_sql_editor_panel();

  void sql_editor_reordered(SqlEditorPanel *editor, int new_index);

  bool is_closing() const {
    return _closing;
  }

private:
  int _sql_editors_serial = 0;
  int _scratch_editors_serial = 0;
  std::shared_ptr<wb::SSHTunnel> _tunnel;
  db_mgmt_SSHConnectionRef _sshConnection;

  void sql_editor_panel_switched();
  void sql_editor_panel_closed(mforms::AppView *view);

  void set_editor_tool_items_enbled(const std::string &name, bool flag);
  void set_editor_tool_items_checked(const std::string &name, bool flag);

public:
  void set_tool_item_checked(const std::string &name, bool flag);

  boost::signals2::signal<void(MySQLEditor::Ref, bool)> sql_editor_list_changed;

  SqlEditorPanel *run_sql_in_scratch_tab(const std::string &sql, bool reuse_if_possible, bool start_collapsed);
  SqlEditorPanel *add_sql_editor(bool scratch = false,
                                 bool start_collapsed = false); // returns index of the added sql_editor
  void remove_sql_editor(SqlEditorPanel *panel);
  SqlEditorPanel *sql_editor_panel(int index);
  int sql_editor_count();
  int sql_editor_panel_index(SqlEditorPanel *panel);

  virtual mforms::DragOperation drag_over(mforms::View *sender, base::Point p, mforms::DragOperation allowedOperations,
                                          const std::vector<std::string> &formats);
  virtual mforms::DragOperation files_dropped(mforms::View *sender, base::Point p,
                                              mforms::DragOperation allowedOperations,
                                              const std::vector<std::string> &file_names);

private:
  int count_connection_editors(const std::string &conn_name);

protected:
  std::string create_title();
  void title_changed();
  void check_server_problems();

public:
  virtual std::string get_title() {
    return _title;
  }
  void update_title();

  int getTunnelPort() const;

  std::map<std::string, std::string> &connection_details() {
    return _connection_details;
  }
  int server_version();
  std::set<std::string> valid_charsets();

private:
  grt::StringRef do_connect(std::shared_ptr<wb::SSHTunnel> tunnel, sql::Authentication::Ref &auth,
                            struct ConnectionErrorInfo *autherr_ptr);
  std::string get_client_lib_version();
  grt::StringRef do_disconnect();

  void update_connected_state();

public:
  bool connect(std::shared_ptr<wb::SSHTunnel> tunnel);
  bool connected() const;
  bool connectionIsValid() const {
    return _connection.is_valid();
  }
  void checkIfOffline();
  bool offline();
  bool ping() const;
  void finish_startup();
  void cancel_query();
  void reset();
  void commit();
  void rollback();
  bool auto_commit();
  void auto_commit(bool value);
  void toggle_autocommit();
  void toggle_collect_field_info();
  bool collect_field_info() const;
  void toggle_collect_ps_statement_events();
  bool collect_ps_statement_events() const;

  void set_connection(db_mgmt_ConnectionRef conn);

  void run_editor_contents(bool current_statement_only);

  void limit_rows(const std::string &limit_text);

  std::string sql_mode() const {
    return _sql_mode;
  };
  int lower_case_table_names() const {
    return _lower_case_table_names;
  }

private:
  void do_commit();

public:
  db_mgmt_ConnectionRef connection_descriptor() const {
    return _connection;
  }

  db_mgmt_SSHConnectionRef getSSHConnection();

  bool get_session_variable(sql::Connection *dbc_conn, const std::string &name, std::string &value);

private:
  void cache_sql_mode();
  void update_sql_mode_for_editors();

  void query_ps_statistics(std::int64_t conn_id, std::map<std::string, std::int64_t> &stats);

  std::vector<SqlEditorForm::PSStage> query_ps_stages(std::int64_t stmt_event_id);
  std::vector<SqlEditorForm::PSWait> query_ps_waits(std::int64_t stmt_event_id);

private:
  void create_connection(sql::Dbc_connection_handler::Ref &dbc_conn, db_mgmt_ConnectionRef db_mgmt_conn,
                         std::shared_ptr<wb::SSHTunnel> tunnel, sql::Authentication::Ref auth,
                         bool autocommit_mode, bool user_connection);
  void init_connection(sql::Connection *dbc_conn_ref, const db_mgmt_ConnectionRef &connectionProperties,
                       sql::Dbc_connection_handler::Ref &dbc_conn, bool user_connection);
  void close_connection(sql::Dbc_connection_handler::Ref &dbc_conn);
  base::RecMutexLock ensure_valid_dbc_connection(sql::Dbc_connection_handler::Ref &dbc_conn,
                                                 base::RecMutex &dbc_conn_mutex, bool throw_on_block = false,
                                                 bool lockOnly = false);
  base::RecMutexLock ensure_valid_usr_connection(bool throw_on_block = false, bool lockOnly = false);
  base::RecMutexLock ensure_valid_aux_connection(bool throw_on_block = false, bool lockOnly = false);

  std::vector<std::pair<std::string, std::string>> runQueryForCache(const std::string &query);

public:
  base::RecMutexLock ensure_valid_aux_connection(sql::Dbc_connection_handler::Ref &conn, bool lockOnly = false);
  parsers::MySQLParserContext::Ref work_parser_context() {
    return _work_parser_context;
  };

private:
  void send_message_keep_alive();
  bool send_message_keep_alive_bool_wrapper() {
    send_message_keep_alive();
    return false;
  } // need it for ThreadedTimer, which expects callbacks to return bool
  void reset_keep_alive_thread();

  base::RecMutexLock getAuxConnection(sql::Dbc_connection_handler::Ref &conn, bool lockOnly = false);
  base::RecMutexLock getUserConnection(sql::Dbc_connection_handler::Ref &conn, bool lockOnly = false);

  void onCacheAction(bool active);

public:
  ColumnWidthCache *column_width_cache() {
    return _column_width_cache;
  }

  bool exec_editor_sql(SqlEditorPanel *editor, bool sync, bool current_statement_only = false,
                       bool wrap_with_non_std_delimiter = false, bool dont_add_limit_clause = false,
                       SqlEditorResult *into_result = NULL);
  void exec_sql_retaining_editor_contents(const std::string &sql_script, SqlEditorPanel *editor, bool sync,
                                          bool dont_add_limit_clause = false);

  RecordsetsRef exec_sql_returning_results(const std::string &sql_script, bool dont_add_limit_clause);

  void exec_management_sql(const std::string &sql, bool log);
  db_query_ResultsetRef exec_management_query(const std::string &sql, bool log);

  void exec_main_sql(const std::string &sql, bool log);
  db_query_ResultsetRef exec_main_query(const std::string &sql, bool log);

  void explain_current_statement();
  bool is_running_query();

  sql::Authentication::Ref dbc_auth_data() {
    return _dbc_auth;
  }

private:
  enum ExecFlags { NeedNonStdDelimiter = 1 << 1, DontAddLimitClause = 1 << 2, ShowWarnings = 1 << 3 };
  void update_live_schema_tree(const std::string &sql);

  grt::StringRef do_exec_sql(Ptr self_ptr, std::shared_ptr<std::string> sql, SqlEditorPanel *editor, ExecFlags flags,
                             RecordsetsRef result_list);

  void handle_command_side_effects(const std::string &sql);

public:
  GrtThreadedTask::Ref exec_sql_task;

  std::function<void()> post_query_slot; // called after a query is executed
private:
  int on_exec_sql_finished();

public:
  bool continue_on_error() {
    return _continueOnError;
  }
  void continue_on_error(bool val);

private:
  typedef boost::signals2::signal<int(long long, const std::string &, const std::string &),
                                  boost::signals2::last_value<int>>
    Error_cb;
  typedef boost::signals2::signal<int(float), boost::signals2::last_value<int>> Batch_exec_progress_cb;
  typedef boost::signals2::signal<int(long, long), boost::signals2::last_value<int>> Batch_exec_stat_cb;

public:
  Error_cb on_sql_script_run_error;

private:
  int sql_script_apply_error(long long, const std::string &, const std::string &, std::string &);
  int sql_script_apply_progress(float);
  int sql_script_stats(long, long);

  void abort_apply_object_alter_script();

public:
  void apply_object_alter_script(const std::string &alter_script, bec::DBObjectEditorBE *obj_editor, RowId log_id);
  bool run_live_object_alteration_wizard(const std::string &alter_script, bec::DBObjectEditorBE *obj_editor,
                                         RowId log_id, const std::string &log_context);

private:
  void apply_changes_to_recordset(Recordset::Ptr rs_ptr);
  bool run_data_changes_commit_wizard(Recordset::Ptr rs_ptr, bool skip_commit);
  void apply_data_changes_commit(const std::string &sql_script_text, Recordset::Ptr rs_ptr, bool skip_commit);
  void update_editor_title_schema(const std::string &schema);

public:
  bool can_close();
  bool can_close_(bool interactive);

  void check_external_file_changes();

public:
  SqlEditorPanel *new_sql_script_file();
  SqlEditorPanel *new_sql_scratch_area(bool start_collapsed = false);
  void new_scratch_area() {
    new_sql_scratch_area(false);
  }
  void open_file(const std::string &path, bool in_new_tab, bool askForFile = true);
  void open_file(const std::string &path = "") {
    open_file(path, true, !path.empty());
  }

public:
  void active_schema(const std::string &value);
  std::string active_schema() const;

  void schemaListRefreshed(std::vector<std::string> const &schemas);

  void schema_meta_data_refreshed(const std::string &schema_name, base::StringListPtr tables, base::StringListPtr views,
                                  base::StringListPtr procedures, base::StringListPtr functions);

private:
  void cache_active_schema_name();

public:
  void request_refresh_schema_tree();

public:
  std::string fetch_data_from_stored_procedure(std::string proc_call, std::shared_ptr<sql::ResultSet> &rs);

  DbSqlEditorLog::Ref log() {
    return _log;
  }
  DbSqlEditorHistory::Ref history() {
    return _history;
  }
  std::string restore_sql_from_history(int entry_index, std::list<int> &detail_indexes);
  int exec_sql_error_count() {
    return _exec_sql_error_count;
  }

  std::shared_ptr<SqlEditorTreeController> get_live_tree() {
    return _live_tree;
  }
  void schema_tree_did_populate();

  std::function<void(const std::string &, bool)> output_text_slot;

public:
  // Result should be RowId but that requires to change the task callback type (at least for 64bit builds).
  int add_log_message(int msg_type, const std::string &msg, const std::string &context, const std::string &duration);
  void set_log_message(RowId log_message_index, int msg_type, const std::string &msg, const std::string &context,
                       const std::string &duration);
  void refresh_log_messages(bool ignore_last_message_timestamp);

protected:
  DbSqlEditorLog::Ref _log;
  DbSqlEditorHistory::Ref _history;
  bool _serverIsOffline = false;

  std::string _title;

private:
  virtual void handle_grt_notification(const std::string &name, grt::ObjectRef sender, grt::DictRef info);
  virtual void handle_notification(const std::string &name, void *sender, base::NotificationInfo &info);
  void setup_side_palette();

  void schema_row_selected();
  void side_bar_filter_changed(const std::string &filter);

  void note_connection_open_outcome(int error);

public:
  void inspect_object(const std::string &name, const std::string &object, const std::string &type);

  void toolbar_command(const std::string &command);

  bool save_snippet();

  void show_output_area();

  mforms::View *get_sidebar();
  mforms::View *get_side_palette();

  void set_autosave_disabled(const bool autosave_disabled);
  bool get_autosave_disabled(void);

private:
  wb::WBContextSQLIDE *_wbsql;
  GrtVersionRef _version;
  mforms::MenuBar *_menu = nullptr;
  mforms::ToolBar *_toolbar = nullptr;
  std::string _connectionInfo;
  base::LockFile *_autosave_lock = nullptr;
  std::string _autosave_path;

  mforms::DockingPoint *_tabdock = nullptr;

  // Set when we triggered a refresh asynchronously.
  boost::signals2::connection _overviewRefreshPending;
  boost::signals2::connection _editorRefreshPending;

  int _keep_alive_task_id = 0;
  base::Mutex _keep_alive_thread_mutex;

  Batch_exec_progress_cb on_sql_script_run_progress;
  Batch_exec_stat_cb on_sql_script_run_statistics;

  bool _autosave_disabled = false;
  bool _loading_workspace = false;
  bool _cancel_connect = false;
  bool _closing = false;
  bool _startup_done = false;
  bool _is_running_query = false;
  bool _continueOnError = false;
  bool _has_pending_log_messages = false;

  double _last_log_message_timestamp;
  int _exec_sql_error_count;

  std::shared_ptr<SqlEditorTreeController> _live_tree;

  mforms::View *_side_palette_host = nullptr;
  QuerySidePalette *_side_palette = nullptr;
  std::string _pending_expand_nodes;

  std::map<std::string, std::string> _connection_details;
  std::set<std::string> _charsets;

  std::string _sql_mode;
  int _lower_case_table_names;
  parsers::MySQLParserContext::Ref _work_parser_context; // Never use in a background thread.

  db_mgmt_ConnectionRef _connection;
  // connection for maintenance operations, fetching schema contents & live editors (DDL only)
  sql::Dbc_connection_handler::Ref _aux_dbc_conn;
  base::RecMutex _aux_dbc_conn_mutex;

  // connection for running sql scripts
  sql::Dbc_connection_handler::Ref _usr_dbc_conn;
  mutable base::RecMutex _usr_dbc_conn_mutex;

  sql::Authentication::Ref _dbc_auth;

  ServerState _last_server_running_state = UnknownState;

  ColumnWidthCache *_column_width_cache = nullptr;

  parsers::SymbolTable _staticServerSymbols; // Charsets, collations, engines.
  parsers::SymbolTable _databaseSymbols; // All available db objects reachable via the current connection.

  void activate_command(const std::string &command);
  void readStaticServerSymbols();

  // workaround for managed code windows
  struct PrivateMutex;
  std::unique_ptr<PrivateMutex> _pimplMutex;
};
