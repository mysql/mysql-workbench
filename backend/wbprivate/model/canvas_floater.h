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

#ifndef _CANVAS_FLOATER_H_
#define _CANVAS_FLOATER_H_

#include "mdc.h"

namespace wbfig {
  class Titlebar;
};

namespace wb {

  class Button : public mdc::Button {
  public:
    Button(mdc::Layer *layer);

    virtual void draw_contents(mdc::CairoCtx *cr);
  };

  class Floater : public mdc::Box {
    typedef mdc::Box super;

  public:
    Floater(mdc::Layer *layer, const std::string &title);
    virtual ~Floater();

    void set_title(const std::string &title);

  protected:
    wbfig::Titlebar *_title;
    mdc::Box _content_box;

  private:
    base::Point _drag_offset;
    bool _dragging;

    void update_position();

    virtual bool on_button_press(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                 mdc::EventState state);
    virtual bool on_button_release(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                   mdc::EventState state);
    virtual bool on_drag(mdc::CanvasItem *target, const base::Point &point, mdc::EventState state);
  };
};

#endif
