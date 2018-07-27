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

#include <cairo/cairo.h>

#include "spatial_draw_box.h"
#include "mforms/box.h"
#include "mforms/label.h"
#include "mforms/panel.h"
#include "mforms/menubar.h"
#include "mforms/progressbar.h"

#include "base/log.h"

DEFAULT_LOG_DOMAIN("spatial_draw_box");

class ProgressPanel : public mforms::Box {
public:
  ProgressPanel(const std::string &title) : mforms::Box(false), _timer(0) {
    set_back_color("#eeeeee");
    set_padding(32);
    set_spacing(8);

    _title.set_text(title);
    _title.set_style(mforms::BoldStyle);
    add(&_title, false, true);
    add(&_label, false, true);
    add(&_progress, false, true);
  }

  ~ProgressPanel() {
    if (_timer)
      mforms::Utilities::cancel_timeout(_timer);
  }

  void set_progress(const std::string &doing_what, float pct) {
    _label.set_text(doing_what);
    _progress.set_value(pct);
  }

  void start(std::function<bool(std::string &, float &)> progress_fetcher, float interval) {
    _progress_fetcher = progress_fetcher;
    _timer = mforms::Utilities::add_timeout(interval, std::bind(&ProgressPanel::update, this));
  }

  void stop() {
    mforms::Utilities::cancel_timeout(_timer);
    _timer = 0;
  }

private:
  mforms::TimeoutHandle _timer;
  mforms::Label _title;
  mforms::Label _label;
  mforms::ProgressBar _progress;
  std::function<bool(std::string &, float &)> _progress_fetcher;

  bool update() {
    std::string what;
    float pct;
    if (_progress_fetcher(what, pct)) {
      _label.set_text(what);
      _progress.set_value(pct);
    }

    return true;
  }
};

void *SpatialDrawBox::do_render_layers(void *data) {
  SpatialDrawBox *self = (SpatialDrawBox *)data;
  {
    base::MutexLock lock(self->_thread_mutex);
    self->render(self->_needs_reprojection);
    if (!self->_quitting)
      mforms::Utilities::perform_from_main_thread(std::bind(&SpatialDrawBox::render_done, self));
    else
      delete self->_progress;
  }

  return NULL;
}

void SpatialDrawBox::render_in_thread(bool reproject) { 
  if (_renderThread != nullptr) {
    logDebug3("Render thread didn't finish yet, waiting.\n");
    g_thread_join(_renderThread);
    _renderThread = nullptr;
  }

  _needs_reprojection = reproject;
  if (!_rendering && !_layers.empty()) {
    _current_layer = NULL;
    _rendering = true;
    _progress = new ProgressPanel("Rendering spatial data, please wait.");
    _progress->start(std::bind(&SpatialDrawBox::get_progress, this, std::placeholders::_1, std::placeholders::_2),
                     0.2f);
    _renderThread = base::create_thread(do_render_layers, this);
    work_started(_progress, reproject);

    set_needs_repaint();
  }
}

void *SpatialDrawBox::render_done() {
  _progress->stop();

  _rendering = false;

  work_finished(_progress);
  delete _progress;
  _progress = NULL;

  set_needs_repaint();

  return NULL;
}

void SpatialDrawBox::render(bool reproject) {
  int width = get_width();
  int height = get_height();

  spatial::ProjectionView visible_area;

  visible_area.MaxLat = _max_lat;
  visible_area.MaxLon = _max_lon;
  visible_area.MinLat = _min_lat;
  visible_area.MinLon = _min_lon;
  // we need to make a fix cause some projections will fail

  if (_proj == spatial::ProjBonne) {
    if (visible_area.MaxLat > 154.0)
      visible_area.MaxLat = 154.0;
    if (visible_area.MaxLon > 64.0)
      visible_area.MaxLon = 64.0;
    if (visible_area.MinLat < -154.0)
      visible_area.MinLat = -154.0;
    if (visible_area.MinLon < -64.0)
      visible_area.MinLon = -64.0;
  } else {
    if (visible_area.MaxLat > 179.0)
      visible_area.MaxLat = 179.0;
    if (visible_area.MaxLon > 89.0)
      visible_area.MaxLon = 89.0;
    if (visible_area.MinLat < -179.0)
      visible_area.MinLat = -179.0;
    if (visible_area.MinLon < -89.0)
      visible_area.MinLon = -89.0;
  }

  visible_area.height = height;
  visible_area.width = width;

  try {
    if (_spatial_reprojector == NULL)
      _spatial_reprojector =
        new spatial::Converter(visible_area, spatial::Projection::get_instance().get_projection(spatial::ProjGeodetic),
                               spatial::Projection::get_instance().get_projection(_proj));
  } catch (std::exception &exc) {
    logError("SpatialDrawBox::render: %s\n", exc.what());
    return;
  }

  _spatial_reprojector->change_projection(visible_area, NULL,
                                          spatial::Projection::get_instance().get_projection(_proj));

  // TODO lat/long ranges must be adjusted accordingly to account for the aspect ratio of the visible area
  std::shared_ptr<mdc::ImageSurface> surface(new mdc::ImageSurface(get_width(), get_height(), CAIRO_FORMAT_ARGB32));
  _cache = surface;

  if (_ctx_cache != NULL)
    delete _ctx_cache;
  _ctx_cache = new mdc::CairoCtx(*surface);

  _current_work = "Rendering layers...";
  _current_layer = NULL;
  _current_layer_index = 0;

  if (_zoom_level != 1) {
    _ctx_cache->translate(base::Point(this->get_width() / 2.0, this->get_height() / 2.0));
    _ctx_cache->scale(base::Point(_zoom_level, _zoom_level));
    _ctx_cache->translate(base::Point(-this->get_width() / 2.0, -this->get_height() / 2.0));
  }

  _ctx_cache->translate(base::Point(_offset_x, _offset_y));

  _ctx_cache->set_line_width(0);

  if (reproject && !_background_layer->hidden())
    _background_layer->render(_spatial_reprojector);

  int i = 0;

  base::MutexLock lock(_layer_mutex);
  for (std::deque<spatial::Layer *>::iterator it = _layers.begin(); it != _layers.end() && !_quitting; ++it, ++i) {
    _current_work = base::strfmt("Rendering %i objects in layer %i...", (int)(*it)->size(), i + 1);

    _current_layer_index = i;
    _current_layer = *it;
    if (!(*it)->hidden()) {
      if (reproject)
        (*it)->render(_spatial_reprojector);
      (*it)->repaint(*_ctx_cache, _zoom_level, base::Rect());
    }
  }

  if (reproject)
    _needs_reprojection = false;
}

bool SpatialDrawBox::get_progress(std::string &action, float &pct) {
  bool changed = false;
  _progress_mutex.lock();
  float current_progress = (float)_current_layer_index / _layers.size();
  if (_current_layer)
    current_progress += (1.0f / _layers.size()) * _current_layer->query_render_progress();

  if (pct != current_progress || action != _current_work) {
    changed = true;
    action = _current_work;
    pct = current_progress;
  }
  _progress_mutex.unlock();
  return changed;
}

SpatialDrawBox::SpatialDrawBox()
  : _background_layer(NULL),
    _last_autozoom_layer(0),
    _proj(spatial::ProjRobinson),
    _ctx_cache(NULL),
    _spatial_reprojector(NULL),
    _zoom_level(1.0),
    _offset_x(0),
    _offset_y(0),
    _ready(false),
    _dragging(false),
    _rendering(false),
    _quitting(false),
    _needs_reprojection(true),
    _select_pending(false),
    _selecting(false) {
  _displaying_restricted = false;
  _min_lat = -179;
  _max_lat = 179;
  _min_lon = -89;
  _max_lon = 89;

  _current_layer = NULL;
  _progress = NULL;
  _renderThread = nullptr;
}

SpatialDrawBox::~SpatialDrawBox() {
  _quitting = true;
  if (_renderThread != nullptr) {
    logDebug3("Waiting for render thread to finish.\n");
    g_thread_join(_renderThread);
    _renderThread = nullptr;
  }
  clear();
  // lock the mutex, so that if the worker is still busy, we'll wait for it

  base::MutexLock lock(_thread_mutex);
  delete _ctx_cache;
  _ctx_cache = NULL;
}

void SpatialDrawBox::set_projection(spatial::ProjectionType proj) {
  if (_spatial_reprojector)
    _spatial_reprojector->change_projection(NULL, spatial::Projection::get_instance().get_projection(proj));

  _proj = proj;
  invalidate(true);
}

void SpatialDrawBox::zoom_out() {
  _zoom_level -= 0.2f;
  if (_zoom_level < 1.0)
    _zoom_level = 1.0;
  bool reproject = false;
  if (_zoom_level == 1.0 && !_hw_zoom_history.empty()) {
    spatial::Envelope env = _hw_zoom_history.top();
    _hw_zoom_history.pop();
    _min_lat = env.top_left.x;
    _max_lat = env.bottom_right.x;
    _min_lon = env.bottom_right.y;
    _max_lon = env.top_left.y;
    _offset_x = 0;
    _offset_y = 0;
    reproject = true;
  }
  invalidate(reproject);
}

void SpatialDrawBox::zoom_in() {
  _zoom_level += 0.2f;
  invalidate();
}

void SpatialDrawBox::auto_zoom(spatial::LayerId layer_id) {
  if (_layers.empty())
    return;

  _last_autozoom_layer = layer_id;

  spatial::Layer *lay = get_layer(layer_id);
  if (lay == NULL)
    return;

  spatial::Envelope env = lay->get_envelope();
  if (!env.is_init())
    return;

  double h = fabs(env.top_left.y - env.bottom_right.y);
  double w = fabs(env.top_left.x - env.bottom_right.x);

  const double ratio = 2.011235955; // taken from (179 *2) / (89*2) world boundaries

  if (h > w) {
    env.bottom_right.x = env.top_left.x + (h * ratio);
    if (env.bottom_right.x > 180 || env.bottom_right.x < -180) {
      env.bottom_right.x = 180;
      env.top_left.x = env.bottom_right.x - (h * ratio);
    }
    if (env.top_left.x > 180 || env.top_left.y < -180) {
      env.top_left.x = -180;
      env.bottom_right.x = env.top_left.x + (h * ratio);
    }
  } else {
    env.bottom_right.y = env.top_left.y - (w / ratio);
    if (env.bottom_right.y < -90 || env.bottom_right.y > 90) {
      env.bottom_right.y = -90;
      env.top_left.y = env.bottom_right.y + (w * ratio);
    }
    if (env.top_left.y < -90 || env.top_left.y > 90) {
      env.top_left.y = 90;
      env.bottom_right.y = env.top_left.y - (w * ratio);
    }
  }
  _min_lat = env.top_left.x;
  _max_lat = env.bottom_right.x;
  _min_lon = env.bottom_right.y;
  _max_lon = env.top_left.y;
  _displaying_restricted = true;
}

void SpatialDrawBox::center_on(double lat, double lon) {
  // XXX
  invalidate();
}

void SpatialDrawBox::reset_view() {
  clear_pins();

  _min_lat = -179;
  _max_lat = 179;
  _min_lon = -89;
  _max_lon = 89;

  _zoom_level = 1;
  _offset_x = 0;
  _offset_y = 0;

  while (!_hw_zoom_history.empty())
    _hw_zoom_history.pop();

  invalidate(_displaying_restricted);
  _displaying_restricted = false;
}

void SpatialDrawBox::select_area(bool flag) {
  if (flag)
    mforms::App::get()->set_status_text("Click and drag to select an area to display.");
  else
    mforms::App::get()->set_status_text("");
  _select_pending = flag;
}

void SpatialDrawBox::clear() {
  delete _background_layer;
  _background_layer = NULL;

  for (std::deque<spatial::Layer *>::iterator i = _layers.begin(); i != _layers.end(); ++i)
    (*i)->interrupt();

  base::MutexLock lock(_layer_mutex);
  for (std::deque<spatial::Layer *>::iterator i = _layers.begin(); i != _layers.end(); ++i)
    delete *i;
  _layers.clear();
  if (_spatial_reprojector) {
    _spatial_reprojector->interrupt();
    delete _spatial_reprojector;
    _spatial_reprojector = NULL;
  }
}

void SpatialDrawBox::set_background(spatial::Layer *layer) {
  if (_background_layer)
    delete _background_layer;
  _background_layer = layer;
}

void SpatialDrawBox::set_context_menu(mforms::ContextMenu *menu) {
  _menu = menu;
}

void SpatialDrawBox::add_layer(spatial::Layer *layer) {
  base::MutexLock lock(_layer_mutex);
  layer->set_fill_polygons(get_option("SqlEditor::FillUpPolygons", 1) >= 1);
  _layers.push_back(layer);
}

void SpatialDrawBox::remove_layer(spatial::Layer *layer) {
  base::MutexLock lock(_layer_mutex);
  layer->interrupt();
  std::deque<spatial::Layer *>::iterator l = std::find(_layers.begin(), _layers.end(), layer);
  if (l != _layers.end())
    _layers.erase(l);
}

void SpatialDrawBox::change_layer_order(const std::vector<spatial::LayerId> &order) {
  base::MutexLock lock(_layer_mutex);
  std::map<spatial::LayerId, spatial::Layer *> layers;
  for (std::deque<spatial::Layer *>::iterator it = _layers.begin(); it != _layers.end(); ++it)
    layers[(*it)->layer_id()] = *it;

  _layers.clear();
  std::map<spatial::LayerId, spatial::Layer *>::iterator it;
  for (size_t i = 0; i < order.size(); ++i) {
    it = layers.find(order[i]);
    if (it != layers.end())
      _layers.push_back(it->second);
  }
}

spatial::Layer *SpatialDrawBox::get_layer(spatial::LayerId layer_id) {
  base::MutexLock lock(_layer_mutex);
  for (std::deque<spatial::Layer *>::iterator it = _layers.begin(); it != _layers.end(); ++it) {
    if ((*it)->layer_id() == layer_id) {
      return *it;
    }
  }
  if (_background_layer && layer_id == _background_layer->layer_id())
    return _background_layer;
  return NULL;
}

void SpatialDrawBox::show_layer(spatial::LayerId layer_id, bool flag) {
  if (layer_id == 1 && _background_layer) {
    _background_layer->set_show(flag);
    invalidate(true);
  } else {
    base::MutexLock lock(_layer_mutex);
    for (std::deque<spatial::Layer *>::iterator i = _layers.begin(); i != _layers.end(); ++i)
      if ((*i)->layer_id() == layer_id) {
        (*i)->set_show(flag);
        invalidate(true);
        return;
      }
  }
}

void SpatialDrawBox::activate() {
  if (!_ready) {
    _ready = true;
    invalidate(true);
  }
}

void SpatialDrawBox::invalidate(bool reproject) {
  if (_ready)
    render_in_thread(reproject);
  set_needs_repaint(); // repaint the grid
}

bool SpatialDrawBox::mouse_double_click(mforms::MouseButton button, int x, int y) {
  int dx, dy;
  dx = this->get_width() / 2;
  dy = this->get_height() / 2;
  _offset_x = (int)(_initial_offset_x - (x - dx) / _zoom_level);
  _offset_y = (int)(_initial_offset_y - (y - dy) / _zoom_level);
  _dragging = false;
  zoom_in();

  return false;
}

bool SpatialDrawBox::mouse_down(mforms::MouseButton button, int x, int y) {
  if (button == mforms::MouseButtonLeft) {
    _initial_offset_x = _offset_x;
    _initial_offset_y = _offset_y;
    _drag_x = x;
    _drag_y = y;
    if (_select_pending || _selecting) {
      _selecting = true;
      //      _select_pending = false;
    } else
      _dragging = true;
  } else if (button == mforms::MouseButtonRight) {
    double lat = 0, lon = 0;
    screen_to_world(x, y, lat, lon);
    _clicked_coordinates = std::make_pair(lat, lon);
    _right_clicked_point = base::Point(x, y);

    if (_menu) {
      std::pair<int, int> p = client_to_screen(x, y);
      _menu->popup_at(this, base::Point(p.first, p.second));
    }
  }
  return true;
}

bool SpatialDrawBox::mouse_up(mforms::MouseButton button, int x, int y) {
  if (button == mforms::MouseButtonLeft && _dragging) {
    if (_drag_x == x && _drag_y == y) {
      // handle feature click
      if (position_clicked_cb)
        position_clicked_cb(base::Point(x, y));
    } else {
      mouse_move(button, x, y);
      invalidate();
    }
    _dragging = false;
  } else if (button == mforms::MouseButtonLeft && _selecting) {
    restrict_displayed_area(_drag_x, _drag_y, x, y);
    _selecting = false;
    set_needs_repaint();
    mforms::App::get()->set_status_text("");

    if (area_selected)
      area_selected();
  }
  return true;
}

bool SpatialDrawBox::mouse_move(mforms::MouseButton button, int x, int y) {
  if (_dragging) {
    _offset_x = (int)(_initial_offset_x + (x - _drag_x) / _zoom_level);
    _offset_y = (int)(_initial_offset_y + (y - _drag_y) / _zoom_level);
    set_needs_repaint();
  } else if (_selecting) {
    _select_x = x;
    _select_y = y;
    set_needs_repaint();
  }

  position_changed_cb(base::Point(x, y));

  return true;
}

int SpatialDrawBox::clicked_row_id() {
  int row_id = -1;

  base::Point p(_right_clicked_point.x - _offset_x, _right_clicked_point.y - _offset_y);
  base::MutexLock lock(_layer_mutex);
  for (std::deque<spatial::Layer *>::iterator it = _layers.begin(); it != _layers.end(); ++it) {
    spatial::Feature *feature;
    feature = (*it)->feature_closest(p);
    if (feature) {
      row_id = feature->row_id();
      break;
    }
  }

  return row_id;
}

void SpatialDrawBox::restrict_displayed_area(int x1, int y1, int x2, int y2, bool no_invalidate) {
  double lat1, lat2;
  double lon1, lon2;

  if (x1 > x2)
    std::swap(x1, x2);
  if (y1 > y2)
    std::swap(y1, y2);

  if (screen_to_world(x1, y1, lat1, lon1) && screen_to_world(x2, y2, lat2, lon2)) {
    double h = fabs(lat2 - lat1);

    double ratio = 2.011235955; // taken from (179 *2) / (89*2) world boundaries
    lon2 = lon1 + h * ratio;

    _zoom_level = 1.0;
    _offset_x = 0;
    _offset_y = 0;

    spatial::Envelope env;
    env.top_left.x = _min_lat;
    env.bottom_right.x = _max_lat;
    env.bottom_right.y = _min_lon;
    env.top_left.y = _max_lon;
    _hw_zoom_history.push(env);

    _min_lat = lon1;
    _max_lat = lon2;
    _min_lon = lat2;
    _max_lon = lat1;

    _displaying_restricted = true;
    if (!no_invalidate)
      invalidate(true);
  }
}

void SpatialDrawBox::repaint(cairo_t *crt, int x, int y, int w, int h) {
  std::shared_ptr<mdc::Surface> cache(_cache);
  mdc::CairoCtx cr(crt);
  if (cache) {
    cr.set_color(_background_layer && _background_layer->fill() ? _background_layer->color() : base::Color(1, 1, 1));
    cr.paint();
    cr.set_source_surface(cache->get_surface(), 0, 0);
    if (_rendering) // if we're currently re-rendering the image, we paint the old version half transparent
      cr.paint_with_alpha(0.4);
    else
      cr.paint();
  } else if (!_progress) {
    cr.set_color(_background_layer && _background_layer->fill() ? _background_layer->color() : base::Color(1, 1, 1));
    cr.paint();
  }

  if (_background_layer && !_background_layer->hidden()) {
    cr.save();
    if (_zoom_level != 1) {
      cr.translate(base::Point(this->get_width() / 2, this->get_height() / 2));
      cr.scale(base::Point(_zoom_level, _zoom_level));
      cr.translate(base::Point(-this->get_width() / 2, -this->get_height() / 2));
    }

    cr.translate(base::Point(_offset_x, _offset_y));

    cr.set_line_width(0);
    _background_layer->repaint(cr, _zoom_level, base::Rect());
    cr.restore();
  }

  if (_rendering) {
    cr.set_color(base::Color(0, 0, 0));
    cr.move_to(base::Point(10, 20));
    cr.show_text("Repainting...");
  } else {
    for (std::vector<Pin>::const_iterator pin = _pins.begin(); pin != _pins.end(); ++pin) {
      int x, y;
      if (pin->icon) {
        world_to_screen(pin->lat, pin->lon, x, y);
        base::Size size = mforms::Utilities::getImageSize(pin->icon);
        mforms::Utilities::paint_icon(cr.get_cr(), pin->icon, x - size.width / 2, y - size.height + 2);
      }
    }
  }

  if (_selecting) {
    cr.set_line_width(2);
    cr.set_color(base::Color(0, 0, 0));
    cr.rectangle(base::Rect(std::min(_drag_x, _select_x), std::min(_drag_y, _select_y), abs(_select_x - _drag_x),
                            abs(_select_y - _drag_y)));
    cr.stroke();
  }
}

bool SpatialDrawBox::screen_to_world(const int &x, const int &y, double &lat, double &lon) {
  if (_spatial_reprojector) {
    //     TODO check if x, y are inside the world image
    //    if (x >= _offset_x && y >= _offset_y) <- this is not working when we do rectangular zoom

    base::Point p = apply_cairo_transformation(base::Point(x, y));
    return _spatial_reprojector->to_latlon((int)p.x, (int)p.y, lat, lon);
  }
  return false;
}

void SpatialDrawBox::world_to_screen(const double &lat, const double &lon, int &x, int &y) {
  if (_spatial_reprojector) {
    _spatial_reprojector->from_latlon(lat, lon, x, y);

    base::Point p = unapply_cairo_transformation(base::Point(x, y));

    x = (int)p.x;
    y = (int)p.y;
  }
}

void SpatialDrawBox::save_to_png(const std::string &destination) {
  std::shared_ptr<mdc::ImageSurface> surface(new mdc::ImageSurface(get_width(), get_height(), CAIRO_FORMAT_ARGB32));
  mdc::CairoCtx ctx(*surface);
  this->repaint(ctx.get_cr(), 0, 0, get_width(), get_height());
  surface->save_to_png(destination);
}

void SpatialDrawBox::clear_pins() {
  _pins.clear();
  set_needs_repaint();
}

base::Point SpatialDrawBox::unapply_cairo_transformation(const base::Point &p) const {
  double xx = p.x, yy = p.y;
  _ctx_cache->user_to_device(&xx, &yy);
  return base::Point(xx, yy);
}

base::Point SpatialDrawBox::apply_cairo_transformation(const base::Point &p) const {
  double xx = p.x, yy = p.y;
  _ctx_cache->device_to_user(&xx, &yy);
  return base::Point(xx, yy);
}

void SpatialDrawBox::place_pin(cairo_surface_t *pin, const base::Point &p) {
  double lat, lon;
  screen_to_world((int)p.x, (int)p.y, lat, lon);
  _pins.push_back(Pin(lat, lon, pin));
  set_needs_repaint();
}
