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

#include "base/file_functions.h"

#include "mdc_image.h"
#include "mdc_image_manager.h"

using namespace mdc;
using namespace base;

ImageFigure::ImageFigure(Layer *layer) : Figure(layer) {
  _image = 0;
}

ImageFigure::~ImageFigure() {
  cairo_surface_destroy(_image);
}

Size ImageFigure::get_image_size() const {
  return Size(cairo_image_surface_get_width(_image), cairo_image_surface_get_height(_image));
}

Size ImageFigure::calc_min_size() {
  Size size(1, 1);
  if (_image && _auto_sizing) {
    size = get_image_size();
    size.width += 2 * _xpadding;
    size.height += 2 * _ypadding;
  }

  return size;
}

void ImageFigure::draw_contents(CairoCtx *cr) {
  if (_image) {
    int w = cairo_image_surface_get_width(_image);
    int h = cairo_image_surface_get_height(_image);
    Point pos = get_position();

    pos.x = (get_size().width - w) / 2;
    pos.y = (get_size().height - h) / 2;

    pos.round();

    cr->save();
    // cr->translate(pos);
    cr->scale(get_size().width / w, get_size().height / h);
    cr->set_source_surface(_image, 0, 0);
    cr->paint();
    cr->restore();
  }
}

bool ImageFigure::set_image(cairo_surface_t *surface) {
  if (_image != surface) {
    if (_image)
      cairo_surface_destroy(_image);
    _image = cairo_surface_reference(surface);
    set_size(get_image_size());
    _min_size_invalid = true;
    set_needs_relayout();
  }
  return true;
}

bool ImageFigure::set_image(const std::string &path) {
  cairo_surface_t *image = ImageManager::get_instance()->get_image_nocache(path);
  if (image) {
    bool flag = set_image(image);
    cairo_surface_destroy(image);
    return flag;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

/**
*	Helper function to read data from a given file.
*/
cairo_status_t read_png_data(void *closure, unsigned char *data, unsigned int length) {
  FILE *png_file = (FILE *)closure;
  if (fread(data, 1, length, png_file) == length)
    return CAIRO_STATUS_SUCCESS;

  return CAIRO_STATUS_READ_ERROR;
}

//--------------------------------------------------------------------------------------------------

/**
*	Creates a cairo image surface and fills it with data from a png file.
*	The code can load from utf-8 encoded paths.
*/
cairo_surface_t *mdc::surface_from_png_image(const std::string &file_name) {
  FILE *png_file = base_fopen(file_name.c_str(), "r");
  if (png_file == NULL)
    return NULL;

  cairo_surface_t *image = cairo_image_surface_create_from_png_stream(read_png_data, png_file);
  fclose(png_file);
  if (image == NULL || cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
    if (image != NULL)
      cairo_surface_destroy(image);
    return NULL;
  }

  return image;
}

//--------------------------------------------------------------------------------------------------
