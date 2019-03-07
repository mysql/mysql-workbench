/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __DB_SQL_EDITOR_VIEW_H__
#define __DB_SQL_EDITOR_VIEW_H__

#include "mforms/toolbar.h"
#include "sqlide/wb_sql_editor_form.h"
#include "gtk_helpers.h"
#include "form_view_base.h"
#include "notebook_dockingpoint.h"
#include "overview_panel.h"
#include "active_label.h"
#include "sqlide_output_view.h"
#include <glib.h>

class SqlSnippetsView;
class ToolbarManager;
class QueryView;

//==============================================================================
//
//==============================================================================
class DbSqlEditorView : public Gtk::Box, public FormViewBase {
public:
  DbSqlEditorView(SqlEditorForm::Ref editor_be);
  static DbSqlEditorView *create(SqlEditorForm::Ref editor_be);
  virtual ~DbSqlEditorView();

  virtual void init();
  virtual bool on_close();
  virtual void dispose();

  virtual bec::BaseEditor *get_be() {
    return NULL;
  }
  virtual bec::UIForm *get_form() const {
    return _be.get();
  }
  virtual Gtk::Widget *get_panel() {
    return this;
  }
  // virtual void toggle_sidebar();

  virtual bool perform_command(const std::string &command);

  SqlEditorForm::Ref be() {
    return _be;
  }

  void close_appview_tab(mforms::AppView *aview);
  void output_text(const std::string &text, bool bring_to_front);

  virtual bool close_focused_tab();

protected:
  virtual void plugin_tab_added(PluginEditorBase *plugin);

private:
  void polish();
  void set_busy_tab(int);
  void on_exec_sql_done();

  void editor_page_switched(Gtk::Widget *page, guint index);
  void editor_page_reordered(Gtk::Widget *page, guint index);
  void editor_page_added(Gtk::Widget *page, guint index);
  void editor_page_removed(Gtk::Widget *page, guint index);

  mforms::Menu *init_tab_menu(Gtk::Widget *w);
  void tab_menu_handler(const std::string &action, ActiveLabel *sender, Gtk::Widget *widget);
  void reenable_items_in_tab_menus();

  void set_maximized_editor_mode(bool flag, bool hide_schemas = false);

  SqlEditorForm::Ref _be;
  Gtk::Paned _top_pane;
  Gtk::Paned _top_right_pane;
  Gtk::Paned _main_pane;
  QueryOutputView _output;
  Gtk::Widget *_side_palette;

  sigc::connection _polish_conn;
  sigc::connection _sig_restore_sidebar;

  NotebookDockingPoint _dock_delegate;
  mforms::DockingPoint *_dpoint;

  ActiveLabel *_busy_tab;

  const bool _right_aligned;
  bool _editor_maximized;
};

#endif // __DB_SQL_EDITOR_VIEW_H__
