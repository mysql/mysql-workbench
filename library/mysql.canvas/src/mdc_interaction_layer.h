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

#ifndef _MDC_INTERACTION_LAYER_H_
#define _MDC_INTERACTION_LAYER_H_

#include "mdc_layer.h"

namespace mdc {

  class ItemHandle;

  class MYSQLCANVAS_PUBLIC_FUNC InteractionLayer : public Layer {
  public:
    InteractionLayer(CanvasView *view);

    virtual void repaint(const base::Rect &bounds);

    void add_handle(ItemHandle *handle);
    void remove_handle(ItemHandle *handle);
    ItemHandle *get_handle_at(const base::Point &pos);

    void set_active_area(const base::Rect &rect);
    void reset_active_area();

    bool handle_mouse_move(const base::Point &pos, EventState state);
    bool handle_mouse_button_top(MouseButton button, bool press, const base::Point &pos, EventState state);
    bool handle_mouse_button_bottom(MouseButton button, bool press, const base::Point &pos, EventState state);

    void start_selection_rectangle(const base::Point &pos, EventState state);
    void update_selection_rectangle(const base::Point &end, EventState state);
    void end_selection_rectangle(const base::Point &pos, EventState state);

    void start_dragging_rectangle(const base::Point &pos);
    void update_dragging_rectangle(const base::Point &pos);
    void draw_dragging_rectangle();
    base::Rect finish_dragging_rectangle();

    boost::signals2::signal<void(CairoCtx *)> *signal_custom_repaint() {
      return &_custom_repaint;
    }

  protected:
    std::list<ItemHandle *> _handles;
    ItemHandle *_dragging_handle;
    base::Point _dragging_pos;

    boost::signals2::signal<void(CairoCtx *)> _custom_repaint;

    base::Rect _active_area;

    base::Point _selection_start;
    base::Point _selection_end;
    bool _selection_started;
    bool _selection_started_by_us;

    base::Point _dragging_rectangle_start;
    base::Point _dragging_rectangle_end;
    bool _dragging_rectangle;

    void draw_selection(const base::Rect &clip);
  };

} // end of mdc namespace

#endif
