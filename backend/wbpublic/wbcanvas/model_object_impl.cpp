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

#include "model_object_impl.h"
#include "model_diagram_impl.h"

using namespace base;

void model_Object::ImplData::notify_realized() {
  _notified_unrealize = false;
  model_DiagramRef::cast_from(_self->owner())->get_data()->notify_object_realize(_self);
}

void model_Object::ImplData::notify_will_unrealize() {
  if (!_notified_unrealize) {
    model_DiagramRef owner(model_DiagramRef::cast_from(_self->owner()));
    if (owner.is_valid() && owner->get_data()) {
      _notified_unrealize = true;
      owner->get_data()->notify_object_will_unrealize(_self);
    } else
      g_warning("will_unrealize on obj not in diagram");
  }
}

void model_Object::ImplData::set_in_view(bool flag) {
  _in_view = flag;
  if (flag)
    try_realize();
  else
    unrealize();
}

bool model_Object::ImplData::try_realize() {
  if (is_realizable()) {
    realize();
    return true;
  }
  return false;
}

void model_Object::ImplData::highlight(const Color *color) {
  if (get_canvas_item()) {
    get_canvas_item()->set_highlight_color(color);
    get_canvas_item()->set_highlighted(true);
  }
}

void model_Object::ImplData::unhighlight() {
  if (get_canvas_item())
    get_canvas_item()->set_highlighted(false);
}
