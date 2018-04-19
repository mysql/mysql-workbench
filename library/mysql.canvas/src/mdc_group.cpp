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

#ifndef _MSC_VER
#include <float.h>
#endif

#include "mdc_group.h"
#include "mdc_canvas_view.h"
#include "mdc_algorithms.h"
#include "mdc_interaction_layer.h"

using namespace mdc;
using namespace base;

Group::Group(Layer *layer) : Layouter(layer) {
#ifdef no_group_activate
  _activated = false;
#endif
  _freeze_bounds_updates = 0;

  set_accepts_focus(true);
  set_accepts_selection(true);

  scoped_connect(signal_focus_change(), std::bind(&Group::focus_changed, this, std::placeholders::_1, this));
}

Group::~Group() {
}

void Group::repaint(const Rect &clipArea, bool direct) {
  CairoCtx *cr = _layer->get_view()->cairoctx();
  Rect clipRect = clipArea;

  clipRect.pos = clipRect.pos - get_position();

  if (_selected) {
    Color color(0.7, 0.8, 1);
    Rect bounds = get_bounds();

    bounds.pos.x = ceil(bounds.pos.x) + 0.5;
    bounds.pos.y = ceil(bounds.pos.y) + 0.5;

    cr->save();
    cr->set_color(color, 0.75);
    cr->set_line_width(6);
    cr->rectangle(bounds);
    cr->stroke();

    cr->restore();
  }

  cr->save();
  cr->translate(get_position());
  for (std::list<CanvasItem *>::reverse_iterator iter = _contents.rbegin(); iter != _contents.rend(); ++iter) {
    if ((*iter)->get_visible() && (*iter)->intersects(clipRect))
      (*iter)->repaint(clipRect, false);
  }
  cr->restore();
}

void Group::add(CanvasItem *item) {
  Group *parent_group = dynamic_cast<Group *>(item->get_parent());

  assert(item != this);

  bool select = false;

  if (parent_group) {
    select = item->get_selected();
    parent_group->remove(item);
  }

  item->set_parent(this);

  _contents.push_front(item);
  update_bounds();

  if (select)
    item->set_selected(true);

  ItemInfo info;

  info.connection =
    item->signal_focus_change()->connect(std::bind(&Group::focus_changed, this, std::placeholders::_1, item));
  _content_info[item] = info;
}

void Group::remove(CanvasItem *item) {
  _content_info[item].connection.disconnect();

  _content_info.erase(item);

  item->set_parent(0);
  _contents.remove(item);
  update_bounds();
}

void Group::dissolve() {
  Point delta = get_position();
  Group *parent_group = dynamic_cast<Group *>(get_parent());

  if (!parent_group) {
    puts("can't dissolve group with no parent");
    return;
  }

  for (std::list<CanvasItem *>::iterator iter = _contents.begin(); iter != _contents.end(); ++iter) {
    Point pos = (*iter)->get_position();

    (*iter)->set_position(pos + delta);

    parent_group->add(*iter);
  }
}

void Group::freeze() {
  _freeze_bounds_updates++;
}

void Group::thaw() {
  assert(_freeze_bounds_updates > 0);

  _freeze_bounds_updates--;
  if (_freeze_bounds_updates == 0)
    update_bounds();
}

bool Group::has_item(CanvasItem *item) {
  return std::find(_contents.begin(), _contents.end(), item) != _contents.end();
}

void Group::move_to(const Point &point) {
  Layouter::move_to(point);
  update_bounds();
}

void Group::update_bounds() {
  if (_freeze_bounds_updates == 0) {
    std::list<CanvasItem *>::const_iterator it = _contents.begin();
    Rect rect;

    if (it != _contents.end()) {
      rect = (*it)->get_bounds();
      ++it;
    }
    while (it != _contents.end()) {
      Rect bounds = (*it)->get_bounds();
      Rect obounds = rect;

      rect.set_xmin(std::min(obounds.left(), bounds.left()));
      rect.set_ymin(std::min(obounds.top(), bounds.top()));
      rect.set_xmax(std::max(obounds.right(), bounds.right()));
      rect.set_ymax(std::max(obounds.bottom(), bounds.bottom()));
      ++it;
    }
    expand_bound(rect, 5, 5);

    //   set_position(rect.pos);
    resize_to(rect.size);
  }
}

void Group::foreach (const std::function<void(CanvasItem *)> &slot) {
  for (std::list<CanvasItem *>::const_iterator it = _contents.begin(); it != _contents.end();) {
    std::list<CanvasItem *>::const_iterator next = it;
    ++next;
    slot(*it);
    it = next;
  }
}

void Group::set_selected(bool flag) {
  if ((_selected != 0) == flag)
    return;

  _selected = flag;

  for (std::list<CanvasItem *>::const_iterator it = _contents.begin(); it != _contents.end(); ++it)
    (*it)->set_selected(flag);

  _layer->queue_repaint(get_bounds());
}

CanvasItem *Group::get_direct_subitem_at(const Point &point) {
  Point npoint = point - get_position();

  for (std::list<CanvasItem *>::const_iterator iter = _contents.begin(); iter != _contents.end(); ++iter) {
    if ((*iter)->get_visible() && (*iter)->contains_point(npoint)) {
      Group *subgroup = dynamic_cast<Group *>((*iter));
      if (subgroup) {
        CanvasItem *subitem = subgroup->get_direct_subitem_at(npoint);
        if (subitem)
          return subitem;
      }
      return (*iter);
    }
  }

  return 0;
}

CanvasItem *Group::get_other_item_at(const Point &point, CanvasItem *other_item) {
  Point npoint = point - get_position();

  for (std::list<CanvasItem *>::const_iterator iter = _contents.begin(); iter != _contents.end(); ++iter) {
    if ((*iter)->get_visible() && (*iter)->contains_point(npoint) && *iter != other_item) {
      Layouter *litem = dynamic_cast<Layouter *>(*iter);
      if (litem) {
        CanvasItem *item = litem->get_item_at(npoint);
        if (item && other_item != item)
          return item;
      }
      return *iter;
    }
  }

  return 0;
}

CanvasItem *Group::get_item_at(const Point &point) {
  return get_other_item_at(point, 0);
}

void Group::raise_item(CanvasItem *item, CanvasItem *above) {
  restack_up(_contents, item, above);
}

void Group::lower_item(CanvasItem *item) {
  restack_down(_contents, item);
}

void Group::move_item(CanvasItem *item, const Point &pos) {
  move_to(pos + get_position());
}

#ifdef no_group_activate
void Group::activate_group(bool flag) {
  if (flag != _activated) {
    if (flag) {
      _activated = true;
      get_layer()->get_view()->get_interaction_layer()->set_active_area(get_root_bounds());
    } else {
      _activated = false;
      get_layer()->get_view()->get_interaction_layer()->reset_active_area();
    }
  }
}
#endif

void Group::focus_changed(bool f, CanvasItem *item) {
  if (_parent != 0) {
    if (item == this) {
#ifdef no_group_activate
      CanvasItem *nfocused = get_layer()->get_view()->get_focused_item();
      if (!f && _activated && (!nfocused || nfocused->get_parent() != this))
        activate_group(false);
#endif
    } else if (_content_info.find(item) != _content_info.end()) {
#ifdef no_group_activate
      if (f)
        activate_group(true);
      else if (get_layer()->get_view()->get_focused_item() != this)
        activate_group(false);
#endif
    }
  }
}
