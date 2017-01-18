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

#ifndef __MDC_BOUNDS_MAGNET_H__
#define __MDC_BOUNDS_MAGNET_H__

#include "mdc_magnet.h"

namespace mdc {

  class Connector;
  class CanvasItem;

  class BoundsMagnet : public Magnet {
  public:
    BoundsMagnet(CanvasItem *owner);

    virtual base::Point get_position_for_connector(Connector *conn, const base::Point &srcpos) const;

  protected:
    virtual void owner_bounds_changed(const base::Rect &obounds);
  };
}

#endif
