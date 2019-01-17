/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#import "MFImageBox.h"
#import "MFView.h"
#import "MFMForms.h"

#include "mforms/app.h"

@implementation MFImageBoxImpl

- (instancetype)initWithObject:(::mforms::ImageBox*)aImage
{
  self= [super initWithFrame: NSMakeRect(10, 10, 10, 10)];
  if (self)
  {
    self.imageFrameStyle = NSImageFrameNone;
    mOwner= aImage;
    mOwner->set_data(self);
    mScale= NO;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (mforms::Object*)mformsObject
{
  return mOwner;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)minimumSize
{
  NSSize minSize = super.minimumSize;
  if (!mScale)
    return { MAX(minSize.width, self.image.size.width), MAX(minSize.height, self.image.size.height) };
  return minSize;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)preferredSize: (NSSize)proposal {
  return [self minimumSize];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityImageRole;
}

//----------------------------------------------------------------------------------------------------------------------

static bool imagebox_create(mforms::ImageBox *image)
{
  return [[MFImageBoxImpl alloc] initWithObject: image] != nil;
}

//----------------------------------------------------------------------------------------------------------------------

static void imagebox_set_image(mforms::ImageBox *self, const std::string &file)
{
  if (self)
  {
    MFImageBoxImpl *impl= self->get_data();

    std::string full_path = mforms::App::get()->get_resource_path(file);
    NSImage *image = [[NSImage alloc] initWithContentsOfFile: wrap_nsstring(full_path)];
    impl.image = image;
    impl.frameSize = image.size;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void imagebox_set_image_data(mforms::ImageBox *self, const char *data, size_t length)
{
  if (self)
  {
    MFImageBoxImpl *impl= self->get_data();
    NSSize oldSize= impl.frame.size;
    
    NSImage *image= [[NSImage alloc] initWithData: [NSData dataWithBytes: (void*)data length: length]];
    if (!image.valid)
      throw std::invalid_argument("Invalid image data");

    impl.image = image;
    
    if (!NSEqualSizes(image.size, oldSize))
      [impl.superview relayout];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void imagebox_set_alignment(mforms::ImageBox *self, mforms::Alignment alignment)
{
  if (self)
  {
    MFImageBoxImpl *impl= self->get_data();
    switch (alignment)
    {
      case mforms::BottomLeft:
        impl.imageAlignment = NSImageAlignBottomLeft;
        break;
      case mforms::MiddleLeft:
        impl.imageAlignment = NSImageAlignLeft;
        break;
      case mforms::TopLeft:
        impl.imageAlignment = NSImageAlignTopLeft;
        break;
      case mforms::BottomCenter:
        impl.imageAlignment = NSImageAlignBottom;
        break;
      case mforms::TopCenter:
        impl.imageAlignment = NSImageAlignTop;
        break;
      case mforms::MiddleCenter:
        impl.imageAlignment = NSImageAlignCenter;
        break;
      case mforms::BottomRight:
        impl.imageAlignment = NSImageAlignBottomRight;
        break;
      case mforms::MiddleRight:
        impl.imageAlignment = NSImageAlignRight;
        break;
      case mforms::TopRight:
        impl.imageAlignment = NSImageAlignTopRight;
        break;
      case mforms::NoAlign:
        break;
    }
  }  
}

//----------------------------------------------------------------------------------------------------------------------

static void imagebox_set_scale(mforms::ImageBox *self, bool flag)
{
  if (self)
  {
    MFImageBoxImpl *impl= self->get_data();
    impl->mScale= flag;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void cf_imagebox_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_imagebox_impl.create= &imagebox_create;
  f->_imagebox_impl.set_image= &imagebox_set_image;
  f->_imagebox_impl.set_image_data= &imagebox_set_image_data;
  f->_imagebox_impl.set_image_align= &imagebox_set_alignment;
  f->_imagebox_impl.set_scale_contents= &imagebox_set_scale;
}

@end
