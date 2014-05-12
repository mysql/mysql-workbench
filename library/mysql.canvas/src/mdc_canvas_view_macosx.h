/*
 *  mdc_canvas_view_macosx.h
 *  mdcanvas
 *
 *  Created by Alfredo Kojima on 07/Mar/5.
 *  Copyright 2007 MySQL AB. All rights reserved.
 *
 */

#ifndef _MDC_CANVAS_MANAGER_MACOSX_H_
#define _MDC_CANVAS_MANAGER_MACOSX_H_

#include "mdc_canvas_view.h"
#include <cairo-quartz.h>
#include <OpenGL/gl.h>

namespace mdc {
  
class QuartzCanvasView : public CanvasView 
{
public:
  QuartzCanvasView(CGContextRef cgContext, int width, int height);
  virtual ~QuartzCanvasView();
  
  void reset_context(CGContextRef cgContext);
  
  virtual bool has_gl() const { return false; }
  virtual void begin_repaint(int, int, int, int);
  virtual void end_repaint();
  
//  virtual bool initialize();
  virtual void update_view_size(int width, int height);
  
protected:
  CGContextRef _cgContext;
};



};


#endif /* _MDC_CANVAS_MANAGER_MACOSX_H_ */
