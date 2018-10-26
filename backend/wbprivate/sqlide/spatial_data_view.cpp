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

#include "base/log.h"
#include "base/file_utilities.h"
#include "spatial_data_view.h"
#include "spatial_draw_box.h"
#include "grt/spatial_handler.h"
#include "wb_sql_editor_form.h"
#include "wb_sql_editor_result_panel.h"

#include <algorithm>
#include <cstdlib>

#include "mforms/app.h"
#include "mforms/toolbar.h"
#include "mforms/menubar.h"
#include "mforms/checkbox.h"
#include "mforms/treeview.h"
#include "mforms/label.h"
#include "mforms/textbox.h"
#include "mforms/filechooser.h"

#include "mdc.h"

DEFAULT_LOG_DOMAIN("spatial");

class RecordsetLayer : public spatial::Layer {
  Recordset::Ptr _rset;
  int _geom_column;
  bool _loaded;

public:
  RecordsetLayer(int layer_id, base::Color color, Recordset::Ptr rset, int column)
    : spatial::Layer(layer_id, color), _rset(rset), _geom_column(column), _loaded(false) {
  }

  virtual void load_data() {
    Recordset::Ref rs(recordset());
    if (rs && !_loaded) {
      _loaded = true;
      logInfo("Loading %li rows/features from resultset\n", (long)rs->row_count());

      _render_progress = 0.0;
      ssize_t row_count = rs->row_count();
      float step = 1.0f / row_count;

      for (ssize_t c = row_count, row = 0; row < c; row++) {
        std::string geom_data; // data in MySQL internal binary geometry format.. this is neither WKT nor WKB
        // but the internal format seems to be 4 bytes of SRID followed by WKB data
        if (rs->get_raw_field(row, _geom_column, geom_data) && !geom_data.empty())
          add_feature((int)row, geom_data, false);

        _render_progress += step;
      }
    }
  }

  Recordset::Ref recordset() {
    return _rset.lock();
  }
};

class GridLayer : public spatial::Layer {
public:
  GridLayer(int layer_id, base::Color color)
    : spatial::Layer(layer_id, color) // the color is the background color, the grid color is always gray
  {
    _show = true;

    set_fill_polygons(true);
    std::string data =
      "GEOMETRYCOLLECTION(LINESTRING(-179 -89,-165 -89,-150 -89,-135 -89,-120 -89,-105 -89,-89 -89,-75 -89,-60 -89,-45 "
      "-89,-30 -89,-15 -89,0 -89,15 -89,30 -89,45 -89,60 -89,75 -89,89 -89,105 -89,120 -89,135 -89,150 -89,165 -89,179 "
      "-89),LINESTRING(-179 -75,-165 -75,-150 -75,-135 -75,-120 -75,-105 -75,-89 -75,-75 -75,-60 -75,-45 -75,-30 "
      "-75,-15 -75,0 -75,15 -75,30 -75,45 -75,60 -75,75 -75,89 -75,105 -75,120 -75,135 -75,150 -75,165 -75,179 "
      "-75),LINESTRING(-179 -60,-165 -60,-150 -60,-135 -60,-120 -60,-105 -60,-89 -60,-75 -60,-60 -60,-45 -60,-30 "
      "-60,-15 -60,0 -60,15 -60,30 -60,45 -60,60 -60,75 -60,89 -60,105 -60,120 -60,135 -60,150 -60,165 -60,179 "
      "-60),LINESTRING(-179 -45,-165 -45,-150 -45,-135 -45,-120 -45,-105 -45,-89 -45,-75 -45,-60 -45,-45 -45,-30 "
      "-45,-15 -45,0 -45,15 -45,30 -45,45 -45,60 -45,75 -45,89 -45,105 -45,120 -45,135 -45,150 -45,165 -45,179 "
      "-45),LINESTRING(-179 -30,-165 -30,-150 -30,-135 -30,-120 -30,-105 -30,-89 -30,-75 -30,-60 -30,-45 -30,-30 "
      "-30,-15 -30,0 -30,15 -30,30 -30,45 -30,60 -30,75 -30,89 -30,105 -30,120 -30,135 -30,150 -30,165 -30,179 "
      "-30),LINESTRING(-179 -15,-165 -15,-150 -15,-135 -15,-120 -15,-105 -15,-89 -15,-75 -15,-60 -15,-45 -15,-30 "
      "-15,-15 -15,0 -15,15 -15,30 -15,45 -15,60 -15,75 -15,89 -15,105 -15,120 -15,135 -15,150 -15,165 -15,179 "
      "-15),LINESTRING(-179 0,-165 0,-150 0,-135 0,-120 0,-105 0,-89 0,-75 0,-60 0,-45 0,-30 0,-15 0,0 0,15 0,30 0,45 "
      "0,60 0,75 0,89 0,105 0,120 0,135 0,150 0,165 0,179 0),LINESTRING(-179 15,-165 15,-150 15,-135 15,-120 15,-105 "
      "15,-89 15,-75 15,-60 15,-45 15,-30 15,-15 15,0 15,15 15,30 15,45 15,60 15,75 15,89 15,105 15,120 15,135 15,150 "
      "15,165 15,179 15),LINESTRING(-179 30,-165 30,-150 30,-135 30,-120 30,-105 30,-89 30,-75 30,-60 30,-45 30,-30 "
      "30,-15 30,0 30,15 30,30 30,45 30,60 30,75 30,89 30,105 30,120 30,135 30,150 30,165 30,179 30),LINESTRING(-179 "
      "45,-165 45,-150 45,-135 45,-120 45,-105 45,-89 45,-75 45,-60 45,-45 45,-30 45,-15 45,0 45,15 45,30 45,45 45,60 "
      "45,75 45,89 45,105 45,120 45,135 45,150 45,165 45,179 45),LINESTRING(-179 60,-165 60,-150 60,-135 60,-120 "
      "60,-105 60,-89 60,-75 60,-60 60,-45 60,-30 60,-15 60,0 60,15 60,30 60,45 60,60 60,75 60,89 60,105 60,120 60,135 "
      "60,150 60,165 60,179 60),LINESTRING(-179 75,-165 75,-150 75,-135 75,-120 75,-105 75,-89 75,-75 75,-60 75,-45 "
      "75,-30 75,-15 75,0 75,15 75,30 75,45 75,60 75,75 75,89 75,105 75,120 75,135 75,150 75,165 75,179 "
      "75),LINESTRING(-179 89,-165 89,-150 89,-135 89,-120 89,-105 89,-89 89,-75 89,-60 89,-45 89,-30 89,-15 89,0 "
      "89,15 89,30 89,45 89,60 89,75 89,89 89,105 89,120 89,135 89,150 89,165 89,179 89),LINESTRING(-179 -89,-179 "
      "-75,-179 -60,-179 -45,-179 -30,-179 -15,-179 0,-179 15,-179 30,-179 45,-179 60,-179 75,-179 89),LINESTRING(-165 "
      "-89,-165 -75,-165 -60,-165 -45,-165 -30,-165 -15,-165 0,-165 15,-165 30,-165 45,-165 60,-165 75,-165 "
      "89),LINESTRING(-150 -89,-150 -75,-150 -60,-150 -45,-150 -30,-150 -15,-150 0,-150 15,-150 30,-150 45,-150 "
      "60,-150 75,-150 89),LINESTRING(-135 -89,-135 -75,-135 -60,-135 -45,-135 -30,-135 -15,-135 0,-135 15,-135 "
      "30,-135 45,-135 60,-135 75,-135 89),LINESTRING(-120 -89,-120 -75,-120 -60,-120 -45,-120 -30,-120 -15,-120 "
      "0,-120 15,-120 30,-120 45,-120 60,-120 75,-120 89),LINESTRING(-105 -89,-105 -75,-105 -60,-105 -45,-105 -30,-105 "
      "-15,-105 0,-105 15,-105 30,-105 45,-105 60,-105 75,-105 89),LINESTRING(-89 -89,-89 -75,-89 -60,-89 -45,-89 "
      "-30,-89 -15,-89 0,-89 15,-89 30,-89 45,-89 60,-89 75,-89 89),LINESTRING(-75 -89,-75 -75,-75 -60,-75 -45,-75 "
      "-30,-75 -15,-75 0,-75 15,-75 30,-75 45,-75 60,-75 75,-75 89),LINESTRING(-60 -89,-60 -75,-60 -60,-60 -45,-60 "
      "-30,-60 -15,-60 0,-60 15,-60 30,-60 45,-60 60,-60 75,-60 89),LINESTRING(-45 -89,-45 -75,-45 -60,-45 -45,-45 "
      "-30,-45 -15,-45 0,-45 15,-45 30,-45 45,-45 60,-45 75,-45 89),LINESTRING(-30 -89,-30 -75,-30 -60,-30 -45,-30 "
      "-30,-30 -15,-30 0,-30 15,-30 30,-30 45,-30 60,-30 75,-30 89),LINESTRING(-15 -89,-15 -75,-15 -60,-15 -45,-15 "
      "-30,-15 -15,-15 0,-15 15,-15 30,-15 45,-15 60,-15 75,-15 89),LINESTRING(0 -89,0 -75,0 -60,0 -45,0 -30,0 -15,0 "
      "0,0 15,0 30,0 45,0 60,0 75,0 89),LINESTRING(15 -89,15 -75,15 -60,15 -45,15 -30,15 -15,15 0,15 15,15 30,15 45,15 "
      "60,15 75,15 89),LINESTRING(30 -89,30 -75,30 -60,30 -45,30 -30,30 -15,30 0,30 15,30 30,30 45,30 60,30 75,30 "
      "89),LINESTRING(45 -89,45 -75,45 -60,45 -45,45 -30,45 -15,45 0,45 15,45 30,45 45,45 60,45 75,45 "
      "89),LINESTRING(60 -89,60 -75,60 -60,60 -45,60 -30,60 -15,60 0,60 15,60 30,60 45,60 60,60 75,60 "
      "89),LINESTRING(75 -89,75 -75,75 -60,75 -45,75 -30,75 -15,75 0,75 15,75 30,75 45,75 60,75 75,75 "
      "89),LINESTRING(89 -89,89 -75,89 -60,89 -45,89 -30,89 -15,89 0,89 15,89 30,89 45,89 60,89 75,89 "
      "89),LINESTRING(105 -89,105 -75,105 -60,105 -45,105 -30,105 -15,105 0,105 15,105 30,105 45,105 60,105 75,105 "
      "89),LINESTRING(120 -89,120 -75,120 -60,120 -45,120 -30,120 -15,120 0,120 15,120 30,120 45,120 60,120 75,120 "
      "89),LINESTRING(135 -89,135 -75,135 -60,135 -45,135 -30,135 -15,135 0,135 15,135 30,135 45,135 60,135 75,135 "
      "89),LINESTRING(150 -89,150 -75,150 -60,150 -45,150 -30,150 -15,150 0,150 15,150 30,150 45,150 60,150 75,150 "
      "89),LINESTRING(165 -89,165 -75,165 -60,165 -45,165 -30,165 -15,165 0,165 15,165 30,165 45,165 60,165 75,165 "
      "89),LINESTRING(179 -89,179 -75,179 -60,179 -45,179 -30,179 -15,179 0,179 15,179 30,179 45,179 60,179 75,179 "
      "89))";

    add_feature(0, data, true);
  }

  virtual void repaint(mdc::CairoCtx &cr, float scale, const base::Rect &clip_area) {
    std::deque<spatial::ShapeContainer>::const_iterator it;

    cr.save();
    cr.set_line_width(0.5);
    cr.set_color(base::Color(0.4, 0.4, 0.4));
    for (std::deque<spatial::Feature *>::iterator it = _features.begin(); it != _features.end() && !_interrupt; ++it)
      (*it)->repaint(cr, scale, clip_area);

    cr.restore();
  }
};

SpatialDataView::SpatialDataView(SqlEditorResult *owner) : mforms::Box(false), _owner(owner), _activated(false) {
  _splitter = mforms::manage(new mforms::Splitter(true, true));
  _rendering = false;

  _main_box = mforms::manage(new mforms::Box(true));
  _viewer = mforms::manage(new SpatialDrawBox());
  _viewer->position_changed_cb = std::bind(&SpatialDataView::update_coordinates, this, std::placeholders::_1);
  _viewer->position_clicked_cb = std::bind(&SpatialDataView::handle_click, this, std::placeholders::_1);
  _viewer->work_started = std::bind(&SpatialDataView::work_started, this, std::placeholders::_1, std::placeholders::_2);
  _viewer->work_finished = std::bind(&SpatialDataView::work_finished, this, std::placeholders::_1);
  _viewer->get_option = std::bind(&SpatialDataView::get_option, this, std::placeholders::_1, std::placeholders::_2);
  _viewer->area_selected = std::bind(&SpatialDataView::area_selected, this);

  _active_layer = 0;

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
    projection_types.push_back("Robinson");
    projection_types.push_back("Mercator");
    projection_types.push_back("Equirectangular");
    projection_types.push_back("Bonne");

    _projection_picker = mforms::manage(new mforms::ToolBarItem(mforms::SelectorItem));
    _projection_picker->set_selector_items(projection_types);

    scoped_connect(_projection_picker->signal_activated(),
                   std::bind(&SpatialDataView::projection_item_activated, this, std::placeholders::_1));

    _toolbar->add_item(_projection_picker);

    _toolbar->add_separator_item();

    item = mforms::manage(new mforms::ToolBarItem(mforms::LabelItem));
    item->set_text("Tool:");
    _toolbar->add_item(item);

    item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
    item->set_name("Reset Tool");
    item->setInternalName("reset_tool");
    item->set_icon(mforms::App::get()->get_resource_path("wb_arrow.png"));
    item->set_tooltip("Pan map and select feature to view");
    item->signal_activated()->connect(std::bind(&SpatialDataView::change_tool, this, item));
    _toolbar->add_item(item);
    item->set_checked(true);

    item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
    item->set_name("Zoom to Area");
    item->setInternalName("zoom_to_area");
    item->set_icon(mforms::App::get()->get_resource_path("qe_sql-editor-tb-icon_zoom-area.png"));
    item->set_tooltip("Zoom to area. Click and drag in the map to select an area to be zoomed into.");
    item->signal_activated()->connect(std::bind(&SpatialDataView::change_tool, this, item));
    _toolbar->add_item(item);

    _toolbar->add_separator_item();

    item = mforms::manage(new mforms::ToolBarItem(mforms::LabelItem));
    item->set_text("Zoom:");
    _toolbar->add_item(item);

    item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    item->set_icon(mforms::App::get()->get_resource_path("qe_sql-editor-tb-icon_zoom-out.png"));
    item->set_tooltip("Zoom out one step");
    item->signal_activated()->connect(std::bind(&SpatialDrawBox::zoom_out, _viewer));
    _toolbar->add_item(item);

    item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    item->set_icon(mforms::App::get()->get_resource_path("qe_sql-editor-tb-icon_zoom-in.png"));
    item->set_tooltip("Zoom in one step");
    item->signal_activated()->connect(std::bind(&SpatialDrawBox::zoom_in, _viewer));
    _toolbar->add_item(item);

    item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    item->set_icon(mforms::App::get()->get_resource_path("qe_sql-editor-tb-icon_zoom-reset.png"));
    item->set_tooltip("Reset zoom to the outermost zoom level");
    item->signal_activated()->connect(std::bind(&SpatialDrawBox::reset_view, _viewer));
    _toolbar->add_item(item);

    _toolbar->add_separator_item();

    item = mforms::manage(new mforms::ToolBarItem(mforms::LabelItem));
    item->set_text("Jump To:");
    _toolbar->add_item(item);

    item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    item->set_icon(mforms::App::get()->get_resource_path("qe_sql-editor-tb-icon_zoom-jump.png"));
    item->set_tooltip("Specify coordinates to center screen on.");
    item->signal_activated()->connect(std::bind(&SpatialDataView::jump_to, this));
    _toolbar->add_item(item);

    _toolbar->add_separator_item();
    item = mforms::manage(new mforms::ToolBarItem(mforms::LabelItem));
    item->set_text("Export:");
    _toolbar->add_item(item);

    item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    item->set_icon(mforms::App::get()->get_resource_path("record_export.png"));
    item->set_tooltip(_("Export visible area as PNG image."));
    item->signal_activated()->connect(std::bind(&SpatialDataView::export_image, this));
    _toolbar->add_item(item);
  }
  add(_toolbar, false, true);

  _splitter->add(_viewer, 100);

  _option_box = mforms::manage(new mforms::Box(false));
  _option_box->set_spacing(4);
  _option_box->set_padding(8);

#if defined(__APPLE__) || defined(_MSC_VER)
  _option_box->set_back_color("#f0f0f0");
#endif

  _map_menu = new mforms::ContextMenu();
  _map_menu->add_item_with_title("Copy Coordinates", std::bind(&SpatialDataView::copy_coordinates, this), "Copy Coordinates", "");
  _map_menu->add_item_with_title("Copy Record for Feature", std::bind(&SpatialDataView::copy_record, this), "Copy Record for Feature", "");
  _map_menu->add_item_with_title("View Record for Feature", std::bind(&SpatialDataView::view_record, this), "View Record for Feature", "");
  _map_menu->signal_will_show()->connect(std::bind(&SpatialDataView::map_menu_will_show, this));

  _viewer->set_context_menu(_map_menu);

  _layer_menu = new mforms::ContextMenu();
  //  _layer_menu->add_item_with_title("Set Color...", std::bind(&SpatialDataView::activate, this));
  //  _layer_menu->add_item_with_title("Properties...", std::bind(&SpatialDataView::activate, this));

  mforms::MenuItem *mitem = mforms::manage(new mforms::MenuItem("Fill Polygons", mforms::CheckedMenuItem));
  mitem->set_name("Fillup Polygon");
  mitem->setInternalName("fillup_polygon");
  mitem->signal_clicked()->connect(std::bind(&SpatialDataView::fillup_polygon, this, mitem));
  _layer_menu->add_item(mitem);

  _layer_menu->add_separator();
  _layer_menu->add_item_with_title("Refresh", std::bind(&SpatialDataView::refresh_layers, this), "Refresh", "refresh");

  _layer_menu->add_separator();
  _layer_menu->add_item_with_title("Move Layer Up", std::bind(&SpatialDataView::layer_menu_action, this, "layer_up"),
                                   "Move Layer Up", "layer_up");
  _layer_menu->add_item_with_title("Move Layer Down",
                                   std::bind(&SpatialDataView::layer_menu_action, this, "layer_down"), "Move Layer Down", "layer_down");

  _layer_menu->signal_will_show()->connect(std::bind(&SpatialDataView::layer_menu_will_show, this));

  _layer_tree = mforms::manage(new mforms::TreeView(mforms::TreeFlatList));
  _layer_tree->add_column(mforms::CheckColumnType, "", 25, true);
  _layer_tree->add_column(mforms::IconStringColumnType, "Layer", 120, false, true);
  _layer_tree->add_column(mforms::StringColumnType, "Source", 200, false, true);
  _layer_tree->end_columns();
  _layer_tree->set_cell_edit_handler(
    std::bind(&SpatialDataView::tree_toggled, this, std::placeholders::_1, std::placeholders::_3));
  _layer_tree->set_context_menu(_layer_menu);
  _layer_tree->signal_node_activated()->connect(
    std::bind(&SpatialDataView::activate_layer, this, std::placeholders::_1, std::placeholders::_2));
  _layer_tree->signal_changed()->connect(
    std::bind(&SpatialDataView::activate_layer, this, mforms::TreeNodeRef(),
              -42)); // unused dummy value... should just not conflict with possibly valid values

  _layer_tree->set_row_overlay_handler(std::bind(&SpatialDataView::layer_overlay_handler, this, std::placeholders::_1));
  _layer_tree->set_size(-1, 150);
  _option_box->add(_layer_tree, true, true);

  _mouse_pos_label = mforms::manage(new mforms::Label("Lat:\nLon:"));
  _option_box->add(_mouse_pos_label, false, true);

  _info_box = mforms::manage(new mforms::TextBox(mforms::VerticalScrollBar));
  _option_box->add(_info_box, true, true);
  _info_box->set_value("Click a feature to view its record");

  _option_box->set_size(220, -1);
  _splitter->add(_option_box, 200);

  _splitter->signal_position_changed()->connect(std::bind(&SpatialDataView::call_refresh_viewer, this));

  add(_splitter, true, true);
}

std::vector<std::string> SpatialDataView::layer_overlay_handler(mforms::TreeNodeRef node) {
  std::vector<std::string> icons;
  icons.push_back(mforms::App::get()->get_resource_path("wb_item_overlay_autozoom.png"));
  return icons;
}

void SpatialDataView::call_refresh_viewer() {
  if (!_rendering) {
    if (_spliter_change_timeout != 0) {
      mforms::Utilities::cancel_timeout(_spliter_change_timeout);
      _spliter_change_timeout = 0;
    }
    _spliter_change_timeout = mforms::Utilities::add_timeout(0.5, std::bind(&SpatialDataView::refresh_viewer, this));
  }
}

bool SpatialDataView::refresh_viewer() {
  if (_rendering)
    return false;
  _spliter_change_timeout = 0;
  _viewer->invalidate(true);

  return false;
}

void SpatialDataView::change_tool(mforms::ToolBarItem *item) {
  item->set_checked(true);
  if (item->getInternalName() == "reset_tool") {
    _toolbar->set_item_checked("zoom_to_area", false);
    _viewer->select_area(false);
  } else {
    _viewer->select_area(true);
    _toolbar->set_item_checked("reset_tool", false);
  }
}

int SpatialDataView::get_option(const char *opt_name, int default_value) {
  return bec::GRTManager::get()->get_app_option_int(opt_name, default_value) != 0;
}

void SpatialDataView::area_selected() {
  _toolbar->set_item_checked("zoom_to_area", false);
  _toolbar->set_item_checked("reset_tool", true);
  _viewer->select_area(false);
}

void SpatialDataView::fillup_polygon(mforms::MenuItem *mitem) {
  if (_layer_tree->is_enabled()) {
    spatial::Layer *layer = _viewer->get_layer(get_selected_layer_id());
    if (layer)
      layer->set_fill_polygons(mitem->get_checked());

    _viewer->invalidate();
  }
}

void SpatialDataView::projection_item_activated(mforms::ToolBarItem *item) {
  std::string action = item->get_text();
  if (action == "Mercator")
    _viewer->set_projection(spatial::ProjMercator);
  else if (action == "Equirectangular")
    _viewer->set_projection(spatial::ProjEquirectangular);
  else if (action == "Robinson")
    _viewer->set_projection(spatial::ProjRobinson);
  else if (action == "Bonne")
    _viewer->set_projection(spatial::ProjBonne);
}

SpatialDataView::~SpatialDataView() {
  if (_spliter_change_timeout != 0) {
    mforms::Utilities::cancel_timeout(_spliter_change_timeout);
    _spliter_change_timeout = 0;
  }
  delete _layer_menu;
}

static double parse_latitude(const std::string &s) {
  double parsed = 0.0;

  if (s.empty())
    throw std::invalid_argument("Invalid value");

  // check if in degrees
  if (s.find("\xc2\xb0") != std::string::npos) // look for degree sign in utf8
  {
    int deg = 0, min = 0;
    float sec = 0;
    char o = *s.rbegin();

    if (o != 'N' && o != 'S' && o != '"' && !isdigit(o))
      throw std::invalid_argument("Latitude value must be N or S");

    if (sscanf(s.c_str(), "%i\xc2\xb0%i'%f\"", &deg, &min, &sec) == 0)
      throw std::invalid_argument("Unable to parse latitude value " + s);

    parsed = deg + (min / 60.0) + (sec / 3600.0);
    if (o == 'S')
      parsed = -parsed;
  } else
    parsed = strtod(s.c_str(), NULL);

  return parsed;
}

static double parse_longitude(const std::string &s) {
  double parsed = 0.0;

  if (s.empty())
    throw std::invalid_argument("Invalid value");

  // check if in degrees
  if (s.find("\xc2\xb0") != std::string::npos) // look for degree sign in utf8
  {
    int deg = 0, min = 0;
    float sec = 0;
    char o = *s.rbegin();

    if (o != 'E' && o != 'W' && o != '"' && !isdigit(o))
      throw std::invalid_argument("Longitude value must be E or W");

    if (sscanf(s.c_str(), "%i\xc2\xb0%i'%f\"", &deg, &min, &sec) == 0)
      throw std::invalid_argument("Unable to parse longitude value " + s);

    parsed = deg + (min / 60.0) + (sec / 3600.0);
    if (o == 'W')
      parsed = -parsed;
  } else
    parsed = strtod(s.c_str(), NULL);

  return parsed;
}

void SpatialDataView::jump_to() {
  std::string ret;
  bool badformat = false;
  if (mforms::Utilities::request_input("Jump to Coordinates", "Enter coordinates in Lat, Lon:", "", ret)) {
    std::string lat, lon;
    if (base::partition(ret, ",", lat, lon)) {
      double plat = parse_latitude(base::strip_text(lat));
      double plon = parse_longitude(base::strip_text(lon));

      _viewer->center_on(plat, plon);
    } else
      badformat = true;
  }

  if (badformat) {
    mforms::Utilities::show_message(
      "Jump to Coordinates",
      "Coordinates must be in Lat, Lon format.\nEx.: 40.32321312, -120.3232131 or 54°50'26.7\"N 98°23'51.0\"E", "OK");
  }
}

void SpatialDataView::export_image() {
  mforms::FileChooser fc(mforms::SaveFile);
  fc.set_title("Save Spatial View Image to File");
  fc.set_extensions("PNG Files (*.png)|*.png", "png");
  if (fc.run_modal()) {
    try {
      _viewer->save_to_png(fc.get_path());
      mforms::Utilities::show_message(
        _("Save to File"), base::strfmt(_("Image has been succesfully saved to '%s'"), fc.get_path().c_str()), _("OK"));
    } catch (std::exception &exc) {
      mforms::Utilities::show_error(
        _("Save to File"), base::strfmt(_("Could not save to file '%s': %s"), fc.get_path().c_str(), exc.what()),
        _("OK"));
    }
  }
}

spatial::LayerId SpatialDataView::get_selected_layer_id() {
  mforms::TreeNodeRef node(_layer_tree->get_selected_node());
  if (node)
    return base::atoi<int>(node->get_tag(), 0);
  return 0;
}

void SpatialDataView::auto_zoom(LayerId layer) {
  _viewer->clear_pins();
  _viewer->auto_zoom(layer);
  _viewer->invalidate(true);
}

void SpatialDataView::copy_coordinates() {
  std::pair<double, double> p = _viewer->clicked_coordinates();

  mforms::Utilities::set_clipboard_text(base::strfmt("%.6f, %.6f", p.first, p.second));
}

RecordsetLayer *SpatialDataView::active_layer() {
  std::deque<spatial::Layer *> layers(_viewer->get_layers());

  for (std::deque<spatial::Layer *>::const_iterator l = layers.begin(); l != layers.end(); ++l) {
    if ((*l)->layer_id() == _active_layer)
      return dynamic_cast<RecordsetLayer *>(*l);
  }
  return NULL;
}

int SpatialDataView::row_id_for_action(RecordsetLayer *&layer) {
  layer = active_layer();
  if (layer)
    return _viewer->clicked_row_id();
  return -1;
}

void SpatialDataView::map_menu_will_show() {
}

void SpatialDataView::layer_menu_will_show() {
  spatial::Layer *layer = _viewer->get_layer(get_selected_layer_id());

  _layer_menu->set_item_enabled("set_active", layer && layer->layer_id() != _grid_layer);
  _layer_menu->set_item_checked("fillup_polygon", layer && layer->fill());

  mforms::TreeNodeRef node = _layer_tree->get_selected_node();
  spatial::LayerId bg_layer_id = _viewer->get_background()->layer_id();
  if (node.is_valid() && base::atoi<int>(node->get_tag(), 0) != bg_layer_id) {
    mforms::TreeNodeRef pnode = node->previous_sibling(), nnode = node->next_sibling();

    _layer_menu->set_item_enabled("layer_up", pnode.is_valid() && base::atoi<int>(pnode->get_tag(), 0) != bg_layer_id);
    _layer_menu->set_item_enabled("layer_down",
                                  nnode.is_valid() && base::atoi<int>(nnode->get_tag(), 0) != bg_layer_id);
  } else {
    _layer_menu->set_item_enabled("layer_up", false);
    _layer_menu->set_item_enabled("layer_down", false);
  }
}

void SpatialDataView::copy_record() {
  RecordsetLayer *layer = NULL;
  int row_id = row_id_for_action(layer);
  if (layer) {
    bool flag = false;
    if (row_id >= 0) {
      Recordset::Ref rs(layer->recordset());
      if (rs) {
        std::string text;
        std::string value;

        for (size_t i = 0; i < rs->get_column_count(); i++) {
          if (i > 0)
            text.append(",");
          if (rs->get_field(row_id, i, value))
            text.append(value);
        }
        mforms::Utilities::set_clipboard_text(text);
        flag = true;
      }
    }
    if (!flag)
      mforms::App::get()->set_status_text("No row found for clicked coordinates.");
  } else
    mforms::App::get()->set_status_text("No visible layers.");
}

void SpatialDataView::view_record() {
  RecordsetLayer *layer = NULL;
  int row_id = row_id_for_action(layer);
  if (layer) {
    if (row_id >= 0) {
      _owner->view_record_in_form(row_id);
    } else
      mforms::App::get()->set_status_text("No row found for clicked coordinates.");
  } else
    mforms::App::get()->set_status_text("No visible layers.");
}

void SpatialDataView::work_started(mforms::View *progress_panel, bool reprojecting) {
  _rendering = true;
  _layer_tree->set_enabled(false);
  _layer_menu->set_item_enabled("refresh", false);
  if (reprojecting) {
    progress_panel->set_size(500, 150);
    _viewer->add(progress_panel, mforms::MiddleCenter);
    // this is causing a loop in the Mac, where the relayout causes the splitter to be resized
    // which then triggers a re-render and so on... commenting out the relayout() seems to
    // not have any effects, so let's try that...
    // relayout();
  }
}

void SpatialDataView::work_finished(mforms::View *progress_panel) {
  _rendering = false;
  _layer_tree->set_enabled(true);
  _layer_menu->set_item_enabled("refresh", true);
  _viewer->remove(progress_panel);
  _main_box->show(true);
}

void SpatialDataView::activate() {
  if (!_activated) {
    _activated = true;
    if (_splitter->get_divider_position() != this->get_width() - 200)
      _splitter->set_divider_position(this->get_width() - 200);
  }
  _viewer->activate();
}

void SpatialDataView::refresh_layers() {
  std::vector<SpatialDataView::SpatialDataSource> spatial_columns; // = _owner->get_spatial_columns();

  for (int c = _owner->owner()->owner()->sql_editor_count(), editor = 0; editor < c; editor++) {
    SqlEditorPanel *panel = _owner->owner()->owner()->sql_editor_panel(editor);
    if (panel != NULL) {
      for (size_t i = 0; i < panel->result_panel_count(); ++i) {
        SqlEditorResult *result = panel->result_panel((int)i);
        if (result) {
          std::vector<SpatialDataView::SpatialDataSource> tmp(result->get_spatial_columns());
          std::copy(tmp.begin(), tmp.end(), std::back_inserter(spatial_columns));
        }
      }
    }
  }

  set_geometry_columns(spatial_columns);
  if (get_option("SqlEditor::SpatialAutoZoom", 1) >= 1)
    _viewer->auto_zoom(_active_layer);
}

mforms::TreeNodeRef static move_node_to(mforms::TreeNodeRef &node, mforms::TreeNodeRef &new_parent, int index) {
  mforms::TreeNodeRef new_node = new_parent->insert_child(index);
  new_node->set_bool(0, node->get_bool(0));
  new_node->set_string(1, node->get_string(1));
  new_node->set_string(2, node->get_string(2));
  new_node->set_tag(node->get_tag());
  new_node->set_data(node->get_data());
  node->remove_from_parent();
  return new_node;
}

void SpatialDataView::layer_menu_action(const std::string &action) {
  mforms::TreeNodeRef node = _layer_tree->get_selected_node();
  mforms::TreeNodeRef group_node = node->get_parent();
  size_t node_index = node->get_child_index(node), new_index = node_index;

  if (action == "layer_up") {
    if (node->previous_sibling().is_valid())
      new_index = node_index - 1;
  } else if (action == "layer_down") {
    if (node->next_sibling().is_valid())
      new_index = node_index + 2;
  }

  node = move_node_to(node, group_node, (int)new_index);
  spatial::Layer *layer = _viewer->get_layer(base::atoi<int>(node->get_tag(), 0));
  if (layer)
    set_color_icon(node, 1, layer->color());

  std::vector<int> order;
  order.reserve(_layer_tree->count());

  for (int i = 0; i < _layer_tree->count(); ++i) {
    spatial::LayerId layer_id = base::atoi<int>(_layer_tree->node_at_row(i)->get_tag(), 0);
    if (layer_id != _viewer->get_background()->layer_id())
      order.push_back(layer_id);
  }

  _viewer->change_layer_order(order);
  _layer_tree->select_node(node);
  _viewer->invalidate(false);
}

void SpatialDataView::set_color_icon(mforms::TreeNodeRef node, int column, const base::Color &color) {
  static std::string path;
  if (path.empty()) {
    path = mforms::Utilities::get_special_folder(mforms::ApplicationData) + "/tmpicons";
    base::create_directory(path, 0700);
  }
  std::string p = path + "/" + base::strfmt("%02x%02x%02x.png", (unsigned char)(color.red * 255),
                                            (unsigned char)(color.green * 255), (unsigned char)(color.blue * 255));

  if (!base::file_exists(p)) {
    cairo_surface_t *surf = cairo_image_surface_create(CAIRO_FORMAT_RGB24, 16, 16);
    cairo_t *cr = cairo_create(surf);
    cairo_set_source_rgb(cr, color.red, color.green, color.blue);
    cairo_paint(cr);
    cairo_destroy(cr);
    cairo_surface_write_to_png(surf, p.c_str());
    cairo_surface_destroy(surf);
  }
  node->set_icon_path(column, p);
}

void SpatialDataView::tree_toggled(const mforms::TreeNodeRef &node, const std::string &value) {
  if (_layer_tree->is_enabled()) {
    bool show = value == "1";
    node->set_bool(0, show);

    _viewer->show_layer(base::atoi<int>(node->get_tag(), 0), show);
  }
}

void SpatialDataView::activate_layer(mforms::TreeNodeRef node, int column) {
  if (!node)
    node = _layer_tree->get_selected_node();

  if (node) {
    if (column == -1)
      auto_zoom(base::atoi<int>(node->get_tag(), 0));
    else
      set_active_layer(base::atoi<int>(node->get_tag(), 0));
  }
}

static spatial::Layer *find_layer_for(std::deque<spatial::Layer *> &layers, Recordset::Ref rset, int column) {
  for (std::deque<spatial::Layer *>::iterator l = layers.begin(); l != layers.end(); ++l) {
    RecordsetLayer *rsl = dynamic_cast<RecordsetLayer *>(*l);
    if (rsl && rsl->recordset() == rset)
      return *l;
  }
  return NULL;
}

void SpatialDataView::set_active_layer(spatial::LayerId layer) {
  if (_grid_layer == layer)
    return;

  _active_layer = layer;

  mforms::TreeNodeTextAttributes plain;
  for (int i = 0; i < _layer_tree->count(); i++) {
    mforms::TreeNodeRef node(_layer_tree->node_at_row(i));
    if (node) {
      if (base::atoi<int>(node->get_tag(), -1) == _active_layer) {
        mforms::TreeNodeTextAttributes attribs;
        attribs.bold = true;
        node->set_attributes(1, attribs);
        node->set_attributes(2, attribs);
      } else {
        node->set_attributes(1, plain);
        node->set_attributes(2, plain);
      }
    }
  }
}

void SpatialDataView::set_geometry_columns(const std::vector<SpatialDataSource> &sources) {
  static base::Color layer_colors[] = {
    base::Color::parse("#b8ddf3"), // background color

    base::Color(0.9, 1, 0.9),      base::Color(1, 0.9, 1.0),

    base::Color(0.8, 0.8, 0.4),    base::Color(0.4, 0.8, 0.8), base::Color(0.8, 0.4, 0.8),

    base::Color(0.8, 0.4, 0.4),    base::Color(0.4, 0.8, 0.4), base::Color(0.4, 0.4, 0.8),

    base::Color(0.0, 0.6, 0.6),    base::Color(0.6, 0.0, 0.6), base::Color(0.6, 0.6, 0.0),

    base::Color(0.6, 0.0, 0.0),    base::Color(0.0, 0.6, 0.0), base::Color(0.0, 0.0, 0.6)};

  if (_layer_tree->count() == 0) {
    base::Color color(layer_colors[0]);
    mforms::TreeNodeRef node = _layer_tree->add_node();
    node->set_string(1, "Grid");
    set_color_icon(node, 1, color);
    node->set_bool(0, true);
    _grid_layer = spatial::new_layer_id();
    node->set_tag(base::strfmt("%i", _grid_layer));
    _viewer->set_background(new GridLayer(_grid_layer, color));
  }

  std::deque<spatial::Layer *> layers(_viewer->get_layers());
  // remove layers that are gone
  for (std::deque<spatial::Layer *>::iterator l = layers.begin(); l != layers.end(); ++l) {
    RecordsetLayer *rsl = dynamic_cast<RecordsetLayer *>(*l);
    if (rsl) {
      Recordset::Ref rset(rsl->recordset());
      bool found = false;
      if (rset) {
        for (std::vector<SpatialDataSource>::const_iterator iter = sources.begin(); iter != sources.end(); ++iter) {
          if (!iter->resultset.expired() && iter->resultset.lock() == rset) {
            found = true;
            break;
          }
        }
      }
      if (!found) {
        // find the node for the layer
        for (int i = 0; i < _layer_tree->count(); i++) {
          mforms::TreeNodeRef node;
          if (base::atoi<int>((node = _layer_tree->node_at_row(i))->get_tag(), 0) == (*l)->layer_id()) {
            node->remove_from_parent();
            break;
          }
        }
        _viewer->remove_layer(*l);
        delete *l;
        *l = NULL;
      }
    }
  }

  int idx = 1;
  for (std::vector<SpatialDataSource>::const_iterator iter = sources.begin(); iter != sources.end(); ++iter) {
    // check if already exists
    if (!iter->resultset.expired() && find_layer_for(layers, iter->resultset.lock(), iter->column_index))
      continue;

    int layer_id = spatial::new_layer_id();
    base::Color color(layer_colors[(idx++) % (sizeof(layer_colors) / sizeof(base::Color))]);
    mforms::TreeNodeRef node = _layer_tree->add_node();
    node->set_bool(0, false);
    node->set_string(1, iter->column);

    node->set_string(2, iter->source);
    node->set_tag(base::strfmt("%i", layer_id));
    set_color_icon(node, 1, color);

    spatial::Layer *layer = NULL;
    if (iter->column_index >= 0) {
      layer = new RecordsetLayer(layer_id, color, iter->resultset, iter->column_index);
      if (_owner->recordset()->key() == iter->resultset.lock()->key()) {
        layer->set_show(true);
        node->set_bool(0, true);
        set_active_layer(layer_id);
      }
    } else {
      // from file
    }
    if (layer) {
      _viewer->add_layer(layer);
    }
  }
}

void SpatialDataView::update_coordinates(base::Point p) {
  double lat, lon;
  if (_viewer->screen_to_world((int)p.x, (int)p.y, lat, lon))
    _mouse_pos_label->set_text(base::strfmt("Lat:  %s\nLon: %s",
                                            spatial::Converter::dec_to_dms(lat, spatial::AxisLat, 2).c_str(),
                                            spatial::Converter::dec_to_dms(lon, spatial::AxisLon, 2).c_str()));
  else
    _mouse_pos_label->set_text("Lat: \nLon: ");
}

void SpatialDataView::handle_click(base::Point p) {
  RecordsetLayer *layer = active_layer();
  std::string text;

  _viewer->clear_pins();
  if (layer) {
    spatial::Feature *feature = layer->feature_closest(_viewer->apply_cairo_transformation(p));
    if (feature) {
      int row_id = feature->row_id();
      if (row_id >= 0) {
        Recordset::Ref rs(layer->recordset());
        if (rs) {
          std::string value;

          _viewer->place_pin(mforms::Utilities::load_icon("qe_sql-editor-resultset-tb-pinned.png"), p);

          for (size_t i = 0; i < rs->get_column_count(); i++) {
            if (i > 0)
              text.append("\n");
            text.append(rs->get_column_caption(i)).append(": ");
            if (rs->get_field(row_id, i, value))
              text.append(value);
          }
        }
      }
    }
  }
  _info_box->set_value(text);
}
