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

#ifndef _MDC_LAYOUTER_H_
#define _MDC_LAYOUTER_H_

#include "mdc_common.h"
#include "mdc_canvas_item.h"
#include "mdc_draw_util.h"

namespace mdc {

  class Figure;

  class MYSQLCANVAS_PUBLIC_FUNC Layouter : public CanvasItem {
  public:
    Layouter(Layer *layer);
    virtual ~Layouter();

    virtual CanvasItem *get_item_at(const base::Point &pos) = 0;
    virtual void remove(CanvasItem *item) = 0;
    virtual void remove_all();

    virtual void foreach (const std::function<void(CanvasItem *)> &slot) = 0;

    virtual void render(CairoCtx *cr);
    virtual void render_gl(mdc::CairoCtx *cr);

    virtual CanvasItem *find_item_with_tag(const std::string &tag);

    void set_draw_background(bool flag);
    void set_background_corners(mdc::CornerMask mask, float radius);
    void set_background_color(const base::Color &color);
    void set_border_color(const base::Color &color);

    virtual void stroke_outline(CairoCtx *cr, float offset = 0) const;
    virtual void stroke_outline_gl(float offset = 0) const;

  protected:
    base::Color _border_color;
    base::Color _background_color;
    mdc::CornerMask _corner_mask;
    float _corner_radius;
    bool _draw_background;
  };
};

#endif /* _MDC_LAYOUTER_H_ */
