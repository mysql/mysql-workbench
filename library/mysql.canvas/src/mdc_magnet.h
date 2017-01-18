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

#ifndef __MDC_MAGNET_H__
#define __MDC_MAGNET_H__

#include "mdc_canvas_public.h"
#include "base/trackable.h"

namespace mdc {

  class Connector;
  class CanvasItem;

  class MYSQLCANVAS_PUBLIC_FUNC Magnet : public base::trackable {
  public:
    Magnet(CanvasItem *owner);
    virtual ~Magnet();

    virtual bool allows_connection(Connector *conn) const;
    virtual bool allows_disconnection(Connector *conn) const;

    void remove_all_connectors();

    virtual bool add_connector(Connector *conn);
    virtual void remove_connector(Connector *conn);

    virtual base::Point get_position_for_connector(Connector *conn, const base::Point &srcpos) const;
    virtual base::Point get_position() const;

    virtual double constrain_angle(double angle) const {
      return angle;
    }

    void set_connection_validator(const std::function<bool(Connector *)> &slot);
    void set_disconnection_validator(const std::function<bool(Connector *)> &slot);

    CanvasItem *get_owner() const {
      return _owner;
    }

  protected:
    CanvasItem *_owner;

    std::list<Connector *> _connectors;

    std::function<bool(Connector *)> _connection_slot;
    std::function<bool(Connector *)> _disconnection_slot;

    virtual void notify_connected();

    void owner_bounds_changed(const base::Rect &obounds);
    virtual void owner_parent_bounds_changed(CanvasItem *item, const base::Rect &obounds);
  };

} // end of mdc namespace

#endif
