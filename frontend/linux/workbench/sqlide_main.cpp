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

#include "sqlide_form.h"
#include "workbench/wb_context_ui.h"
#include "workbench/wb_context.h"
#include "plugin_editor_base.h"
#include "sqlide_main.h"

class ToolbarManager;

static void close_plugin(PluginEditorBase *editor, wb::WBContext *wb) {
  wb->close_gui_plugin(dynamic_cast<GUIPluginBase *>(editor));
}

static FormViewBase *create_db_sql_editor_view(std::shared_ptr<bec::UIForm> form, wb::WBContext *wb) {
  SqlEditorForm::Ref editor_be = SqlEditorForm::Ref(std::dynamic_pointer_cast<SqlEditorForm>(form));

  DbSqlEditorView *view = Gtk::manage(DbSqlEditorView::create(editor_be));

  view->set_close_editor_callback(sigc::bind(sigc::ptr_fun(close_plugin), wb));

  view->init();
  return view;
}

void setup_sqlide(std::string &name, sigc::slot<FormViewBase *, std::shared_ptr<bec::UIForm> > &create_function) {
  name = WB_MAIN_VIEW_DB_QUERY;

  create_function = sigc::bind(sigc::ptr_fun(create_db_sql_editor_view), wb::WBContextUI::get()->get_wb());
}
