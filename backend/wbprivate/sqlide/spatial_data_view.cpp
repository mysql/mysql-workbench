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
#include "base/log.h"
#include "base/file_utilities.h"
#include "spatial_data_view.h"
#include "spatial_handler.h"
#include "wb_sql_editor_result_panel.h"

#include <algorithm>

#include "mforms/app.h"
#include "mforms/toolbar.h"
#include "mforms/selector.h"
#include "mforms/menubar.h"
#include "mforms/drawbox.h"
#include "mforms/label.h"
#include "mforms/panel.h"
#include "mforms/checkbox.h"
#include "mforms/treenodeview.h"
#include "mforms/drawbox.h"

#include "mdc.h"

#include <gdal/ogrsf_frmts.h>
#include <gdal/ogr_api.h>
#include <gdal/gdal_pam.h>

DEFAULT_LOG_DOMAIN("sqlide");

static double ZoomLevels[] = {
  0.0, 0.2, 0.4, 0.6, 0.7, 0.8, 0.85, 0.90, 0.95, 0.97, 0.98, 0.99, 0.995
};

struct LayerInfo
{
  GIS::SpatialHandler* handler;
  std::deque<GIS::ShapeContainer> shapes;

  base::Color color;
  bool show;
};


class SpatialDrawBox : public mforms::DrawBox
{
  std::deque<LayerInfo> _layers;
  GIS::ProjectionType _proj;
  mdc::Surface *_cache;

  int _zoom_level;
  int _offset_x, _offset_y;

  int _drag_x, _drag_y;
  bool _dragging;

  bool _needs_rerender;

public:
  SpatialDrawBox()
  : _proj(GIS::ProjDefault), _zoom_level(0), _offset_x(0), _offset_y(0), _dragging(false),
  _needs_rerender(false)
  {
    _cache = NULL;
  }

  ~SpatialDrawBox()
  {
    delete _cache;
  }

  void set_projection(GIS::ProjectionType proj)
  {
    _proj = proj;
    invalidate();
  }

  void zoom_out()
  {
    _zoom_level = std::min((int)(sizeof(ZoomLevels)/sizeof(double))-1, _zoom_level+1);
    invalidate();
  }

  void zoom_in()
  {
    _zoom_level = std::max(0, _zoom_level-1);
    invalidate();
  }

  void add_layer_with_data(const std::string &geom_data, base::Color color)
  {
    GIS::SpatialHandler *h = new GIS::SpatialHandler();
    h->importFromMySQL(geom_data);

    LayerInfo l;
    l.handler = h;
    l.color = color;
    l.show = true;

    _layers.push_back(l);
    invalidate();
  }

  void invalidate()
  {
    delete _cache;
    _cache = NULL;

    set_needs_repaint();
    _needs_rerender = true;
  }


  virtual bool mouse_down(mforms::MouseButton button, int x, int y)
  {
    if (button == 0)
    {
      _drag_x = x;
      _drag_y = y;
      _dragging = true;
    }
    return false;
  }

  virtual bool mouse_up(mforms::MouseButton button, int x, int y)
  {
    if (button == 0 && _dragging)
    {
      mouse_move(button, x, y);
      _dragging = false;
    }
    return false;
  }

  virtual bool mouse_move(mforms::MouseButton button, int x, int y)
  {
    if (_dragging)
    {
      _offset_x += _drag_x - x;
      _offset_y -= _drag_y - y;
      if (_offset_x < 0)
        _offset_x = 0;
      if (_offset_y < 0)
        _offset_y = 0;
      set_needs_repaint();
    }
    return false;
  }


  virtual void repaint(cairo_t *crt, int x, int y, int w, int h)
  {
    mdc::CairoCtx cr(crt);

    cr.set_color(base::Color(1, 1, 1));
    cr.paint();

    if (_needs_rerender)
    {
      _needs_rerender = false;

      double zoom = ZoomLevels[_zoom_level];
      int width = get_width();
      int height = get_height();

      GIS::ProjectionView visible_area;

      // calculate how much the offset in pixels corresponds to in lon/lat values, so that gdal will adjust the
      // clipping area to the area we want to view
      double dlo = 0, dla = 0;
      screen_to_world(_offset_x, _offset_y, dla, dlo);

      visible_area.MaxLat = 180 - 180*zoom + dla;
      visible_area.MaxLng = 90 - 90*zoom + dlo;
      visible_area.MinLat = -180 + 180*zoom + dla;
      visible_area.MinLng = -90 + 90*zoom + dlo;

      visible_area.height = height;
      visible_area.width = width;
      visible_area.type = _proj;

      // TODO lat/long ranges must be adjusted according to account for the aspect ratio of the visible area


      for (std::deque<LayerInfo>::iterator it = _layers.begin(); it != _layers.end(); ++it)
      {
        std::deque<GIS::ShapeContainer> shapes;
        // method names must get_output() like.. camel case is only for class/struct names
        it->handler->getOutput(visible_area, shapes); //XXX separate width/height and projection type into separate params
        it->shapes = shapes;
      }
    }

    if (!_cache)
    {
      _cache = new mdc::ImageSurface(get_width(), get_height(), CAIRO_FORMAT_ARGB32);
      mdc::CairoCtx ctx(*_cache);
      // cache everything into a bitmap
      for (std::deque<LayerInfo>::const_iterator it = _layers.begin(); it != _layers.end(); ++it)
        repaint_layer(ctx, *it);
    }

    cr.set_source_surface(_cache->get_surface(), 0, 0);
    cr.paint();
  }

  void screen_to_world(int x, int y, double &lat, double &lon)
  {
    // convert screen pixel values to the equivalent in latitude/longitude values for the current zoom level

    //XXX must be done using gdal
  }

  void world_to_screen(double lat, double lon, int &x, int &y)
  {

  }

  void repaint_layer(mdc::CairoCtx &cr, const LayerInfo &layer)
  {
    std::deque<GIS::ShapeContainer>::const_iterator it;

    cr.set_line_width(1);
    cr.set_color(layer.color);
    cr.save();
    for(it = layer.shapes.begin(); it != layer.shapes.end(); it++)
    {
      if ((*it).type == GIS::ShapePolygon || (*it).type == GIS::ShapeLineString)
      {
        cr.move_to((*it).points[0]);
        for (size_t i = 0; i < (*it).points.size(); i++)
          cr.line_to((*it).points[i]);
        cr.stroke();
      }
      else
        log_debug("Unknown type %i\n", it->type);
    }
    cr.restore();
  }

};


SpatialDataView::SpatialDataView(SqlEditorResult *owner)
: mforms::Box(false), _owner(owner)
{
  _main_box = mforms::manage(new mforms::Box(true));
  _viewer = mforms::manage(new SpatialDrawBox());

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
    projection_types.push_back("Robinson");

    _projection_picker = mforms::manage(new mforms::ToolBarItem(mforms::SelectorItem));
    _projection_picker->set_selector_items(projection_types);

    scoped_connect(_projection_picker->signal_activated(),boost::bind(&SpatialDataView::projection_item_activated, this, _1));

    _toolbar->add_item(_projection_picker);

    _toolbar->add_separator_item();

    item = mforms::manage(new mforms::ToolBarItem(mforms::LabelItem));
    item->set_text("Zoom:");
    _toolbar->add_item(item);

    item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    item->set_icon(mforms::App::get()->get_resource_path("navigator_zoom_out.png"));
    item->signal_activated()->connect(boost::bind(&SpatialDrawBox::zoom_in, _viewer));
    _toolbar->add_item(item);

    item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    item->set_icon(mforms::App::get()->get_resource_path("navigator_zoom_in.png")); //XXX need @2x icons
    item->signal_activated()->connect(boost::bind(&SpatialDrawBox::zoom_out, _viewer));
    _toolbar->add_item(item);

    _toolbar->add_separator_item();
    item = mforms::manage(new mforms::ToolBarItem(mforms::LabelItem));
    item->set_text("External Data:");
    _toolbar->add_item(item);

    item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    item->set_icon(mforms::App::get()->get_resource_path("tiny_open.png"));
    _toolbar->add_item(item);
  }
  add(_toolbar, false, true);

  _main_box->add(_viewer, true, true);

  _option_box = mforms::manage(new mforms::Box(false));
  _option_box->set_spacing(4);
  _option_box->set_padding(8);

#ifdef __APPLE__
  _option_box->set_back_color("#f0f0f0");
#endif

  _layer_menu = new mforms::ContextMenu();
  _layer_menu->add_item_with_title("Set Color...", boost::bind(&SpatialDataView::activate, this));
  _layer_menu->add_item_with_title("Properties...", boost::bind(&SpatialDataView::activate, this));

  _layer_tree = mforms::manage(new mforms::TreeNodeView(mforms::TreeFlatList));
  _layer_tree->add_column(mforms::CheckColumnType, "", 25, true);
  _layer_tree->add_column(mforms::IconStringColumnType, "Layer", 150, false);
  _layer_tree->end_columns();
  _layer_tree->set_cell_edit_handler(boost::bind(&SpatialDataView::tree_toggled, this, _1, _3));
  _layer_tree->set_context_menu(_layer_menu);
  _option_box->add(_layer_tree, true, true);

  _option_box->set_size(200, -1);
  _main_box->add(_option_box, false, true);

  add(_main_box, true, true);
}

void SpatialDataView::projection_item_activated(mforms::ToolBarItem *item)
{
  std::string action = item->get_text();
  if (action == "Mercator")
  {
    _viewer->set_projection(GIS::ProjMercator);

    fprintf(stderr, "Set 0\n");
  }
  else if(action == "Equirectangular")
  {
    _viewer->set_projection(GIS::ProjEquirectangular);
    fprintf(stderr, "Set 1\n");
  }
  else if(action == "Robinson")
  {
    _viewer->set_projection(GIS::ProjRobinson);
    fprintf(stderr, "Set 2\n");
  }
}

SpatialDataView::~SpatialDataView()
{
  delete _layer_menu;
}


void SpatialDataView::activate()
{
}


void SpatialDataView::show_column_data(const SpatialDataView::SpatialDataSource &source, bool show)
{
  Recordset::Ref rset(_owner->recordset());

  for (ssize_t c = rset->row_count(), row = 0; row < c; row++)
  {
    std::string geom_data; // data in MySQL internal binary geometry format.. this is neither WKT nor WKB
    // but the internal format seems to be 4 bytes of SRID followed by WKB data
    if (rset->get_raw_field(row, source.column_index, geom_data) && !geom_data.empty())
    {
      if (show)
        _viewer->add_layer_with_data(geom_data, source.color);
//      else
//        _viewer->remove_layer();

      g_message("--> [%i,%i] %s (%i)\n", (int)row, source.column_index, geom_data.c_str(), (int)geom_data.size());
    }
  }
}


void SpatialDataView::set_color_icon(mforms::TreeNodeRef node, int column, const base::Color &color)
{
  static std::string path;
  if (path.empty())
  {
    path = mforms::Utilities::get_special_folder(mforms::ApplicationData) + "/tmpicons";
    base::create_directory(path, 0700);
  }
  std::string p = path + "/" + base::strfmt("%02x%02x%02x.png", (unsigned char)color.red*255, (unsigned char)color.green*255, (unsigned char)color.blue*255);

  if (!base::file_exists(p))
  {
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


void SpatialDataView::tree_toggled(const mforms::TreeNodeRef &node, const std::string &value)
{
  bool show = value == "1";
  node->set_bool(0, show);

  show_column_data(_sources[_layer_tree->row_for_node(node)], show);
}


void SpatialDataView::set_geometry_columns(const std::vector<SpatialDataSource> &columns)
{
  static base::Color layer_colors[] = {
    base::Color(0.4, 1,   1),
    base::Color(1,   0.4, 0.4),
    base::Color(1,   1,   0.4),
    base::Color(0.4, 1,   0.4)
  };

  _sources = columns;

  bool first = true;
  int i = 0;
  for (std::vector<SpatialDataSource>::iterator iter = _sources.begin(); iter != _sources.end(); ++iter, ++i)
  {
    mforms::TreeNodeRef node = _layer_tree->add_node();
    node->set_string(1, iter->column);
    set_color_icon(node, 1, layer_colors[i]);
    if (i >= sizeof(layer_colors) / sizeof(base::Color))
      i = sizeof(layer_colors) / sizeof(base::Color) - 1;
    node->set_bool(0, first);
    first = false;

    iter->color = layer_colors[i];
  }
  tree_toggled(_layer_tree->node_at_row(0), "1");

  // standard background layer
  mforms::TreeNodeRef node = _layer_tree->add_node();
  node->set_string(1, "World Map");
  set_color_icon(node, 1, base::Color(1, 0, 1));
  node->set_bool(0, true);
//  tree_toggled(node, "1");
}


