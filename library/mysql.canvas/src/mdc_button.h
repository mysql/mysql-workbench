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

#ifndef __MDC_BUTTON_H__
#define __MDC_BUTTON_H__

#include "mdc_icon_text.h"

namespace mdc {

  enum ButtonType { ActionButton, ToggleButton, ExpanderButton };

  class MYSQLCANVAS_PUBLIC_FUNC Button : public IconTextFigure {
  public:
    Button(Layer *layer, ButtonType type);
    ~Button();

    void set_active(bool flag);
    bool get_active();

    void set_image(cairo_surface_t *image);
    void set_alt_image(cairo_surface_t *image);

    virtual void draw_contents(CairoCtx *cr);
    virtual base::Size calc_min_size();

    boost::signals2::signal<void()> *signal_activate() {
      return &_action_signal;
    }

  protected:
    ButtonType _button_type;
    bool _active;
    bool _pressed;
    bool _inside;

    cairo_surface_t *_image;
    cairo_surface_t *_alt_image;

    boost::signals2::signal<void()> _action_signal;

    virtual bool on_button_press(CanvasItem *target, const base::Point &point, MouseButton button, EventState state);
    virtual bool on_button_release(CanvasItem *target, const base::Point &point, MouseButton button, EventState state);
    virtual bool on_enter(CanvasItem *target, const base::Point &point);
    virtual bool on_leave(CanvasItem *target, const base::Point &point);
    virtual bool on_drag(CanvasItem *target, const base::Point &point, EventState state);
  };

} // end of mdc namespace

#endif /* __MDC_BUTTON_H__ */
