/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "mforms/drawbox.h"
#include "base/threading.h"
#include <deque>
#include <stack>

#include "mdc.h"
#include "spatial_handler.h"

namespace mforms
{
  class ContextMenu;
};

class ProgressPanel;

class SpatialDrawBox : public mforms::DrawBox
{
  base::Mutex _layer_mutex;
  spatial::Layer *_background_layer;
  std::deque<spatial::Layer*> _layers;
  size_t _last_autozoom_layer;
  spatial::ProjectionType _proj;
  boost::shared_ptr<mdc::Surface> _cache;
  base::Mutex _thread_mutex;
  spatial::Converter *_spatial_reprojector;

  ProgressPanel *_progress;

  mforms::ContextMenu *_menu;

  std::stack<spatial::Envelope> _hw_zoom_history;
  float _zoom_level;
  int _offset_x, _offset_y;
  
  int _initial_offset_x, _initial_offset_y;
  int _drag_x, _drag_y;
  int _select_x, _select_y;

  std::pair<double,double> _clicked_coordinates;
  base::Point _right_clicked_point;

  double _min_lat, _max_lat;
  double _min_lon, _max_lon;

  bool _ready;
  bool _dragging;

  bool _rendering;
  bool _quitting;
  bool _needs_reprojection;
  bool _select_pending;
  bool _selecting;
  bool _displaying_restricted;

  base::Mutex _progress_mutex;
  std::string _current_work;

  spatial::Layer* _current_layer;
  int _current_layer_index;

  static void *do_render_layers(void *data);

  void render_in_thread(bool reproject);

  void *render_done();

  void render(bool reproject = false);
  bool get_progress(std::string &action, float &pct);

  void restrict_displayed_area(int x1, int y1, int x2, int y2, bool no_invalidate = false);

public:
  boost::function<void (mforms::View*, bool reprojecting)> work_started;
  boost::function<void (mforms::View*)> work_finished;
  boost::function<int (const char*, int)> get_option;

public:
  SpatialDrawBox();
  ~SpatialDrawBox();

  boost::function<void (const std::string&,const std::string&)> position_changed_cb;

  void set_context_menu(mforms::ContextMenu *menu);
  std::pair<double,double> clicked_coordinates() { return _clicked_coordinates; }
  int clicked_row_id();

  std::deque<spatial::Layer*> get_layers() { return _layers; }
  
  void set_projection(spatial::ProjectionType proj);

  void reset_view();
  void zoom_out();
  void zoom_in();
  void auto_zoom(const size_t layer_idx = (size_t)-1); //by default we set it to max, cause 0 can be also idx
  void select_area();

  void center_on(double lat, double lon);

  void clear();
  void set_background(spatial::Layer *layer);
  void add_layer(spatial::Layer *layer);
  void remove_layer(spatial::Layer *layer);

  void show_layer(int layer_id, bool flag);
  void fillup_polygon(int layer_id, bool flag);

  void activate();

  void invalidate(bool reproject = false);

  virtual bool mouse_double_click(mforms::MouseButton button, int x, int y);
  virtual bool mouse_down(mforms::MouseButton button, int x, int y);
  virtual bool mouse_up(mforms::MouseButton button, int x, int y);
  virtual bool mouse_move(mforms::MouseButton button, int x, int y);
  virtual void repaint(cairo_t *crt, int x, int y, int w, int h);

  bool screen_to_world(int x, int y, double &lat, double &lon);
  void world_to_screen(double lat, double lon, int &x, int &y);
};
