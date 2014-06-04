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

SpatialCanvasLayer::SpatialCanvasLayer(mdc::CanvasView *view, GIS::SpatialHandler *handler)
: mdc::Layer(view)
{
  this->_shandler = handler;
}


SpatialCanvasLayer::~SpatialCanvasLayer()
{
  if (_shandler)
    delete _shandler;
}


void SpatialCanvasLayer::repaint(const base::Rect &bounds)
{
  if (_shandler)
  {
    GIS::ProjectionView view;
    base::Size s = _owner->get_total_view_size();
    view.height = s.height;
    view.width = s.width;
    view.MaxLat = 180;
    view.MaxLng = 90;
    view.MinLat = -180;
    view.MinLng = -90;
    std::deque<GIS::ShapeContainer> shapes;
    _shandler->getOutput(view, shapes);
    std::deque<GIS::ShapeContainer>::iterator it;
    mdc::CairoCtx *cr= _owner->cairoctx();
    cr->set_color(base::Color(1,0,0));
    cr->save();
    for(it = shapes.begin(); it != shapes.end(); it++)
    {
      if ((*it).type == GIS::ShapePolygon)
      {
        cr->move_to((*it).points[0]);
        for (size_t i = 0; i < (*it).points.size(); i++)
          cr->line_to((*it).points[i]);
        cr->stroke();
      }

    }
    cr->restore();

  }
//  mdc::CairoCtx *cr= _owner->cairoctx();
//
//  cr->save();
//  cr->rectangle(10, 10, 50, 50);
//  cr->set_color(base::Color(1,0,0));
//  cr->fill();
//  cr->restore();
}
