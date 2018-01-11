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
