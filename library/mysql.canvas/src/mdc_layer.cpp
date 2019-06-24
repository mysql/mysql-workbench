/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mdc_layer.h"
#include "mdc_canvas_view.h"
#include "mdc_algorithms.h"
#include "mdc_draw_util.h"
#include "mdc_item_handle.h"
#include "mdc_area_group.h"
#include "mdc_selection.h"

using namespace mdc;
using namespace base;

Layer::Layer(CanvasView *view) : _owner(view) {
  _visible = true;
  _needs_repaint = true;

  _root_area = new AreaGroup(this);
  _root_area->resize_to(_owner->get_total_view_size());
  _root_area->set_accepts_focus(false);
  _root_area->set_accepts_selection(false);
  _root_area->set_draw_background(false);

  scoped_connect(view->signal_resized(), std::bind(&Layer::view_resized, this));
}

void Layer::set_root_area(AreaGroup *group) {
  delete _root_area;
  _root_area = group;
  _root_area->set_cache_toplevel_contents(false);
  _root_area->resize_to(_owner->get_total_view_size());
}

Layer::~Layer() {
  delete _root_area;
}

void Layer::set_name(const std::string &name) {
  _name = name;
}

void Layer::view_resized() {
  _root_area->resize_to(_owner->get_total_view_size());
}

void Layer::set_visible(bool flag) {
  if (_visible != flag) {
    _visible = flag;
    if (flag)
      queue_repaint();
    _owner->queue_repaint();
  }
}

void Layer::add_item(CanvasItem *item, AreaGroup *location) {
  get_view()->lock();

  if (!location)
    _root_area->add(item);
  else
    location->add(item);

  item->set_needs_relayout();

  get_view()->unlock();

  queue_repaint();
}

void Layer::remove_item(CanvasItem *item) {
  get_view()->get_selection()->remove(item);

  if (item->get_parent())
    dynamic_cast<Layouter *>(item->get_parent())->remove(item);

  std::list<CanvasItem *>::iterator iter = std::find(_relayout_queue.begin(), _relayout_queue.end(), item);
  if (iter != _relayout_queue.end())
    _relayout_queue.erase(iter);

  queue_repaint();
}

static void invalidate(CanvasItem *item) {
  item->invalidate_cache();
  Layouter *l = dynamic_cast<Layouter *>(item);
  if (l)
    l->foreach(std::bind(&invalidate, std::placeholders::_1));
}

void Layer::invalidate_caches() {
  _root_area->foreach(std::bind(&invalidate, std::placeholders::_1));
}

void Layer::set_needs_repaint_all_items() {
  _root_area->foreach (std::bind(&CanvasItem::set_needs_repaint, std::placeholders::_1));
}

void Layer::repaint_pending() {
  if (_needs_repaint) {
    // XXX record pending areas and repaint only what's needed
    repaint(Rect(Point(0, 0), _owner->get_total_view_size()));
    _needs_repaint = false;
  }
}

void Layer::repaint(const Rect &bounds) {
  for (std::list<CanvasItem *>::iterator iter = _relayout_queue.begin(); iter != _relayout_queue.end(); ++iter) {
    (*iter)->relayout();
  }
  _relayout_queue.clear();

  if (_visible)
    _root_area->repaint(bounds, false);
}

void Layer::repaint_for_export(const Rect &aBounds) {
  for (std::list<CanvasItem *>::iterator iter = _relayout_queue.begin(); iter != _relayout_queue.end(); ++iter) {
    (*iter)->relayout();
  }
  _relayout_queue.clear();

  if (_visible)
    _root_area->repaint(aBounds, true);
}

//--------------------------------------------------------------------------------------------------

void Layer::queue_repaint() {
  _needs_repaint = true;
  _owner->queue_repaint();
}

//--------------------------------------------------------------------------------------------------

void Layer::queue_repaint(const Rect &bounds) {
  _needs_repaint = true;
  _owner->queue_repaint(bounds);
}

//--------------------------------------------------------------------------------------------------

void Layer::queue_relayout(CanvasItem *item) {
  if (!item->is_toplevel())
    throw std::logic_error("trying to queue non-toplevel item for relayout");

  if (std::find(_relayout_queue.begin(), _relayout_queue.end(), item) == _relayout_queue.end()) {
    queue_repaint();
    _relayout_queue.push_back(item);
  }
}

CanvasItem *Layer::get_other_item_at(const Point &point, CanvasItem *item) {
  return _root_area->get_other_item_at(point, item);
}

CanvasItem *Layer::get_item_at(const Point &point) {
  return _root_area->get_item_at(point);
}

CanvasItem *Layer::get_top_item_at(const Point &point) {
  return _root_area->get_direct_subitem_at(point);
}

static std::list<CanvasItem *> get_items_bounded_by(const Rect &rect, const Layer::ItemCheckFunc &pred, Group *group) {
  std::list<CanvasItem *> &items = group->get_contents();
  std::list<CanvasItem *> result;

  for (std::list<CanvasItem *>::iterator iter = items.begin(); iter != items.end(); ++iter) {
    Group *g;

    if (bounds_intersect((*iter)->get_root_bounds(), rect) && (!pred || pred(*iter)))
      result.push_back(*iter);

    g = dynamic_cast<Group *>(*iter);
    if (g && bounds_intersect(g->get_root_bounds(), rect)) {
      std::list<CanvasItem *> tmp = get_items_bounded_by(rect, pred, g);

      result.insert(result.end(), tmp.begin(), tmp.end());
    }
  }
  return result;
}

std::list<CanvasItem *> Layer::get_items_bounded_by(const Rect &rect, const ItemCheckFunc &pred,
                                                    mdc::Group *inside_group) {
  if (!inside_group)
    inside_group = _root_area;
  return ::get_items_bounded_by(rect, pred, inside_group);
}

Rect Layer::get_bounds_of_item_list(const std::list<CanvasItem *> &items) {
  std::list<CanvasItem *>::const_iterator it = items.begin();
  Rect rect;

  if (it != items.end()) {
    rect = (*it)->get_bounds();
    ++it;
  }
  while (it != items.end()) {
    Rect bounds = (*it)->get_bounds();
    Rect obounds = rect;

    rect.set_xmin(std::min(obounds.left(), bounds.left()));
    rect.set_ymin(std::min(obounds.top(), bounds.top()));
    rect.set_xmax(std::max(obounds.right(), bounds.right()));
    rect.set_ymax(std::max(obounds.bottom(), bounds.bottom()));
    ++it;
  }

  return rect;
}

Group *Layer::create_group_with(const std::list<CanvasItem *> &contents) {
  if (contents.size() <= 1)
    return 0;

  Rect bounds = get_bounds_of_item_list(contents);

  Group *group = new Group(this);
  group->set_position(bounds.pos);

  group->freeze();

  for (std::list<CanvasItem *>::const_reverse_iterator iter = contents.rbegin(); iter != contents.rend(); ++iter) {
    group->add(*iter);
    (*iter)->set_position((*iter)->get_position() - bounds.pos);
  }

  group->thaw();

  add_item(group);

  queue_repaint(group->get_bounds());

  return group;
}

AreaGroup *Layer::create_area_group_with(const std::list<CanvasItem *> &contents) {
  if (contents.size() <= 1)
    return 0;

  Rect bounds = get_bounds_of_item_list(contents);

  bounds = expand_bound(bounds, 20, 20);

  AreaGroup *group = new AreaGroup(this);
  group->set_position(bounds.pos);
  group->resize_to(bounds.size);

  //  group->freeze();

  for (std::list<CanvasItem *>::const_reverse_iterator iter = contents.rbegin(); iter != contents.rend(); ++iter) {
    _root_area->remove(*iter);
    group->add(*iter);
    (*iter)->set_position((*iter)->get_position() - bounds.pos);
  }

  //  group->thaw();

  _root_area->add(group);

  group->set_needs_render();
  queue_repaint();

  //  set_needs_repaint(group->get_bounds());

  return group;
}

void Layer::dissolve_group(Group *group) {
  group->dissolve();
  remove_item(group);
  delete group;
}
