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

#include "mdc_canvas_view_image.h"

using namespace mdc;
using namespace base;

ImageCanvasView::ImageCanvasView(int width, int height, cairo_format_t format)
  : CanvasView(width, height), _buffer(0), _format(format) {
}

ImageCanvasView::~ImageCanvasView() {
  if (_buffer)
    cairo_surface_destroy(_buffer);
}

void ImageCanvasView::begin_repaint(int x, int y, int w, int h) {
}

void ImageCanvasView::end_repaint() {
}

void ImageCanvasView::update_view_size(int width, int height) {
  if (_buffer && _view_width == width && _view_height == height)
    return;

  if (_buffer)
    cairo_surface_destroy(_buffer);

  _buffer = cairo_image_surface_create(_format, width, height);

  delete _cairo;
  _cairo = new CairoCtx(_buffer);
  cairo_set_tolerance(_cairo->get_cr(), 0.1);

  update_offsets();
  queue_repaint();

  _viewport_changed_signal();
}

void ImageCanvasView::save_to(const std::string &path) {
  // clear in white
  memset(cairo_image_surface_get_data(_buffer), 0xff, cairo_image_surface_get_stride(_buffer) * _view_height);

  render_for_export(Rect(Point(0, 0), get_total_view_size()), 0);

  cairo_surface_write_to_png(_buffer, path.c_str());
}

const unsigned char *ImageCanvasView::get_image_data(size_t &size) {
  repaint();

  size = cairo_image_surface_get_stride(_buffer) * _view_height;

  return cairo_image_surface_get_data(_buffer);
}
