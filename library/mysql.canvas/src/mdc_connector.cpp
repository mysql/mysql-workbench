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

#include "mdc_connector.h"
#include "mdc_magnet.h"
#include "mdc_canvas_item.h"

using namespace mdc;
using namespace base;

Connector::Connector(CanvasItem *owner) : _owner(owner), _magnet(0), _tag(0) {
  _draggable = true;
}

void Connector::set_update_handler(const std::function<void(Connector *)> &update_handler) {
  _update_handler = update_handler;
}

Connector::~Connector() {
  if (_magnet)
    _magnet->remove_connector(this);
}

void Connector::set_draggable(bool flag) {
  _draggable = flag;
}

void Connector::connect(Magnet *magnet) {
  if (_magnet)
    throw std::logic_error("connecting an already connected connector");

  magnet->add_connector(this);
  _magnet = magnet;
  magnet_moved(magnet);
}

bool Connector::try_connect(Magnet *magnet) {
  if (_magnet == magnet)
    return true;

  if (magnet->allows_connection(this)) {
    connect(magnet);
    return true;
  }
  return false;
}

bool Connector::try_disconnect() {
  if (_magnet != 0) {
    if (_magnet->allows_disconnection(this)) {
      disconnect();
      return true;
    }
  }
  return false;
}

void Connector::magnet_moved(Magnet *magnet) {
  if (_update_handler)
    _update_handler(this);
}

Point Connector::get_position(const Point &srcpos) {
  // returns the position that the connector should be
  if (_magnet)
    return _magnet->get_position_for_connector(this, srcpos);

  return Point();
}

Point Connector::get_position() {
  // returns the position that the connector should be
  if (_magnet)
    return _magnet->get_position();

  return Point();
}

void Connector::disconnect() {
  if (_magnet) {
    _magnet->remove_connector(this);
    _magnet = 0;
  }
}

mdc::CanvasItem *Connector::get_connected_item() {
  if (_magnet)
    return _magnet->get_owner();
  return 0;
}
