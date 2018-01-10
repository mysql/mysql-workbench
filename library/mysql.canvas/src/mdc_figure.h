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

#ifndef _MDC_FIGURE_H_
#define _MDC_FIGURE_H_

#include "mdc_canvas_item.h"

namespace mdc {

  class MYSQLCANVAS_PUBLIC_FUNC Figure : public CanvasItem {
  public:
    Figure(Layer *layer);

    virtual void render(CairoCtx *cr);
    virtual void render_gl(mdc::CairoCtx *cr);
    virtual base::Point get_intersection_with_line_to(const base::Point &p);

    void set_pen_color(const base::Color &color);
    void set_fill_color(const base::Color &color);
    void set_line_width(float width);

    virtual void draw_contents(CairoCtx *cr) = 0;
    virtual void draw_contents_gl();

    virtual void stroke_outline(CairoCtx *cr, float offset) const;
    virtual void stroke_outline_gl(float offset) const;

  protected:
    base::Color _pen_color;
    base::Color _fill_color;
    float _line_width;
  };
};

#endif /* _MDC_FIGURE_H_ */
