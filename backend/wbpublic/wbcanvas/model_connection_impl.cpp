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

#include "model_connection_impl.h"
#include "model_figure_impl.h"
#include "model_diagram_impl.h"
#include "model_model_impl.h"
#include "base/string_utilities.h"

using namespace base;

model_Connection::ImplData::ImplData(model_Connection *self)
  : model_Object::ImplData(self), _line(0), _above_caption(0), _below_caption(0), _start_caption(0), _end_caption(0) {
  scoped_connect(self->signal_changed(),
                 std::bind(&ImplData::member_changed, this, std::placeholders::_1, std::placeholders::_2));
}

void model_Connection::ImplData::unrealize() {
  if (_line)
    _line->get_view()->remove_item(_line);

  delete _line;
  _line = 0;

  delete _above_caption;
  delete _below_caption;
  delete _start_caption;
  delete _end_caption;

  _above_caption = 0;
  _below_caption = 0;
  _start_caption = 0;
  _end_caption = 0;
}

void model_Connection::ImplData::object_realized(const model_ObjectRef &object) {
  if (object == self()->_startFigure || object == self()->_endFigure)
    try_realize();
}

void model_Connection::ImplData::member_changed(const std::string &name, const grt::ValueRef &ovalue) {
  if (_line) {
    if (name == "drawSplit")
      _line->set_splitted(*self()->_drawSplit != 0);
    else if (name == "visible") {
      _line->set_visible(*self()->_visible != 0);
      if (_above_caption)
        _above_caption->set_visible(*self()->_visible != 0);
      if (_below_caption)
        _below_caption->set_visible(*self()->_visible != 0);
      if (_start_caption)
        _start_caption->set_visible(*self()->_visible != 0);
      if (_end_caption)
        _end_caption->set_visible(*self()->_visible != 0);
    }
    /* this step should be done by the concrete subclass
    else if (name == "endFigure" || name == "startFigure")
    {
      unrealize();
      try_realize();
    } */
    else if (name == "owner") {
      if (!_object_realized.connected() && self()->owner().is_valid())
        _object_realized = self()->owner()->get_data()->signal_object_realized()->connect(
          std::bind(&ImplData::object_realized, this, std::placeholders::_1));
    }
  }
}

mdc::CanvasItem *model_Connection::ImplData::get_start_canvas_item() {
  if (self()->_startFigure.is_valid()) {
    model_Figure::ImplData *bridge = self()->_startFigure->get_data();

    if (bridge)
      return bridge->get_canvas_item();
  }
  return 0;
}

mdc::CanvasItem *model_Connection::ImplData::get_end_canvas_item() {
  if (self()->_endFigure.is_valid()) {
    model_Figure::ImplData *bridge = self()->_endFigure->get_data();

    if (bridge)
      return bridge->get_canvas_item();
  }
  return 0;
}

//--------------------------------------------------------------------------------------------------

bool model_Connection::ImplData::is_realizable() {
  if (_in_view && self()->owner().is_valid()) {
    try {
      if (is_canvas_view_valid() && get_start_canvas_item() && get_end_canvas_item())
        return true;
    } catch (std::exception &) {
      return false;
    }
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

mdc::CanvasView *model_Connection::ImplData::get_canvas_view() const {
  if (self()->owner().is_valid()) {
    model_Diagram::ImplData *view = self()->owner()->get_data();
    if (view)
      return view->get_canvas_view();
  }
  return 0;
}

//--------------------------------------------------------------------------------------------------

bool model_Connection::ImplData::is_canvas_view_valid() {
  if (self()->owner().is_valid()) {
    model_Diagram::ImplData *view = self()->owner()->get_data();
    if (view)
      return view->is_canvas_view_valid();
  }
  return false;
}

wbfig::CaptionFigure *model_Connection::ImplData::create_caption() {
  wbfig::CaptionFigure *figure = new wbfig::CaptionFigure(_line->get_layer(), self()->owner()->get_data(), self());
  figure->set_tag(self()->id());
  figure->set_font(_caption_font);
  _line->get_layer()->add_item(figure);
  figure->set_fill_background(false);
  figure->set_state_drawing(false);
  figure->set_draggable(true);
  figure->set_accepts_selection(true);
  figure->set_highlight_through_text(true);
  figure->set_visible(*self()->_visible != 0);
  scoped_connect(figure->signal_bounds_changed(),
                 std::bind(&model_Connection::ImplData::caption_bounds_changed, this, std::placeholders::_1, figure));
  return figure;
}

//--------------------------------------------------------------------------------------------------

void model_Connection::ImplData::set_above_caption(const std::string &text) {
  if (text.empty()) {
    delete _above_caption;
    _above_caption = 0;
    return;
  }

  if (self()->owner()->owner()->get_data()->get_int_option("workbench.physical.Connection:ShowCaptions", 0) == 0) {
    delete _above_caption;
    _above_caption = 0;
    return;
  }

  if (!_above_caption) {
    _above_caption = create_caption();
  }
  _above_caption->set_text(text);

  update_above_caption_pos();
}

void model_Connection::ImplData::set_below_caption(const std::string &text) {
  if (text.empty()) {
    delete _below_caption;
    _below_caption = 0;
    return;
  }

  if (self()->owner()->owner()->get_data()->get_int_option("workbench.physical.Connection:ShowCaptions", 0) == 0) {
    delete _below_caption;
    _below_caption = 0;
    return;
  }

  if (!_below_caption) {
    _below_caption = create_caption();
  }
  _below_caption->set_text(text);

  update_below_caption_pos();
}

void model_Connection::ImplData::set_start_caption(const std::string &text) {
  if (text.empty()) {
    delete _start_caption;
    _start_caption = 0;
    return;
  }

  if (!_start_caption) {
    _start_caption = create_caption();
  }
  _start_caption->set_text(text);

  update_start_caption_pos();
}

void model_Connection::ImplData::set_end_caption(const std::string &text) {
  if (text.empty()) {
    delete _end_caption;
    _end_caption = 0;
    return;
  }

  if (!_end_caption) {
    _end_caption = create_caption();
  }
  _end_caption->set_text(text);

  update_end_caption_pos();
}

void model_Connection::ImplData::caption_bounds_changed(const Rect &obounds, mdc::TextFigure *figure) {
  if (figure == _above_caption)
    _above_offset =
      figure->get_root_position() - _line->get_middle_caption_pos(_above_caption->get_size(), wbfig::Connection::Above);
  else if (figure == _below_caption)
    _below_offset =
      figure->get_root_position() - _line->get_middle_caption_pos(_below_caption->get_size(), wbfig::Connection::Below);
  else if (figure == _start_caption)
    _start_offset = figure->get_root_position() - _line->get_start_caption_pos(_start_caption->get_size());
  else if (figure == _end_caption)
    _end_offset = figure->get_root_position() - _line->get_end_caption_pos(_end_caption->get_size());
}

void model_Connection::ImplData::update_above_caption_pos() {
  Point pos = _line->get_middle_caption_pos(_above_caption->get_min_size(), wbfig::Connection::Above);

  _above_caption->move_to(pos + _above_offset);
}

void model_Connection::ImplData::update_below_caption_pos() {
  Point pos = _line->get_middle_caption_pos(_below_caption->get_min_size(), wbfig::Connection::Below);

  _below_caption->move_to(pos + _below_offset);
}

void model_Connection::ImplData::update_start_caption_pos() {
  Point pos = _line->get_start_caption_pos(_start_caption->get_min_size());

  _start_caption->move_to(pos + _start_offset);
}

void model_Connection::ImplData::update_end_caption_pos() {
  Point pos = _line->get_end_caption_pos(_end_caption->get_min_size());

  _end_caption->move_to(pos + _end_offset);
}

void model_Connection::ImplData::layout_changed() {
  if (_above_caption)
    update_above_caption_pos();

  if (_below_caption)
    update_below_caption_pos();

  if (_start_caption)
    update_start_caption_pos();

  if (_end_caption)
    update_end_caption_pos();
}

void model_Connection::ImplData::finish_realize() {
  _line->set_tag(self()->id());

  _line->set_splitted(*self()->_drawSplit != 0);

  if (self()->owner()->owner()->get_data()->get_int_option("workbench.physical.Connection:CenterCaptions", 0) != 0)
    _line->set_center_captions(true);
  else
    _line->set_center_captions(false);

  _line->set_visible(*self()->_visible != 0);
  if (_above_caption)
    _above_caption->set_visible(*self()->_visible != 0);
  if (_below_caption)
    _below_caption->set_visible(*self()->_visible != 0);
  if (_start_caption)
    _start_caption->set_visible(*self()->_visible != 0);
  if (_end_caption)
    _end_caption->set_visible(*self()->_visible != 0);

  std::string font = self()->owner()->owner()->get_data()->get_string_option(
    base::strfmt("%s:CaptionFont", self()->class_name().c_str()), "");
  if (!font.empty())
    _caption_font = mdc::FontSpec::from_string(font);

  scoped_connect(_line->signal_layout_changed(), std::bind(&model_Connection::ImplData::layout_changed, this));

  self()->owner()->get_data()->stack_connection(model_ConnectionRef(self()), _line);

  _object_realized.disconnect();
}
