/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "mdc.h"
#include "grt.h"

#include "grts/structs.model.h"

#include "wbpublic_public_interface.h"

#include "model_object_impl.h"

#include "figure_common.h"

#include "badge_figure.h"

namespace bec {
  class UndoAction;
};

class WBPUBLICBACKEND_PUBLIC_FUNC model_Figure::ImplData : public model_Object::ImplData {
  typedef BridgeBase super;

  friend class model_Diagram::ImplData;

protected:
  std::list<BadgeFigure *> _badges;

  bool _resizable;
  bool _connected_update_options;
  bool _realizing;

  ImplData(model_Figure *owner);

  virtual void update_options(const std::string &key);

  void finish_realize();

  void member_changed(const std::string &member, const grt::ValueRef &ovalue);

  void figure_resized(const base::Rect &rect);
  void figure_bounds_changed(const base::Rect &rect);

  virtual bool is_realizable();

  void relayout_badges();

public:
  mdc::CanvasView *get_canvas_view() const;
  bool is_canvas_view_valid();

  void set_layer(const model_LayerRef &layer);

  void add_badge(BadgeFigure *badge);
  void remove_badge(BadgeFigure *badge);
  BadgeFigure *get_badge_with_id(const std::string &badge_id);

public:
  virtual void render_mini(mdc::CairoCtx *cr);
  virtual void unrealize();

  virtual void highlight(const base::Color *color = 0);
  virtual void unhighlight();

private:
  model_Figure *self() const {
    return (model_Figure *)_self;
  }
};
