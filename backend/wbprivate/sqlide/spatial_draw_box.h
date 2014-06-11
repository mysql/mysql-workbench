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

#ifndef _SPATIAL_DRAW_BOX_H_
#define _SPATIAL_DRAW_BOX_H_

#include "mforms/drawbox.h"
#include "base/threading.h"
#include <deque>

#include "mdc.h"
#include "spatial_handler.h"

class ProgressPanel;

class SpatialDrawBox : public mforms::DrawBox
{
  base::Mutex _layer_mutex;
  spatial::Layer *_background_layer;
  std::deque<spatial::Layer*> _layers;
  spatial::ProjectionType _proj;
  boost::shared_ptr<mdc::Surface> _cache;
  base::Mutex _thread_mutex;
  spatial::Converter *_spatial_reprojector;

  ProgressPanel *_progress;

  double _zoom_level;
  int _offset_x, _offset_y;
  
  int _initial_offset_x, _initial_offset_y;
  int _drag_x, _drag_y;

  bool _ready;
  bool _dragging;

  bool _rendering;
  bool _quitting;
  bool _needs_reprojection;

  base::Mutex _progress_mutex;
  std::string _current_work;

  spatial::Layer* _current_layer;
  int _current_layer_index;

  static void *do_render_layers(void *data);

  void render_in_thread(bool reproject);

  void *render_done();

  void render(bool reproject = false);
  bool get_progress(std::string &action, float &pct);

public:
  boost::function<void (mforms::View*, bool reprojecting)> work_started;
  boost::function<void (mforms::View*)> work_finished;

public:
  SpatialDrawBox();
  ~SpatialDrawBox();

  std::deque<spatial::Layer*> get_layers() { return _layers; }
  
  void set_projection(spatial::ProjectionType proj);

  void reset_view();
  void zoom_out();
  void zoom_in();

  void clear();
  void set_background(spatial::Layer *layer);
  void add_layer(spatial::Layer *layer);
  void remove_layer(spatial::Layer *layer);

  void show_layer(int layer_id, bool flag);

  void activate();

  void invalidate(bool reproject = false);

  virtual bool mouse_double_click(mforms::MouseButton button, int x, int y);
  virtual bool mouse_down(mforms::MouseButton button, int x, int y);
  virtual bool mouse_up(mforms::MouseButton button, int x, int y);
  virtual bool mouse_move(mforms::MouseButton button, int x, int y);
  virtual void repaint(cairo_t *crt, int x, int y, int w, int h);

  void screen_to_world(int x, int y, double &lat, double &lon);
  void world_to_screen(double lat, double lon, int &x, int &y);
};


#endif
