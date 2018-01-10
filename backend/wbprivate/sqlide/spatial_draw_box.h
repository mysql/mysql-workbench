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

#include "mforms/drawbox.h"
#include "base/threading.h"
#include <deque>
#include <stack>

#include "mdc.h"
#include "grt/spatial_handler.h"

namespace mforms {
  class ContextMenu;
};

class ProgressPanel;

class SpatialDrawBox : public mforms::DrawBox {
  base::Mutex _layer_mutex;
  spatial::Layer *_background_layer;
  std::deque<spatial::Layer *> _layers;
  spatial::LayerId _last_autozoom_layer;
  spatial::ProjectionType _proj;
  std::shared_ptr<mdc::Surface> _cache;
  mdc::CairoCtx *_ctx_cache;
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

  std::pair<double, double> _clicked_coordinates;
  base::Point _right_clicked_point;

  struct Pin {
    double lat, lon;
    cairo_surface_t *icon;

    Pin(const Pin &other) : lat(other.lat), lon(other.lon), icon(other.icon) {
      cairo_surface_reference(icon);
    }
    Pin(double lat_, double lon_, cairo_surface_t *i) : lat(lat_), lon(lon_), icon(i) {
    }

    ~Pin() {
      if (icon)
        cairo_surface_destroy(icon);
    }
  };
  std::vector<Pin> _pins;

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

  spatial::Layer *_current_layer;
  int _current_layer_index;

  GThread *_renderThread;

  static void *do_render_layers(void *data);

  void render_in_thread(bool reproject);

  void *render_done();

  void render(bool reproject = false);
  bool get_progress(std::string &action, float &pct);

  void restrict_displayed_area(int x1, int y1, int x2, int y2, bool no_invalidate = false);

public:
  std::function<void(mforms::View *, bool reprojecting)> work_started;
  std::function<void(mforms::View *)> work_finished;
  std::function<int(const char *, int)> get_option;
  std::function<void()> area_selected;

public:
  SpatialDrawBox();
  ~SpatialDrawBox();

  std::function<void(base::Point)> position_changed_cb;
  std::function<void(base::Point)> position_clicked_cb;

  void set_context_menu(mforms::ContextMenu *menu);
  std::pair<double, double> clicked_coordinates() {
    return _clicked_coordinates;
  }
  int clicked_row_id();

  std::deque<spatial::Layer *> get_layers() {
    return _layers;
  }

  void set_projection(spatial::ProjectionType proj);

  void reset_view();
  void zoom_out();
  void zoom_in();
  void auto_zoom(const spatial::LayerId layer_idx = 0);
  void select_area(bool flag);

  void center_on(double lat, double lon);

  void clear();
  void set_background(spatial::Layer *layer);
  spatial::Layer *get_background() {
    return _background_layer;
  }

  void add_layer(spatial::Layer *layer);
  void remove_layer(spatial::Layer *layer);
  void change_layer_order(const std::vector<spatial::LayerId> &order);

  spatial::Layer *get_layer(spatial::LayerId layer_id);

  void show_layer(spatial::LayerId layer_id, bool flag);

  void activate();

  void invalidate(bool reproject = false);

  virtual bool mouse_double_click(mforms::MouseButton button, int x, int y);
  virtual bool mouse_down(mforms::MouseButton button, int x, int y);
  virtual bool mouse_up(mforms::MouseButton button, int x, int y);
  virtual bool mouse_move(mforms::MouseButton button, int x, int y);
  virtual void repaint(cairo_t *crt, int x, int y, int w, int h);

  base::Point offset() {
    return base::Point(_offset_x, _offset_y);
  }

  bool screen_to_world(const int &x, const int &y, double &lat, double &lon);
  void world_to_screen(const double &lat, const double &lon, int &x, int &y);

  base::Point apply_cairo_transformation(const base::Point &p) const;
  base::Point unapply_cairo_transformation(const base::Point &p) const;
  void clear_pins();
  void place_pin(cairo_surface_t *pin, const base::Point &p);
  void save_to_png(const std::string &destination);
};
