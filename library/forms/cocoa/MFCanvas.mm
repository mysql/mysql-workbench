/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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

#import "mforms/mforms.h"
#import "MCanvasScrollView.h"
#import "MCanvasViewer.h"
#include "mdc.h"

#import "MFCanvas.h"

@implementation MFCanvasImpl

- (instancetype)initWithObject:(::mforms::Canvas*)canvas
{
  self= [super initWithFrame:NSMakeRect(10,10,10,20)];
  if (self)
  {
    mCanvas = [[MCanvasViewer alloc] initWithFrame: self.bounds];

    [mCanvas setupQuartz];
    [self setContentCanvas: mCanvas];

    mOwner= canvas;
    mOwner->set_data(self);
  }
  return self;
}




static bool cf_create(mforms::Canvas *self)
{
  return [[MFCanvasImpl alloc] initWithObject: self] != nil;
}


static mdc::CanvasView *cf_canvas(mforms::Canvas *self)
{
  MFCanvasImpl *impl = self->get_data();
  return impl->mCanvas.canvas;
}


void cf_canvas_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_canvas_impl.create = cf_create;
  f->_canvas_impl.canvas = cf_canvas;
}


@end
