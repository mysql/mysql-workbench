/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mdc_figure.h"
#include "mdc_draw_util.h"

namespace mdc {

  class MYSQLCANVAS_PUBLIC_FUNC ImageFigure : public Figure {
  public:
    ImageFigure(Layer *layer);
    virtual ~ImageFigure();

    virtual void draw_contents(CairoCtx *cr);

    bool set_image(cairo_surface_t *surface);
    bool set_image(const std::string &path);
    base::Size get_image_size() const;

    cairo_surface_t *get_image() {
      return _image;
    }

  protected:
    cairo_surface_t *_image;

    virtual base::Size calc_min_size();
  };

  MYSQLCANVAS_PUBLIC_FUNC cairo_surface_t *surface_from_png_image(const std::string &file_name);

} // end of mdc namespace
