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

#pragma once

#include "mdc.h"
#include "grt.h"

#include "grts/structs.model.h"

#include "wbpublic_public_interface.h"

#include "model_object_impl.h"

#include "figure_common.h"
#include "connection_figure.h"

class WBPUBLICBACKEND_PUBLIC_FUNC model_Connection::ImplData : public model_Object::ImplData {
  typedef model_Object::ImplData super;

protected:
  wbfig::Connection *_line;
  wbfig::CaptionFigure *_above_caption;
  base::Point _above_offset;
  wbfig::CaptionFigure *_below_caption;
  base::Point _below_offset;
  wbfig::CaptionFigure *_start_caption;
  base::Point _start_offset;
  wbfig::CaptionFigure *_end_caption;
  base::Point _end_offset;

  mdc::FontSpec _caption_font;

  boost::signals2::scoped_connection _object_realized;

  bool _destroying;

  ImplData(model_Connection *owner);

  void member_changed(const std::string &name, const grt::ValueRef &ovalue);

  void layout_changed();

  void update_above_caption_pos();
  void update_below_caption_pos();

  void update_start_caption_pos();
  void update_end_caption_pos();

  virtual void caption_bounds_changed(const base::Rect &obounds, mdc::TextFigure *figure);

  void object_realized(const model_ObjectRef &object);

  wbfig::CaptionFigure *create_caption();

public:
  mdc::CanvasView *get_canvas_view() const;
  bool is_canvas_view_valid();
  virtual bool is_realizable();
  void finish_realize();

  void set_above_caption(const std::string &text);
  void set_below_caption(const std::string &text);

  void set_start_caption(const std::string &text);
  void set_end_caption(const std::string &text);

public:
  virtual mdc::CanvasItem *get_canvas_item() const {
    return _line;
  }
  virtual mdc::CanvasItem *get_start_canvas_item();
  virtual mdc::CanvasItem *get_end_canvas_item();

  virtual void unrealize();

private:
  model_Connection *self() const {
    return (model_Connection *)_self;
  }
};
