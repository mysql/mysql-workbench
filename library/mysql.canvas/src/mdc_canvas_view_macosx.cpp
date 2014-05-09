/*
 *  mdc_canvas_view_macosx.cpp
 *  mdcanvas
 *
 *  Created by Alfredo Kojima on 07/Mar/5.
 *  Copyright 2007 MySQL AB. All rights reserved.
 *
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

