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
