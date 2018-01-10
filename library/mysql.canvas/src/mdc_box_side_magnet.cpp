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

#include "mdc_canvas_item.h"
#include "mdc_box_side_magnet.h"
#include "mdc_connector.h"

using namespace mdc;
using namespace base;

BoxSideMagnet::BoxSideMagnet(CanvasItem *owner) : Magnet(owner) {
  for (size_t i = 0; i < sizeof(_counts) / sizeof(*_counts); i++)
    _counts[i] = 0;
}

void BoxSideMagnet::set_compare_slot(const std::function<bool(Connector *, Connector *, Side)> &compare) {
  _compare = compare;
}

double BoxSideMagnet::constrain_angle(double angle) const { /*
                                                             switch (_side)
                                                             {
                                                             case Top:
                                                               return 90;
                                                             case Bottom:
                                                               return 270;
                                                             case Left:
                                                               return 180;
                                                             case Right:
                                                               return 0;
                                                             }*/
  return angle;
}

void BoxSideMagnet::set_connector_side(Connector *conn, Side side) {
  Side oside = Unknown;
  bool changed = false;

  if (_connector_info.find(conn) != _connector_info.end()) {
    oside = _connector_info[conn];

    if (oside != side)
      changed = true;

    _counts[oside]--;
  } else
    changed = true;
  _counts[side]++;

  _connector_info[conn] = side;

  // reorder the connectors
  if (_compare) {
    bool ok = false;
    std::list<Connector *>::iterator prev = _connectors.begin();
    for (std::list<Connector *>::iterator iter = _connectors.begin(); iter != _connectors.end(); ++iter) {
      if (get_connector_side(*iter) == side) {
        if (*iter != conn && !_compare(*iter, conn, side)) {
          // the comparison callback is not strictly ordered
          if (_compare(*iter, conn, side) == _compare(conn, *iter, side))
            throw std::logic_error("magnet comparison callback is not strictly ordered");

          if (*prev != conn) {
            _connectors.remove(conn);
            _connectors.insert(iter, conn);
            changed = true;
          }
          ok = true;
          break;
        }
        prev = iter;
      }
    }
    if (!ok && conn != _connectors.back()) {
      changed = true;
      _connectors.remove(conn);
      _connectors.push_back(conn);
    }
  }

  if (changed) {
    if (oside != Unknown && oside != side)
      notify_connectors(oside);
    notify_connectors(side);
  }
}

BoxSideMagnet::Side BoxSideMagnet::get_connector_side(Connector *conn) const {
  std::map<Connector *, Side>::const_iterator iter;

  if ((iter = _connector_info.find(conn)) != _connector_info.end())
    return iter->second;

  return Unknown;
}

double BoxSideMagnet::connector_position(Side side, Connector *conn, double length) const {
  size_t pos = 0;

  for (std::list<Connector *>::const_iterator iter = _connectors.begin(); iter != _connectors.end(); ++iter) {
    if (*iter == conn)
      break;

    if (get_connector_side(*iter) == side)
      ++pos;
  }

  return (length / (_counts[side] + 1)) * (pos + 1);
}

Point BoxSideMagnet::get_position_for_connector(Connector *conn, const Point &srcpos) const {
  Rect bounds(_owner->get_root_bounds());
  Point pos;
  Side side;

  switch ((side = get_connector_side(conn))) {
    case Unknown:
      break;
    case Top:
      pos.y = bounds.top();
      if (conn && !_connector_info.empty())
        pos.x = bounds.left() + connector_position(side, conn, bounds.width());
      else
        pos.x = bounds.xcenter();
      break;
    case Bottom:
      pos.y = _owner->get_root_bounds().bottom();
      if (conn && !_connector_info.empty())
        pos.x = bounds.left() + connector_position(side, conn, bounds.width());
      else
        pos.x = bounds.xcenter();
      break;
    case Left:
      pos.x = _owner->get_root_bounds().left();
      if (conn && !_connector_info.empty())
        pos.y = bounds.top() + connector_position(side, conn, bounds.height());
      else
        pos.y = bounds.ycenter();
      break;
    case Right:
      pos.x = _owner->get_root_bounds().right();
      if (conn && !_connector_info.empty())
        pos.y = bounds.top() + connector_position(side, conn, bounds.height());
      else
        pos.y = bounds.ycenter();
      break;
  }

  return pos;
}

void BoxSideMagnet::remove_connector(Connector *conn) {
  Magnet::remove_connector(conn);
  _counts[_connector_info[conn]]--;
  _connector_info.erase(_connector_info.find(conn));
}

void BoxSideMagnet::notify_connectors(Side side) {
  for (std::map<Connector *, Side>::iterator iter = _connector_info.begin(); iter != _connector_info.end(); ++iter) {
    if (iter->second == side)
      iter->first->magnet_moved(this);
  }
}

void BoxSideMagnet::reorder_connector_closer_to(Connector *conn, const Point &pos) {
  Rect bounds(_owner->get_root_bounds());
  Side side = get_connector_side(conn);
  int order, i;

  switch (side) {
    case Top:
    case Bottom:
      order = (int)((pos.x - bounds.left()) / (bounds.width() / (_counts[side] + 1)));
      break;
    case Left:
    case Right:
      order = (int)((pos.y - bounds.top()) / (bounds.height() / (_counts[side] + 1)));
      break;
    default:
      return;
  }

  i = 0;
  for (std::list<Connector *>::iterator iter = _connectors.begin(); iter != _connectors.end(); ++iter) {
    if (get_connector_side(*iter) == side) {
      if (i == order) {
        if (*iter == conn)
          return;

        _connectors.remove(conn);
        _connectors.insert(iter, conn);
        break;
      }
      ++i;
    }
  }
}

/*
*********************************************************************************
* @brief Reorder connectors in the same side in their "natural" order.
*
* If a ordering callback is set, connectors that are in the same side will
* be ordered according to that.
*********************************************************************************
*/
void BoxSideMagnet::reorder_connectors() {
  if (_compare) {
    // since we only have 1 list of connectors for all 4 sides
    // we do comparison in 2 steps, 1st by side and then by the callback

    _connectors.sort(CompareConnectors(this));
  }
}
