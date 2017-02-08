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

#ifndef _MDC_BACK_LAYER_H_
#define _MDC_BACK_LAYER_H_

#include "mdc_layer.h"

namespace mdc {

  class MYSQLCANVAS_PUBLIC_FUNC BackLayer : public Layer {
  public:
    BackLayer(CanvasView *view);
    virtual ~BackLayer();

    virtual void repaint(const base::Rect &bounds);

    void set_grid_visible(bool flag);
    void set_paper_visible(bool flag);

    bool get_grid_visible() {
      return _grid_visible;
    }
    bool get_paper_visible() {
      return _paper_visible;
    }

    void set_color(const base::Color &color);

  protected:
    base::Color _fill_color;
    base::Color _line1_color;
    base::Color _line2_color;

    // display lists for caching the grid
    GLint _grid1_dl;
    GLint _grid2_dl;
    // canvas position the display lists were generated
    base::Point _grid_dl_start;
    base::Rect _grid_dl_area;
    double _grid_dl_size;

    bool _grid_visible;
    bool _paper_visible;

    void render_page_borders(const base::Rect &aBounds);
    void render_grid(const base::Rect &aBounds);
  };

} // end of mdc namespace

#endif
