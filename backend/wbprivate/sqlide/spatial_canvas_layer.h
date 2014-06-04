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

#ifndef _SPATIAL_CANVAS_LAYER_H_
#define _SPATIAL_CANVAS_LAYER_H_

#include "mdc.h"
#include "spatial_handler.h"

class SpatialCanvasLayer : public mdc::Layer
{
  GIS::SpatialHandler *_shandler;
public:
  SpatialCanvasLayer(mdc::CanvasView *view, GIS::SpatialHandler *handler);
  virtual ~SpatialCanvasLayer();

  virtual void repaint(const base::Rect &bounds);
};

#endif
