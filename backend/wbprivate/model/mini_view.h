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

#ifndef _MINI_VIEW_H_
#define _MINI_VIEW_H_

#include "wbcanvas/base_bridge.h"

namespace mdc {
  class CanvasView;
  struct CairoCtx;
};

namespace wb {
  class MiniView : public mdc::Figure {
  private:
    mdc::CanvasView *_canvas_view;
    model_DiagramRef _model_diagram;
    base::Color _backgroundColor;

    base::Point _click_pos;

    bool _updating_viewport;
    bool _skip_viewport_update;

    mdc::RectangleFigure *_viewport_figure;

    boost::signals2::scoped_connection _view_repaint_connection;
    boost::signals2::scoped_connection _view_viewport_change_connection;

    void render_figure(mdc::CairoCtx *cr, const model_FigureRef &elem);
    void render_layer(mdc::CairoCtx *cr, const model_LayerRef &layer);
    void render_layer_figures(mdc::CairoCtx *cr, const model_LayerRef &layer);
    virtual void draw_contents(mdc::CairoCtx *cr);

    void viewport_changed();

    void viewport_dragged(const base::Rect &rect);

    base::Rect get_scaled_target_bounds(double &scale);

    bool view_button_cb(mdc::CanvasView *, mdc::MouseButton, bool, base::Point, mdc::EventState);
    bool view_motion_cb(mdc::CanvasView *, base::Point, mdc::EventState);

  public:
    MiniView(mdc::Layer *output_layer);
    virtual ~MiniView();

    void update_size();
    void set_active_view(mdc::CanvasView *canvas_view, const model_DiagramRef &model_diagram);
    void setBackgroundColor(base::Color const& color);
  };
};

#endif /* _MINI_VIEW_H_ */
