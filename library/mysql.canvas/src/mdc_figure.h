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
