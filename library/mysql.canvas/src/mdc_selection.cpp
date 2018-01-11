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

#include "mdc_selection.h"
#include "mdc_interaction_layer.h"
#include "mdc_canvas_view.h"
#include "mdc_canvas_item.h"
#include "mdc_algorithms.h"
#include "mdc_group.h"

using namespace mdc;
using namespace base;

Selection::Selection(CanvasView *view) : _view(view) {
  _block_signals = 0;
}

Selection::~Selection() {
}

void Selection::lock() {
  _mutex.lock();
}

void Selection::unlock() {
  _mutex.unlock();
}

void Selection::toggle(CanvasItem *item) {
  if (item->get_selected())
    remove(item);
  else
    add(item);
}

void Selection::set(CanvasItem *item) {
  lock();
  if (empty())
    add(item);
  else if (_items.size() == 1 && *_items.begin() == item)
    ;
  else {
    bool found = false;

    for (ContentType::iterator next, it = _items.begin(); it != _items.end(); it = next) {
      next = it;
      ++next;

      if (*it == item) {
        found = true;
        continue;
      } else
        remove(*it);
    }
    if (!found)
      add(item);
  }

  // set focus to the item
  _view->focus_item(item);

  unlock();
}

void Selection::add(CanvasItem *item) {
  if (_drag_data.empty()) {
    bool notify = false;

    lock();
    if (!item->get_selected() && item->accepts_selection()) {
      Group *group = dynamic_cast<Group *>(item->get_parent());

      // if the item is in a group, include only the group
      if (group && typeid(*group) == typeid(Group))
        add(group);
      else {
        item->set_selected(true);

        _items.insert(item);

        if (!_drag_data.empty()) {
          DragData data;
          // Surface *surf= _view->create_temp_surface(item->get_size());

          // item->render_to_surface(surf->get_surface());

          data.offset = _drag_data[0].offset - item->get_root_position();
          // data.image= surf;

          _drag_data[item] = data;
        }
        notify = true;
      }
    }
    unlock();
    if (notify)
      _signal_changed(true, item);
  }
}

void Selection::remove(CanvasItem *item) {
  if (_drag_data.empty()) {
    bool notify = false;

    lock();

    item->set_selected(false);
    if (_items.find(item) != _items.end()) {
      _items.erase(item);
      notify = true;
    }
    _drag_data.erase(item);

    unlock();

    if (notify)
      _signal_changed(false, item);
  }
}

void Selection::begin_multi_selection() {
  _old_state = _items;
  _current_selection.clear();
}

void Selection::end_multi_selection() {
  _old_state.clear();
  _current_selection.clear();

  // if there's a single item selected, focus it
  if (_items.size() == 1)
    _view->focus_item(*_items.begin());
}

void Selection::add(const std::list<CanvasItem *> &items) {
  _block_signals++;
  lock();
  for (std::list<CanvasItem *>::const_iterator i = items.begin(); i != items.end(); ++i)
    add(*i);
  unlock();
  _block_signals--;
}

void Selection::toggle(const std::list<CanvasItem *> &items) {
  ContentType new_selection;

  _block_signals++;
  lock();

  for (std::list<CanvasItem *>::const_iterator i = items.begin(); i != items.end(); ++i) {
    if (_old_state.find(*i) != _old_state.end())
      remove(*i);
    else
      add(*i);
    new_selection.insert(*i);
    _current_selection.erase(*i);
  }

  for (ContentType::iterator iter = _current_selection.begin(); iter != _current_selection.end(); ++iter)
    toggle(*iter);
  _current_selection = new_selection;

  unlock();
  _block_signals--;
}

void Selection::remove_items_outside(const Rect &rect) {
  _block_signals++;
  lock();
  for (ContentType::iterator next, it = _items.begin(); it != _items.end(); it = next) {
    next = it;
    ++next;
    if (!bounds_intersect(rect, (*it)->get_root_bounds()))
      remove(*it);
  }
  unlock();
  _block_signals--;
}

void Selection::begin_moving(const Point &mouse_pos) {
  _signal_begin_drag();

  lock();
  for (ContentType::iterator it = _items.begin(); it != _items.end(); ++it) {
    DragData data;

    data.position = (*it)->get_root_position();
    data.offset = mouse_pos - data.position;

    _drag_data[*it] = data;
  }
  _drag_data[0].offset = mouse_pos;
  unlock();
}

void Selection::update_move(const Point &mouse_pos) {
  Point snap_offset;

  lock();
  if (_view->get_grid_snapping() && !_items.empty()) {
    Point pos, npos;
    npos = pos = mouse_pos - _drag_data[*_items.begin()].offset;
    npos = _view->snap_to_grid(npos);

    snap_offset = npos - pos;
  }

  for (ContentType::const_iterator i = _items.begin(); i != _items.end(); ++i) {
    Group *group = dynamic_cast<Group *>((*i)->get_parent());
    if (!group) {
      printf("INTERNAL INCONSISTENCY: an item being moved does not have a Group parent.\n");
      // any item being moved must have an areagroup as parent, in other words they must
      // be "top level" items. Grouped items can't be moved alone, and thus selecting a
      // grouped object should mean its group is in the selection list.
      // an item contained in an already selected item should not be selected individually. (?)
      continue;
    }

    DragData &data(_drag_data[*i]);

    Point npos = mouse_pos - data.offset + snap_offset;

    if (!group->get_selected() && (*i)->is_draggable()) {
      data.position = npos;
      group->move_item(*i, data.position - group->get_root_position());
    }
  }
  unlock();
}

bool Selection::is_moving() {
  return !_drag_data.empty();
}

void Selection::end_moving() {
  _signal_end_drag();
  lock();
  // for (std::list<CanvasItem*>::const_iterator i= _items.begin(); i!= _items.end(); ++i)
  const ContentType::const_iterator last = _items.end();
  for (ContentType::const_iterator i = _items.begin(); i != last; ++i) {
    Group *group = dynamic_cast<Group *>((*i)->get_parent());
    DragData &data(_drag_data[*i]);

    if (!group->get_selected() && (*i)->is_draggable()) {
      Point position = data.position - group->get_root_position();
      group->move_item(*i, _view->snap_to_grid(position));
    }
  }
  _drag_data.clear();
  unlock();

  _view->queue_repaint();
}

/*
void Selection::render_drag_images(CairoCtx *cr)
{
  if (!_drag_data.empty())
  {
    cr->save();

    cr->set_operator(CAIRO_OPERATOR_OVER);
    for (std::map<CanvasItem*, DragData>::iterator iter= _drag_data.begin(); iter != _drag_data.end(); ++iter)
    {
      if (iter->second.image)
      {
        _view->paint_item_cache(cr, iter->second.position.x, iter->second.position.y,
          iter->second.image->get_surface(), 0.7);
      }
    }

    cr->restore();
  }
}*/

void Selection::clear(bool keep_move_info) {
  bool was_empty = empty();

  lock();
  for (ContentType::const_iterator i = _items.begin(); i != _items.end(); ++i)
    (*i)->set_selected(false);
  _items.clear();

  if (!_drag_data.empty() && keep_move_info) {
    DragData data(_drag_data[0]);
    _drag_data.clear();
    _drag_data[0] = data;
  } else
    _drag_data.clear();

  unlock();

  if (!was_empty)
    _signal_changed(false, 0);
}
