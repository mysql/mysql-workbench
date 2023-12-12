/*
 * Copyright (c) 2007, 2023, Oracle and/or its affiliates. All rights reserved.
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

#include <regex>

#include "grtpp_util.h"
#include "wb_sql_editor_tree_controller.h"
#include "wb_sql_editor_form.h"
#include "execute_routine_wizard.h"
#include "wb_sql_editor_panel.h"

#include "base/sqlstring.h"
#include "base/util_functions.h"
#include "base/string_utilities.h"
#include "base/boost_smart_ptr_helpers.h"
#include "base/log.h"
#include "base/notifications.h"

#include "workbench/wb_command_ui.h"
#include "workbench/wb_context_ui.h"

#include <boost/signals2/connection.hpp>

#include "diff/diffchange.h"
#include "grtdb/diff_dbobjectmatch.h"
#include "db.mysql/src/module_db_mysql.h"

#include "grtdb/editor_table.h"
#include "grtdb/db_helpers.h"
#include "grtsqlparser/sql_facade.h"

#include "workbench/wb_db_schema.h"

#include "objimpl/wrapper/mforms_ObjectReference_impl.h"

#include "advanced_sidebar.h"

#include "mforms/textentry.h"
#include "mforms/hypertext.h"
#include "mforms/splitter.h"
#include "mforms/tabview.h"
#include "mforms/panel.h"
#include "mforms/menubar.h"
#include "mforms/utilities.h"

using namespace grt;
using namespace bec;
using namespace wb;
using namespace base;
using namespace parsers;

using boost::signals2::scoped_connection;

DEFAULT_LOG_DOMAIN("SqlEditorSchemaTree");

static const char *SQL_EXCEPTION_MSG_FORMAT = _("Error Code: %i\n%s");
static const char *EXCEPTION_MSG_FORMAT = _("Error: %s");

//----------------------------------------------------------------------------------------------------------------------

#define CATCH_ANY_EXCEPTION_AND_DISPATCH(statement)                                                                 \
  catch (sql::SQLException & e) {                                                                                   \
    _owner->add_log_message(DbSqlEditorLog::ErrorMsg, strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()), \
                            statement, "");                                                                         \
    logError("SQLException executing %s: %s\n", std::string(statement).c_str(),                                     \
             strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()).c_str());                                 \
  }                                                                                                                 \
  CATCH_EXCEPTION_AND_DISPATCH(statement)

#define CATCH_EXCEPTION_AND_DISPATCH(statement)                                                               \
  catch (std::exception & e) {                                                                                \
    _owner->add_log_message(DbSqlEditorLog::ErrorMsg, strfmt(EXCEPTION_MSG_FORMAT, e.what()), statement, ""); \
    logError("Exception executing %s: %s\n", std::string(statement).c_str(),                                  \
             strfmt(EXCEPTION_MSG_FORMAT, e.what()).c_str());                                                 \
  }

#define CATCH_ANY_EXCEPTION_AND_DISPATCH(statement)                                                                 \
  catch (sql::SQLException & e) {                                                                                   \
    _owner->add_log_message(DbSqlEditorLog::ErrorMsg, strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()), \
                            statement, "");                                                                         \
    logError("SQLException executing %s: %s\n", std::string(statement).c_str(),                                     \
             strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()).c_str());                                 \
  }                                                                                                                 \
  CATCH_EXCEPTION_AND_DISPATCH(statement)

#define CATCH_ANY_EXCEPTION_AND_DISPATCH_TO_DEFAULT_LOG(statement)                                        \
  catch (sql::SQLException & e) {                                                                         \
    grt::GRT::get()->send_error(strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()), statement); \
  }                                                                                                       \
  catch (std::exception & e) {                                                                            \
    grt::GRT::get()->send_error(strfmt(EXCEPTION_MSG_FORMAT, e.what()), statement);                       \
  }

//----------------------------------------------------------------------------------------------------------------------

std::shared_ptr<SqlEditorTreeController> SqlEditorTreeController::create(SqlEditorForm *owner) {
  std::shared_ptr<SqlEditorTreeController> instance(new SqlEditorTreeController(owner));

  instance->_base_schema_tree.set_delegate(instance);
  instance->_base_schema_tree.set_fetch_delegate(instance);

  instance->_filtered_schema_tree.set_delegate(instance);
  instance->_filtered_schema_tree.set_fetch_delegate(instance);
  instance->_filtered_schema_tree.set_base(&instance->_base_schema_tree);

  return instance;
}

//----------------------------------------------------------------------------------------------------------------------

SqlEditorTreeController::SqlEditorTreeController(SqlEditorForm *owner)
  : _owner(owner),
    _schema_side_bar(nullptr),
    _admin_side_bar(nullptr),
    _task_tabview(nullptr),
    _taskbar_box(nullptr),
    _schema_tree(&_base_schema_tree),
    _base_schema_tree(bec::versionToEnum(owner->rdbms_version())),
    _filtered_schema_tree(bec::versionToEnum(owner->rdbms_version())),
    live_schema_fetch_task(GrtThreadedTask::create()),
    live_schemata_refresh_task(GrtThreadedTask::create()),
    _is_refreshing_schema_tree(false),
    _use_show_procedure(false),
    _side_splitter(nullptr),
    _info_tabview(nullptr),
    _object_info(nullptr),
    _session_info(nullptr) {

  grt::GRTNotificationCenter::get()->add_grt_observer(this, "GRNDBObjectEditorCreated");
  grt::GRTNotificationCenter::get()->add_grt_observer(this, "GRNPreferencesDidClose");
  grt::GRTNotificationCenter::get()->add_grt_observer(this, "GRNSQLEditorReconnected");
  base::NotificationCenter::get()->add_observer(this, "GNColorsChanged");

  _base_schema_tree.is_schema_contents_enabled(
    bec::GRTManager::get()->get_app_option_int("DbSqlEditor:ShowSchemaTreeSchemaContents", 1) != 0);
  _filtered_schema_tree.is_schema_contents_enabled(
    bec::GRTManager::get()->get_app_option_int("DbSqlEditor:ShowSchemaTreeSchemaContents", 1) != 0);

  _base_schema_tree.sql_editor_text_insert_signal.connect(
    std::bind(&SqlEditorTreeController::insert_text_to_active_editor, this, std::placeholders::_1));
  _filtered_schema_tree.sql_editor_text_insert_signal.connect(
    std::bind(&SqlEditorTreeController::insert_text_to_active_editor, this, std::placeholders::_1));

  live_schemata_refresh_task->desc("Live Schema Refresh Task");
  live_schemata_refresh_task->send_task_res_msg(false);
  live_schemata_refresh_task->msg_cb(std::bind(&SqlEditorForm::add_log_message, _owner, std::placeholders::_1,
                                               std::placeholders::_2, std::placeholders::_3, ""));

  live_schema_fetch_task->desc("Live Schema Fetch Task");
  live_schema_fetch_task->send_task_res_msg(false);
  live_schema_fetch_task->msg_cb(std::bind(&SqlEditorForm::add_log_message, _owner, std::placeholders::_1,
                                           std::placeholders::_2, std::placeholders::_3, ""));
}

//----------------------------------------------------------------------------------------------------------------------

SqlEditorTreeController::~SqlEditorTreeController() {
  grt::GRTNotificationCenter::get()->remove_grt_observer(this);

  delete _taskbar_box;
  delete _info_tabview;
  delete _schema_side_bar;
  delete _admin_side_bar;
  delete _task_tabview;

  if (_side_splitter != NULL)
    _side_splitter->release();

  delete _session_info;
  delete _object_info;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::finish_init() {
  // Box to host the management and SQL IDE task bars.
  _taskbar_box = new mforms::Box(false);

  // Left hand sidebar tabview with admin and schema tree pages.
  _task_tabview = new mforms::TabView(mforms::TabViewSelectorSecondary);
  _task_tabview->set_name("SQL IDE Sidebar Tabview");
  _schema_side_bar = dynamic_cast<wb::AdvancedSidebar *>(mforms::TaskSidebar::create("SchemaTree"));
  scoped_connect(_schema_side_bar->on_section_command(),
                 std::bind(&SqlEditorTreeController::sidebar_action, this, std::placeholders::_1));
  _admin_side_bar = dynamic_cast<wb::SimpleSidebar *>(mforms::TaskSidebar::create("Simple"));
  scoped_connect(_admin_side_bar->on_section_command(),
                 std::bind(&SqlEditorTreeController::sidebar_action, this, std::placeholders::_1));
  
  _admin_side_bar->set_name("Administration Page");
  _schema_side_bar->set_name("Schema Page");

  mforms::TaskSectionFlags flags = mforms::TaskSectionRefreshable;
  _schema_side_bar->add_section("SchemaTree", "Schema Tree", _("SCHEMAS"), flags);

  _task_tabview->add_page(_admin_side_bar, _("Administration"));
  _task_tabview->add_page(_schema_side_bar, _("Schemas"));

  int i = (int)bec::GRTManager::get()->get_app_option_int("DbSqlEditor:ActiveTaskTab", 0);
  if (i < 0)
    i = 0;
  else if (i >= 2)
    i = 1;
  _task_tabview->set_active_tab(i);

  _schema_side_bar->get_context_menu()->signal_will_show()->connect(
    std::bind(&SqlEditorTreeController::context_menu_will_show, this, std::placeholders::_1));
  _schema_side_bar->set_schema_model(&_base_schema_tree);
  _schema_side_bar->set_filtered_schema_model(&_filtered_schema_tree);
  _schema_side_bar->set_selection_color(base::HighlightColor);

  int initial_splitter_pos =
    (int)bec::GRTManager::get()->get_app_option_int("DbSqlEditor:SidebarInitialSplitterPos", 500);
  _side_splitter = mforms::manage(new mforms::Splitter(false, false));
  _side_splitter->set_name("SQL IDE Sidebar Splitter");

#ifdef _MSC_VER
  mforms::Panel *panel;
  _side_splitter->set_back_color(base::Color::getApplicationColorAsString(AppColorMainBackground, false));
  panel = mforms::manage(new mforms::Panel(mforms::StyledHeaderPanel));
  panel->set_title("Navigator");
  panel->add(_taskbar_box);
  _taskbar_box->add(_task_tabview, true, true);
  _side_splitter->add(panel, 200);
#else
  _taskbar_box->add(_task_tabview, true, true);
  _side_splitter->add(_taskbar_box, 200);
#endif

  _info_tabview = new mforms::TabView(mforms::TabViewSelectorSecondary);
  _info_tabview->set_name("Info View");

#ifdef _MSC_VER
  panel = mforms::manage(new mforms::Panel(mforms::StyledHeaderPanel));
  panel->set_title(_("Information"));
  panel->add(_info_tabview);
  _side_splitter->add(panel, 200);
#else
  _side_splitter->add(_info_tabview, 200);
#endif

  _object_info = new mforms::HyperText();
  _session_info = new mforms::HyperText();
#ifdef _MSC_VER
  _object_info->set_padding(3, 3, 3, 3);
  _session_info->set_padding(3, 3, 3, 3);
#endif

  _info_tabview->add_page(_object_info, _("Object Info"));
  _info_tabview->add_page(_session_info, _("Session"));

  updateColors();

  scoped_connect(_schema_side_bar->signal_filter_changed(),
                 std::bind(&SqlEditorTreeController::side_bar_filter_changed, this, std::placeholders::_1));
  scoped_connect(_schema_side_bar->tree_node_selected(),
                 std::bind(&SqlEditorTreeController::schema_row_selected, this));

  // update the info box
  schema_row_selected();

  tree_refresh();

  // make sure to restore the splitter pos after layout is ready
  bec::GRTManager::get()->run_once_when_idle(
    this, std::bind(&mforms::Splitter::set_divider_position, _side_splitter, initial_splitter_pos));

  // Connect the splitter change event after the setup is done to avoid wrong triggering.
  _splitter_connection = _side_splitter->signal_position_changed()->connect(
    std::bind(&SqlEditorTreeController::sidebar_splitter_changed, this));

  // Setup grt access to sidebar.
  db_query_EditorRef editor(_owner->wbsql()->get_grt_editor_object(_owner));
  if (editor.is_valid())
    editor->sidebar(mforms_to_grt(_admin_side_bar, "TaskSidebar"));

  if (!_owner->connected())
    _info_tabview->set_active_tab(1);
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::prepare_close() {
  // Explicitly disconnect from the splitter change event as it sends unwanted change notifications
  // when controls are freed on shutdown.
  _splitter_connection.disconnect();

  if (_schema_side_bar)
    bec::GRTManager::get()->set_app_option("DbSqlEditor:SidebarCollapseState",
                                           grt::StringRef(_schema_side_bar->get_collapse_states()));

  int tab = _task_tabview->get_active_tab();
  bec::GRTManager::get()->set_app_option("DbSqlEditor:ActiveTaskTab", grt::IntegerRef(tab));
  tab = _info_tabview->get_active_tab();
  bec::GRTManager::get()->set_app_option("DbSqlEditor:ActiveInfoTab", grt::IntegerRef(tab));
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::schema_row_selected() {
  std::list<mforms::TreeNodeRef> nodes;
  std::string details;

  if (_schema_side_bar) {
    Color textColor = Color::getSystemColor(base::SystemColor::LabelColor);

    nodes = _schema_side_bar->get_schema_tree()->get_selection();
    if (nodes.empty() || nodes.size() > 1)
      textColor.alpha = 0.75;

    details = "<html><body style='padding:15px; font-family:";
    details += DEFAULT_FONT_FAMILY;
    details += "; font-size: 9pt; color: " + textColor.to_html() + ";'>";
    if (nodes.empty()) {
      details += "<b>No object selected</b>";
    } else if (nodes.size() > 1) {
      details += "<b>Multiple selected objects</b>";
    } else {
      // When there's a single node selected, gets the details and tells it to notify if changes in it occur.
      details += _schema_tree->get_field_description(nodes.front());
      _schema_tree->set_notify_on_reload(nodes.front());
    }
    details += "</body></html>";

    _object_info->set_markup_text(details);

    // send out notification about selection change
    grt::DictRef info(true);
    info.gset("selection-size", (int)nodes.size());
    grt::GRTNotificationCenter::get()->send_grt("GRNLiveDBObjectSelectionDidChange",
                                                _owner->wbsql()->get_grt_editor_object(_owner), info);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::side_bar_filter_changed(const std::string &filter) {
  if (filter.length() > 0)
    _schema_tree = &_filtered_schema_tree;
  else
    _schema_tree = &_base_schema_tree;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::sidebar_splitter_changed() {
  int pos = _side_splitter->get_divider_position();
  if (pos > 0)
    bec::GRTManager::get()->set_app_option("DbSqlEditor:SidebarInitialSplitterPos", grt::IntegerRef(pos));
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorTreeController::fetch_data_for_filter(
  const std::string &schema_filter, const std::string &object_filter,
  const wb::LiveSchemaTree::NewSchemaContentArrivedSlot &arrived_slot) {
  std::string wb_internal_schema = bec::GRTManager::get()->get_app_option_string("workbench:InternalSchema");

  sql::Dbc_connection_handler::Ref conn;

  RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

  InternalSchema internal_schema(wb_internal_schema, conn);

  // Validates the remote search is available
  bool remote_search_enabled = internal_schema.is_remote_search_deployed();

  if (!remote_search_enabled) {
    if (mforms::Utilities::show_message(
          _("Search Objects in Server"),
          base::strfmt(_("To enable searching objects in the remote server, MySQL Workbench needs to create a stored "
                         "procedure in a custom schema (%s)."),
                       wb_internal_schema.c_str()),
          _("Create"), _("Cancel")) == 1) {
      // Performs the deployment, any error will be returned
      std::string error = internal_schema.deploy_remote_search();

      if (!error.length())
        remote_search_enabled = true;
      else {
        std::string userName = _owner->connection_descriptor()->parameterValues().get_string("userName");
        std::string msgFmt =
          _("The user %s has no privileges to create the required schema and stored procedures "
            "to enable remote search in this server. \n"
            "Ensure your database administrator creates a schema for internal use of MySQL Workbench"
            " with full privileges for the user %s, once created configure it in "
            "Preferences->General->Internal Workbench Schema and retry.\n\n%s.");

        std::string message = base::strfmt(msgFmt.c_str(), userName.c_str(), userName.c_str(), error.c_str());

        mforms::Utilities::show_error("Search Objects in Server", message, "Ok");
      }
    }
  }

  // If the remote search is available performs the search
  if (remote_search_enabled) {
    bool sync = !bec::GRTManager::get()->in_main_thread();
    logDebug3("Fetch data for filter %s.%s\n", schema_filter.c_str(), object_filter.c_str());

    live_schema_fetch_task->exec(sync, std::bind(&SqlEditorTreeController::do_fetch_data_for_filter, this,
                                                 weak_ptr_from(this), schema_filter, object_filter, arrived_slot));
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<std::string> SqlEditorTreeController::fetch_schema_list() {
  static std::set<std::string> systemSchemaNames{"information_schema", "performance_schema", "mysql"};

  std::vector<std::string> schemata_names;
  try {
    sql::Dbc_connection_handler::Ref conn;

    RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

    bool showSystemSchemas = bec::GRTManager::get()->get_app_option_int("DbSqlEditor:ShowMetadataSchemata", 0) != 0;

    std::unique_ptr<sql::ResultSet> rs(conn->ref->getMetaData()->getSchemata());
    while (rs->next()) {
      std::string name = rs->getString(1);
      if (name[0] == '.')
        continue;
      if (showSystemSchemas || systemSchemaNames.count(name) == 0)
        schemata_names.push_back(name);
    }
  }
  CATCH_ANY_EXCEPTION_AND_DISPATCH(_("Get schemata"))
  return schemata_names;
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorTreeController::fetch_schema_contents(
  const std::string &schema_name, const wb::LiveSchemaTree::NewSchemaContentArrivedSlot &arrived_slot) {
  // in windows we use TreeViewAdv feature to expand nodes asynchronously
  // that is this function is already called from a separate thread
  // and it must have items loaded when it returns.
  bool sync = !bec::GRTManager::get()->in_main_thread();
  logDebug3("Fetch schema contents for %s\n", schema_name.c_str());

  live_schema_fetch_task->exec(sync, std::bind(&SqlEditorTreeController::do_fetch_live_schema_contents, this,
                                               weak_ptr_from(this), schema_name, arrived_slot));

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::refresh_live_object_in_overview(wb::LiveSchemaTree::ObjectType type,
                                                              const std::string schema_name,
                                                              const std::string old_obj_name,
                                                              const std::string new_obj_name) {
  try {
    // update schema tree even if no object was added/dropped, to clear details attribute which contents might to be
    // changed
    _schema_tree->update_live_object_state(type, schema_name, old_obj_name, new_obj_name);
  }
  CATCH_ANY_EXCEPTION_AND_DISPATCH_TO_DEFAULT_LOG(_("Refresh live schema object"))
}

//----------------------------------------------------------------------------------------------------------------------

mforms::View *SqlEditorTreeController::get_sidebar() {
  return _side_splitter;
}

//----------------------------------------------------------------------------------------------------------------------

grt::StringRef SqlEditorTreeController::do_fetch_live_schema_contents(
  std::weak_ptr<SqlEditorTreeController> self_ptr, const std::string &schema_name,
  wb::LiveSchemaTree::NewSchemaContentArrivedSlot arrived_slot) {
  RETVAL_IF_FAIL_TO_RETAIN_WEAK_PTR(SqlEditorTreeController, self_ptr, self, grt::StringRef(""))
  try {
    StringListPtr tables(new std::list<std::string>());
    StringListPtr views(new std::list<std::string>());
    StringListPtr procedures(new std::list<std::string>());
    StringListPtr functions(new std::list<std::string>());

    MutexLock schema_contents_mutex(_schema_contents_mutex);
    if (!arrived_slot)
      return grt::StringRef("");

    {
      sql::Dbc_connection_handler::Ref conn;
      RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));
      std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());

      {
        std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
        std::unique_ptr<sql::ResultSet> rs(
          stmt->executeQuery(std::string(sqlstring("SHOW FULL TABLES FROM !", 0) << schema_name)));
        while (rs->next()) {
          std::string name = rs->getString(1);
          std::string type = rs->getString(2);

          if (type == "VIEW")
            views->push_back(name);
          else
            tables->push_back(name);
        }
      }
        {
          std::unique_ptr<sql::ResultSet> rs(
            stmt->executeQuery(std::string(sqlstring("SHOW PROCEDURE STATUS WHERE Db=?", 0) << schema_name)));

          while (rs->next()) {
            std::string name = rs->getString(2);
            procedures->push_back(name);
          }
        }
        {
          std::unique_ptr<sql::ResultSet> rs(
            stmt->executeQuery(std::string(sqlstring("SHOW FUNCTION STATUS WHERE Db=?", 0) << schema_name)));
          while (rs->next()) {
            std::string name = rs->getString(2);
            functions->push_back(name);
          }
        }
    }

    if (arrived_slot) {
      std::function<void()> schema_contents_arrived =
        std::bind(arrived_slot, schema_name, tables, views, procedures, functions, false);
      bec::GRTManager::get()->run_once_when_idle(this, schema_contents_arrived);
    }

    // Let the owner form know we got fresh schema meta data. Can be used to update caches.
    _owner->schema_meta_data_refreshed(schema_name, tables, views, procedures, functions);
  } catch (const sql::SQLException &e) {
    _owner->add_log_message(DbSqlEditorLog::ErrorMsg, strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()),
                            "Error loading schema content", "");
    logError("SQLException executing %s: %s\n", std::string("Error loading schema content").c_str(),
             strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()).c_str());

    if (arrived_slot) {
      StringListPtr empty_list;
      std::function<void()> schema_contents_arrived =
        std::bind(arrived_slot, schema_name, empty_list, empty_list, empty_list, empty_list, false);
      bec::GRTManager::get()->run_once_when_idle(this, schema_contents_arrived);
    }
  }

  return grt::StringRef("");
}

//----------------------------------------------------------------------------------------------------------------------

grt::StringRef SqlEditorTreeController::do_fetch_data_for_filter(
  std::weak_ptr<SqlEditorTreeController> self_ptr, const std::string &schema_filter, const std::string &object_filter,
  wb::LiveSchemaTree::NewSchemaContentArrivedSlot arrived_slot) {
  RETVAL_IF_FAIL_TO_RETAIN_WEAK_PTR(SqlEditorTreeController, self_ptr, self, grt::StringRef(""))

  logDebug3("Searching data for %s.%s\n", schema_filter.c_str(), object_filter.c_str());

  std::shared_ptr<sql::ResultSet> dbc_resultset;
  std::map<std::string, int> schema_directory;
  std::string last_schema;

  std::string wb_internal_schema = bec::GRTManager::get()->get_app_option_string("workbench:InternalSchema");

  try {
    // Creates the template for the sqlstring
    std::string procedure(base::sqlstring("CALL !.SEARCH_OBJECTS(?,?,0)", 0) << wb_internal_schema << schema_filter
                                                                             << object_filter);

    // Gets the data
    std::string error = _owner->fetch_data_from_stored_procedure(procedure, dbc_resultset);

    if (dbc_resultset && !error.length()) {
      StringListPtr tables(new std::list<std::string>());
      StringListPtr views(new std::list<std::string>());
      StringListPtr procedures(new std::list<std::string>());
      StringListPtr functions(new std::list<std::string>());

      // Creates the needed schema/objects
      while (dbc_resultset->next()) {
        std::string schema = dbc_resultset->getString(1);
        std::string object = dbc_resultset->getString(2);
        std::string type = dbc_resultset->getString(3);

        // A schema change occurred, need to create the structure for the data loaded so far
        if (schema != last_schema && last_schema != "") {
          if (arrived_slot)
            bec::GRTManager::get()->run_once_when_idle(
              this, std::bind(arrived_slot, last_schema, tables, views, procedures, functions, true));

          tables->clear();
          views->clear();
          procedures->clear();
          functions->clear();
        }

        last_schema = schema;

        if (type == "T")
          tables->push_back(object);
        else if (type == "V")
          views->push_back(object);
        else if (type == "P")
          procedures->push_back(object);
        else
          functions->push_back(object);
      }

      if (last_schema != "" && arrived_slot)
        bec::GRTManager::get()->run_once_when_idle(
          this, std::bind(arrived_slot, last_schema, tables, views, procedures, functions, true));
    } else {
      std::string userName = _owner->connection_descriptor()->parameterValues().get_string("userName");

      std::string msgFmt =
        _("The user %s has no privileges on %s to create temporal tables or execute required stored procedures "
          "used in remote search in this server.\n"
          "Ensure your database administrator grants you full access to the schema %s and retry.\n\n%s.");

      std::string message = base::strfmt(msgFmt.c_str(), userName.c_str(), wb_internal_schema.c_str(),
                                         wb_internal_schema.c_str(), error.c_str());

      mforms::Utilities::show_error(_("Search Objects in Server"), message, _("Ok"));
    }
  }

  CATCH_ANY_EXCEPTION_AND_DISPATCH_TO_DEFAULT_LOG(_("Get data for filter"))

  return grt::StringRef("");
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::fetch_column_data(const std::string &schema_name, const std::string &obj_name,
                                                wb::LiveSchemaTree::ObjectType type,
                                                const wb::LiveSchemaTree::NodeChildrenUpdaterSlot &updater_slot) {
  // Searches for the target node...
  mforms::TreeNodeRef node = _schema_tree->get_node_for_object(schema_name, type, obj_name);
  LiveSchemaTree::ViewData *pdata = NULL;

  if (node)
    pdata = dynamic_cast<LiveSchemaTree::ViewData *>(node->get_data());

  // Loads the information...
  StringListPtr columns(new std::list<std::string>);
  std::map<std::string, LiveSchemaTree::ColumnData> column_data;

  logDebug3("Fetching column data for %s.%s\n", schema_name.c_str(), obj_name.c_str());

  try {
    sql::Dbc_connection_handler::Ref conn;

    RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

    std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
    std::unique_ptr<sql::ResultSet> rs(
      stmt->executeQuery(std::string(base::sqlstring("SHOW FULL COLUMNS FROM !.!", 0) << schema_name << obj_name)));

    while (rs->next()) {
      LiveSchemaTree::ColumnData col_node(type);
      std::string column_name = rs->getString(1);

      columns->push_back(column_name);

      std::string type = rs->getString(2);
      std::string collation = rs->isNull(3) ? "" : rs->getString(3);
      std::string nullable = rs->getString(4);
      std::string key = rs->getString(5);
      std::string default_value = rs->getString(6);
      std::string extra = rs->getString(7);

      base::replaceStringInplace(type, "unsigned", "UN");

      if (extra == "auto_increment")
        type += " AI";

      col_node.name = column_name;
      col_node.type = type;
      col_node.charset_collation = collation;
      col_node.is_pk = key == "PRI";
      col_node.is_id = (col_node.is_pk || (nullable == "NO" && key == "UNI"));
      col_node.is_idx = key != "";
      col_node.default_value = default_value;

      column_data[column_name] = col_node;
    }

    // If information was found, creates the TreeNode structure for it
    if (columns->size()) {
      // Creates the node if it didn't exist...
      if (!node) {
        node = _schema_tree->create_node_for_object(schema_name, type, obj_name);

        if (node)
          pdata = dynamic_cast<LiveSchemaTree::ViewData *>(node->get_data());
        else
          logWarning("Error fetching column information for '%s'.'%s'\n", schema_name.c_str(), obj_name.c_str());
      }

      if (pdata) {
        // Identifies the node that will be the parent for the loaded columns...
        mforms::TreeNodeRef target_parent;
        if (pdata->get_type() == LiveSchemaTree::Table) {
          target_parent = node->get_child(wb::LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX);
          type = LiveSchemaTree::TableColumn;
        } else if (pdata->get_type() == LiveSchemaTree::View) {
          target_parent = node;
          type = LiveSchemaTree::ViewColumn;
        }

        if (target_parent) {
          updater_slot(target_parent, columns, type, false, false);

          for (int index = 0; index < target_parent->count(); index++) {
            mforms::TreeNodeRef child = target_parent->get_child(index);
            LiveSchemaTree::LSTData *pchilddata = dynamic_cast<LiveSchemaTree::LSTData *>(child->get_data());
            LiveSchemaTree::LSTData *psource = &column_data[child->get_string(0)];
            pchilddata->copy(psource);
          }

          pdata->columns_load_error = false;
          pdata->set_loaded_data(LiveSchemaTree::COLUMN_DATA);
          _schema_tree->notify_on_reload(target_parent);
        }
      }
    }
  } catch (const sql::SQLException &exc) {
    logWarning("Error fetching column information for '%s'.'%s': %s\n", schema_name.c_str(), obj_name.c_str(),
               exc.what());

    // Sets flag indicating error loading columns ( Used for broken views )
    if (pdata) {
      if (exc.getErrorCode() == 1356)
        pdata->columns_load_error = true;

      pdata->details = exc.what();

      _schema_tree->update_node_icon(node);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::fetch_trigger_data(const std::string &schema_name, const std::string &obj_name,
                                                 wb::LiveSchemaTree::ObjectType type,
                                                 const wb::LiveSchemaTree::NodeChildrenUpdaterSlot &updater_slot) {
  // Loads the information...
  StringListPtr triggers(new std::list<std::string>);
  //  std::list<std::string> triggers;
  std::map<std::string, LiveSchemaTree::TriggerData> trigger_data_dict;

  try {
    sql::Dbc_connection_handler::Ref conn;

    RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

    std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
    std::unique_ptr<sql::ResultSet> rs(
      stmt->executeQuery(std::string(base::sqlstring("SHOW TRIGGERS FROM ! LIKE ?", 0) << schema_name << obj_name)));

    while (rs->next()) {
      wb::LiveSchemaTree::TriggerData trigger_node;

      std::string name = rs->getString(1);
      trigger_node.event_manipulation = wb::LiveSchemaTree::internalize_token(rs->getString(2));
      trigger_node.timing = wb::LiveSchemaTree::internalize_token(rs->getString(5));

      triggers->push_back(name);
      trigger_data_dict[name] = trigger_node;
    }

    // If information was found, creates the TreeNode structure for it
    // Searches for the target node...
    mforms::TreeNodeRef node = _schema_tree->get_node_for_object(schema_name, type, obj_name);

    // Creates the node if it didn't exist...
    if (!node)
      node = _schema_tree->create_node_for_object(schema_name, type, obj_name);

    // Identifies the node that will be the parent for the loaded columns...
    mforms::TreeNodeRef target_parent = node->get_child(wb::LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX);
    updater_slot(target_parent, triggers, LiveSchemaTree::Trigger, false, false);

    for (int index = 0; index < target_parent->count(); index++) {
      mforms::TreeNodeRef child = target_parent->get_child(index);
      LiveSchemaTree::LSTData *pchilddata = dynamic_cast<LiveSchemaTree::LSTData *>(child->get_data());
      LiveSchemaTree::LSTData *psource = &trigger_data_dict[child->get_string(0)];
      pchilddata->copy(psource);
    }

    // Where there data or not the triggers were loaded
    LiveSchemaTree::ViewData *pdata = dynamic_cast<LiveSchemaTree::ViewData *>(node->get_data());
    pdata->set_loaded_data(LiveSchemaTree::TRIGGER_DATA);
    _schema_tree->notify_on_reload(target_parent);
  } catch (const sql::SQLException &exc) {
    logWarning("Error fetching trigger information for '%s'.'%s': %s\n", schema_name.c_str(), obj_name.c_str(),
              exc.what());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::fetch_index_data(const std::string &schema_name, const std::string &obj_name,
                                               wb::LiveSchemaTree::ObjectType type,
                                               const wb::LiveSchemaTree::NodeChildrenUpdaterSlot &updater_slot) {
  // Loads the information...
  StringListPtr indexes(new std::list<std::string>());
  std::map<std::string, LiveSchemaTree::IndexData> index_data_dict;

  try {
    sql::Dbc_connection_handler::Ref conn;

    RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

    std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
    std::unique_ptr<sql::ResultSet> rs(
      stmt->executeQuery(std::string(base::sqlstring("SHOW INDEXES FROM !.!", 0) << schema_name << obj_name)));

    bool supportVisibility = _owner->rdbms_version().is_valid() && is_supported_mysql_version_at_least(_owner->rdbms_version(), 8, 0, 0);

    while (rs->next()) {
      LiveSchemaTree::IndexData index_data;

      std::string name = rs->getString(3);

      // Inserts the index to the list
      if (!index_data_dict.count(name)) {
        indexes->push_back(name);

        index_data.type = wb::LiveSchemaTree::internalize_token(rs->getString(11));
        index_data.unique = (rs->getInt(2) == 0);
        if (supportVisibility) {
          index_data.visible = rs->getString(14) == "YES";
        }

        index_data_dict[name] = index_data;
      }

      // Adds the column
      index_data_dict[name].columns.push_back(rs->getString(5));
    }

    // Searches for the target node...
    mforms::TreeNodeRef node = _schema_tree->get_node_for_object(schema_name, type, obj_name);

    // Creates the node if it didn't exist...
    if (!node)
      node = _schema_tree->create_node_for_object(schema_name, type, obj_name);

    // Identifies the node that will be the parent for the loaded indexes...
    mforms::TreeNodeRef target_parent = node->get_child(wb::LiveSchemaTree::TABLE_INDEXES_NODE_INDEX);
    updater_slot(target_parent, indexes, LiveSchemaTree::Index, false, false);

    for (int index = 0; index < target_parent->count(); index++) {
      mforms::TreeNodeRef child = target_parent->get_child(index);
      LiveSchemaTree::LSTData *pchilddata = dynamic_cast<LiveSchemaTree::LSTData *>(child->get_data());
      LiveSchemaTree::LSTData *psource = &index_data_dict[child->get_string(0)];
      pchilddata->copy(psource);
    }

    LiveSchemaTree::ViewData *pdata = dynamic_cast<LiveSchemaTree::ViewData *>(node->get_data());
    pdata->set_loaded_data(LiveSchemaTree::INDEX_DATA);
    _schema_tree->notify_on_reload(target_parent);
  } catch (const sql::SQLException &exc) {
    logWarning("Error fetching index information for '%s'.'%s': %s\n", schema_name.c_str(), obj_name.c_str(), exc.what());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::fetch_foreign_key_data(const std::string &schema_name, const std::string &obj_name,
                                                     wb::LiveSchemaTree::ObjectType type,
                                                     const wb::LiveSchemaTree::NodeChildrenUpdaterSlot &updater_slot) {
  StringListPtr foreign_keys(new std::list<std::string>());
  std::map<std::string, LiveSchemaTree::FKData> fk_data_dict;

  sql::Dbc_connection_handler::Ref conn;

  RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

  try {
    std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
    std::unique_ptr<sql::ResultSet> rs(
      stmt->executeQuery(std::string(base::sqlstring("SHOW CREATE TABLE !.!", 0) << schema_name << obj_name)));

    while (rs->next()) {
      std::string statement = rs->getString(2);

      size_t def_start = statement.find("(");
      size_t def_end = statement.rfind(")");

      std::vector<std::string> def_lines = base::split(statement.substr(def_start, def_end - def_start), "\n");

      const std::string pattern =
        "CONSTRAINT\\s*(\\S*)\\s*FOREIGN KEY\\s*\\((\\S*)\\)\\s*REFERENCES\\s*(\\S*)\\s*\\((\\S*)\\)\\s*((\\w*\\s*)*),?$";

      std::regex selfRegex(pattern, std::regex_constants::icase);

      std::string fk_name;
      std::string fk_columns;
      std::string fk_ref_table;
      std::string fk_ref_columns;
      std::string fk_rules;

      for (size_t index = 0; index < def_lines.size(); index++) {
        auto resultBegin = std::sregex_iterator(def_lines[index].begin(), def_lines[index].end(), selfRegex);
        auto resultEnd = std::sregex_iterator();
        auto count = std::distance(resultBegin, resultEnd);
        if (count > 0) {
          
          for (auto iter = resultBegin; iter != resultEnd; ++iter) {
            std::smatch match = *iter;
            for (int i = 0; i < match.length(); ++i) {
              switch (i) {
                case 1:
                  fk_name = match[i].str();
                  fk_name = base::unquote_identifier(fk_name);
                  break;
                case 2:
                  fk_columns = match[i].str();
                  break;
                case 3:
                  fk_ref_table = match[i].str();
                  fk_ref_table = base::unquote_identifier(fk_ref_table);
                  ;
                  break;
                case 4:
                  fk_ref_columns = match[i].str();
                  break;
                case 5:
                  fk_rules = match[i].str();
                  break;
                default:
                  break;
              }

            }
          }
          
          // Parses the list fields
          std::vector<std::string> fk_column_list = base::split(fk_columns, ",");
          std::vector<std::string> fk_ref_column_list = base::split(fk_ref_columns, ",");
          std::vector<std::string> fk_rule_tokens = base::split(fk_rules, " ");

          // Create the foreign key node
          wb::LiveSchemaTree::FKData new_fk;
          foreign_keys->push_back(fk_name);
          new_fk.referenced_table = (fk_ref_table);

          // Set the default update and delete rules
          new_fk.update_rule = new_fk.delete_rule = wb::LiveSchemaTree::internalize_token("RESTRICT");

          // A rule has at least 3 tokens so the number of tokens could be
          // 3 or 4 for 1 rule and 6,7,8 for two rules, so we get the number with this
          size_t rule_count = fk_rule_tokens.size() / 3;

          int token_offset = 0;
          for (size_t index = 0; index < rule_count; index++) {
            // Skips the ON token
            token_offset++;

            // Gets the UPDATE/DELETE token
            std::string rule = fk_rule_tokens[token_offset++];

            // Gets the action
            std::string action = fk_rule_tokens[token_offset++];

            if (action == "SET" || action == "NO")
              action += " " + fk_rule_tokens[token_offset++];

            const unsigned char value = wb::LiveSchemaTree::internalize_token(action);

            if (rule == "UPDATE")
              new_fk.update_rule = value;
            else
              new_fk.delete_rule = value;
          }

          std::string from(""), to("");
          for (size_t column_index = 0; column_index < fk_column_list.size(); column_index++) {
            std::string from_col = base::unquote_identifier(fk_column_list[column_index]);
            std::string to_col = base::unquote_identifier(fk_ref_column_list[column_index]);

            if (from.empty())
              from = from_col;
            else
              from.append(", ").append(from_col);
            if (to.empty())
              to = to_col;
            else
              to.append(", ").append(to_col);
          }
          new_fk.from_cols = from;
          new_fk.to_cols = to;
          fk_data_dict[fk_name] = new_fk;
        }
      }
    }

    // Searches for the target node...
    mforms::TreeNodeRef node = _schema_tree->get_node_for_object(schema_name, type, obj_name);

    // Creates the node if it didn't exist...
    if (!node)
      node = _schema_tree->create_node_for_object(schema_name, type, obj_name);

    // Identifies the node that will be the parent for the loaded columns...
    mforms::TreeNodeRef target_parent = node->get_child(wb::LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX);
    updater_slot(target_parent, foreign_keys, LiveSchemaTree::ForeignKey, false, false);

    for (int index = 0; index < target_parent->count(); index++) {
      mforms::TreeNodeRef child = target_parent->get_child(index);
      LiveSchemaTree::LSTData *pchilddata = dynamic_cast<LiveSchemaTree::LSTData *>(child->get_data());
      LiveSchemaTree::LSTData *psource = &fk_data_dict[child->get_string(0)];
      pchilddata->copy(psource);
    }

    LiveSchemaTree::ViewData *pdata = dynamic_cast<LiveSchemaTree::ViewData *>(node->get_data());
    pdata->set_loaded_data(LiveSchemaTree::FK_DATA);
    _schema_tree->notify_on_reload(target_parent);
  } catch (const sql::SQLException &exc) {
    logWarning("Error fetching foreign key information for '%s'.'%s': %s\n", schema_name.c_str(), obj_name.c_str(),
              exc.what());
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorTreeController::fetch_object_details(const std::string &schema_name, const std::string &object_name,
                                                   wb::LiveSchemaTree::ObjectType type, short flags,
                                                   const wb::LiveSchemaTree::NodeChildrenUpdaterSlot &updater_slot) {
  // If the type has not been specified, pulls it from the database
  // Most of the time the type will be specified as it can bee retrieved from
  // the LST, this is to handle the case when a query is executed using direct SQL
  // and the tree hasn't been populated with the target object
  if (type == wb::LiveSchemaTree::Any)
    type = fetch_object_type(schema_name, object_name);

  if (type != wb::LiveSchemaTree::Any) {
    if (flags & wb::LiveSchemaTree::COLUMN_DATA)
      fetch_column_data(schema_name, object_name, type, updater_slot);

    if (flags & wb::LiveSchemaTree::INDEX_DATA)
      fetch_index_data(schema_name, object_name, type, updater_slot);

    if (flags & wb::LiveSchemaTree::TRIGGER_DATA)
      fetch_trigger_data(schema_name, object_name, type, updater_slot);

    if (flags & wb::LiveSchemaTree::FK_DATA)
      fetch_foreign_key_data(schema_name, object_name, type, updater_slot);
  }

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorTreeController::fetch_routine_details(const std::string &schema_name, const std::string &obj_name,
                                                    wb::LiveSchemaTree::ObjectType type) {
  bool ret_val = false;
  std::string object = type == LiveSchemaTree::Function ? "FUNCTION" : "PROCEDURE";
  std::string statement = "SHOW CREATE " + object + " !.!";
  // Loads the information...
  std::list<std::string> indexes;
  std::map<std::string, LiveSchemaTree::IndexData> index_data_dict;

  try {
    sql::Dbc_connection_handler::Ref conn;

    RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

    std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
    std::unique_ptr<sql::ResultSet> rs(
      stmt->executeQuery(std::string(base::sqlstring(statement.c_str(), 0) << schema_name << obj_name)));

    if (rs->next()) {
      LiveSchemaTree::IndexData index_data;

      std::string ddl = rs->getString(3);

      SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms(_owner->rdbms());
      SqlFacade::String_tuple_list parameters;
      std::string ddl_type, ddl_name, ddl_ret, ddl_comments;
      ddl = "DELIMITER " + bec::GRTManager::get()->get_app_option_string("SqlDelimiter", "$$") + "\n" + ddl;
      sql_facade->parseRoutineDetails(ddl, ddl_type, ddl_name, parameters, ddl_ret, ddl_comments);

      std::string details = "";
      if (parameters.size()) {
        details = _("<b>Parameters:</b>");
        details += "<table border=0>";

        SqlFacade::String_tuple_list::iterator index, end = parameters.end();
        for (index = parameters.begin(); index != end; index++) {
          details += "<tr><td style=\"border:none; padding-left: 15px;\">" + index->first + ":</td>";
          details +=
            "<td style=\"border:none; padding-left: 15px;\"><font color='#717171'>" + index->second + "</td></tr>";
        }

        details += "</table>";
      }

      if (!ddl_ret.empty())
        details += "<br><br><b><font color='#000000'>Returns: </b><font color='#717171'>" + ddl_ret;

      if (!ddl_comments.empty())
        details += "<br><br><b><font color='#000000'>Comments: </b><font color='#717171'>" + ddl_comments;

      LiveSchemaTree::ObjectData new_data;
      new_data.details = details; // ddl.substr(start, end - start);
      new_data.fetched = true;

      // Searches for the target node...
      mforms::TreeNodeRef node = _schema_tree->get_node_for_object(schema_name, type, obj_name);

      // The node should exist as this method is only called for a selected node
      if (node) {
        LiveSchemaTree::ObjectData *ptargetdata = dynamic_cast<LiveSchemaTree::ObjectData *>(node->get_data());
        ptargetdata->copy(&new_data);
        ret_val = true;
      }
    }
  } catch (const sql::SQLException &exc) {
    logWarning("Error fetching routine information for '%s'.'%s': %s\n", schema_name.c_str(), obj_name.c_str(),
              exc.what());
  }

  return ret_val;
}

//----------------------------------------------------------------------------------------------------------------------

wb::LiveSchemaTree::ObjectType SqlEditorTreeController::fetch_object_type(const std::string &schema_name,
                                                                          const std::string &obj_name) {
  wb::LiveSchemaTree::ObjectType type = wb::LiveSchemaTree::Any;

  try {
    MutexLock schema_contents_mutex(_schema_contents_mutex);

    {
      sql::Dbc_connection_handler::Ref conn;
      RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

      {
        std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
        std::unique_ptr<sql::ResultSet> rs(
          stmt->executeQuery(std::string(sqlstring("SHOW FULL TABLES FROM ! LIKE ?", 0) << schema_name << obj_name)));
        while (rs->next()) {
          std::string str_type = rs->getString(2);

          if (str_type == "VIEW")
            type = wb::LiveSchemaTree::View;
          else
            type = wb::LiveSchemaTree::Table;
        }
      }
    }
  }
  CATCH_ANY_EXCEPTION_AND_DISPATCH_TO_DEFAULT_LOG(_("Get schema contents"));

  return type;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::tree_refresh() {
  if (_owner->connected()) {
    live_schemata_refresh_task->exec(false, std::bind((grt::StringRef(SqlEditorTreeController::*)(SqlEditorForm::Ptr)) &
                                                        SqlEditorTreeController::do_refresh_schema_tree_safe,
                                                      this, weak_ptr_from(_owner)));
    _schema_tree->set_enabled(true);
  } else {
    _schema_tree->set_no_connection();
    _schema_tree->set_enabled(false);
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorTreeController::sidebar_action(const std::string &name) {
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Activate one or more objects. The term "activate" is a bit misleading as we do other operations too
 * (like clipboard handling).
 */
void SqlEditorTreeController::tree_activate_objects(const std::string &action,
                                                    const std::vector<wb::LiveSchemaTree::ChangeRecord> &changes) {
  // Most of the activations should lead to a single result (e.g. all clipboard ops go into one string).
  std::string action_modifier; // action can contain prefix denoting action modifier
  std::string real_action =
    action; // if action modifier is present real_action will store part after w/o modifier prefix

  if (real_action == "select_data_columns") {
    typedef std::string TableName;
    typedef std::string ColumnName;
    std::map<TableName, std::map<ColumnName, std::string> > table_column_types;
    std::string text;
    typedef std::map<TableName, std::string> TableStringMap;
    TableStringMap first_text;
    TableStringMap second_text;

    // cache default values for tables
    for (size_t i = 0; i < changes.size(); i++) {
      TableName full_table_name = sqlstring("!.!", 0) << changes[i].schema << changes[i].name;
      std::string column_type;

      if (!first_text[full_table_name].empty())
        first_text[full_table_name].append(", ");
      first_text[full_table_name].append(changes[i].detail);
    }

    for (const TableStringMap::value_type &table_columns : first_text)
      text += strfmt("SELECT %s\nFROM %s;\n", table_columns.second.c_str(), table_columns.first.c_str());

    _owner->run_sql_in_scratch_tab(text, false, true);
  }

  try {
    for (size_t i = 0; i < changes.size(); i++) {
      std::string sql;
      switch (changes[i].type) {
        case LiveSchemaTree::Schema:
          if (real_action == "filter") {
            _schema_side_bar->get_filter_entry()->set_value(changes[i].name);
            (*_schema_side_bar->get_filter_entry()->signal_changed())();
          } else if (real_action == "inspect")
            _owner->inspect_object(changes[i].name, "", "db.Schema");
          else if (real_action == "alter")
            do_alter_live_object(LiveSchemaTree::Schema, changes[i].name, changes[i].name);
          else
            _owner->active_schema(changes[i].name);
          break;

        case LiveSchemaTree::Table:
          if (real_action == "activate" || real_action == "edit_data" || real_action == "select_data")
            sql = sqlstring("SELECT * FROM !.!;", base::QuoteOnlyIfNeeded) << changes[i].schema << changes[i].name;
          else if (real_action == "inspect")
            _owner->inspect_object(changes[i].schema, changes[i].name, "db.Table");
          else if (real_action == "alter")
            do_alter_live_object(changes[i].type, changes[i].schema, changes[i].name);
          break;

        case LiveSchemaTree::View:
          if (real_action == "activate" || real_action == "edit_data" || real_action == "select_data")
            sql = sqlstring("SELECT * FROM !.!;", base::QuoteOnlyIfNeeded) << changes[i].schema << changes[i].name;
          else if (real_action == "inspect")
            _owner->inspect_object(changes[i].schema, changes[i].name, "db.View");
          else if (real_action == "alter")
            do_alter_live_object(changes[i].type, changes[i].schema, changes[i].name);
          break;

        case LiveSchemaTree::Procedure:
        case LiveSchemaTree::Function:
          if (real_action == "alter")
            do_alter_live_object(changes[i].type, changes[i].schema, changes[i].name);
          else if (real_action == "execute")
            sql = run_execute_routine_wizard(changes[i].type, changes[i].schema, changes[i].name);
          break;

        default:
          break;
      }

      if (!sql.empty()) {
        bool _autosave = _owner->get_autosave_disabled();
        _owner->set_autosave_disabled(true);
        SqlEditorPanel *ed = _owner->run_sql_in_scratch_tab(sql, false, true);
        if (ed)
          ed->set_title(changes[i].name);
        _owner->set_autosave_disabled(_autosave);
      }
    }
  } catch (std::exception &exc) {
    mforms::Utilities::show_error("Error", exc.what(), "OK");
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Convenience API for the activation interface.
 */
void SqlEditorTreeController::schema_object_activated(const std::string &action, wb::LiveSchemaTree::ObjectType type,
                                                      const std::string &schema, const std::string &name) {
  std::vector<wb::LiveSchemaTree::ChangeRecord> changes;
  wb::LiveSchemaTree::ChangeRecord record = {type, schema, name, ""};
  changes.push_back(record);
  tree_activate_objects(action, changes);
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::do_alter_live_object(wb::LiveSchemaTree::ObjectType type, const std::string &schema_name,
                                                   const std::string &aobj_name) {
  std::string used_schema_name = schema_name;
  std::string obj_name = aobj_name;
  try {
    db_mgmt_RdbmsRef rdbms = _owner->rdbms();
    // std::string database_package= *rdbms->databaseObjectPackage();

    if (rdbms.is_valid()) {
      rdbms = grt::shallow_copy_object(rdbms);
      rdbms->version(grt::shallow_copy_object(_owner->rdbms_version()));
      rdbms->version()->owner(rdbms);
    }

    // reset_references on the catalog is called when we try to apply changes (generate the alter script).
    db_mysql_CatalogRef client_state_catalog = grt::GRT::get()->create_object<db_mysql_Catalog>("db.mysql.Catalog");
    client_state_catalog->name("default");
    client_state_catalog->oldName("default");
    client_state_catalog->version(rdbms->version());
    grt::replace_contents(client_state_catalog->simpleDatatypes(), rdbms->simpleDatatypes());
    grt::replace_contents(client_state_catalog->characterSets(), rdbms->characterSets());
    // XXX this should be changed when/if global userDatatypes are added
    // XXX    grt::replace_contents(client_state_catalog->userDatatypes(),
    // XXX workbench_physical_ModelRef::cast_from(_live_physical_overview->get_model())->catalog()->userDatatypes());

    db_mysql_SchemaRef schema;
    if (wb::LiveSchemaTree::Schema != type) {
      if (used_schema_name == "")
        used_schema_name = _owner->active_schema();

      if (used_schema_name == "") {
        mforms::Utilities::show_warning(
          strfmt(_("No Schema Selected")),
          _("A default schema must be set by double clicking its name in the SCHEMA list."), _("OK"));

        return;
      } else {
        schema = grt::GRT::get()->create_object<db_mysql_Schema>("db.mysql.Schema");
        schema->owner(client_state_catalog);

        schema->name(used_schema_name);
        schema->oldName(used_schema_name);
        client_state_catalog->schemata().insert(schema);
        client_state_catalog->defaultSchema(schema);
      }
    }

    bool is_object_new = obj_name.empty();

    std::string ddl_script;
    std::string sql_mode;
    if (!is_object_new) {
      // parse selected object DDL into auxiliary catalog
      ddl_script = get_object_ddl_script(type, used_schema_name, obj_name);
      if (ddl_script.empty()) {
        logWarning("Unable to get DDL for %s.%s\n", used_schema_name.c_str(), obj_name.c_str());
        return;
      }
      {
        sql::Dbc_connection_handler::Ref conn;
        RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));
        if (conn)
          _owner->get_session_variable(conn->ref.get(), "sql_mode", sql_mode);
      }

      // if this is a View, then auto-reformat it before sending it to parser/editor
      if (type == wb::LiveSchemaTree::View &&
          bec::GRTManager::get()->get_app_option_int("DbSqlEditor:ReformatViewDDL", 0)) {
        try {
          grt::Module *module = grt::GRT::get()->get_module("SQLIDEUtils");
          grt::BaseListRef args(true);
          args.ginsert(grt::StringRef(ddl_script));
          ddl_script = grt::StringRef::cast_from(module->call_function("reformatSQLStatement", args));
        } catch (std::exception &exc) {
          logWarning("Error reformatting view code: %s\n", exc.what());
        }
      }

      if (!parse_ddl_into_catalog(client_state_catalog, strfmt("`%s`.`%s`", schema_name.c_str(), obj_name.c_str()),
                                  ddl_script, sql_mode, schema_name)) {
        logWarning("Error parsing DDL for %s.%s: %s\n", schema_name.c_str(), obj_name.c_str(), ddl_script.c_str());
        return;
      }
    }

    // Make a copy of the catalog before we modify it with new/edited objects,
    // so we can later do a diff between both to find changes.
    db_CatalogRef server_state_catalog = grt::copy_object(client_state_catalog);

    // "reset_references" for the created object is called when the base editor is destroyed.
    db_DatabaseObjectRef db_object;
    switch (type) {
      case wb::LiveSchemaTree::Schema:
        db_object = is_object_new
                      ? create_new_schema(client_state_catalog)
                      : db_SchemaRef::cast_from(find_named_object_in_list(client_state_catalog->schemata(), obj_name));
        break;
      case wb::LiveSchemaTree::Table:
        db_object = is_object_new ? create_new_table(schema)
                                  : db_TableRef::cast_from(find_named_object_in_list(schema->tables(), obj_name));
        break;
      case wb::LiveSchemaTree::View:
        db_object = is_object_new ? create_new_view(schema)
                                  : db_ViewRef::cast_from(find_named_object_in_list(schema->views(), obj_name));
        break;
      case wb::LiveSchemaTree::Procedure:
      case wb::LiveSchemaTree::Function:
        db_object = is_object_new ? create_new_routine(schema, type)
                                  : db_RoutineRef::cast_from(find_named_object_in_list(schema->routines(), obj_name));
        break;
      default:
        break;
    }

    if (db_object.is_valid()) {
      db_object->customData().set("sqlMode", grt::StringRef(sql_mode));
      db_object->customData().set("originalObjectDDL", grt::StringRef(ddl_script));
      open_alter_object_editor(db_object, server_state_catalog);
    } else
      logError("Failed to create/alter `%s`.`%s`", used_schema_name.c_str(), obj_name.c_str());
  } catch (const std::exception &e) {
    logError("Failed to create/alter `%s`.`%s`: %s", used_schema_name.c_str(), obj_name.c_str(), e.what());
    mforms::Utilities::show_error(
      strfmt(_("Failed to create/alter `%s`.`%s`"), used_schema_name.c_str(), obj_name.c_str()), e.what(), _("OK"));
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::open_alter_object_editor(db_DatabaseObjectRef object,
                                                       db_CatalogRef server_state_catalog) {
  db_CatalogRef client_state_catalog;
  if (db_SchemaRef::can_wrap(object)) {
    if (!object->owner().is_valid())
      throw std::invalid_argument("schema object does not have owner set to expected value");
    client_state_catalog = db_CatalogRef::cast_from(object->owner());
  } else {
    if (!object->owner().is_valid())
      throw std::invalid_argument("object does not have owner set to expected schema value");
    if (!object->owner()->owner().is_valid())
      throw std::invalid_argument("object does not have owner set to expected schema value");
    client_state_catalog = db_CatalogRef::cast_from(object->owner()->owner());
  }

  sql::Dbc_connection_handler::Ref conn;
  grt::NormalizedComparer comparer;
  {
    RecMutexLock lock(_owner->ensure_valid_aux_connection(conn));
    // db_object->customData().set("CaseSensitive",grt::IntegerRef(conn->ref->getMetaData()->storesMixedCaseIdentifiers()));
    // TODO use DB_Plugin here somehow
    comparer.load_db_options(conn->ref->getMetaData());
  }

  db_mgmt_RdbmsRef rdbms = _owner->rdbms();
  // std::string database_package= *rdbms->databaseObjectPackage();

  if (rdbms.is_valid()) {
    rdbms = grt::shallow_copy_object(rdbms);
    rdbms->version(grt::shallow_copy_object(_owner->rdbms_version()));
    rdbms->version()->owner(rdbms);
  }

  if (!client_state_catalog->version().is_valid())
    client_state_catalog->version(rdbms->version());
  if (!server_state_catalog->version().is_valid())
    server_state_catalog->version(rdbms->version());

  object->customData().set("DBSettings", comparer.get_options_dict());
  object->customData().set("liveRdbms", _owner->rdbms());
  object->customData().set("ownerSqlEditor", _owner->wbsql()->get_grt_editor_object(_owner));

  object->customData().set("clientStateCatalog", client_state_catalog);
  object->customData().set("serverStateCatalog", server_state_catalog);

  // TODO: make docking/non-docking switchable via preferences.
  //_context_ui->get_wb()->open_object_editor(db_object, bec::StandaloneWindowFlag);
  bec::GRTManager::get()->open_object_editor(object, bec::ForceNewWindowFlag);
}

//----------------------------------------------------------------------------------------------------------------------

std::string SqlEditorTreeController::run_execute_routine_wizard(wb::LiveSchemaTree::ObjectType type,
                                                                const std::string &schema_name,
                                                                const std::string &obj_name) {
  std::pair<std::string, std::string> script = get_object_create_script(type, schema_name, obj_name);
  if (script.second.empty())
    return ""; // get_object_create_script() already showed an error.

  db_mysql_RoutineRef routine(grt::Initialized);
  parsers::MySQLParserServices::Ref services = parsers::MySQLParserServices::get();

  db_mysql_CatalogRef catalog(grt::Initialized);
  catalog->version(_owner->rdbms_version());
  grt::replace_contents(catalog->simpleDatatypes(), _owner->rdbms()->simpleDatatypes());

  db_mysql_SchemaRef schema(grt::Initialized);
  schema->owner(catalog);
  schema->name(schema_name);
  catalog->schemata().insert(schema);

  routine->owner(schema);
  schema->routines().insert(routine);

  std::string previous_sql_mode;
  std::string sql_mode = _owner->work_parser_context()->sqlMode();
  if (!script.first.empty()) {
    previous_sql_mode = sql_mode;
    sql_mode = script.first;
    _owner->work_parser_context()->updateSqlMode(script.first);
  }

  size_t error_count = services->parseRoutine(_owner->work_parser_context(), routine, script.second);

  if (!previous_sql_mode.empty())
    _owner->work_parser_context()->updateSqlMode(previous_sql_mode);

  if (error_count > 0) {
    logWarning("Error parsing SQL code for %s.%s:\n%s\n", schema_name.c_str(), obj_name.c_str(), script.second.c_str());

    std::vector<ParserErrorInfo> errors = _owner->work_parser_context()->errorsWithOffset(0);
    mforms::Utilities::show_error(_("Error parsing sql code for object"), errors[0].message, "OK");
    return "";
  }

  ExecuteRoutineWizard wizard(routine, sql_mode);
  wizard.center();
  return wizard.run();
}

//----------------------------------------------------------------------------------------------------------------------

db_SchemaRef SqlEditorTreeController::create_new_schema(db_CatalogRef owner) {
  db_SchemaRef object = grt::GRT::get()->create_object<db_Schema>(owner->schemata()->content_type_spec().object_class);
  object->owner(owner);
  object->name("new_schema");
  owner->schemata().insert(object);
  owner->defaultSchema(object);
  return object;
}

//----------------------------------------------------------------------------------------------------------------------

db_TableRef SqlEditorTreeController::create_new_table(db_SchemaRef owner) {
  db_TableRef object = grt::GRT::get()->create_object<db_Table>(owner->tables()->content_type_spec().object_class);
  object->owner(owner);
  object->name("new_table");
  owner->tables().insert(object);
  return object;
}

//----------------------------------------------------------------------------------------------------------------------

db_ViewRef SqlEditorTreeController::create_new_view(db_SchemaRef owner) {
  db_ViewRef object = grt::GRT::get()->create_object<db_View>(owner->views()->content_type_spec().object_class);
  object->owner(owner);
  object->name("new_view");
  owner->views().insert(object);
  return object;
}

//----------------------------------------------------------------------------------------------------------------------

db_RoutineRef SqlEditorTreeController::create_new_routine(db_SchemaRef owner, wb::LiveSchemaTree::ObjectType type) {
  db_RoutineRef object =
    grt::GRT::get()->create_object<db_Routine>(owner->routines()->content_type_spec().object_class);
  object->owner(owner);

  if (type == wb::LiveSchemaTree::Procedure) {
    object->name("new_procedure");
    object->routineType("procedure");
  } else if (type == wb::LiveSchemaTree::Function) {
    object->name("new_function");
    object->routineType("function");
  }

  owner->routines().insert(object);
  return object;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::tree_create_object(wb::LiveSchemaTree::ObjectType type, const std::string &schema_name,
                                                 const std::string &obj_name) {
  do_alter_live_object(type, schema_name, obj_name);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Generates an alter script for the given db object using the specified online DDL options.
 * These are however only applied if the server version is >= 5.6.
 */
std::string SqlEditorTreeController::generate_alter_script(const db_mgmt_RdbmsRef &rdbms,
                                                           db_DatabaseObjectRef db_object, std::string algorithm,
                                                           std::string lock) {
  DbMySQLImpl *diffsql_module = grt::GRT::get()->find_native_module<DbMySQLImpl>("DbMySQL");

  db_CatalogRef server_cat = db_CatalogRef::cast_from(db_object->customData().get("serverStateCatalog"));
  db_CatalogRef client_cat = db_CatalogRef::cast_from(db_object->customData().get("clientStateCatalog"));

  db_CatalogRef client_cat_copy = db_CatalogRef::cast_from(grt::copy_object(client_cat));
  db_CatalogRef server_cat_copy = db_CatalogRef::cast_from(grt::copy_object(server_cat));

  grt::DictRef diff_options(true);
  // diff_options.set("CaseSensitive",db_object->customData().get("CaseSensitive"));
  grt::DictRef db_settings = grt::DictRef::cast_from(db_object->customData().get("DBSettings"));
  if (_owner->rdbms_version().is_valid() && is_supported_mysql_version_at_least(_owner->rdbms_version(), 5, 6)) {
    db_settings.gset("AlterAlgorithm", algorithm != "DEFAULT" ? algorithm : "");
    db_settings.gset("AlterLock", lock != "DEFAULT" ? lock : "");
  }
  diff_options.set("DBSettings", db_settings);

  std::string alter_script =
    diffsql_module->makeAlterScriptForObject(server_cat_copy, client_cat_copy, db_object, diff_options);
  client_cat_copy->reset_references();
  server_cat_copy->reset_references();

  return alter_script;
}

//----------------------------------------------------------------------------------------------------------------------

std::string SqlEditorTreeController::get_object_ddl_script(wb::LiveSchemaTree::ObjectType type,
                                                           const std::string &schema_name,
                                                           const std::string &obj_name) {
  std::string delimiter =  bec::GRTManager::get()->get_app_option_string("SqlDelimiter", "$$");
  std::string ddl_script = "delimiter " + delimiter + "\n\n";

  // Triggers are fetched prior to table ddl, but should appear after table created.
  std::string additional_ddls;

  try {
    sql::Dbc_connection_handler::Ref conn;
    std::string query;

    RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

    // Can't use getSchemaObjects() because it silently ignores errors.
    switch (type) {
      case wb::LiveSchemaTree::Schema:
        query = base::sqlstring("SHOW CREATE SCHEMA !", 0) << obj_name;
        break;

      case wb::LiveSchemaTree::Table: {
        // triggers
        std::vector<std::string> triggers;
        {
          std::string trigger_query = base::sqlstring("SHOW TRIGGERS FROM ! WHERE ! = ?", 0) << schema_name << "Table"
                                                                                             << obj_name;
          std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
          std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(trigger_query));

          if (rs.get()) {
            while (rs->next())
              triggers.push_back(rs->getString(1));
          }
        }

        for (size_t index = 0; index < triggers.size(); index++) {
          std::string trigger_query = base::sqlstring("SHOW CREATE TRIGGER !.!", 0) << schema_name << triggers[index];
          std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
          std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(trigger_query));

          if (rs.get() && rs->next()) {
            std::string trigger_ddl = (rs->getString(3));
            additional_ddls += trigger_ddl;
            additional_ddls += delimiter + "\n\n";
          }
        }
      }

        query = base::sqlstring("SHOW CREATE TABLE !.!", 0) << schema_name << obj_name;
        break;

      case wb::LiveSchemaTree::View:
        query = base::sqlstring("SHOW CREATE VIEW !.!", 0) << schema_name << obj_name;
        break;

      case wb::LiveSchemaTree::Procedure:
        query = base::sqlstring("SHOW CREATE PROCEDURE !.!", 0) << schema_name << obj_name;
        break;

      case wb::LiveSchemaTree::Function:
        query = base::sqlstring("SHOW CREATE FUNCTION !.!", 0) << schema_name << obj_name;
        break;

      default:
        break;
    }

    std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
    std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(query));

    // Note: show create procedure includes the sql mode in the result before the actual DDL.
    if (rs.get() && rs->next()) {
      if (type == wb::LiveSchemaTree::Function || type == wb::LiveSchemaTree::Procedure)
        ddl_script += rs->getString(3) + delimiter + "\n\n";
      else
        ddl_script += rs->getString(2) + delimiter + "\n\n";
    }
    ddl_script += additional_ddls;
  } catch (const sql::SQLException &e) {
    // Error 1356 comes up when any of the referenced tables in a view are invalid (e.g. dropped)
    // or the definer/invoker has no rights to access it.
    // Solve this by using the I_S.
    if (type == wb::LiveSchemaTree::View && e.getErrorCode() == 1356) {
      sql::Dbc_connection_handler::Ref conn;
      std::string query, view;
      RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));
      query = base::sqlstring(
                "SELECT DEFINER, SECURITY_TYPE, VIEW_DEFINITION FROM INFORMATION_SCHEMA.VIEWS WHERE TABLE_SCHEMA = ? "
                "AND TABLE_NAME = ?",
                0)
              << schema_name << obj_name;
      std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
      std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(query));

      if (rs.get() && rs->next()) {
        std::string view, definer;
        std::vector<std::string> definer_tokens = base::split(rs->getString(1), "@", 2);

        view = base::sqlstring("!.!", 0) << schema_name << obj_name;
        definer = base::sqlstring("!@!", 0) << definer_tokens[0] << definer_tokens[1];
        ddl_script += "CREATE ALGORITHM=UNDEFINED DEFINER=" + definer;
        ddl_script += " SQL SECURITY " + rs->getString(2);
        ddl_script += " VIEW " + view + " AS ";
        ddl_script += rs->getString(3) + delimiter + "\n\n";
      }
    } else {
      ddl_script.clear();
      std::string err = e.what();
      logError("Error getting SQL definition for %s.%s: %s\n", schema_name.c_str(), obj_name.c_str(), e.what());
      if (bec::GRTManager::get()->in_main_thread())
        mforms::Utilities::show_error("Error getting DDL for object", e.what(), "OK", "", "");
      else
        bec::GRTManager::get()->run_once_when_idle(
          std::bind(&mforms::Utilities::show_error, "Error getting DDL for object", err, "OK", "", ""));
    }
  }
  return ddl_script;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Retrieves the original DDL text that was used to create the object.
 * Returns a tuple of <sql_mode, script>. The sql mode is what was used to create the object,
 * if it is a routine. Otherwise this value is empty.
 */
std::pair<std::string, std::string> SqlEditorTreeController::get_object_create_script(
  wb::LiveSchemaTree::ObjectType type, const std::string &schema_name, const std::string &obj_name) {
  std::pair<std::string, std::string> result;

  try {
    sql::Dbc_connection_handler::Ref conn;
    std::string query;

    RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

    // cant use getSchemaObjects() because it silently ignores errors
    switch (type) {
      case wb::LiveSchemaTree::Schema:
        query = base::sqlstring("SHOW CREATE SCHEMA !", 0) << obj_name;
        break;

      case wb::LiveSchemaTree::Table:
        query = base::sqlstring("SHOW CREATE TABLE !.!", 0) << schema_name << obj_name;
        break;

      case wb::LiveSchemaTree::View:
        query = base::sqlstring("SHOW CREATE VIEW !.!", 0) << schema_name << obj_name;
        break;

      case wb::LiveSchemaTree::Procedure:
        query = base::sqlstring("SHOW CREATE PROCEDURE !.!", 0) << schema_name << obj_name;
        break;

      case wb::LiveSchemaTree::Function:
        query = base::sqlstring("SHOW CREATE FUNCTION !.!", 0) << schema_name << obj_name;
        break;

      default:
        break;
    }

    std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
    std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(query));

    if (rs.get() && rs->next()) {
      if (type == wb::LiveSchemaTree::Function || type == wb::LiveSchemaTree::Procedure) {
        result.first = rs->getString(2);
        result.second = rs->getString(3);
      } else
        result.second = rs->getString(2);
    }
  } catch (const sql::SQLException &e) {
    if (type == wb::LiveSchemaTree::View && e.getErrorCode() == 1356) {
      // Error for not being allowed to run SHOW CREATE VIEW. Use I_S instead to get the code.
      sql::Dbc_connection_handler::Ref conn;
      std::string query, view;
      RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));
      query = base::sqlstring(
                "SELECT DEFINER, SECURITY_TYPE, VIEW_DEFINITION FROM INFORMATION_SCHEMA.VIEWS WHERE TABLE_SCHEMA = ? "
                "AND TABLE_NAME = ?",
                0)
              << schema_name << obj_name;
      std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
      std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(query));

      if (rs.get() && rs->next()) {
        std::string view, definer;
        std::vector<std::string> definer_tokens = base::split(rs->getString(1), "@", 2);

        view = base::sqlstring("!.!", 0) << schema_name << obj_name;
        definer = base::sqlstring("!@!", 0) << definer_tokens[0] << definer_tokens[1];
        result.second = "CREATE ALGORITHM = UNDEFINED DEFINER = " + definer;
        result.second += " SQL SECURITY " + rs->getString(2);
        result.second += " VIEW " + view + " AS\n";
        result.second += rs->getString(3);
      }
    } else {
      logError("Error getting SQL definition for %s.%s: %s\n", schema_name.c_str(), obj_name.c_str(), e.what());
      mforms::Utilities::show_error("Error getting DDL for object", e.what(), "OK");
    }
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 *	Returns a list of trigger create scripts for the given table.
 */
std::vector<std::string> SqlEditorTreeController::get_trigger_sql_for_table(const std::string &schema_name,
                                                                            const std::string &table_name) {
  std::vector<std::string> result;

  try {
    sql::Dbc_connection_handler::Ref conn;
    RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

    std::vector<std::string> triggers;
    {
      std::string trigger_query = base::sqlstring("SHOW TRIGGERS FROM ! WHERE ! = ?", 0) << schema_name << "Table"
                                                                                         << table_name;
      std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
      std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(trigger_query));

      if (rs.get()) {
        while (rs->next())
          triggers.push_back(rs->getString(1));
      }
    }

    for (size_t index = 0; index < triggers.size(); index++) {
      std::string trigger_query = base::sqlstring("SHOW CREATE TRIGGER !.!", 0) << schema_name << triggers[index];
      std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
      std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(trigger_query));

      if (rs.get() && rs->next())
        result.push_back(rs->getString(3));
    }
  } catch (const sql::SQLException &e) {
    logError("Error getting SQL definition for %s.%s: %s\n", schema_name.c_str(), table_name.c_str(), e.what());
    mforms::Utilities::show_error("Error getting DDL for object", e.what(), "OK");
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::refresh_live_object_in_editor(bec::DBObjectEditorBE *obj_editor, bool using_old_name) {
  db_DatabaseObjectRef db_object = obj_editor->get_dbobject();

  db_mysql_CatalogRef client_state_catalog =
    db_mysql_CatalogRef::cast_from(db_object->customData().get("clientStateCatalog"));

  std::string obj_name = using_old_name ? db_object->oldName() : db_object->name();
  // don't refresh new objects that where not applied yet
  if (obj_name.empty())
    return;
  obj_editor->freeze_refresh_on_object_change();

  if (obj_name != *db_object->name())
    db_object->name(obj_name);
  obj_editor->thaw_refresh_on_object_change(true);

  std::string schema_name = db_SchemaRef::can_wrap(db_object) ? std::string() : *db_object->owner()->name();
  db_SchemaRef schema;
  if (!schema_name.empty())
    schema = db_SchemaRef::cast_from(db_object->owner());

  wb::LiveSchemaTree::ObjectType db_object_type = wb::LiveSchemaTree::Any;

  if (db_SchemaRef::can_wrap(db_object)) {
    db_object_type = wb::LiveSchemaTree::Schema;
  } else {
    if (db_TableRef::can_wrap(db_object)) {
      db_object_type = wb::LiveSchemaTree::Table;

      // reset selection of fkeys/indices to avoid warnings
      bec::TableEditorBE *table_editor = dynamic_cast<bec::TableEditorBE *>(obj_editor);
      table_editor->get_fks()->select_fk(NodeId());
      table_editor->get_indexes()->select_index(NodeId());
    } else if (db_ViewRef::can_wrap(db_object)) {
      db_object_type = wb::LiveSchemaTree::View;
    } else if (db_RoutineRef::can_wrap(db_object)) {
      db_RoutineRef db_routine = db_RoutineRef::cast_from(db_object);
      std::string obj_type = db_routine->routineType();

      if (obj_type == "function")
        db_object_type = wb::LiveSchemaTree::Function;
      else
        db_object_type = wb::LiveSchemaTree::Procedure;
    }
  }

  obj_editor->freeze_refresh_on_object_change();
  client_state_catalog->schemata().remove_all(); // Clean up. We only want the single object in it, which we edit.

  // reparse object's DDL
  std::string ddl_script;
  std::string sql_mode;
  {
    ddl_script = get_object_ddl_script(db_object_type, schema_name, obj_name);
    if (!ddl_script.empty()) {
      if (db_object_type == wb::LiveSchemaTree::View &&
          bec::GRTManager::get()->get_app_option_int("DbSqlEditor:ReformatViewDDL", 0)) {
        try {
          grt::Module *module = grt::GRT::get()->get_module("SQLIDEUtils");
          grt::BaseListRef args(true);
          args.ginsert(grt::StringRef(ddl_script));
          ddl_script = grt::StringRef::cast_from(module->call_function("reformatSQLStatement", args));
        } catch (std::exception &exc) {
          logWarning("Error reformatting view code: %s\n", exc.what());
        }
      }

      try {
        sql::Dbc_connection_handler::Ref conn;
        RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));
        if (conn)
          _owner->get_session_variable(conn->ref.get(), "sql_mode", sql_mode);
      }
      CATCH_ANY_EXCEPTION_AND_DISPATCH(_("Get 'sql_mode' session variable"));

      parse_ddl_into_catalog(client_state_catalog, strfmt("`%s`.`%s`", schema_name.c_str(), obj_name.c_str()),
                             ddl_script, sql_mode, schema_name);
    }
  }

  // Settings dict here only to copy it to the new db object.
  grt::DictRef dbSettings = grt::DictRef::cast_from(db_object->customData().get("DBSettings"));
  switch (db_object_type) {
    case wb::LiveSchemaTree::Table:
      if ((client_state_catalog->schemata()->count() > 0) &&
          (client_state_catalog->schemata()[0]->tables()->count() > 0))
        db_object = client_state_catalog->schemata()[0]->tables()[0];
      break;
    case wb::LiveSchemaTree::View:
      if ((client_state_catalog->schemata()->count() > 0) &&
          (client_state_catalog->schemata()[0]->views()->count() > 0))
        db_object = client_state_catalog->schemata()[0]->views()[0];
      break;
    case wb::LiveSchemaTree::Procedure:
    case wb::LiveSchemaTree::Function:
      if ((client_state_catalog->schemata()->count() > 0) &&
          (client_state_catalog->schemata()[0]->routines()->count() > 0))
        db_object = client_state_catalog->schemata()[0]->routines()[0];
      break;
    default: // wb::LiveSchemaTree::Schema, there are more cases, but we only use those listed here.
      if (client_state_catalog->schemata()->count() > 0)
        db_object = client_state_catalog->schemata()[0];
      break;
  }

  // The current name becomes now also the old name, so that we create alter scripts from the current name,
  // not the one from before this change.
  db_object->oldName(obj_name);

  {
    db_CatalogRef server_state_catalog(db_CatalogRef::cast_from(grt::copy_object(client_state_catalog)));
    db_object->customData().set("serverStateCatalog", server_state_catalog);
    db_object->customData().set("clientStateCatalog", client_state_catalog);
  }
  db_object->customData().set("originalObjectDDL", grt::StringRef(ddl_script));
  db_object->customData().set("sqlMode", grt::StringRef(sql_mode));

  db_object->customData().set("DBSettings", dbSettings);
  db_object->customData().set("liveRdbms", _owner->rdbms());
  db_object->customData().set("ownerSqlEditor", _owner->wbsql()->get_grt_editor_object(_owner));

  obj_editor->thaw_refresh_on_object_change();

  // Update the editor's grt object (the one it is managing), as this has now been
  // recreated. Will cause a UI refresh as well. If there was a parse error (which can only be
  // an internal error, since we start freshly) stay with the old object.
  obj_editor->set_object(db_object);

  // Enable refresh of sql editor contents.
  MySQLEditor::Ref active_sql_editor;
  if (obj_editor->has_editor())
    active_sql_editor = obj_editor->get_sql_editor();
  if (active_sql_editor)
    active_sql_editor->set_refresh_enabled(true);
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorTreeController::parse_ddl_into_catalog(db_mysql_CatalogRef catalog, const std::string &objectDescription,
                                                     const std::string &sql, std::string sqlMode,
                                                     const std::string &schema) {
  std::string currentSqlMode = _owner->work_parser_context()->sqlMode();

  grt::DictRef options(true);
  options.set("reuse_existing_objects", grt::IntegerRef(1));
  options.set("schema", grt::StringRef(schema));

  if (!sqlMode.empty())
    _owner->work_parser_context()->updateSqlMode(sqlMode);

  parsers::MySQLParserServices::Ref services = parsers::MySQLParserServices::get();
  size_t errorCount = services->parseSQLIntoCatalog(_owner->work_parser_context(), catalog, sql, options);

  bool haveErrors = false;

  if (options.has_key("sql_mode") && (errorCount > 0)) {
    if (sqlMode.find("ANSI_QUOTES") != std::string::npos)
      sqlMode = base::replaceString(sqlMode, "ANSI_QUOTES", "");
    else
      sqlMode += ", ANSI_QUOTES";
    _owner->work_parser_context()->updateSqlMode(sqlMode);

    errorCount = services->parseSQLIntoCatalog(_owner->work_parser_context(), catalog, sql, options);
    _owner->work_parser_context()->updateSqlMode(currentSqlMode);

    if (errorCount == 0) { // Error(s) solved by new sql mode -> inconsistency.
      if (mforms::Utilities::show_warning(
            strfmt(_("Error Parsing DDL for %s"), objectDescription.c_str()),
            _("The object's DDL retrieved from the server is inconsistent with respect to the SQL_MODE variable "
              "set for the connection. In particular the current state of the ANSI_QUOTES flag contradicts "
              "the value set when the object had been created. This may lead to errors when trying to "
              "apply changes. As a workaround you may want to temporarily change the SQL_MODE variable "
              "to its previous value.\nDo you want to view the DDL or cancel processing it?"),
            _("View DDL"), _("Cancel")) == mforms::ResultOk) {
        _owner->new_sql_scratch_area();
        insert_text_to_active_editor(sql);
      }
      return false;
    } else
      haveErrors = true;
  } else
    haveErrors = errorCount > 0;

  _owner->work_parser_context()->updateSqlMode(currentSqlMode);
  if (haveErrors) {
    if (mforms::Utilities::show_error(strfmt(_("Error Parsing DDL for %s"), objectDescription.c_str()),
                                      _("There was an error while parsing the DDL retrieved from the server.\n"
                                        "Do you want to view the DDL or cancel processing it?"),
                                      _("View DDL"), _("Cancel")) == mforms::ResultOk) {
      _owner->new_sql_scratch_area();
      insert_text_to_active_editor(sql);
    }
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorTreeController::apply_changes_to_object(bec::DBObjectEditorBE *obj_editor, bool dry_run) {
  std::string log_descr;
  RowId log_id = -1;
  if (!dry_run) {
    log_descr = strfmt(_("Apply changes to %s"), obj_editor->get_name().c_str());
    log_id = _owner->add_log_message(DbSqlEditorLog::BusyMsg, "Applying object changes ...", log_descr, "");
  }
  try {
    if (!dry_run && obj_editor->has_editor() && obj_editor->get_sql_editor()->has_sql_errors()) {
      // We won't be able to apply with errors. Inform the user and return false.
      mforms::Utilities::show_error(_("Apply Changes to Object"),
                                    _("The object's DDL statement contains syntax errors.\n"
                                    "You cannot modify this object until you fix the errors."),
                                    _("OK"));
      return false;
    }

    db_DatabaseObjectRef db_object = obj_editor->get_dbobject();

    if (!dry_run) {
      ValueRef hasErrors = db_object->customData().get("triggerInvalid");
      if (hasErrors.is_valid() && (IntegerRef::cast_from(hasErrors) != 0)) {
        int res = mforms::Utilities::show_warning(_("Apply Changes to Object"),
                                                  _("The tables's trigger SQL code contains errors.\n"
                                                    "This will lead to invalid sql generated.\n"
                                                    "Are you sure you want to apply the DDL statement as is?"),
                                                  _("Yes"), _("No"));

        if (res != mforms::ResultOk) {
          _owner->set_log_message(log_id, DbSqlEditorLog::ErrorMsg, "Cancelled", log_descr, "");
          return false;
        }
      }

      // check for name conflicts
      // if the object is new or its name was changed
      std::string obj_name = db_object->name();
      std::string obj_old_name = db_object->oldName();
      if (_owner->lower_case_table_names() != 0) // if 1 or 2, treat everything as case insensitive
      {
        obj_name = tolower(obj_name);
        obj_old_name = tolower(obj_old_name);
        if (_owner->lower_case_table_names() == 1 && obj_name != *db_object->name() &&
            (db_TableRef::can_wrap(db_object) || db_ViewRef::can_wrap(db_object) ||
             db_SchemaRef::can_wrap(db_object))) // server will force everything to be lowercase
        {
          mforms::Utilities::show_message_and_remember(
            "Apply Changes to Object",
            base::strfmt("The server is configured with lower_case_table_names=1, which only allows lowercase "
                         "characters in schema and table names.\nThe object will be created as `%s`.",
                         obj_name.c_str()),
            "OK", "", "", "sqlide:lower_case_table_names", "");
          db_object->name(obj_name);
        }
      }
      // now here
      if (obj_name != obj_old_name) {
        std::list<std::string> obj_types;
        std::list<std::string> validation_queries;

        std::string schema_name = db_SchemaRef::can_wrap(db_object) ? std::string() : *db_object->owner()->name();

        {
          if (db_SchemaRef::can_wrap(db_object)) {
            obj_types.push_back("schema");
            validation_queries.push_back(sqlstring("SHOW DATABASES LIKE ?", 0) << db_object->name());
          } else if (db_TableRef::can_wrap(db_object) || db_ViewRef::can_wrap(db_object)) {
            obj_types.push_back("table");
            obj_types.push_back("view");
            std::string tables_format = base::strfmt(
              "SHOW FULL TABLES FROM ! WHERE `Tables_in_%s` = ? AND Table_type != 'VIEW'", schema_name.c_str());
            std::string views_format = base::strfmt(
              "SHOW FULL TABLES FROM ! WHERE `Tables_in_%s` = ? AND Table_type = 'VIEW'", schema_name.c_str());

            validation_queries.push_back(sqlstring(tables_format.c_str(), 0) << schema_name << db_object->name());
            validation_queries.push_back(sqlstring(views_format.c_str(), 0) << schema_name << db_object->name());
          } else if (db_RoutineRef::can_wrap(db_object)) {
            db_RoutineRef db_routine = db_RoutineRef::cast_from(db_object);

            std::string type = db_routine->routineType();
            if (type == "function")
              validation_queries.push_back(sqlstring("SHOW FUNCTION STATUS WHERE Db=? AND NAME = ?", 0)
                                           << schema_name << db_routine->name());
            else
              validation_queries.push_back(sqlstring("SHOW PROCEDURE STATUS WHERE Db=? AND NAME = ?", 0)
                                           << schema_name << db_routine->name());

            obj_types.push_back(type);
          }
        }

        sql::Dbc_connection_handler::Ref conn;

        RecMutexLock lock(_owner->ensure_valid_aux_connection(conn));

        for (const std::string &obj_type : obj_types) {
          std::string query = validation_queries.front();
          validation_queries.pop_front();

          std::unique_ptr<sql::ResultSet> rs(conn->ref->createStatement()->executeQuery(query));
          if (rs->next()) {
            mforms::Utilities::show_error(_("Apply Changes to Object"),
                                          strfmt(_("Selected name conflicts with existing %s `%s`."), obj_type.c_str(),
                                                 (*db_object->name()).c_str()),
                                          _("OK"));
            _owner->set_log_message(log_id, DbSqlEditorLog::ErrorMsg,
                                    strfmt(_("Selected name conflicts with existing %s `%s`."), obj_type.c_str(),
                                           (*db_object->name()).c_str()),
                                    log_descr, "");
            return false;
          }
        }
      }
    }

    // Generate the initial version of the alter script. This might be altered in the wizard
    // depending on the online DDL options.
    std::string algorithm;
    wb::WBContextUI::get()->get_wb_options_value("", "DbSqlEditor:OnlineDDLAlgorithm", algorithm);
    std::string lock;
    wb::WBContextUI::get()->get_wb_options_value("", "DbSqlEditor:OnlineDDLLock", lock);
    std::string alter_script = generate_alter_script(_owner->rdbms(), db_object, algorithm, lock);

    // The alter_script may contain a dummy USE statement.
    if (alter_script.empty() ||
        (alter_script.find("CREATE") == std::string::npos && alter_script.find("ALTER") == std::string::npos &&
         alter_script.find("DROP") == std::string::npos)) {
      if (!dry_run && !_owner->on_sql_script_run_error.empty())
        _owner->on_sql_script_run_error(log_id, _("No changes to object were detected."), "");
      if (!dry_run) {
        _owner->set_log_message(log_id, DbSqlEditorLog::NoteMsg, _("No changes detected"), log_descr, "");

        // Because of message throttling the previous log message doesn't cause a refresh of the UI
        // (it comes quicker than the throttling timeout). So we explicitly do a refresh.
        _owner->refresh_log_messages(true);
      }
      return false; // no changes detected
    }

    if (dry_run)
      return true; // some changes were detected

    return _owner->run_live_object_alteration_wizard(alter_script, obj_editor, log_id, log_descr);
  } catch (const std::exception &e) {
    if (!_owner->on_sql_script_run_error.empty())
      _owner->on_sql_script_run_error(log_id, e.what(), "");
    _owner->set_log_message(log_id, DbSqlEditorLog::ErrorMsg, e.what(), log_descr, "");
    logError("Exception applying changes to live object: %s\n", e.what());
  }
  return true; // some changes were detected and applied
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::create_live_table_stubs(bec::DBObjectEditorBE *table_editor) {
  db_DatabaseObjectRef db_object = table_editor->get_dbobject();
  if (db_object->customData().has_key("isLiveTableListLoaded"))
    return;

  try {
    sql::Dbc_connection_handler::Ref conn;

    RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

    db_CatalogRef catalog = table_editor->get_catalog();
    grt::ListRef<db_Schema> schemata = catalog->schemata();
    db_SchemaRef schema;
    grt::ListRef<db_Table> tables;
    db_TableRef table;

    std::string database_package = *_owner->rdbms()->databaseObjectPackage();
    std::string schema_typename = database_package + ".Schema";
    std::string table_typename = database_package + ".Table";

    std::string prev_schema_name;

    std::shared_ptr<sql::ResultSet> rs;
    {
      std::string schema_name = db_SchemaRef::cast_from(db_object->owner())->name();
      std::list<sql::SQLString> table_types;
      table_types.push_back("TABLE");
      rs.reset(conn->ref->getMetaData()->getTables("", schema_name, "%", table_types));
    }
    while (rs->next()) {
      std::string schema_name = rs->getString(2);
      std::string table_name = rs->getString(3);
      if (prev_schema_name != schema_name) {
        schema = find_named_object_in_list(schemata, schema_name);
        if (!schema.is_valid()) {
          schema = grt::GRT::get()->create_object<db_Schema>(schema_typename);
          schema->owner(catalog);
          schema->name(schema_name);
          schema->oldName(schema_name);
          schema->modelOnly(1);
          schemata.insert(schema);
        }
        tables = schema->tables();
        prev_schema_name = schema_name;
      }
      table = find_named_object_in_list(tables, table_name);
      if (!table.is_valid()) {
        table = grt::GRT::get()->create_object<db_Table>(table_typename);
        table->owner(schema);
        table->name(table_name);
        table->oldName(table_name);
        table->modelOnly(1);
        table->isStub(1);
        tables.insert(table);
      }
    }

    db_object->customData().set("isLiveTableListLoaded", IntegerRef(1));
  }
  CATCH_ANY_EXCEPTION_AND_DISPATCH(_("Create live table stub"));
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorTreeController::expand_live_table_stub(bec::DBObjectEditorBE *table_editor,
                                                     const std::string &schema_name, const std::string &obj_name) {
  db_CatalogRef catalog = table_editor->get_catalog();
  db_TableRef table;
  db_SchemaRef schema = find_named_object_in_list(catalog->schemata(), schema_name);
  if (schema.is_valid()) {
    table = find_named_object_in_list(schema->tables(), obj_name);
    if (table.is_valid() && table->customData().has_key("isStubExpanded"))
      return true; // stub table has already been expanded
  }

  std::string ddl_script = get_object_ddl_script(wb::LiveSchemaTree::Table, schema_name, obj_name);
  if (ddl_script.empty())
    return false;

  {
    SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms(_owner->rdbms());
    Sql_parser::Ref sql_parser = sql_facade->sqlParser();
    sql_parser->messages_enabled(false);
    grt::DictRef options(true);
    {
      std::string sql_mode;
      sql::Dbc_connection_handler::Ref conn;
      RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));
      if (conn && _owner->get_session_variable(conn->ref.get(), "sql_mode", sql_mode))
        options.gset("sql_mode", sql_mode);
      else
        logWarning("Unable to get sql_mode for connection\n");
    }
    db_SchemaRef old_default_schema = catalog->defaultSchema();
    if (!schema.is_valid()) {
      // target schema doesn't exist yet, create a stub for it
      schema = db_mysql_SchemaRef(grt::Initialized);
      schema->owner(catalog);
      schema->name(schema_name);
      schema->comment("stub");
      catalog->schemata().insert(schema);
    }
    catalog->defaultSchema(schema);
    sql_parser->parse_sql_script(catalog, ddl_script, options);
    catalog->defaultSchema(old_default_schema);
  }

  // find parsed table
  if (!schema.is_valid())
    schema = find_named_object_in_list(catalog->schemata(), schema_name);
  if (!table.is_valid() && schema.is_valid())
    table = find_named_object_in_list(schema->tables(), obj_name);

  if (table.is_valid() && table != table_editor->get_dbobject()) {
    table->modelOnly(0);
    table->isStub(1);
    table->customData().set("isStubExpanded", IntegerRef(1));
  }

  return table.is_valid();
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorTreeController::activate_live_object(GrtObjectRef object) {
  std::string obj_name = *object->name();
  std::string owner_name = *object->owner()->name();

  if (db_SchemaRef::can_wrap(object))
    schema_object_activated("activate", LiveSchemaTree::Schema, "", obj_name);
  else if (db_TableRef::can_wrap(object))
    schema_object_activated("activate", LiveSchemaTree::Table, owner_name, obj_name);
  else if (db_ViewRef::can_wrap(object))
    schema_object_activated("activate", LiveSchemaTree::View, owner_name, obj_name);
  else if (db_RoutineRef::can_wrap(object)) {
    db_RoutineRef routine = db_RoutineRef::cast_from(object);
    std::string type = routine->routineType();

    if (type == "function")
      schema_object_activated("activate", LiveSchemaTree::Function, owner_name, obj_name);
    else
      schema_object_activated("activate", LiveSchemaTree::Procedure, owner_name, obj_name);
  } else
    return false;

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::on_active_schema_change(const std::string &schema) {
  _base_schema_tree.set_active_schema(schema);
  _filtered_schema_tree.set_active_schema(schema);

  if (_schema_side_bar != NULL)
    bec::GRTManager::get()->run_once_when_idle(
      this, std::bind(&mforms::View::set_needs_repaint, _schema_side_bar->get_schema_tree()));
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::mark_busy(bool busy) {
  if (_schema_side_bar != NULL)
    _schema_side_bar->mark_section_busy("", busy);
}

//----------------------------------------------------------------------------------------------------------------------

grt::StringRef SqlEditorTreeController::do_refresh_schema_tree_safe(SqlEditorForm::Ptr self_ptr) {
  RETVAL_IF_FAIL_TO_RETAIN_WEAK_PTR(SqlEditorForm, self_ptr, self, grt::StringRef(""))

  if (_is_refreshing_schema_tree)
    return grt::StringRef("");

  _is_refreshing_schema_tree = true;
  StringListPtr schema_list(new std::list<std::string>());

  std::vector<std::string> schemaList = fetch_schema_list();
  _owner->schemaListRefreshed(schemaList);

  schema_list->assign(schemaList.begin(), schemaList.end());
  bec::GRTManager::get()->run_once_when_idle(this,
                                             std::bind(&LiveSchemaTree::update_schemata, _schema_tree, schema_list));
  bec::GRTManager::get()->run_once_when_idle(this, std::bind(&SqlEditorForm::schema_tree_did_populate, _owner));

  _is_refreshing_schema_tree = false;

  return grt::StringRef("");
}

//----------------------------------------------------------------------------------------------------------------------

wb::LiveSchemaTree *SqlEditorTreeController::get_schema_tree() {
  return _schema_tree;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::handle_grt_notification(const std::string &name, grt::ObjectRef sender,
                                                      grt::DictRef info) {
  if (name == "GRNDBObjectEditorCreated") {
    grt::ValueRef object = info.get("object");
    bec::DBObjectEditorBE *editor =
      dynamic_cast<bec::DBObjectEditorBE *>(bec::UIForm::form_with_id(info.get_string("form")));
    if (editor && db_DatabaseObjectRef::can_wrap(object)) {
      db_DatabaseObjectRef obj(db_DatabaseObjectRef::cast_from(object));
      if (obj->customData().get("ownerSqlEditor") == _owner->wbsql()->get_grt_editor_object(_owner)) {
        editor->on_apply_changes_to_live_object = std::bind(&SqlEditorTreeController::apply_changes_to_object, this,
                                                            std::placeholders::_1, std::placeholders::_2);
        editor->on_refresh_live_object =
          std::bind(&SqlEditorTreeController::refresh_live_object_in_editor, this, std::placeholders::_1, true);
        editor->on_create_live_table_stubs =
          std::bind(&SqlEditorTreeController::create_live_table_stubs, this, std::placeholders::_1);
        editor->on_expand_live_table_stub =
          std::bind(&SqlEditorTreeController::expand_live_table_stub, this, std::placeholders::_1,
                    std::placeholders::_2, std::placeholders::_3);
      }
    }
  } else if (name == "GRNSQLEditorReconnected") {
    if (sender == _owner->wbsql()->get_grt_editor_object(_owner)) {
      _session_info->set_markup_text(_owner->get_connection_info());
      tree_refresh();
    }
  } else if (name == "GNColorsChanged") {
    updateColors();
  }
}

//----------------------------------------------------------------------------------------------------------------------

const std::string objectInfoStyles = "<style>"
  "body { font-family: '" DEFAULT_FONT_FAMILY "'; color: »color1«; }"
  "tr.heading { font-weight: 700; }"
  "td.name { color: »color2«; }"
  "td.value { font-weight: 700; color: »color2«;}"
  "table { width: 100%; white-space: nowrap; }"
  "</style>"
;

void SqlEditorTreeController::updateColors() {
  _schema_side_bar->set_selection_color(base::HighlightColor);
  _side_splitter->set_back_color(base::Color::getApplicationColorAsString(AppColorMainBackground, false));

#ifdef _MSC_VER
  _object_info->set_back_color(base::Color::getApplicationColorAsString(base::AppColorPanelContentArea, false));
  _session_info->set_back_color(base::Color::getApplicationColorAsString(base::AppColorPanelContentArea, false));
#elif __APPLE__
  _object_info->set_back_color(base::Color::getSystemColor(base::WindowBackgroundColor).to_html());
  _session_info->set_back_color(base::Color::getSystemColor(base::WindowBackgroundColor).to_html());
  _schema_side_bar->set_back_color(base::Color::getSystemColor(base::WindowBackgroundColor).to_html());
#else
  _object_info->set_back_color("#ebebeb");
  _session_info->set_back_color("#ebebeb");
#endif

  schema_row_selected(); // Refresh object HTML.
  _object_info->set_needs_repaint();

#ifdef __linux__
  _session_info->set_markup_text(_owner->get_connection_info());
#else
  Color textColor = base::Color::getSystemColor(base::LabelColor);
  std::string html = base::replaceString(objectInfoStyles, "»color1«", textColor.to_html());
  textColor.alpha = 0.75;
  base::replaceStringInplace(html, "»color2«", textColor.to_html());
  html = "<html><head>" + html + "</head>";
  html += _owner->get_connection_info() + "</html>";
  _session_info->set_markup_text(html);
#endif

  _session_info->set_needs_repaint();
}

//----------------------------------------------------------------------------------------------------------------------

int SqlEditorTreeController::insert_text_to_active_editor(const std::string &str) {
  SqlEditorPanel *editor(_owner->active_sql_editor_panel());
  if (editor) {
    editor->editor_be()->insert_text(str);
    editor->editor_be()->focus();
  }
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorTreeController::context_menu_will_show(mforms::MenuItem *parent_item) {
  if (!parent_item) {
    grt::DictRef info(true);

    db_query_EditorRef sender(_owner->wbsql()->get_grt_editor_object(_owner));

    grt::ListRef<db_query_LiveDBObject> selection(
      grt::ListRef<db_query_LiveDBObject>::cast_from(_schema_tree->get_selected_objects()));

    info.set("menu", mforms_to_grt(_schema_side_bar->get_context_menu()));
    info.gset("menu-plugins-index", _schema_side_bar->get_context_menu()->get_item_index(
                                      _schema_side_bar->get_context_menu()->find_item("refresh")) -
                                      2);
    info.set("selection", selection);

    grt::GRTNotificationCenter::get()->send_grt("GRNLiveDBObjectMenuWillShow", sender, info);
  }
}

//----------------------------------------------------------------------------------------------------------------------
