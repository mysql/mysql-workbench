/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "layer_figure.h"
#include "figure_common.h"
#include <grts/structs.model.h>

using namespace std;
using namespace wbfig;
using namespace base;

#define MIN_LAYER_TITLE_WIDTH 120.0
#define MIN_LAYER_SIZE 10.0

//--------------------------------------------------------------------------------------------------

LayerAreaGroup::LayerAreaGroup(mdc::Layer *layer, FigureEventHub *hub, model_Object *represented_object)
  : mdc::AreaGroup(layer), _represented_object(represented_object), _hub(hub) {
  set_cache_toplevel_contents(false);

  set_auto_sizing(false);
  //  set_background(_layer_back);
  set_draggable(true);

  // Even though the extents are invalid we set some reasonable default values for width and height.
  // This is necessary to make working with them in environments where the figure is never rendered
  // possible (e.g. in unit tests).
  _extents_invalid = true;
  _extents.width = 130;
  _extents.height = 20;

  _font = layer->get_view()->get_default_font();
  _font.weight = mdc::WBold;

  _title_fore = Color::black();
  _title_back = Color(0.85, 0.85, 0.85);

  _min_size.width = MIN_LAYER_SIZE;
  _min_size.height = MIN_LAYER_SIZE;

  _drag_selects_contents = true;

  _resizing = false;

  _text_texture = 0;
  _display_list = 0;
}

//--------------------------------------------------------------------------------------------------

LayerAreaGroup::~LayerAreaGroup() {
#ifndef __APPLE__
  if (_display_list != 0)
    glDeleteLists(_display_list, 1);

  if (_text_texture != 0)
    glDeleteTextures(1, &_text_texture);
#endif
}

//--------------------------------------------------------------------------------------------------

void LayerAreaGroup::set_title(const std::string &title) {
  _title = title;
  _extents_invalid = true;
  set_needs_repaint();
}

void LayerAreaGroup::set_font(const mdc::FontSpec &font) {
  _font = font;
  _extents_invalid = true;
  set_needs_repaint();
}

static void get_bounding_area(mdc::CanvasItem *item, Point *maxpos) {
  Rect bounds = item->get_bounds();

  maxpos->x = max(maxpos->x, bounds.right());
  maxpos->y = max(maxpos->y, bounds.bottom());
}

void LayerAreaGroup::move_item(mdc::CanvasItem *item, const Point &pos) {
  item->move_to(pos);
}

bool LayerAreaGroup::on_drag_handle(mdc::ItemHandle *handle, const Point &pos, bool dragging) {
  if (!_resizing) {
    Point maxpos;

    _initial_bounds = get_root_bounds();
    _resizing = true;

    foreach (std::bind(get_bounding_area, std::placeholders::_1, &maxpos))
      ;

    _min_size.width = max(maxpos.x, MIN_LAYER_SIZE);
    _min_size.height = max(maxpos.y, MIN_LAYER_SIZE);
    _min_size_invalid = false;
  }

  bool flag = mdc::AreaGroup::on_drag_handle(handle, get_view()->snap_to_grid(pos), dragging);

  if (!dragging) {
    _resizing = false;
    _min_size.width = MIN_LAYER_SIZE;
    _min_size.height = MIN_LAYER_SIZE;
    _min_size_invalid = false;

    _resize_signal(_initial_bounds);
  }

  return flag;
}

bool LayerAreaGroup::on_button_press(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                                     mdc::EventState state) {
  if (mdc::bounds_contain_point(get_title_bounds(), point.x, point.y)) {
    _drag_selects_contents = false;
  }

  if (!_hub || !_hub->figure_button_release(_represented_object, target, point, button, state))
    return super::on_button_press(target, point, button, state);

  return false;
}

bool LayerAreaGroup::on_button_release(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                                       mdc::EventState state) {
  bool ret = false;

  if (!_hub || !_hub->figure_button_release(_represented_object, target, point, button, state))
    ret = super::on_button_release(target, point, button, state);

  _drag_selects_contents = true;

  return ret;
}

//--------------------------------------------------------------------------------------------------

bool LayerAreaGroup::on_click(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                              mdc::EventState state) {
  if (!_hub || !_hub->figure_click(_represented_object, target, point, button, state))
    return mdc::AreaGroup::on_click(target, point, button, state);

  return false;
}

//--------------------------------------------------------------------------------------------------

bool LayerAreaGroup::on_double_click(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                                     mdc::EventState state) {
  if (!_hub || !_hub->figure_double_click(_represented_object, target, point, button, state))
    return mdc::AreaGroup::on_double_click(target, point, button, state);

  return false;
}

//--------------------------------------------------------------------------------------------------

bool LayerAreaGroup::on_enter(mdc::CanvasItem *target, const Point &point) {
  if (!_hub || !_hub->figure_enter(_represented_object, target, point))
    return super::on_enter(target, point);
  return false;
}

bool LayerAreaGroup::on_leave(mdc::CanvasItem *target, const Point &point) {
  if (!_hub || !_hub->figure_leave(_represented_object, target, point))
    return super::on_leave(target, point);
  return false;
}

#define TEXT_PADDING 5

void LayerAreaGroup::render(mdc::CairoCtx *cr) {
  mdc::AreaGroup::render(cr);

  if (_extents_invalid) {
    cr->get_text_extents(_font, _title, _extents);
    _extents_invalid = false;
  }

  Size size = get_title_bounds().size;

  cr->save();
  cr->translate(get_position());
  cr->set_color(_title_back);
  cr->new_path();
  cr->move_to(Point(0, 0));
  cr->line_to(Point(size.width, 0));
  cr->line_to(Point(size.width, size.height - 5));
  cr->line_to(Point(size.width - 5, size.height));
  cr->line_to(Point(0, size.height));
  cr->close_path();
  cr->fill();

  cr->set_color(_title_fore);
  cr->move_to(Point(TEXT_PADDING + _extents.x_bearing, TEXT_PADDING - _extents.y_bearing));
  cr->set_font(_font);
  cr->show_text(_title);

  cr->restore();
}

//--------------------------------------------------------------------------------------------------

void LayerAreaGroup::render_gl(mdc::CairoCtx *cr) {
  mdc::AreaGroup::render_gl(cr);

#ifndef __APPLE__
  if (_extents_invalid) {
    cr->get_text_extents(_font, _title, _extents);
    _extents_invalid = false;
  }

  Size size = get_title_bounds().size;
  Point text_position = Point(_extents.x_bearing, -_extents.y_bearing);

  // Render caption for the layer figure. This is done via Cairo and a texture.
  bool generate_display_list = _display_list == 0;

  // Check if we need to regenerate the cache. if so, do it and load it as a texture.
  Size texture_size = get_texture_size(size);
  if (_needs_render || _text_texture == 0) {
    generate_display_list = true;

    // Create a temporary surface we can render the text into.
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (unsigned int)texture_size.width,
                                                          (unsigned int)texture_size.height);
    _layer->get_view()->bookkeep_cache_mem(cairo_image_surface_get_stride(surface) *
                                           cairo_image_surface_get_height(surface));
    memset(cairo_image_surface_get_data(surface), 0,
           cairo_image_surface_get_stride(surface) * cairo_image_surface_get_height(surface));

    // Create a context for the temporary surface and render the text.
    mdc::CairoCtx texture_context(surface);
    texture_context.set_color(_title_fore);
    texture_context.move_to(text_position);
    texture_context.set_font(_font);
    texture_context.show_text(_title);
    _needs_render = false;

    if (_text_texture == 0)
      glGenTextures(1, &_text_texture);

    // setup the texture
    glBindTexture(GL_TEXTURE_2D, _text_texture);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // don't tile the image
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    // load the texture into opengl
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (int)texture_size.width, (int)texture_size.height, 0, GL_BGRA,
                 GL_UNSIGNED_BYTE, cairo_image_surface_get_data(surface));

    // Once we transferred the pixel data we don't need the cache anymore.
    _layer->get_view()->bookkeep_cache_mem(-cairo_image_surface_get_stride(surface) *
                                           cairo_image_surface_get_height(surface));
    cairo_surface_destroy(surface);
  }

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslated(get_position().x, get_position().y, 0);

  if (generate_display_list) {
    if (_display_list == 0)
      _display_list = glGenLists(1);

    glNewList(_display_list, GL_COMPILE);

    mdc::gl_setcolor(_title_back);

    glBegin(GL_POLYGON);
    glVertex2f(0.0, 0.0);
    glVertex2d(size.width, 0);
    glVertex2d(size.width, size.height - 5);
    glVertex2d(size.width - 5, size.height);
    glVertex2d(0, size.height);
    glEnd();

    glEnable(GL_TEXTURE_2D);

    // Render the texture.
    glBindTexture(GL_TEXTURE_2D, _text_texture);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glTranslated(TEXT_PADDING, TEXT_PADDING, 0);
    glBegin(GL_QUADS);

    // Due to round-up to power of two the actual texture coordinates are usually somewhere within the actual texture.
    cairo_user_to_device_distance(_layer->get_view()->cairoctx()->get_cr(), &size.width, &size.height);

    double max_x_coordinate = size.width / texture_size.width;
    double max_y_coordinate = size.height / texture_size.height;

    glTexCoord2d(0, 0);
    glVertex2d(0, 0);

    glTexCoord2d(max_x_coordinate, 0);
    glVertex2d(size.width, 0);

    glTexCoord2d(max_x_coordinate, max_y_coordinate);
    glVertex2d(size.width, size.height);

    glTexCoord2d(0, max_y_coordinate);
    glVertex2d(0, size.height);

    glEnd();
    glDisable(GL_TEXTURE_2D);

    glEndList();
  }

  glCallList(_display_list);

  glPopMatrix();
#endif
}

Rect LayerAreaGroup::get_title_bounds() const {
  Rect rect;

  double width = _extents.width + 10;
  if (width < MIN_LAYER_TITLE_WIDTH)
    width = MIN_LAYER_TITLE_WIDTH;

  Size size = get_size();

  if (width > size.width - _extents.height - 2 * TEXT_PADDING)
    width = size.width - _extents.height - 2 * TEXT_PADDING;

  rect.size.width = width + 2 * TEXT_PADDING;
  rect.size.height = _extents.height + 2 * TEXT_PADDING;

  return rect;
}
