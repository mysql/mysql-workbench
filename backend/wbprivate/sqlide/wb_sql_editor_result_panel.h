/*
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __wb_sql_editor_result_tab__
#define __wb_sql_editor_result_tab__

#include "workbench/wb_backend_public_interface.h"

#include "mforms/appview.h"
#include "mforms/tabview_dock.h"
#include "mforms/tabswitcher.h"

#include "sqlide/recordset_be.h"

#include "grts/structs.db.query.h"

#include <boost/signals2.hpp>

namespace mforms
{
  class ToolBar;
  class ToolBarItem;
  class ContextMenu;
  class TreeNodeView;
  class RecordGrid;
};

class SqlEditorPanel;

class ResultFormView;

class MYSQLWBBACKEND_PUBLIC_FUNC SqlEditorResult : public mforms::AppView
{
  SqlEditorPanel *_owner;
  Recordset::Ptr _rset;

  class DockingDelegate;
  
public:
  SqlEditorResult(SqlEditorPanel *owner);
  void set_recordset(Recordset::Ref rset);

  virtual ~SqlEditorResult();

  Recordset::Ref recordset() const;

  std::string caption() const;

  db_query_ResultPanelRef grtobj() { return _grtobj; }
  db_query_ResultsetRef result_grtobj() { return _result_grtobj; }

  virtual bool can_close();
  virtual void close();

  void show_export_recordset();
  void show_import_recordset();

  mforms::RecordGrid *result_grid() { return _result_grid; }

  mforms::DockingPoint *dock() { return &_tabdock; }

  void apply_changes();
  void discard_changes();
  bool has_pending_changes();

  virtual void set_title(const std::string &title);

  void set_pinned(bool flag) { _pinned = flag; }
  bool pinned() const { return _pinned; }

private:
  mforms::TabView _tabview;
  mforms::TabSwitcher _switcher;
  DockingDelegate *_tabdock_delegate;
  mforms::DockingPoint _tabdock;

  mforms::AppView *_column_info_box;
  mforms::AppView  *_query_stats_box;
  mforms::AppView  *_resultset_placeholder;
  mforms::AppView  *_execution_plan_placeholder;
  ResultFormView *_form_result_view;
  mforms::ContextMenu *_column_info_menu;
  std::list<mforms::ToolBar*> _toolbars;
  mforms::RecordGrid *_result_grid;
  boost::signals2::signal<void (bool)> _collapse_toggled;
  boost::signals2::connection _collapse_toggled_sig;

  db_query_ResultPanelRef _grtobj;
  db_query_ResultsetRef _result_grtobj;

  std::vector<std::string> _column_width_storage_ids;

  bool _column_info_created;
  bool _query_stats_created;
  bool _form_view_created;

  bool _pinned;

  void switch_tab();
  
  void toggle_switcher_collapsed();
  void switcher_collapsed();
  
  void create_query_stats_panel();
  void create_column_info_panel();

  void dock_result_grid(mforms::RecordGrid *view);

  void restore_grid_column_widths();
  
  void add_switch_toggle_toolbar_item(mforms::ToolBar *tbar);

  void copy_column_info_name(mforms::TreeNodeView *tree);
  void copy_column_info(mforms::TreeNodeView *tree);

  void on_recordset_column_resized(int column);
};


#endif /* __wb_sql_editor_result_tab__ */
