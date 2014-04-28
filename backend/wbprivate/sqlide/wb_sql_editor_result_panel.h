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

#include "mforms/box.h"

#include "sqlide/recordset_be.h"

#include <boost/signals2.hpp>

namespace mforms
{
  class TabView;
  class TabSwitcher;
  class ToolBar;
  class ToolBarItem;
  class ContextMenu;
  class TreeNodeView;
};

class SqlEditorForm;

class ResultFormView;

class MYSQLWBBACKEND_PUBLIC_FUNC SqlEditorResult : public mforms::Box
{
  SqlEditorForm *_owner;
  Recordset::Ptr _rset;

  SqlEditorResult(SqlEditorForm *owner, Recordset::Ref rset);

public:
  typedef boost::shared_ptr<SqlEditorResult> Ref;

  static Ref create(SqlEditorForm *owner, Recordset::Ref rset);
  virtual ~SqlEditorResult();

  Recordset::Ref recordset() const;

  std::string caption() const;

  void show_export_recordset();
  void show_import_recordset();
  
  void dock_result_grid(mforms::View *view);
  mforms::View *result_grid() { return _result_grid; }
private:
  int _column_info_tab;
  int _query_stats_tab;
  int _form_result_tab;
  int _result_grid_tab;
  mforms::TabView *_tabview;
  mforms::TabSwitcher *_switcher;
  mforms::Box *_column_info_box;
  mforms::Box  *_query_stats_box;
  ResultFormView *_form_result_view;
  mforms::ContextMenu *_column_info_menu;
  std::list<mforms::ToolBar*> _toolbars;
  mforms::View *_result_grid;
  boost::signals2::signal<void (bool)> _collapse_toggled;
  boost::signals2::connection _collapse_toggled_sig;
  bool _column_info_created;
  bool _query_stats_created;
  bool _form_view_created;

  void switch_tab();
  
  void toggle_switcher_collapsed();
  void switcher_collapsed();
  
  void create_query_stats_panel();
  void create_column_info_panel();

  void add_switch_toggle_toolbar_item(mforms::ToolBar *tbar);

  void copy_column_info_name(mforms::TreeNodeView *tree);
  void copy_column_info(mforms::TreeNodeView *tree);
};


#endif /* __wb_sql_editor_result_tab__ */
