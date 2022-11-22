/*
 * Copyright (c) 2007, 2022, Oracle and/or its affiliates.
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

#include "wb_sql_editor_form.h"

#include "grtdb/db_helpers.h"
#include "grtsqlparser/sql_facade.h"
#include "grtdb/editor_dbobject.h"
#include "grtdb/db_object_helpers.h"

#include "grtui/confirm_save_dialog.h"

#include "sqlide/recordset_be.h"
#include "sqlide/recordset_cdbc_storage.h"
#include "sqlide/wb_sql_editor_snippets.h"
#include "sqlide/wb_sql_editor_panel.h"
#include "sqlide/wb_sql_editor_result_panel.h"
#include "sqlide/wb_sql_editor_tree_controller.h"
#include "sqlide/sql_script_run_wizard.h"

#include "sqlide/column_width_cache.h"

#include "objimpl/db.query/db_query_Resultset.h"
#include "objimpl/wrapper/mforms_ObjectReference_impl.h"

#include "base/string_utilities.h"
#include "base/notifications.h"
#include "base/sqlstring.h"
#include "base/file_functions.h"
#include "base/file_utilities.h"
#include "base/log.h"
#include "base/boost_smart_ptr_helpers.h"
#include "base/util_functions.h"
#include "base/scope_exit_trigger.h"
#include "base/threading.h"

#include "workbench/wb_command_ui.h"
#include "workbench/wb_context_names.h"
#include "workbench/SSHSessionWrapper.h"
#include "workbench/wb_context_ui.h"
#include "workbench/wb_context.h"

#include <mysql_connection.h>

#include <boost/signals2/connection.hpp>

#include "query_side_palette.h"

#include "mforms/menubar.h"
#include "mforms/hypertext.h" // needed for d-tor
#include "mforms/tabview.h"   // needed for d-tor
#include "mforms/splitter.h"  // needed for d-tor
#include "mforms/toolbar.h"
#include "mforms/code_editor.h"

#include "grtsqlparser/mysql_parser_services.h"

#include "wb_tunnel.h"

#include <math.h>
#include <mutex>
#include <thread>

using namespace bec;
using namespace grt;
using namespace wb;
using namespace base;
using namespace parsers;

using boost::signals2::scoped_connection;

using namespace std::string_literals;

DEFAULT_LOG_DOMAIN("SQL Editor Form")

static const char *SQL_EXCEPTION_MSG_FORMAT = _("Error Code: %i\n%s");
static const char *EXCEPTION_MSG_FORMAT = _("Error: %s");

#define CATCH_SQL_EXCEPTION_AND_DISPATCH(statement, log_message_index, duration)                        \
  catch (sql::SQLException & e) {                                                                       \
    set_log_message(log_message_index, DbSqlEditorLog::ErrorMsg,                                        \
                    strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()), statement, duration); \
  }

#define CATCH_EXCEPTION_AND_DISPATCH(statement)                                                       \
  catch (std::exception & e) {                                                                        \
    add_log_message(DbSqlEditorLog::ErrorMsg, strfmt(EXCEPTION_MSG_FORMAT, e.what()), statement, ""); \
  }

#define CATCH_ANY_EXCEPTION_AND_DISPATCH(statement)                                                                    \
  catch (sql::SQLException & e) {                                                                                      \
    add_log_message(DbSqlEditorLog::ErrorMsg, strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()), statement, \
                    "");                                                                                               \
  }                                                                                                                    \
  CATCH_EXCEPTION_AND_DISPATCH(statement)

#define CATCH_ANY_EXCEPTION_AND_DISPATCH_TO_DEFAULT_LOG(statement)                                        \
  catch (sql::SQLException & e) {                                                                         \
    grt::GRT::get()->send_error(strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()), statement); \
  }                                                                                                       \
  catch (std::exception & e) {                                                                            \
    grt::GRT::get()->send_error(strfmt(EXCEPTION_MSG_FORMAT, e.what()), statement);                       \
  }

db_mgmt_ServerInstanceRef getServerInstance(const db_mgmt_ConnectionRef &connection) {
  grt::ValueRef ret = grt::GRT::get()->get("/wb/rdbmsMgmt/storedInstances");
  if (grt::ListRef<db_mgmt_ServerInstance>::can_wrap(ret))
  {
    auto list = grt::ListRef<db_mgmt_ServerInstance>::cast_from(ret);
    for (const auto &item: list) {
      if (item->connection() == connection)
        return item;
    }
  }

  return db_mgmt_ServerInstanceRef();
}

//----------------------------------------------------------------------------------------------------------------------

class Timer {
public:
  Timer(bool run_immediately) : _is_running(false), _start_timestamp(0), _duration(0) {
    if (run_immediately)
      run();
  }
  void reset() {
    _is_running = false;
    _start_timestamp = 0;
    _duration = 0;
  }
  void run() {
    if (_is_running)
      return;
    _is_running = true;
    _start_timestamp = timestamp();
  }
  void stop() {
    if (!_is_running)
      return;
    _is_running = false;
    _duration += timestamp() - _start_timestamp;
  }
  double duration() {
    return _is_running ? (_duration + timestamp() - _start_timestamp) : _duration;
  }
  std::string duration_formatted() {
    double d = duration(), dd;
    dd = d;
    int zeroes = 1;

    while (dd < 1.0 && dd > 0.0) {
      zeroes++;
      dd *= 10;
    }

    return strfmt(strfmt(_("%%.%if sec"), std::max(3, zeroes)).c_str(), d);
  }

private:
  bool _is_running;
  double _start_timestamp;
  double _duration;
};

//----------------------------------------------------------------------------------------------------------------------

struct SqlEditorForm::PrivateMutex {
  std::mutex _symbolsMutex;
};

//----------------------------------------------------------------------------------------------------------------------

SqlEditorForm::Ref SqlEditorForm::create(wb::WBContextSQLIDE *wbsql, const db_mgmt_ConnectionRef &conn) {
  SqlEditorForm::Ref instance(new SqlEditorForm(wbsql));

  if (conn.is_valid())
    instance->set_connection(conn);

  return instance;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::set_tab_dock(mforms::DockingPoint *dp) {
  _tabdock = dp;
  grtobj()->dockingPoint(mforms_to_grt(dp));
  scoped_connect(_tabdock->signal_view_switched(), std::bind(&SqlEditorForm::sql_editor_panel_switched, this));
  scoped_connect(_tabdock->signal_view_undocked(),
                 std::bind(&SqlEditorForm::sql_editor_panel_closed, this, std::placeholders::_1));
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::report_connection_failure(const std::string &error, const db_mgmt_ConnectionRef &target) {
  std::string message;
  logError("SQL editor could not be connected: %s\n", error.c_str());
  mforms::App::get()->set_status_text(_("Could not connect to target database."));

  if (error.find("exceeded the 'max_user_connections' resource") != std::string::npos) {
    mforms::Utilities::show_error(_("Could not Connect to Database Server"),
                                  base::strfmt("%s\n\nMySQL Workbench requires at least 2 connections to the server, "
                                               "one for management purposes and another for user queries.",
                                               error.c_str()),
                                  "OK");
    return;
  }

  message =
    "Your connection attempt failed for user '%user%' to the MySQL server at %server%:%port%:\n  %error%\n"
    "\n"
    "Please:\n"
    "1 Check that MySQL is running on address %server%\n"
    "2 Check that MySQL is reachable on port %port% (note: 3306 is the default, but this can be changed)\n"
    "3 Check the user %user% has rights to connect to %server% from your address (MySQL rights define what clients can "
    "connect to the server and from which machines) \n"
    "4 Make sure you are both providing a password if needed and using the correct password for %server% connecting "
    "from the host address you're connecting from";

  message = base::replaceString(message, "%user%", target->parameterValues().get_string("userName"));
  message = base::replaceString(message, "%port%", target->parameterValues().get("port").toString());
  message = base::replaceString(message, "%server%", target->parameterValues().get_string("hostName", "localhost"));
  message = base::replaceString(message, "%error%", error);

  logError("%s", (message + '\n').c_str());
  mforms::Utilities::show_error(_("Cannot Connect to Database Server"), message, _("Close"));
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::report_connection_failure(const grt::server_denied &info, const db_mgmt_ConnectionRef &target) {
  std::string message;

  logError("Server is alive, but has login restrictions: %d, %s\n", info.errNo, info.what());

  mforms::App::get()->set_status_text(_("Connection restricted"));

  message = "Your connection attempt failed for user '";
  message += target->parameterValues().get_string("userName");
  message += "' from your host to server at "; //%server%:%port%\n";
  message += target->parameterValues().get_string("hostName", "localhost");
  message += ":";
  message += target->parameterValues().get("port").toString() + "\n";
  if (info.errNo == 3159)
    message += "Only connections with enabled SSL support are accepted.\n";
  else if (info.errNo == 3032)
    message += "The server is in super-user mode and does not accept any other connection.\n";

  message += "\nThe server response was:\n";
  message += info.what();

  mforms::Utilities::show_error(_("Cannot Connect to Database Server"), message, _("Close"));
}

//----------------------------------------------------------------------------------------------------------------------

SqlEditorForm::SqlEditorForm(wb::WBContextSQLIDE *wbsql)
  : exec_sql_task(GrtThreadedTask::create()),
    _history(DbSqlEditorHistory::create()),
    _wbsql(wbsql),
    _version(grt::Initialized),
    _live_tree(SqlEditorTreeController::create(this)),
    _aux_dbc_conn(new sql::Dbc_connection_handler()),
    _usr_dbc_conn(new sql::Dbc_connection_handler()),
    _pimplMutex (new PrivateMutex) {
  _log = DbSqlEditorLog::create(this, 500);

  NotificationCenter::get()->add_observer(this, "GNApplicationActivated");
  NotificationCenter::get()->add_observer(this, "GNMainFormChanged");
  NotificationCenter::get()->add_observer(this, "GNFormTitleDidChange");
  NotificationCenter::get()->add_observer(this, "GNColorsChanged");
  GRTNotificationCenter::get()->add_grt_observer(this, "GRNServerStateChanged");
  exec_sql_task->desc("execute sql queries");
  exec_sql_task->send_task_res_msg(false);
  exec_sql_task->msg_cb(std::bind(&SqlEditorForm::add_log_message, this, std::placeholders::_1, std::placeholders::_2,
                                  std::placeholders::_3, ""));

  _last_log_message_timestamp = timestamp();

  long keep_alive_interval = bec::GRTManager::get()->get_app_option_int("DbSqlEditor:KeepAliveInterval", 600);

  if (keep_alive_interval != 0) {
    logDebug3("Creating KeepAliveInterval timer...\n");
    _keep_alive_task_id = ThreadedTimer::add_task(
      TimerTimeSpan, keep_alive_interval, false, std::bind(&SqlEditorForm::send_message_keep_alive_bool_wrapper, this));
  }

  _lower_case_table_names = 0;

  _continueOnError = (bec::GRTManager::get()->get_app_option_int("DbSqlEditor:ContinueOnError", 0) != 0);

  // set initial autocommit mode value
  _usr_dbc_conn->autocommit_mode = (bec::GRTManager::get()->get_app_option_int("DbSqlEditor:AutocommitMode", 1) != 0);

  _databaseSymbols.addDependencies({ &_staticServerSymbols });
}

//----------------------------------------------------------------------------------------------------------------------

SqlEditorForm::~SqlEditorForm() {
  if (_editorRefreshPending.connected())
    _editorRefreshPending.disconnect();

  if (_overviewRefreshPending.connected())
    _overviewRefreshPending.disconnect();

  // We need to remove it from cache, if not someone will be able to login without providing PW
  if (_connection.is_valid())
    mforms::Utilities::forget_cached_password(_connection->hostIdentifier(),
                                              _connection->parameterValues().get_string("userName"));

  delete _column_width_cache;

  // debug: ensure that close() was called when the tab is closed
  if (_toolbar != nullptr)
    logFatal("SqlEditorForm::close() was not called\n");

  NotificationCenter::get()->remove_observer(this);
  GRTNotificationCenter::get()->remove_grt_observer(this);

  delete _autosave_lock;
  _autosave_lock = 0;

  // Destructor can be called before the startup was finished.
  // On Windows the side palette is a child of the palette host and hence gets freed when we
  // free the host. On other platforms both are the same. In any case, double freeing it is
  // not a good idea.
  if (_side_palette_host != nullptr)
    _side_palette_host->release();

  delete _toolbar;
  delete _menu;

  reset_keep_alive_thread();
  _sshConnection.release();
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::cancel_connect() {
  _cancel_connect = true;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::check_server_problems() {
  //_lower_case_table_names
  std::string compile_os;
  if (_usr_dbc_conn && get_session_variable(_usr_dbc_conn->ref.get(), "version_compile_os", compile_os)) {
    if ((_lower_case_table_names == 0 && (base::hasPrefix(compile_os, "Win") || base::hasPrefix(compile_os, "osx"))) ||
        (_lower_case_table_names == 2 && base::hasPrefix(compile_os, "Win")))
      mforms::Utilities::show_message_and_remember(
        _("Server Configuration Problems"),
        "A server configuration problem was detected.\nThe server is in a system that does not properly support the "
        "selected lower_case_table_names option value. Some problems may occur.\nPlease consult the MySQL server "
        "documentation.",
        _("OK"), "", "", "SQLIDE::check_server_problems::lower_case_table_names", "");
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::finish_startup() {
  setup_side_palette();

  _live_tree->finish_init();

  std::string cache_dir = bec::GRTManager::get()->get_user_datadir() + "/cache/";
  try {
    base::create_directory(cache_dir, 0700); // No-op if the folder already exists.
  } catch (std::exception &e) {
    logError("Could not create %s: %s\n", cache_dir.c_str(), e.what());
  }

  _column_width_cache = new ColumnWidthCache(sanitize_file_name(get_session_name()), cache_dir);

  if (_usr_dbc_conn && !_usr_dbc_conn->active_schema.empty())
    _live_tree->on_active_schema_change(_usr_dbc_conn->active_schema);
  readStaticServerSymbols();

  bec::GRTManager::get()->run_once_when_idle(this, std::bind(&SqlEditorForm::update_menu_and_toolbar, this));

  this->check_server_problems();

  // We need to check this before sending GRNSQLEditorOpened cause offline() function that's called
  // from python which is connected to this notification will deadlock on PythonLock.
  checkIfOffline();

  // refresh snippets again, in case the initial load from DB is pending for shared snippets
  _side_palette->refresh_snippets();

  GRTNotificationCenter::get()->send_grt("GRNSQLEditorOpened", grtobj(), grt::DictRef());

  int keep_alive_interval = (int)bec::GRTManager::get()->get_app_option_int("DbSqlEditor:KeepAliveInterval", 600);

  //  We have to set these variables so that the server doesn't timeout before we ping everytime
  // From http://dev.mysql.com/doc/refman/5.7/en/communication-errors.html for reasones to loose the connection
  // - The client had been sleeping more than wait_timeout or interactive_timeout seconds without issuing any requests
  // to the server
  //  We're adding 10 seconds for communication delays
  {
    std::string value;

    if (get_session_variable(_usr_dbc_conn->ref.get(), "wait_timeout", value) &&
        base::atoi<int>(value) < keep_alive_interval)
      exec_main_sql(base::strfmt("SET @@SESSION.wait_timeout=%d", keep_alive_interval + 10), false);

    if (get_session_variable(_usr_dbc_conn->ref.get(), "interactive_timeout", value) &&
        base::atoi<int>(value) < keep_alive_interval)
      exec_main_sql(base::strfmt("SET @@SESSION.interactive_timeout=%d", keep_alive_interval + 10), false);
  }

  _startup_done = true;
}

//----------------------------------------------------------------------------------------------------------------------

base::RecMutexLock SqlEditorForm::getAuxConnection(sql::Dbc_connection_handler::Ref &conn, bool lockOnly) {
  RecMutexLock lock(ensure_valid_aux_connection(false, lockOnly));
  conn = _aux_dbc_conn;
  return lock;
}

//----------------------------------------------------------------------------------------------------------------------

base::RecMutexLock SqlEditorForm::getUserConnection(sql::Dbc_connection_handler::Ref &conn, bool lockOnly) {
  RecMutexLock lock(ensure_valid_usr_connection(false, lockOnly));
  conn = _usr_dbc_conn;
  return lock;
}

//----------------------------------------------------------------------------------------------------------------------

db_query_EditorRef SqlEditorForm::grtobj() {
  return wbsql()->get_grt_editor_object(this);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the name for this WQE instance derived from the connection it uses.
 * Used for workspace and action log.
 */
std::string SqlEditorForm::get_session_name() {
  if (_connection.is_valid()) {
    std::string name = _connection->name();
    if (name.empty())
      name = _connection->hostIdentifier();
    return name;
  }
  return "unconnected";
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::restore_last_workspace() {
  std::string name = get_session_name();
  if (!name.empty())
    load_workspace(sanitize_file_name(name));

  if (_tabdock->view_count() == 0)
    new_sql_scratch_area(false);

  // Gets the title for a NEW editor.
  _title = create_title();
  title_changed();
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::title_changed() {
  base::NotificationInfo info;
  info["form"] = form_id();
  info["title"] = _title;
  info["connection"] = _connection.is_valid() ? _connection->name() : "";
  base::NotificationCenter::get()->send("GNFormTitleDidChange", this, info);
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::handle_grt_notification(const std::string &name, grt::ObjectRef sender, grt::DictRef info) {
  if (name == "GRNServerStateChanged") {
    db_mgmt_ConnectionRef conn(db_mgmt_ConnectionRef::cast_from(info.get("connection")));

    ServerState new_state = UnknownState;
    if (info.get_int("state") == 1) {
      _serverIsOffline = false;
      new_state = RunningState;
    } else if (info.get_int("state") == -1) {
      _serverIsOffline = true;
      new_state = OfflineState;
    } else {
      _serverIsOffline = false;
      new_state = PossiblyStoppedState;
    }

    if (_last_server_running_state != new_state) {
      _last_server_running_state = new_state;
      if ((new_state == RunningState || new_state == OfflineState) && ping()) {
        // if new state is running but we're already connected, don't do anything
        return;
      }
      // reconnect when idle, to avoid any deadlocks
      if (conn.is_valid() && conn == connection_descriptor())
        bec::GRTManager::get()->run_once_when_idle(this, std::bind(&WBContextSQLIDE::reconnect_editor, wbsql(), this));
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::handle_notification(const std::string &name, void *sender, base::NotificationInfo &info) {
  if (name == "GNMainFormChanged") {
    if (_side_palette)
      _side_palette->close_popover();
    if (info["form"] == form_id())
      update_menu_and_toolbar();
  } else if (name == "GNFormTitleDidChange") {
    // Validates only if another editor to the same connection has sent the notification
    if (info["form"] != form_id() && _connection.is_valid() && _connection->name() == info["connection"]) {
      // This code is reached when at least 2 editors to the same host
      // have been opened, so the label of the old editor (which may not
      // contain the schema name should be updated with it).
      update_title();
    }
  } else if (name == "GNColorsChanged") {
    // Single colors or the entire color scheme changed.
    update_toolbar_icons();
  } else if (name == "GNApplicationActivated") {
    check_external_file_changes();
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::reset_keep_alive_thread() {
  MutexLock keep_alive_thread_lock(_keep_alive_thread_mutex);
  if (_keep_alive_task_id) {
    ThreadedTimer::remove_task(_keep_alive_task_id);
    _keep_alive_task_id = 0;
  }
}

//----------------------------------------------------------------------------------------------------------------------

grt::StringRef SqlEditorForm::do_disconnect() {
  if (_usr_dbc_conn->ref.get()) {
    {
      RecMutexLock lock(_usr_dbc_conn_mutex);
      close_connection(_usr_dbc_conn);
      _usr_dbc_conn->ref.reset();
    }

    {
      RecMutexLock lock(_aux_dbc_conn_mutex);
      close_connection(_aux_dbc_conn);
      _aux_dbc_conn->ref.reset();
    }
  }

  return grt::StringRef();
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::close() {
  grt::ValueRef option(bec::GRTManager::get()->get_app_option("workbench:SaveSQLWorkspaceOnClose"));

  if (option.is_valid() && *grt::IntegerRef::cast_from(option)) {
    bec::GRTManager::get()->replace_status_text("Saving workspace state...");
    if (_autosave_path.empty()) {
      save_workspace(sanitize_file_name(get_session_name()), false);
      delete _autosave_lock;
    } else {
      auto_save();

      // Remove auto lock first or renaming the folder will fail.
      delete _autosave_lock;
      std::string new_name(base::strip_extension(_autosave_path) + ".workspace");
      int try_count = 0;

      // Rename our temporary workspace if one exists to make it a persistent one.
      if (base::file_exists(_autosave_path)) {
        for (;;) {
          try {
            if (base::file_exists(new_name))
              base::remove_recursive(new_name);
            base::rename(_autosave_path, new_name);
          } catch (base::file_error &err) {
            std::string path(dirname(_autosave_path));
            do {
              ++try_count;
              new_name =
                base::makePath(path, sanitize_file_name(get_session_name()).append(strfmt("-%i.workspace", try_count)));
            } while (file_exists(new_name));

            if (err.code() == base::already_exists)
              continue;
            logWarning("Could not rename autosave directory %s: %s\n", _autosave_path.c_str(), err.what());
          }

          break;
        }
      }
    }
    _autosave_lock = 0;
  } else {
    delete _autosave_lock;
    _autosave_lock = 0;
    if (!_autosave_path.empty())
      base_rmdir_recursively(_autosave_path.c_str());
  }

  // Ensure all processing is stopped before freeing the info structure, otherwise references
  // are kept that prevent the correct deletion of the editor.
  if (_tabdock) {
    for (std::size_t c = _tabdock->view_count(), i = 0; i < c; i++) {
      SqlEditorPanel *p = sql_editor_panel((int)i);
      if (p)
        p->editor_be()->stop_processing();
    }

    _closing = true;
    _tabdock->close_all_views();
    _closing = false;
  }
  bec::GRTManager::get()->replace_status_text("Closing SQL Editor...");
  wbsql()->editor_will_close(this);

  exec_sql_task->exec(true, std::bind(&SqlEditorForm::do_disconnect, this));
  exec_sql_task->disconnect_callbacks();
  reset_keep_alive_thread();
  bec::GRTManager::get()->replace_status_text("SQL Editor closed");

  delete _menu;
  _menu = nullptr;
  delete _toolbar;
  _toolbar = nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

std::string SqlEditorForm::get_form_context_name() const {
  return WB_CONTEXT_QUERY;
}

//----------------------------------------------------------------------------------------------------------------------

db_mgmt_SSHConnectionRef SqlEditorForm::getSSHConnection() {
  try {
    if (!_sshConnection.is_valid()) {
      if (_connection.is_valid()) {
        auto val = getServerInstance(_connection);
        if (val.is_valid()) {
          db_mgmt_SSHConnectionRef object(grt::Initialized);
          object->owner(wb::WBContextUI::get()->get_wb()->get_root());
          object->name(_connection->name());
          object->set_data(new ssh::SSHSessionWrapper(val));
          _sshConnection = object;
        }
      }
    }
  } catch (std::runtime_error &) {
    logError("Unable to create db_mgmt_SSHConnectionRef object\n");
  }
  return _sshConnection;
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorForm::get_session_variable(sql::Connection *dbc_conn, const std::string &name, std::string &value) {
  if (dbc_conn) {
    SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms(rdbms());
    Sql_specifics::Ref sql_specifics = sql_facade->sqlSpecifics();
    std::string query = sql_specifics->query_variable(name);
    if (query.empty())
      return false;

    const std::unique_ptr<sql::Statement> statement(dbc_conn->createStatement());
    const std::unique_ptr<sql::ResultSet> rs(statement->executeQuery(query));
    if (rs->next()) {
      value = rs->getString(2);
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::schema_tree_did_populate() {
  if (!_pending_expand_nodes.empty() &&
      bec::GRTManager::get()->get_app_option_int("DbSqlEditor:SchemaTreeRestoreState", 1)) {
    std::string schema, groups;
    base::partition(_pending_expand_nodes, ":", schema, groups);

    mforms::TreeNodeRef node =
      _live_tree->get_schema_tree()->get_node_for_object(schema, wb::LiveSchemaTree::Schema, "");
    if (node) {
      static const char *nodes[] = {"tables", "views", "procedures", "functions", nullptr};
      node->expand();
      for (int i = 0; nodes[i]; i++)
        if (strstr(groups.c_str(), nodes[i])) {
          mforms::TreeNodeRef child = node->get_child(i);
          if (child)
            child->expand();
        }
    }
    _pending_expand_nodes.clear();
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::string SqlEditorForm::fetch_data_from_stored_procedure(std::string proc_call,
                                                            std::shared_ptr<sql::ResultSet> &rs) {
  std::string ret_val("");
  try {
    RecMutexLock aux_dbc_conn_mutex(ensure_valid_aux_connection());
    std::unique_ptr<sql::Statement> stmt(_aux_dbc_conn->ref->createStatement());
    stmt->execute(std::string(proc_call));
    do {
      rs.reset(stmt->getResultSet());
    } while (stmt->getMoreResults());
  } catch (const sql::SQLException &exc) {
    logWarning("Error retrieving data from stored procedure '%s': Error %d : %s", proc_call.c_str(), exc.getErrorCode(),
               exc.what());
    ret_val = base::strfmt("MySQL Error : %s (code %d)", exc.what(), exc.getErrorCode());
  }

  return ret_val;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::update_sql_mode_for_editors() {
  for (int c = sql_editor_count(), i = 0; i < c; i++) {
    SqlEditorPanel *panel = sql_editor_panel(i);
    if (panel)
      panel->editor_be()->set_sql_mode(_sql_mode);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::cache_sql_mode() {
  std::string sql_mode;
  if (_usr_dbc_conn && get_session_variable(_usr_dbc_conn->ref.get(), "sql_mode", sql_mode)) {
    if (sql_mode != _sql_mode) {
      _sql_mode = sql_mode;
      bec::GRTManager::get()->run_once_when_idle(this, std::bind(&SqlEditorForm::update_sql_mode_for_editors, this));
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::query_ps_statistics(std::int64_t conn_id, std::map<std::string, std::int64_t> &stats) {
  static const char *stat_fields[] = {"EVENT_ID",
                                      "THREAD_ID",
                                      "TIMER_WAIT",
                                      "LOCK_TIME",
                                      "ERRORS",
                                      "WARNINGS",
                                      "ROWS_AFFECTED",
                                      "ROWS_SENT",
                                      "ROWS_EXAMINED",
                                      "CREATED_TMP_DISK_TABLES",
                                      "CREATED_TMP_TABLES",
                                      "SELECT_FULL_JOIN",
                                      "SELECT_FULL_RANGE_JOIN",
                                      "SELECT_RANGE",
                                      "SELECT_RANGE_CHECK",
                                      "SELECT_SCAN",
                                      "SORT_MERGE_PASSES",
                                      "SORT_RANGE",
                                      "SORT_ROWS",
                                      "SORT_SCAN",
                                      "NO_INDEX_USED",
                                      "NO_GOOD_INDEX_USED",
                                      nullptr};
  RecMutexLock lock(ensure_valid_aux_connection());

  std::unique_ptr<sql::Statement> stmt(_aux_dbc_conn->ref->createStatement());

  try {
    std::unique_ptr<sql::ResultSet> result(stmt->executeQuery(base::strfmt(
      "SELECT st.* FROM performance_schema.events_statements_current st JOIN performance_schema.threads thr"
      " ON thr.thread_id = st.thread_id WHERE thr.processlist_id = %lli",
      (long long int)conn_id)));
    while (result->next()) {
      for (const char **field = stat_fields; *field; ++field) {
        stats[*field] = result->getInt64(*field);
      }
    }
  } catch (sql::SQLException &exc) {
    logException("Error querying performance_schema.events_statements_current\n", exc);
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<SqlEditorForm::PSStage> SqlEditorForm::query_ps_stages(std::int64_t stmt_event_id) {
  RecMutexLock lock(ensure_valid_aux_connection());

  std::unique_ptr<sql::Statement> stmt(_aux_dbc_conn->ref->createStatement());
  std::vector<PSStage> stages;
  try {
    std::unique_ptr<sql::ResultSet> result(stmt->executeQuery(
      base::strfmt("SELECT st.* FROM performance_schema.events_stages_history_long st WHERE st.nesting_event_id = %lli",
                   (long long int)stmt_event_id)));
    while (result->next()) {
      double wait_time = (double)result->getInt64("TIMER_WAIT") / 1000000000.0; // ps to ms
      std::string event = result->getString("EVENT_NAME");

      // rename the stage/sql/Sending data event to something more suitable
      if (event == "stage/sql/Sending data")
        event = "executing (storage engine)";

      bool flag = false;
      for (std::vector<PSStage>::iterator iter = stages.begin(); iter != stages.end(); ++iter) {
        if (iter->name == event) {
          flag = true;
          iter->wait_time += wait_time;
          break;
        }
      }

      if (!flag) {
        PSStage stage;
        stage.name = event;
        stage.wait_time = wait_time;
        stages.push_back(stage);
      }
    }
  } catch (sql::SQLException &exc) {
    logException("Error querying performance_schema.event_stages_history\n", exc);
  }

  return stages;
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<SqlEditorForm::PSWait> SqlEditorForm::query_ps_waits(std::int64_t stmt_event_id) {
  RecMutexLock lock(ensure_valid_aux_connection());

  std::unique_ptr<sql::Statement> stmt(_aux_dbc_conn->ref->createStatement());
  std::vector<PSWait> waits;
  try {
    std::unique_ptr<sql::ResultSet> result(stmt->executeQuery(
      base::strfmt("SELECT st.* FROM performance_schema.events_waits_history_long st WHERE st.nesting_event_id = %lli",
                   (long long int)stmt_event_id)));
    while (result->next()) {
      double wait_time = (double)result->getInt64("TIMER_WAIT") / 1000000000.0; // ps to ms
      std::string event = result->getString("EVENT_NAME");
      bool flag = false;
      for (std::vector<PSWait>::iterator iter = waits.begin(); iter != waits.end(); ++iter) {
        if (iter->name == event) {
          flag = true;
          iter->wait_time += wait_time;
          break;
        }
      }

      if (!flag) {
        PSWait wait;
        wait.name = event;
        wait.wait_time = wait_time;
        waits.push_back(wait);
      }
    }
  } catch (sql::SQLException &exc) {
    logException("Error querying performance_schema.event_waits_history\n", exc);
  }
  return waits;
}

//----------------------------------------------------------------------------------------------------------------------

SqlEditorPanel *SqlEditorForm::run_sql_in_scratch_tab(const std::string &sql, bool reuse_if_possible,
                                                      bool start_collapsed) {
  SqlEditorPanel *editor;

  if (!(editor = active_sql_editor_panel()) || !reuse_if_possible || !editor->is_scratch())
    editor = new_sql_scratch_area(start_collapsed);
  editor->editor_be()->get_editor_control()->set_text(sql.c_str());
  run_editor_contents(false);
  editor->editor_be()->get_editor_control()->reset_dirty();

  return editor;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::reset() {
  SqlEditorPanel *panel = active_sql_editor_panel();
  if (panel)
    panel->editor_be()->cancel_auto_completion();
}

//----------------------------------------------------------------------------------------------------------------------

void logToWorkbenchLog(int messageType, std::string const &msg) {
  switch (messageType) {
    case DbSqlEditorLog::ErrorMsg:
      logError("%s\n", msg.c_str());
      break;

    case DbSqlEditorLog::WarningMsg:
      logWarning("%s\n", msg.c_str());
      break;

    case DbSqlEditorLog::NoteMsg:
      logDebug("%s\n", msg.c_str());
      break;

    case DbSqlEditorLog::OKMsg:
      logDebug2("%s\n", msg.c_str());
      break;

    case DbSqlEditorLog::BusyMsg:
      logDebug3("%s\n", msg.c_str());
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

int SqlEditorForm::add_log_message(int messageType, const std::string &msg, const std::string &context,
                                   const std::string &duration) {
  RowId new_log_message_index = _log->add_message(messageType, context, msg, duration);
  _has_pending_log_messages = true;
  refresh_log_messages(false);
  if (messageType == DbSqlEditorLog::ErrorMsg || messageType == DbSqlEditorLog::WarningMsg)
    _exec_sql_error_count++;

  logToWorkbenchLog(messageType, msg);
  return (int)new_log_message_index;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::set_log_message(RowId log_message_index, int messageType, const std::string &msg,
                                    const std::string &context, const std::string &duration) {
  if (log_message_index != (RowId)-1) {
    _log->set_message(log_message_index, messageType, context, msg, duration);
    _has_pending_log_messages = true;
    if (messageType == DbSqlEditorLog::ErrorMsg || messageType == DbSqlEditorLog::WarningMsg)
      ++_exec_sql_error_count;
    refresh_log_messages(messageType == DbSqlEditorLog::BusyMsg); // Force refresh only for busy messages.
  }

  logToWorkbenchLog(messageType, msg);
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::refresh_log_messages(bool ignore_last_message_timestamp) {
  if (_has_pending_log_messages) {
    bool is_refresh_needed = ignore_last_message_timestamp;
    if (!ignore_last_message_timestamp) {
      double now = timestamp();
      int progress_status_update_interval =
        (int)(bec::GRTManager::get()->get_app_option_int("DbSqlEditor:ProgressStatusUpdateInterval", 500) / 1000.);

      if (_last_log_message_timestamp + progress_status_update_interval < now)
        is_refresh_needed = true;
      _last_log_message_timestamp = now;
    }
    if (is_refresh_needed) {
      _log->refresh();
      _has_pending_log_messages = false;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::init_connection(sql::Connection *dbc_conn_ref, const db_mgmt_ConnectionRef &connectionProperties,
                                    sql::Dbc_connection_handler::Ref &dbc_conn, bool user_connection) {
  db_mgmt_RdbmsRef rdbms = db_mgmt_RdbmsRef::cast_from(_connection->driver()->owner());
  SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms(rdbms);
  Sql_specifics::Ref sql_specifics = sql_facade->sqlSpecifics();

  // connection startup script
  {
    std::list<std::string> sql_script;
    {
      sql_specifics->get_connection_startup_script(sql_script);
      bool use_ansi_quotes = (connectionProperties->parameterValues().get_int("useAnsiQuotes", 0) != 0);
      if (use_ansi_quotes) {
        std::string sql = sql_specifics->setting_ansi_quotes();
        if (!sql.empty())
          sql_script.push_back(sql);
      }
    }

    // check if SQL_SAFE_UPDATES should be enabled (only for user connections, don't do it for the aux connection)
    if (bec::GRTManager::get()->get_app_option_int("DbSqlEditor:SafeUpdates", 1) && user_connection)
      sql_script.push_back("SET SQL_SAFE_UPDATES=1");

    std::unique_ptr<sql::Statement> stmt(dbc_conn_ref->createStatement());
    sql::SqlBatchExec sql_batch_exec;
    sql_batch_exec(stmt.get(), sql_script);

    if (!user_connection) {
      std::string sql_mode;
      if (get_session_variable(dbc_conn_ref, "sql_mode", sql_mode) && sql_mode.find("MYSQL40") != std::string::npos) {
        // MYSQL40 used CREATE TABLE ... TYPE=<engine> instead of ENGINE=<engine>, which is not supported by our reveng
        // code
        std::vector<std::string> options(base::split(sql_mode, ","));
        for (std::vector<std::string>::iterator i = options.begin(); i != options.end(); ++i) {
          if (*i == "MYSQL40") {
            options.erase(i);
            break;
          }
        }

        std::unique_ptr<sql::Statement> stmt(dbc_conn_ref->createStatement());
        std::string query = base::sqlstring("SET SESSION SQL_MODE=?", 0) << base::join(options, ",");
        stmt->execute(query);
      }
    }
  }

  // remember connection id
  {
    std::string query_connection_id = sql_specifics->query_connection_id();
    if (!query_connection_id.empty()) {
      std::unique_ptr<sql::Statement> stmt(dbc_conn_ref->createStatement());
      stmt->execute(query_connection_id);
      std::shared_ptr<sql::ResultSet> rs(stmt->getResultSet());
      rs->next();
      dbc_conn->id = rs->getInt(1);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void set_active_schema(SqlEditorForm::Ptr self, const std::string &schema) {
  SqlEditorForm::Ref ed(self.lock());
  if (ed)
    ed->active_schema(schema);
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::create_connection(sql::Dbc_connection_handler::Ref &dbc_conn, db_mgmt_ConnectionRef db_mgmt_conn,
                                      std::shared_ptr<SSHTunnel> tunnel, sql::Authentication::Ref auth,
                                      bool autocommit_mode, bool user_connection) {
  dbc_conn->is_stop_query_requested = false;

  sql::DriverManager *dbc_drv_man = sql::DriverManager::getDriverManager();

  db_mgmt_ConnectionRef temp_connection = db_mgmt_ConnectionRef::cast_from(grt::CopyContext().copy(db_mgmt_conn));

  int read_timeout = (int)bec::GRTManager::get()->get_app_option_int("DbSqlEditor:ReadTimeOut");

  if (read_timeout < 0) {
      bec::GRTManager::get()->set_app_option("DbSqlEditor:ReadTimeOut", grt::IntegerRef((int)0));
      read_timeout = 0;
  }

  temp_connection->parameterValues().set("OPT_READ_TIMEOUT", grt::IntegerRef(read_timeout));

  int connect_timeout = (int)bec::GRTManager::get()->get_app_option_int("DbSqlEditor:ConnectionTimeOut");
  if (connect_timeout > 0)
    temp_connection->parameterValues().set("OPT_CONNECT_TIMEOUT", grt::IntegerRef(connect_timeout));

  temp_connection->parameterValues().set("CLIENT_INTERACTIVE", grt::IntegerRef(1));

  try {
    dbc_conn->ref = dbc_drv_man->getConnection(temp_connection, tunnel, auth,
                                               std::bind(&SqlEditorForm::init_connection, this, std::placeholders::_1,
                                                         std::placeholders::_2, dbc_conn, user_connection));

    note_connection_open_outcome(0); // success
  } catch (sql::SQLException &exc) {
    note_connection_open_outcome(exc.getErrorCode());
    throw;
  }

  //! dbms-specific code
  if (dbc_conn->ref->getMetaData()->getDatabaseMajorVersion() < 5) {
    throw std::runtime_error("MySQL Server version is older than 5.x, which is not supported");
  }

  // get SSL enabled info
  {
    std::unique_ptr<sql::Statement> stmt(dbc_conn->ref->createStatement());
    std::unique_ptr<sql::ResultSet> result(stmt->executeQuery("SHOW SESSION STATUS LIKE 'Ssl_cipher'"));
    if (result->next()) {
      dbc_conn->ssl_cipher = result->getString(2);
    }
  }

  // Activate default schema, if it's empty, use last active
  if (dbc_conn->active_schema.empty()) {
    std::string default_schema = temp_connection->parameterValues().get_string("schema");

    if (default_schema.empty())
      default_schema = temp_connection->parameterValues().get_string("DbSqlEditor:LastDefaultSchema");
    if (!default_schema.empty()) {
      try {
        dbc_conn->ref->setSchema(default_schema);
        dbc_conn->active_schema = default_schema;

        bec::GRTManager::get()->run_once_when_idle(this,
                                                   std::bind(&set_active_schema, shared_from_this(), default_schema));
      } catch (std::exception &exc) {
        logError("Can't restore default schema (%s): %s\n", default_schema.c_str(), exc.what());
        temp_connection->parameterValues().gset("DbSqlEditor:LastDefaultSchema", "");
      }
    }
  } else
    dbc_conn->ref->setSchema((dbc_conn->active_schema));

  dbc_conn->ref->setAutoCommit(autocommit_mode);
  dbc_conn->autocommit_mode = dbc_conn->ref->getAutoCommit();
}

//----------------------------------------------------------------------------------------------------------------------

struct ConnectionErrorInfo {
  sql::AuthenticationError *auth_error;
  bool password_expired;
  bool server_probably_down;
  bool serverIsOffline;
  grt::server_denied *serverException;

  ConnectionErrorInfo()
    : auth_error(nullptr),
      password_expired(false),
      server_probably_down(false),
      serverIsOffline(false),
      serverException(nullptr) {
  }
  ~ConnectionErrorInfo() {
    delete auth_error;
    delete serverException;
  }
};

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::set_connection(db_mgmt_ConnectionRef conn) {
  if (_connection.is_valid())
    logWarning("Setting connection on an editor with a connection already set\n");

  _connection = conn;

  _dbc_auth = sql::Authentication::create(_connection, "");

  // initialize the password with a cached value
  {
    std::string password = conn->parameterValues().get_string("password");
    bool ok = true;
    if (password.empty()) {
      if (!mforms::Utilities::find_password(conn->hostIdentifier(), conn->parameterValues().get_string("userName"),
                                            password)) {
        if (!mforms::Utilities::find_cached_password(conn->hostIdentifier(),
                                                     conn->parameterValues().get_string("userName"), password)) {
          ok = false;
        }
      }
    }
    if (ok)
      _dbc_auth->set_password(password.c_str());
  }

  // send editor open notification again, in case the connection is being set after the connection
  // tab is opened. this will be caught by the admin code to init itself
  if (_startup_done)
    GRTNotificationCenter::get()->send_grt("GRNSQLEditorOpened", grtobj(), grt::DictRef());
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorForm::connect(std::shared_ptr<SSHTunnel> tunnel) {
  sql::Authentication::Ref auth = _dbc_auth; // sql::Authentication::create(_connection, "");
  enum PasswordMethod { NoPassword, KeychainPassword, InteractivePassword } current_method = NoPassword;

  reset();

  // In the 1st connection attempt, no password is supplied
  // If it fails, keychain is checked and used if it exists
  // If it fails, an interactive password request is made

  // connect
  for (;;) {
    // if an error happens in the worker thread, this ptr will be set
    ConnectionErrorInfo error_ptr;

    // connection must happen in the worker thread
    try {
      exec_sql_task->exec(true, std::bind(&SqlEditorForm::do_connect, this, tunnel, auth, &error_ptr));

      // check if user cancelled
      if (_cancel_connect) // return false, so it looks like the server is down
      {
        close();
        return false;
      }
    } catch (grt::grt_runtime_error &) {
      if (error_ptr.serverException != nullptr)
        throw grt::server_denied(*error_ptr.serverException);

      if (error_ptr.password_expired)
        throw std::runtime_error(":PASSWORD_EXPIRED");

      if (!error_ptr.auth_error)
        throw;
      else if (error_ptr.server_probably_down || error_ptr.serverIsOffline)
        return false;

      // check if user cancelled
      if (_cancel_connect) // return false, so it looks like the server is down
      {
        close();
        return false;
      }

      if (current_method == NoPassword) {
        // lookup in keychain
        std::string pwd;
        if (sql::DriverManager::getDriverManager()->findStoredPassword(auth->connectionProperties(), pwd)) {
          auth->set_password(pwd.c_str());
          current_method = KeychainPassword;
        } else {
          // not in keychain, go straight to interactive
          pwd = sql::DriverManager::getDriverManager()->requestPassword(auth->connectionProperties(), true);
          auth->set_password(pwd.c_str());
          current_method = InteractivePassword;
        }
      } else if (current_method == KeychainPassword) {
        // now try interactive
        std::string pwd = sql::DriverManager::getDriverManager()->requestPassword(auth->connectionProperties(), true);
        auth->set_password(pwd.c_str());
      } else // if interactive failed, pass the exception higher up to be displayed to the user
        throw;
      continue;
    }
    break;
  }

  // XXX: ouch, what if we ever change the init sequence, *nobody* will look here to note the side effect.
  // we should only send this after the initial connection
  // assumes setup_side_palette() is called in finish_init(), signalizing that the editor was already initialized once
  if (_side_palette) // we're in a thread here, so make sure the notification is sent from the main thread
  {
    bec::GRTManager::get()->run_once_when_idle(this, std::bind(&SqlEditorForm::update_connected_state, this));
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::update_connected_state() {
  grt::DictRef args(true);
  args.gset("connected", connected());
  GRTNotificationCenter::get()->send_grt("GRNSQLEditorReconnected", grtobj(), args);

  update_menu_and_toolbar();
}

//----------------------------------------------------------------------------------------------------------------------

std::string SqlEditorForm::get_client_lib_version() {
  std::string version;
  sql::DriverManager *dbc_driver_man = sql::DriverManager::getDriverManager();
  if (dbc_driver_man != nullptr)
    version = dbc_driver_man->getClientLibVersion();
  return version;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Formats a single text line for the connection info output.
 */
std::string createTableRow(const std::string &name, const std::string &value = "") {
  if (value.empty()) // Empty value means: heading row.
    return "<tr class='heading'>"s +
      "<td style='border:none; padding-left: 0px;' colspan=2>" + name + "</td>"
      "</tr>";

  return "<tr>"s +
    "<td style='border:none; padding-left: 15px;'>" + name + "</td>"
    "<td style='border:none;'>" + value + "</td>"
    "</tr>";

}

//----------------------------------------------------------------------------------------------------------------------

grt::StringRef SqlEditorForm::do_connect(std::shared_ptr<SSHTunnel> tunnel, sql::Authentication::Ref &auth,
                                         ConnectionErrorInfo *err_ptr) {
  try {
    RecMutexLock aux_dbc_conn_mutex(_aux_dbc_conn_mutex);
    RecMutexLock usr_dbc_conn_mutex(_usr_dbc_conn_mutex);

    _aux_dbc_conn->ref.reset();
    _usr_dbc_conn->ref.reset();

    _connection_details["name"] = _connection->name();
    _connection_details["hostName"] = _connection->parameterValues().get_string("hostName");
    _connection_details["port"] = strfmt("%li\n", (long)_connection->parameterValues().get_int("port"));

    _connection_details["socket"] = _connection->parameterValues().get_string("socket");
    _connection_details["driverName"] = _connection->driver()->name();
    _connection_details["userName"] = _connection->parameterValues().get_string("userName");

    // During the connection process create also a description about the connection details that can be shown
    // in the SQL IDE. Only the body content is created here. Surrounding code is added by the tree controller.
    // Start with the default content, in case we cannot connect.
    _connectionInfo = "<b><span style='color: red'>No connection established</span></b>";

    std::string newConnectionInfo = "<body><table style='border: none; border-collapse: collapse;'>" +
       createTableRow("Connection Details") + createTableRow("Name: ", _connection->name());

    _tunnel = tunnel;
    if (_tunnel == nullptr) {
      if (_connection->driver()->name() == "MysqlNativeSocket") {
      #ifdef _MSC_VER
        std::string name = _connection->parameterValues().get_string("socket", "");
        if (name.empty())
          name = "pipe";
      #else
        std::string name = _connection->parameterValues().get_string("socket", "");
        if (name.empty())
          name = "UNIX socket";
      #endif

        newConnectionInfo += createTableRow("Host:", "localhost (" + name + ")");
      } else {
        newConnectionInfo += createTableRow("Host:", _connection->parameterValues().get_string("hostName"));
        newConnectionInfo += createTableRow("Port:", std::to_string(_connection->parameterValues().get_int("port")));
      }
    }

    // open connections
    create_connection(_aux_dbc_conn, _connection, tunnel, auth, _aux_dbc_conn->autocommit_mode, false);
    create_connection(_usr_dbc_conn, _connection, tunnel, auth, _usr_dbc_conn->autocommit_mode, true);
    _serverIsOffline = false;
    cache_sql_mode();

    try {
      std::string value;
      get_session_variable(_usr_dbc_conn->ref.get(), "version_comment", value);
      _connection_details["dbmsProductName"] = value;
      get_session_variable(_usr_dbc_conn->ref.get(), "version", value);
      _connection_details["dbmsProductVersion"] = value;

      logInfo("Opened connection '%s' to %s version %s\n", _connection->name().c_str(),
        _connection_details["dbmsProductName"].c_str(), _connection_details["dbmsProductVersion"].c_str());

      _version = parse_version(_connection_details["dbmsProductVersion"]);
      _version->name(grt::StringRef(_connection_details["dbmsProductName"]));

      db_query_EditorRef editor(grtobj());
      if (editor.is_valid()) // this will be valid only on reconnections
        editor->serverVersion(_version);

      newConnectionInfo += createTableRow("Login User:", _connection->parameterValues().get_string("userName"));

      // check the actual user we're logged in as
      if (_usr_dbc_conn && _usr_dbc_conn->ref.get()) {
        const std::unique_ptr<sql::Statement> statement(_usr_dbc_conn->ref->createStatement());
        const std::unique_ptr<sql::ResultSet> rs(statement->executeQuery("SELECT current_user()"));
        if (rs->next())
          newConnectionInfo += createTableRow("Current User:", rs->getString(1));
      }

      newConnectionInfo += createTableRow(
        "SSL cipher:", _usr_dbc_conn->ssl_cipher.empty() ? "SSL not used" : _usr_dbc_conn->ssl_cipher);

      if (_tunnel != nullptr) {
        auto &config = _tunnel->getConfig();
        newConnectionInfo += createTableRow("SSH Tunnel");
        newConnectionInfo += createTableRow("Target:", config.remoteSSHhost + ":" + std::to_string(config.remoteSSHport));
        newConnectionInfo += createTableRow("Local port:", std::to_string(config.localport));
        newConnectionInfo += createTableRow("Remote port:", std::to_string(config.remoteport));
        newConnectionInfo += createTableRow("Remote host:", config.remotehost);
        newConnectionInfo += createTableRow("Config file:", config.configFile);
      }

      newConnectionInfo += createTableRow("Server");
      newConnectionInfo += createTableRow("Product:", _connection_details["dbmsProductName"]);
      newConnectionInfo += createTableRow("Version:", _connection_details["dbmsProductVersion"]);

      newConnectionInfo += createTableRow("Connector");
      newConnectionInfo += createTableRow("Version:", get_client_lib_version());

      if (_usr_dbc_conn && get_session_variable(_usr_dbc_conn->ref.get(), "lower_case_table_names", value))
        _lower_case_table_names = base::atoi<int>(value, 0);

      parsers::MySQLParserServices::Ref services = parsers::MySQLParserServices::get();
      _work_parser_context =
        services->createParserContext(rdbms()->characterSets(), _version, _sql_mode, _lower_case_table_names != 0);

      // If we came so far, we probably have a valid connection.
      _connectionInfo = newConnectionInfo + "</table>";
    }
    CATCH_ANY_EXCEPTION_AND_DISPATCH(_("Get connection information"));
  } catch (sql::AuthenticationError &authException) {
    err_ptr->auth_error = new sql::AuthenticationError(authException);
    throw;
  } catch (sql::SQLException &exc) {
    logException("SqlEditorForm: exception in do_connect method", exc);

    _version = bec::intToVersion(50717); // Set a meaningful default version if we cannot open a connection.

    switch (exc.getErrorCode()) {
      case 1820: // ER_MUST_CHANGE_PASSWORD_LOGIN
        err_ptr->password_expired = true;
        break;

      case 2013:
      case 2003:
      case 2002: { // ERROR 2003 (HY000): Can't connect to MySQL server on X.Y.Z.W (or via socket)
        add_log_message(WarningMsg, exc.what(), "Could not connect, server may not be running.", "");

        err_ptr->server_probably_down = true;

        if (_connection.is_valid()) {
          // if there's no connection, then we continue anyway if this is a local connection or
          // a remote connection with remote admin enabled..
          grt::Module *m = grt::GRT::get()->get_module("WbAdmin");
          grt::BaseListRef args(true);
          args.ginsert(_connection);
          if (!m || *grt::IntegerRef::cast_from(m->call_function("checkConnectionForRemoteAdmin", args)) == 0) {
            logError("Connection failed but remote admin does not seem to be available, rethrowing exception...\n");
            throw;
          }
          logInfo("Error %i connecting to server, assuming server is down and opening editor with no connection\n",
                  exc.getErrorCode());
        }

        logInfo("Error %i connecting to server, assuming server is down and opening editor with no connection\n",
                exc.getErrorCode());

        // Create a parser with some sensible defaults if we cannot connect.
        // We specify no charsets here, disabling parsing of repertoires.
        parsers::MySQLParserServices::Ref services = parsers::MySQLParserServices::get();
        _work_parser_context = services->createParserContext(GrtCharacterSetsRef(true), _version, _sql_mode, true);
        return grt::StringRef();
      }

      case 3032: {
        err_ptr->serverIsOffline = true;
        _serverIsOffline = true;
        add_log_message(WarningMsg, exc.what(), "Could not connect, server is in offline mode.", "");

        if (_connection.is_valid()) {
          // if there's no connection, then we continue anyway if this is a local connection or
          // a remote connection with remote admin enabled..
          grt::GRT::get()->get_module("WbAdmin");
          grt::BaseListRef args(true);
          args.ginsert(_connection);
        }

        logInfo("Error %i connecting to server, server is in offline mode. Only superuser connection are allowed. "
                "Opening editor with no connection\n", exc.getErrorCode());

        // Create a parser with some sensible defaults if we cannot connect.
        // We specify no charsets here, disabling parsing of repertoires.
        parsers::MySQLParserServices::Ref services = parsers::MySQLParserServices::get();
        _work_parser_context = services->createParserContext(GrtCharacterSetsRef(true), _version, _sql_mode, true);

        return grt::StringRef();
      }

      case 3159: {
        // require SSL, offline mode
        // we need to change exception type so we can properly handle it in
        err_ptr->serverException = new grt::server_denied(exc.what(), exc.getErrorCode());
        break;
      }
    }

    _connectionInfo += "</body>";
    throw;
  }

  _connectionInfo += "</body>";
  
  return grt::StringRef();
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorForm::connected() const {
  // If the conn mutex is locked by someone else, then we assume the conn is in use and thus,
  // there's a connection.
  bool busy = !_usr_dbc_conn_mutex.tryLock();
  if (!busy)
    _usr_dbc_conn_mutex.unlock();
  if (_usr_dbc_conn && (busy || _usr_dbc_conn->ref.get_ptr()))
    return true; // we don't need to PING the server every time we want to check if the editor is connected
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::checkIfOffline() {
  bool locked = _usr_dbc_conn_mutex.tryLock();
  size_t counter = 1;
  while (!locked) {
    if (counter >= 30) {
      logError("Can't lock conn mutex for 30 seconds, assuming server is not offline.\n");
      break;
    }

    logDebug3("Can't lock connection mutex, trying again in one sec.\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    counter++;
    locked = _usr_dbc_conn_mutex.tryLock();
  }

  if (locked) {
    std::string result;
    if (_usr_dbc_conn && get_session_variable(_usr_dbc_conn->ref.get(), "offline_mode", result)) {
      if (base::string_compare(result, "ON") == 0)
        _serverIsOffline = true;
    }

    _usr_dbc_conn_mutex.unlock();
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorForm::offline() {
  if (_serverIsOffline)
    return true;

  if (!connected())
    return false;

  return _serverIsOffline;
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorForm::ping() const {
  // If the conn mutex is locked by someone else, then we assume the conn is in use and thus,
  // there's a connection.
  bool locked = _usr_dbc_conn_mutex.tryLock();
  if (!locked)
    return true;

  if (_usr_dbc_conn && _usr_dbc_conn->ref.get_ptr()) {
    std::unique_ptr<sql::Statement> stmt(_usr_dbc_conn->ref->createStatement());
    try {
      stmt->execute("do 1");
      _usr_dbc_conn_mutex.unlock();
      return true;
    } catch (const std::exception &ex) {
      logError("Failed to ping the server: %s\n", ex.what());
    }
  }

  _usr_dbc_conn_mutex.unlock();

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

base::RecMutexLock SqlEditorForm::ensure_valid_aux_connection(sql::Dbc_connection_handler::Ref &conn, bool lockOnly) {
  RecMutexLock lock(ensure_valid_dbc_connection(_aux_dbc_conn, _aux_dbc_conn_mutex, lockOnly));
  conn = _aux_dbc_conn;
  return lock;
}

//----------------------------------------------------------------------------------------------------------------------

RecMutexLock SqlEditorForm::ensure_valid_aux_connection(bool throw_on_block, bool lockOnly) {
  return ensure_valid_dbc_connection(_aux_dbc_conn, _aux_dbc_conn_mutex, throw_on_block, lockOnly);
}

//----------------------------------------------------------------------------------------------------------------------

RecMutexLock SqlEditorForm::ensure_valid_usr_connection(bool throw_on_block, bool lockOnly) {
  return ensure_valid_dbc_connection(_usr_dbc_conn, _usr_dbc_conn_mutex, throw_on_block, lockOnly);
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::close_connection(sql::Dbc_connection_handler::Ref &dbc_conn) {
  sql::Dbc_connection_handler::Ref myref(dbc_conn);
  if (dbc_conn && dbc_conn->ref.get_ptr()) {
    try {
      dbc_conn->ref->close();
    } catch (sql::SQLException &) {
      // ignore if the connection is already closed
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

RecMutexLock SqlEditorForm::ensure_valid_dbc_connection(sql::Dbc_connection_handler::Ref &dbc_conn,
                                                        base::RecMutex &dbc_conn_mutex, bool throw_on_block,
                                                        bool lockOnly) {
  RecMutexLock mutex_lock(dbc_conn_mutex, throw_on_block);
  bool valid = false;

  sql::Dbc_connection_handler::Ref myref(dbc_conn);
  if (dbc_conn && dbc_conn->ref.get_ptr()) {
    if (lockOnly) // this is a special case, we need it in some situations like for example recordset_cdbc
      return mutex_lock;

    try {
      // use connector::isValid to check if server connection is valid
      // this will also ping the server and reconnect if needed
      valid = dbc_conn->ref->isValid();
    } catch (std::exception &exc) {
      logError("CppConn::isValid exception: %s", exc.what());
      valid = false;
    }
    if (!valid) {
      bool user_connection = _usr_dbc_conn ? dbc_conn->ref.get_ptr() == _usr_dbc_conn->ref.get_ptr() : false;

      if (dbc_conn->autocommit_mode) {
        sql::AuthenticationSet authset;
        std::shared_ptr<SSHTunnel> tunnel = sql::DriverManager::getDriverManager()->getTunnel(_connection);

        create_connection(dbc_conn, _connection, tunnel, sql::Authentication::Ref(), dbc_conn->autocommit_mode,
                          user_connection);
        if (!dbc_conn->ref->isClosed())
          valid = true;
      }
    } else
      valid = true;
  }
  if (!valid)
    throw grt::db_not_connected("DBMS connection is not available");

  return mutex_lock;
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorForm::auto_commit() {
  if (_usr_dbc_conn)
    return _usr_dbc_conn->autocommit_mode;
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::auto_commit(bool value) {
  if (!_usr_dbc_conn)
    return;
  {
    const char *STATEMENT = value ? "AUTOCOMMIT=1" : "AUTOCOMMIT=0";
    try {
      RecMutexLock usr_dbc_conn_mutex = ensure_valid_usr_connection();
      _usr_dbc_conn->ref->setAutoCommit(value);
      _usr_dbc_conn->autocommit_mode = _usr_dbc_conn->ref->getAutoCommit();
    }
    CATCH_ANY_EXCEPTION_AND_DISPATCH(STATEMENT)
  }
  update_menu_and_toolbar();
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::toggle_autocommit() {
  auto_commit(!auto_commit());
  update_menu_and_toolbar();
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::toggle_collect_field_info() {
  if (_connection.is_valid())
    _connection->parameterValues().set("CollectFieldMetadata", grt::IntegerRef(collect_field_info() ? 0 : 1));
  update_menu_and_toolbar();
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorForm::collect_field_info() const {
  if (_connection.is_valid())
    return _connection->parameterValues().get_int("CollectFieldMetadata", 1) != 0;
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::toggle_collect_ps_statement_events() {
  if (_connection.is_valid())
    _connection->parameterValues().set("CollectPerfSchemaStatsForQueries",
                                       grt::IntegerRef(collect_ps_statement_events() ? 0 : 1));
  update_menu_and_toolbar();
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorForm::collect_ps_statement_events() const {
  if (_connection.is_valid() && is_supported_mysql_version_at_least(rdbms_version(), 5, 6))
    return _connection->parameterValues().get_int("CollectPerfSchemaStatsForQueries", 1) != 0;
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::cancel_query() {
  std::string query_kill_query;
  {
    db_mgmt_RdbmsRef rdbms = db_mgmt_RdbmsRef::cast_from(_connection->driver()->owner());
    SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms(rdbms);
    Sql_specifics::Ref sql_specifics = sql_facade->sqlSpecifics();
    query_kill_query = sql_specifics->query_kill_query(_usr_dbc_conn->id);
  }
  if (query_kill_query.empty())
    return;

  const char *STATEMENT = "INTERRUPT";
  RowId log_message_index = add_log_message(DbSqlEditorLog::BusyMsg, _("Running..."), STATEMENT, "");
  Timer timer(false);

  try {
    {
      RecMutexLock aux_dbc_conn_mutex(ensure_valid_aux_connection());
      std::unique_ptr<sql::Statement> stmt(_aux_dbc_conn->ref->createStatement());
      {
        base::ScopeExitTrigger schedule_timer_stop(std::bind(&Timer::stop, &timer));
        timer.run();
        stmt->execute(query_kill_query);

        // this can potentially cause threading issues, since connector driver isn't thread-safe
        // close_connection(_usr_dbc_conn);

        // connection drop doesn't interrupt fetching stage (surprisingly)
        // to workaround that we set special flag and check it periodically during fetching
        _usr_dbc_conn->is_stop_query_requested = is_running_query();
      }
    }

    if (_usr_dbc_conn->is_stop_query_requested) {
      bec::GRTManager::get()->replace_status_text("Query Cancelled");
      set_log_message(log_message_index, DbSqlEditorLog::NoteMsg, _("OK - Query cancelled"), STATEMENT,
                      timer.duration_formatted());
    } else
      set_log_message(log_message_index, DbSqlEditorLog::NoteMsg, _("OK - Query already completed"), STATEMENT,
                      timer.duration_formatted());

    // reconnect but only if in autocommit mode
    if (_usr_dbc_conn->autocommit_mode) {
      // this will restore connection if it was established previously
      exec_sql_task->execute_in_main_thread(std::bind(&SqlEditorForm::send_message_keep_alive, this), false, true);
    }
  }
  CATCH_SQL_EXCEPTION_AND_DISPATCH(STATEMENT, log_message_index, "")
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::commit() {
  exec_sql_retaining_editor_contents("COMMIT", nullptr, false);
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::rollback() {
  exec_sql_retaining_editor_contents("ROLLBACK", nullptr, false);
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::explain_current_statement() {
  SqlEditorPanel *panel = active_sql_editor_panel();
  if (panel) {
    SqlEditorResult *result = panel->add_panel_for_recordset(Recordset::Ref());
    result->set_title("Explain");

    grt::BaseListRef args(true);
    args.ginsert(panel->grtobj());
    args.ginsert(result->grtobj());
    // run the visual explain plugin, so it will fill the result panel
    grt::GRT::get()->call_module_function("SQLIDEQueryAnalysis", "visualExplain", args);
  }
}

//----------------------------------------------------------------------------------------------------------------------

// Should actually be called _retaining_old_recordsets
void SqlEditorForm::exec_sql_retaining_editor_contents(const std::string &sql_script, SqlEditorPanel *editor, bool sync,
                                                       bool dont_add_limit_clause) {
  auto_save();

  if (!connected())
    throw grt::db_not_connected("Not connected");

  if (editor) {
    editor->query_started(true);
    exec_sql_task->finish_cb(std::bind(&SqlEditorPanel::query_finished, editor), true);
    exec_sql_task->fail_cb(std::bind(&SqlEditorPanel::query_failed, editor, std::placeholders::_1), true);
  }

  exec_sql_task->exec(sync, std::bind(&SqlEditorForm::do_exec_sql, this, weak_ptr_from(this),
                                      std::shared_ptr<std::string>(new std::string(sql_script)), editor,
                                      (ExecFlags)(dont_add_limit_clause ? DontAddLimitClause : 0), RecordsetsRef()));
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::run_editor_contents(bool current_statement_only) {
  SqlEditorPanel *panel(active_sql_editor_panel());
  if (panel) {
    exec_editor_sql(panel, false, current_statement_only, current_statement_only);
  }
}

//----------------------------------------------------------------------------------------------------------------------

RecordsetsRef SqlEditorForm::exec_sql_returning_results(const std::string &sql_script, bool dont_add_limit_clause) {
  if (!connected())
    throw grt::db_not_connected("Not connected");

  RecordsetsRef rsets(new Recordsets());

  do_exec_sql(weak_ptr_from(this), std::shared_ptr<std::string>(new std::string(sql_script)), nullptr,
              (ExecFlags)(dont_add_limit_clause ? DontAddLimitClause : 0), rsets);

  return rsets;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Runs the current content of the given editor on the target server and returns true if the query
 * was actually started (useful for the platform layers to show a busy animation).
 *
 * @param editor The editor whose content is to be executed.
 * @param sync If true wait for completion.
 * @param current_statement_only If true then only the statement where the cursor is in is executed.
 *                               Otherwise the current selection is executed (if there is one) or
 *                               the entire editor content.
 * @param use_non_std_delimiter If true the code is wrapped with a non standard delimiter to
 *                                    allow running the sql regardless of the delimiters used by the
 *                                    user (e.g. for view/sp definitions).
 * @param dont_add_limit_clause If true the automatic addition of the LIMIT clause is suppressed, which
 *                              is used to limit on the number of return rows (avoid huge result sets
 *                              by accident).
 * @param into_result If not nullptr, the resultset grid will be displayed inside it, instead of creating
 *                     a new one in editor. The query/script must return at most one recordset.
 */
bool SqlEditorForm::exec_editor_sql(SqlEditorPanel *editor, bool sync, bool current_statement_only,
                                    bool use_non_std_delimiter, bool dont_add_limit_clause,
                                    SqlEditorResult *into_result) {

  logDebug("Executing SQL in editor: %s (current statement only: %s)...\n", editor->get_title().c_str(),
           current_statement_only ? "yes" : "no");

  std::shared_ptr<std::string> shared_sql;
  if (current_statement_only)
    shared_sql.reset(new std::string(editor->editor_be()->current_statement()));
  else {
    std::string sql = editor->editor_be()->selected_text();
    if (sql.empty()) {
      std::pair<const char *, std::size_t> text = editor->text_data();
      shared_sql.reset(new std::string(text.first, text.second));
    } else
      shared_sql.reset(new std::string(sql));
  }

  if (shared_sql->empty())
    return false;

  ExecFlags flags = (ExecFlags)0;

  if (use_non_std_delimiter)
    flags = (ExecFlags)(flags | NeedNonStdDelimiter);
  if (dont_add_limit_clause)
    flags = (ExecFlags)(flags | DontAddLimitClause);
  if (bec::GRTManager::get()->get_app_option_int("DbSqlEditor:ShowWarnings", 1))
    flags = (ExecFlags)(flags | ShowWarnings);
  auto_save();

  // If we're filling an already existing result panel, we shouldn't close the old result sets.
  editor->query_started(into_result ? true : false);
  exec_sql_task->finish_cb(std::bind(&SqlEditorPanel::query_finished, editor), true);
  exec_sql_task->fail_cb(std::bind(&SqlEditorPanel::query_failed, editor, std::placeholders::_1), true);

  if (into_result) {
    logDebug2("Running into existing rsets\n");

    RecordsetsRef rsets(new Recordsets());

    exec_sql_task->exec(sync, std::bind(&SqlEditorForm::do_exec_sql, this, weak_ptr_from(this), shared_sql,
                                        (SqlEditorPanel *)nullptr, flags, rsets));

    if (rsets->size() > 1)
      logError("Statement returns too many resultsets\n");
    if (!rsets->empty())
      into_result->set_recordset((*rsets)[0]);
  } else {
    logDebug2("Running without considering existing rsets\n");

    exec_sql_task->exec(sync, std::bind(&SqlEditorForm::do_exec_sql, this, weak_ptr_from(this), shared_sql, editor,
                                        flags, RecordsetsRef()));
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::update_live_schema_tree(const std::string &sql) {
  bec::GRTManager::get()->run_once_when_idle(this, std::bind(&SqlEditorForm::handle_command_side_effects, this, sql));
}

//----------------------------------------------------------------------------------------------------------------------

grt::StringRef SqlEditorForm::do_exec_sql(Ptr self_ptr, std::shared_ptr<std::string> sql, SqlEditorPanel *editor,
                                          ExecFlags flags, RecordsetsRef result_list) {

  logDebug("Background task for sql execution started\n");

  bool use_non_std_delimiter = (flags & NeedNonStdDelimiter) != 0;
  bool dont_add_limit_clause = (flags & DontAddLimitClause) != 0;
  std::map<std::string, std::int64_t> ps_stats;
  std::vector<PSStage> ps_stages;
  std::vector<PSWait> ps_waits;
  bool query_ps_stats = collect_ps_statement_events();
  std::string query_ps_statement_events_error;
  std::string statement;
  int max_query_size_to_log = (int)bec::GRTManager::get()->get_app_option_int("DbSqlEditor:MaxQuerySizeToHistory", 0);
  int limit_rows = 0;
  if (bec::GRTManager::get()->get_app_option_int("SqlEditor:LimitRows") != 0)
    limit_rows = (int)bec::GRTManager::get()->get_app_option_int("SqlEditor:LimitRowsCount", 0);

  bec::GRTManager::get()->replace_status_text(_("Executing Query..."));

  std::shared_ptr<SqlEditorForm> self_ref = (self_ptr).lock();
  SqlEditorForm *self = (self_ref).get();
  if (!self) {
    logError("Couldn't aquire lock for SQL editor form\n");
    return grt::StringRef("");
  }

  // add_log_message() will increment this variable on errors or warnings
  _exec_sql_error_count = 0;

  bool interrupted = true;
  sql::Driver *dbc_driver = nullptr;
  try {
    RecMutexLock use_dbc_conn_mutex(ensure_valid_usr_connection());

    dbc_driver = _usr_dbc_conn->ref->getDriver();
    dbc_driver->threadInit();

    bool is_running_query = true;
    AutoSwap<bool> is_running_query_keeper(_is_running_query, is_running_query);
    update_menu_and_toolbar();

    _has_pending_log_messages = false;
    base::ScopeExitTrigger schedule_log_messages_refresh(std::bind(&SqlEditorForm::refresh_log_messages, this, true));

    SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms(rdbms());
    Sql_syntax_check::Ref sql_syntax_check = sql_facade->sqlSyntaxCheck();
    Sql_specifics::Ref sql_specifics = sql_facade->sqlSpecifics();

    bool ran_set_sql_mode = false;
    bool logging_queries;
    std::vector<std::pair<std::size_t, std::size_t>> statement_ranges;
    sql_facade->splitSqlScript(sql->c_str(), sql->size(),
                               use_non_std_delimiter ? sql_specifics->non_std_sql_delimiter() : ";", statement_ranges);

    if (statement_ranges.size() > 1) {
      query_ps_stats = false;
      query_ps_statement_events_error = "Query stats can only be fetched when a single statement is executed.";
    }

    if (!max_query_size_to_log || max_query_size_to_log >= (int)sql->size()) {
      logging_queries = true;
    } else {
      std::list<std::string> warning;

      warning.push_back(base::strfmt("Skipping history entries for %li statements, total %li bytes",
                                     (long)statement_ranges.size(), (long)sql->size()));
      _history->add_entry(warning);
      logging_queries = false;
    }

    // Intentionally allow any value. For values <= 0 show no result set at all.
    ssize_t max_resultset_count = bec::GRTManager::get()->get_app_option_int("DbSqlEditor::MaxResultsets", 50);
    ssize_t total_result_count = (editor != nullptr) ? editor->resultset_count() : 0; // Consider pinned result sets.

    bool results_left = false;
    for (auto &statement_range : statement_ranges) {
      logDebug3("Executing statement range: %lu, %lu...\n", statement_range.first, statement_range.second);

      statement = sql->substr(statement_range.first, statement_range.second);
      std::list<std::string> sub_statements;
      sql_facade->splitSqlScript(statement, sub_statements);
      std::size_t multiple_statement_count = sub_statements.size();
      bool is_multiple_statement = (1 < multiple_statement_count);

      {
        statement = strip_text(statement, false, true);
        if (statement.empty())
          continue;

        Sql_syntax_check::Statement_type statement_type = sql_syntax_check->determine_statement_type(statement);

        logDebug3("Determined statement type: %u\n", statement_type);
        if (Sql_syntax_check::sql_empty == statement_type)
          continue;

        std::string schema_name;
        std::string table_name;

        if (logging_queries) {
          std::list<std::string> statements;
          statements.push_back(statement);
          _history->add_entry(statements);
        }

        Recordset_cdbc_storage::Ref data_storage;

        // for select queries add limit clause if specified by global option
        if (!is_multiple_statement && (Sql_syntax_check::sql_select == statement_type)) {
          data_storage = Recordset_cdbc_storage::create();
          data_storage->set_gather_field_info(true);
          data_storage->rdbms(rdbms());
          data_storage->setUserConnectionGetter(
            std::bind(&SqlEditorForm::getUserConnection, this, std::placeholders::_1, std::placeholders::_2));
          data_storage->setAuxConnectionGetter(
            std::bind(&SqlEditorForm::getAuxConnection, this, std::placeholders::_1, std::placeholders::_2));

          SqlFacade::String_tuple_list column_names;

          if (!table_name.empty() ||
              sql_facade->parseSelectStatementForEdit(statement, schema_name, table_name, column_names)) {
            data_storage->schema_name(schema_name.empty() ? _usr_dbc_conn->active_schema : schema_name);
            data_storage->table_name(table_name);
            logDebug3("Result will be editable\n");
          } else {
            data_storage->readonly_reason(
              "Statement must be a SELECT for columns of a single table with a primary key for its results to be "
              "editable.");
            logDebug3("Result will not be editable\n");
          }

          data_storage->sql_query(statement);

          {
            bool do_limit = !dont_add_limit_clause && limit_rows > 0;
            data_storage->limit_rows(do_limit);

            if (limit_rows > 0)
              data_storage->limit_rows_count(limit_rows);
          }
          statement = data_storage->decorated_sql_query();
        }

        {
          RowId log_message_index = add_log_message(DbSqlEditorLog::BusyMsg, _("Running..."), statement,
                                                    ((Sql_syntax_check::sql_select == statement_type) ? "? / ?" : "?"));

          bool statement_failed = false;
          long long updated_rows_count = -1;
          Timer statement_exec_timer(false);
          Timer statement_fetch_timer(false);
          std::shared_ptr<sql::Statement> dbc_statement(_usr_dbc_conn->ref->createStatement());
          bool is_result_set_first = false;

          if (_usr_dbc_conn->is_stop_query_requested)
            throw std::runtime_error(
              _("Query execution has been stopped, the connection to the DB server was not restarted, any open "
                "transaction remains open"));

          try {
            {
              base::ScopeExitTrigger schedule_statement_exec_timer_stop(std::bind(&Timer::stop, &statement_exec_timer));
              statement_exec_timer.run();
              is_result_set_first = dbc_statement->execute(statement);
            }
            logDebug3("Query executed successfully\n");

            updated_rows_count = dbc_statement->getUpdateCount();

            // XXX: coalesce all the special queries here and act on them *after* all queries have run.
            // Especially the drop command is redirected twice to idle tasks, kicking so in totally asynchronously
            // and killing any intermittent USE commands.
            // Updating the UI during a run of many commands is not useful either.
            if (Sql_syntax_check::sql_use == statement_type)
              cache_active_schema_name();
            if (Sql_syntax_check::sql_set == statement_type && statement.find("@sql_mode") != std::string::npos)
              ran_set_sql_mode = true;
            if (Sql_syntax_check::sql_drop == statement_type)
              update_live_schema_tree(statement);
          } catch (sql::SQLException &e) {
            std::string err_msg;
            // safe mode
            switch (e.getErrorCode()) {
              case 1046: // not default DB selected
                err_msg = strfmt(_("Error Code: %i. %s\nSelect the default DB to be used by double-clicking its name "
                                   "in the SCHEMAS list in the sidebar."),
                                 e.getErrorCode(), e.what());
                break;
              case 1175: // safe mode
                err_msg = strfmt(_("Error Code: %i. %s\nTo disable safe mode, toggle the option in Preferences -> SQL "
                                   "Editor and reconnect."),
                                 e.getErrorCode(), e.what());
                break;
              default:
                err_msg = strfmt(_("Error Code: %i. %s"), e.getErrorCode(), e.what());
                break;
            }
            set_log_message(log_message_index, DbSqlEditorLog::ErrorMsg, err_msg, statement,
                            statement_exec_timer.duration_formatted());
            statement_failed = true;
          } catch (std::exception &e) {
            std::string err_msg = strfmt(_("Error: %s"), e.what());
            set_log_message(log_message_index, DbSqlEditorLog::ErrorMsg, err_msg, statement,
                            statement_exec_timer.duration_formatted());
            statement_failed = true;
          }
          if (statement_failed) {
            if (_continueOnError)
              continue; // goto next statement
            else
              goto stop_processing_sql_script;
          }

          sql::mysql::MySQL_Connection *mysql_connection =
            dynamic_cast<sql::mysql::MySQL_Connection *>(dbc_statement->getConnection());
          sql::SQLString last_statement_info;
          if (mysql_connection != nullptr)
            last_statement_info = mysql_connection->getLastStatementInfo();
          if (updated_rows_count >= 0) {
            std::string message = strfmt(_("%lli row(s) affected"), updated_rows_count);
            bool has_warning = false;
            if (flags & ShowWarnings) {
              std::string warnings_message;
              const sql::SQLWarning *warnings = dbc_statement->getWarnings();
              if (warnings) {
                int count = 0;
                const sql::SQLWarning *w = warnings;
                while (w) {
                  warnings_message.append(strfmt("\n%i %s", w->getErrorCode(), w->getMessage().c_str()));
                  count++;
                  w = w->getNextWarning();
                }
                message.append(strfmt(_(", %i warning(s):"), count));
                has_warning = true;
              }
              if (!warnings_message.empty())
                message.append(warnings_message);
            }
            if (!last_statement_info->empty())
              message.append("\n").append(last_statement_info);
            set_log_message(log_message_index, has_warning ? DbSqlEditorLog::WarningMsg : DbSqlEditorLog::OKMsg,
                            message, statement, statement_exec_timer.duration_formatted());
          }

          logDebug2("Processing result sets\n");
          int resultset_count = 0;
          bool more_results = is_result_set_first;
          bool reuse_log_msg = false;
          if ((updated_rows_count < 0) || is_multiple_statement) {
            for (std::size_t processed_substatements_count = 0;
                 processed_substatements_count < multiple_statement_count; ++processed_substatements_count) {
              do {
                if (more_results) {
                  if (total_result_count == max_resultset_count) {
                    int result = mforms::Utilities::show_warning(
                      _("Maximum result count reached"),
                      "No further result tabs will be displayed as the maximm number has been reached. \nYou may stop "
                      "the operation, leaving the connection out of sync. You'll have to got o 'Query->Reconnect to "
                      "server' menu item to reset the state.\n\n Do you want to cancel the operation?",
                      "Yes", "No");
                    if (result == mforms::ResultOk) {
                      add_log_message(DbSqlEditorLog::ErrorMsg,
                                      "No more results could be displayed. Operation canceled by user.", statement,
                                      "");
                      dbc_statement->cancel();
                      dbc_statement->close();
                      return grt::StringRef("");
                    }
                    add_log_message(
                      DbSqlEditorLog::WarningMsg,
                      "No more results will be displayed because the maximum number of result sets was reached.",
                      statement, "");
                  }

                  if (!reuse_log_msg && ((updated_rows_count >= 0) || (resultset_count)))
                    log_message_index = add_log_message(DbSqlEditorLog::BusyMsg, _("Fetching..."), statement, "- / ?");
                  else
                    set_log_message(log_message_index, DbSqlEditorLog::BusyMsg, _("Fetching..."), statement,
                                    statement_exec_timer.duration_formatted() + " / ?");
                  reuse_log_msg = false;
                  std::shared_ptr<sql::ResultSet> dbc_resultset;
                  {
                    base::ScopeExitTrigger schedule_statement_fetch_timer_stop(
                      std::bind(&Timer::stop, &statement_fetch_timer));
                    statement_fetch_timer.run();

                    // need a separate exception catcher here, because sometimes a query error
                    // will only throw an exception after fetching starts, which causes the busy spinner
                    // to be active forever, since the exception is logged in a new log_id/row
                    // XXX this could also be caused by a bug in Connector/C++
                    try {
                      dbc_resultset.reset(dbc_statement->getResultSet());
                    } catch (sql::SQLException &e) {
                      std::string err_msg;
                      // safe mode
                      switch (e.getErrorCode()) {
                        case 1046: // not default DB selected
                          err_msg = strfmt(_("Error Code: %i. %s\nSelect the default DB to be used by double-clicking "
                                             "its name in the SCHEMAS list in the sidebar."),
                                           e.getErrorCode(), e.what());
                          break;
                        case 1175: // safe mode
                          err_msg = strfmt(_("Error Code: %i. %s\nTo disable safe mode, toggle the option in "
                                             "Preferences -> SQL Editor and reconnect."),
                                           e.getErrorCode(), e.what());
                          break;
                        default:
                          err_msg = strfmt(_("Error Code: %i. %s"), e.getErrorCode(), e.what());
                          break;
                      }

                      set_log_message(log_message_index, DbSqlEditorLog::ErrorMsg, err_msg, statement,
                                      statement_exec_timer.duration_formatted());

                      if (_continueOnError)
                        continue; // goto next statement
                      else
                        goto stop_processing_sql_script;
                    }
                  }

                  std::string exec_and_fetch_durations =
                    (((updated_rows_count >= 0) || (resultset_count)) ? std::string("-")
                                                                      : statement_exec_timer.duration_formatted()) +
                    " / " + statement_fetch_timer.duration_formatted();
                  if (total_result_count >= max_resultset_count)
                    set_log_message(log_message_index, DbSqlEditorLog::OKMsg, "Row count could not be verified",
                                    statement, exec_and_fetch_durations);
                  else if (dbc_resultset) {
                    if (!data_storage) {
                      data_storage = Recordset_cdbc_storage::create();
                      data_storage->set_gather_field_info(true);
                      data_storage->rdbms(rdbms());
                      data_storage->setUserConnectionGetter(std::bind(&SqlEditorForm::getUserConnection, this,
                                                                      std::placeholders::_1, std::placeholders::_2));
                      data_storage->setAuxConnectionGetter(std::bind(&SqlEditorForm::getAuxConnection, this,
                                                                     std::placeholders::_1, std::placeholders::_2));
                      if (table_name.empty())
                        data_storage->sql_query(statement);
                      data_storage->schema_name(schema_name);
                      data_storage->table_name(table_name);
                    }

                    data_storage->dbc_statement(dbc_statement);
                    data_storage->dbc_resultset(dbc_resultset);
                    data_storage->reloadable(!is_multiple_statement &&
                                             (Sql_syntax_check::sql_select == statement_type));

                    logDebug3("Creation and setup of a new result set...\n");

                    Recordset::Ref rs = Recordset::create(exec_sql_task);
                    rs->is_field_value_truncation_enabled(true);
                    rs->setPreserveRowFilter(
                      bec::GRTManager::get()->get_app_option_int("SqlEditor:PreserveRowFilter") == 1);
                    rs->apply_changes_cb =
                      std::bind(&SqlEditorForm::apply_changes_to_recordset, this, Recordset::Ptr(rs));
                    rs->generator_query(statement);

                    {
                      if (query_ps_stats) {
                        query_ps_statistics(_usr_dbc_conn->id, ps_stats);
                        ps_stages = query_ps_stages(ps_stats["EVENT_ID"]);
                        ps_waits = query_ps_waits(ps_stats["EVENT_ID"]);
                        query_ps_stats = false;
                      }
                      RecordsetData *rdata = new RecordsetData();
                      rdata->duration = statement_exec_timer.duration();
                      rdata->ps_stat_error = query_ps_statement_events_error;
                      rdata->ps_stat_info = ps_stats;
                      rdata->ps_stage_info = ps_stages;
                      rdata->ps_wait_info = ps_waits;
                      rs->set_client_data(rdata);
                    }

                    rs->data_storage(data_storage);
                    rs->reset(true);

                    if (data_storage->valid()) // query statement
                    {
                      if (result_list)
                        result_list->push_back(rs);

                      if (editor)
                        editor->add_panel_for_recordset_from_main(rs);

                      std::string statement_res_msg = std::to_string(rs->row_count()) + _(" row(s) returned");
                      if (!last_statement_info->empty())
                        statement_res_msg.append("\n").append(last_statement_info);

                      set_log_message(log_message_index, DbSqlEditorLog::OKMsg, statement_res_msg, statement,
                                      exec_and_fetch_durations);
                    }
                    ++resultset_count;
                  } else {
                    reuse_log_msg = true;
                  }
                  ++total_result_count;
                  data_storage.reset();
                }
              } while ((more_results = dbc_statement->getMoreResults()));
            }
          }

          if ((updated_rows_count < 0) && !(resultset_count))
            set_log_message(log_message_index, DbSqlEditorLog::OKMsg, _("OK"), statement,
                            statement_exec_timer.duration_formatted());
        }
      }
    } // statement range loop

    if (results_left) {
      exec_sql_task->execute_in_main_thread(
        std::bind(&mforms::Utilities::show_warning, _("Result set limit reached"),
                  _("There were more results than "
                    "result tabs could be opened, because the set maximum limit was reached. You can change this "
                    "limit in the preferences."),
                  _("OK"), "", ""),
        true, false);
    }

    bec::GRTManager::get()->replace_status_text(_("Query Completed"));
    interrupted = false;

  stop_processing_sql_script:
    if (interrupted)
      bec::GRTManager::get()->replace_status_text(_("Query interrupted"));
    // try to minimize the times this is called, since this will change the state of the connection
    // after a user query is ran (eg, it will reset all warnings)
    if (ran_set_sql_mode)
      cache_sql_mode();
  }
  CATCH_ANY_EXCEPTION_AND_DISPATCH(statement)

  if (dbc_driver)
    dbc_driver->threadEnd();

  logDebug("SQL execution finished\n");

  update_menu_and_toolbar();

  _usr_dbc_conn->is_stop_query_requested = false;

  return grt::StringRef("");
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::exec_management_sql(const std::string &sql, bool log) {
  sql::Dbc_connection_handler::Ref conn;
  base::RecMutexLock lock(ensure_valid_aux_connection(conn));
  if (conn) {
    RowId rid = log ? add_log_message(DbSqlEditorLog::BusyMsg, _("Executing "), sql, "- / ?") : 0;
    const std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
    Timer statement_exec_timer(false);
    try {
      stmt->execute(sql);
    } catch (sql::SQLException &e) {
      if (log)
        set_log_message(rid, DbSqlEditorLog::ErrorMsg, strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()),
                        sql, "");
      throw;
    }
    CATCH_EXCEPTION_AND_DISPATCH(sql);

    if (log)
      set_log_message(rid, DbSqlEditorLog::OKMsg, _("OK"), sql, statement_exec_timer.duration_formatted());

    handle_command_side_effects(sql);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::exec_main_sql(const std::string &sql, bool log) {
  base::RecMutexLock lock(ensure_valid_usr_connection());
  if (_usr_dbc_conn) {
    RowId rid = log ? add_log_message(DbSqlEditorLog::BusyMsg, _("Executing "), sql, "- / ?") : 0;
    const std::unique_ptr<sql::Statement> stmt(_usr_dbc_conn->ref->createStatement());
    Timer statement_exec_timer(false);
    try {
      stmt->execute(sql);
    } catch (sql::SQLException &e) {
      if (log)
        set_log_message(rid, DbSqlEditorLog::ErrorMsg, strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()),
                        sql, "");
      throw;
    }
    CATCH_EXCEPTION_AND_DISPATCH(sql);

    if (log)
      set_log_message(rid, DbSqlEditorLog::OKMsg, _("OK"), sql, statement_exec_timer.duration_formatted());

    handle_command_side_effects(sql);
  }
}

//----------------------------------------------------------------------------------------------------------------------

static wb::LiveSchemaTree::ObjectType str_to_object_type(const std::string &object_type) {
  if (object_type == "db.Table")
    return LiveSchemaTree::Table;
  else if (object_type == "db.View")
    return LiveSchemaTree::View;
  else if (object_type == "db.StoredProcedure")
    return LiveSchemaTree::Procedure;
  else if (object_type == "db.Function")
    return LiveSchemaTree::Function;
  else if (object_type == "db.Index")
    return LiveSchemaTree::Index;
  else if (object_type == "db.Trigger")
    return LiveSchemaTree::Trigger;
  else if (object_type == "db.Schema")
    return LiveSchemaTree::Schema;

  return LiveSchemaTree::NoneType;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::handle_command_side_effects(const std::string &sql) {
  SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms(rdbms());

  std::string object_type;
  std::string schema_name = active_schema();
  std::vector<std::pair<std::string, std::string>> object_names;

  // special hack, check for some special commands and update UI accordingly
  if (sql_facade->parseDropStatement(sql, object_type, object_names) && !object_names.empty()) {
    wb::LiveSchemaTree::ObjectType obj = str_to_object_type(object_type);
    if (obj != wb::LiveSchemaTree::NoneType) {
      std::vector<std::pair<std::string, std::string>>::reverse_iterator rit;

      if (obj == wb::LiveSchemaTree::Schema) {
        for (rit = object_names.rbegin(); rit != object_names.rend(); ++rit)
          _live_tree->refresh_live_object_in_overview(obj, (*rit).first, (*rit).first, "");

        if (!object_names.empty())
          schema_name = object_names.back().first;

        if ((schema_name.size() > 0) && (active_schema() == schema_name) && connection_descriptor().is_valid()) {
          std::string default_schema = connection_descriptor()->parameterValues().get_string("schema", "");
          if (schema_name == default_schema)
            default_schema = "";
          bec::GRTManager::get()->run_once_when_idle(this,
                                                     std::bind(&set_active_schema, shared_from_this(), default_schema));
        }
      } else {
        for (rit = object_names.rbegin(); rit != object_names.rend(); ++rit)
          _live_tree->refresh_live_object_in_overview(obj, (*rit).first.empty() ? schema_name : (*rit).first,
                                                      (*rit).second, "");
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

db_query_ResultsetRef SqlEditorForm::exec_management_query(const std::string &sql, bool log) {
  sql::Dbc_connection_handler::Ref conn;
  base::RecMutexLock lock(ensure_valid_aux_connection(conn));
  if (conn) {
    RowId rid = log ? add_log_message(DbSqlEditorLog::BusyMsg, _("Executing "), sql, "- / ?") : 0;
    const std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
    Timer statement_exec_timer(false);
    try {
      std::shared_ptr<sql::ResultSet> results(stmt->executeQuery(sql));

      if (log)
        set_log_message(rid, DbSqlEditorLog::OKMsg, _("OK"), sql, statement_exec_timer.duration_formatted());

      return grtwrap_recordset(grtobj(), results);
    } catch (sql::SQLException &e) {
      if (log)
        set_log_message(rid, DbSqlEditorLog::ErrorMsg, strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()),
                        sql, "");
      throw;
    }
  }
  return db_query_ResultsetRef();
}

//----------------------------------------------------------------------------------------------------------------------

db_query_ResultsetRef SqlEditorForm::exec_main_query(const std::string &sql, bool log) {
  base::RecMutexLock lock(ensure_valid_usr_connection());
  if (_usr_dbc_conn) {
    RowId rid = log ? add_log_message(DbSqlEditorLog::BusyMsg, _("Executing "), sql, "- / ?") : 0;
    const std::unique_ptr<sql::Statement> stmt(_usr_dbc_conn->ref->createStatement());
    Timer statement_exec_timer(false);
    try {
      std::shared_ptr<sql::ResultSet> results(stmt->executeQuery(sql));

      if (log)
        set_log_message(rid, DbSqlEditorLog::OKMsg, _("OK"), sql, statement_exec_timer.duration_formatted());

      return grtwrap_recordset(grtobj(), results);
    } catch (sql::SQLException &e) {
      if (log)
        set_log_message(rid, DbSqlEditorLog::ErrorMsg, strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()),
                        sql, "");
      throw;
    }
  }
  return db_query_ResultsetRef();
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorForm::is_running_query() {
  return _is_running_query;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::continue_on_error(bool val) {
  if (_continueOnError == val)
    return;

  _continueOnError = val;
  bec::GRTManager::get()->set_app_option("DbSqlEditor:ContinueOnError", grt::IntegerRef((int)_continueOnError));

  if (_menu)
    _menu->set_item_checked("query.continueOnError", continue_on_error());
  set_editor_tool_items_checked("query.continueOnError", continue_on_error());
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::send_message_keep_alive() {
  try {
    logDebug3("KeepAliveInterval tick\n");
    // ping server and reset connection timeout counter
    // this also checks the connection state and restores it if possible
    ensure_valid_aux_connection();
    ensure_valid_usr_connection();
  } catch (const std::exception &) {
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::apply_changes_to_recordset(Recordset::Ptr rs_ptr) {
  RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, rs_ptr, rs)

  try {
    bool auto_commit = false;

    // we need transaction to enforce atomicity of change set
    // so if autocommit is currently enabled disable it temporarily
    {
      RecMutexLock usr_dbc_conn_mutex = ensure_valid_usr_connection();
      auto_commit = _usr_dbc_conn->ref->getAutoCommit();
    }
    ScopeExitTrigger autocommit_mode_keeper;
    int res = -2;

    if (!auto_commit) {
      res = mforms::Utilities::show_warning(
        _("Apply Changes to Recordset"),
        _("Autocommit is currently disabled and a transaction might be open.\n"
          "Recordset changes will be applied within that transaction and will be left uncommited until you explicitly "
          "commit it manually.\n"
          "If you want it to be executed separately, click Cancel and commit the transaction first."),
        _("Apply"), _("Cancel"));
    } else {
      autocommit_mode_keeper.slot = std::bind(&sql::Connection::setAutoCommit, _usr_dbc_conn->ref.get(), auto_commit);
      RecMutexLock usr_dbc_conn_mutex = ensure_valid_usr_connection();
      _usr_dbc_conn->ref->setAutoCommit(false);
    }

    if (res != mforms::ResultCancel) // only if not canceled
    {
      on_sql_script_run_error.disconnect_all_slots();
      on_sql_script_run_progress.disconnect_all_slots();
      on_sql_script_run_statistics.disconnect_all_slots();

      Recordset_data_storage::Ref data_storage_ref = rs->data_storage();
      Recordset_sql_storage *sql_storage = dynamic_cast<Recordset_sql_storage *>(data_storage_ref.get());

      scoped_connection c1(
        on_sql_script_run_error.connect(std::bind(&SqlEditorForm::add_log_message, this, DbSqlEditorLog::ErrorMsg,
                                                  std::placeholders::_2, std::placeholders::_3, "")));

      bool skip_commit;
      if (auto_commit)
        skip_commit = false;
      else
        skip_commit = true; // if we're in an open tx, then do not commit

      bool is_data_changes_commit_wizard_enabled =
        (0 != bec::GRTManager::get()->get_app_option_int("DbSqlEditor:IsDataChangesCommitWizardEnabled", 1));
      if (is_data_changes_commit_wizard_enabled) {
        run_data_changes_commit_wizard(rs_ptr, skip_commit);
      } else {
        sql_storage->is_sql_script_substitute_enabled(false);

        scoped_connection on_sql_script_run_error_conn(
          sql_storage->on_sql_script_run_error.connect(on_sql_script_run_error));
        rs->do_apply_changes(rs_ptr, Recordset_data_storage::Ptr(data_storage_ref), skip_commit);
      }

      // Since many messages could have been added it is possible the
      // the action log has not been refresh, this triggers a refresh
      refresh_log_messages(true);
    }
  }
  CATCH_ANY_EXCEPTION_AND_DISPATCH(_("Apply changes to recordset"))
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorForm::run_data_changes_commit_wizard(Recordset::Ptr rs_ptr, bool skip_commit) {
  RETVAL_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, rs_ptr, rs, false)

  // set underlying recordset data storage to use sql substitute (potentially modified by user)
  // instead of generating sql based on swap db contents
  Recordset_data_storage::Ref data_storage_ref = rs->data_storage();
  Recordset_sql_storage *sql_storage = dynamic_cast<Recordset_sql_storage *>(data_storage_ref.get());
  if (!sql_storage)
    return false;
  sql_storage->init_sql_script_substitute(rs_ptr, true);
  sql_storage->is_sql_script_substitute_enabled(true);
  const Sql_script &sql_script = sql_storage->sql_script_substitute();
  ;
  std::string sql_script_text = Recordset_sql_storage::statements_as_sql_script(sql_script.statements);

  // No need for online DDL settings or callback as we are dealing with data here, not metadata.
  SqlScriptRunWizard wizard(rdbms_version(), "", "");

  scoped_connection c1(
    on_sql_script_run_error.connect(std::bind(&SqlScriptApplyPage::on_error, wizard.apply_page, std::placeholders::_1,
                                              std::placeholders::_2, std::placeholders::_3)));
  scoped_connection c2(on_sql_script_run_progress.connect(
    std::bind(&SqlScriptApplyPage::on_exec_progress, wizard.apply_page, std::placeholders::_1)));
  scoped_connection c3(on_sql_script_run_statistics.connect(
    std::bind(&SqlScriptApplyPage::on_exec_stat, wizard.apply_page, std::placeholders::_1, std::placeholders::_2)));
  wizard.values().gset("sql_script", sql_script_text);
  wizard.apply_page->apply_sql_script =
    std::bind(&SqlEditorForm::apply_data_changes_commit, this, std::placeholders::_1, rs_ptr, skip_commit);
  wizard.run_modal();

  return !wizard.has_errors();
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::apply_object_alter_script(const std::string &alter_script, bec::DBObjectEditorBE *obj_editor,
                                              RowId log_id) {
  set_log_message(
    log_id, DbSqlEditorLog::BusyMsg, "",
    obj_editor ? strfmt(_("Applying changes to %s..."), obj_editor->get_name().c_str()) : _("Applying changes..."), "");

  SqlFacade::Ref sql_splitter = SqlFacade::instance_for_rdbms(rdbms());
  std::list<std::string> statements;
  sql_splitter->splitSqlScript(alter_script, statements);

  int max_query_size_to_log = (int)bec::GRTManager::get()->get_app_option_int("DbSqlEditor:MaxQuerySizeToHistory", 0);

  sql::SqlBatchExec sql_batch_exec;
  sql_batch_exec.stop_on_error(true);

  sql_batch_exec.error_cb(std::ref(on_sql_script_run_error));
  sql_batch_exec.batch_exec_progress_cb(std::ref(on_sql_script_run_progress));
  sql_batch_exec.batch_exec_stat_cb(std::ref(on_sql_script_run_statistics));

  long sql_batch_exec_err_count = 0;
  {
    try {
      RecMutexLock usr_dbc_conn_mutex(ensure_valid_usr_connection(true));
      std::unique_ptr<sql::Statement> stmt(_usr_dbc_conn->ref->createStatement());
      sql_batch_exec_err_count = sql_batch_exec(stmt.get(), statements);
    } catch (sql::SQLException &e) {
      set_log_message(log_id, DbSqlEditorLog::ErrorMsg, strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()),
                      strfmt(_("Apply ALTER script for %s"), obj_editor->get_name().c_str()), "");
      throw; // re-throw exception so that the wizard will see that something went wrong
    } catch (base::mutex_busy_error &) {
      set_log_message(log_id, DbSqlEditorLog::ErrorMsg,
                      strfmt(EXCEPTION_MSG_FORMAT, "Your connection to MySQL is currently busy. Please retry later."),
                      strfmt(_("Apply ALTER script for %s"), obj_editor->get_name().c_str()), "");
      throw std::runtime_error("Connection to MySQL currently busy.");
    } catch (std::exception &e) {
      set_log_message(log_id, DbSqlEditorLog::ErrorMsg, strfmt(EXCEPTION_MSG_FORMAT, e.what()),
                      strfmt(_("Apply ALTER script for %s"), obj_editor->get_name().c_str()), "");
      throw;
    }
  }

  if (!max_query_size_to_log || max_query_size_to_log >= (int)alter_script.size())
    _history->add_entry(sql_batch_exec.sql_log());

  // refresh object's state only on success, to not lose changes made by user
  if (obj_editor && (0 == sql_batch_exec_err_count)) {
    db_DatabaseObjectRef db_object = obj_editor->get_dbobject();

    set_log_message(log_id, DbSqlEditorLog::OKMsg, strfmt(_("Changes applied to %s"), obj_editor->get_name().c_str()),
                    "", "");

    // refresh state of created/altered object in physical overview
    {
      std::string schema_name = db_SchemaRef::can_wrap(db_object) ? std::string() : *db_object->owner()->name();
      db_SchemaRef schema;
      if (!schema_name.empty())
        schema = db_SchemaRef::cast_from(db_object->owner());

      wb::LiveSchemaTree::ObjectType db_object_type = wb::LiveSchemaTree::Any;
      if (db_SchemaRef::can_wrap(db_object))
        db_object_type = wb::LiveSchemaTree::Schema;
      else if (db_TableRef::can_wrap(db_object))
        db_object_type = wb::LiveSchemaTree::Table;
      else if (db_ViewRef::can_wrap(db_object))
        db_object_type = wb::LiveSchemaTree::View;
      else if (db_RoutineRef::can_wrap(db_object)) {
        db_RoutineRef db_routine = db_RoutineRef::cast_from(db_object);

        std::string obj_type = db_routine->routineType();

        if (obj_type == "function")
          db_object_type = wb::LiveSchemaTree::Function;
        else
          db_object_type = wb::LiveSchemaTree::Procedure;
      }

      //_live_tree->refresh_live_object_in_overview(db_object_type, schema_name, db_object->oldName(),
      //db_object->name());
      // Run refresh on main thread, but only if there's not another refresh pending already.
      if (!_overviewRefreshPending.connected()) {
        _overviewRefreshPending = bec::GRTManager::get()->run_once_when_idle(
          this, std::bind(&SqlEditorTreeController::refresh_live_object_in_overview, _live_tree, db_object_type,
                          schema_name, db_object->oldName(), db_object->name()));
      }
    }

    //_live_tree->refresh_live_object_in_editor(obj_editor, false);
    if (!_editorRefreshPending.connected()) {
      _editorRefreshPending = bec::GRTManager::get()->run_once_when_idle(
        this, std::bind(&SqlEditorTreeController::refresh_live_object_in_editor, _live_tree, obj_editor, false));
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::apply_data_changes_commit(const std::string &sql_script_text, Recordset::Ptr rs_ptr,
                                              bool skip_commit) {
  RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, rs_ptr, rs);

  // this lock is supposed to be acquired lower in call-stack by SqlEditorForm::apply_changes_to_recordset
  // MutexLock usr_conn_mutex= ensure_valid_usr_connection();

  Recordset_data_storage::Ref data_storage_ref = rs->data_storage();
  Recordset_sql_storage *sql_storage = dynamic_cast<Recordset_sql_storage *>(data_storage_ref.get());
  if (!sql_storage)
    return;

  int max_query_size_to_log = (int)bec::GRTManager::get()->get_app_option_int("DbSqlEditor:MaxQuerySizeToHistory", 0);

  Sql_script sql_script = sql_storage->sql_script_substitute();
  sql_script.statements.clear();
  SqlFacade::Ref sql_splitter = SqlFacade::instance_for_rdbms(rdbms());
  sql_splitter->splitSqlScript(sql_script_text, sql_script.statements);

  scoped_connection on_sql_script_run_error_conn(sql_storage->on_sql_script_run_error.connect(on_sql_script_run_error));
  scoped_connection on_sql_script_run_progress_conn(
    sql_storage->on_sql_script_run_progress.connect(on_sql_script_run_progress));
  scoped_connection on_sql_script_run_statistics_conn(
    sql_storage->on_sql_script_run_statistics.connect(on_sql_script_run_statistics));

  sql_storage->sql_script_substitute(sql_script);
  rs->setPreserveRowFilter(bec::GRTManager::get()->get_app_option_int("SqlEditor:PreserveRowFilter") == 1);
  rs->do_apply_changes(rs_ptr, Recordset_data_storage::Ptr(data_storage_ref), skip_commit);

  if (!max_query_size_to_log || max_query_size_to_log >= (int)sql_script_text.size())
    _history->add_entry(sql_script.statements);
}

//----------------------------------------------------------------------------------------------------------------------

std::string SqlEditorForm::active_schema() const {
  return (_usr_dbc_conn) ? _usr_dbc_conn->active_schema : std::string();
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::schemaListRefreshed(std::vector<std::string> const &schemas) {
  std::unique_lock<std::mutex> lock(_pimplMutex->_symbolsMutex);
  _databaseSymbols.clear(); // Doesn't clear the dependencies.

  for (auto schema : schemas) {
    _databaseSymbols.addNewSymbol<SchemaSymbol>(nullptr, schema);
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Reads all relevant built-in symbols like engines and collations in our static server symbols list.
 */
void SqlEditorForm::readStaticServerSymbols() {
  std::unique_lock<std::mutex> lock(_pimplMutex->_symbolsMutex); // Probably not needed, as this runs during startup.

  if (_usr_dbc_conn->ref.get() != nullptr) {
    const std::unique_ptr<sql::Statement> statement(_usr_dbc_conn->ref.get()->createStatement());

    {
      const std::unique_ptr<sql::ResultSet> rs(statement->executeQuery("show engines"));
      while (rs->next()) {
        std::string name = rs->getString(1);
        std::string support = rs->getString(2);
        if (support != "NO") { // Can be YES, NO or DEFAULT.
          _staticServerSymbols.addNewSymbol<EngineSymbol>(nullptr, name);
        }
      }
    }

    {
      const std::unique_ptr<sql::ResultSet> rs(statement->executeQuery("show charset"));
      while (rs->next()) {
        _staticServerSymbols.addNewSymbol<CharsetSymbol>(nullptr, rs->getString(1));
      }
    }

    {
      const std::unique_ptr<sql::ResultSet> rs(statement->executeQuery("show collation"));
      while (rs->next()) {
        _staticServerSymbols.addNewSymbol<CollationSymbol>(nullptr, rs->getString(1));
      }
    }

    {
      const std::unique_ptr<sql::ResultSet> rs(statement->executeQuery("show variables"));
      while (rs->next()) {
        _staticServerSymbols.addNewSymbol<SystemVariableSymbol>(nullptr, "@@" + rs->getString(1));
      }
    }

  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Notification from the tree controller that (some) schema meta data has been refreshed. We use this
 * info to update the database symbol table.
 */
void SqlEditorForm::schema_meta_data_refreshed(const std::string &schema_name, base::StringListPtr tables,
                                               base::StringListPtr views, base::StringListPtr procedures,
                                               base::StringListPtr functions) {
  std::unique_lock<std::mutex> lock(_pimplMutex->_symbolsMutex);
  std::unique_ptr<sql::Statement> statement;
  RecMutexLock usr_dbc_conn_mutex(ensure_valid_usr_connection());
  if (_usr_dbc_conn->ref.get() != nullptr)
    statement.reset(_usr_dbc_conn->ref->createStatement());

  auto schemaSymbols = _databaseSymbols.getSymbolsOfType<SchemaSymbol>();
  bool hasPerformanceSchema = std::find_if(schemaSymbols.begin(), schemaSymbols.end(), [](auto symbol) -> bool {
    return symbol->name == "performance_schema";
  }) != schemaSymbols.end();

  for (SchemaSymbol *schemaSymbol : schemaSymbols) {
    if (schemaSymbol->name == schema_name) {
      schemaSymbol->clear();
      for (auto table : *tables) {
        TableSymbol *tableSymbol = _databaseSymbols.addNewSymbol<TableSymbol>(schemaSymbol, table);

        // Fetch column info for each table.
        if (statement != nullptr) {
          std::unique_ptr<sql::ResultSet> rs(statement->executeQuery(
            std::string(base::sqlstring("SHOW FULL COLUMNS FROM !.!", 0) << schema_name << table)));

          while (rs->next()) {
            _databaseSymbols.addNewSymbol<ColumnSymbol>(tableSymbol, rs->getString(1), nullptr);
          }
        }
      }

      for (auto view : *views) {
        ViewSymbol *viewSymbol = _databaseSymbols.addNewSymbol<ViewSymbol>(schemaSymbol, view);

        // Same for each view.
        if (statement != nullptr) {
          std::unique_ptr<sql::ResultSet> rs(statement->executeQuery(
            std::string(base::sqlstring("SHOW FULL COLUMNS FROM !.!", 0) << schema_name << view)));

          while (rs->next()) {
            _databaseSymbols.addNewSymbol<ColumnSymbol>(viewSymbol, rs->getString(1), nullptr);
          }
        }
      }

      for (auto procedure : *procedures) {
        _databaseSymbols.addNewSymbol<StoredRoutineSymbol>(schemaSymbol, procedure, nullptr);
      }

      for (auto function : *functions) {
        _databaseSymbols.addNewSymbol<StoredRoutineSymbol>(schemaSymbol, function, nullptr);
      }

      if (statement != nullptr) {
        auto metaInfo = _usr_dbc_conn->ref->getMetaData();
        if (hasPerformanceSchema && (metaInfo->getDatabaseMajorVersion() > 7
            || (metaInfo->getDatabaseMajorVersion() == 5 && metaInfo->getDatabaseMinorVersion() > 6))) {
          std::unique_ptr<sql::ResultSet> rs(
            statement->executeQuery("SELECT VARIABLE_NAME FROM performance_schema.user_variables_by_thread")
          );

          while (rs->next()) {
            _databaseSymbols.addNewSymbol<UserVariableSymbol>(nullptr, "@" + rs->getString(1), nullptr);
          }
        }
      }

      return;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::cache_active_schema_name() {
  std::string schema = _usr_dbc_conn->ref->getSchema();
  _usr_dbc_conn->active_schema = schema;
  _aux_dbc_conn->active_schema = schema;

  exec_sql_task->execute_in_main_thread(std::bind(&SqlEditorForm::update_editor_title_schema, this, schema), false,
                                        true);
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::active_schema(const std::string &value) {
  try {
    if (value == active_schema())
      return;

    {
      RecMutexLock aux_dbc_conn_mutex(ensure_valid_aux_connection());
      if (!value.empty())
        _aux_dbc_conn->ref->setSchema(value);
      _aux_dbc_conn->active_schema = value;
    }

    {
      RecMutexLock usr_dbc_conn_mutex(ensure_valid_usr_connection());
      if (!value.empty())
        _usr_dbc_conn->ref->setSchema(value);
      _usr_dbc_conn->active_schema = value;
    }

    if (_tabdock) {
      // set current schema for the editors to notify the autocompleter
      for (int c = _tabdock->view_count(), i = 0; i < c; i++) {
        SqlEditorPanel *panel = sql_editor_panel(i);
        if (panel)
          panel->editor_be()->set_current_schema(value);
      }
    }
    _live_tree->on_active_schema_change(value);
    // remember active schema
    _connection->parameterValues().gset("DbSqlEditor:LastDefaultSchema", value);

    update_editor_title_schema(value);

    if (value.empty())
      bec::GRTManager::get()->replace_status_text(_("Active schema was cleared"));
    else
      bec::GRTManager::get()->replace_status_text(strfmt(_("Active schema changed to %s"), value.c_str()));

    grt::GRT::get()->call_module_function("Workbench", "saveConnections", grt::BaseListRef());
  }
  CATCH_ANY_EXCEPTION_AND_DISPATCH(_("Set active schema"))
}

//----------------------------------------------------------------------------------------------------------------------

db_mgmt_RdbmsRef SqlEditorForm::rdbms() {
  if (_connection.is_valid()) {
    if (!_connection->driver().is_valid())
      throw std::runtime_error("Connection has invalid driver, check connection parameters.");
    return db_mgmt_RdbmsRef::cast_from(_connection->driver()->owner());
  } else
    return db_mgmt_RdbmsRef::cast_from(grt::GRT::get()->get("/wb/rdbmsMgmt/rdbms/0/"));
}

//----------------------------------------------------------------------------------------------------------------------

int SqlEditorForm::count_connection_editors(const std::string &conn_name) {
  int count = 0;
  std::weak_ptr<SqlEditorForm> editor;

  std::list<std::weak_ptr<SqlEditorForm>>::iterator index, end;

  end = _wbsql->get_open_editors()->end();
  for (index = _wbsql->get_open_editors()->begin(); index != end; index++) {
    SqlEditorForm::Ref editor((*index).lock());
    if (editor->_connection.is_valid()) {
      std::string editor_connection = editor->_connection->name();
      if (editor_connection == conn_name)
        count++;
    }
  }

  return count;
}

//----------------------------------------------------------------------------------------------------------------------

std::string SqlEditorForm::create_title() {
  std::string caption;
  std::string editor_connection = get_session_name();
  if (_connection.is_valid()) {
    if (!editor_connection.empty())
      caption += strfmt("%s", editor_connection.c_str());
    else {
      if (_connection->driver()->name() == "MysqlNativeSocket")
        caption += "localhost";
      else
        caption += strfmt("%s", truncate_text(editor_connection, 21).c_str());
    }

    // only show schema name if there's more than 1 tab to the same connection, to save space
    if (!_usr_dbc_conn->active_schema.empty() && count_connection_editors(editor_connection) > 1)
      caption += strfmt(" (%s)", truncate_text(_usr_dbc_conn->active_schema, 20).c_str());

    if (_connection_details.find("dbmsProductVersion") != _connection_details.end() &&
        !bec::is_supported_mysql_version(_connection_details["dbmsProductVersion"]))
      caption += " - Warning - not supported";
  } else
    caption = editor_connection;
  return caption;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::update_title() {
  std::string temp_title = create_title();
  if (_title != temp_title) {
    _title = temp_title;
    title_changed();
  }
}

//----------------------------------------------------------------------------------------------------------------------

int SqlEditorForm::getTunnelPort() const {
  if (_tunnel)
    return _tunnel->getConfig().localport;
  return -1;
}

//----------------------------------------------------------------------------------------------------------------------

GrtVersionRef SqlEditorForm::rdbms_version() const {
  return _version;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the current server version (or a reasonable default if not connected) in compact form
 * as needed for parsing on various occasions (context help, auto completion, error parsing).
 */
int SqlEditorForm::server_version() {
  GrtVersionRef version = rdbms_version();

  // Create a server version of the form "Mmmrr" as long int for quick comparisons.
  if (version.is_valid())
    return (int)(version->majorNumber() * 10000 + version->minorNumber() * 100 + version->releaseNumber());
  else
    return 50503;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns a list of valid charsets for this connection as needed for parsing.
 */
std::set<std::string> SqlEditorForm::valid_charsets() {
  if (_charsets.empty()) {
    grt::ListRef<db_CharacterSet> list = rdbms()->characterSets();
    for (std::size_t i = 0; i < list->count(); i++)
      _charsets.insert(base::tolower(*list[i]->name()));

    // 3 character sets were added in version 5.5.3. Remove them from the list if the current version
    // is lower than that.
    if (server_version() < 50503) {
      _charsets.erase("utf8mb4");
      _charsets.erase("utf16");
      _charsets.erase("utf32");
    }
  }
  return _charsets;
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorForm::save_snippet() {
  SqlEditorPanel *panel = active_sql_editor_panel();
  if (!panel)
    return false;
  std::string text;
  std::size_t start, end;
  if (panel->editor_be()->selected_range(start, end))
    text = panel->editor_be()->selected_text();
  else
    text = panel->editor_be()->current_statement();

  if (text.empty())
    return false;

  DbSqlEditorSnippets::get_instance()->add_snippet("", text, true);
  bec::GRTManager::get()->replace_status_text("SQL saved to snippets list.");

  _side_palette->refresh_snippets();

  bec::GRTManager::get()->run_once_when_idle(this, std::bind(&QuerySidePalette::edit_last_snippet, _side_palette));

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorForm::can_close() {
  return can_close_(true);
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorForm::can_close_(bool interactive) {
  if (exec_sql_task && exec_sql_task->is_busy()) {
    bec::GRTManager::get()->replace_status_text(_("Cannot close SQL IDE while being busy"));
    return false;
  }

  if (!bec::UIForm::can_close())
    return false;

  _live_tree->prepare_close();
  bec::GRTManager::get()->set_app_option("DbSqlEditor:ActiveSidePaletteTab",
                                         grt::IntegerRef(_side_palette->get_active_tab()));

  bool check_scratch_editors = true;
  bool save_workspace_on_close = false;

  // if Save of workspace on close is enabled, we don't need to check whether there are unsaved
  // SQL editors but other stuff should be checked.
  grt::ValueRef option(bec::GRTManager::get()->get_app_option("workbench:SaveSQLWorkspaceOnClose"));
  if (option.is_valid() && *grt::IntegerRef::cast_from(option)) {
    save_workspace_on_close = true;
    check_scratch_editors = false;
  }
  bool editor_needs_review = false;
  if (interactive) {
    ConfirmSaveDialog dialog(
      0, "Close SQL Editor",
      "The following files/resultsets have unsaved changes.\nDo you want to review these changes before closing?");
    for (int i = 0; i < sql_editor_count(); i++) {
      SqlEditorPanel *panel = sql_editor_panel(i);
      if (!panel)
        continue;

      bool check_editor = !panel->is_scratch() || check_scratch_editors;
      if (panel->filename().empty() && save_workspace_on_close)
        check_editor = false;

      if (panel->is_dirty() && check_editor) {
        editor_needs_review = true;
        dialog.add_item("Script Buffers", panel->get_title());
      }

      std::list<SqlEditorResult *> rset(panel->dirty_result_panels());
      for (auto *r : rset) {
        dialog.add_item("Resultset", r->caption());
      }
    }

    bool review = false;
    if (dialog.change_count() > 1) {
      switch (dialog.run()) {
        case ConfirmSaveDialog::ReviewChanges:
          review = true;
          break;

        case ConfirmSaveDialog::DiscardChanges:
          review = false;
          break;

        case ConfirmSaveDialog::Cancel:
          return false;
      }
    } else if (dialog.change_count() == 1)
      review = true;

    // review changes 1 by 1
    if (review && editor_needs_review) {
      _closing = true;
      for (int i = 0; i < sql_editor_count(); i++) {
        SqlEditorPanel *panel = sql_editor_panel(i);
        if (panel && !panel->can_close()) {
          _closing = false;
          return false;
        }
      }
    }
  } else { // !interactive, return false if there's any unsaved edits in editor or resultsets
    for (int i = 0; i < sql_editor_count(); i++) {
      SqlEditorPanel *panel = sql_editor_panel(i);
      if (panel) {
        if (editor_needs_review && panel->is_dirty())
          return false;

        if (!panel->dirty_result_panels().empty())
          return false;
      }
    }
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::check_external_file_changes() {
  for (int i = 0; i < sql_editor_count(); i++) {
    SqlEditorPanel *panel = sql_editor_panel(i);
    if (panel)
      panel->check_external_file_changes();
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorForm::update_editor_title_schema(const std::string &schema) {
  _live_tree->on_active_schema_change(schema);

  // Gets the editor label including the schema name only if
  // the number of opened editors to the same host is > 1
  update_title();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Called whenever a connection to a server is opened, whether it succeeds or not.
 *
 * Call this when a connection to the server is opened. If the connection succeeded, pass 0 as
 * the error and if it fails, pass the error code.
 *
 * The error will be used to determine whether the connection failed because the server is possibly
 * down (or doesn't exist) or some other reason (like wrong password).
 */
void SqlEditorForm::note_connection_open_outcome(int error) {
  ServerState newState;
  switch (error) {
    case 0:
      newState = RunningState; // success = running;
      break;
    case 2002: // CR_CONNECTION_ERROR
    case 2003: // CR_CONN_HOST_ERROR
      newState = PossiblyStoppedState;
      break;
    case 2013: // Lost packet blabla, can happen on failure when using ssh tunnel
      newState = PossiblyStoppedState;
      break;
    default:
      // there may be other errors that could indicate server stopped and maybe
      // some errors that can't tell anything about the server state
      newState = RunningState;
      break;
  }

  if (_last_server_running_state != newState && newState != UnknownState) {
    grt::DictRef info(true);
    _last_server_running_state = newState;

    if (newState == RunningState)
      info.gset("state", 1);
    else if (newState == OfflineState)
      info.gset("state", -1);
    else
      info.gset("state", 0);

    info.set("connection", connection_descriptor());

    logDebug2("Notifying server state change of %s to %s\n", connection_descriptor()->hostIdentifier().c_str(),
              (newState == RunningState || newState == OfflineState) ? "running" : "not running");
    GRTNotificationCenter::get()->send_grt("GRNServerStateChanged", grtobj(), info);
  }
}

//----------------------------------------------------------------------------------------------------------------------
