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

#ifndef _LAYER_FIGURE_H_
#define _LAYER_FIGURE_H_

#include "mdc.h"

#include <grts/structs.model.h>

namespace wbfig {
  class FigureEventHub;

  class LayerAreaGroup : public mdc::AreaGroup {
    typedef mdc::AreaGroup super;
    model_Object *_represented_object;
    FigureEventHub *_hub;
    base::Rect _initial_bounds;

    mdc::FontSpec _font;
    std::string _title;
    base::Color _title_fore;
    base::Color _title_back;

    cairo_text_extents_t _extents;
    bool _resizing;
    bool _extents_invalid;

    // OpenGL render support.
    GLuint _text_texture;
    GLuint _display_list; // OpenGL's rendering list for this item.

    boost::signals2::signal<void(base::Rect)> _resize_signal;

    virtual bool on_drag_handle(mdc::ItemHandle *handle, const base::Point &pos, bool dragging);

    virtual bool on_button_press(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                 mdc::EventState state);
    virtual bool on_button_release(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                   mdc::EventState state);
    virtual bool on_click(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                          mdc::EventState state);
    virtual bool on_double_click(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                 mdc::EventState state);
    virtual bool on_enter(mdc::CanvasItem *target, const base::Point &point);
    virtual bool on_leave(mdc::CanvasItem *target, const base::Point &point);

    virtual void move_item(mdc::CanvasItem *item, const base::Point &pos);

    virtual void render(mdc::CairoCtx *cr);
    virtual void render_gl(mdc::CairoCtx *cr);

    base::Rect get_title_bounds() const;

  public:
    LayerAreaGroup(mdc::Layer *layer, FigureEventHub *hub, model_Object *represented_object);
    ~LayerAreaGroup();

    void set_title(const std::string &title);
    void set_font(const mdc::FontSpec &font);

    boost::signals2::signal<void(base::Rect)> *signal_interactive_resize() {
      return &_resize_signal;
    }

    bool in_user_resize() const {
      return _resizing;
    }
  };
};

#endif
