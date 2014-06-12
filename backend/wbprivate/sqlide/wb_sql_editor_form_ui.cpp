/* 
 * Copyright (c) 2011, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_sql_editor_form.h"
#include "wb_sql_editor_form_ui.h"
#include "query_side_palette.h"
#include "workbench/wb_context_names.h"
#include "objimpl/db.query/db_query_Resultset.h"
#include "objimpl/ui/mforms_ObjectReference_impl.h"

#include "grtsqlparser/sql_facade.h"
#include "grtdb/editor_dbobject.h"
#include "grtdb/db_helpers.h"

#include "sqlide/wb_sql_editor_tree_controller.h"
#include "sqlide/sql_script_run_wizard.h"

#include "workbench/wb_command_ui.h"
#include "workbench/wb_context_ui.h"
#include "base/boost_smart_ptr_helpers.h"

#include <boost/signals2/connection.hpp>

#include "mforms/menubar.h"
#include "mforms/toolbar.h"
#include "mforms/code_editor.h"

using namespace bec;
using namespace grt;
using namespace wb;
using namespace base;

using boost::signals2::scoped_connection;


void SqlEditorForm::activate_command(const std::string &command)
{
  _wbsql->get_cmdui()->activate_command(command);
}


mforms::ToolBar *SqlEditorForm::get_toolbar()
{
  if (!_toolbar)
  {
    _toolbar = _wbsql->get_cmdui()->create_toolbar("data/dbquery_toolbar.xml", boost::bind(&SqlEditorForm::activate_command, this, _1));
    
    update_menu_and_toolbar();
    update_toolbar_icons();
  }
  return _toolbar;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorForm::limit_rows(mforms::MenuItem *menu, const char *limit)
{
  int c = menu->item_count();
  bool found = false;
  for (int i = 0; i < c; i++)
  {
    mforms::MenuItem *item = menu->get_item(i);
    if (item->get_type() != mforms::SeparatorMenuItem)
    {
      if (item->get_name().compare(limit) == 0)
      {
        item->set_checked(true);
        found = true;
      }
      else
        item->set_checked(false);
    }
  }

  int limit_num = atoi(limit);

  _grtm->set_app_option("SqlEditor:LimitRows", grt::IntegerRef(limit_num > 0));
  if (limit_num > 0)
    _grtm->set_app_option("SqlEditor:LimitRowsCount", grt::IntegerRef(limit_num));

  set_editor_tool_items_checked("query.toggleLimit", (limit_num > 0));

  // special handling for custom values not in the predefined list
  mforms::MenuItem *citem = menu->find_item("custom");
  if (!found)
  {
    if (!citem)
      citem = menu->add_item_with_title(base::strfmt("Custom (%s)", limit), boost::bind(&SqlEditorForm::limit_rows, this, menu, limit), "custom");
    else
      citem->set_title(base::strfmt("Custom (%s)", limit));
    citem->set_checked(true);
  }
  else
  {
    if (citem)
      menu->remove_item(citem);
  }
}


mforms::MenuBar *SqlEditorForm::get_menubar()
{
  if (!_menu)
  {
    _menu = _wbsql->get_cmdui()->create_menubar_for_context(WB_CONTEXT_QUERY);

    // special handling for Query -> Row Limit submenu
    int limit_count = _grtm->get_app_option_int("SqlEditor:LimitRows") ? _grtm->get_app_option_int("SqlEditor:LimitRowsCount") : 0;

    mforms::MenuItem *limit_item = _menu->find_item("limit_rows");
    if (limit_item)
    {
      static const char* items[] = {
        "10", "50", "100", "200", "300", "400", "500", "1000", "2000", "5000", "10000", "50000", NULL
      };

      limit_item->add_item_with_title("Don't Limit", boost::bind(&SqlEditorForm::limit_rows, this, limit_item, "0"), "0");
      limit_item->add_separator();
      for (int i = 0; items[i]; i++)
        limit_item->add_item_with_title(items[i], boost::bind(&SqlEditorForm::limit_rows, this, limit_item, items[i]), items[i]);

      if (limit_count <= 0)
        limit_rows(limit_item, "0");
      else
        limit_rows(limit_item, base::strfmt("%i", limit_count).c_str());
    }

    update_menu_and_toolbar();
    
    _menu->set_item_enabled("query.save_edits", false);
    _menu->set_item_enabled("query.discard_edits", false);
    _menu->set_item_enabled("query.export", false);
    
    _menu->set_item_checked("query.stopOnError", !continue_on_error());
  }
  return _menu;
}

void SqlEditorForm::update_menu_and_toolbar()
{
  if (!_grtm->in_main_thread())
  {
    exec_sql_task->execute_in_main_thread(boost::bind(&SqlEditorForm::update_menu_and_toolbar, this),
                                          false,
                                          false);
    return;
  }
  
  bool running = is_running_query();
  bool connected = this->connected();

  if (_menu)
  {
    _menu->set_item_enabled("query.cancel", running && connected);
    _menu->set_item_enabled("query.execute", !running && connected);
    _menu->set_item_enabled("query.reconnect", !running);
    _menu->set_item_enabled("wb.sqlide.executeToTextOutput", !running && connected);
    _menu->set_item_enabled("query.execute_current_statement", !running && connected);
    _menu->set_item_enabled("query.explain", !running && connected);
    _menu->set_item_enabled("query.explain_current_statement", !running && connected);
    _menu->set_item_enabled("query.commit", !running && !auto_commit() && connected);
    _menu->set_item_enabled("query.rollback", !running && !auto_commit() && connected);
    _menu->set_item_enabled("query.stopOnError", connected);
    mforms::MenuItem *item = _menu->find_item("query.autocommit");
    if (item)
    {
      item->set_enabled(!running && connected);
      item->set_checked(auto_commit());    
    }
    item = _menu->find_item("query.gatherPSInfo");
    if (item)
    {
      item->set_enabled(!running && connected && bec::is_supported_mysql_version_at_least(_version, 5, 5));
      item->set_checked(collect_ps_statement_events());
    }
  }

  if (_toolbar)
  {
    _toolbar->set_item_enabled("query.new_schema", connected);
    _toolbar->set_item_enabled("query.show_inspector", connected);
    _toolbar->set_item_enabled("query.new_table", connected);
    _toolbar->set_item_enabled("query.new_view", connected);
    _toolbar->set_item_enabled("query.new_routine", connected);
    _toolbar->set_item_enabled("query.new_function", connected);
    _toolbar->set_item_enabled("wb.dbsearch", connected);
  }
  
  set_editor_tool_items_enbled("query.cancel", running && connected);
  
  set_editor_tool_items_enbled("query.execute", !running && connected);
  set_editor_tool_items_enbled("query.reconnect", !running);
  set_editor_tool_items_enbled("wb.sqlide.executeToTextOutput", !running && connected);
  set_editor_tool_items_enbled("query.execute_current_statement", !running && connected);
  set_editor_tool_items_enbled("query.explain", !running && connected);
  set_editor_tool_items_enbled("query.explain_current_statement", !running && connected);
  
  set_editor_tool_items_enbled("query.commit", !running && !auto_commit() && connected);
  set_editor_tool_items_enbled("query.rollback", !running && !auto_commit() && connected);
  set_editor_tool_items_enbled("query.autocommit", !running && connected);
  set_editor_tool_items_enbled("query.stopOnError", connected);
  set_editor_tool_items_checked("query.autocommit", auto_commit());
  set_editor_tool_items_checked("query.stopOnError", !_continue_on_error);
  set_editor_tool_items_checked("query.toggleLimit",  _grtm->get_app_option_int("SqlEditor:LimitRows") != 0);
}

//--------------------------------------------------------------------------------------------------

std::string find_icon_name(std::string icon_name, bool use_win8)
{
  std::string::size_type dot_position = icon_name.rfind(".");
  if (dot_position != std::string::npos)
  {
    std::string extension = icon_name.substr(dot_position);
    std::string name = icon_name.substr(0, dot_position);
    bool using_win8 = name.rfind("_win8") == name.size() - 5;
    if (use_win8 != using_win8)
    {
      if (use_win8)
        icon_name = name + "_win8" + extension;
      else
        icon_name = name.substr(0, name.size() - 5) + extension;
    }
  }

  return icon_name;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorForm::update_toolbar_icons()
{
  bool use_win8;
  
  switch (base::Color::get_active_scheme())
  {
  case base::ColorSchemeStandardWin8:
  case base::ColorSchemeStandardWin8Alternate:
    use_win8 = true;
    break;

  default:
    use_win8 = false;
  }

  mforms::ToolBarItem *item = _toolbar->find_item("wb.toggleSidebar");
  if (item != NULL)
  {
    item->set_icon(find_icon_name(item->get_icon(), use_win8));
    item->set_alt_icon(find_icon_name(item->get_alt_icon(), use_win8));
  }
    
  item = _toolbar->find_item("wb.toggleOutputArea");
  if (item != NULL)
  {
    item->set_icon(find_icon_name(item->get_icon(), use_win8));
    item->set_alt_icon(find_icon_name(item->get_alt_icon(), use_win8));
  }
    
  item = _toolbar->find_item("wb.toggleSecondarySidebar");
  if (item != NULL)
  {
    item->set_icon(find_icon_name(item->get_icon(), use_win8));
    item->set_alt_icon(find_icon_name(item->get_alt_icon(), use_win8));
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorForm::validate_menubar()
{
  if (get_menubar())
    _wbsql->get_wbui()->get_command_ui()->revalidate_menu_bar(get_menubar());
}

//--------------------------------------------------------------------------------------------------

/**
 * Setup of schema browser/object info/quick help side palette.
 */
void SqlEditorForm::setup_side_palette()
{  
  // Right hand side (quick help, snippets).
  _side_palette = mforms::manage(new QuerySidePalette(shared_from_this()));
  
#ifdef _WIN32
  mforms::Panel* panel;
  panel = mforms::manage(new mforms::Panel(mforms::StyledHeaderPanel));
  panel->set_title(_("SQL Additions"));
  panel->add(_side_palette);
  _side_palette_host = panel;
#else
  _side_palette_host = _side_palette;
#endif

  _side_palette->set_active_tab(_grtm->get_app_option_int("DbSqlEditor:ActiveSidePaletteTab", 0));
}

//--------------------------------------------------------------------------------------------------

void SqlEditorForm::toolbar_command(const std::string& command)
{
  if (command == "query.new_schema")
    _live_tree->tree_create_object(LiveSchemaTree::Schema, "", "");
  else if (command == "query.new_table")
    _live_tree->tree_create_object(LiveSchemaTree::Table, "", "");
  else if (command == "query.new_view")
    _live_tree->tree_create_object(LiveSchemaTree::View, "", "");
  else if (command == "query.new_routine")
    _live_tree->tree_create_object(LiveSchemaTree::Procedure, "", "");
  else if (command == "query.new_function")
    _live_tree->tree_create_object(LiveSchemaTree::Function, "", "");
  else if (command == "query.show_inspector")
  {
    db_query_EditorRef editor(_wbsql->get_grt_editor_object(this));
    if (editor.is_valid())
    {
      grt::BaseListRef args(_grtm->get_grt());
      args.ginsert(editor);

      grt::ListRef<db_query_LiveDBObject> selection = grt::ListRef<db_query_LiveDBObject>::cast_from(get_live_tree()->get_schema_tree()->get_selected_objects());
      grt::BaseListRef selected_items(_grtm->get_grt());
      GRTLIST_FOREACH (db_query_LiveDBObject, selection, iterator)
      {
        std::string type = (*iterator)->type();
        if (type == "db.Schema" || type == "db.Table" || type == "db.Index")
          selected_items.ginsert((*iterator));
      }

      if (selected_items->count() > 0)
      {
        args.ginsert(selected_items);
        grt::Module *module = _grtm->get_grt()->get_module("SQLIDEUtils");
        if (module)
          module->call_function("showInspector", args);
      }
      else if (!active_schema().empty())
      {
        db_query_LiveDBObjectRef obj(_grtm->get_grt());
        obj->schemaName(active_schema());
        obj->type("db.Schema");
        obj->name(active_schema());
        selected_items.ginsert(obj);
        args.ginsert(selected_items);
        grt::Module *module = _grtm->get_grt()->get_module("SQLIDEUtils");
        if (module)
          module->call_function("showInspector", args);
      }
      else
        mforms::Utilities::show_warning(_("Selection empty"),
        _("Select a schema, table or index object in the schema tree to show the inspector."), "Close");
    }
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorForm::show_output_area()
{
  mforms::ToolBarItem *item = _toolbar->find_item("wb.toggleOutputArea");
  if (item && !item->get_checked())
  {
    item->set_checked(true);
    item->callback();
  }
}

//--------------------------------------------------------------------------------------------------


mforms::View* SqlEditorForm::get_sidebar()
{
  return _live_tree->get_sidebar();
}

//--------------------------------------------------------------------------------------------------

mforms::View *SqlEditorForm::get_side_palette()
{
  return _side_palette_host;
}

//--------------------------------------------------------------------------------------------------
// Editor Toolbar

static void toggle_continue_on_error(SqlEditorForm *sql_editor_form)
{
  sql_editor_form->continue_on_error(!sql_editor_form->continue_on_error());
}


static void toggle_limit(mforms::ToolBarItem *item, SqlEditorForm *sql_editor_form)
{
  bool do_limit = item->get_checked();

  sql_editor_form->grt_manager()->set_app_option("SqlEditor:LimitRows", do_limit ? grt::IntegerRef(1) : grt::IntegerRef(0));

  std::string limit = do_limit ? base::strfmt("%li", sql_editor_form->grt_manager()->get_app_option_int("SqlEditor:LimitRowsCount")) : "0";

  mforms::MenuItem *menu = sql_editor_form->get_menubar()->find_item("limit_rows");
  int c = menu->item_count();
  for (int i = 0; i < c; i++)
  {
    mforms::MenuItem *item = menu->get_item(i);
    if (item->get_type() != mforms::SeparatorMenuItem)
    {
      if (item->get_name() == limit)
        item->set_checked(true);
      else
        item->set_checked(false);
    }
  }
}

//--------------------------------------------------------------------------------------------------

boost::shared_ptr<mforms::ToolBar> SqlEditorForm::setup_editor_toolbar(MySQLEditor::Ref editor)
{
  boost::shared_ptr<mforms::ToolBar> tbar(new mforms::ToolBar(mforms::SecondaryToolBar));
#ifdef _WIN32
  tbar->set_size(-1, 27);
#endif
  mforms::ToolBarItem *item;
  
  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("query.openFile");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_open.png"));
  item->set_tooltip(_("Open a script file in this editor"));
  scoped_connect(item->signal_activated(),boost::bind((void (SqlEditorForm::*)(const std::string&,bool))&SqlEditorForm::open_file, this, "", false));
  tbar->add_item(item);
  
  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("query.saveFile");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_save.png"));
  item->set_tooltip(_("Save the script to a file."));
  scoped_connect(item->signal_activated(),boost::bind(&SqlEditorForm::save_file, this));
  tbar->add_item(item);
  
  tbar->add_item(mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem)));
  
  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("query.execute");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_execute.png"));
  item->set_tooltip(_("Execute the selected portion of the script or everything, if there is no selection"));
  scoped_connect(item->signal_activated(),boost::bind(&SqlEditorForm::run_editor_contents, this, false));
  tbar->add_item(item);
  
  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("query.execute_current_statement");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_execute-current.png"));
  item->set_tooltip(_("Execute the statement under the keyboard cursor"));
  scoped_connect(item->signal_activated(),boost::bind(&SqlEditorForm::run_editor_contents, this, true));
  tbar->add_item(item);  
  
  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("query.explain_current_statement");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_explain.png"));
  item->set_tooltip(_("Execute the EXPLAIN command on the statement under the cursor"));
  _wbsql->get_cmdui()->scoped_connect(item->signal_activated(),
                                      boost::bind((void (wb::CommandUI::*)(const std::string&))&wb::CommandUI::activate_command, 
                                                  _wbsql->get_cmdui(), "plugin:wb.sqlide.visual_explain"));
  tbar->add_item(item);
  
  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("query.cancel");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_stop.png"));
  item->set_tooltip(_("Stop the query being executed (the connection to the DB server will not be restarted and any open transactions will remain open)"));
  scoped_connect(item->signal_activated(),boost::bind(&SqlEditorForm::cancel_query, this));
  tbar->add_item(item);  

  tbar->add_item(mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem)));

  item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
  item->set_name("query.stopOnError");
  item->set_alt_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_stop-on-error-on.png"));
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_stop-on-error-off.png"));
  item->set_tooltip(_("Toggle whether execution of SQL script should continue after failed statements"));
  scoped_connect(item->signal_activated(),boost::bind(toggle_continue_on_error, this));
  
  tbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
  item->set_name("query.toggleLimit");
  item->set_alt_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_row-limit-on.png"));
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_row-limit-off.png"));
  item->set_tooltip(_("Toggle limiting of number of rows returned by queries.\nWorkbech will automatically add the LIMIT clause with the configured number of rows to SELECT queries.\nYou can change the limit number in Preferences or in the Query -> Limit Rows menu."));
  scoped_connect(item->signal_activated(),boost::bind(toggle_limit, item, this));
  tbar->add_item(item);
  
  tbar->add_item(mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem)));
  
  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("query.commit");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_commit.png"));
  item->set_tooltip(_("Commit the current transaction.\nNOTE: all query tabs in the same connection share the same transaction. To have independent transactions, you must open a new connection."));
  scoped_connect(item->signal_activated(),boost::bind(&SqlEditorForm::commit, this));
  tbar->add_item(item);  
  
  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("query.rollback");
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_rollback.png"));
  item->set_tooltip(_("Rollback the current transaction.\nNOTE: all query tabs in the same connection share the same transaction. To have independent transactions, you must open a new connection."));
  scoped_connect(item->signal_activated(),boost::bind(&SqlEditorForm::rollback, this));
  tbar->add_item(item);
  
  item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
  item->set_name("query.autocommit");
  item->set_alt_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_autocommit-on.png"));
  item->set_icon(IconManager::get_instance()->get_icon_path("qe_sql-editor-tb-icon_autocommit-off.png"));
  item->set_tooltip(_("Toggle autocommit mode. When enabled, each statement will be committed immediately.\nNOTE: all query tabs in the same connection share the same transaction. To have independent transactions, you must open a new connection."));
  scoped_connect(item->signal_activated(),boost::bind(&SqlEditorForm::toggle_autocommit, this));
  tbar->add_item(item);  
  
  tbar->add_item(mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem)));

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("add_snippet");
  item->set_icon(IconManager::get_instance()->get_icon_path("snippet_add.png"));
  item->set_tooltip(_("Save current statement or selection to the snippet list."));
  scoped_connect(item->signal_activated(),boost::bind(&SqlEditorForm::save_snippet, this));
  tbar->add_item(item);
  tbar->add_item(mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem)));

  // adds generic SQL editor toolbar buttons
  editor->set_base_toolbar(tbar.get());

  return tbar;
}


void SqlEditorForm::set_editor_tool_items_enbled(const std::string &name, bool flag)
{
  for (Sql_editors::iterator editor = _sql_editors.begin(); editor != _sql_editors.end(); ++editor)
    (*editor)->toolbar->set_item_enabled(name, flag);
}


void SqlEditorForm::set_editor_tool_items_checked(const std::string &name, bool flag)
{
  for (Sql_editors::iterator editor = _sql_editors.begin(); editor != _sql_editors.end(); ++editor)
    (*editor)->toolbar->set_item_checked(name, flag);
}

void SqlEditorForm::set_tool_item_checked(const std::string& name, bool flag)
{
  _toolbar->set_item_checked(name, flag);
}

bool SqlEditorForm::run_live_object_alteration_wizard(const std::string &alter_script, bec::DBObjectEditorBE* obj_editor, RowId log_id,
                                                      const std::string &log_context)
{
  on_sql_script_run_error.disconnect_all_slots();
  on_sql_script_run_progress.disconnect_all_slots();
  on_sql_script_run_statistics.disconnect_all_slots();
  
  // Determine the current online DDL settings, so the wizard can initialize its local settings.
  std::string algorithm;
  wbsql()->get_wbui()->get_wb_options_value("", "DbSqlEditor:OnlineDDLAlgorithm", algorithm);
  std::string lock;
  wbsql()->get_wbui()->get_wb_options_value("", "DbSqlEditor:OnlineDDLLock", lock);
  
  SqlScriptRunWizard wizard(_grtm, rdbms_version(), algorithm, lock);
  if (obj_editor)
    wizard.regenerate_script = boost::bind(&SqlEditorTreeController::generate_alter_script, get_live_tree(), rdbms(),
                                           obj_editor->get_dbobject(), _1, _2);
  scoped_connection c1(on_sql_script_run_error.connect(boost::bind(&SqlScriptApplyPage::on_error, wizard.apply_page, _1, _2, _3)));
  scoped_connection c2(on_sql_script_run_progress.connect(boost::bind(&SqlScriptApplyPage::on_exec_progress, wizard.apply_page, _1)));
  scoped_connection c3(on_sql_script_run_statistics.connect(boost::bind(&SqlScriptApplyPage::on_exec_stat, wizard.apply_page, _1, _2)));
  
  std::string errors;
  
  scoped_connection c4(on_sql_script_run_error.connect(boost::bind(&SqlEditorForm::sql_script_apply_error, this, _1, _2, _3, boost::ref(errors))));
  scoped_connection c5(on_sql_script_run_progress.connect(boost::bind(&SqlEditorForm::sql_script_apply_progress, this, _1)));
  scoped_connection c6(on_sql_script_run_statistics.connect(boost::bind(&SqlEditorForm::sql_script_stats, this, _1, _2)));
  
  wizard.values().gset("sql_script", alter_script);
  wizard.apply_page->apply_sql_script= boost::bind(&SqlEditorForm::apply_object_alter_script, this, _1, obj_editor, log_id);
  wizard.run_modal();
  
  if (wizard.applied() && !wizard.has_errors())
    set_log_message(log_id, DbSqlEditorLog::OKMsg, _("Changes applied"), log_context, "");
  else
    set_log_message(log_id, DbSqlEditorLog::ErrorMsg, errors, log_context, "");
  
  return wizard.applied() && !wizard.has_errors();
}


int SqlEditorForm::sql_script_apply_error(long long code, const std::string& msg, const std::string& stmt, std::string &errors)
{
  if (code >= 0)
    errors.append(strfmt("Error %li: ", (long)code));
  errors.append(msg).append("\n");
  if (!stmt.empty())
    errors.append("SQL Statement:\n").append(stmt).append("\n\n");
  return 0;
}


int SqlEditorForm::sql_script_apply_progress(float)
{
  return 0;
}


int SqlEditorForm::sql_script_stats(long, long)
{
  return 0;
}


void SqlEditorForm::on_recordset_context_menu_show(Recordset::Ptr rs_ptr, MySQLEditor::Ptr editor_ptr)
{
  Recordset::Ref rs(rs_ptr.lock());
  if (rs)
  {
    grt::DictRef info(_grtm->get_grt());

    std::vector<int> selection(rs->selected_rows());
    grt::IntegerListRef rows(_grtm->get_grt());
    for (std::vector<int>::const_iterator i = selection.begin(); i != selection.end(); ++i)
      rows.insert(*i);

    info.set("selected-rows", rows);
    info.gset("selected-column", rs->selected_column());
    info.set("menu", mforms_to_grt(info.get_grt(), rs->get_context_menu()));

    db_query_QueryBufferRef qbuffer(editor_ptr.lock()->grtobj());
    if (qbuffer.is_valid() && db_query_QueryEditorRef::can_wrap(qbuffer))
    {
      db_query_QueryEditorRef qeditor(db_query_QueryEditorRef::cast_from(qbuffer));
      for (size_t c = qeditor->resultsets().count(), i = 0; i < c; i++)
      {
        if (dynamic_cast<WBRecordsetResultset*>(qeditor->resultsets()[i]->get_data())->recordset == rs)
        {
          grt::GRTNotificationCenter::get()->send_grt("GRNSQLResultsetMenuWillShow", qeditor->resultsets()[i], info);
          break;
        }
      }
    }
  }
}

