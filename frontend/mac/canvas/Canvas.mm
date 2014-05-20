/*
 * Copyright (c) 2014 Oracle and/or its affiliates. All rights reserved.
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

#import "mforms/canvas.h"
#import "MCanvasScrollView.h"
#import "MCanvasViewer.h"
#include "mdc.h"

using namespace mforms;


namespace mforms
{
  class ConcreteCanvas : public mforms::Canvas
  {
    MCanvasScrollView *scroller;
    MCanvasViewer *viewer;

  public:
    ConcreteCanvas()
    {
      scroller = [[MCanvasScrollView alloc] initWithFrame: NSMakeRect(0, 0, 100, 100)];
      viewer = [[MCanvasViewer alloc] initWithFrame: NSMakeRect(0, 0, 100, 100)];

      [viewer setupQuartz];
      [scroller setContentCanvas: [viewer autorelease]];
      
      set_data(scroller);
    }

    virtual ~ConcreteCanvas()
    {
      [scroller release];
    }

    virtual mdc::CanvasView *canvas()
    {
      return [viewer canvas];
    }
  };
};


static Canvas* create_canvas()
{
  return new ConcreteCanvas();
}


void cf_canvas_init()
{
  mforms::Canvas::register_factory(create_canvas);
}
