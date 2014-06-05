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

#ifndef __MySQLWorkbench__spatial_data_view__
#define __MySQLWorkbench__spatial_data_view__

#include "workbench/wb_backend_public_interface.h"

#include "sqlide/recordset_be.h"

#include "mforms/box.h"
#include "mforms/utilities.h"
#include <deque>

namespace mforms
{
  class ToolBar;
  class ToolBarItem;
  class Selector;
  class TreeNodeView;
  struct TreeNodeRef;
  class Label;
};

class SpatialDrawBox;
class SqlEditorResult;

class SpatialDataView : public mforms::Box
{
public:
  struct SpatialDataSource
  {
    std::string column;
    int column_index;
    std::string type;
  };

private:
  SqlEditorResult *_owner;

  mforms::ToolBar *_toolbar;

  mforms::Box *_main_box;
  mforms::Box *_option_box;

  mforms::ToolBarItem *_projection_picker;
  mforms::TreeNodeView *_layer_tree;

  SpatialDrawBox *_viewer;

  void tree_toggled(const mforms::TreeNodeRef &node, const std::string &value);
  void show_column_data(int column, bool show);

public:
  SpatialDataView(SqlEditorResult *owner);
  virtual ~SpatialDataView();

  mforms::ToolBar *get_toolbar() { return _toolbar; }

  void set_geometry_columns(const std::vector<SpatialDataSource> &columns);

  void activate();
};

#endif /* defined(__MySQLWorkbench__spatial_data_view__) */
