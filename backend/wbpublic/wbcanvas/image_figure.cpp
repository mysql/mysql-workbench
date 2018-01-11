/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "image_figure.h"

using namespace wbfig;
using namespace base;

Image::Image(mdc::Layer *layer, FigureEventHub *hub, const model_ObjectRef &self)
  : BaseFigure(layer, hub, self), _image(layer) {
  set_cache_toplevel_contents(false);
  set_accepts_focus();
  set_accepts_selection();
  set_allowed_resizing(true, true);

  add(&_image, true, true, false);

  _image.set_auto_sizing(true);

  _keep_aspect_ratio = false;
}

static void constrain_aspect_ratio(mdc::ItemHandle *handle, Size &size, double ratio) {
  if ((handle->get_tag() & (HDL_LEFT | HDL_RIGHT)) != 0)
    size.height = size.width / ratio;
  else
    size.width = size.height * ratio;
}

cairo_surface_t *Image::get_image() {
  return _image.get_image();
}

void Image::keep_aspect_ratio(bool flag) {
  _keep_aspect_ratio = flag;

  if (flag) {
    cairo_surface_t *surf = _image.get_image();

    if (surf) {
      double aspect = get_aspect_ratio();
      double width = _image.get_size().width;
      double new_height = width / aspect;

      if (fabs(_image.get_size().height - new_height) > 1)
        set_fixed_size(Size(width, new_height));

      set_drag_handle_constrainer(
        std::bind(constrain_aspect_ratio, std::placeholders::_1, std::placeholders::_2, aspect));
    }
  } else
    set_drag_handle_constrainer(std::function<void(mdc::ItemHandle *, Size &)>());
}

double Image::get_aspect_ratio() {
  Size size = _image.get_image_size();

  return size.width / size.height;
}

void Image::set_image(cairo_surface_t *image) {
  _image.set_image(image);
}

bool Image::set_image(const std::string &filename) {
  return _image.set_image(filename);
}

void Image::set_allow_manual_resizing(bool flag) {
  if (!flag && _image.auto_sizing()) {
    // if auto-sizing is being disabled, set the fixed size to the current size, so that it
    // has an initial size
    _image.set_fixed_size(_image.get_size());
  }
  _image.set_auto_sizing(!flag);

  if (!flag)
    relayout();
}

//--------------------------------------------------------------------------------------------------

bool Image::on_click(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button, mdc::EventState state) {
  if (!_hub->figure_click(represented_object(), target, point, button, state))
    return super::on_click(target, point, button, state);
  return false;
}

//--------------------------------------------------------------------------------------------------

bool Image::on_double_click(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                            mdc::EventState state) {
  if (!_hub->figure_double_click(represented_object(), target, point, button, state))
    return super::on_double_click(target, point, button, state);
  return false;
}

//--------------------------------------------------------------------------------------------------
