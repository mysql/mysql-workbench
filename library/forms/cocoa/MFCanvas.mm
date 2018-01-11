/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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
