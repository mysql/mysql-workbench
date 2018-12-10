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

#include "mdc_item_handle.h"
#include "mdc_interaction_layer.h"
#include "mdc_canvas_view.h"

using namespace mdc;
using namespace base;

ItemHandle::ItemHandle(InteractionLayer *ilayer, CanvasItem *item, const Point &point)
  : _item(item), _layer(ilayer), _display_list(0), _pos(point), _dirty(true), _highlighted(false), _draggable(true) {
}

//--------------------------------------------------------------------------------------------------

ItemHandle::~ItemHandle() {
  _layer->remove_handle(this);
  if (_display_list != 0)
    glDeleteLists(_display_list, 1);
}

//--------------------------------------------------------------------------------------------------

void ItemHandle::move(const Point &point) {
  Point delta = _pos;

  _layer->queue_repaint(get_bounds());

  _pos = point;

  delta.x = _pos.x - delta.x;
  delta.y = _pos.y - delta.y;

  _layer->queue_repaint(get_bounds());
  _dirty = true;
}

//--------------------------------------------------------------------------------------------------

void ItemHandle::set_highlighted(bool flag) {
  _highlighted = flag;
  _layer->queue_repaint(get_bounds());
  _dirty = true;
}

//--------------------------------------------------------------------------------------------------

void ItemHandle::set_draggable(bool flag) {
  _draggable = flag;
}

//--------------------------------------------------------------------------------------------------

void ItemHandle::set_color(const Color &color) {
  _color = color;
  _dirty = true;
}

//--------------------------------------------------------------------------------------------------

void ItemHandle::repaint(CairoCtx *cr) {
  Rect r = get_bounds();

  if (_layer->get_view()->has_gl())
    paint_gl(r);
  else {
    if (_draggable) {
      cr->set_color(_color);
      cr->set_line_width(1);
      cr->rectangle(r);
      cr->fill_preserve();

      if (_highlighted)
        cr->set_color(Color(0, 1, 1, 1));
      else
        cr->set_color(Color(0, 0, 0, 1));
      cr->stroke();
    } else {
      if (_highlighted)
        cr->set_color(Color(0.5, 1, 1, 1));
      else
        cr->set_color(Color(0.5, 0.5, 0.5, 1));
      cr->set_line_width(1);
      cr->rectangle(r);
      cr->fill_preserve();
      cr->set_color(Color::black());
      cr->stroke();
    }
  }
}

//--------------------------------------------------------------------------------------------------

void ItemHandle::paint_gl(Rect &r) {
  if (_dirty || _display_list == 0) {
    _dirty = false;
    if (_display_list == 0)
      _display_list = glGenLists(1);
    glNewList(_display_list, GL_COMPILE_AND_EXECUTE);

    // Color for the polygon border.
    Color border_color;
    if (_draggable) {
      if (_highlighted)
        border_color = Color(0, 1, 1);
      else
        border_color = Color(0, 0, 0);
    } else {
      if (_highlighted)
        border_color = Color(0.5, 1, 1);
      else
        border_color = Color(0.5, 0.5, 0.5);
    }
    gl_box(r, border_color, _color);
    glEndList();
  } else
    glCallList(_display_list);
}
