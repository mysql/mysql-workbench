/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#ifndef _MDC_CANVAS_VIEW_OPENGL_H_
#define _MDC_CANVAS_VIEW_OPENGL_H_

#include "mdc_canvas_public.h"
#include "mdc_canvas_view.h"

namespace mdc {

  MYSQLCANVAS_PUBLIC_FUNC std::string detect_opengl_version();

  class MYSQLCANVAS_PUBLIC_FUNC OpenGLCanvasView : public CanvasView {
  public:
    OpenGLCanvasView(int width, int height);
    virtual ~OpenGLCanvasView();

    virtual bool has_gl() const {
      return true;
    }

    virtual bool initialize();

    static void check_error();

    virtual void make_current() = 0;
    virtual void remove_current() = 0;
    virtual void swap_buffers() = 0;

    virtual void begin_repaint(int, int, int, int);
    virtual void end_repaint();
  };

} // end of mdc namespace

#endif /* _MDC_CANVAS_VIEW_OPENGL_H_ */
