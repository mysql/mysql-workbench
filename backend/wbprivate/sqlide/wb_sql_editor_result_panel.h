/*
 * Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "base/notifications.h"

#include "workbench/wb_backend_public_interface.h"

#include "mforms/appview.h"
#include "mforms/tabview_dock.h"
#include "mforms/tabswitcher.h"

#include "sqlide/recordset_be.h"

#include "grts/structs.db.query.h"

#include <boost/signals2.hpp>

#include "spatial_data_view.h"
#include "wb_sql_editor_panel.h"

namespace mforms {
  class ToolBar;
  class ToolBarItem;
  class ContextMenu;
  class TreeView;
  class GridView;
  class ScrollPanel;
};

class ResultFormView;

class MYSQLWBBACKEND_PUBLIC_FUNC SqlEditorResult : public mforms::AppView, public base::Observer {
  SqlEditorPanel *_owner;
  Recordset::Ptr _rset;

  class DockingDelegate;

public:
  SqlEditorResult(SqlEditorPanel *owner);
  void set_recordset(Recordset::Ref rset);

  virtual ~SqlEditorResult();

  Recordset::Ref recordset() const;

  std::string caption() const;

  db_query_ResultPanelRef grtobj() {
    return _grtobj;
  }
  db_query_ResultsetRef result_grtobj() {
    return _grtobj->resultset();
  }

  virtual bool can_close();
  virtual void close();

  void show_export_recordset();
  void show_import_recordset();
  void dock_result_grid(mforms::View *view);
  //  mforms::View *result_grid() { return _result_grid; }

  SqlEditorPanel *owner() {
    return _owner;
  }

  std::vector<SpatialDataView::SpatialDataSource> get_spatial_columns();

  mforms::GridView *result_grid() {
    return _result_grid;
  }

  mforms::DockingPoint *dock() {
    return &_tabdock;
  }

  void apply_changes();
  void discard_changes();
  bool has_pending_changes();

  virtual void set_title(const std::string &title);

  void set_pinned(bool flag) {
    _pinned = flag;
  }
  bool pinned() const {
    return _pinned;
  }

  void view_record_in_form(int row_id);

  void open_field_editor(int row, int column);

private:
  mforms::TabView _tabview;
  mforms::TabSwitcher _switcher;
  DockingDelegate *_tabdock_delegate;
  mforms::DockingPoint _tabdock;

  mforms::AppView *_column_info_box;
  mforms::AppView *_query_stats_box;
  mforms::ScrollPanel *_query_stats_panel;
  mforms::AppView *_resultset_placeholder;
  mforms::AppView *_execution_plan_placeholder;
  ResultFormView *_form_result_view;
  SpatialDataView *_spatial_result_view;
  mforms::ContextMenu *_column_info_menu;
  mforms::ContextMenu *_grid_header_menu;
  std::list<mforms::ToolBar *> _toolbars;
  mforms::GridView *_result_grid;
  boost::signals2::signal<void(bool)> _collapse_toggled;
  boost::signals2::connection _collapse_toggled_sig;

  db_query_ResultPanelRef _grtobj;

  std::vector<std::string> _column_width_storage_ids;

  bool _column_info_created;
  bool _query_stats_created;
  bool _form_view_created;
  bool _spatial_view_initialized;

  bool _pinned;

  void handle_notification(const std::string &name, void *sender, base::NotificationInfo &info);
  void updateColors();

  void update_selection_for_menu_extra(mforms::ContextMenu *menu, const std::vector<int> &rows, int column);
  void switch_tab();

  void toggle_switcher_collapsed();
  void switcher_collapsed();

  void create_query_stats_panel();
  void create_column_info_panel();
  void create_spatial_view_panel_if_needed();

  void dock_result_grid(mforms::GridView *view);

  void restore_grid_column_widths();
  std::vector<float> get_autofit_column_widths(Recordset *rs);
  void reset_column_widths();

  void add_switch_toggle_toolbar_item(mforms::ToolBar *tbar);

  void copy_column_info_name(mforms::TreeView *tree);
  void copy_column_info(mforms::TreeView *tree);

  void copy_column_name();
  void copy_all_column_names();

  void reset_sorting();
  void on_recordset_column_resized(int column);
  void onRecordsetColumnsResized(const std::vector<int> cols);
};
