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

#ifndef __MDC_CONNECTOR_H__
#define __MDC_CONNECTOR_H__

#include "mdc_common.h"
#include "base/trackable.h"

namespace mdc {

  class Magnet;
  class CanvasItem;

  class MYSQLCANVAS_PUBLIC_FUNC Connector : public base::trackable {
  public:
    Connector(CanvasItem *owner);
    virtual ~Connector();

    void set_update_handler(const std::function<void(Connector *)> &update_handler);

    virtual bool try_connect(Magnet *magnet);
    virtual bool try_disconnect();

    virtual void connect(Magnet *magnet);
    virtual void disconnect();

    void set_draggable(bool flag);
    bool is_draggable() {
      return _draggable;
    }

    void set_tag(int tag) {
      _tag = tag;
    }
    int get_tag() {
      return _tag;
    }

    Magnet *get_connected_magnet() {
      return _magnet;
    }
    CanvasItem *get_connected_item();
    CanvasItem *get_owner() {
      return _owner;
    }

    base::Point get_position(const base::Point &srcpos);
    base::Point get_position();

    // callback for Magnet
    virtual void magnet_moved(Magnet *magnet);

  protected:
    CanvasItem *_owner;
    Magnet *_magnet;
    int _tag;

    bool _draggable;

    std::function<void(Connector *)> _update_handler;
  };

} // end of mdc namespace

#endif
