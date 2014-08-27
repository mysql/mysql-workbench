/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "python_context.h"
#include "base/file_utilities.h"
#include "base/string_utilities.h"
#include "base/sqlstring.h"
#include "base/trackable.h"
#include "base/log.h"
#include "base/notifications.h"
#include "base/ui_form.h"

#include "grtpp.h"


#include "grts/structs.h"
#include "grts/structs.app.h"

#include "grt/editor_base.h"

#include "workbench/wb_context.h"
#include "workbench/wb_context_ui.h"
#include "workbench/wb_command_ui.h"

#include "sqlide/wb_context_sqlide.h"
#include "sqlide/wb_sql_editor_form.h"
#include "sqlide/wb_sql_editor_panel.h"
#include "sqlide/wb_sql_editor_snippets.h"
#include "sqlide/wb_sql_editor_help.h"
#include "sqlide/wb_sql_editor_result_panel.h"
#include "sqlide/wb_sql_editor_tree_controller.h"
#include "grt/common.h"

#include "objimpl/db.query/db_query_Editor.h"
#include "objimpl/db.query/db_query_Resultset.h"
#include "objimpl/db.query/db_query_EditableResultset.h"
#include "objimpl/db.query/db_query_QueryBuffer.h"
#include "objimpl/wrapper/grt_PyObject_impl.h"
#include "grts/structs.db.query.h"

#include "grtdb/db_helpers.h"

#include "mforms/code_editor.h"
#include "mforms/menubar.h"
#include "mforms/toolbar.h"

using namespace wb;
using namespace bec;
using namespace base;

DEFAULT_LOG_DOMAIN(DOMAIN_WQE_BE)

static std::map<std::string,std::string> auto_save_sessions;

//------------------------------------------------------------------------------------------------


class MYSQLWBBACKEND_PUBLIC_FUNC db_query_EditorConcreteImplData : public db_query_Editor::ImplData, public base::trackable
{
  void sql_editor_list_changed(MySQLEditor::Ref editor, bool added)
  {
    boost::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
    {
      if (added)
      {
        editor->grtobj()->owner(_self);
        _self->queryEditors().insert(db_query_QueryEditorRef::cast_from(editor->grtobj()));
      }
      else
      {
        _self->queryEditors().remove_value(db_query_QueryEditorRef::cast_from(editor->grtobj()));
        editor->grtobj()->reset_references();
      }
    }
  }

public:
  db_query_EditorConcreteImplData(boost::shared_ptr<SqlEditorForm> editor,
                                  const db_query_EditorRef &self)
  : _self(dynamic_cast<db_query_Editor*>(self.valueptr())), _editor(editor)
  {
    for (int c = editor->sql_editor_count(), i = 0; i < c; i++)
    {
      SqlEditorPanel *panel = editor->sql_editor_panel(i);
      if (panel)
      {
        db_query_QueryEditorRef qb(panel->grtobj());
        qb->owner(self);
        _self->queryEditors().insert(qb);
      }
    }
    
    editor->sql_editor_list_changed.connect(boost::bind(&db_query_EditorConcreteImplData::sql_editor_list_changed, this, _1, _2));
  }
  
  boost::shared_ptr<SqlEditorForm> editor_object() const { return _editor; }
  
  virtual db_mgmt_ConnectionRef connection() const
  {
    boost::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
      return _editor->connection_descriptor();
    return db_mgmt_ConnectionRef();
  }

  virtual grt::IntegerRef isConnected() const
  {
    boost::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
      return grt::IntegerRef(_editor->connected() ? 1 : 0);
    return grt::IntegerRef(0);
  }

  virtual db_query_QueryEditorRef addQueryEditor()
  {
    boost::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
    {
      _editor->new_sql_script_file();
      
      return _editor->active_sql_editor_panel()->grtobj();
    }
    return db_query_QueryEditorRef();
  }
  
  virtual grt::IntegerRef addToOutput(const std::string &text, long bringToFront)
  {
    boost::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
      ref->output_text_slot(text, bringToFront != 0);
    
    return grt::IntegerRef(0);
  }
  
  virtual grt::ListRef<db_query_Resultset> executeScript(const std::string &sql)
  {
    grt::ListRef<db_query_Resultset> result(_self->get_grt());
    boost::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
    { 
      ref->grt_manager()->replace_status_text("Executing query...");
      
      try
      {
        RecordsetsRef rsets(ref->exec_sql_returning_results(sql, true));

        for (std::vector<Recordset::Ref>::const_iterator iter= rsets->begin(); iter != rsets->end(); ++iter)
          result.insert(grtwrap_recordset(_self, *iter));
      
        ref->grt_manager()->replace_status_text("Query finished.");
      }
      catch (sql::SQLException &exc)
      {
        log_error("Exception executing SQL code from GRT interface: %s\n", exc.what());
      }
    }
    return result;
  }
  
  virtual grt::IntegerRef executeScriptAndOutputToGrid(const std::string &sql)
  {
    boost::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)      
      ref->exec_sql_retaining_editor_contents(sql, NULL, true);

    return grt::IntegerRef(0);
  }

  virtual db_query_ResultsetRef executeManagementQuery(const std::string &sql, bool log)
  {
    boost::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
    {
      return ref->exec_management_query(sql, log);
    }
    return db_query_ResultsetRef();
  }

  virtual void executeManagementCommand(const std::string &sql, bool log)
  {
    boost::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
      ref->exec_management_sql(sql, log);
  }
  
  virtual db_query_ResultsetRef executeQuery(const std::string &sql, bool log)
  {
    boost::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
    {
      return ref->exec_main_query(sql, log);
    }
    return db_query_ResultsetRef();
  }
  
  virtual void executeCommand(const std::string &sql, bool log, bool background)
  {
    boost::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
    {
      if (background)
        ref->exec_sql_retaining_editor_contents(sql, NULL, false);
      else
        ref->exec_main_sql(sql, log);
    }
  }

  virtual db_query_EditableResultsetRef createTableEditResultset(const std::string &schema, const std::string &table, const std::string &where, bool showGrid)
  {
    boost::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
    {      
      std::string query;
      
      query= base::sqlstring("SELECT * FROM !.!", 0) << schema << table;
      if (!where.empty())
        query.append(" ").append(where);
      
      if (showGrid)
      {
        executeScriptAndOutputToGrid(query);
      }
      else
      {
        RecordsetsRef rsets(ref->exec_sql_returning_results(query, true));
        
        if (rsets->size() == 1 && !(*rsets)[0]->is_readonly())
          return grtwrap_editablerecordset(_self, (*rsets)[0]);
      }
    }
    return db_query_EditableResultsetRef();
  }
  
  virtual void activeSchema(const std::string &schema)
  {
    boost::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)      
      ref->active_schema(schema);
  }
  
  virtual std::string activeSchema()
  {
    boost::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)      
      return ref->active_schema();
    return "";
  }
  
  virtual db_query_QueryEditorRef activeQueryEditor()
  {
    boost::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
    {
      SqlEditorPanel *panel = ref->active_sql_editor_panel();
      if (panel)
        return panel->grtobj();
    }
    return db_query_QueryEditorRef();
  }

  virtual void editLiveObject(const db_DatabaseObjectRef &object, const db_CatalogRef &catalog)
  {
    boost::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
    {
      ref->get_live_tree()->open_alter_object_editor(object, catalog);
    }
  }

  virtual void alterLiveObject(const std::string &type, const std::string &schemaName, const std::string &objectName)
  {
    boost::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
    {
      wb::LiveSchemaTree::ObjectType otype;
      if (type == "db.Schema")
        otype = wb::LiveSchemaTree::Schema;
      else if (type == "db.Table")
        otype = wb::LiveSchemaTree::Table;
      else if (type == "db.View")
        otype = wb::LiveSchemaTree::View;
      else if (type == "db.StoredProcedure")
        otype = wb::LiveSchemaTree::Procedure;
      else if (type == "db.Function")
        otype = wb::LiveSchemaTree::Function;
      else
        return;
      ref->get_live_tree()->do_alter_live_object(otype, schemaName, objectName);
    }
  }

  virtual grt::ListRef<db_query_LiveDBObject> schemaTreeSelection() const
  {
    boost::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
      return grt::ListRef<db_query_LiveDBObject>::cast_from(ref->get_live_tree()->get_schema_tree()->get_selected_objects());
    return grt::ListRef<db_query_LiveDBObject>();
  }

  void detach()
  {
    _editor.reset();
  }


  grt_PyObjectRef createCPyConnection()
  {
    boost::shared_ptr<SqlEditorForm> ref(_editor);

    WillEnterPython lock;
    grt::PythonContext *py = grt::PythonContext::get();
    py->run_buffer("import mysql.connector");
    PyObject *ctor = py->get_global("mysql.connector.Connect");
    if (!ctor)
      throw std::logic_error("Could not get handle to Connector method");

    grt::AutoPyObject kwarg(PyDict_New());

    grt::DictRef params(ref->connection_descriptor()->parameterValues());

    PyDict_SetItemString(kwarg, "host", grt::AutoPyObject(PyString_FromString(params.get_string("hostName").c_str())));
    PyDict_SetItemString(kwarg, "port", grt::AutoPyObject(PyLong_FromSize_t(params.get_int("port"))));
    PyDict_SetItemString(kwarg, "user", grt::AutoPyObject(PyString_FromString(params.get_string("userName").c_str())));
    PyDict_SetItemString(kwarg, "password", grt::AutoPyObject(PyString_FromString(ref->dbc_auth_data()->password())));

    grt::AutoPyObject connection(PyObject_Call(ctor, grt::AutoPyObject(PyTuple_New(0)), kwarg));
    if (!connection)
      throw grt::python_error("error opening connection");

    return pyobject_to_grt(_self->get_grt(), connection);
  }
    
protected:  
  db_query_Editor *_self;
  boost::shared_ptr<SqlEditorForm> _editor;
};


//------------------------------------------------------------------------------------------------

void WBContextSQLIDE::call_in_editor(void (SqlEditorForm::*method)())
{
  SqlEditorForm *form= get_active_sql_editor();
  if (form)
    (form->*method)();
}

void WBContextSQLIDE::call_in_editor_panel(void (SqlEditorPanel::*method)())
{
  SqlEditorForm *form= get_active_sql_editor();
  if (form)
  {
    SqlEditorPanel *panel = form->active_sql_editor_panel();
    if (panel)
      (panel->*method)();
  }
}


void WBContextSQLIDE::call_in_editor_str(void (SqlEditorForm::*method)(const std::string &arg), const std::string &arg)
{
  SqlEditorForm *form= get_active_sql_editor();
  if (form)
    (form->*method)(arg);
}


void WBContextSQLIDE::call_in_editor_bool(void (SqlEditorForm::*method)(bool arg), bool arg)
{
  SqlEditorForm *form= get_active_sql_editor();
  if (form)
    (form->*method)(arg);
}

static void call_export(wb::WBContextSQLIDE *sqlide)
{
  SqlEditorForm *form= sqlide->get_active_sql_editor();
  if (form)
  {
    SqlEditorPanel *panel = form->active_sql_editor_panel();
    if (panel && panel->active_result_panel())
      panel->active_result_panel()->show_export_recordset();
  }
}


inline bool has_active_resultset(wb::WBContextSQLIDE *sqlide)
{
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  if (form)
  {
    SqlEditorPanel *panel = form->active_sql_editor_panel();
    if (panel)
      return panel->active_result_panel() != NULL;
  }
  return false;
}

static bool validate_export(wb::WBContextSQLIDE *sqlide)
{
  return has_active_resultset(sqlide);
}

static void call_run_file(wb::WBContextSQLIDE *sqlide)
{
  std::string path= sqlide->get_wbui()->get_wb()->show_file_dialog("open", _("Execute SQL Script"), "SQL Files (*.sql)|*.sql");
  if (!path.empty())
    sqlide->run_file(path);
}

static void call_save_file(wb::WBContextSQLIDE *sqlide)
{
  SqlEditorForm *editor = sqlide->get_active_sql_editor();
  if (editor)
  {
    SqlEditorPanel *panel = editor->active_sql_editor_panel();
    if (panel)
    {
      panel->save();
    }
  }
}


static void call_save_file_as(wb::WBContextSQLIDE *sqlide)
{
  SqlEditorForm *editor = sqlide->get_active_sql_editor();
  if (editor)
  {
    SqlEditorPanel *panel = editor->active_sql_editor_panel();
    if (panel)
    {
      panel->save_as("");
    }
  }
}


static void call_revert(wb::WBContextSQLIDE *sqlide)
{
  SqlEditorForm *editor = sqlide->get_active_sql_editor();
  if (editor)
  {
    SqlEditorPanel *panel = editor->active_sql_editor_panel();
    if (panel)
    {
      if (panel->is_dirty())
      {
        int rc = mforms::Utilities::show_message(_("Revert to Saved"),
                            base::strfmt(_("Do you want to revert to the most recently saved version of '%s'?\nAny changes since them will be lost."),
                                         panel->filename().c_str()),
                                                 _("Revert"), _("Cancel"), "");
        if (rc != mforms::ResultOk)
          return;
        
        panel->revert_to_saved();
      }
    }
  }
}

static bool validate_revert(wb::WBContextSQLIDE *sqlide)
{
  SqlEditorForm *editor = sqlide->get_active_sql_editor();
  if (editor)
  {
    SqlEditorPanel *panel = editor->active_sql_editor_panel();
    if (panel)
      return !panel->is_scratch() && !panel->filename().empty();
  }
  return false;
}


static void call_continue_on_error(wb::WBContextSQLIDE *sqlide)
{
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  if (form)
    form->continue_on_error(!form->continue_on_error());
}


static void call_reconnect(wb::WBContextSQLIDE *sqlide)
{
  SqlEditorForm *form= sqlide->get_active_sql_editor();

  sqlide->reconnect_editor(form);
}


static void call_new_connection(wb::WBContextSQLIDE *sqlide)
{
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  if (form)
  {
    db_mgmt_ConnectionRef conn(form->connection_descriptor());
    sqlide->get_wbui()->get_wb()->add_new_query_window(conn);
  }
}


static void call_exec_sql(wb::WBContextSQLIDE *sqlide, bool current_statement_only)
{
  SqlEditorForm *form= sqlide->get_active_sql_editor();
  if (form)
    form->run_editor_contents(current_statement_only);
}  

static bool validate_exec_sql(wb::WBContextSQLIDE *sqlide)
{
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  return (form && !form->is_running_query() && form->connected());
}


static void call_save_edits(wb::WBContextSQLIDE *sqlide)
{
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  if (form)
  {
    SqlEditorPanel *panel = form->active_sql_editor_panel();
    if (panel)
    {
      SqlEditorResult *result = panel->active_result_panel();
      if (result)
        result->apply_changes();
    }
  }
}

static void call_discard_edits(wb::WBContextSQLIDE *sqlide)
{
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  if (form)
  {
    SqlEditorPanel *panel = form->active_sql_editor_panel();
    if (panel)
    {
      SqlEditorResult *result = panel->active_result_panel();
      if (result)
        result->discard_changes();
    }
  }
}

//--------------------------------------------------------------------------------------------------

static bool validate_save_edits(wb::WBContextSQLIDE *sqlide)
{
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  if (form)
  {
    SqlEditorPanel *panel = form->active_sql_editor_panel();
    if (panel)
    {
      SqlEditorResult *result = panel->active_result_panel();
      if (result)
        return result->has_pending_changes();
    }
  }
  return false;
}


//--------------------------------------------------------------------------------------------------

static bool validate_list_members(wb::WBContextSQLIDE *sqlide)
{
  return sqlide->get_grt_manager()->get_app_option_int("DbSqlEditor:CodeCompletionEnabled") != 0;
}

//--------------------------------------------------------------------------------------------------

static void new_script_tab(wb::WBContextSQLIDE *sqlide)
{
  SqlEditorForm *form= sqlide->get_active_sql_editor();
  if (form)
  {
    if (sqlide->get_grt_manager()->get_app_option_int("DbSqlEditor:DiscardUnsavedQueryTabs", 0))
      form->new_sql_scratch_area();
    else
      form->new_sql_script_file();
  }
}

//--------------------------------------------------------------------------------------------------

static bool validate_toolbar_alias_toggle(wb::WBContextSQLIDE *sqlide, const std::string &item_name);

static void call_toolbar_alias_toggle(wb::WBContextSQLIDE *sqlide, const std::string &item_name)
{
  SqlEditorForm *form= sqlide->get_active_sql_editor();
  if (form)
  {
    mforms::ToolBarItem *item = form->get_toolbar()->find_item(item_name);
    if (item)
    {
      item->set_checked(!item->get_checked());
      item->callback();
      validate_toolbar_alias_toggle(sqlide, item_name); // update menu item title
    }
  }
}

static bool validate_toolbar_alias_toggle(wb::WBContextSQLIDE *sqlide, const std::string &item_name)
{
  SqlEditorForm *form= sqlide->get_active_sql_editor();
  if (form)
  {
    mforms::ToolBarItem *item = form->get_toolbar()->find_item(item_name);
    mforms::MenuItem *mitem = NULL;

    mitem = form->get_menubar()->find_item("view");
    if (mitem)
      mitem = mitem->find_item("sidebars");
    if (mitem)
      mitem = mitem->find_item("alias."+item_name);
    if (item && mitem)
    {
      std::string title = mitem->get_title();
      if (item->get_checked())
        base::replace(title, "Show", "Hide");
      else
        base::replace(title, "Hide", "Show");
      mitem->set_title(title);
    }
  }
  return true;
}


//--------------------------------------------------------------------------------------------------

WBContextSQLIDE::WBContextSQLIDE(WBContextUI *wbui)
: _wbui(wbui), _auto_save_active(false), _option_change_signal_connected(false)
{
}

//--------------------------------------------------------------------------------------------------

WBContextSQLIDE::~WBContextSQLIDE()
{
  base::NotificationCenter::get()->remove_observer(this);
}

//--------------------------------------------------------------------------------------------------

void WBContextSQLIDE::option_changed(grt::internal::OwnedDict*dict, bool, const std::string&key)
{
  if (key == "workbench:AutoSaveSQLEditorInterval" && dict == _wbui->get_wb()->get_wb_options().valueptr())
  {
    auto_save_workspaces();
  }
}

//--------------------------------------------------------------------------------------------------

bool WBContextSQLIDE::auto_save_workspaces()
{
  WBContext *wb= _wbui->get_wb();
  ssize_t interval= wb->get_root()->options()->options().get_int("workbench:AutoSaveSQLEditorInterval", 60);
  if (interval <= 0 || !_auto_save_active)
    return false;

  for (std::list<boost::weak_ptr<SqlEditorForm> >::const_iterator iter = _open_editors.begin();
       iter != _open_editors.end(); ++iter)
  {
    SqlEditorForm::Ref editor((*iter).lock());
    try 
    {
      if (editor)
        editor->auto_save();
    }
    catch (const std::exception &exception)
    {
      log_warning("Exception during auto-save of SQL Editors: %s\n", exception.what());
      wb->get_grt_manager()->replace_status_text(base::strfmt("Error during auto-save of SQL Editors: %s", exception.what()));
    }
  }
  
  
  if (interval != _auto_save_interval)
  {
    // schedule new interval
    wb->get_grt_manager()->run_every(boost::bind(&WBContextSQLIDE::auto_save_workspaces, this), (double)interval);
    return false;
  }
  
  return true;
}

//--------------------------------------------------------------------------------------------------

void WBContextSQLIDE::detect_auto_save_files(const std::string &autosave_dir)
{
  // look for SQLEditor autosave workspace folders
  std::list<std::string> autosaves;
  try
  {
    autosaves = base::scan_for_files_matching(bec::make_path(autosave_dir, "sql_workspaces/*.autosave"));
  }
  catch (const std::runtime_error& e)
  {
    log_error("Error while scanning for sql workspaces: %s\n", e.what());
  }

  for (std::list<std::string>::const_iterator d = autosaves.begin(); d != autosaves.end(); ++d)
  {
    gchar *conn_id;
    gsize length;
    if (g_file_get_contents(bec::make_path(*d, "connection_id").c_str(),
                            &conn_id, &length, NULL))
    {
      ::auto_save_sessions[std::string(conn_id, length)] = *d;
      g_free(conn_id);
      log_info("Found auto-save workspace %s\n", d->c_str());
    }
    else
      log_warning("Found incomplete auto-save workspace %s\n", d->c_str());
  }
}


std::map<std::string, std::string> WBContextSQLIDE::auto_save_sessions()
{
  return ::auto_save_sessions;
}

//--------------------------------------------------------------------------------------------------

bec::GRTManager *WBContextSQLIDE::get_grt_manager()
{
  return _wbui->get_wb()->get_grt_manager();
}


CommandUI *WBContextSQLIDE::get_cmdui()
{
  return _wbui->get_command_ui();
}


void WBContextSQLIDE::handle_notification(const std::string &name, void *sender, base::NotificationInfo &info)
{
  if (name == "GNAppClosing")
    finalize();
}


void WBContextSQLIDE::init()
{
  DbSqlEditorSnippets::setup(this, bec::make_path(get_grt_manager()->get_user_datadir(), "snippets"));
  
  //scoped_connect(_wbui->get_wb()->signal_app_closing(),boost::bind(&WBContextSQLIDE::finalize, this));
  base::NotificationCenter::get()->add_observer(this, "GNAppClosing");
  
  // setup some builtin commands handled by ourselves for the SQL IDE
  wb::CommandUI *cmdui = _wbui->get_command_ui();

  cmdui->add_builtin_command("alias.wb.toggleSidebar", boost::bind(call_toolbar_alias_toggle, this, "wb.toggleSidebar"), boost::bind(validate_toolbar_alias_toggle, this, "wb.toggleSidebar"));
  cmdui->add_builtin_command("alias.wb.toggleSecondarySidebar", boost::bind(call_toolbar_alias_toggle, this, "wb.toggleSecondarySidebar"), boost::bind(validate_toolbar_alias_toggle, this, "wb.toggleSecondarySidebar"));
  cmdui->add_builtin_command("alias.wb.toggleOutputArea", boost::bind(call_toolbar_alias_toggle, this, "wb.toggleOutputArea"), boost::bind(validate_toolbar_alias_toggle, this, "wb.toggleOutputArea"));

  cmdui->add_builtin_command("query.execute", boost::bind(call_exec_sql, this, false), boost::bind(validate_exec_sql, this));
  cmdui->add_builtin_command("query.execute_current_statement", boost::bind(call_exec_sql, this, true), boost::bind(validate_exec_sql, this));
  
  cmdui->add_builtin_command("query.save_edits", boost::bind(call_save_edits, this), boost::bind(validate_save_edits, this));
  cmdui->add_builtin_command("query.discard_edits", boost::bind(call_discard_edits, this), boost::bind(validate_save_edits, this));
  
  cmdui->add_builtin_command("query.commit", boost::bind(&WBContextSQLIDE::call_in_editor, this, &SqlEditorForm::commit));
  cmdui->add_builtin_command("query.rollback", boost::bind(&WBContextSQLIDE::call_in_editor, this, &SqlEditorForm::rollback));
  cmdui->add_builtin_command("query.autocommit", boost::bind(&WBContextSQLIDE::call_in_editor, this, &SqlEditorForm::toggle_autocommit));
  cmdui->add_builtin_command("query.gatherPSInfo", boost::bind(&WBContextSQLIDE::call_in_editor, this, &SqlEditorForm::toggle_collect_ps_statement_events));

  cmdui->add_builtin_command("query.new_schema", boost::bind(&WBContextSQLIDE::call_in_editor_str, this, &SqlEditorForm::toolbar_command, "query.new_schema"));
  cmdui->add_builtin_command("query.show_inspector", boost::bind(&WBContextSQLIDE::call_in_editor_str, this, &SqlEditorForm::toolbar_command, "query.show_inspector"));
  cmdui->add_builtin_command("query.new_table", boost::bind(&WBContextSQLIDE::call_in_editor_str, this, &SqlEditorForm::toolbar_command, "query.new_table")); 
  cmdui->add_builtin_command("query.new_view", boost::bind(&WBContextSQLIDE::call_in_editor_str, this, &SqlEditorForm::toolbar_command, "query.new_view"));
  cmdui->add_builtin_command("query.new_routine", boost::bind(&WBContextSQLIDE::call_in_editor_str, this, &SqlEditorForm::toolbar_command, "query.new_routine"));
  cmdui->add_builtin_command("query.new_function", boost::bind(&WBContextSQLIDE::call_in_editor_str, this, &SqlEditorForm::toolbar_command, "query.new_function"));

  cmdui->add_builtin_command("query.new_connection", boost::bind(call_new_connection, this));
  
  cmdui->add_builtin_command("query.newQuery", boost::bind(&WBContextSQLIDE::call_in_editor, this, &SqlEditorForm::new_scratch_area));
  //cmdui->add_builtin_command("query.newFile", boost::bind(&WBContextSQLIDE::call_in_editor, this, &SqlEditorForm::new_sql_script_file));
  cmdui->add_builtin_command("query.newFile", boost::bind(new_script_tab, this));
  cmdui->add_builtin_command("query.openFile", boost::bind(&WBContextSQLIDE::call_in_editor_str, this, (void(SqlEditorForm::*)(const std::string&))&SqlEditorForm::open_file, ""));
  cmdui->add_builtin_command("query.saveFile", boost::bind(call_save_file, this));
  cmdui->add_builtin_command("query.saveFileAs", boost::bind(call_save_file_as, this));
  cmdui->add_builtin_command("query.revert", boost::bind(call_revert, this), boost::bind(validate_revert, this));

  cmdui->add_builtin_command("query.runFile", boost::bind(call_run_file, this));

  cmdui->add_builtin_command("query.export", boost::bind(call_export, this), boost::bind(validate_export, this));
  
  cmdui->add_builtin_command("query.cancel", boost::bind(&WBContextSQLIDE::call_in_editor, this, &SqlEditorForm::cancel_query));

  cmdui->add_builtin_command("query.reconnect", boost::bind(call_reconnect, this));

  cmdui->add_builtin_command("query.stopOnError", boost::bind(call_continue_on_error, this));
  
  cmdui->add_builtin_command("query.explain_current_statement",
                                               boost::bind(&WBContextSQLIDE::call_in_editor, this, &SqlEditorForm::explain_current_statement));

  cmdui->add_builtin_command("query.jump_to_placeholder", boost::bind(&WBContextSQLIDE::call_in_editor_panel, this, &SqlEditorPanel::jump_to_placeholder));
  cmdui->add_builtin_command("list-members", boost::bind(&WBContextSQLIDE::call_in_editor_panel, this, &SqlEditorPanel::list_members),
                             boost::bind(validate_list_members, this));
}


void WBContextSQLIDE::finalize()
{
  std::list<SqlEditorForm::Ptr>::iterator next, ed = _open_editors.begin(); 
  while (ed != _open_editors.end())
  {
    next = ed;
    ++next;
    if (!ed->expired())
    {
      ed->lock()->close();
    }
    ed = next;
  }
}


void WBContextSQLIDE::reconnect_editor(SqlEditorForm *editor)
{
  boost::shared_ptr<sql::TunnelConnection> tunnel;

  // open tunnel, if needed
  try
  {
    tunnel = sql::DriverManager::getDriverManager()->getTunnel(editor->connection_descriptor());
  }
  catch (grt::user_cancelled)
  {
    editor->grt_manager()->replace_status_text("Tunnel connection cancelled.");
    return;
  }
  try
  {
    if (editor && !editor->is_running_query())
    {
      editor->grt_manager()->replace_status_text("Reconnecting...");
      if (editor->connect(tunnel))
        editor->grt_manager()->replace_status_text("Connection reopened.");
      else
      {
        editor->grt_manager()->replace_status_text("Could not reconnect.");
        if (tunnel.get())
        {
          // check whether this was a tunnel related error
          std::string type, message;
          while (tunnel->get_message(type, message))
          {
            log_debug("From tunnel %s: %s\n", type.c_str(), message.c_str());
            if (type == "ERROR")
              mforms::Utilities::show_error("Reconnect", "Tunnel error: "+message, "OK");
          }
        }
      }
    }
  }
  catch (std::exception &exc)
  {
    SqlEditorForm::report_connection_failure(exc.what(), editor->connection_descriptor());
    return;
  }
}

static void *connect_editor(SqlEditorForm::Ref editor, boost::shared_ptr<sql::TunnelConnection> tunnel)
{
  try
  {
    log_debug3("Connecting SQL editor...\n");
    editor->connect(tunnel);
  }
  catch (sql::AuthenticationError &exc)
  {
    log_error("Got an authentication error during connection: %s\n", exc.what());
    return new std::string(exc.what());
  }
  catch (grt::user_cancelled &)
  {
    log_info("User cancelled connection\n");
    return new std::string(":CANCELLED");
  }
  catch (std::exception &exc)
  {
    if (tunnel.get())
    {
      // check whether this was a tunnel related error
      std::string type, message;
      while (tunnel->get_message(type, message))
      {
        log_debug("From tunnel %s: %s\n", type.c_str(), message.c_str());
        if (type == "ERROR")
          return new std::string("Tunnel error: "+message);
      }
    }
    log_error("Got an exception during connection: %s\n", exc.what());
    return new std::string(exc.what());
  }
  log_debug3("Connection to SQL editor succeeded\n");
  return new std::string();
}

static bool cancel_connect_editor(SqlEditorForm::Ref editor)
{
  log_debug3("Cancelling connection...\n");
  editor->cancel_connect();
  return true;
}

SqlEditorForm::Ref WBContextSQLIDE::create_connected_editor(const db_mgmt_ConnectionRef &conn)
{
  // start by opening the tunnel, if needed
  boost::shared_ptr<sql::TunnelConnection> tunnel = sql::DriverManager::getDriverManager()->getTunnel(conn);

  SqlEditorForm::Ref editor(SqlEditorForm::create(this, conn));

  void *result_ptr = 0;
  if (!mforms::Utilities::run_cancelable_task(_("Opening SQL Editor"), 
                                              strfmt(_("An SQL editor instance for '%s' is opening and should be available in a "
                                                       "moment.\n\nPlease stand by..."), conn->name().c_str()),
                                              boost::bind(connect_editor, editor, tunnel),
                                              boost::bind(cancel_connect_editor, editor),
                                              result_ptr))
    throw grt::user_cancelled("canceled");
  if (!result_ptr)
    throw grt::user_cancelled("connection error");
  
  std::string *result = (std::string*)result_ptr;
  if (result->empty())
    delete result;
  else
  {
    std::string tmp(*result);
    delete result;
    
    if (tmp == ":PASSWORD_EXPIRED")
    {
      grt::BaseListRef args(conn->get_grt(), grt::AnyType);
      args.ginsert(conn);
      ssize_t result = *grt::IntegerRef::cast_from(conn->get_grt()->call_module_function("WbAdmin", "handleExpiredPassword", args));
      if (result != 0)
        return create_connected_editor(conn);
      throw grt::user_cancelled("password reset cancelled by user");
    }
    else if (tmp == ":CANCELLED")
    {
      throw grt::user_cancelled("Cancelled");
    }
    
    throw std::runtime_error(tmp);
  }

  {
    // Create entry for grt tree and update volatile data in the connection.
    db_query_EditorRef object(_wbui->get_wb()->get_grt());
    object->owner(_wbui->get_wb()->get_root());
    object->name(conn->name());
    
    object->set_data(new db_query_EditorConcreteImplData(editor, object));

    {
      std::map<std::string, std::string> details(editor->connection_details());
      grt::DictRef parameter_values = conn->parameterValues();
      parameter_values.gset("serverVersion", details["dbmsProductVersion"]);
      std::time_t time = std::time(0);
      parameter_values.gset("lastConnected", (long)time);

      object->serverVersion(editor->rdbms_version());
    }

    _wbui->get_wb()->get_root()->sqlEditors().insert(object);
  }

  _open_editors.push_back(editor);

  editor->finish_startup();

  // setup auto-save for model
  if (!_auto_save_active)
  {
    _auto_save_active= true;
    ssize_t interval = _wbui->get_wb()->get_root()->options()->options().get_int("workbench:AutoSaveSQLEditorInterval", 60);
    if (interval > 0)
      _wbui->get_wb()->get_grt_manager()->run_every(boost::bind(&WBContextSQLIDE::auto_save_workspaces, this), (double)interval);
    _auto_save_interval = interval;

    if (!_option_change_signal_connected)
    {
      scoped_connect(_wbui->get_wb()->get_root()->options()->signal_dict_changed(),boost::bind(&WBContextSQLIDE::option_changed, this, _1, _2, _3));
      _option_change_signal_connected= true;
    }
  }
  
  if (::auto_save_sessions.find(conn.id()) != ::auto_save_sessions.end())
  {
    ::auto_save_sessions.erase(conn.id());
    _wbui->refresh_home_connections();
  }
  
  return editor;
}


SqlEditorForm* WBContextSQLIDE::get_active_sql_editor()
{
  bec::UIForm *form= _wbui->get_active_main_form();
  if (form)
    return dynamic_cast<SqlEditorForm*>(form);
  return 0;
}

bool WBContextSQLIDE::activate_live_object(GrtObjectRef object)
{
  SqlEditorForm *editor= get_active_sql_editor();
  if (!editor)
    return false;
  return editor->get_live_tree()->activate_live_object(object);
}

//--------------------------------------------------------------------------------------------------

void WBContextSQLIDE::open_document(const std::string &path)
{
  SqlEditorForm *editor= get_active_sql_editor();
  if (editor)
  {
    editor->open_file(path);
  }
  else
    mforms::Utilities::show_error(_("Open SQL Script"),
                                  _("Please select a connected SQL Editor tab to open a script file."),
                                  _("OK"));
}


void WBContextSQLIDE::run_file(const std::string &path)
{
  SqlEditorForm *editor= get_active_sql_editor();
  if (editor)
  {
    
  }
  else
    mforms::Utilities::show_error(_("Execute SQL Script"),
                                  _("Please select a connected SQL Editor tab to run a script file."),
                                  _("OK"));
}


static bool compare(SqlEditorForm::Ptr ptr, SqlEditorForm *editor)
{
  return ptr.lock().get() == editor;
}


void WBContextSQLIDE::editor_will_close(SqlEditorForm* editor)
{
  std::list<SqlEditorForm::Ptr>::iterator iter = std::find_if(_open_editors.begin(), _open_editors.end(), 
                                                              boost::bind(compare, _1, editor));
  if (iter != _open_editors.end())
  {
    // delete entry from grt tree
    grt::ListRef<db_query_Editor> editors(_wbui->get_wb()->get_root()->sqlEditors());
    
    for (size_t c= editors.count(), i= 0; i < c; i++)
    {
      db_query_EditorRef current_editor(editors[i]);
      if (dynamic_cast<db_query_EditorConcreteImplData*>(current_editor->get_data())->editor_object().get() == editor)
      {
        current_editor->reset_references();
        dynamic_cast<db_query_EditorConcreteImplData*>(current_editor->get_data())->detach();
        
        editors.remove(i);
        break;
      }
    }

    _open_editors.erase(iter);
    
    if (_open_editors.empty())
      _auto_save_active = false;
  }
}


bool WBContextSQLIDE::request_quit()
{  
  for (std::list<SqlEditorForm::Ptr>::iterator ed = _open_editors.begin(); ed != _open_editors.end(); ++ed)
  {
    if (!ed->expired() && !ed->lock()->can_close())
      return false;
  }
  return true;
}


void WBContextSQLIDE::update_plugin_arguments_pool(bec::ArgumentPool &args)
{
  SqlEditorForm *editor_ptr= get_active_sql_editor();
  if (editor_ptr)
  {
    db_query_EditorRef editor(get_grt_editor_object(editor_ptr));
    if (editor.is_valid())
    {
      db_query_QueryEditorRef qeditor(editor->activeQueryEditor());
      if (qeditor.is_valid())
      {
        db_query_ResultPanelRef rpanel(qeditor->activeResultPanel());
      
        args.add_entries_for_object("activeSQLEditor", editor);
        args.add_entries_for_object("activeQueryBuffer", qeditor);
        args.add_entries_for_object("activeQueryEditor", qeditor);
        args.add_entries_for_object("", qeditor);
        if (rpanel.is_valid() && rpanel->resultset().is_valid())
          args.add_entries_for_object("activeResultset", rpanel->resultset(), "db.query.Resultset");
      }
      else
        args.add_entries_for_object("activeSQLEditor", editor);
    }
  }
}


db_query_EditorRef WBContextSQLIDE::get_grt_editor_object(SqlEditorForm *editor)
{
  if (editor)
  {
    grt::ListRef<db_query_Editor> list(_wbui->get_wb()->get_root()->sqlEditors());
    for (grt::ListRef<db_query_Editor>::const_iterator ed= list.begin(); ed != list.end(); ++ed)
    {
      if (dynamic_cast<db_query_EditorConcreteImplData*>((*ed)->get_data())->editor_object().get() == editor)
        return *ed;
    }
  }
  return db_query_EditorRef();
}






//--------------------------------------------------------------------------------------------------

static struct RegisterNotifDocs
{
  RegisterNotifDocs()
  {
    base::NotificationCenter::get()->register_notification("GRNSQLEditorOpened",
                                                           "sqlide",
                                                           "Sent when a connection tab finishes initializing and is about to be shown on screen.",
                                                           "db.query.Editor instance",
                                                           "");

    base::NotificationCenter::get()->register_notification("GRNSQLEditorReconnected",
                                                           "sqlide",
                                                           "Sent when the connection state of a SQL editor changes (reconnect, disconnect) it's DB connection. Resent whenever the user clicks Reconnect.",
                                                           "db.query.Editor instance",
                                                           "connected - whether the connection is open");

    base::NotificationCenter::get()->register_notification("GRNServerStateChanged",
                                                           "sqlide",
                                                           "Sent by the Admin module when it is detected that the server state (running or stopped) changed.",
                                                           "db.query.Editor instance",
                                                           "state - running or stopped\n"
                                                           "connection - the connection object for the server");

    base::NotificationCenter::get()->register_notification("GRNLiveDBObjectMenuWillShow",
                                                           "sqlide",
                                                           "Sent when the context menu is about to be shown for a live DB object (eg. from the live schema tree in the SQL IDE sidebar).",
                                                           "db.query.Editor instance",
                                                           "menu - mforms.ObjectReference of a mforms.Menu object which is being shown\n"
                                                           "selection - a list of db.query.LiveDBObject for the selected objects");

    base::NotificationCenter::get()->register_notification("GRNLiveDBObjectSelectionDidChange",
                                                           "sqlide",
                                                           "Sent when the selection in the schema tree changes. Avoid hooking slow actions to this as it will make object selection less smooth.",
                                                           "db.query.Editor instance",
                                                           "selection-size - number of selected items in the tree");

    base::NotificationCenter::get()->register_notification("GRNSQLResultSetMenuWillShow",
                                                         "sqlide",
                                                         "Sent when the context menu is about to be shown for a resultset in the SQL IDE.",
                                                         "db.query.Resultset instance",
                                                         "menu - mforms.ObjectReference of a mforms.Menu object which is being shown\n"
                                                         "selected_rows - a list of int values of the selected rows\n"
                                                         "selected_column - if this value is set, the selection is a single cell at selected_rows[0], selected_column");
  }
} initdocs;


