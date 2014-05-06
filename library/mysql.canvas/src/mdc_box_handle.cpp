/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "mdc_box_handle.h"

using namespace mdc;
using namespace base;

BoxHandle::BoxHandle(InteractionLayer *ilayer, CanvasItem *item, const Point &pos)
: ItemHandle(ilayer, item, pos)
{
  set_color(Color(1,1,1));
}


BoxHandle::~BoxHandle()
{
}


Rect BoxHandle::get_bounds() const
{
  Rect r;
  if (_draggable)
  {
    r.pos.x= _pos.x - 3.5;
    r.pos.y= _pos.y - 3.5;
    r.size.width= 8;
    r.size.height= 8;
  }
  else
  {
    r.pos.x= _pos.x - 2.5;
    r.pos.y= _pos.y - 2.5;
    r.size.width= 6;
    r.size.height= 6;
  }
  return r;
}
