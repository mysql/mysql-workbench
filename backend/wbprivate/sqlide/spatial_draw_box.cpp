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


#include "spatial_draw_box.h"
#include "mforms/box.h"
#include "mforms/label.h"
#include "mforms/panel.h"
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
    _timer = mforms::Utilities::add_timeout(interval, boost::bind(&ProgressPanel::update, this));
    _progress_fetcher = progress_fetcher;
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
    _progress->start(boost::bind(&SpatialDrawBox::get_progress, this, _1, _2),
                     0.2);
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

  GIS::ProjectionView visible_area;

  // calculate how much the offset in pixels corresponds to in lon/lat values, so that gdal will adjust the
  // clipping area to the area we want to view

  visible_area.MaxLat = 179;
  visible_area.MaxLon = 89;
  visible_area.MinLat = -179;
  visible_area.MinLon = -89;

  visible_area.height = height;
  visible_area.width = width;
  visible_area.type = _proj;

  // TODO lat/long ranges must be adjusted accordingly to account for the aspect ratio of the visible area

  boost::shared_ptr<mdc::ImageSurface> surface(new mdc::ImageSurface(get_width(), get_height(), CAIRO_FORMAT_ARGB32));
  mdc::CairoCtx ctx(*surface);

  _current_work = "Rendering layers...";
  _current_layer = NULL;
  _current_layer_index = 0;

  ctx.translate(base::Point(_offset_x, _offset_y));
  ctx.scale(base::Point(_zoom_level, _zoom_level));

  if (reproject)
    _background_layer->render(visible_area);

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
        (*it)->render(visible_area);
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
    current_progress += (1.0/_layers.size()) * _current_layer->query_render_progress();

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
: _background_layer(NULL),
_proj(GIS::ProjRobinson), _zoom_level(1.0), _offset_x(0), _offset_y(0), _ready(false), _dragging(false),
_rendering(false), _quitting(false), _needs_reprojection(true)
{
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

void SpatialDrawBox::set_projection(GIS::ProjectionType proj)
{
  _proj = proj;
  invalidate(true);
}

void SpatialDrawBox::zoom_out()
{
  _zoom_level -= 0.3;
  if (_zoom_level < 0)
    _zoom_level = 0;
  invalidate();
}

void SpatialDrawBox::zoom_in()
{
  _zoom_level += 0.3;
  invalidate();
}

void SpatialDrawBox::reset_view()
{
  _zoom_level = 1.0;
  _offset_x = 0;
  _offset_y = 0;
  invalidate();
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
}

void SpatialDrawBox::set_background(spatial::Layer *layer)
{
  if (_background_layer)
    delete _background_layer;
  _background_layer = layer;
}

void SpatialDrawBox::add_layer(spatial::Layer *layer)
{
  {
    base::MutexLock lock(_layer_mutex);
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
    invalidate();
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
  // zoom in and center the map at the clicked position
 
  zoom_in();

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
    _dragging = true;
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
  return true;
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
    _background_layer->repaint(cr, _zoom_level, base::Rect());
    cr.restore();
  }

  if (_rendering)
  {
    cr.set_color(base::Color(0, 0, 0));
    cr.move_to(base::Point(10, 20));
    cr.show_text("Repainting...");
  }
}

void SpatialDrawBox::screen_to_world(GIS::SpatialHandler *handler, int x, int y, double &lat, double &lon)
{
  handler->toLatLng(x, y, lat, lon);
}

void SpatialDrawBox::world_to_screen(GIS::SpatialHandler *handler, double lat, double lon, int &x, int &y)
{
  
  //    handler->fromLatLng(lat, lon, x, y);
}
