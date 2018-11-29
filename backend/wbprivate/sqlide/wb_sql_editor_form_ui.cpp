/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/log.h"

#include "mforms/menubar.h"
#include "mforms/toolbar.h"
#include "mforms/code_editor.h"

using namespace bec;
using namespace grt;
using namespace wb;
using namespace base;

DEFAULT_LOG_DOMAIN("SQL Editor Form")

using boost::signals2::scoped_connection;

void SqlEditorForm::activate_command(const std::string &command) {
  _wbsql->get_cmdui()->activate_command(command);
}

mforms::ToolBar *SqlEditorForm::get_toolbar() {
  if (!_toolbar) {
    _toolbar = _wbsql->get_cmdui()->create_toolbar(
      "data/dbquery_toolbar.xml", std::bind(&SqlEditorForm::activate_command, this, std::placeholders::_1));
    _toolbar->set_name("SQL IDE Toolbar");

    update_menu_and_toolbar();
    update_toolbar_icons();
  }
  return _toolbar;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorForm::limit_rows(const std::string &limit_text) {
  int limit;
  if (sscanf(limit_text.c_str(), _("Limit to %i rows"), &limit) < 1)
    limit = 0; // Don't Limit

  mforms::MenuItem *menu = _menu->find_item("limit_rows");
  int c = menu->item_count();
  bool found = false;
  for (int i = 0; i < c; i++) {
    mforms::MenuItem *item = menu->get_item(i);
    if (item->get_type() != mforms::SeparatorMenuItem) {
      if (item->getInternalName().compare(limit_text) == 0) {
        item->set_checked(true);
        found = true;
      } else
        item->set_checked(false);
    }
  }

  bec::GRTManager::get()->set_app_option("SqlEditor:LimitRows", grt::IntegerRef(limit > 0));
  if (limit > 0)
    bec::GRTManager::get()->set_app_option("SqlEditor:LimitRowsCount", grt::IntegerRef(limit));

  // special handling for custom values not in the predefined list
  mforms::MenuItem *citem = menu->find_item("custom");
  if (!found) {
    std::string caption = base::strfmt("Limit to %i rows", limit);
    std::string accessibilityName = base::strfmt("Limit to %i Rows", limit);
    if (!citem)
      citem = menu->add_item_with_title(caption, std::bind(&SqlEditorForm::limit_rows, this, caption), accessibilityName, caption);
    else
      citem->set_title(caption);
    citem->set_checked(true);
  } else {
    if (citem)
      menu->remove_item(citem);
  }

  // update the editors
  for (int i = 0; i < sql_editor_count(); i++) {
    SqlEditorPanel *panel = sql_editor_panel(i);
    if (panel)
      panel->update_limit_rows();
  }
}

static int limit_counts[] = {10, 50, 100, 200, 300, 400, 500, 1000, 2000, 5000, 10000, 50000, 0};

mforms::MenuBar *SqlEditorForm::get_menubar() {
  if (!_menu) {
    _menu = _wbsql->get_cmdui()->create_menubar_for_context(WB_CONTEXT_QUERY);

    // special handling for Query -> Row Limit submenu
    int limit_count = int(bec::GRTManager::get()->get_app_option_int("SqlEditor:LimitRows")
                            ? bec::GRTManager::get()->get_app_option_int("SqlEditor:LimitRowsCount")
                            : 0);

    mforms::MenuItem *limit_item = _menu->find_item("limit_rows");
    if (limit_item) {
      std::string dont_limit = _("Don't Limit");
      std::string active_limit = base::strfmt(_("Limit to %i rows"), limit_count);

      limit_item->add_check_item_with_title(dont_limit, std::bind(&SqlEditorForm::limit_rows, this, dont_limit),
                                            "Do Not Limit", dont_limit);
      limit_item->add_separator();
      for (int i = 0; limit_counts[i] != 0; i++) {
          std::string caption = base::strfmt(_("Limit to %i rows"), limit_counts[i]);
          std::string accessibilityName = base::strfmt(_("Limit to %i Rows"), limit_counts[i]);
        if (limit_counts[i] == limit_count)
          active_limit = caption;
        limit_item->add_check_item_with_title(caption, std::bind(&SqlEditorForm::limit_rows, this, caption), accessibilityName, caption);
      }
      if (limit_count <= 0)
        limit_rows(dont_limit);
      else
        limit_rows(active_limit);
    }

    auto item = _menu->find_item("query.cancel");
    if (item != nullptr)
      item->add_validator([this]() { return is_running_query() && connected(); });
    item = _menu->find_item("query.execute");
    if (item != nullptr)
      item->add_validator([this]() {
        return !is_running_query() && connected() &&
               (active_sql_editor_panel() ? active_sql_editor_panel()->getInternalName() == "db.query.QueryBuffer" : false);
      });
    item = _menu->find_item("query.reconnect");
    if (item != nullptr)
      item->add_validator([this]() { return !is_running_query(); });
    item = _menu->find_item("wb.sqlide.executeToTextOutput");
    if (item != nullptr)
      item->add_validator([this]() { return !is_running_query() && connected(); });
    item = _menu->find_item("wb.sqlide.verticalOutput");
    if (item != nullptr)
      item->add_validator([this]() { return !is_running_query() && connected(); });
    item = _menu->find_item("query.execute_current_statement");
    if (item != nullptr)
      item->add_validator([this]() {
        return !is_running_query() && connected() &&
               (active_sql_editor_panel() ? active_sql_editor_panel()->getInternalName() == "db.query.QueryBuffer" : false);
      });
    item = _menu->find_item("query.explain_current_statement");
    if (item != nullptr)
      item->add_validator([this]() {
        return !is_running_query() && connected() &&
               (active_sql_editor_panel() ? active_sql_editor_panel()->getInternalName() == "db.query.QueryBuffer" : false);
      });
    item = _menu->find_item("query.commit");
    if (item != nullptr)
      item->add_validator([this]() { return !is_running_query() && connected() && !auto_commit(); });
    item = _menu->find_item("query.rollback");
    if (item != nullptr)
      item->add_validator([this]() { return !is_running_query() && connected() && !auto_commit(); });
    item = _menu->find_item("query.continueOnError");
    if (item != nullptr)
      item->add_validator([this]() { return !is_running_query(); });
    item = _menu->find_item("query.autocommit");
    if (item != nullptr)
      item->add_validator([this]() { return !is_running_query() && connected(); });
    item = _menu->find_item("query.gatherPSInfo");
    if (item != nullptr)
      item->add_validator([this]() {
        return !is_running_query() && connected() && bec::is_supported_mysql_version_at_least(_version, 5, 5);
      });
    item = _menu->find_item("run_script");
    if (item != nullptr)
      item->add_validator([this]() { return connected(); });

    update_menu_and_toolbar();

    _menu->set_item_enabled("query.save_edits", false);
    _menu->set_item_enabled("query.discard_edits", false);
    _menu->set_item_enabled("query.export", false);

    _menu->set_item_checked("query.continueOnError", continue_on_error());
  }
  return _menu;
}

void SqlEditorForm::update_menu_and_toolbar() {
  if (!bec::GRTManager::get()->in_main_thread()) {
    exec_sql_task->execute_in_main_thread(std::bind(&SqlEditorForm::update_menu_and_toolbar, this), false, false);
    return;
  }

  logDebug2("Updating SQL menu and toolbar\n");

  bool running = is_running_query();
  bool connected = this->connected();

  if (_menu) {
    _menu->validate();

    mforms::MenuItem *item = _menu->find_item("query.autocommit");
    if (item)
      item->set_checked(auto_commit());

    item = _menu->find_item("query.gatherPSInfo");
    if (item)
      item->set_checked(collect_ps_statement_events());
  }

  if (_toolbar) {
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
  set_editor_tool_items_enbled("query.explain_current_statement", !running && connected);

  set_editor_tool_items_enbled("query.commit", !running && !auto_commit() && connected);
  set_editor_tool_items_enbled("query.rollback", !running && !auto_commit() && connected);
  set_editor_tool_items_enbled("query.autocommit", !running && connected);
  set_editor_tool_items_enbled("query.continueOnError", connected);
  set_editor_tool_items_checked("query.autocommit", auto_commit());
  set_editor_tool_items_checked("query.continueOnError", _continueOnError);
  set_editor_tool_items_checked("query.toggleLimit",
                                bec::GRTManager::get()->get_app_option_int("SqlEditor:LimitRows") != 0);
}

//--------------------------------------------------------------------------------------------------

std::string find_icon_name(std::string icon_name, bool use_win8) {
  std::string::size_type dot_position = icon_name.rfind(".");
  if (dot_position != std::string::npos) {
    std::string extension = icon_name.substr(dot_position);
    std::string name = icon_name.substr(0, dot_position);
    bool using_win8 = name.rfind("_win8") == name.size() - 5;
    if (use_win8 != using_win8) {
      if (use_win8)
        icon_name = name + "_win8" + extension;
      else
        icon_name = name.substr(0, name.size() - 5) + extension;
    }
  }

  return icon_name;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorForm::update_toolbar_icons() {
  bool use_win8;

  switch (base::Color::get_active_scheme()) {
    case base::ColorSchemeStandardWin8:
    case base::ColorSchemeStandardWin8Alternate:
      use_win8 = true;
      break;

    default:
      use_win8 = false;
  }

  mforms::ToolBarItem *item = _toolbar->find_item("wb.toggleSidebar");
  if (item != NULL) {
    item->set_icon(find_icon_name(item->get_icon(), use_win8));
    item->set_alt_icon(find_icon_name(item->get_alt_icon(), use_win8));
  }

  item = _toolbar->find_item("wb.toggleOutputArea");
  if (item != NULL) {
    item->set_icon(find_icon_name(item->get_icon(), use_win8));
    item->set_alt_icon(find_icon_name(item->get_alt_icon(), use_win8));
  }

  item = _toolbar->find_item("wb.toggleSecondarySidebar");
  if (item != NULL) {
    item->set_icon(find_icon_name(item->get_icon(), use_win8));
    item->set_alt_icon(find_icon_name(item->get_alt_icon(), use_win8));
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorForm::validate_menubar() {
  if (get_menubar())
    wb::WBContextUI::get()->get_command_ui()->revalidate_menu_bar(get_menubar());
}

//--------------------------------------------------------------------------------------------------

/**
 * Setup of schema browser/object info/quick help side palette.
 */
void SqlEditorForm::setup_side_palette() {
  // Right hand side (quick help, snippets).
  _side_palette = mforms::manage(new QuerySidePalette(shared_from_this()));

#ifdef _MSC_VER
  mforms::Panel *panel;
  panel = mforms::manage(new mforms::Panel(mforms::StyledHeaderPanel));
  panel->set_title(_("SQL Additions"));
  panel->add(_side_palette);
  _side_palette_host = panel;
#else
  _side_palette_host = _side_palette;
#endif

  _side_palette->set_active_tab((int)bec::GRTManager::get()->get_app_option_int("DbSqlEditor:ActiveSidePaletteTab", 0));
}

//--------------------------------------------------------------------------------------------------

void SqlEditorForm::toolbar_command(const std::string &command) {
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
  else if (command == "query.show_inspector") {
    db_query_EditorRef editor(_wbsql->get_grt_editor_object(this));
    if (editor.is_valid()) {
      grt::BaseListRef args(true);
      args.ginsert(editor);

      grt::ListRef<db_query_LiveDBObject> selection =
        grt::ListRef<db_query_LiveDBObject>::cast_from(get_live_tree()->get_schema_tree()->get_selected_objects());
      grt::BaseListRef selected_items(true);
      GRTLIST_FOREACH(db_query_LiveDBObject, selection, iterator) {
        std::string type = (*iterator)->type();
        if (type == "db.Schema" || type == "db.Table" || type == "db.Index")
          selected_items.ginsert((*iterator));
      }

      if (selected_items->count() > 0) {
        args.ginsert(selected_items);
        grt::Module *module = grt::GRT::get()->get_module("SQLIDEUtils");
        if (module)
          module->call_function("showInspector", args);
      } else if (!active_schema().empty()) {
        db_query_LiveDBObjectRef obj(grt::Initialized);
        obj->schemaName(active_schema());
        obj->type("db.Schema");
        obj->name(active_schema());
        selected_items.ginsert(obj);
        args.ginsert(selected_items);
        grt::Module *module = grt::GRT::get()->get_module("SQLIDEUtils");
        if (module)
          module->call_function("showInspector", args);
      } else
        mforms::Utilities::show_warning(
          _("Selection empty"), _("Select a schema, table or index object in the schema tree to show the inspector."),
          "Close");
    }
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorForm::inspect_object(const std::string &schema, const std::string &object, const std::string &type) {
  db_query_EditorRef editor(_wbsql->get_grt_editor_object(this));
  if (editor.is_valid()) {
    grt::BaseListRef selected_items(true);
    grt::BaseListRef args(true);
    args.ginsert(editor);

    db_query_LiveDBObjectRef obj(grt::Initialized);
    obj->type(type);
    obj->schemaName(schema);
    obj->name(object);
    selected_items.ginsert(obj);

    args.ginsert(selected_items);
    grt::GRT::get()->call_module_function("SQLIDEUtils", "showInspector", args);
  }
}

//--------------------------------------------------------------------------------------------------

void SqlEditorForm::show_output_area() {
  mforms::ToolBarItem *item = _toolbar->find_item("wb.toggleOutputArea");
  if (item && !item->get_checked()) {
    item->set_checked(true);
    item->callback();
  }
}

//--------------------------------------------------------------------------------------------------

mforms::View *SqlEditorForm::get_sidebar() {
  return _live_tree->get_sidebar();
}

//--------------------------------------------------------------------------------------------------

mforms::View *SqlEditorForm::get_side_palette() {
  return _side_palette_host;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorForm::set_editor_tool_items_enbled(const std::string &name, bool flag) {
  if (_tabdock) {
    for (int c = _tabdock->view_count(), i = 0; i < c; i++) {
      SqlEditorPanel *panel = dynamic_cast<SqlEditorPanel *>(_tabdock->view_at_index(i));
      if (panel)
        panel->get_toolbar()->set_item_enabled(name, flag);
    }
  }
}

void SqlEditorForm::set_editor_tool_items_checked(const std::string &name, bool flag) {
  if (_tabdock) {
    for (int c = _tabdock->view_count(), i = 0; i < c; i++) {
      SqlEditorPanel *panel = dynamic_cast<SqlEditorPanel *>(_tabdock->view_at_index(i));
      if (panel)
        panel->get_toolbar()->set_item_checked(name, flag);
    }
  }
}

void SqlEditorForm::set_tool_item_checked(const std::string &name, bool flag) {
  _toolbar->set_item_checked(name, flag);
}

bool SqlEditorForm::run_live_object_alteration_wizard(const std::string &alter_script,
                                                      bec::DBObjectEditorBE *obj_editor, RowId log_id,
                                                      const std::string &log_context) {
  on_sql_script_run_error.disconnect_all_slots();
  on_sql_script_run_progress.disconnect_all_slots();
  on_sql_script_run_statistics.disconnect_all_slots();

  // Determine the current online DDL settings, so the wizard can initialize its local settings.
  std::string algorithm;
  wb::WBContextUI::get()->get_wb_options_value("", "DbSqlEditor:OnlineDDLAlgorithm", algorithm);
  std::string lock;
  wb::WBContextUI::get()->get_wb_options_value("", "DbSqlEditor:OnlineDDLLock", lock);

  SqlScriptRunWizard wizard(rdbms_version(), algorithm, lock);
  if (obj_editor)
    wizard.regenerate_script = std::bind(&SqlEditorTreeController::generate_alter_script, get_live_tree(), rdbms(),
                                         obj_editor->get_dbobject(), std::placeholders::_1, std::placeholders::_2);
  scoped_connection c1(
    on_sql_script_run_error.connect(std::bind(&SqlScriptApplyPage::on_error, wizard.apply_page, std::placeholders::_1,
                                              std::placeholders::_2, std::placeholders::_3)));
  scoped_connection c2(on_sql_script_run_progress.connect(
    std::bind(&SqlScriptApplyPage::on_exec_progress, wizard.apply_page, std::placeholders::_1)));
  scoped_connection c3(on_sql_script_run_statistics.connect(
    std::bind(&SqlScriptApplyPage::on_exec_stat, wizard.apply_page, std::placeholders::_1, std::placeholders::_2)));

  std::string errors;

  scoped_connection c4(
    on_sql_script_run_error.connect(std::bind(&SqlEditorForm::sql_script_apply_error, this, std::placeholders::_1,
                                              std::placeholders::_2, std::placeholders::_3, std::ref(errors))));
  scoped_connection c5(on_sql_script_run_progress.connect(
    std::bind(&SqlEditorForm::sql_script_apply_progress, this, std::placeholders::_1)));
  scoped_connection c6(on_sql_script_run_statistics.connect(
    std::bind(&SqlEditorForm::sql_script_stats, this, std::placeholders::_1, std::placeholders::_2)));

  wizard.values().gset("sql_script", alter_script);
  wizard.apply_page->apply_sql_script =
    std::bind(&SqlEditorForm::apply_object_alter_script, this, std::placeholders::_1, obj_editor, log_id);
  wizard.abort_apply = std::bind(&SqlEditorForm::abort_apply_object_alter_script, this);
  wizard.run_modal();

  if (wizard.applied() && !wizard.has_errors())
    set_log_message(log_id, DbSqlEditorLog::OKMsg, _("Changes applied"), log_context, "");
  else
    set_log_message(log_id, DbSqlEditorLog::ErrorMsg, errors, log_context, "");

  return wizard.applied() && !wizard.has_errors();
}

void SqlEditorForm::abort_apply_object_alter_script() {
  cancel_query();
}

int SqlEditorForm::sql_script_apply_error(long long code, const std::string &msg, const std::string &stmt,
                                          std::string &errors) {
  if (code >= 0)
    errors.append(strfmt("Error %li: ", (long)code));
  errors.append(msg).append("\n");
  if (!stmt.empty())
    errors.append("SQL Statement:\n").append(stmt).append("\n\n");
  return 0;
}

int SqlEditorForm::sql_script_apply_progress(float) {
  return 0;
}

int SqlEditorForm::sql_script_stats(long, long) {
  return 0;
}

void SqlEditorForm::handle_tab_menu_action(const std::string &action, int tab_index) {
  if (action == "new_tab")
    new_sql_script_file();
  else if (action == "save_tab") {
    SqlEditorPanel *editor = sql_editor_panel(tab_index);
    if (editor)
      editor->save();
  } else if (action == "copy_path") {
    SqlEditorPanel *editor = sql_editor_panel(tab_index);
    if (editor)
      mforms::Utilities::set_clipboard_text(editor->filename());
  } else if (action == "close_tab") {
    if (_tabdock->view_at_index(tab_index)->on_close())
      bec::GRTManager::get()->run_once_when_idle(
        this, std::bind(&mforms::DockingPoint::close_view_at_index, _tabdock, tab_index));
  } else if (action == "close_other_tabs") {
    for (int i = _tabdock->view_count() - 1; i >= 0; --i) {
      if (i != tab_index) {
        auto view = _tabdock->view_at_index(i);
        if (view != nullptr)
          view->close();
      }
    }
  }
}

void SqlEditorForm::handle_history_action(const std::string &action, const std::string &sql) {
  if (action == "copy")
    mforms::Utilities::set_clipboard_text(sql);
  else if (action == "append") {
    SqlEditorPanel *panel = active_sql_editor_panel();
    if (panel)
      panel->editor_be()->append_text(sql);
  } else if (action == "replace") {
    SqlEditorPanel *panel = active_sql_editor_panel();
    if (panel)
      panel->editor_be()->sql(sql.c_str());
  } else
    throw std::invalid_argument("invalid history action " + action);
}
