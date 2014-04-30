/* 
 * Copyright (c) 2007, 2013, Oracle and/or its affiliates. All rights reserved.
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
#include "stdafx.h"

#include "note_figure.h"

using namespace wbfig;
using namespace base;

Note::Note(mdc::Layer *layer, FigureEventHub *hub, const model_ObjectRef &self)
: BaseFigure(layer, hub, self), _text(layer)
{
  set_allowed_resizing(true, true);
  set_accepts_focus(true);
  set_accepts_selection(true);
  set_auto_sizing(true);

  set_border_color(Color(0.5, 0.5, 0.5, 0.2));
  set_background_color(Color::White());
  set_draw_background(true);

  _text.set_padding(8, 8);
  _text.set_font(get_view()->get_default_font());
  _text.set_pen_color(Color::Black());
  _text.set_multi_line(true);

  add(&_text, false, false, true);
}


Note::~Note()
{
}


void Note::set_text(const std::string &text)
{
  _text.set_text(text);
  set_needs_relayout();
}


void Note::set_text_color(const Color &color)
{
  _text.set_pen_color(color);
}


void Note::set_content_font(const mdc::FontSpec &font)
{
  _text.set_font(font);
  set_needs_relayout();
}

//--------------------------------------------------------------------------------------------------

bool Note::on_click(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button, mdc::EventState state)
{
  if (!_hub->figure_click(represented_object(), target, point, button, state))
    return super::on_click(target, point, button, state);
  return false;
}

//--------------------------------------------------------------------------------------------------

bool Note::on_double_click(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button, mdc::EventState state)
{
  if (!_hub->figure_double_click(represented_object(), target, point, button, state))
    return super::on_double_click(target, point, button, state);
  return false;
}

//--------------------------------------------------------------------------------------------------
