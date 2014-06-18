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

#ifndef _WB_SQL_EDITOR_FORM_H_
#define _WB_SQL_EDITOR_FORM_H_

#include "workbench/wb_backend_public_interface.h"

#include "base/file_utilities.h"
#include "base/ui_form.h"

#include "grts/structs.workbench.h"
#include "grts/structs.db.mgmt.h"
#include "grtpp_notifications.h"
#include "grt/refresh_ui.h"

#include "sqlide/recordset_be.h"
#include "sqlide/sql_editor_be.h"
#include "sqlide/db_sql_editor_log.h"
#include "sqlide/db_sql_editor_history_be.h"
#include "sqlide/wb_context_sqlide.h"

#include "cppdbc.h"

#include <boost/enable_shared_from_this.hpp>

#include "mforms/view.h"

namespace mforms {
  class ToolBar;
  class View;
  class MenuItem;
};

namespace bec
{
  class DBObjectEditorBE;
}


#define MAIN_DOCKING_POINT "db.query.Editor:main"
#define RESULT_DOCKING_POINT "db.Query.QueryEditor:result"

class QuerySidePalette;
class SqlEditorTreeController;
class AutoCompleteCache;
class SqlEditorResult;

typedef std::vector<Recordset::Ref> Recordsets;
typedef boost::shared_ptr<Recordsets> RecordsetsRef;

class MYSQLWBBACKEND_PUBLIC_FUNC SqlEditorForm : public bec::UIForm, public bec::RefreshUI, grt::GRTObserver,
                                                 public boost::enable_shared_from_this<SqlEditorForm>,
                                                 mforms::DropDelegate
{
public:
#if defined(ENABLE_TESTING)
  friend class EditorFormTester;
#endif

  enum
  {
    RefreshEditorTitle,     // refresh the caption of active editor
    RefreshRecordsetTitle,  // refresh caption of active recordset
    QueryExecutionStarted,  // show busy marker for active editor
    SaveRecordsetChanges,   // commit changes being made to active recordset (as if focus was removed)
    DiscardRecordsetChanges // revert changes being made to active recordset (as if esc was pressed)
  };

  enum ServerState
  {
    UnknownState,
    RunningState,
    PossiblyStoppedState
  };

  class RecordsetData : public Recordset::ClientData
  {
  public:
    boost::shared_ptr<SqlEditorResult> result_panel;
    MySQLEditor::Ptr editor;
    std::string generator_query;

    double duration;
    std::string ps_stat_error;
    std::map<std::string, boost::int64_t> ps_stat_info;
  };

public:
  typedef boost::shared_ptr<SqlEditorForm> Ref;
  typedef boost::weak_ptr<SqlEditorForm> Ptr;
  static SqlEditorForm::Ref create(wb::WBContextSQLIDE *wbsql, const db_mgmt_ConnectionRef &conn);
  static void report_connection_failure(const std::string &error, const db_mgmt_ConnectionRef &target);
  
protected:
  SqlEditorForm(wb::WBContextSQLIDE *wbsql, const db_mgmt_ConnectionRef &conn);

  void update_menu_and_toolbar();
  void update_toolbar_icons();
public:
  virtual ~SqlEditorForm();

  void cancel_connect();
  virtual void close();
  virtual bool is_main_form() { return true; }
  virtual std::string get_form_context_name() const;
  
  virtual mforms::MenuBar *get_menubar();
  virtual mforms::ToolBar *get_toolbar();
  std::string get_session_name();

  void auto_save();
  void save_workspace(const std::string &workspace_name, bool is_autosave);
  bool load_workspace(const std::string &workspace_name);
  
  void restore_last_workspace();
public:
  bec::GRTManager * grt_manager() const { return _grtm; }
  wb::WBContextSQLIDE *wbsql() const { return _wbsql; }

  void validate_menubar();

  void jump_to_placeholder();
private:
  wb::WBContextSQLIDE *_wbsql;
  GrtVersionRef _version;
  bec::GRTManager *_grtm;
  mforms::MenuBar *_menu;
  mforms::ToolBar *_toolbar;
  std::string _connection_info;
  base::LockFile *_autosave_lock;
  std::string _autosave_path;
  bool _autosave_disabled;
  bool _loading_workspace;
  bool _cancel_connect;

  void activate_command(const std::string &command);
  void rename_autosave_files(int from, int to);

public:
  // do NOT use rdbms->version().. it's not specific for this connection
  db_mgmt_RdbmsRef rdbms();
  GrtVersionRef rdbms_version() const;

  std::string get_connection_info() const { return _connection_info; }
  
public:

  MySQLEditor::Ref active_sql_editor();
  MySQLEditor::Ref sql_editor(int index);
  int sql_editor_index(MySQLEditor::Ref);
  std::string sql_editor_path(int index) { return _sql_editors[index]->filename; }
  std::string sql_editor_caption(int index=-1);
  void sql_editor_caption(int new_index, std::string caption);
  bool sql_editor_is_scratch(int index) { return _sql_editors[index]->is_scratch; }
  bool sql_editor_start_collapsed(int index);
  bool sql_editor_will_close(int index);
  bool sql_editor_reorder(MySQLEditor::Ref, int new_index);
  void sql_editor_open_file(int index, const std::string &file_path, const std::string &encoding= "");
  boost::shared_ptr<mforms::ToolBar> sql_editor_toolbar(int index) { return _sql_editors[index]->toolbar; }

private:
  int sql_editor_index_for_recordset(long rset);
  RecordsetsRef sql_editor_recordsets(const int index);

  struct EditorInfo {
    typedef boost::shared_ptr<EditorInfo> Ref;
    
    boost::shared_ptr<mforms::ToolBar> toolbar;
    std::string filename;
    std::string autosave_filename;
    std::string orig_encoding;
    std::string caption;
    MySQLEditor::Ref editor;
    RecordsetsRef recordsets;
    boost::shared_ptr<SqlEditorResult> active_result;
    base::Mutex recordset_mutex;
    time_t file_timestamp;
    int rs_sequence;
    bool is_scratch;
    bool start_collapsed;
    bool busy;
    
    EditorInfo() : rs_sequence(0), is_scratch(false), start_collapsed(false), busy(false) { memset(&file_timestamp, 0, sizeof(file_timestamp)); }
  };
  typedef std::vector<EditorInfo::Ref> Sql_editors;
  Sql_editors _sql_editors;
  int _sql_editors_serial;
  int _scratch_editors_serial;
  base::Mutex _sql_editors_mutex;
  
  boost::shared_ptr<mforms::ToolBar> setup_editor_toolbar(MySQLEditor::Ref editor);
  void set_editor_tool_items_enbled(const std::string &name, bool flag);
  void set_editor_tool_items_checked(const std::string &name, bool flag);
public:
  void set_tool_item_checked(const std::string &name, bool flag);

  boost::signals2::signal<void (MySQLEditor::Ref, bool)> sql_editor_list_changed;

  int run_sql_in_scratch_tab(const std::string &sql, bool reuse_if_possible, bool start_collapsed);
  int add_sql_editor(bool scratch = false, bool start_collapsed = false); // returns index of the added sql_editor
  void remove_sql_editor(int index);
  int sql_editor_count();
  int active_sql_editor_index() { return _active_sql_editor_index; }
  void active_sql_editor_index(int val);
  
  MySQLEditor::Ref active_sql_editor_or_new_scratch();

  virtual mforms::DragOperation drag_over(mforms::View *sender, base::Point p, const std::vector<std::string> &formats);
  virtual mforms::DragOperation files_dropped(mforms::View *sender, base::Point p, const std::vector<std::string> &file_names);
private:
  int _active_sql_editor_index;
  int _updating_sql_editor;

  int count_connection_editors(const std::string& conn_name);
  void set_sql_editor_text(const char *sql, int editor_index = -1);

protected:
  std::string create_title();
  void title_changed();
  void check_server_problems();
public:
  virtual std::string get_title() { return _title; }
  void update_title();

  std::map<std::string, std::string> &connection_details() { return _connection_details; }
  int server_version();
  std::set<std::string> valid_charsets();
private:
  std::map<std::string, std::string> _connection_details;
  std::set<std::string> _charsets;

  grt::StringRef do_connect(grt::GRT *grt, boost::shared_ptr<sql::TunnelConnection> tunnel, sql::Authentication::Ref &auth,
    struct ConnectionErrorInfo *autherr_ptr);
  grt::StringRef do_disconnect(grt::GRT *grt);

  void update_connected_state();
public:
  bool connect(boost::shared_ptr<sql::TunnelConnection> tunnel);
  bool connected() const;
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
  void toggle_collect_ps_statement_events();
  bool collect_field_info() const;
  bool collect_ps_statement_events() const;
  
  void run_editor_contents(bool current_statement_only);

  void list_members();

  void limit_rows(mforms::MenuItem *menu, const char *limit);

  std::string sql_mode() const { return _sql_mode; };
  int lower_case_table_names() const { return _lower_case_table_names; }
private:
  void do_commit();
public:  
  db_mgmt_ConnectionRef connection_descriptor() const { return _connection; }

  bool get_session_variable(sql::Connection *dbc_conn, const std::string &name, std::string &value);
  
private:
  void cache_sql_mode();
  void query_ps_statistics(boost::int64_t conn_id, std::map<std::string, boost::int64_t> &stats);
private:
  std::string _sql_mode;
  int _lower_case_table_names;
  parser::ParserContext::Ref _autocomplete_context; // Temporary, until auto completion is refactored.

private:
  void create_connection(sql::Dbc_connection_handler::Ref &dbc_conn, db_mgmt_ConnectionRef db_mgmt_conn, boost::shared_ptr<sql::TunnelConnection> tunnel, sql::Authentication::Ref auth, bool autocommit_mode, bool user_connection);
  void init_connection(sql::Connection* dbc_conn_ref, const db_mgmt_ConnectionRef& connectionProperties, sql::Dbc_connection_handler::Ref& dbc_conn, bool user_connection);
  void close_connection(sql::Dbc_connection_handler::Ref &dbc_conn);
  base::RecMutexLock ensure_valid_dbc_connection(sql::Dbc_connection_handler::Ref &dbc_conn, base::RecMutex &dbc_conn_mutex);
  base::RecMutexLock ensure_valid_usr_connection();
  base::RecMutexLock ensure_valid_aux_connection();

public:
  base::RecMutexLock ensure_valid_aux_connection(sql::Dbc_connection_handler::Ref &conn);

private:
  bec::TimerActionThread *_keep_alive_thread;
  base::Mutex _keep_alive_thread_mutex;
private:
  void send_message_keep_alive();
  void reset_keep_alive_thread();
  
  db_mgmt_ConnectionRef _connection;
  // connection for maintenance operations, fetching schema contents & live editors (DDL only)
  sql::Dbc_connection_handler::Ref _aux_dbc_conn;
  base::RecMutex _aux_dbc_conn_mutex;

  // connection for running sql scripts
  sql::Dbc_connection_handler::Ref _usr_dbc_conn;
  mutable base::RecMutex _usr_dbc_conn_mutex;

  sql::Authentication::Ref _dbc_auth;

  ServerState _last_server_running_state;

  AutoCompleteCache *_auto_completion_cache;
  base::RecMutexLock get_autocompletion_connection(sql::Dbc_connection_handler::Ref &conn);
  void on_cache_action(bool active);

public:
  bool exec_editor_sql(MySQLEditor::Ref editor, bool sync, bool current_statement_only = false,
    bool wrap_with_non_std_delimiter = false, bool dont_add_limit_clause = false);
  void exec_sql_retaining_editor_contents(const std::string &sql_script, MySQLEditor::Ref editor, bool sync, bool dont_add_limit_clause= false);

  RecordsetsRef exec_sql_returning_results(const std::string &sql_script, bool dont_add_limit_clause);

  void exec_management_sql(const std::string &sql, bool log);
  db_query_ResultsetRef exec_management_query(const std::string &sql, bool log);

  void exec_main_sql(const std::string &sql, bool log);
  db_query_ResultsetRef exec_main_query(const std::string &sql, bool log);

  void explain_sql();
  void explain_current_statement();
  bool is_running_query();
private:
  enum ExecFlags {
    Retaining           = 1 << 0, 
    NeedNonStdDelimiter = 1 << 1,
    DontAddLimitClause  = 1 << 2,
    ShowWarnings        = 1 << 3
  };
  void update_live_schema_tree(const std::string &sql);

  grt::StringRef do_exec_sql(grt::GRT *grt, Ptr self_ptr, boost::shared_ptr<std::string> sql,
    MySQLEditor::Ref editor, ExecFlags flags, RecordsetsRef result_list);
  void do_explain_sql(const std::string &sql);

  void handle_command_side_effects(const std::string &sql);
public:
  GrtThreadedTask::Ref exec_sql_task;
private:
  int on_exec_sql_finished();
  bool _is_running_query;
  bool _continue_on_error;
  
public:
  bool continue_on_error() { return _continue_on_error; }
  void continue_on_error(bool val);

private:
  typedef boost::signals2::signal<int (long long, const std::string&, const std::string&),boost::signals2::last_value<int> > Error_cb;
  typedef boost::signals2::signal<int (float),boost::signals2::last_value<int> > Batch_exec_progress_cb;
  typedef boost::signals2::signal<int (long, long),boost::signals2::last_value<int> > Batch_exec_stat_cb;
  
public:
  Error_cb on_sql_script_run_error;
private:
  Batch_exec_progress_cb on_sql_script_run_progress;
  Batch_exec_stat_cb on_sql_script_run_statistics;  
  
  int sql_script_apply_error(long long, const std::string&, const std::string&, std::string&);
  int sql_script_apply_progress(float);
  int sql_script_stats(long, long);
  
public:
  void apply_object_alter_script(std::string &alter_script, bec::DBObjectEditorBE* obj_editor, RowId log_id);
  bool run_live_object_alteration_wizard(const std::string &alter_script, bec::DBObjectEditorBE* obj_editor, RowId log_id, const std::string &log_context);

public:
  int recordset_count(int editor);
  boost::shared_ptr<SqlEditorResult> result_panel(Recordset::Ref rset);

  Recordset::Ref recordset(int editor, int idx);
  Recordset::Ref recordset_for_key(int editor, long key);
  Recordset::Ref active_recordset(int editor);
  void active_recordset(int editor, Recordset::Ref rset);

  void active_result_panel(int editor, boost::shared_ptr<SqlEditorResult> value);
  boost::shared_ptr<SqlEditorResult> active_result_panel(int editor);
  bool recordset_reorder(int editor, Recordset::Ref value, int new_index);
public:
  boost::signals2::signal<void (int, Recordset::Ref, bool)> recordset_list_changed;
private:
  void on_close_recordset(Recordset::Ptr rs_ptr);
private:
  void apply_changes_to_recordset(Recordset::Ptr rs_ptr);
  bool run_data_changes_commit_wizard(Recordset::Ptr rs_ptr);
  void apply_data_changes_commit(std::string &sql_script_text, Recordset::Ptr rs_ptr);
  void recall_recordset_query(Recordset::Ptr rs_ptr);
  void update_editor_title_schema(const std::string& schema);

  void on_recordset_context_menu_show(Recordset::Ptr rs_ptr, MySQLEditor::Ptr editor_ptr);
public:  
  bool can_close();
  bool can_close_(bool interactive);

  void check_external_file_changes();
public:
  void new_sql_script_file();
  void new_sql_scratch_area(bool start_collapsed = false);
  void open_file(const std::string &path, bool in_new_tab);
  void open_file(const std::string &path = "") { open_file(path, true); }
  void save_file();
  bool save_sql_script_file(const std::string &file_path, int editor_index);
  void revert_sql_script_file();

public:
  boost::signals2::signal<int (int)> sql_editor_new_ui;

public:
  void active_schema(const std::string &value);
  std::string active_schema() const;
  void schema_meta_data_refreshed(const std::string &schema_name,
                                  const std::vector<std::pair<std::string,bool> >& tables, 
                                  const std::vector<std::pair<std::string,bool> >& procedures, 
                                  bool just_append);
private:
  void cache_active_schema_name();

public:
  void request_refresh_schema_tree();
  
private:
  boost::shared_ptr<SqlEditorTreeController> _live_tree;
    
  mforms::View* _side_palette_host;
  QuerySidePalette* _side_palette;

public:
  std::string fetch_data_from_stored_procedure(std::string proc_call, boost::shared_ptr<sql::ResultSet> &rs);

  DbSqlEditorLog::Ref log() { return _log; }
  DbSqlEditorHistory::Ref history() { return _history; }
  std::string restore_sql_from_history(int entry_index, std::list<int> &detail_indexes);
  int exec_sql_error_count() { return _exec_sql_error_count; }
  
  boost::shared_ptr<SqlEditorTreeController> get_live_tree() { return _live_tree; }
  
  boost::function<void (const std::string&, bool)> output_text_slot;
protected:
  DbSqlEditorLog::Ref _log;
  DbSqlEditorHistory::Ref _history;

public:
  // Result should be RowId but that requires to change the task callback type (at least for 64bit builds).
  int add_log_message(int msg_type, const std::string &msg, const std::string &context, const std::string &duration);
  void set_log_message(RowId log_message_index, int msg_type, const std::string &msg, const std::string &context, const std::string &duration);
  void refresh_log_messages(bool ignore_last_message_timestamp);
private:
  bool _has_pending_log_messages;
  double _last_log_message_timestamp;
  int _exec_sql_error_count;

protected:
  double _progress_status_update_interval;
  std::string _title;

private:
  virtual void handle_grt_notification(const std::string &name, grt::ObjectRef sender, grt::DictRef info);
  virtual void handle_notification(const std::string &name, void *sender, base::NotificationInfo &info);
  void setup_side_palette();
  
  void schema_row_selected();
  void side_bar_filter_changed(const std::string& filter);

  void note_connection_open_outcome(int error);

public:
  void inspect_object(const std::string &name, const std::string &object, const std::string &type);

  void toolbar_command(const std::string& command);

  bool save_snippet();

  void show_output_area();

  mforms::View *get_sidebar();
  mforms::View *get_side_palette();

  void set_autosave_disabled(const bool autosave_disabled);
  bool get_autosave_disabled(void);
};


#endif /* _WB_SQL_EDITOR_FORM_H_ */
