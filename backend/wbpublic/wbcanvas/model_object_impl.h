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

#pragma once

#include "mdc.h"
#include "grt.h"

#include "grts/structs.model.h"

#include "base_bridge.h"

#include "model_model_impl.h"

class WBPUBLICBACKEND_PUBLIC_FUNC model_Object::ImplData : public BridgeBase {
protected:
  model_Object *_self;
  bool _in_view;
  bool _notified_unrealize;

  ImplData(model_Object *object) : _self(object), _in_view(false), _notified_unrealize(false) {
  }

  virtual GrtObject *get_object() {
    return _self;
  }

  void notify_realized();
  void notify_will_unrealize();

public:
  virtual mdc::CanvasItem *get_canvas_item() const = 0;
  virtual void highlight(const base::Color *color = 0);
  virtual void unhighlight();

  virtual void set_in_view(bool flag);
  virtual bool in_view() {
    return _in_view;
  }

  virtual bool try_realize();

  virtual bool is_realizable() = 0;
  virtual bool realize() = 0;
  virtual void unrealize() = 0;
};
