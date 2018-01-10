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

#include "mdc_button.h"

using namespace mdc;
using namespace base;

Button::Button(Layer *layer, ButtonType type)
  : IconTextFigure(layer),
    _button_type(type),
    _active(false),
    _pressed(false),
    _inside(false),
    _image(NULL),
    _alt_image(NULL) {
}

Button::~Button() {
  if (_image != NULL)
    cairo_surface_destroy(_image);
  if (_alt_image != NULL)
    cairo_surface_destroy(_alt_image);
}

void Button::set_active(bool flag) {
  if (_active != flag) {
    _active = flag;
    set_needs_render();
  }
}

bool Button::get_active() {
  return _active;
}

void Button::set_image(cairo_surface_t *image) {
  if (_image != image) {
    if (_image)
      cairo_surface_destroy(_image);
    _image = cairo_surface_reference(image);

    if (_pressed)
      IconTextFigure::set_icon(_image);
  }
}

void Button::set_alt_image(cairo_surface_t *image) {
  if (_alt_image != image) {
    if (_alt_image)
      cairo_surface_destroy(_alt_image);
    _alt_image = cairo_surface_reference(image);

    if (!_pressed)
      IconTextFigure::set_icon(_alt_image);
  }
}

Size Button::calc_min_size() {
  if (_button_type == ExpanderButton)
    return Size(10, 10);
  else
    return IconTextFigure::calc_min_size();
}

void Button::draw_contents(CairoCtx *cr) {
  Point pos = get_position();

  if (_button_type == ExpanderButton) {
    cr->save();
    cr->set_color(_pen_color);

    pos = pos + Point((get_size().width - 9) / 2, (get_size().height - 9) / 2);
    cr->translate(pos);

    if (_active) {
      cr->move_to(0, 1);
      cr->line_to(9, 1);
      cr->line_to(4.5, 9);
      cr->close_path();
    } else {
      cr->move_to(0, 0);
      cr->line_to(8, 4.5);
      cr->line_to(0, 9);
      cr->close_path();
    }
    cr->fill();
    cr->restore();
  } else {
    cr->save();
    if (_pressed)
      cr->translate(1, 1);
    IconTextFigure::draw_contents(cr);
    cr->restore();
  }
}

bool Button::on_button_press(CanvasItem *target, const Point &point, MouseButton button, EventState state) {
  if (button == ButtonLeft) {
    _pressed = true;
    if (_alt_image)
      IconTextFigure::set_icon(_alt_image);
    set_needs_render();
    return true;
  }
  return false;
}

bool Button::on_button_release(CanvasItem *target, const Point &point, MouseButton button, EventState state) {
  if (button == ButtonLeft) {
    bool activate = false;

    if (_inside && _pressed) {
      if (_button_type == ToggleButton || _button_type == ExpanderButton)
        _active = !_active;
      activate = true;
    }
    _pressed = false;
    if (_image)
      IconTextFigure::set_icon(_image);
    set_needs_render();
    if (activate)
      _action_signal();
    return true;
  }
  return false;
}

bool Button::on_enter(CanvasItem *target, const Point &point) {
  _inside = true;
  if (_pressed) {
    if (_image)
      IconTextFigure::set_icon(_alt_image);
    set_needs_render();
    return true;
  }
  return Figure::on_enter(target, point);
}

bool Button::on_leave(CanvasItem *target, const Point &point) {
  _inside = false;
  if (_pressed) {
    if (_alt_image)
      IconTextFigure::set_icon(_alt_image);
    set_needs_render();
    return true;
  }
  return Figure::on_leave(target, point);
}

bool Button::on_drag(CanvasItem *target, const Point &point, EventState active) {
  // ignore and don't allow drag
  return true;
}
