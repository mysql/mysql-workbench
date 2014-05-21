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

#include "stdafx.h"
#include "spatial_data_view.h"
#include "spatial_canvas_layer.h"
#include "wb_sql_editor_result_panel.h"

#include "mforms/app.h"
#include "mforms/toolbar.h"
#include "mforms/selector.h"
#include "mforms/drawbox.h"
#include "mforms/label.h"
#include "mforms/panel.h"
#include "mforms/checkbox.h"
#include "mforms/treenodeview.h"

#include "mforms/canvas.h"
#include "mdc.h"


SpatialDataView::SpatialDataView(SqlEditorResult *owner)
: mforms::Box(false), _owner(owner)
{
  _toolbar = mforms::manage(new mforms::ToolBar(mforms::SecondaryToolBar));
  {
    mforms::ToolBarItem *item;
    item = mforms::manage(new mforms::ToolBarItem(mforms::TitleItem));
    item->set_text("Spatial View");
    _toolbar->add_item(item);

    _toolbar->add_separator_item();

    item = mforms::manage(new mforms::ToolBarItem(mforms::LabelItem));
    item->set_text("Projection:");
    _toolbar->add_item(item);

    std::vector<std::string> projection_types;
    projection_types.push_back("Equirectangular");
    projection_types.push_back("Mercator");

    _projection_picker = mforms::manage(new mforms::ToolBarItem(mforms::SelectorItem));
    _projection_picker->set_selector_items(projection_types);
    _toolbar->add_item(_projection_picker);

    _toolbar->add_separator_item();

    item = mforms::manage(new mforms::ToolBarItem(mforms::LabelItem));
    item->set_text("Zoom:");
    _toolbar->add_item(item);

    item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    item->set_icon(mforms::App::get()->get_resource_path("navigator_zoom_out.png"));
    _toolbar->add_item(item);

    item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    item->set_icon(mforms::App::get()->get_resource_path("navigator_zoom_in.png")); //XXX need @2x icons
    _toolbar->add_item(item);

    _toolbar->add_separator_item();
  }
  add(_toolbar, false, true);

  _main_box = mforms::manage(new mforms::Box(true));
  _viewer = mforms::manage(new mforms::Canvas());
  _main_box->add(_viewer, true, true);

  _option_box = mforms::manage(new mforms::Box(false));
  _option_box->set_spacing(4);
  _option_box->set_padding(8);

#ifdef __APPLE__
  _option_box->set_back_color("#f0f0f0");
#endif

  _layer_tree = mforms::manage(new mforms::TreeNodeView(mforms::TreeFlatList));
  _layer_tree->add_column(mforms::CheckColumnType, "", 30, true);
  _layer_tree->add_column(mforms::StringColumnType, "Layer", 150, false);
  _layer_tree->end_columns();
  _layer_tree->set_cell_edit_handler(boost::bind(&SpatialDataView::tree_toggled, this, _1, _3));
  _option_box->add(_layer_tree, true, true);

  _option_box->set_size(200, -1);
  _main_box->add(_option_box, false, true);

  add(_main_box, true, true);

  // configure the canvas
  _layer = new SpatialCanvasLayer(_viewer->canvas());
  _layer->set_name("spatial");
  _viewer->canvas()->add_layer(_layer);
  _viewer->canvas()->get_background_layer()->set_visible(true);
}


void SpatialDataView::show_column_data(int column, bool show)
{
  Recordset::Ref rset(_owner->recordset());

  for (ssize_t c = rset->row_count(), row = 0; row < c; row++)
  {
    std::string geom_data; // data in MySQL internal binary geometry format.. this is neither WKT nor WKB
    // but the internal format seems to be 4 bytes of SRID followed by WKB data
    if (rset->get_raw_field(row, column, geom_data) && !geom_data.empty())
    {
      g_message("--> [%i,%i] %s (%i)\n", (int)row, column, geom_data.c_str(), (int)geom_data.size());
    }
  }
}


void SpatialDataView::tree_toggled(const mforms::TreeNodeRef &node, const std::string &value)
{
  bool show = value == "1";
  node->set_bool(0, show);

  if (node->get_tag().empty())
  {
    // toggle the background
  }
  else
  {
    int index = atoi(node->get_tag().c_str());
    show_column_data(index, show);
  }
}


void SpatialDataView::set_geometry_columns(const std::vector<SpatialDataSource> &columns)
{
  bool first = true;
  for (std::vector<SpatialDataSource>::const_iterator iter = columns.begin(); iter != columns.end(); ++iter)
  {
    mforms::TreeNodeRef node = _layer_tree->add_node();
    node->set_string(1, iter->column);
    node->set_bool(0, first);
    node->set_tag(base::strfmt("%i", iter->column_index));
    first = false;
  }
  tree_toggled(_layer_tree->node_at_row(0), "1");

  // standard background layer
  mforms::TreeNodeRef node = _layer_tree->add_node();
  node->set_string(1, "World Map");
  node->set_bool(0, true);
  tree_toggled(node, "1");
}


