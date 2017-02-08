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
#ifndef _WORKBENCH_MODEL_NOTEFIGURE_IMPL_H_
#define _WORKBENCH_MODEL_NOTEFIGURE_IMPL_H_

#include "model_figure_impl.h"
#include "grts/structs.workbench.model.h"

#include "note_figure.h"

class WBPUBLICBACKEND_PUBLIC_FUNC workbench_model_NoteFigure::ImplData : public model_Figure::ImplData {
  typedef model_Figure::ImplData super;

protected:
  wbfig::Note *_figure;

  virtual bool realize();

public:
  ImplData(workbench_model_NoteFigure *self);
  virtual ~ImplData(){};

  void set_text(const std::string &text);
  void set_text_color(const std::string &color);
  void set_font(const std::string &font);

  virtual mdc::CanvasItem *get_canvas_item() const {
    return _figure;
  }

  virtual void unrealize();

private:
  workbench_model_NoteFigure *self() const {
    return (workbench_model_NoteFigure *)_self;
  }
};

#endif
