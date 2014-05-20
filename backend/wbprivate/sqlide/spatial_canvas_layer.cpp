/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "spatial_canvas_layer.h"

SpatialCanvasLayer::SpatialCanvasLayer(mdc::CanvasView *view)
: mdc::Layer(view)
{
}


SpatialCanvasLayer::~SpatialCanvasLayer()
{
}


void SpatialCanvasLayer::repaint(const base::Rect &bounds)
{
  mdc::CairoCtx *cr= _owner->cairoctx();

  cr->save();
  cr->rectangle(10, 10, 50, 50);
  cr->set_color(base::Color(1,0,0));
  cr->fill();
  cr->restore();
}