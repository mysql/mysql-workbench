/*
* Copyright (c) 2007, 2015, Oracle and/or its affiliates. All rights reserved.
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

#include "mdc_canvas_view_macosx.h"

namespace mdc
{
  std::string detect_opengl_version()
  {
    return "2.0";//XXX
  }  
};

using namespace mdc;


QuartzCanvasView::QuartzCanvasView(CGContextRef cgContext, int width, int height)
  : CanvasView(width, height), _cgContext(cgContext)
{
  _crsurface= cairo_quartz_surface_create_for_cg_context(_cgContext, _view_width, _view_height);
  _cairo= new CairoCtx(_crsurface);
}


QuartzCanvasView::~QuartzCanvasView()
{
}


void QuartzCanvasView::reset_context(CGContextRef cgContext)
{
  delete _cairo;
  cairo_surface_destroy(_crsurface);
  
  _cgContext= cgContext;
  
  _crsurface= cairo_quartz_surface_create_for_cg_context(_cgContext, _view_width, _view_height);
  _cairo= new CairoCtx(_crsurface);
}


void QuartzCanvasView::update_view_size(int width, int height)
{
  if (_view_width != width || _view_height != height)
  {
    _view_width= width;
    _view_height= height;
    
    delete _cairo;
    cairo_surface_destroy(_crsurface);
    
    _crsurface= cairo_quartz_surface_create_for_cg_context(_cgContext, width, height);
    _cairo= new CairoCtx(_crsurface);
    
    update_offsets();
    queue_repaint();
    
    _viewport_changed_signal();
  }
}


void QuartzCanvasView::begin_repaint(int, int, int, int)
{
}


void QuartzCanvasView::end_repaint()
{
}

