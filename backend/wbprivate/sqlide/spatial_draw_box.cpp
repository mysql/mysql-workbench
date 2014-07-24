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


#include "spatial_draw_box.h"
#include "mforms/box.h"
#include "mforms/label.h"
#include "mforms/panel.h"
#include "mforms/menubar.h"
#include "mforms/progressbar.h"

#include "base/log.h"

DEFAULT_LOG_DOMAIN("spatial");

class ProgressPanel : public mforms::Box
{
public:
  ProgressPanel(const std::string &title)
  : mforms::Box(false), _timer(0)
  {
    set_back_color("#eeeeee");
    set_padding(32);
    set_spacing(8);

    _title.set_text(title);
    _title.set_style(mforms::BoldStyle);
    add(&_title, false, true);
    add(&_label, false, true);
    add(&_progress, false, true);
  }

  ~ProgressPanel()
  {
    if (_timer)
      mforms::Utilities::cancel_timeout(_timer);
  }

  void set_progress(const std::string &doing_what, float pct)
  {
    _label.set_text(doing_what);
    _progress.set_value(pct);
  }

  void start(boost::function<bool (std::string&, float&)> progress_fetcher, float interval)
  {
    _progress_fetcher = progress_fetcher;
    _timer = mforms::Utilities::add_timeout(interval, boost::bind(&ProgressPanel::update, this));
  }

  void stop()
  {
    mforms::Utilities::cancel_timeout(_timer);
    _timer = 0;
  }

private:
  mforms::TimeoutHandle _timer;
  mforms::Label _title;
  mforms::Label _label;
  mforms::ProgressBar _progress;
  boost::function<bool (std::string&, float&)> _progress_fetcher;

  bool update()
  {
    std::string what;
    float pct;
    if (_progress_fetcher(what, pct))
    {
      _label.set_text(what);
      _progress.set_value(pct);
    }

    return true;
  }
};


void *SpatialDrawBox::do_render_layers(void *data)
{
  SpatialDrawBox *self = (SpatialDrawBox*)data;
  {
    base::MutexLock lock(self->_thread_mutex);
    self->render(self->_needs_reprojection);
    if (!self->_quitting)
      mforms::Utilities::perform_from_main_thread(boost::bind(&SpatialDrawBox::render_done, self));
    else
      delete self->_progress;
  }

  return NULL;
}

void SpatialDrawBox::render_in_thread(bool reproject)
{
  _needs_reprojection = reproject;
  if (!_rendering && !_layers.empty())
  {
    _current_layer = NULL;
    _rendering = true;
    _progress = new ProgressPanel("Rendering spatial data, please wait.");
    _progress->start(boost::bind(&SpatialDrawBox::get_progress, this, _1, _2), 0.2f);
    base::create_thread(do_render_layers, this);
    work_started(_progress, reproject);

    set_needs_repaint();
  }
}

void *SpatialDrawBox::render_done()
{
  _progress->stop();

  _rendering = false;

  work_finished(_progress);
  delete _progress;
  _progress = NULL;

  set_needs_repaint();

  return NULL;
}

void SpatialDrawBox::render(bool reproject)
{
  int width = get_width();
  int height = get_height();

  spatial::ProjectionView visible_area;

  visible_area.MaxLat = _max_lat;
  visible_area.MaxLon = _max_lon;
  visible_area.MinLat = _min_lat;
  visible_area.MinLon = _min_lon;
  //we need to make a fix cause some projections will fail

  if (_proj == spatial::ProjBonne)
  {
    if (visible_area.MaxLat > 154.0)
      visible_area.MaxLat = 154.0;
    if (visible_area.MaxLon > 64.0)
      visible_area.MaxLon = 64.0;
    if (visible_area.MinLat < -154.0)
      visible_area.MinLat = -154.0;
    if (visible_area.MinLon < -64.0)
      visible_area.MinLon = -64.0;
  }
  else
  {
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

  if (_spatial_reprojector == NULL)
    _spatial_reprojector = new spatial::Converter(visible_area,
                              spatial::Projection::get_instance().get_projection(spatial::ProjGeodetic),
                              spatial::Projection::get_instance().get_projection(_proj));

  _spatial_reprojector->change_projection(visible_area, NULL, spatial::Projection::get_instance().get_projection(_proj));




  // TODO lat/long ranges must be adjusted accordingly to account for the aspect ratio of the visible area

  boost::shared_ptr<mdc::ImageSurface> surface(new mdc::ImageSurface(get_width(), get_height(), CAIRO_FORMAT_ARGB32));
  mdc::CairoCtx ctx(*surface);

  _current_work = "Rendering layers...";
  _current_layer = NULL;
  _current_layer_index = 0;

  ctx.translate(base::Point(_offset_x, _offset_y));
  ctx.scale(base::Point(_zoom_level, _zoom_level));
  ctx.set_line_width(0);

  if (reproject && !_background_layer->hidden())
    _background_layer->render(_spatial_reprojector);

  int i = 0;

  base::MutexLock lock(_layer_mutex);
  for (std::deque<spatial::Layer*>::iterator it = _layers.begin(); it != _layers.end() && !_quitting; ++it, ++i)
  {
    _current_work = base::strfmt("Rendering %i objects in layer %i...", (int)(*it)->size(), i+1);

    _current_layer_index = i;
    _current_layer = *it;
    if (!(*it)->hidden())
    {
      if (reproject)
        (*it)->render(_spatial_reprojector);
      (*it)->repaint(ctx, _zoom_level, base::Rect());

    }
  }

  _cache = surface;

  if (reproject)
    _needs_reprojection = false;
}


bool SpatialDrawBox::get_progress(std::string &action, float &pct)
{
  bool changed = false;
  _progress_mutex.lock();
  float current_progress = (float)_current_layer_index / _layers.size();
  if (_current_layer)
    current_progress += (1.0f / _layers.size()) * _current_layer->query_render_progress();

  if (pct != current_progress || action != _current_work)
  {
    changed = true;
    action = _current_work;
    pct = current_progress;
  }
  _progress_mutex.unlock();
  return changed;
}


SpatialDrawBox::SpatialDrawBox()
: _background_layer(NULL), _last_autozoom_layer(-1),
_proj(spatial::ProjRobinson),  _spatial_reprojector(NULL),
_zoom_level(1.0), _offset_x(0), _offset_y(0), _ready(false), _dragging(false),
_rendering(false), _quitting(false), _needs_reprojection(true), _select_pending(false), _selecting(false)
{
  _displaying_restricted = false;
  _min_lat = -179;
  _max_lat = 179;
  _min_lon = -89;
  _max_lon = 89;

  _current_layer = NULL;
  _progress = NULL;

}

SpatialDrawBox::~SpatialDrawBox()
{
  _quitting = true;
  clear();
  // lock the mutex, so that if the worker is still busy, we'll wait for it
  _thread_mutex.lock();
}

void SpatialDrawBox::set_projection(spatial::ProjectionType proj)
{
  if (_spatial_reprojector)
    _spatial_reprojector->change_projection(NULL, spatial::Projection::get_instance().get_projection(proj));

  _proj = proj;
  invalidate(true);
}

void SpatialDrawBox::zoom_out()
{
  _zoom_level -= 0.2f;
  if (_zoom_level < 1.0)
    _zoom_level = 1.0;
  bool reproject = false;
  if (_zoom_level == 1.0 && !_hw_zoom_history.empty())
  {
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

void SpatialDrawBox::zoom_in()
{
  _zoom_level += 0.2f;
  invalidate();
}

void SpatialDrawBox::auto_zoom(const size_t layer_idx)
{
  if (_layers.empty())
    return;

  _last_autozoom_layer = layer_idx;

  spatial::Layer* lay = NULL;
  if (_last_autozoom_layer == (size_t)-1 || _last_autozoom_layer >= _layers.size())
    lay = _layers.back();
  else
  {
    for (std::deque<spatial::Layer*>::iterator it = _layers.begin(); it != _layers.end(); ++it)
    {
      if ((size_t)(*it)->layer_id() == layer_idx)
      {
        lay = *it;
        break;
      }
    }
  }

  if (lay == NULL)
    return;

  spatial::Envelope env = lay->get_envelope();
  if (!env.is_init())
    return;

  double h = std::abs(env.top_left.y - env.bottom_right.y);

  const double ratio = 2.011235955; // taken from (179 *2) / (89*2) world boundaries

  env.bottom_right.x = env.top_left.x + h * ratio;

  _min_lat = env.top_left.x;
  _max_lat = env.bottom_right.x;
  _min_lon = env.bottom_right.y;
  _max_lon = env.top_left.y;
  _displaying_restricted = true;
}


void SpatialDrawBox::center_on(double lat, double lon)
{
  //XXX
  invalidate();
}


void SpatialDrawBox::reset_view()
{
  _min_lat = -179;
  _max_lat = 179;
  _min_lon = -89;
  _max_lon = 89;

  _zoom_level = 1;
  _offset_x = 0;
  _offset_y = 0;

  while(!_hw_zoom_history.empty())
    _hw_zoom_history.pop();

  auto_zoom(_last_autozoom_layer);

  invalidate(_displaying_restricted);
  _displaying_restricted = false;
}



void SpatialDrawBox::select_area()
{
  mforms::App::get()->set_status_text("Click and drag to select an area to display.");
  _select_pending = true;
}


void SpatialDrawBox::clear()
{
  delete _background_layer;
  _background_layer = NULL;

  for (std::deque<spatial::Layer*>::iterator i = _layers.begin(); i != _layers.end(); ++i)
    (*i)->interrupt();

  base::MutexLock lock(_layer_mutex);
  for (std::deque<spatial::Layer*>::iterator i = _layers.begin(); i != _layers.end(); ++i)
    delete *i;
  _layers.clear();
  if (_spatial_reprojector)
  {
    _spatial_reprojector->interrupt();
    delete _spatial_reprojector;
    _spatial_reprojector = NULL;
  }
}

void SpatialDrawBox::set_background(spatial::Layer *layer)
{
  if (_background_layer)
    delete _background_layer;
  _background_layer = layer;
}

void SpatialDrawBox::set_context_menu(mforms::ContextMenu *menu)
{
  _menu = menu;
}

void SpatialDrawBox::add_layer(spatial::Layer *layer)
{
  {

    base::MutexLock lock(_layer_mutex);
    layer->set_fill_polygons((bool)get_option("SqlEditor::FillUpPolygons", 1));
    _layers.push_back(layer);
  }
}

void SpatialDrawBox::remove_layer(spatial::Layer *layer)
{
  {
    base::MutexLock lock(_layer_mutex);
    layer->interrupt();
    std::deque<spatial::Layer*>::iterator l = std::find(_layers.begin(), _layers.end(), layer);
    if (l != _layers.end())
      _layers.erase(l);
  }
}

void SpatialDrawBox::show_layer(int layer_id, bool flag)
{
  if (layer_id == 0 && _background_layer)
  {
    _background_layer->set_show(flag);
    invalidate(true);
  }
  else
  {
    base::MutexLock lock(_layer_mutex);
    for (std::deque<spatial::Layer*>::iterator i = _layers.begin(); i != _layers.end(); ++i)
      if ((*i)->layer_id() == layer_id)
      {
        (*i)->set_show(flag);
        invalidate(true);
        return;
      }
  }
}

void SpatialDrawBox::fillup_polygon(int layer_id, bool flag)
{
  if (layer_id != 0)
  {
    base::MutexLock lock(_layer_mutex);
    for (std::deque<spatial::Layer*>::iterator i = _layers.begin(); i != _layers.end(); ++i)
      if ((*i)->layer_id() == layer_id)
      {
        (*i)->set_fill_polygons(flag);
        invalidate(true);
        return;
      }
  }
}

void SpatialDrawBox::activate()
{
  if (!_ready)
  {
    _ready = true;
    invalidate(true);
  }
}

void SpatialDrawBox::invalidate(bool reproject)
{
  if (_ready)
    render_in_thread(reproject);
  set_needs_repaint(); // repaint the grid
}


bool SpatialDrawBox::mouse_double_click(mforms::MouseButton button, int x, int y)
{
  double lat, lon;

  // zoom in and center the map at the clicked position
  if (screen_to_world(x, y, lat, lon))
  {
    double clat, clon;
    if (screen_to_world(get_width()/2, get_height()/2, clat, clon))
    {
      zoom_in();
      
      int dx, dy;
      world_to_screen(clat - lat, clon - lon, dx, dy);
      _offset_x = (dx - get_width()/2);
      _offset_y = (dy - get_height()/2);
      invalidate();
      _dragging = false;
    }
  }
  return false;
}

bool SpatialDrawBox::mouse_down(mforms::MouseButton button, int x, int y)
{
  if (button == mforms::MouseButtonLeft)
  {
    _initial_offset_x = _offset_x;
    _initial_offset_y = _offset_y;
    _drag_x = x;
    _drag_y = y;
    if (_select_pending || _selecting)
    {
      _selecting = true;
      _select_pending = false;
    }
    else
      _dragging = true;
  }
  else if (button == mforms::MouseButtonRight)
  {
    double lat = 0, lon = 0;
    screen_to_world(x, y, lat, lon);
    _clicked_coordinates = std::make_pair(lat, lon);

    if (_menu)
    {
      std::pair<int,int> p = client_to_screen(x, y);
      _menu->popup_at(this, base::Point(p.first, p.second));
    }
  }
  else if (button == mforms::MouseButtonOther)
  {
    base::Point p(x - _offset_x, y - _offset_y);
    base::MutexLock lock(_layer_mutex);
    for (std::deque<spatial::Layer*>::iterator it = _layers.begin(); it != _layers.end(); ++it)
    {
      if ((*it)->within(p))
      {
        fprintf(stderr, "Object clicked.\n");
        break;
      }
    }
  }
  return true;
}

bool SpatialDrawBox::mouse_up(mforms::MouseButton button, int x, int y)
{
  if (button == mforms::MouseButtonLeft && _dragging)
  {
    mouse_move(button, x, y);
    invalidate();
    _dragging = false;
  }
  else if (button == mforms::MouseButtonLeft && _selecting)
  {
    restrict_displayed_area(_drag_x, _drag_y, x, y);
    _selecting = false;
    set_needs_repaint();
    mforms::App::get()->set_status_text("");
  }
  return true;
}

bool SpatialDrawBox::mouse_move(mforms::MouseButton button, int x, int y)
{
  if (_dragging)
  {
    _offset_x = _initial_offset_x + x - _drag_x;
    _offset_y = _initial_offset_y + y - _drag_y;
    set_needs_repaint();
  }
  else if (_selecting)
  {
    _select_x = x;
    _select_y = y;
    set_needs_repaint();
  }

  double lat, lon;
  if (screen_to_world(x, y, lat, lon))
    position_changed_cb(spatial::Converter::dec_to_dms(lat, spatial::AxisLat, 2), spatial::Converter::dec_to_dms(lon, spatial::AxisLon, 2));
  else
    position_changed_cb("", "");

  return true;
}


void SpatialDrawBox::restrict_displayed_area(int x1, int y1, int x2, int y2, bool no_invalidate)
{
  double lat1, lat2;
  double lon1, lon2;

  if (x1 > x2) std::swap(x1, x2);
  if (y1 > y2) std::swap(y1, y2);

  if (screen_to_world(x1, y1, lat1, lon1) &&
      screen_to_world(x2, y2, lat2, lon2))
  {
    _zoom_level = 1.0;
    _offset_x = 0;
    _offset_y = 0;

    double h = std::abs(lat2 - lat1);

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

void SpatialDrawBox::repaint(cairo_t *crt, int x, int y, int w, int h)
{
  boost::shared_ptr<mdc::Surface> cache(_cache);
  mdc::CairoCtx cr(crt);
  if (cache)
  {
    cr.set_color(base::Color(1, 1, 1));
    cr.paint();
    cr.set_source_surface(cache->get_surface(), 0, 0);
    if (_rendering) // if we're currently re-rendering the image, we paint the old version half transparent
      cr.paint_with_alpha(0.4);
    else
      cr.paint();
  }
  else if (!_progress)
  {
    cr.set_color(base::Color(1, 1, 1));
    cr.paint();
  }

  if (_background_layer && !_background_layer->hidden())
  {
    cr.save();
    cr.translate(base::Point(_offset_x, _offset_y));
    cr.scale(base::Point(_zoom_level, _zoom_level));
    cr.set_line_width(0);
    _background_layer->repaint(cr, _zoom_level, base::Rect());
    cr.restore();
  }

  if (_rendering)
  {
    cr.set_color(base::Color(0, 0, 0));
    cr.move_to(base::Point(10, 20));
    cr.show_text("Repainting...");
  }

  if (_selecting)
  {
    cr.set_line_width(2);
    cr.set_color(base::Color(0, 0, 0));
    cr.rectangle(base::Rect(std::min(_drag_x, _select_x), std::min(_drag_y, _select_y),
                            abs(_select_x-_drag_x), abs(_select_y-_drag_y)));
    cr.stroke();
  }
}

bool SpatialDrawBox::screen_to_world(int x, int y, double &lat, double &lon)
{
  if (_spatial_reprojector)
  {
//     TODO check if x, y are inside the world image
//    if (x >= _offset_x && y >= _offset_y) <- this is not working when we do rectangular zoom
    return _spatial_reprojector->to_latlon(x - _offset_x, y - _offset_y, lat, lon);
  }
  return false;
}

void SpatialDrawBox::world_to_screen(double lat, double lon, int &x, int &y)
{
  if (_spatial_reprojector)
  {
    _spatial_reprojector->from_latlon(lat, lon, x, y);
    x += _offset_x;
    y += _offset_y;
  }
}
