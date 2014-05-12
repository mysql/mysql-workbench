/* 
 * Copyright (c) 2007, 2012, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MDC_IMAGE_H_
#define _MDC_IMAGE_H_

#include "mdc_figure.h"
#include "mdc_draw_util.h"

BEGIN_MDC_DECLS

class MYSQLCANVAS_PUBLIC_FUNC ImageFigure : public Figure {
public:  
  ImageFigure(Layer *layer);
  virtual ~ImageFigure();

  virtual void draw_contents(CairoCtx *cr);

  bool set_image(cairo_surface_t *surface);
  bool set_image(const std::string &path);
  base::Size get_image_size() const;

  cairo_surface_t *get_image() { return _image; }

protected:
  cairo_surface_t *_image;

  virtual base::Size calc_min_size();
};


END_MDC_DECLS


#endif /* _MDC_IMAGE_H_ */
