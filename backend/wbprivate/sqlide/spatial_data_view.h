/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "workbench/wb_backend_public_interface.h"

#include "sqlide/recordset_be.h"

#include "mforms/box.h"
#include "mforms/utilities.h"
#include "mforms/splitter.h"
#include "mforms/treeview.h"

#include <deque>

namespace mforms {
  class ToolBar;
  class ToolBarItem;
  class Selector;
  class TreeView;
  struct TreeNodeRef;
  class Label;
  class ContextMenu;
  class MenuItem;
  class TextBox;
};

class SpatialDrawBox;
class SqlEditorResult;
class SqlEditorForm;
class ProgressPanel;

typedef int
  LayerId; // must be the same as spatial::LayerId, which we can't import b/c gdal creates some weird dependencies

class SpatialDataView : public mforms::Box {
public:
  struct SpatialDataSource {
    std::string source;
    Recordset::Ptr resultset;
    std::string column;
    int column_index;
    std::string type;
  };

private:
  SqlEditorResult *_owner;
  bool _activated;
  mforms::ToolBar *_toolbar;

  mforms::Box *_main_box;
  mforms::Box *_option_box;
  mforms::Splitter *_splitter;

  mforms::ToolBarItem *_projection_picker;
  mforms::TreeView *_layer_tree;
  mforms::ContextMenu *_layer_menu;
  mforms::ContextMenu *_map_menu;

  mforms::TextBox *_info_box;

  LayerId _active_layer;
  LayerId _grid_layer;

  mforms::Label *_mouse_pos_label;

  SpatialDrawBox *_viewer;

  mforms::TimeoutHandle _spliter_change_timeout;
  bool _rendering;

  void call_refresh_viewer();

  bool refresh_viewer();

  void tree_toggled(const mforms::TreeNodeRef &node, const std::string &value);

  void set_color_icon(mforms::TreeNodeRef node, int column, const base::Color &color);

  void work_started(mforms::View *progress, bool reprojecting);
  void work_finished(mforms::View *progress);

  void update_coordinates(base::Point p);
  void handle_click(base::Point p);

  void jump_to();
  void export_image();
  void auto_zoom(LayerId layer);
  void copy_coordinates();

  void change_tool(mforms::ToolBarItem *item);

  std::vector<std::string> layer_overlay_handler(mforms::TreeNodeRef node);

  // layer currently selected in the treeview
  LayerId get_selected_layer_id();
  // layer that's currently set as the active one (bolded in treeview)
  class RecordsetLayer *active_layer();
  void set_active_layer(LayerId layer);

  int row_id_for_action(class RecordsetLayer *&layer);
  void copy_record();
  void view_record();

  void map_menu_will_show();
  void layer_menu_will_show();

  void area_selected();
  void activate_layer(mforms::TreeNodeRef, int column);

public:
  SpatialDataView(SqlEditorResult *owner);
  virtual ~SpatialDataView();

  mforms::ToolBar *get_toolbar() {
    return _toolbar;
  }

  void set_geometry_columns(const std::vector<SpatialDataSource> &columns);
  int get_option(const char *opt_name, int default_value);

  void fillup_polygon(mforms::MenuItem *mitem);
  void projection_item_activated(mforms::ToolBarItem *item);

  void activate();
  void refresh_layers();

  void layer_menu_action(const std::string &action);
};
