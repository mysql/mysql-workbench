/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

#import "MFButton.h"
#import "MFMForms.h"

@implementation MFButtonImpl

- (instancetype)initWithObject: (mforms::Button*)aButton
                    buttonType: (mforms::ButtonType)type
{
  if (aButton == nil)
    return nil;

  self = [super initWithFrame: NSMakeRect(10, 10, 30, 30)];
  if (self)
  {
    mOwner = aButton;
    mOwner->set_data(self);
    
    [self setTitle: @""];
    switch (type)
    {
      case ::mforms::AdminActionButton:
      case ::mforms::PushButton:
        // buttons have some extra padding to the sides that we want to skip 
        mTopLeftOffset= NSMakePoint(6, 2);
        mBottomRightOffset= NSMakePoint(5, 5);

        [self setBezelStyle: NSRoundedBezelStyle];
        break;
      case ::mforms::ToolButton:
        mTopLeftOffset= NSZeroPoint;
        mBottomRightOffset= NSZeroPoint;

        [self setImagePosition: NSImageOnly];
        [self setBordered: NO];
        break;
      case ::mforms::SmallButton:
        // buttons have some extra padding to the sides that we want to skip
//        [[self cell] setControlSize: NSSmallControlSize];
        [[self cell] setFont: [NSFont systemFontOfSize: [NSFont smallSystemFontSize]]];
        [self setBezelStyle: NSRoundRectBezelStyle];
        break;
    }
    [self setTarget: self];
    [self setAction: @selector(performCallback:)];
  }
  return self;
}

-(instancetype)initWithFrame: (NSRect)frame
{
  return [self initWithObject: nil buttonType: mforms::PushButton];
}

-(instancetype)initWithCoder: (NSCoder *)coder
{
  return [self initWithObject: nil buttonType: mforms::PushButton];
}

- (NSString*)description
{
  return [NSString stringWithFormat:@"<%@ '%@'>", [self className], [self title]];
}

- (mforms::Object*)mformsObject
{
  return mOwner;
}


- (void)setFrame:(NSRect)frame
{
  if (![self widthIsFixed])
    frame.origin.x-= mTopLeftOffset.x;
  frame.origin.y-= mTopLeftOffset.y;
  
  // add back the extra padding for the button
  if (![self widthIsFixed])
    frame.size.width+= mTopLeftOffset.x + mBottomRightOffset.x;
  frame.size.height+= mTopLeftOffset.y + mBottomRightOffset.y;
  
  [super setFrame:frame];
}


- (void)performCallback:(id)sender
{
  mOwner->callback();
}

- (NSSize)minimumSize
{
  NSSize size= [[self cell] cellSize];
  if ([self imagePosition] == NSImageOnly)
  {
    size.width += 6;
    size.height += 6;
  }
  // remove the extra padding given by the cell size
  size.width-= mTopLeftOffset.x + mBottomRightOffset.x;
  size.height-= mTopLeftOffset.y + mBottomRightOffset.y;
  
  // add some internal padding to the button to make it look nicer
  if (mAddPadding)
    size.width+= size.height;
  
  return size;
}


- (void)setTitle:(NSString*)title
{
  [super setTitle: title];

  [[self superview] subviewMinimumSizeChanged];
}


static bool button_create(::mforms::Button *self, ::mforms::ButtonType type)
{
  [[[MFButtonImpl alloc] initWithObject:self buttonType:type] autorelease];
  
  return true;  
}

static void button_set_icon(::mforms::Button *self, const std::string &icon)
{
  if ( self )
  {
    MFButtonImpl* button = self->get_data();
    
    if ( button )
    {
      std::string full_path= mforms::App::get()->get_resource_path(icon);
      NSImage *image= [[[NSImage alloc] initWithContentsOfFile:wrap_nsstring(full_path)] autorelease];
      [button setImage: image];
      [button sizeToFit];
    }
  }  
}

static void button_set_text(::mforms::Button *self, const std::string &text)
{
  if ( self )
  {
    MFButtonImpl* button = self->get_data();
    
    if ( button )
    {
      [button setTitle:[wrap_nsstring(text) stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"_"]]];
    }
  }
}


static void button_enable_internal_padding(::mforms::Button *self, bool pad)
{
  if ( self )
  {
    MFButtonImpl* button = self->get_data();
    
    if ( button )
    {
      button->mAddPadding= pad;
    }
  }
}


void cf_button_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_button_impl.create= &button_create;
  f->_button_impl.set_text= &button_set_text;
  f->_button_impl.set_icon= &button_set_icon;
  f->_button_impl.enable_internal_padding= &button_enable_internal_padding;
}


@end


