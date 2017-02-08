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

#pragma once

#include "mdc_canvas_view.h"
#include <cairo/cairo-quartz.h>
#include <OpenGL/gl.h>

namespace mdc {

  class QuartzCanvasView : public CanvasView {
  public:
    QuartzCanvasView(int width, int height);
    virtual ~QuartzCanvasView();

    void set_target_context(CGContextRef cgContext);

    virtual bool has_gl() const {
      return false;
    }
    virtual void begin_repaint(int, int, int, int);
    virtual void end_repaint();

    virtual void update_view_size(int width, int height);

  private:
    CGContextRef _context;
    cairo_surface_t *_offlineSurface;
  };
};
