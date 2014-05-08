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
#include "stdafx.h"

#include "wb_sql_editor_form.h"
#include "wb_sql_editor_form_ui.h"
#include "wb_sql_editor_panel.h"
#include "query_side_palette.h"
#include "workbench/wb_context_names.h"
#include "objimpl/db.query/db_query_Resultset.h"
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
    item = _menu->find_item("query.gatherFieldInfo");
    if (item)
    {
      item->set_enabled(!running && connected);
      item->set_checked(collect_field_info());
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


void SqlEditorForm::set_editor_tool_items_enbled(const std::string &name, bool flag)
{
  if (_tabdock)
  {
    for (int c = _tabdock->view_count(), i = 0; i < c; i++)
    {
      SqlEditorPanel* panel = dynamic_cast<SqlEditorPanel*>(_tabdock->view_at_index(i));
      if (panel)
        panel->get_toolbar()->set_item_enabled(name, flag);
    }
  }
}


void SqlEditorForm::set_editor_tool_items_checked(const std::string &name, bool flag)
{
  if (_tabdock)
  {
    for (int c = _tabdock->view_count(), i = 0; i < c; i++)
    {
      SqlEditorPanel* panel = dynamic_cast<SqlEditorPanel*>(_tabdock->view_at_index(i));
      if (panel)
        panel->get_toolbar()->set_item_checked(name, flag);
    }
  }
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
    wizard.regenerate_script = boost::bind(&SqlEditorTreeController::generate_alter_script, get_live_tree(), obj_editor->get_rdbms(),
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

