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

#include <pcre.h>
#include <boost/foreach.hpp>
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
using namespace parser;

using boost::signals2::scoped_connection;

DEFAULT_LOG_DOMAIN("SqlEditorSchemaTree");

static const char *SQL_EXCEPTION_MSG_FORMAT= _("Error Code: %i\n%s");
static const char *EXCEPTION_MSG_FORMAT= _("Error: %s");

#define CATCH_ANY_EXCEPTION_AND_DISPATCH(statement) \
catch (sql::SQLException &e)\
{\
_owner->add_log_message(DbSqlEditorLog::ErrorMsg, strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()), statement, "");\
log_error("SQLException executing %s: %s\n", std::string(statement).c_str(), strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()).c_str());\
}\
CATCH_EXCEPTION_AND_DISPATCH(statement)

#define CATCH_EXCEPTION_AND_DISPATCH(statement) \
catch (std::exception &e)\
{\
_owner->add_log_message(DbSqlEditorLog::ErrorMsg, strfmt(EXCEPTION_MSG_FORMAT, e.what()), statement, "");\
log_error("Exception executing %s: %s\n", std::string(statement).c_str(), strfmt(EXCEPTION_MSG_FORMAT, e.what()).c_str());\
}

#define CATCH_ANY_EXCEPTION_AND_DISPATCH(statement) \
catch (sql::SQLException &e)\
{\
_owner->add_log_message(DbSqlEditorLog::ErrorMsg, strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()), statement, "");\
log_error("SQLException executing %s: %s\n", std::string(statement).c_str(), strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()).c_str());\
}\
CATCH_EXCEPTION_AND_DISPATCH(statement)

#define CATCH_ANY_EXCEPTION_AND_DISPATCH_TO_DEFAULT_LOG(statement) \
catch (sql::SQLException &e)\
{\
_grtm->get_grt()->send_error(strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()), statement);\
}\
catch (std::exception &e)\
{\
_grtm->get_grt()->send_error(strfmt(EXCEPTION_MSG_FORMAT, e.what()), statement);\
}


boost::shared_ptr<SqlEditorTreeController> SqlEditorTreeController::create(SqlEditorForm *owner)
{
  boost::shared_ptr<SqlEditorTreeController> instance(new SqlEditorTreeController(owner));

  instance->_base_schema_tree.set_delegate(instance);
  instance->_base_schema_tree.set_fetch_delegate(instance);

  instance->_filtered_schema_tree.set_delegate(instance);
  instance->_filtered_schema_tree.set_fetch_delegate(instance);
  instance->_filtered_schema_tree.set_base(&instance->_base_schema_tree);

  return instance;
}


SqlEditorTreeController::SqlEditorTreeController(SqlEditorForm *owner)
  : _owner(owner), _grtm(owner->grt_manager()),
    _schema_side_bar(NULL), _admin_side_bar(NULL),
    _task_tabview(NULL),
    _taskbar_box(NULL),
    _schema_tree(&_base_schema_tree),
    _base_schema_tree(_grtm->get_grt()),
    _filtered_schema_tree(_grtm->get_grt()),
    live_schema_fetch_task(GrtThreadedTask::create(_grtm)),
    live_schemata_refresh_task(GrtThreadedTask::create(_grtm)),
    _is_refreshing_schema_tree(false),
    _unified_mode(false),
    _side_splitter(NULL),
    _info_tabview(NULL),
    _object_info(NULL),
    _session_info(NULL)
{
  grt::GRTNotificationCenter::get()->add_grt_observer(this, "GRNDBObjectEditorCreated");
  grt::GRTNotificationCenter::get()->add_grt_observer(this, "GRNPreferencesDidClose");
  grt::GRTNotificationCenter::get()->add_grt_observer(this, "GRNSQLEditorReconnected");

  _base_schema_tree.is_schema_contents_enabled(_grtm->get_app_option_int("DbSqlEditor:ShowSchemaTreeSchemaContents", 1) != 0);
  _filtered_schema_tree.is_schema_contents_enabled(_grtm->get_app_option_int("DbSqlEditor:ShowSchemaTreeSchemaContents", 1) != 0);

  _base_schema_tree.sql_editor_text_insert_signal.connect(boost::bind(&SqlEditorTreeController::insert_text_to_active_editor, this, _1));
  _filtered_schema_tree.sql_editor_text_insert_signal.connect(boost::bind(&SqlEditorTreeController::insert_text_to_active_editor, this, _1));

  live_schemata_refresh_task->send_task_res_msg(false);
  live_schemata_refresh_task->msg_cb(boost::bind(&SqlEditorForm::add_log_message, _owner, _1, _2, _3, ""));

  live_schema_fetch_task->send_task_res_msg(false);
  live_schema_fetch_task->msg_cb(boost::bind(&SqlEditorForm::add_log_message, _owner, _1, _2, _3, ""));
}


SqlEditorTreeController::~SqlEditorTreeController()
{
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

//--------------------------------------------------------------------------------------------------

void SqlEditorTreeController::finish_init()
{
  _unified_mode = _grtm->get_app_option_int("DbSqlEditor:SidebarModeCombined", 0) == 1;

  // Box to host the management and SQL IDE task bars in tab view or stacked mode.
  _taskbar_box = new mforms::Box(false);

  // Left hand sidebar tabview with admin and schema tree pages.
  _task_tabview = new mforms::TabView(mforms::TabViewSelectorSecondary);
  _schema_side_bar = (wb::SimpleSidebar *)mforms::TaskSidebar::create("SchemaTree");
  scoped_connect(_schema_side_bar->on_section_command(), boost::bind(&SqlEditorTreeController::sidebar_action, this, _1));
  _admin_side_bar = (wb::SimpleSidebar *)mforms::TaskSidebar::create("Simple");
  scoped_connect(_admin_side_bar->on_section_command(), boost::bind(&SqlEditorTreeController::sidebar_action, this, _1));

  mforms::TaskSectionFlags flags = mforms::TaskSectionRefreshable | mforms::TaskSectionToggleModeButton;
  if (_unified_mode)
    flags = flags | mforms::TaskSectionToggleModeButtonPreSelected;
  _schema_side_bar->add_section("SchemaTree", _("SCHEMAS"), flags);

  if (!_unified_mode)
  {
    _task_tabview->add_page(_admin_side_bar, _("Management"));
    _task_tabview->add_page(_schema_side_bar, _("Schemas"));

    int i = _grtm->get_app_option_int("DbSqlEditor:ActiveTaskTab", 0);
    if (i < 0)
      i = 0;
    else if (i >= 2)
      i = 1;
    _task_tabview->set_active_tab(i);
  }
  else
  {
    _task_tabview->show(false);
    _taskbar_box->add(_admin_side_bar, false, true);
    _taskbar_box->add(_schema_side_bar, true, true);
  }

  _schema_side_bar->get_context_menu()->signal_will_show()->connect(boost::bind(&SqlEditorTreeController::context_menu_will_show, this, _1));
  _schema_side_bar->set_schema_model(&_base_schema_tree);
  _schema_side_bar->set_filtered_schema_model(&_filtered_schema_tree);
  _schema_side_bar->set_selection_color(mforms::SystemColorHighlight);

  int initial_splitter_pos = _grtm->get_app_option_int("DbSqlEditor:SidebarInitialSplitterPos", 500);
  _side_splitter = mforms::manage(new mforms::Splitter(false, true));

#ifdef _WIN32
  mforms::Panel* panel;
  _side_splitter->set_back_color(base::Color::get_application_color_as_string(AppColorMainBackground, false));
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

#ifdef _WIN32
  panel = mforms::manage(new mforms::Panel(mforms::StyledHeaderPanel));
  panel->set_title(_("Information"));
  panel->add(_info_tabview);
  _side_splitter->add(panel, 30);
#else
  _side_splitter->add(_info_tabview, 30);
#endif

  _object_info = new mforms::HyperText();
  _session_info = new mforms::HyperText();
#ifdef _WIN32
  _object_info->set_back_color(base::Color::get_application_color_as_string(AppColorPanelContentArea, false));
  _object_info->set_padding(3, 3, 3, 3);
  _session_info->set_back_color(base::Color::get_application_color_as_string(AppColorPanelContentArea, false));
  _session_info->set_padding(3, 3, 3, 3);
#else
  _object_info->set_back_color("#ebebeb");
  _session_info->set_back_color("#ebebeb");
#endif

  _info_tabview->add_page(_object_info, _("Object Info"));
  _info_tabview->add_page(_session_info, _("Session"));

  _session_info->set_markup_text(_owner->get_connection_info());

  scoped_connect(_schema_side_bar->signal_filter_changed(),boost::bind(&SqlEditorTreeController::side_bar_filter_changed, this, _1));
  scoped_connect(_schema_side_bar->tree_node_selected(),boost::bind(&SqlEditorTreeController::schema_row_selected, this));

  // update the info box
  schema_row_selected();

  tree_refresh();

  // make sure to restore the splitter pos after layout is ready
  _grtm->run_once_when_idle(this, boost::bind(&mforms::Splitter::set_position, _side_splitter, initial_splitter_pos));

  // Connect the splitter change event after the setup is done to avoid wrong triggering.
  _splitter_connection = _side_splitter->signal_position_changed()->connect(boost::bind(&SqlEditorTreeController::sidebar_splitter_changed, this));

  // Setup grt access to sidebar.
  db_query_EditorRef editor(_owner->wbsql()->get_grt_editor_object(_owner));
  if (editor.is_valid())
    editor->sidebar(mforms_to_grt(_grtm->get_grt(), _admin_side_bar, "TaskSidebar"));

  if (!_owner->connected())
      _info_tabview->set_active_tab(1);
}

void SqlEditorTreeController::prepare_close()
{
  // Explicitly disconnect from the splitter change event as it sends unwanted change notifications
  // when controls are freed on shutdown.
  _splitter_connection.disconnect();

  if (_schema_side_bar)
    _grtm->set_app_option("DbSqlEditor:SidebarCollapseState", grt::StringRef(_schema_side_bar->get_collapse_states()));

  int tab = _task_tabview->get_active_tab();
  _grtm->set_app_option("DbSqlEditor:ActiveTaskTab", grt::IntegerRef(tab));
  tab = _info_tabview->get_active_tab();
  _grtm->set_app_option("DbSqlEditor:ActiveInfoTab", grt::IntegerRef(tab));
}


void SqlEditorTreeController::schema_row_selected()
{
  std::list<mforms::TreeNodeRef> nodes;
  std::string details;

  if (_schema_side_bar)
  {
    nodes = _schema_side_bar->get_schema_tree()->get_selection();
    if (nodes.empty())
      details = std::string("<html><body style=\"font-family:") + DEFAULT_FONT_FAMILY + "; font-size: 8pt\">"
        "<b><font color=\"#aaa\">No object selected</font></b></body></html>";
    else if (nodes.size() > 1)
      details = std::string("<html><body style=\"font-family:") + DEFAULT_FONT_FAMILY + "; font-size: 8pt\">"
        "<b><font color=\"#aaa\">Multiple selected objects</font></b></body></html>";
    else
    {
      // When there's a single node selected, gets the details
      // and tells it to notify if changes in it occur
      details = std::string("<html><body style=\"font-family:") + DEFAULT_FONT_FAMILY + "; font-size: 8pt\">" +
        _schema_tree->get_field_description(nodes.front()) + "</body></html>";
      _schema_tree->set_notify_on_reload(nodes.front());
    }

    _object_info->set_markup_text(details);


    // send out notification about selection change
    grt::DictRef info(_grtm->get_grt());
    info.gset("selection-size", (int)nodes.size());
    grt::GRTNotificationCenter::get()->send_grt("GRNLiveDBObjectSelectionDidChange", _owner->wbsql()->get_grt_editor_object(_owner), info);
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorTreeController::side_bar_filter_changed(const std::string& filter)
{
  if (filter.length() > 0)
    _schema_tree = &_filtered_schema_tree;
  else
    _schema_tree = &_base_schema_tree;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorTreeController::sidebar_splitter_changed()
{
  int pos = _side_splitter->get_position();
  if (pos > 0)
    _grtm->set_app_option("DbSqlEditor:SidebarInitialSplitterPos", grt::IntegerRef(pos));
}

//--------------------------------------------------------------------------------------------------

bool SqlEditorTreeController::fetch_data_for_filter(const std::string &schema_filter, const std::string &object_filter, const wb::LiveSchemaTree::NewSchemaContentArrivedSlot &arrived_slot)
{
  std::string wb_internal_schema = _grtm->get_app_option_string("workbench:InternalSchema");

  sql::Dbc_connection_handler::Ref conn;

  RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

  InternalSchema internal_schema(wb_internal_schema, conn);

  // Validates the remote search is available
  bool remote_search_enabled = internal_schema.is_remote_search_deployed();

  if (!remote_search_enabled)
  {
    if ( mforms::Utilities::show_message(_("Search Objects in Server"), base::strfmt(_("To enable searching objects in the remote server, MySQL Workbench needs to create a stored procedure in a custom schema (%s)."), wb_internal_schema.c_str()), _("Create"), _("Cancel")) == 1 )
    {
      // Performs the deployment, any error will be returned
      std::string error = internal_schema.deploy_remote_search();

      if (!error.length())
        remote_search_enabled = true;
      else
      {
        std::string userName = _owner->connection_descriptor()->parameterValues().get_string("userName");
        std::string msgFmt = _("The user %s has no privileges to create the required schema and stored procedures "
                              "to enable remote search in this server. \n"
                              "Ensure your database administrator creates a schema for internal use of MySQL Workbench"
                              " with full privileges for the user %s, once created configure it in "
                              "Preferences->General->Internal Workbench Schema and retry.\n\n%s.");

        std::string message = base::strfmt( msgFmt.c_str(), userName.c_str(), userName.c_str(), error.c_str());

        mforms::Utilities::show_error("Search Objects in Server", message, "Ok");
      }
    }
  }


  // If the remote search is available performs the search
  if (remote_search_enabled)
  {
    bool sync= !_grtm->in_main_thread();
    log_debug3("Fetch data for filter %s.%s\n", schema_filter.c_str(), object_filter.c_str());
    live_schema_fetch_task->exec(sync,
                                 boost::bind(&SqlEditorTreeController::do_fetch_data_for_filter, this, _1,
                                             weak_ptr_from(this), schema_filter, object_filter, arrived_slot));

  }

  return true;
}

//--------------------------------------------------------------------------------------------------


std::list<std::string> SqlEditorTreeController::fetch_schema_list()
{
  std::list<std::string> schemata_names;
  try
  {
    sql::Dbc_connection_handler::Ref conn;

    RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

    bool show_metadata_schemata= (0 != _grtm->get_app_option_int("DbSqlEditor:ShowMetadataSchemata", 0));

    std::auto_ptr<sql::ResultSet> rs(conn->ref->getMetaData()->getSchemata());
    while (rs->next())
    {
      std::string name= rs->getString(1);
      static std::map<std::string, bool> metadata_schemata_names;
      class MetadataSchemataNamesInitializer
      {
      public:
        MetadataSchemataNamesInitializer(std::map<std::string, bool> &metadata_schemata_names)
        {
          //! dbms-specific code
          //TODO: what it is used for?
          metadata_schemata_names["information_schema"];
          metadata_schemata_names["performance_schema"];
          metadata_schemata_names["mysql"];
        }
      };
      static MetadataSchemataNamesInitializer metadata_schemata_initializer(metadata_schemata_names);

      if (show_metadata_schemata ||
         (metadata_schemata_names.end() == metadata_schemata_names.find(name) && name[0] != '.'))
        schemata_names.push_back(name);
    }
  }
  CATCH_ANY_EXCEPTION_AND_DISPATCH(_("Get schemata"))
  return schemata_names;
}


bool SqlEditorTreeController::fetch_schema_contents(const std::string &schema_name,
                                                        const wb::LiveSchemaTree::NewSchemaContentArrivedSlot &arrived_slot)
{
  // in windows we use TreeViewAdv feature to expand nodes asynchronously
  // that is this function is already called from a separate thread
  // and it must have items loaded when it returns.
  bool sync= !_grtm->in_main_thread();
  log_debug3("Fetch schema contents for %s\n", schema_name.c_str());
  live_schema_fetch_task->exec(sync,
                               boost::bind(&SqlEditorTreeController::do_fetch_live_schema_contents, this, _1,
                                           weak_ptr_from(this), schema_name, arrived_slot));

  return true;//!
}


void SqlEditorTreeController::refresh_live_object_in_overview(wb::LiveSchemaTree::ObjectType type, const std::string schema_name, const std::string old_obj_name, const std::string new_obj_name)
{
  try
  {
    // update schema tree even if no object was added/dropped, to clear details attribute which contents might to be changed
    _schema_tree->update_live_object_state(type, schema_name, old_obj_name, new_obj_name);
  }
  CATCH_ANY_EXCEPTION_AND_DISPATCH_TO_DEFAULT_LOG(_("Refresh live schema object"))
}


mforms::View *SqlEditorTreeController::get_sidebar()
{
  return _side_splitter;
}


grt::StringRef SqlEditorTreeController::do_fetch_live_schema_contents(grt::GRT *grt, boost::weak_ptr<SqlEditorTreeController> self_ptr, const std::string &schema_name, wb::LiveSchemaTree::NewSchemaContentArrivedSlot arrived_slot)
{
  RETVAL_IF_FAIL_TO_RETAIN_WEAK_PTR (SqlEditorTreeController, self_ptr, self, grt::StringRef(""))
  try
  {
    std::list<std::string> *tables = new std::list<std::string>();
    std::list<std::string> *views  = new std::list<std::string>();
    std::list<std::string> *procedures = new std::list<std::string>();
    std::list<std::string> *functions = new std::list<std::string>();
    std::vector<std::pair<std::string, bool> > table_list;
    std::vector<std::pair<std::string, bool> > routine_list;

    MutexLock schema_contents_mutex(_schema_contents_mutex);

    if (arrived_slot.empty())
      return grt::StringRef("");

    {
      sql::Dbc_connection_handler::Ref conn;
      RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

      {
        std::auto_ptr<sql::ResultSet> rs(conn->ref->createStatement()->executeQuery(std::string(sqlstring("SHOW FULL TABLES FROM !", 0) << schema_name)));
        while (rs->next())
        {
          std::string name = rs->getString(1);
          std::string type = rs->getString(2);

          if (type == "VIEW")
            views->push_back(name);
          else
            tables->push_back(name);

          table_list.push_back(std::make_pair(name, type == "VIEW"));
        }
      }
      {
        std::auto_ptr<sql::ResultSet> rs(conn->ref->createStatement()->executeQuery(std::string(sqlstring("SHOW PROCEDURE STATUS WHERE Db=?", 0) << schema_name)));

        while (rs->next())
        {
          std::string name = rs->getString(2);
          procedures->push_back(name);
          routine_list.push_back(std::make_pair(name, false));
        }
      }
      {
        std::auto_ptr<sql::ResultSet> rs(conn->ref->createStatement()->executeQuery(std::string(sqlstring("SHOW FUNCTION STATUS WHERE Db=?", 0) << schema_name)));
        while (rs->next())
        {
          std::string name = rs->getString(2);
          functions->push_back(name);
          routine_list.push_back(std::make_pair(name, true));
        }
      }
    }

    if (arrived_slot)
    {
      boost::function<void ()> schema_contents_arrived = boost::bind(arrived_slot, schema_name, tables, views, procedures, functions, false);
      _grtm->run_once_when_idle(this, schema_contents_arrived);
    }

    // Let the owner form know we got fresh schema meta data. Can be used to update caches.
    _owner->schema_meta_data_refreshed(schema_name, table_list, routine_list, false);
  }
  catch (const sql::SQLException& e)
  {
    _owner->add_log_message(DbSqlEditorLog::ErrorMsg, strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()), "Error loading schema content", "");\
    log_error("SQLException executing %s: %s\n", std::string("Error loading schema content").c_str(), strfmt(SQL_EXCEPTION_MSG_FORMAT, e.getErrorCode(), e.what()).c_str());\

    if (arrived_slot)
    {
      std::list<std::string> *empty_list = NULL;
      boost::function<void ()> schema_contents_arrived = boost::bind(arrived_slot, schema_name, empty_list, empty_list, empty_list, empty_list, false);
      _grtm->run_once_when_idle(this, schema_contents_arrived);
    }
  }

  return grt::StringRef("");
}

//--------------------------------------------------------------------------------------------------

grt::StringRef SqlEditorTreeController::do_fetch_data_for_filter(grt::GRT *grt, boost::weak_ptr<SqlEditorTreeController> self_ptr, const std::string &schema_filter, const std::string &object_filter, wb::LiveSchemaTree::NewSchemaContentArrivedSlot arrived_slot)
{
  RETVAL_IF_FAIL_TO_RETAIN_WEAK_PTR (SqlEditorTreeController, self_ptr, self, grt::StringRef(""))

  log_debug3("Searching data for %s.%s\n", schema_filter.c_str(), object_filter.c_str());

  boost::shared_ptr<sql::ResultSet> dbc_resultset;
  std::map<std::string, int> schema_directory;
  std::string last_schema;

  std::string wb_internal_schema = _grtm->get_app_option_string("workbench:InternalSchema");

  try
  {
    // Creates the template for the sqlstring
    std::string procedure(base::sqlstring("CALL !.SEARCH_OBJECTS(?,?,0)",0) << wb_internal_schema << schema_filter << object_filter);

    // Gets the data
    std::string error = _owner->fetch_data_from_stored_procedure(procedure, dbc_resultset);

    if (dbc_resultset && !error.length())
    {
      std::list<std::string> *tables = new std::list<std::string>();
      std::list<std::string> *views = new std::list<std::string>();
      std::list<std::string> *procedures = new std::list<std::string>();
      std::list<std::string> *functions = new std::list<std::string>();

      // Creates the needed schema/objects
      while (dbc_resultset->next())
      {
        std::string schema= dbc_resultset->getString(1);
        std::string object= dbc_resultset->getString(2);
        std::string type = dbc_resultset->getString(3);

        // A schema change occurred, need to create the structure for the data loaded so far
        if (schema != last_schema && last_schema != "")
        {
          if (arrived_slot)
            _grtm->run_once_when_idle(this, boost::bind(arrived_slot, last_schema, tables, views, procedures, functions, true));

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
        _grtm->run_once_when_idle(this, boost::bind(arrived_slot, last_schema, tables, views, procedures, functions, true));
    }
    else
    {
      std::string userName = _owner->connection_descriptor()->parameterValues().get_string("userName");

      std::string msgFmt = _("The user %s has no privileges on %s to create temporal tables or execute required stored procedures "
                            "used in remote search in this server.\n"
                            "Ensure your database administrator grants you full access to the schema %s and retry.\n\n%s.");

      std::string message = base::strfmt( msgFmt.c_str(), userName.c_str(), wb_internal_schema.c_str(), wb_internal_schema.c_str(), error.c_str());

      mforms::Utilities::show_error(_("Search Objects in Server"), message, _("Ok"));
    }
  }

  CATCH_ANY_EXCEPTION_AND_DISPATCH_TO_DEFAULT_LOG(_("Get data for filter"))

  return grt::StringRef("");

}

//--------------------------------------------------------------------------------------------------

void SqlEditorTreeController::fetch_column_data(const std::string& schema_name, const std::string& obj_name, wb::LiveSchemaTree::ObjectType type, const wb::LiveSchemaTree::NodeChildrenUpdaterSlot &updater_slot)
{

  // Searches for the target node...
  mforms::TreeNodeRef node = _schema_tree->get_node_for_object(schema_name, type, obj_name);
  LiveSchemaTree::ViewData *pdata = NULL;

  if (node)
    pdata = dynamic_cast<LiveSchemaTree::ViewData*>(node->get_data());

  // Loads the information...
  std::list<std::string> columns;
  std::map<std::string, LiveSchemaTree::ColumnData> column_data;

  log_debug3("Fetching column data for %s.%s\n", schema_name.c_str(), obj_name.c_str());

  try
  {
    sql::Dbc_connection_handler::Ref conn;

    RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

    std::auto_ptr<sql::Statement> stmt(conn->ref->createStatement());
    std::auto_ptr<sql::ResultSet> rs(stmt->executeQuery(std::string(base::sqlstring("SHOW FULL COLUMNS FROM !.!", 0) << schema_name << obj_name)));

    while (rs->next())
    {
      LiveSchemaTree::ColumnData col_node(type);
      std::string column_name= rs->getString(1);

      columns.push_back(column_name);

      std::string type = rs->getString(2);
      std::string collation = rs->isNull(3) ? "" : rs->getString(3);
      std::string nullable = rs->getString(4);
      std::string key = rs->getString(5);
      std::string default_value = rs->getString(6);
      std::string extra = rs->getString(7);

      base::replace(type, "unsigned", "UN");

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
    if (columns.size())
    {
      // Creates the node if it didn't exist...
      if (!node)
      {
        node = _schema_tree->create_node_for_object(schema_name, type, obj_name);

        if (node)
          pdata = dynamic_cast<LiveSchemaTree::ViewData*>(node->get_data());
        else
          log_warning("Error fetching column information for '%s'.'%s'", schema_name.c_str(), obj_name.c_str());
      }

      if (pdata)
      {
        // Identifies the node that will be the parent for the loaded columns...
        mforms::TreeNodeRef target_parent;
        if (pdata->get_type() == LiveSchemaTree::Table)
        {
          target_parent = node->get_child(wb::LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX);
          type = LiveSchemaTree::TableColumn;
        }
        else if (pdata->get_type() == LiveSchemaTree::View)
        {
          target_parent = node;
          type = LiveSchemaTree::ViewColumn;
        }

        if (target_parent)
        {
          updater_slot(target_parent, columns, type, false, false);

          for(int index = 0; index < target_parent->count(); index++)
          {
            mforms::TreeNodeRef child = target_parent->get_child(index);
            LiveSchemaTree::LSTData *pchilddata = dynamic_cast<LiveSchemaTree::LSTData*>(child->get_data());
            LiveSchemaTree::LSTData *psource = &column_data[child->get_string(0)];
            pchilddata->copy(psource);
          }

          pdata->columns_load_error = false;
          pdata->set_loaded_data(LiveSchemaTree::COLUMN_DATA);
          _schema_tree->notify_on_reload(target_parent);
        }
      }
    }
  }
  catch (const sql::SQLException& exc)
  {
    log_warning("Error fetching column information for '%s'.'%s': %s", schema_name.c_str(), obj_name.c_str(), exc.what());

    // Sets flag indicating error loading columns ( Used for broken views )
    if (pdata)
    {
      if (exc.getErrorCode() == 1356)
        pdata->columns_load_error = true;

      pdata->details = exc.what();

      _schema_tree->update_node_icon(node);
    }
  }
}


void SqlEditorTreeController::fetch_trigger_data(const std::string& schema_name, const std::string& obj_name, wb::LiveSchemaTree::ObjectType type, const wb::LiveSchemaTree::NodeChildrenUpdaterSlot &updater_slot)
{
  // Loads the information...
  std::list<std::string> triggers;
  std::map<std::string, LiveSchemaTree::TriggerData> trigger_data_dict;

  try
  {
    sql::Dbc_connection_handler::Ref conn;

    RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

    std::auto_ptr<sql::Statement> stmt(conn->ref->createStatement());
    std::auto_ptr<sql::ResultSet> rs(stmt->executeQuery(std::string(base::sqlstring("SHOW TRIGGERS FROM ! LIKE ?", 0) << schema_name << obj_name)));

    while (rs->next())
    {
      wb::LiveSchemaTree::TriggerData trigger_node;

      std::string name = rs->getString(1);
      trigger_node.event_manipulation= wb::LiveSchemaTree::internalize_token(rs->getString(2));
      trigger_node.timing= wb::LiveSchemaTree::internalize_token(rs->getString(5));

      triggers.push_back(name);
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

    for(int index = 0; index < target_parent->count(); index++)
    {
      mforms::TreeNodeRef child = target_parent->get_child(index);
      LiveSchemaTree::LSTData *pchilddata = dynamic_cast<LiveSchemaTree::LSTData*>(child->get_data());
      LiveSchemaTree::LSTData *psource = &trigger_data_dict[child->get_string(0)];
      pchilddata->copy(psource);
    }

    // Where there data or not the triggers were loaded
    LiveSchemaTree::ViewData *pdata = dynamic_cast<LiveSchemaTree::ViewData*>(node->get_data());
    pdata->set_loaded_data(LiveSchemaTree::TRIGGER_DATA);
    _schema_tree->notify_on_reload(target_parent);
  }
  catch (const sql::SQLException& exc)
  {
    g_warning("Error fetching trigger information for '%s'.'%s': %s", schema_name.c_str(), obj_name.c_str(), exc.what());
  }
}


void SqlEditorTreeController::fetch_index_data(const std::string& schema_name, const std::string& obj_name, wb::LiveSchemaTree::ObjectType type, const wb::LiveSchemaTree::NodeChildrenUpdaterSlot &updater_slot)
{
  // Loads the information...
  std::list<std::string> indexes;
  std::map<std::string, LiveSchemaTree::IndexData> index_data_dict;

  try
  {
    sql::Dbc_connection_handler::Ref conn;

    RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

    std::auto_ptr<sql::Statement> stmt(conn->ref->createStatement());
    std::auto_ptr<sql::ResultSet> rs(stmt->executeQuery(std::string(base::sqlstring("SHOW INDEXES FROM !.!", 0) << schema_name << obj_name)));

    while (rs->next())
    {
      LiveSchemaTree::IndexData index_data;

      std::string name = rs->getString(3);

      // Inserts the index to the list
      if (!index_data_dict.count(name))
      {
        indexes.push_back(name);

        index_data.type = wb::LiveSchemaTree::internalize_token(rs->getString(11));
        index_data.unique = (rs->getInt(2) == 0);

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

    for(int index = 0; index < target_parent->count(); index++)
    {
      mforms::TreeNodeRef child = target_parent->get_child(index);
      LiveSchemaTree::LSTData *pchilddata = dynamic_cast<LiveSchemaTree::LSTData*>(child->get_data());
      LiveSchemaTree::LSTData *psource = &index_data_dict[child->get_string(0)];
      pchilddata->copy(psource);
    }

    LiveSchemaTree::ViewData *pdata = dynamic_cast<LiveSchemaTree::ViewData*>(node->get_data());
    pdata->set_loaded_data(LiveSchemaTree::INDEX_DATA);
    _schema_tree->notify_on_reload(target_parent);
  }
  catch (const sql::SQLException& exc)
  {
    g_warning("Error fetching index information for '%s'.'%s': %s", schema_name.c_str(), obj_name.c_str(), exc.what());
  }
}


void SqlEditorTreeController::fetch_foreign_key_data(const std::string& schema_name, const std::string& obj_name, wb::LiveSchemaTree::ObjectType type, const wb::LiveSchemaTree::NodeChildrenUpdaterSlot &updater_slot)
{
  std::list<std::string> foreign_keys;
  std::map<std::string, LiveSchemaTree::FKData> fk_data_dict;

  sql::Dbc_connection_handler::Ref conn;

  RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

  try
  {

    std::auto_ptr<sql::Statement> stmt(conn->ref->createStatement());
    std::auto_ptr<sql::ResultSet> rs(stmt->executeQuery(std::string(base::sqlstring("SHOW CREATE TABLE !.!", 0) << schema_name << obj_name)));

    while (rs->next())
    {
      std::string statement = rs->getString(2);

      size_t def_start = statement.find("(");
      size_t def_end = statement.rfind  (")");

      std::vector<std::string> def_lines = base::split(statement.substr(def_start, def_end - def_start), "\n");

      const char *errptr;
      int erroffs=0;
      const char *pattern = "CONSTRAINT\\s*(\\S*)\\s*FOREIGN KEY\\s*\\((\\S*)\\)\\s*REFERENCES\\s*(\\S*)\\s*\\((\\S*)\\)\\s*((\\w*\\s*)*),?$";
      int patres[64];

      pcre *patre= pcre_compile(pattern, 0, &errptr, &erroffs, NULL);
      if (!patre)
        throw std::logic_error("error compiling regex "+std::string(errptr));

      std::string fk_name;
      std::string fk_columns;
      std::string fk_ref_table;
      std::string fk_ref_columns;
      std::string fk_rules;
      const char *value;

      for(size_t index = 0; index < def_lines.size(); index++)
      {
        int rc = pcre_exec(patre, NULL, def_lines[index].c_str(), (int)def_lines[index].length(),
          0, 0, patres, sizeof(patres) / sizeof(int));

        if ( rc > 0 )
        {
          // gets the values timestamp and
          pcre_get_substring(def_lines[index].c_str(), patres, rc, 1, &value);
          fk_name = value;
          pcre_free_substring(value);
          fk_name = base::unquote_identifier(fk_name);

          pcre_get_substring(def_lines[index].c_str(), patres, rc, 2, &value);
          fk_columns = value;
          pcre_free_substring(value);

          pcre_get_substring(def_lines[index].c_str(), patres, rc, 3, &value);
          fk_ref_table = value;
          pcre_free_substring(value);
          fk_ref_table = base::unquote_identifier(fk_ref_table);

          pcre_get_substring(def_lines[index].c_str(), patres, rc, 4, &value);
          fk_ref_columns = value;
          pcre_free_substring(value);

          pcre_get_substring(def_lines[index].c_str(), patres, rc, 5, &value);
          fk_rules = value;
          pcre_free_substring(value);

          // Parses the list fields
          std::vector<std::string> fk_column_list = base::split(fk_columns, ",");
          std::vector<std::string> fk_ref_column_list = base::split(fk_ref_columns, ",");
          std::vector<std::string> fk_rule_tokens = base::split(fk_rules, " ");

          // Create the foreign key node
          wb::LiveSchemaTree::FKData new_fk;
          foreign_keys.push_back(fk_name);
          new_fk.referenced_table = (fk_ref_table);

          // Set the default update and delete rules
          new_fk.update_rule = new_fk.delete_rule = wb::LiveSchemaTree::internalize_token("RESTRICT");

          // A rule has at least 3 tokens so the number of tokens could be
          // 3 or 4 for 1 rule and 6,7,8 for two rules, so we get the number with this
          size_t rule_count = fk_rule_tokens.size() / 3;

          int token_offset = 0;
          for (size_t index = 0; index < rule_count; index++)
          {
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
          for(size_t column_index = 0; column_index < fk_column_list.size(); column_index++)
          {
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

    for(int index = 0; index < target_parent->count(); index++)
    {
      mforms::TreeNodeRef child = target_parent->get_child(index);
      LiveSchemaTree::LSTData *pchilddata = dynamic_cast<LiveSchemaTree::LSTData*>(child->get_data());
      LiveSchemaTree::LSTData *psource = &fk_data_dict[child->get_string(0)];
      pchilddata->copy(psource);
    }

    LiveSchemaTree::ViewData *pdata = dynamic_cast<LiveSchemaTree::ViewData*>(node->get_data());
    pdata->set_loaded_data(LiveSchemaTree::FK_DATA);
    _schema_tree->notify_on_reload(target_parent);
  }
  catch (const sql::SQLException& exc)
  {
    g_warning("Error fetching foreign key information for '%s'.'%s': %s", schema_name.c_str(), obj_name.c_str(), exc.what());
  }
}


bool SqlEditorTreeController::fetch_object_details(const std::string& schema_name, const std::string& object_name, wb::LiveSchemaTree::ObjectType type, short flags, const wb::LiveSchemaTree::NodeChildrenUpdaterSlot &updater_slot)
{
  // If the type has not been specified, pulls it from the database
  // Most of the time the type will be specified as it can bee retrieved from
  // the LST, this is to handle the case when a query is executed using direct SQL
  // and the tree hasn't been populated with the target object
  if (type == wb::LiveSchemaTree::Any)
    type = fetch_object_type(schema_name, object_name);

  if (type != wb::LiveSchemaTree::Any)
  {
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

bool SqlEditorTreeController::fetch_routine_details(const std::string& schema_name, const std::string& obj_name, wb::LiveSchemaTree::ObjectType type)
{
  bool ret_val = false;
  std::string object = type == LiveSchemaTree::Function ? "FUNCTION" : "PROCEDURE";
  std::string statement = "SHOW CREATE " + object + " !.!";
  // Loads the information...
  std::list<std::string> indexes;
  std::map<std::string, LiveSchemaTree::IndexData> index_data_dict;

  try
  {
    sql::Dbc_connection_handler::Ref conn;

    RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

    std::auto_ptr<sql::Statement> stmt(conn->ref->createStatement());
    std::auto_ptr<sql::ResultSet> rs(stmt->executeQuery(std::string(base::sqlstring(statement.c_str(), 0) << schema_name << obj_name)));

    if (rs->next())
    {
      LiveSchemaTree::IndexData index_data;

      std::string ddl = rs->getString(3);

      SqlFacade::Ref sql_facade= SqlFacade::instance_for_rdbms(_owner->rdbms());
      SqlFacade::String_tuple_list parameters;
      std::string ddl_type, ddl_name, ddl_ret, ddl_comments;
      ddl = "DELIMITER $$\n" + ddl;
      sql_facade->parseRoutineDetails(ddl, ddl_type, ddl_name, parameters, ddl_ret, ddl_comments);

      std::string details = "";
      if (parameters.size())
      {
        details = _("<b>Parameters:</b>");
        details += "<table border=0>";

        SqlFacade::String_tuple_list::iterator index, end = parameters.end();
        for(index = parameters.begin(); index != end; index++)
        {
          details += "<tr><td style=\"border:none; padding-left: 15px;\">" + index->first + ":</td>";
          details += "<td style=\"border:none; padding-left: 15px;\"><font color='#717171'>" + index->second + "</td></tr>";
        }

        details += "</table>";
      }

      if (!ddl_ret.empty())
        details += "<br><br><b><font color='#000000'>Returns: </b><font color='#717171'>" + ddl_ret;

      if (!ddl_comments.empty())
        details += "<br><br><b><font color='#000000'>Comments: </b><font color='#717171'>" + ddl_comments;


      LiveSchemaTree::ObjectData new_data;
      new_data.details = details;//ddl.substr(start, end - start);
      new_data.fetched = true;

      // Searches for the target node...
      mforms::TreeNodeRef node = _schema_tree->get_node_for_object(schema_name, type, obj_name);

      // The node should exist as this method is only called for a selected node
      if (node)
      {
        LiveSchemaTree::ObjectData *ptargetdata = dynamic_cast<LiveSchemaTree::ObjectData*>(node->get_data());
        ptargetdata->copy(&new_data);
        ret_val = true;
      }
    }
  }
  catch (const sql::SQLException& exc)
  {
    g_warning("Error fetching routine information for '%s'.'%s': %s", schema_name.c_str(), obj_name.c_str(), exc.what());
  }

  return ret_val;
}

wb::LiveSchemaTree::ObjectType SqlEditorTreeController::fetch_object_type(const std::string& schema_name, const std::string& obj_name)
{
  wb::LiveSchemaTree::ObjectType type = wb::LiveSchemaTree::Any;

  try
  {

    MutexLock schema_contents_mutex(_schema_contents_mutex);

    {
      sql::Dbc_connection_handler::Ref conn;
      RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

      {
        std::auto_ptr<sql::Statement> stmt(conn->ref->createStatement());
        std::auto_ptr<sql::ResultSet> rs(stmt->executeQuery(std::string(sqlstring("SHOW FULL TABLES FROM ! LIKE ?", 0) << schema_name << obj_name)));
        while (rs->next())
        {
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

//--------------------------------------------------------------------------------------------------

void SqlEditorTreeController::tree_refresh()
{
  if (_owner->connected())
    live_schemata_refresh_task->exec(false,
                                   boost::bind((grt::StringRef(SqlEditorTreeController::*)(grt::GRT *, SqlEditorForm::Ptr))&SqlEditorTreeController::do_refresh_schema_tree_safe, this, _1,
                                               weak_ptr_from(_owner)));
  else
    _schema_tree->set_no_connection();
}

//--------------------------------------------------------------------------------------------------

bool SqlEditorTreeController::sidebar_action(const std::string& name)
{
  if (name == "switch_mode_off")
  {
    if (_unified_mode)
    {
      _unified_mode = false;
      _taskbar_box->remove(_admin_side_bar);
      _taskbar_box->remove(_schema_side_bar);

      _task_tabview->add_page(_admin_side_bar, _("Management"));
      _task_tabview->add_page(_schema_side_bar, _("Schemas"));
      _task_tabview->set_active_tab(1);
      _task_tabview->show(true);

      _grtm->set_app_option("DbSqlEditor:SidebarModeCombined", grt::IntegerRef(0));
      _admin_side_bar->update_mode_buttons(false);
      _schema_side_bar->update_mode_buttons(false);
    }
    return true;
  }
  else if (name == "switch_mode_on")
  {
    if (!_unified_mode)
    {
      _unified_mode = true;
      _task_tabview->remove_page(_admin_side_bar);
      _task_tabview->remove_page(_schema_side_bar);
      _task_tabview->show(false);

      _taskbar_box->add(_admin_side_bar, false, true);
      _taskbar_box->add(_schema_side_bar, true, true);
      _schema_side_bar->focus();

      _grtm->set_app_option("DbSqlEditor:SidebarModeCombined", grt::IntegerRef(1));
      _admin_side_bar->update_mode_buttons(true);
      _schema_side_bar->update_mode_buttons(true);
    }
    return true;
  }
  else
    log_warning("unhandled sidebar action %s", name.c_str());

  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Activate one or more objects. The term "activate" is a bit misleading as we do other operations too
 * (like clipboard handling).
 */
void SqlEditorTreeController::tree_activate_objects(const std::string& action,
                                          const std::vector<wb::LiveSchemaTree::ChangeRecord>& changes)
{
  // Most of the activations should lead to a single result (e.g. all clipboard ops go into one string).
  std::string action_modifier; // action can contain prefix denoting action modifier
  std::string real_action= action; // if action modifier is present real_action will store part after w/o modifier prefix

  if (real_action == "select_data_columns")
  {
    typedef std::string TableName;
    typedef std::string ColumnName;
    std::map<TableName, std::map<ColumnName, std::string> > table_column_types;
    std::string text;
    typedef std::map<TableName, std::string> TableStringMap;
    TableStringMap first_text;
    TableStringMap second_text;

    // cache default values for tables
    for (size_t i = 0; i < changes.size(); i++)
    {
      TableName full_table_name = sqlstring("!.!", 0) << changes[i].schema << changes[i].name;
      std::string column_type;

      if (!first_text[full_table_name].empty())
        first_text[full_table_name].append(", ");
      first_text[full_table_name].append(changes[i].detail);
    }

    BOOST_FOREACH(const TableStringMap::value_type &table_columns, first_text)
      text += strfmt("SELECT %s\nFROM %s;\n", table_columns.second.c_str(), table_columns.first.c_str());

    _owner->run_sql_in_scratch_tab(text, false, true);
  }

  try
  {
    for (size_t i = 0; i < changes.size(); i++)
    {
      std::string sql;
      switch (changes[i].type)
      {
        case LiveSchemaTree::Schema:
          if (real_action == "filter")
          {
            _schema_side_bar->get_filter_entry()->set_value(changes[i].name);
            (*_schema_side_bar->get_filter_entry()->signal_changed())();
          }
          else if (real_action == "inspect")
            _owner->inspect_object(changes[i].name, "", "db.Schema");
          else if (real_action == "alter")
            do_alter_live_object(LiveSchemaTree::Schema, changes[i].name, "");
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

      if (!sql.empty())
      {
        bool _autosave = _owner->get_autosave_disabled();
        _owner->set_autosave_disabled(true);
        SqlEditorPanel* ed = _owner->run_sql_in_scratch_tab(sql, false, true);
        if (ed)
          ed->set_title(changes[i].name);
        _owner->set_autosave_disabled(_autosave);
      }
    }
  }
  catch (std::exception &exc)
  {
    mforms::Utilities::show_error("Error", exc.what(), "OK");
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Convenience API for the activation interface.
 */
void SqlEditorTreeController::schema_object_activated(const std::string &action, wb::LiveSchemaTree::ObjectType type,
                                            const std::string &schema, const std::string &name)
{
  std::vector<wb::LiveSchemaTree::ChangeRecord> changes;
  wb::LiveSchemaTree::ChangeRecord record = { type, schema, name, "" };
  changes.push_back(record);
  tree_activate_objects(action, changes);
}

//--------------------------------------------------------------------------------------------------

void SqlEditorTreeController::do_alter_live_object(wb::LiveSchemaTree::ObjectType type, const std::string &schema_name, const std::string &aobj_name)
{
  std::string used_schema_name = schema_name;
  std::string obj_name = aobj_name;
  try
  {
    db_mgmt_RdbmsRef rdbms= _owner->rdbms();
    std::string database_package= *rdbms->databaseObjectPackage();

    if (rdbms.is_valid())
    {
      rdbms = grt::shallow_copy_object(rdbms);
      rdbms->version(grt::shallow_copy_object(_owner->rdbms_version()));
      rdbms->version()->owner(rdbms);
    }

    //TODO needs reset_references to be called on editor close to prevent leaks
    db_CatalogRef client_state_catalog= _grtm->get_grt()->create_object<db_Catalog>(database_package + ".Catalog");
    client_state_catalog->name("default");
    client_state_catalog->oldName("default");
    client_state_catalog->version(rdbms->version());
    grt::replace_contents(client_state_catalog->simpleDatatypes(), rdbms->simpleDatatypes());
    grt::replace_contents(client_state_catalog->characterSets(), rdbms->characterSets());
    //XXX this should be changed when/if global userDatatypes are added
    //XXX    grt::replace_contents(client_state_catalog->userDatatypes(),
    //XXX                          workbench_physical_ModelRef::cast_from(_live_physical_overview->get_model())->catalog()->userDatatypes());

    db_SchemaRef schema;
    if (wb::LiveSchemaTree::Schema != type)
    {
      if (used_schema_name == "")
        used_schema_name = _owner->active_schema();

      if (used_schema_name == "")
      {
        mforms::Utilities::show_warning(strfmt(_("No Schema Selected")),
                                        _("A default schema must be set by double clicking its name in the SCHEMA list."),
                                        _("OK"));

        return;
      }
      else
      {
        schema= _grtm->get_grt()->create_object<db_Schema>(database_package + ".Schema");
        schema->owner(client_state_catalog);

        schema->name(used_schema_name);
        schema->oldName(used_schema_name);
        client_state_catalog->schemata().insert(schema);
        client_state_catalog->defaultSchema(schema);
      }
    }

    bool is_object_new= obj_name.empty();

    std::string ddl_script;
    std::string sql_mode;
    if (!is_object_new)
    {
      // parse selected object DDL into auxiliary catalog
      ddl_script= get_object_ddl_script(type, used_schema_name, obj_name);
      if (ddl_script.empty())
      {
        log_warning("Unable to get DDL for %s.%s", used_schema_name.c_str(), obj_name.c_str());
        return;
      }
      {
        sql::Dbc_connection_handler::Ref conn;
        RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));
        if (conn)
          _owner->get_session_variable(conn->ref.get(), "sql_mode", sql_mode);
      }

      // if this is a View, then auto-reformat it before sending it to parser/editor
      if (type == wb::LiveSchemaTree::View && _grtm->get_app_option_int("DbSqlEditor:ReformatViewDDL", 0))
      {
        try
        {
          grt::Module *module = _grtm->get_grt()->get_module("SQLIDEUtils");
          grt::BaseListRef args(_grtm->get_grt());
          args.ginsert(grt::StringRef(ddl_script));
          ddl_script = grt::StringRef::cast_from(module->call_function("reformatSQLStatement", args));
        }
        catch (std::exception &exc)
        {
          log_warning("Error reformatting view code: %s", exc.what());
        }
      }

      if (!parse_ddl_into_catalog(rdbms, client_state_catalog, strfmt("`%s`.`%s`", schema_name.c_str(), obj_name.c_str()), ddl_script, sql_mode))
      {
        log_warning("Error parsing DDL for %s.%s: %s", schema_name.c_str(), obj_name.c_str(), ddl_script.c_str());
        return;
      }
    }
    //TODO needs reset_references to be called on editor close to prevent leaks
    db_CatalogRef server_state_catalog = grt::copy_object(client_state_catalog);

  retry_search:
    //TODO needs reset_references to be called on editor close to prevent leaks
    db_DatabaseObjectRef db_object;
    switch (type)
    {
      case wb::LiveSchemaTree::Schema:
        db_object= is_object_new ?
          create_new_schema(client_state_catalog) :
          find_named_object_in_list(client_state_catalog->schemata(), obj_name);
        break;
      case wb::LiveSchemaTree::Table:
        db_object= is_object_new ?
          create_new_table(schema) :
          find_named_object_in_list(schema->tables(), obj_name);
        break;
      case wb::LiveSchemaTree::View:
        db_object= is_object_new ?
          create_new_view(schema) :
          find_named_object_in_list(schema->views(), obj_name);
        break;
      case wb::LiveSchemaTree::Procedure:
      case wb::LiveSchemaTree::Function:
        db_object= is_object_new ?
          create_new_routine(schema, type) :
          find_named_object_in_list(schema->routines(), obj_name);
        break;
      default:
        break;
    }
    if (!db_object.is_valid())
    {
      std::string lower;
      // There is a bug in server that causes get_object_ddl_script() for uppercase named tables
      // to be returned in lowercase, in osx. http://bugs.mysql.com/bug.php?id=57830
      // So, if you try to alter FOOBAR, it will not find it, since the table will be revenged
      // in lowercase. To work around that, we convert the object name to lowercase and repeat
      // the search if the 1st try didn't work.
      lower = tolower(obj_name);
      if (lower != obj_name)
      {
        log_warning("Object name %s was not found in catalog, trying to search it as %s",
                  obj_name.c_str(), lower.c_str());
        obj_name = lower;
        goto retry_search;
      }
      else
        log_warning("Object name %s was not found in catalog.", aobj_name.c_str());
      return;
    }

    db_object->customData().set("sqlMode", grt::StringRef(sql_mode));
    db_object->customData().set("originalObjectDDL", grt::StringRef(ddl_script));

    open_alter_object_editor(db_object, server_state_catalog);
  }
  catch (const std::exception & e)
  {
    log_error("Failed to create/alter `%s`.`%s`: %s", used_schema_name.c_str(), obj_name.c_str(), e.what());
    mforms::Utilities::show_error(strfmt(_("Failed to create/alter `%s`.`%s`"), used_schema_name.c_str(), obj_name.c_str()), e.what(), _("OK"));
  }
}


//--------------------------------------------------------------------------------------------------

void SqlEditorTreeController::open_alter_object_editor(db_DatabaseObjectRef object,
                                                       db_CatalogRef server_state_catalog)
{
  db_CatalogRef client_state_catalog;
  if (db_SchemaRef::can_wrap(object))
  {
    if (!object->owner().is_valid())
      throw std::invalid_argument("schema object does not have owner set to expected value");
    client_state_catalog = db_CatalogRef::cast_from(object->owner());
  }
  else
  {
    if (!object->owner().is_valid())
      throw std::invalid_argument("object does not have owner set to expected schema value");
    if (!object->owner()->owner().is_valid())
      throw std::invalid_argument("object does not have owner set to expected schema value");
    client_state_catalog = db_CatalogRef::cast_from(object->owner()->owner());
  }

  sql::Dbc_connection_handler::Ref conn;
  grt::NormalizedComparer comparer(_grtm->get_grt());
  {
    RecMutexLock lock(_owner->ensure_valid_aux_connection(conn));
    //db_object->customData().set("CaseSensitive",grt::IntegerRef(conn->ref->getMetaData()->storesMixedCaseIdentifiers()));
    //TODO use DB_Plugin here somehow
    comparer.load_db_options(conn->ref->getMetaData());
  }
  object->customData().set("DBSettings", comparer.get_options_dict());
  object->customData().set("liveRdbms", _owner->rdbms());
  object->customData().set("ownerSqlEditor", _owner->wbsql()->get_grt_editor_object(_owner));

  object->customData().set("clientStateCatalog", client_state_catalog);
  object->customData().set("serverStateCatalog", server_state_catalog);

  // TODO: make docking/non-docking switchable via preferences.
  //_context_ui->get_wb()->open_object_editor(db_object, bec::StandaloneWindowFlag);
  _grtm->open_object_editor(object, bec::ForceNewWindowFlag);
}

//--------------------------------------------------------------------------------------------------

std::string SqlEditorTreeController::run_execute_routine_wizard(wb::LiveSchemaTree::ObjectType type, 
  const std::string &schema_name, const std::string &obj_name)
{
  std::pair<std::string, std::string> script = get_object_create_script(type, schema_name, obj_name);
  if (script.second.empty())
    return ""; // get_object_create_script() already showed an error.
  
  db_mysql_RoutineRef routine(_grtm->get_grt());
  parser::MySQLParserServices::Ref services = parser::MySQLParserServices::get(_grtm->get_grt());

  db_mysql_CatalogRef catalog(_grtm->get_grt());
  catalog->version(_owner->rdbms_version());
  grt::replace_contents(catalog->simpleDatatypes(), _owner->rdbms()->simpleDatatypes());

  db_mysql_SchemaRef schema(_grtm->get_grt());
  schema->owner(catalog);
  schema->name(schema_name);
  catalog->schemata().insert(schema);

  routine->owner(schema);
  schema->routines().insert(routine);

  std::string previous_sql_mode;
  if (!script.first.empty())
  {
    previous_sql_mode = _owner->work_parser_context()->get_sql_mode();
    _owner->work_parser_context()->use_sql_mode(script.first);
  }

  size_t error_count = services->parseRoutine(_owner->work_parser_context(), routine, script.second);

  if (!previous_sql_mode.empty())
    _owner->work_parser_context()->use_sql_mode(previous_sql_mode);

  if (error_count > 0)
  {
    log_warning("Error parsing SQL code for %s.%s:\n%s\n", schema_name.c_str(), obj_name.c_str(), script.second.c_str());

    std::vector<ParserErrorEntry> errors = _owner->work_parser_context()->get_errors_with_offset(0, false);
    mforms::Utilities::show_error(_("Error parsing sql code for object"), errors[0].message, "OK");
    return "";
  }

  ExecuteRoutineWizard wizard(routine);
  wizard.center();
  return wizard.run();
}

//--------------------------------------------------------------------------------------------------

db_SchemaRef SqlEditorTreeController::create_new_schema(db_CatalogRef owner)
{
  db_SchemaRef object= _grtm->get_grt()->create_object<db_Schema>(owner->schemata()->content_type_spec().object_class);
  object->owner(owner);
  object->name("new_schema");
  owner->schemata().insert(object);
  owner->defaultSchema(object);
  return object;
}

//--------------------------------------------------------------------------------------------------

db_TableRef SqlEditorTreeController::create_new_table(db_SchemaRef owner)
{
  db_TableRef object= _grtm->get_grt()->create_object<db_Table>(owner->tables()->content_type_spec().object_class);
  object->owner(owner);
  object->name("new_table");
  owner->tables().insert(object);
  return object;
}


db_ViewRef SqlEditorTreeController::create_new_view(db_SchemaRef owner)
{
  db_ViewRef object= _grtm->get_grt()->create_object<db_View>(owner->views()->content_type_spec().object_class);
  object->owner(owner);
  object->name("new_view");
  owner->views().insert(object);
  return object;
}


db_RoutineRef SqlEditorTreeController::create_new_routine(db_SchemaRef owner, wb::LiveSchemaTree::ObjectType type)
{
  db_RoutineRef object= _grtm->get_grt()->create_object<db_Routine>(owner->routines()->content_type_spec().object_class);
  object->owner(owner);

  if (type == wb::LiveSchemaTree::Procedure)
  {
    object->name("new_procedure");
    object->routineType("procedure");
  }
  else if (type == wb::LiveSchemaTree::Function)
  {
    object->name("new_function");
    object->routineType("function");
  }

  owner->routines().insert(object);
  return object;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorTreeController::tree_create_object(wb::LiveSchemaTree::ObjectType type, const std::string &schema_name, const std::string &obj_name)
{
  do_alter_live_object(type, schema_name, obj_name);
}

//--------------------------------------------------------------------------------------------------

/**
 * Generates an alter script for the given db object using the specified online DDL options.
 * These are however only applied if the server version is >= 5.6.
 */
std::string SqlEditorTreeController::generate_alter_script(const db_mgmt_RdbmsRef &rdbms, db_DatabaseObjectRef db_object,
  std::string algorithm, std::string lock)
{
  DbMySQLImpl *diffsql_module = _grtm->get_grt()->find_native_module<DbMySQLImpl>("DbMySQL");

  db_CatalogRef server_cat= db_CatalogRef::cast_from(db_object->customData().get("serverStateCatalog"));
  db_CatalogRef client_cat= db_CatalogRef::cast_from(db_object->customData().get("clientStateCatalog"));

  db_CatalogRef client_cat_copy= db_CatalogRef::cast_from(grt::copy_object(client_cat));
  db_CatalogRef server_cat_copy= db_CatalogRef::cast_from(grt::copy_object(server_cat));

  grt::DictRef diff_options(_grtm->get_grt());
  //diff_options.set("CaseSensitive",db_object->customData().get("CaseSensitive"));
  grt::DictRef db_settings = grt::DictRef::cast_from(db_object->customData().get("DBSettings"));
  if (_owner->rdbms_version().is_valid() && is_supported_mysql_version_at_least(_owner->rdbms_version(), 5, 6))
  {
    db_settings.gset("AlterAlgorithm", algorithm != "DEFAULT" ? algorithm : "");
    db_settings.gset("AlterLock", lock != "DEFAULT" ? lock : "");
  }
  diff_options.set("DBSettings", db_settings);

  std::string alter_script= diffsql_module->makeAlterScriptForObject(server_cat_copy, client_cat_copy,
    db_object, diff_options);
  client_cat_copy->reset_references();
  server_cat_copy->reset_references();

  return alter_script;
}

//--------------------------------------------------------------------------------------------------

// Deprecated.
std::string SqlEditorTreeController::get_object_ddl_script(wb::LiveSchemaTree::ObjectType type, const std::string &schema_name, const std::string &obj_name)
{
  //const size_t DDL_COLUMN= 5;
  std::string delimiter;
  {
    SqlFacade::Ref sql_facade= SqlFacade::instance_for_rdbms(_owner->rdbms());
    Sql_specifics::Ref sql_specifics= sql_facade->sqlSpecifics();
    delimiter= sql_specifics->non_std_sql_delimiter();
  }
  std::string ddl_script;
  //triggers are fetched prior to table ddl, but should appear after table created
  std::string additional_ddls;
  ddl_script+= strfmt("delimiter %s\n\n", delimiter.c_str());

  try
  {
    sql::Dbc_connection_handler::Ref conn;
    std::string query;

    RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

    // cant use getSchemaObjects() because it silently ignores errors
    switch (type)
    {
      case wb::LiveSchemaTree::Schema:
        query = base::sqlstring("SHOW CREATE SCHEMA !", 0) << obj_name;
        break;

      case wb::LiveSchemaTree::Table:
      {
        // triggers
        std::vector<std::string> triggers;
        {
          std::string trigger_query = base::sqlstring("SHOW TRIGGERS FROM ! WHERE ! = ?", 0) << schema_name << "Table" << obj_name;
          std::auto_ptr<sql::Statement> stmt(conn->ref->createStatement());
          std::auto_ptr<sql::ResultSet> rs(stmt->executeQuery(trigger_query));

          if (rs.get())
          {
            while(rs->next())
                triggers.push_back(rs->getString(1));
          }
        }

        for(size_t index = 0; index < triggers.size(); index++)
        {
          std::string trigger_query = base::sqlstring("SHOW CREATE TRIGGER !.!", 0) << schema_name << triggers[index];
          std::auto_ptr<sql::Statement> stmt(conn->ref->createStatement());
          std::auto_ptr<sql::ResultSet> rs(stmt->executeQuery(trigger_query));

          if (rs.get() && rs->next())
          {
              std::string trigger_ddl = (rs->getString(3));
              additional_ddls+= trigger_ddl;
              additional_ddls+= delimiter + "\n\n";
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

    std::auto_ptr<sql::Statement> stmt(conn->ref->createStatement());
    std::auto_ptr<sql::ResultSet> rs(stmt->executeQuery(query));

    // Note: show create procedure includes the sql mode in the result before the actual DDL.
    if (rs.get() && rs->next())
    {
      if (type == wb::LiveSchemaTree::Function || type == wb::LiveSchemaTree::Procedure)
        ddl_script += rs->getString(3)  + delimiter + "\n\n";
      else
        ddl_script += rs->getString(2)  + delimiter + "\n\n";
    }
    ddl_script+= additional_ddls;
  }
  catch (const sql::SQLException &e)
  {
    if (type == wb::LiveSchemaTree::View && e.getErrorCode() == 1356)
    {
      sql::Dbc_connection_handler::Ref conn;
      std::string query, view;
      RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));
      query = base::sqlstring("SELECT DEFINER, SECURITY_TYPE, VIEW_DEFINITION FROM INFORMATION_SCHEMA.VIEWS WHERE TABLE_SCHEMA = ? AND TABLE_NAME = ?", 0) << schema_name << obj_name;
      std::auto_ptr<sql::Statement> stmt(conn->ref->createStatement());
      std::auto_ptr<sql::ResultSet> rs(stmt->executeQuery(query));

      if (rs.get() && rs->next())
      {
        std::string view, definer;
        std::vector<std::string> definer_tokens = base::split(rs->getString(1), "@", 2);

        view = base::sqlstring("!.!", 0)  << schema_name << obj_name;
        definer = base::sqlstring("!@!", 0)  << definer_tokens[0] << definer_tokens[1];
        ddl_script += "CREATE ALGORITHM=UNDEFINED DEFINER=" + definer;
        ddl_script += " SQL SECURITY " + rs->getString(2);
        ddl_script += " VIEW " + view + " AS ";
        ddl_script += rs->getString(3) + delimiter + "\n\n";
      }
    }
    else
    {
      log_error("Error getting SQL definition for %s.%s: %s\n", schema_name.c_str(), obj_name.c_str(), e.what());
      mforms::Utilities::show_error("Error getting DDL for object", e.what(), "OK", "", "");
      ddl_script.clear();
    }
  }
  return ddl_script;
}

//--------------------------------------------------------------------------------------------------

/**
 * Retrieves the original DDL text that was used to create the object.
 * Returns a tuple of <sql_mode, script>. The sql mode is what was used to create the object,
 * if it is a routine. Otherwise this value is empty.
 */
std::pair<std::string, std::string> SqlEditorTreeController::get_object_create_script(wb::LiveSchemaTree::ObjectType type,
  const std::string &schema_name, const std::string &obj_name)
{
  std::pair<std::string, std::string> result;

  try
  {
    sql::Dbc_connection_handler::Ref conn;
    std::string query;

    RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

    // cant use getSchemaObjects() because it silently ignores errors
    switch (type)
    {
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

    std::auto_ptr<sql::Statement> stmt(conn->ref->createStatement());
    std::auto_ptr<sql::ResultSet> rs(stmt->executeQuery(query));

    if (rs.get() && rs->next())
    {
      if (type == wb::LiveSchemaTree::Function || type == wb::LiveSchemaTree::Procedure)
      {
        result.first = rs->getString(2);
        result.second = rs->getString(3);
      }
      else
        result.second = rs->getString(2);
    }
  }
  catch (const sql::SQLException &e)
  {
    if (type == wb::LiveSchemaTree::View && e.getErrorCode() == 1356)
    {
      // Error for not being allowed to run SHOW CREATE VIEW. Use I_S instead to get the code.
      sql::Dbc_connection_handler::Ref conn;
      std::string query, view;
      RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));
      query = base::sqlstring("SELECT DEFINER, SECURITY_TYPE, VIEW_DEFINITION FROM INFORMATION_SCHEMA.VIEWS WHERE TABLE_SCHEMA = ? AND TABLE_NAME = ?", 0) << schema_name << obj_name;
      std::auto_ptr<sql::Statement> stmt(conn->ref->createStatement());
      std::auto_ptr<sql::ResultSet> rs(stmt->executeQuery(query));

      if (rs.get() && rs->next())
      {
        std::string view, definer;
        std::vector<std::string> definer_tokens = base::split(rs->getString(1), "@", 2);

        view = base::sqlstring("!.!", 0) << schema_name << obj_name;
        definer = base::sqlstring("!@!", 0) << definer_tokens[0] << definer_tokens[1];
        result.second = "CREATE ALGORITHM = UNDEFINED DEFINER = " + definer;
        result.second += " SQL SECURITY " + rs->getString(2);
        result.second += " VIEW " + view + " AS\n";
        result.second += rs->getString(3);
      }
    }
    else
    {
      log_error("Error getting SQL definition for %s.%s: %s\n", schema_name.c_str(), obj_name.c_str(), e.what());
      mforms::Utilities::show_error("Error getting DDL for object", e.what(), "OK");
    }
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 *	Returns a list of trigger create scripts for the given table.
 */
std::vector<std::string> SqlEditorTreeController::get_trigger_sql_for_table(const std::string &schema_name,
  const std::string &table_name)
{
  std::vector<std::string> result;

  try
  {
    sql::Dbc_connection_handler::Ref conn;
    RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

    std::vector<std::string> triggers;
    {
      std::string trigger_query = base::sqlstring("SHOW TRIGGERS FROM ! WHERE ! = ?", 0) << schema_name << "Table" << table_name;
      std::auto_ptr<sql::Statement> stmt(conn->ref->createStatement());
      std::auto_ptr<sql::ResultSet> rs(stmt->executeQuery(trigger_query));

      if (rs.get())
      {
        while (rs->next())
          triggers.push_back(rs->getString(1));
      }
    }

    for (size_t index = 0; index < triggers.size(); index++)
    {
      std::string trigger_query = base::sqlstring("SHOW CREATE TRIGGER !.!", 0) << schema_name << triggers[index];
      std::auto_ptr<sql::Statement> stmt(conn->ref->createStatement());
      std::auto_ptr<sql::ResultSet> rs(stmt->executeQuery(trigger_query));

      if (rs.get() && rs->next())
        result.push_back(rs->getString(3));
    }
  }
  catch (const sql::SQLException &e)
  {
    log_error("Error getting SQL definition for %s.%s: %s\n", schema_name.c_str(), table_name.c_str(), e.what());
    mforms::Utilities::show_error("Error getting DDL for object", e.what(), "OK");
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorTreeController::refresh_live_object_in_editor(bec::DBObjectEditorBE* obj_editor, bool using_old_name)
{
  db_DatabaseObjectRef db_object= obj_editor->get_dbobject();

  db_CatalogRef client_state_catalog= db_CatalogRef::cast_from(db_object->customData().get("clientStateCatalog"));

  std::string obj_name= using_old_name ? db_object->oldName() : db_object->name();
  // don't refresh new objects that where not applied yet
  if (obj_name.empty())
    return;
  obj_editor->freeze_refresh_on_object_change();

  if (obj_name != *db_object->name())
    db_object->name(obj_name);
  db_object->oldName("");
  obj_editor->thaw_refresh_on_object_change(true);

  std::string schema_name= db_SchemaRef::can_wrap(db_object) ? std::string() : *db_object->owner()->name();
  db_SchemaRef schema;
  if (!schema_name.empty())
    schema= db_SchemaRef::cast_from(db_object->owner());

  wb::LiveSchemaTree::ObjectType db_object_type = wb::LiveSchemaTree::Any;

  if (db_SchemaRef::can_wrap(db_object))
  {
    db_object_type= wb::LiveSchemaTree::Schema;
  }
  else
  {
    if (db_TableRef::can_wrap(db_object))
    {
      db_object_type= wb::LiveSchemaTree::Table;

      // reset selection of fkeys/indices to avoid warnings
      bec::TableEditorBE *table_editor= dynamic_cast <bec::TableEditorBE *> (obj_editor);
      table_editor->get_fks()->select_fk(NodeId());
      table_editor->get_indexes()->select_index(NodeId());

      obj_editor->freeze_refresh_on_object_change();

      db_TableRef table= db_TableRef::cast_from(db_object);
      table->isStub(1);
      table->triggers().remove_all();
      table->foreignKeys().remove_all();
      table->indices().remove_all();
      table->columns().remove_all();

      obj_editor->thaw_refresh_on_object_change(true);
    }
    else if (db_ViewRef::can_wrap(db_object))
    {
      db_object_type= wb::LiveSchemaTree::View;
    }
    else if (db_RoutineRef::can_wrap(db_object))
    {
      db_RoutineRef db_routine = db_RoutineRef::cast_from(db_object);
      std::string obj_type = db_routine->routineType();

      if (obj_type == "function")
        db_object_type = wb::LiveSchemaTree::Function;
      else
        db_object_type = wb::LiveSchemaTree::Procedure;
    }
  }

  obj_editor->freeze_refresh_on_object_change();

  // reparse object's DDL
  std::string ddl_script;
  std::string sql_mode;
  {
    ddl_script= get_object_ddl_script(db_object_type, schema_name, obj_name);
    if (!ddl_script.empty())
    {
      if (db_object_type == wb::LiveSchemaTree::View && _grtm->get_app_option_int("DbSqlEditor:ReformatViewDDL", 0))
      {
        try
        {
          grt::Module *module = _grtm->get_grt()->get_module("SQLIDEUtils");
          grt::BaseListRef args(_grtm->get_grt());
          args.ginsert(grt::StringRef(ddl_script));
          ddl_script = grt::StringRef::cast_from(module->call_function("reformatSQLStatement", args));
        }
        catch (std::exception &exc)
        {
          log_warning("Error reformatting view code: %s", exc.what());
        }
      }

      db_object->oldName(obj_name);

      try
      {
        sql::Dbc_connection_handler::Ref conn;
        RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));
        if (conn)
          _owner->get_session_variable(conn->ref.get(), "sql_mode", sql_mode);
      }
      CATCH_ANY_EXCEPTION_AND_DISPATCH(_("Get 'sql_mode' session variable"));

      parse_ddl_into_catalog(_owner->rdbms(), client_state_catalog,
                             strfmt("`%s`.`%s`", schema_name.c_str(), obj_name.c_str()),
                             ddl_script, sql_mode);
    }
  }

  {
    db_CatalogRef server_state_catalog(db_CatalogRef::cast_from(grt::copy_object(client_state_catalog)));
    db_object->customData().set("serverStateCatalog", server_state_catalog);
  }
  db_object->customData().set("originalObjectDDL", grt::StringRef(ddl_script));
  db_object->customData().set("sqlMode", grt::StringRef(sql_mode));

  obj_editor->thaw_refresh_on_object_change();

  // enable refresh of sql editor contents
  MySQLEditor::Ref active_sql_editor= obj_editor->get_sql_editor();
  if (active_sql_editor)
  {
    active_sql_editor->set_refresh_enabled(true);
    // provoke database object to refresh FE control contents
    (*db_object->signal_changed())("", grt::ValueRef());
  }
}


bool SqlEditorTreeController::parse_ddl_into_catalog(db_mgmt_RdbmsRef rdbms, db_CatalogRef client_state_catalog,
                                           const std::string &obj_descr, const std::string &ddl_script, std::string sql_mode)
{
  SqlFacade::Ref sql_facade= SqlFacade::instance_for_rdbms(rdbms);
  Sql_parser::Ref sql_parser= sql_facade->sqlParser();
  sql_parser->messages_enabled(false);
  grt::DictRef options(_grtm->get_grt());
  options.set("reuse_existing_objects", grt::IntegerRef(1));
  if (!sql_mode.empty())
    options.gset("sql_mode", sql_mode);

  int err_count= sql_parser->parse_sql_script(client_state_catalog, ddl_script, options);
  bool generic_parse_error = false;

  //! dbms-specific code
  if (options.has_key("sql_mode") && (0 < err_count))
  {
    if (std::string::npos != sql_mode.find("ANSI_QUOTES"))
      sql_mode= replace_string(sql_mode, "ANSI_QUOTES", "");
    else
      sql_mode+= ",ANSI_QUOTES";
    options.gset("sql_mode", sql_mode);
    options.set("reuse_existing_objects", grt::IntegerRef(1));
    err_count= sql_parser->parse_sql_script(client_state_catalog, ddl_script, options);
    if (0 == err_count)
    {
      if (mforms::Utilities::show_warning(strfmt(_("Error Parsing DDL for %s"), obj_descr.c_str()),
        _("The object's DDL retrieved from the server is inconsistent with respect to the SQL_MODE variable "
          "set for the connection. In particular the current state of the ANSI_QUOTES flag contradicts "
          "the value set when the object had been created. This may lead to errors when trying to "
          "apply changes. As a workaround you may want to temporarily change the SQL_MODE variable "
          "to its previous value.\nDo you want to view the DDL or cancel processing it?"),
                                          _("View DDL"), _("Cancel")) == mforms::ResultOk)
      {
        _owner->new_sql_scratch_area();
        insert_text_to_active_editor(ddl_script);
      }
      return false;
    }
    else
      generic_parse_error = true;
  }
  else if (err_count > 0)
    generic_parse_error = true;

  if (generic_parse_error)
  {
    if (mforms::Utilities::show_error(strfmt(_("Error Parsing DDL for %s"), obj_descr.c_str()),
      _("There was an error while parsing the DDL retrieved from the server.\n"
      "Do you want to view the DDL or cancel processing it?"),
      _("View DDL"), _("Cancel")) == mforms::ResultOk)
    {
      _owner->new_sql_scratch_area();
      insert_text_to_active_editor(ddl_script);
    }
    return false;
  }

  return true;
}

bool SqlEditorTreeController::apply_changes_to_object(bec::DBObjectEditorBE* obj_editor, bool dry_run)
{
  std::string log_descr;
  RowId log_id = -1;
  if (!dry_run)
  {
    log_descr = strfmt(_("Apply changes to %s"), obj_editor->get_name().c_str());
    log_id = _owner->add_log_message(DbSqlEditorLog::BusyMsg, "Preparing...", log_descr, "");
  }
  try
  {
    if (!dry_run && obj_editor->has_editor() && obj_editor->get_sql_editor()->has_sql_errors())
    {
      int res= mforms::Utilities::show_warning(
                                               _("Apply Changes to Object"),
                                               _("The object's DDL statement contains syntax errors.\n"
                                                 "Are you sure you want to apply the DDL statement unchanged?"),
                                               _("Yes"),
                                               _("No"));

      if (res != mforms::ResultOk)
      {
        _owner->set_log_message(log_id, DbSqlEditorLog::ErrorMsg, "Cancelled", log_descr, "");
        return false;
      }
    }

    db_DatabaseObjectRef db_object= obj_editor->get_dbobject();

    if (!dry_run)
    {
      ValueRef is_trigger_changed= db_object->customData().get("triggerInvalid");
      if (is_trigger_changed.is_valid() && (IntegerRef::cast_from(is_trigger_changed) != 0))
      {
        int res= mforms::Utilities::show_warning(
          _("Apply Changes to Object"),
          _("The tables's trigger SQL code contains errors.\n"
            "This will lead to invalid sql generated.\n"
            "Are you sure you want to apply the DDL statement as is?"),
          _("Yes"),
          _("No"));

        if (res != mforms::ResultOk)
        {
          _owner->set_log_message(log_id, DbSqlEditorLog::ErrorMsg, "Cancelled", log_descr, "");
          return false;
        }
      }

      // check for name conflicts
      // if the object is new or its name was changed
      std::string obj_name= db_object->name();
      std::string obj_old_name= db_object->oldName();
      if (_owner->lower_case_table_names() != 0) // if 1 or 2, treat everything as case insensitive
      {
        obj_name = tolower(obj_name);
        obj_old_name = tolower(obj_old_name);
        if (_owner->lower_case_table_names() == 1 && obj_name != *db_object->name() &&
            (db_TableRef::can_wrap(db_object) || db_ViewRef::can_wrap(db_object) || db_SchemaRef::can_wrap(db_object))) // server will force everything to be lowercase
        {
          mforms::Utilities::show_message_and_remember("Apply Changes to Object",
                                                       base::strfmt("The server is configured with lower_case_table_names=1, which only allows lowercase characters in schema and table names.\nThe object will be created as `%s`.",
                                                                    obj_name.c_str()),
                                                       "OK", "", "", "sqlide:lower_case_table_names", "");
          db_object->name(obj_name);
        }
      }
      // now here
      if (obj_name != obj_old_name)
      {
        std::list<std::string> obj_types;
        std::list<std::string> validation_queries;

        std::string schema_name= db_SchemaRef::can_wrap(db_object) ? std::string() : *db_object->owner()->name();

        {
          if (db_SchemaRef::can_wrap(db_object))
          {
            obj_types.push_back("schema");
            validation_queries.push_back(sqlstring("SHOW DATABASES LIKE ?", 0) << db_object->name());
          }
          else if (db_TableRef::can_wrap(db_object) || db_ViewRef::can_wrap(db_object))
          {
            obj_types.push_back("table");
            obj_types.push_back("view");
            std::string tables_format = base::strfmt("SHOW FULL TABLES FROM ! WHERE `Tables_in_%s` = ? AND Table_type != 'VIEW'", schema_name.c_str());
            std::string views_format = base::strfmt("SHOW FULL TABLES FROM ! WHERE `Tables_in_%s` = ? AND Table_type = 'VIEW'", schema_name.c_str());

            validation_queries.push_back(sqlstring(tables_format.c_str(), 0) << schema_name << db_object->name());
            validation_queries.push_back(sqlstring(views_format.c_str(), 0) << schema_name << db_object->name());
          }
          else if (db_RoutineRef::can_wrap(db_object))
          {
            db_RoutineRef db_routine = db_RoutineRef::cast_from(db_object);

            std::string type = db_routine->routineType();
            if (type == "function")
              validation_queries.push_back(sqlstring("SHOW FUNCTION STATUS WHERE Db=? AND NAME = ?", 0) << schema_name << db_routine->name());
            else
              validation_queries.push_back(sqlstring("SHOW PROCEDURE STATUS WHERE Db=? AND NAME = ?", 0) << schema_name << db_routine->name());

            obj_types.push_back(type);
          }
        }

        sql::Dbc_connection_handler::Ref conn;

        RecMutexLock lock(_owner->ensure_valid_aux_connection(conn));

        BOOST_FOREACH (const std::string &obj_type, obj_types)
        {
          std::string query = validation_queries.front();
          validation_queries.pop_front();

          std::auto_ptr<sql::ResultSet> rs(conn->ref->createStatement()->executeQuery(query));
          if (rs->next())
          {
            mforms::Utilities::show_error(
                                          _("Apply Changes to Object"),
                                          strfmt(_("Selected name conflicts with existing %s `%s`."), obj_type.c_str(), (*db_object->name()).c_str()),
                                          _("OK"));
            _owner->set_log_message(log_id, DbSqlEditorLog::ErrorMsg,
                            strfmt(_("Selected name conflicts with existing %s `%s`."), obj_type.c_str(), (*db_object->name()).c_str()),
                            log_descr, "");
            return false;
          }
        }
      }
    }

    // Generate the initial version of the alter script. This might be altered in the wizard
    // depending on the online DDL options.
    std::string algorithm;
    _owner->wbsql()->get_wbui()->get_wb_options_value("", "DbSqlEditor:OnlineDDLAlgorithm", algorithm);
    std::string lock;
    _owner->wbsql()->get_wbui()->get_wb_options_value("", "DbSqlEditor:OnlineDDLLock", lock);
    std::string alter_script = generate_alter_script(_owner->rdbms(), db_object, algorithm, lock);

    // the alter_script may contain a dummy USE statemnt
    if (alter_script.empty() || (alter_script.find("CREATE") == std::string::npos && alter_script.find("ALTER") == std::string::npos && alter_script.find("DROP") == std::string::npos))
    {
      if (!dry_run && !_owner->on_sql_script_run_error.empty())
        _owner->on_sql_script_run_error(log_id, _("No changes to object were detected."), "");
      if (!dry_run)
      {
        _owner->set_log_message(log_id, DbSqlEditorLog::NoteMsg, _("No changes detected"), log_descr, "");

        // Because of message throttling the previous log message doesn't cause a refresh of the UI
        // (it comes quicker than the throttling timeout). So we explicitly do a refresh.
        _owner->refresh_log_messages(true);
      }
      return false; // no changes detected
    }

    if (dry_run)
      return true; // some changes were detected

    //bool is_live_object_alteration_wizard_enabled= (0 != _options.get_int("DbSqlEditor:IsLiveObjectAlterationWizardEnabled", 1));
    if (true/*is_live_object_alteration_wizard_enabled*/)
    {
      return _owner->run_live_object_alteration_wizard(alter_script, obj_editor, log_id, log_descr);
    }
    else
    {
      _owner->apply_object_alter_script(alter_script, obj_editor, log_id);
    }
  }
  catch (const std::exception &e)
  {
    if (!_owner->on_sql_script_run_error.empty())
      _owner->on_sql_script_run_error(log_id, e.what(), "");
    _owner->set_log_message(log_id, DbSqlEditorLog::ErrorMsg, e.what(), log_descr, "");
    log_error("Exception applying changes to live object: %s\n", e.what());
  }
  return true; // some changes were detected and applied
}


void SqlEditorTreeController::create_live_table_stubs(bec::DBObjectEditorBE *table_editor)
{
  db_DatabaseObjectRef db_object= table_editor->get_dbobject();
  if (db_object->customData().has_key("isLiveTableListLoaded"))
    return;

  try
  {
    sql::Dbc_connection_handler::Ref conn;

    RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));

    db_CatalogRef catalog= table_editor->get_catalog();
    grt::ListRef<db_Schema> schemata= catalog->schemata();
    db_SchemaRef schema;
    grt::ListRef<db_Table> tables;
    db_TableRef table;

    std::string database_package= *_owner->rdbms()->databaseObjectPackage();
    std::string schema_typename= database_package + ".Schema";
    std::string table_typename= database_package + ".Table";
    grt::GRT *grt= _grtm->get_grt();

    std::string prev_schema_name;

    boost::shared_ptr<sql::ResultSet> rs;
    {
      std::string schema_name= db_SchemaRef::cast_from(db_object->owner())->name();
      std::list<sql::SQLString> table_types;
      table_types.push_back("TABLE");
      rs.reset(conn->ref->getMetaData()->getTables("", schema_name, "%", table_types));
    }
    while (rs->next())
    {
      std::string schema_name= rs->getString(2);
      std::string table_name= rs->getString(3);
      if (prev_schema_name != schema_name)
      {
        schema= find_named_object_in_list(schemata, schema_name);
        if (!schema.is_valid())
        {
          schema= grt->create_object<db_Schema>(schema_typename);
          schema->owner(catalog);
          schema->name(schema_name);
          schema->oldName(schema_name);
          schema->modelOnly(1);
          schemata.insert(schema);
        }
        tables= schema->tables();
        prev_schema_name= schema_name;
      }
      table= find_named_object_in_list(tables, table_name);
      if (!table.is_valid())
      {
        table= grt->create_object<db_Table>(table_typename);
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


bool SqlEditorTreeController::expand_live_table_stub(bec::DBObjectEditorBE *table_editor, const std::string &schema_name, const std::string &obj_name)
{
  db_CatalogRef catalog= table_editor->get_catalog();
  db_TableRef table;
  db_SchemaRef schema= find_named_object_in_list(catalog->schemata(), schema_name);
  if (schema.is_valid())
  {
    table= find_named_object_in_list(schema->tables(), obj_name);
    if (table.is_valid() && table->customData().has_key("isStubExpanded"))
      return true; // stub table has already been expanded
  }

  std::string ddl_script = get_object_ddl_script(wb::LiveSchemaTree::Table, schema_name, obj_name);
  if (ddl_script.empty())
    return false;

  {
    SqlFacade::Ref sql_facade= SqlFacade::instance_for_rdbms(_owner->rdbms());
    Sql_parser::Ref sql_parser= sql_facade->sqlParser();
    sql_parser->messages_enabled(false);
    grt::DictRef options(_grtm->get_grt());
    {
      std::string sql_mode;
      sql::Dbc_connection_handler::Ref conn;
      RecMutexLock aux_dbc_conn_mutex(_owner->ensure_valid_aux_connection(conn));
      if (conn && _owner->get_session_variable(conn->ref.get(), "sql_mode", sql_mode))
        options.gset("sql_mode", sql_mode);
      else
        log_warning("Unable to get sql_mode for connection");
    }
    db_SchemaRef old_default_schema = catalog->defaultSchema();
    if (!schema.is_valid())
    {
      // target schema doesn't exist yet, create a stub for it
      schema = db_mysql_SchemaRef(catalog.get_grt());
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
    schema= find_named_object_in_list(catalog->schemata(), schema_name);
  if (!table.is_valid() && schema.is_valid())
    table= find_named_object_in_list(schema->tables(), obj_name);

  if (table.is_valid() && table != table_editor->get_dbobject())
  {
    table->modelOnly(0);
    table->isStub(1);
    table->customData().set("isStubExpanded", IntegerRef(1));
  }

  return table.is_valid();
}




bool SqlEditorTreeController::activate_live_object(GrtObjectRef object)
{
  std::string obj_name= *object->name();
  std::string owner_name= *object->owner()->name();

  if (db_SchemaRef::can_wrap(object))
    schema_object_activated("activate", LiveSchemaTree::Schema, "", obj_name);
  else if (db_TableRef::can_wrap(object))
    schema_object_activated("activate", LiveSchemaTree::Table, owner_name, obj_name);
  else if (db_ViewRef::can_wrap(object))
    schema_object_activated("activate", LiveSchemaTree::View, owner_name, obj_name);
  else if (db_RoutineRef::can_wrap(object))
  {
    db_RoutineRef routine = db_RoutineRef::cast_from(object);
    std::string type = routine->routineType();

    if (type == "function")
      schema_object_activated("activate", LiveSchemaTree::Function, owner_name, obj_name);
    else
      schema_object_activated("activate", LiveSchemaTree::Procedure, owner_name, obj_name);
  }
  else
    return false;

  return true;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorTreeController::on_active_schema_change(const std::string &schema)
{
  _base_schema_tree.set_active_schema(schema);
  _filtered_schema_tree.set_active_schema(schema);

  if (_schema_side_bar != NULL)
    _grtm->run_once_when_idle(this, boost::bind(&mforms::View::set_needs_repaint, _schema_side_bar->get_schema_tree()));
}

//--------------------------------------------------------------------------------------------------

void SqlEditorTreeController::mark_busy(bool busy)
{
  if (_schema_side_bar != NULL)
    _schema_side_bar->mark_section_busy("", busy);
}

//--------------------------------------------------------------------------------------------------

grt::StringRef SqlEditorTreeController::do_refresh_schema_tree_safe(grt::GRT *grt, SqlEditorForm::Ptr self_ptr)
{
  RETVAL_IF_FAIL_TO_RETAIN_WEAK_PTR (SqlEditorForm, self_ptr, self, grt::StringRef(""))

  if (_is_refreshing_schema_tree)
    return grt::StringRef("");

  _is_refreshing_schema_tree= true;

  std::list<std::string> schema_list = fetch_schema_list();
  _grtm->run_once_when_idle(this, boost::bind(&LiveSchemaTree::update_schemata, _schema_tree, schema_list));

  _is_refreshing_schema_tree= false;

  return grt::StringRef("");
}


wb::LiveSchemaTree *SqlEditorTreeController::get_schema_tree()
{
  return _schema_tree;
}


void SqlEditorTreeController::handle_grt_notification(const std::string &name, grt::ObjectRef sender, grt::DictRef info)
{
  if (name == "GRNDBObjectEditorCreated")
  {
    grt::ValueRef object = info.get("object");
    bec::DBObjectEditorBE *editor= dynamic_cast <bec::DBObjectEditorBE *> (bec::UIForm::form_with_id(info.get_string("form")));
    if (editor && db_DatabaseObjectRef::can_wrap(object))
    {
      db_DatabaseObjectRef obj(db_DatabaseObjectRef::cast_from(object));
      if (obj->customData().get("ownerSqlEditor") == _owner->wbsql()->get_grt_editor_object(_owner))
      {
        editor->on_apply_changes_to_live_object= boost::bind(&SqlEditorTreeController::apply_changes_to_object, this, _1, _2);
        editor->on_refresh_live_object= boost::bind(&SqlEditorTreeController::refresh_live_object_in_editor, this, _1, true);
        editor->on_create_live_table_stubs= boost::bind(&SqlEditorTreeController::create_live_table_stubs, this, _1);
        editor->on_expand_live_table_stub= boost::bind(&SqlEditorTreeController::expand_live_table_stub, this, _1, _2, _3);
      }
    }
  }
  else if (name == "GRNPreferencesDidClose" && info.get_int("saved") == 1)
  {
    if (_grtm->get_app_option_int("DbSqlEditor:SidebarModeCombined", 0) == 1)
      sidebar_action("switch_mode_on");
    else
      sidebar_action("switch_mode_off");
  }
  else if (name == "GRNSQLEditorReconnected")
  {
    if (sender == _owner->wbsql()->get_grt_editor_object(_owner))
    {
      _session_info->set_markup_text(_owner->get_connection_info());
      tree_refresh();
    }
  }
}

int SqlEditorTreeController::insert_text_to_active_editor(const std::string& str)
{
  SqlEditorPanel *editor(_owner->active_sql_editor_panel());
  if (editor)
  {
    editor->editor_be()->insert_text(str);
    editor->editor_be()->focus();
  }
  return 0;
}


void SqlEditorTreeController::context_menu_will_show(mforms::MenuItem *parent_item)
{
  if (!parent_item)
  {
    grt::DictRef info(_owner->grt_manager()->get_grt());

    db_query_EditorRef sender(_owner->wbsql()->get_grt_editor_object(_owner));

    grt::ListRef<db_query_LiveDBObject> selection(grt::ListRef<db_query_LiveDBObject>::cast_from(_schema_tree->get_selected_objects()));

    info.set("menu", mforms_to_grt(info.get_grt(), _schema_side_bar->get_context_menu()));
    info.gset("menu-plugins-index", _schema_side_bar->get_context_menu()->get_item_index(_schema_side_bar->get_context_menu()->find_item("refresh"))-2);
    info.set("selection", selection);

    grt::GRTNotificationCenter::get()->send_grt("GRNLiveDBObjectMenuWillShow", sender, info);
  }
}

