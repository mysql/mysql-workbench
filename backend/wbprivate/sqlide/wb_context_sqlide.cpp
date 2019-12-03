/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "python_context.h"
#include "base/file_utilities.h"
#include "base/string_utilities.h"
#include "base/sqlstring.h"
#include "base/trackable.h"
#include "base/log.h"
#include "base/notifications.h"
#include "base/ui_form.h"

#include "grt.h"

#include "grts/structs.h"
#include "grts/structs.app.h"

#include "grt/editor_base.h"
#include "grtui/grtdb_connect_dialog.h"

#include "workbench/wb_context.h"
#include "workbench/wb_context_ui.h"
#include "workbench/wb_command_ui.h"
#include "workbench/SSHSessionWrapper.h"

#include "sqlide/wb_context_sqlide.h"
#include "sqlide/wb_sql_editor_form.h"
#include "sqlide/wb_sql_editor_panel.h"
#include "sqlide/wb_sql_editor_snippets.h"
#include "sqlide/wb_sql_editor_help.h"
#include "sqlide/wb_sql_editor_result_panel.h"
#include "sqlide/wb_sql_editor_tree_controller.h"

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
#include "mforms/filechooser.h"

using namespace wb;
using namespace bec;
using namespace base;

DEFAULT_LOG_DOMAIN(DOMAIN_WQE_BE)

static std::map<std::string, std::string> auto_save_sessions;

//------------------------------------------------------------------------------------------------

class MYSQLWBBACKEND_PUBLIC_FUNC db_query_EditorConcreteImplData : public db_query_Editor::ImplData,
                                                                   public base::trackable {
  void sql_editor_list_changed(MySQLEditor::Ref editor, bool added) {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref) {
      if (added) {
        editor->grtobj()->owner(_self);
        _self->queryEditors().insert(db_query_QueryEditorRef::cast_from(editor->grtobj()));
      } else {
        _self->queryEditors().remove_value(db_query_QueryEditorRef::cast_from(editor->grtobj()));
        editor->grtobj()->reset_references();
      }
    }
  }

public:
  db_query_EditorConcreteImplData(std::shared_ptr<SqlEditorForm> editor, const db_query_EditorRef &self)
    : _self(dynamic_cast<db_query_Editor *>(self.valueptr())), _editor(editor) {
    for (int c = editor->sql_editor_count(), i = 0; i < c; i++) {
      SqlEditorPanel *panel = editor->sql_editor_panel(i);
      if (panel) {
        db_query_QueryEditorRef qb(panel->grtobj());
        qb->owner(self);
        _self->queryEditors().insert(qb);
      }
    }

    editor->sql_editor_list_changed.connect(std::bind(&db_query_EditorConcreteImplData::sql_editor_list_changed, this,
                                                      std::placeholders::_1, std::placeholders::_2));
  }

  std::shared_ptr<SqlEditorForm> editor_object() const {
    return _editor;
  }

  virtual db_mgmt_ConnectionRef connection() const {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
      return _editor->connection_descriptor();
    return db_mgmt_ConnectionRef();
  }

  virtual db_mgmt_SSHConnectionRef sshConnection() const {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
      return _editor->getSSHConnection();
    return db_mgmt_SSHConnectionRef();
  }

  virtual grt::IntegerRef getSSHTunnelPort() const {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
      return _editor->getTunnelPort();
    return -1;
  }

  virtual grt::IntegerRef isConnected() const {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref) {
      if (_editor->offline())
        return grt::IntegerRef(-1);
      else {
        if (_editor->connected())
          return grt::IntegerRef(1);
        else
          return grt::IntegerRef(0);
      }
    }
    return grt::IntegerRef(0);
  }

  virtual db_query_QueryEditorRef addQueryEditor() {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref) {
      _editor->new_sql_script_file();

      return _editor->active_sql_editor_panel()->grtobj();
    }
    return db_query_QueryEditorRef();
  }

  virtual grt::IntegerRef addToOutput(const std::string &text, long bringToFront) {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
      ref->output_text_slot(text, bringToFront != 0);

    return grt::IntegerRef(0);
  }

  virtual grt::ListRef<db_query_Resultset> executeScript(const std::string &sql) {
    grt::ListRef<db_query_Resultset> result(true);
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref) {
      bec::GRTManager::get()->replace_status_text("Executing query...");

      try {
        RecordsetsRef rsets(ref->exec_sql_returning_results(sql, true));

        for (std::vector<Recordset::Ref>::const_iterator iter = rsets->begin(); iter != rsets->end(); ++iter)
          result.insert(grtwrap_recordset(_self, *iter));

        bec::GRTManager::get()->replace_status_text("Query finished.");
      } catch (sql::SQLException &exc) {
        logError("Exception executing SQL code from GRT interface: %s\n", exc.what());
      }
    }
    return result;
  }

  virtual grt::IntegerRef executeScriptAndOutputToGrid(const std::string &sql) {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
      ref->exec_sql_retaining_editor_contents(sql, NULL, true);

    return grt::IntegerRef(0);
  }

  virtual db_query_ResultsetRef executeManagementQuery(const std::string &sql, bool log) {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref) {
      return ref->exec_management_query(sql, log);
    }
    return db_query_ResultsetRef();
  }

  virtual void executeManagementCommand(const std::string &sql, bool log) {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
      ref->exec_management_sql(sql, log);
  }

  virtual db_query_ResultsetRef executeQuery(const std::string &sql, bool log) {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref) {
      return ref->exec_main_query(sql, log);
    }
    return db_query_ResultsetRef();
  }

  virtual void executeCommand(const std::string &sql, bool log, bool background) {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref) {
      if (background)
        ref->exec_sql_retaining_editor_contents(sql, NULL, false);
      else
        ref->exec_main_sql(sql, log);
    }
  }

  virtual db_query_EditableResultsetRef createTableEditResultset(const std::string &schema, const std::string &table,
                                                                 const std::string &where, bool showGrid) {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref) {
      std::string query;

      query = base::sqlstring("SELECT * FROM !.!", 0) << schema << table;
      if (!where.empty())
        query.append(" ").append(where);

      if (showGrid) {
        executeScriptAndOutputToGrid(query);
      } else {
        RecordsetsRef rsets(ref->exec_sql_returning_results(query, true));

        if (rsets->size() == 1 && !(*rsets)[0]->is_readonly())
          return grtwrap_editablerecordset(_self, (*rsets)[0]);
      }
    }
    return db_query_EditableResultsetRef();
  }

  virtual void activeSchema(const std::string &schema) {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
      ref->active_schema(schema);
  }

  virtual std::string activeSchema() {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
      return ref->active_schema();
    return "";
  }

  virtual db_query_QueryEditorRef activeQueryEditor() {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref) {
      SqlEditorPanel *panel = ref->active_sql_editor_panel();
      if (panel)
        return panel->grtobj();
    }
    return db_query_QueryEditorRef();
  }

  virtual void editLiveObject(const db_DatabaseObjectRef &object, const db_CatalogRef &catalog) {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref) {
      ref->get_live_tree()->open_alter_object_editor(object, catalog);
    }
  }

  virtual void alterLiveObject(const std::string &type, const std::string &schemaName, const std::string &objectName) {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref) {
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

  virtual grt::ListRef<db_query_LiveDBObject> schemaTreeSelection() const {
    std::shared_ptr<SqlEditorForm> ref(_editor);
    if (ref)
      return grt::ListRef<db_query_LiveDBObject>::cast_from(
        ref->get_live_tree()->get_schema_tree()->get_selected_objects());
    return grt::ListRef<db_query_LiveDBObject>();
  }

  void detach() {
    _editor.reset();
  }

protected:
  db_query_Editor *_self;
  std::shared_ptr<SqlEditorForm> _editor;
};

//------------------------------------------------------------------------------------------------

void WBContextSQLIDE::call_in_editor(void (SqlEditorForm::*method)()) {
  SqlEditorForm *form = get_active_sql_editor();
  if (form)
    (form->*method)();
}

void WBContextSQLIDE::call_in_editor_panel(void (SqlEditorPanel::*method)()) {
  SqlEditorForm *form = get_active_sql_editor();
  if (form) {
    SqlEditorPanel *panel = form->active_sql_editor_panel();
    if (panel)
      (panel->*method)();
  }
}

void WBContextSQLIDE::call_in_editor_str(void (SqlEditorForm::*method)(const std::string &arg),
                                         const std::string &arg) {
  SqlEditorForm *form = get_active_sql_editor();
  if (form)
    (form->*method)(arg);
}

void WBContextSQLIDE::call_in_editor_str2(void (SqlEditorForm::*method)(const std::string &arg1, bool arg2, bool arg3),
                                          const std::string &arg1, bool arg2, bool arg3) {
  SqlEditorForm *form = get_active_sql_editor();
  if (form)
    (form->*method)(arg1, arg2, arg3);
}

void WBContextSQLIDE::call_in_editor_bool(void (SqlEditorForm::*method)(bool arg), bool arg) {
  SqlEditorForm *form = get_active_sql_editor();
  if (form)
    (form->*method)(arg);
}

static void call_export(wb::WBContextSQLIDE *sqlide) {
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  if (form) {
    SqlEditorPanel *panel = form->active_sql_editor_panel();
    if (panel && panel->active_result_panel())
      panel->active_result_panel()->show_export_recordset();
  }
}

inline bool has_active_resultset(wb::WBContextSQLIDE *sqlide) {
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  if (form) {
    SqlEditorPanel *panel = form->active_sql_editor_panel();
    if (panel)
      return panel->active_result_panel() != NULL;
  }
  return false;
}

static bool validate_export(wb::WBContextSQLIDE *sqlide) {
  return has_active_resultset(sqlide);
}

static void call_save_file(wb::WBContextSQLIDE *sqlide) {
  SqlEditorForm *editor = sqlide->get_active_sql_editor();
  if (editor) {
    SqlEditorPanel *panel = editor->active_sql_editor_panel();
    if (panel) {
      panel->save();
    }
  }
}

static void call_save_file_as(wb::WBContextSQLIDE *sqlide) {
  SqlEditorForm *editor = sqlide->get_active_sql_editor();
  if (editor) {
    SqlEditorPanel *panel = editor->active_sql_editor_panel();
    if (panel) {
      panel->save_as("");
    }
  }
}

static void call_revert(wb::WBContextSQLIDE *sqlide) {
  SqlEditorForm *editor = sqlide->get_active_sql_editor();
  if (editor) {
    SqlEditorPanel *panel = editor->active_sql_editor_panel();
    if (panel) {
      if (panel->is_dirty()) {
        int rc = mforms::Utilities::show_message(
          _("Revert to Saved"), base::strfmt(_("Do you want to revert to the most recently saved version of '%s'?\nAny "
                                               "changes since them will be lost."),
                                             panel->filename().c_str()),
          _("Revert"), _("Cancel"), "");
        if (rc != mforms::ResultOk)
          return;

        panel->revert_to_saved();
      }
    }
  }
}

static bool validate_revert(wb::WBContextSQLIDE *sqlide) {
  SqlEditorForm *editor = sqlide->get_active_sql_editor();
  if (editor) {
    SqlEditorPanel *panel = editor->active_sql_editor_panel();
    if (panel)
      return !panel->is_scratch() && !panel->filename().empty();
  }
  return false;
}

static void call_continue_on_error(wb::WBContextSQLIDE *sqlide) {
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  if (form)
    form->continue_on_error(!form->continue_on_error());
}

static void call_reconnect(wb::WBContextSQLIDE *sqlide) {
  SqlEditorForm *form = sqlide->get_active_sql_editor();

  sqlide->reconnect_editor(form);
}

static void call_new_connection(wb::WBContextSQLIDE *sqlide) {
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  if (form) {
    db_mgmt_ConnectionRef conn(form->connection_descriptor());
    wb::WBContextUI::get()->get_wb()->add_new_query_window(conn);
  }
}

static bool validate_has_connection(wb::WBContextSQLIDE *sqlide) {
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  return form && form->connection_descriptor().is_valid();
}

static void call_open_script(wb::WBContextSQLIDE *sqlide) {
  mforms::FileChooser chooser(mforms::OpenFile);
  chooser.set_title("Open SQL Script");
  chooser.set_extensions("SQL Files (*.sql)|*.sql|Query Browser Files (*.qbquery)|*.qbquery", "sql");
  if (chooser.run_modal()) {
    std::shared_ptr<SqlEditorForm> form = wb::WBContextUI::get()->get_wb()->add_new_query_window();
    if (form) {
      form->open_file(chooser.get_path());
    }
  }
}

static void call_no_connection_empty_tab(wb::WBContextSQLIDE *sqlide) {
  std::shared_ptr<SqlEditorForm> form = wb::WBContextUI::get()->get_wb()->add_new_query_window();
  if (form)
    form->open_file();
}

static void call_exec_sql(wb::WBContextSQLIDE *sqlide, bool current_statement_only) {
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  if (form)
    form->run_editor_contents(current_statement_only);
}

static bool validate_exec_sql(wb::WBContextSQLIDE *sqlide) {
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  return (form && !form->is_running_query() && form->connected());
}

static void call_save_edits(wb::WBContextSQLIDE *sqlide) {
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  if (form) {
    SqlEditorPanel *panel = form->active_sql_editor_panel();
    if (panel) {
      SqlEditorResult *result = panel->active_result_panel();
      if (result)
        result->apply_changes();
    }
  }
}

static void call_discard_edits(wb::WBContextSQLIDE *sqlide) {
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  if (form) {
    SqlEditorPanel *panel = form->active_sql_editor_panel();
    if (panel) {
      SqlEditorResult *result = panel->active_result_panel();
      if (result)
        result->discard_changes();
    }
  }
}

//--------------------------------------------------------------------------------------------------

static bool validate_save_edits(wb::WBContextSQLIDE *sqlide) {
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  if (form) {
    SqlEditorPanel *panel = form->active_sql_editor_panel();
    if (panel) {
      SqlEditorResult *result = panel->active_result_panel();
      if (result)
        return result->has_pending_changes();
    }
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

static bool validate_list_members(wb::WBContextSQLIDE *sqlide) {
  return bec::GRTManager::get()->get_app_option_int("DbSqlEditor:CodeCompletionEnabled") != 0;
}

//--------------------------------------------------------------------------------------------------

static void new_script_tab(wb::WBContextSQLIDE *sqlide) {
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  if (form) {
    if (bec::GRTManager::get()->get_app_option_int("DbSqlEditor:DiscardUnsavedQueryTabs", 0))
      form->new_sql_scratch_area();
    else
      form->new_sql_script_file();
  }
}

//--------------------------------------------------------------------------------------------------

static bool validate_toolbar_alias_toggle(wb::WBContextSQLIDE *sqlide, const std::string &item_name);

static void call_toolbar_alias_toggle(wb::WBContextSQLIDE *sqlide, const std::string &item_name) {
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  if (form) {
    mforms::ToolBarItem *item = form->get_toolbar()->find_item(item_name);
    if (item) {
      item->set_checked(!item->get_checked());
      item->callback();
      validate_toolbar_alias_toggle(sqlide, item_name); // update menu item title
    }
  }
}

static bool validate_toolbar_alias_toggle(wb::WBContextSQLIDE *sqlide, const std::string &item_name) {
  SqlEditorForm *form = sqlide->get_active_sql_editor();
  if (form) {
    mforms::ToolBarItem *item = form->get_toolbar()->find_item(item_name);
    mforms::MenuItem *mitem = NULL;

    mitem = form->get_menubar()->find_item("view");
    if (mitem)
      mitem = mitem->find_item("sidebars");
    if (mitem)
      mitem = mitem->find_item("alias." + item_name);
    if (item && mitem) {
      std::string title = mitem->get_title();
      if (item->get_checked())
        base::replaceStringInplace(title, "Show", "Hide");
      else
        base::replaceStringInplace(title, "Hide", "Show");
      mitem->set_title(title);
    }
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

WBContextSQLIDE::WBContextSQLIDE()
  : _auto_save_handle(0), _auto_save_interval(0), _auto_save_active(false), _option_change_signal_connected(false) {
}

//--------------------------------------------------------------------------------------------------

WBContextSQLIDE::~WBContextSQLIDE() {
  if (_auto_save_handle)
    mforms::Utilities::cancel_timeout(_auto_save_handle);
  base::NotificationCenter::get()->remove_observer(this);
}

//--------------------------------------------------------------------------------------------------

void WBContextSQLIDE::option_changed(grt::internal::OwnedDict *dict, bool, const std::string &key) {
  if (key == "workbench:AutoSaveSQLEditorInterval" &&
      dict == WBContextUI::get()->get_wb()->get_wb_options().valueptr()) {
    auto_save_workspaces();
  }
}

//--------------------------------------------------------------------------------------------------

bool WBContextSQLIDE::auto_save_workspaces() {
  WBContext *wb = WBContextUI::get()->get_wb();
  ssize_t interval = wb->get_root()->options()->options().get_int("workbench:AutoSaveSQLEditorInterval", 60);
  if (interval <= 0 || !_auto_save_active) {
    _auto_save_handle = static_cast<mforms::TimeoutHandle>(NULL);
    _auto_save_active = false;
    return false;
  }
  for (std::list<std::weak_ptr<SqlEditorForm>>::const_iterator iter = _open_editors.begin();
       iter != _open_editors.end(); ++iter) {
    SqlEditorForm::Ref editor((*iter).lock());
    try {
      if (editor)
        editor->auto_save();
    } catch (const std::exception &exception) {
      logWarning("Exception during auto-save of SQL Editors: %s\n", exception.what());
      bec::GRTManager::get()->replace_status_text(
        base::strfmt("Error during auto-save of SQL Editors: %s", exception.what()));
    }
  }

  if (interval != _auto_save_interval) {
    _auto_save_interval = interval;
    if (_auto_save_handle)
      mforms::Utilities::cancel_timeout(_auto_save_handle);
    // schedule new interval
    _auto_save_handle =
      mforms::Utilities::add_timeout((float)interval, std::bind(&WBContextSQLIDE::auto_save_workspaces, this));
    return false;
  }

  return true;
}

//--------------------------------------------------------------------------------------------------

void WBContextSQLIDE::detect_auto_save_files(const std::string &autosave_dir) {
  // look for SQLEditor autosave workspace folders
  std::list<std::string> autosaves;
  try {
    autosaves = base::scan_for_files_matching(base::makePath(autosave_dir, "sql_workspaces/*.autosave"));
  } catch (const std::runtime_error &e) {
    logError("Error while scanning for sql workspaces: %s\n", e.what());
  }

  for (std::list<std::string>::const_iterator d = autosaves.begin(); d != autosaves.end(); ++d) {
    gchar *conn_id;
    gsize length;
    if (g_file_get_contents(base::makePath(*d, "connection_id").c_str(), &conn_id, &length, NULL)) {
      ::auto_save_sessions[std::string(conn_id, length)] = *d;
      g_free(conn_id);
      logInfo("Found auto-save workspace %s\n", d->c_str());
    } else
      logWarning("Found incomplete auto-save workspace %s\n", d->c_str());
  }
}

std::map<std::string, std::string> WBContextSQLIDE::auto_save_sessions() {
  return ::auto_save_sessions;
}

//----------------------------------------------------------------------------------------------------------------------

CommandUI *WBContextSQLIDE::get_cmdui() {
  return wb::WBContextUI::get()->get_command_ui();
}

//----------------------------------------------------------------------------------------------------------------------

void WBContextSQLIDE::handle_notification(const std::string &name, void *sender, base::NotificationInfo &info) {
  if (name == "GNAppClosing")
    finalize();
}

//----------------------------------------------------------------------------------------------------------------------

void WBContextSQLIDE::init() {

  // Access the context help once to start its initial loading.
  help::DbSqlEditorContextHelp::get();

  DbSqlEditorSnippets::setup(this, base::makePath(bec::GRTManager::get()->get_user_datadir(), "snippets"));

  // scoped_connect(wb::WBContextUI::get()->get_wb()->signal_app_closing(),std::bind(&WBContextSQLIDE::finalize, this));
  base::NotificationCenter::get()->add_observer(this, "GNAppClosing");

  // Setup some builtin commands handled by ourselves for the SQL IDE.
  wb::CommandUI *cmdui = wb::WBContextUI::get()->get_command_ui();

  cmdui->add_builtin_command("alias.wb.toggleSidebar", std::bind(call_toolbar_alias_toggle, this, "wb.toggleSidebar"),
                             std::bind(validate_toolbar_alias_toggle, this, "wb.toggleSidebar"));
  cmdui->add_builtin_command("alias.wb.toggleSecondarySidebar",
                             std::bind(call_toolbar_alias_toggle, this, "wb.toggleSecondarySidebar"),
                             std::bind(validate_toolbar_alias_toggle, this, "wb.toggleSecondarySidebar"));
  cmdui->add_builtin_command("alias.wb.toggleOutputArea",
                             std::bind(call_toolbar_alias_toggle, this, "wb.toggleOutputArea"),
                             std::bind(validate_toolbar_alias_toggle, this, "wb.toggleOutputArea"));

  cmdui->add_builtin_command("query.execute", std::bind(call_exec_sql, this, false),
                             std::bind(validate_exec_sql, this));
  cmdui->add_builtin_command("query.execute_current_statement", std::bind(call_exec_sql, this, true),
                             std::bind(validate_exec_sql, this));

  cmdui->add_builtin_command("query.explain_current_statement", std::bind(&WBContextSQLIDE::call_in_editor, this,
                                                                          &SqlEditorForm::explain_current_statement),
                             std::bind(validate_exec_sql, this));

  cmdui->add_builtin_command("query.save_edits", std::bind(call_save_edits, this),
                             std::bind(validate_save_edits, this));
  cmdui->add_builtin_command("query.discard_edits", std::bind(call_discard_edits, this),
                             std::bind(validate_save_edits, this));

  cmdui->add_builtin_command("query.commit", std::bind(&WBContextSQLIDE::call_in_editor, this, &SqlEditorForm::commit));
  cmdui->add_builtin_command("query.rollback",
                             std::bind(&WBContextSQLIDE::call_in_editor, this, &SqlEditorForm::rollback));
  cmdui->add_builtin_command("query.autocommit",
                             std::bind(&WBContextSQLIDE::call_in_editor, this, &SqlEditorForm::toggle_autocommit));
  cmdui->add_builtin_command("query.gatherPSInfo", std::bind(&WBContextSQLIDE::call_in_editor, this,
                                                             &SqlEditorForm::toggle_collect_ps_statement_events));

  cmdui->add_builtin_command("query.new_schema", std::bind(&WBContextSQLIDE::call_in_editor_str, this,
                                                           &SqlEditorForm::toolbar_command, "query.new_schema"));
  cmdui->add_builtin_command(
    "query.show_inspector",
    std::bind(&WBContextSQLIDE::call_in_editor_str, this, &SqlEditorForm::toolbar_command, "query.show_inspector"));
  cmdui->add_builtin_command("query.new_table", std::bind(&WBContextSQLIDE::call_in_editor_str, this,
                                                          &SqlEditorForm::toolbar_command, "query.new_table"));
  cmdui->add_builtin_command("query.new_view", std::bind(&WBContextSQLIDE::call_in_editor_str, this,
                                                         &SqlEditorForm::toolbar_command, "query.new_view"));
  cmdui->add_builtin_command("query.new_routine", std::bind(&WBContextSQLIDE::call_in_editor_str, this,
                                                            &SqlEditorForm::toolbar_command, "query.new_routine"));
  cmdui->add_builtin_command("query.new_function", std::bind(&WBContextSQLIDE::call_in_editor_str, this,
                                                             &SqlEditorForm::toolbar_command, "query.new_function"));

  cmdui->add_builtin_command("query.new_connection", std::bind(call_new_connection, this),
                             std::bind(validate_has_connection, this));

  cmdui->add_builtin_command("query.openScriptNoConnection", std::bind(call_open_script, this));

  cmdui->add_builtin_command("query.newQueryNoconnection", std::bind(call_no_connection_empty_tab, this));
  cmdui->add_builtin_command("query.newQuery",
                             std::bind(&WBContextSQLIDE::call_in_editor, this, &SqlEditorForm::new_scratch_area));
  cmdui->add_builtin_command("query.newFile", std::bind(new_script_tab, this));
  cmdui->add_builtin_command(
    "query.openFile",
    std::bind(&WBContextSQLIDE::call_in_editor_str2, this,
              (void (SqlEditorForm::*)(const std::string &, bool, bool)) & SqlEditorForm::open_file, "", true, true));
  cmdui->add_builtin_command("query.saveFile", std::bind(call_save_file, this));
  cmdui->add_builtin_command("query.saveFileAs", std::bind(call_save_file_as, this));
  cmdui->add_builtin_command("query.revert", std::bind(call_revert, this), std::bind(validate_revert, this));

  cmdui->add_builtin_command("query.export", std::bind(call_export, this), std::bind(validate_export, this));

  cmdui->add_builtin_command("query.cancel",
                             std::bind(&WBContextSQLIDE::call_in_editor, this, &SqlEditorForm::cancel_query));

  cmdui->add_builtin_command("query.reconnect", std::bind(call_reconnect, this));

  cmdui->add_builtin_command("query.continueOnError", std::bind(call_continue_on_error, this));

  cmdui->add_builtin_command("query.jump_to_placeholder", std::bind(&WBContextSQLIDE::call_in_editor_panel, this,
                                                                    &SqlEditorPanel::jump_to_placeholder));
  cmdui->add_builtin_command("list-members",
                             std::bind(&WBContextSQLIDE::call_in_editor_panel, this, &SqlEditorPanel::list_members),
                             std::bind(validate_list_members, this));
}

//----------------------------------------------------------------------------------------------------------------------

void WBContextSQLIDE::finalize() {
  if (_auto_save_handle) {
    mforms::Utilities::cancel_timeout(_auto_save_handle);
    _auto_save_handle = static_cast<mforms::TimeoutHandle>(NULL);
  }
  std::list<SqlEditorForm::Ptr>::iterator next, ed = _open_editors.begin();
  while (ed != _open_editors.end()) {
    next = ed;
    ++next;
    if (!ed->expired()) {
      ed->lock()->close();
    }
    ed = next;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void WBContextSQLIDE::reconnect_editor(SqlEditorForm *editor) {
  std::shared_ptr<wb::SSHTunnel> tunnel;

  if (!editor->connection_descriptor().is_valid()) {
    grtui::DbConnectionDialog dialog(wb::WBContextUI::get()->get_wb()->get_root()->rdbmsMgmt());
    logDebug("No connection associated with editor on reconnect, showing connection selection dialog...\n");
    db_mgmt_ConnectionRef target = dialog.run();
    if (!target.is_valid())
      return;

    editor->set_connection(target);
  }

  // open tunnel, if needed
  try {
    tunnel = sql::DriverManager::getDriverManager()->getTunnel(editor->connection_descriptor());
  } catch (grt::user_cancelled &) {
    bec::GRTManager::get()->replace_status_text("Tunnel connection cancelled.");
    return;
  }
  try {
    if (editor && !editor->is_running_query()) {
      bec::GRTManager::get()->replace_status_text("Reconnecting...");
      if (editor->connect(tunnel))
        bec::GRTManager::get()->replace_status_text("Connection reopened.");
      else {
        bec::GRTManager::get()->replace_status_text("Could not reconnect.");
      }
    }
  } catch (std::exception &exc) {
    SqlEditorForm::report_connection_failure(exc.what(), editor->connection_descriptor());
    return;
  }
}

static void *connect_editor(SqlEditorForm::Ref editor, std::shared_ptr<wb::SSHTunnel> tunnel) {
  try {
    logDebug3("Connecting SQL editor...\n");
    editor->connect(tunnel);
  } catch (sql::AuthenticationError &exc) {
    logError("Got an authentication error during connection: %s\n", exc.what());
    return new std::string(exc.what());
  } catch (grt::server_denied &sd) {
    if (sd.errNo == 3159)
      return new std::string(":SSL_ONLY");
    if (sd.errNo == 3032)
      return new std::string(":OFFLINE_MODE");
  } catch (grt::user_cancelled &) {
    logInfo("User cancelled connection\n");
    return new std::string(":CANCELLED");
  }
  catch (std::exception &exc)
  {
    logError("Got an exception during connection: %s\n", exc.what());
    return new std::string(exc.what());
  }
  logDebug3("Connection to SQL editor succeeded\n");
  return new std::string();
}

static bool cancel_connect_editor(SqlEditorForm::Ref editor) {
  logDebug3("Cancelling connection...\n");
  editor->cancel_connect();
  return true;
}

SqlEditorForm::Ref WBContextSQLIDE::create_connected_editor(const db_mgmt_ConnectionRef &conn) {
  // start by opening the tunnel, if needed
  std::shared_ptr<wb::SSHTunnel> tunnel;

  if (conn.is_valid())
    tunnel = sql::DriverManager::getDriverManager()->getTunnel(conn);

  SqlEditorForm::Ref editor(SqlEditorForm::create(this, conn));

  if (conn.is_valid()) {
    void *result_ptr = 0;
    if (!mforms::Utilities::run_cancelable_task(
          _("Opening SQL Editor"), strfmt(_("An SQL editor instance for '%s' is opening and should be available in a "
                                            "moment.\n\nPlease stand by..."),
                                          conn->name().c_str()),
          std::bind(connect_editor, editor, tunnel), std::bind(cancel_connect_editor, editor), result_ptr))
      throw grt::user_cancelled("canceled");
    if (!result_ptr)
      throw grt::user_cancelled("connection error");

    std::string *result = (std::string *)result_ptr;
    if (result->empty())
      delete result;
    else {
      std::string tmp(*result);
      delete result;

      if (tmp == ":PASSWORD_EXPIRED") {
        grt::BaseListRef args(grt::AnyType);
        args.ginsert(conn);
        ssize_t result =
          *grt::IntegerRef::cast_from(grt::GRT::get()->call_module_function("WbAdmin", "handleExpiredPassword", args));
        if (result != 0)
          return create_connected_editor(conn);
        throw grt::user_cancelled("password reset cancelled by user");
      } else if (tmp == ":CANCELLED") {
        throw grt::user_cancelled("Cancelled");
      } else if (tmp == ":SSL_ONLY") {
        throw grt::server_denied(
          "Connections using insecure transport are prohibited while --require_secure_transport=ON.", 3159);
      } else if (tmp == ":OFFLINE_MODE") {
        throw grt::server_denied("The server is currently in offline mode.", 3032);
      }

      throw std::runtime_error(tmp);
    }
  }

  {
    // Create entry for grt tree and update volatile data in the connection.
    db_query_EditorRef object(grt::Initialized);
    object->owner(wb::WBContextUI::get()->get_wb()->get_root());
    object->name(conn.is_valid() ? conn->name() : "unconnected");

    object->set_data(new db_query_EditorConcreteImplData(editor, object));

    if (conn.is_valid()) {
      std::map<std::string, std::string> details(editor->connection_details());
      grt::DictRef parameter_values = conn->parameterValues();
      parameter_values.gset("serverVersion", details["dbmsProductVersion"]);
      std::time_t time = std::time(0);
      parameter_values.gset("lastConnected", (long)time);

      object->serverVersion(editor->rdbms_version());
    }

    wb::WBContextUI::get()->get_wb()->get_root()->sqlEditors().insert(object);
  }

  _open_editors.push_back(editor);

  editor->finish_startup();

  // setup auto-save for model
  if (!_auto_save_active) {
    _auto_save_active = true;
    ssize_t interval = wb::WBContextUI::get()->get_wb()->get_root()->options()->options().get_int(
      "workbench:AutoSaveSQLEditorInterval", 60);
    if (interval > 0)
      _auto_save_handle =
        mforms::Utilities::add_timeout((float)interval, std::bind(&WBContextSQLIDE::auto_save_workspaces, this));
    _auto_save_interval = interval;

    if (!_option_change_signal_connected) {
      scoped_connect(wb::WBContextUI::get()->get_wb()->get_root()->options()->signal_dict_changed(),
                     std::bind(&WBContextSQLIDE::option_changed, this, std::placeholders::_1, std::placeholders::_2,
                               std::placeholders::_3));
      _option_change_signal_connected = true;
    }
  }

  if (conn.is_valid()) {
    if (::auto_save_sessions.find(conn.id()) != ::auto_save_sessions.end()) {
      ::auto_save_sessions.erase(conn.id());
      wb::WBContextUI::get()->refresh_home_connections();
    }
  }
  return editor;
}

SqlEditorForm *WBContextSQLIDE::get_active_sql_editor() {
  bec::UIForm *form = wb::WBContextUI::get()->get_active_main_form();
  if (form)
    return dynamic_cast<SqlEditorForm *>(form);
  return 0;
}

bool WBContextSQLIDE::activate_live_object(GrtObjectRef object) {
  SqlEditorForm *editor = get_active_sql_editor();
  if (!editor)
    return false;
  return editor->get_live_tree()->activate_live_object(object);
}

//--------------------------------------------------------------------------------------------------

void WBContextSQLIDE::open_document(const std::string &path) {
  SqlEditorForm *editor = get_active_sql_editor();
  if (editor) {
    editor->open_file(path);
  } else {
    std::shared_ptr<SqlEditorForm> editor(wb::WBContextUI::get()->get_wb()->add_new_query_window());
    editor->open_file(path);
  }
}

static bool compare(SqlEditorForm::Ptr ptr, SqlEditorForm *editor) {
  return ptr.lock().get() == editor;
}

void WBContextSQLIDE::editor_will_close(SqlEditorForm *editor) {
  std::list<SqlEditorForm::Ptr>::iterator iter =
    std::find_if(_open_editors.begin(), _open_editors.end(), std::bind(compare, std::placeholders::_1, editor));
  if (iter != _open_editors.end()) {
    // delete entry from grt tree
    grt::ListRef<db_query_Editor> editors(wb::WBContextUI::get()->get_wb()->get_root()->sqlEditors());

    for (size_t c = editors.count(), i = 0; i < c; i++) {
      db_query_EditorRef current_editor(editors[i]);
      if (dynamic_cast<db_query_EditorConcreteImplData *>(current_editor->get_data())->editor_object().get() ==
          editor) {
        current_editor->reset_references();
        dynamic_cast<db_query_EditorConcreteImplData *>(current_editor->get_data())->detach();

        editors.remove(i);
        break;
      }
    }

    _open_editors.erase(iter);

    if (_open_editors.empty())
      _auto_save_active = false;
  }
}

bool WBContextSQLIDE::request_quit() {
  for (std::list<SqlEditorForm::Ptr>::iterator ed = _open_editors.begin(); ed != _open_editors.end(); ++ed) {
    if (!ed->expired() && !ed->lock()->can_close())
      return false;
  }
  return true;
}

void WBContextSQLIDE::update_plugin_arguments_pool(bec::ArgumentPool &args) {
  SqlEditorForm *editor_ptr = get_active_sql_editor();
  if (editor_ptr) {
    db_query_EditorRef editor(get_grt_editor_object(editor_ptr));
    if (editor.is_valid()) {
      db_query_QueryEditorRef qeditor(editor->activeQueryEditor());
      if (qeditor.is_valid()) {
        db_query_ResultPanelRef rpanel(qeditor->activeResultPanel());

        args.add_entries_for_object("activeSQLEditor", editor);
        args.add_entries_for_object("activeQueryBuffer", qeditor);
        args.add_entries_for_object("activeQueryEditor", qeditor);
        args.add_entries_for_object("", qeditor);
        if (rpanel.is_valid() && rpanel->resultset().is_valid())
          args.add_entries_for_object("activeResultset", rpanel->resultset(), "db.query.Resultset");
      } else
        args.add_entries_for_object("activeSQLEditor", editor);
    }
  }
}

db_query_EditorRef WBContextSQLIDE::get_grt_editor_object(SqlEditorForm *editor) {
  if (editor) {
    grt::ListRef<db_query_Editor> list(wb::WBContextUI::get()->get_wb()->get_root()->sqlEditors());
    for (grt::ListRef<db_query_Editor>::const_iterator ed = list.begin(); ed != list.end(); ++ed) {
      if (dynamic_cast<db_query_EditorConcreteImplData *>((*ed)->get_data())->editor_object().get() == editor)
        return *ed;
    }
  }
  return db_query_EditorRef();
}

//--------------------------------------------------------------------------------------------------

static struct RegisterNotifDocs_wb_context_sqlide {
  RegisterNotifDocs_wb_context_sqlide() {
    base::NotificationCenter::get()->register_notification(
      "GRNSQLEditorOpened", "sqlide",
      "Sent when a connection tab finishes initializing and is about to be shown on screen.",
      "db.query.Editor instance", "");

    base::NotificationCenter::get()->register_notification(
      "GRNSQLEditorReconnected", "sqlide",
      "Sent when the connection state of a SQL editor changes (reconnect, disconnect) it's DB connection. Resent "
      "whenever the user clicks Reconnect.",
      "db.query.Editor instance", "connected - whether the connection is open");

    base::NotificationCenter::get()->register_notification(
      "GRNServerStateChanged", "sqlide",
      "Sent by the Admin module when it is detected that the server state (running or stopped) changed.",
      "db.query.Editor instance",
      "state - running or stopped\n"
      "connection - the connection object for the server");

    base::NotificationCenter::get()->register_notification(
      "GRNLiveDBObjectMenuWillShow", "sqlide",
      "Sent when the context menu is about to be shown for a live DB object (eg. from the live schema tree in the SQL "
      "IDE sidebar).",
      "db.query.Editor instance",
      "menu - mforms.ObjectReference of a mforms.Menu object which is being shown\n"
      "selection - a list of db.query.LiveDBObject for the selected objects");

    base::NotificationCenter::get()->register_notification(
      "GRNLiveDBObjectSelectionDidChange", "sqlide",
      "Sent when the selection in the schema tree changes. Avoid hooking slow actions to this as it will make object "
      "selection less smooth.",
      "db.query.Editor instance", "selection-size - number of selected items in the tree");

    base::NotificationCenter::get()->register_notification(
      "GRNSQLResultSetMenuWillShow", "sqlide",
      "Sent when the context menu is about to be shown for a resultset in the SQL IDE.", "db.query.Resultset instance",
      "menu - mforms.ObjectReference of a mforms.Menu object which is being shown\n"
      "selected_rows - a list of int values of the selected rows\n"
      "selected_column - if this value is set, the selection is a single cell at selected_rows[0], selected_column");
  }
} initdocs_wb_context_sqlide;
