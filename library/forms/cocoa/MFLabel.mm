/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
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



#import "MFLabel.h"
#import "MFMForms.h"
#import "NSColor_extras.h"

@implementation MFLabelImpl

- (id)initWithObject:(::mforms::Label*)aLabel
{
  self= [super initWithFrame:NSMakeRect(10,10,10,20)];
  if (self)
  {
    [self setDrawsBackground:NO];
    [self setBezeled:NO];
    [self setEditable:NO];
    
    [[self cell] setSelectable: YES];
    [[self cell] setWraps: NO];
    
    mOwner= aLabel;
    mOwner->set_data(self);
    mStyle= mforms::NormalStyle;
    
    mAlignment= mforms::MiddleLeft;
  }
  return self;
}


- (NSString*)description
{
  return [NSString stringWithFormat:@"<%@ '%@'>", [self className], [self stringValue]];
}

- (mforms::Object*)mformsObject
{
  return mOwner;
}


- (void)setFrame:(NSRect)frame
{
  // do vertical alignment of the textfield here
  NSSize size= [[self cell] cellSizeForBounds: frame];
  
  switch (mAlignment)
  {
    case mforms::NoAlign:
      
    case mforms::TopLeft:
    case mforms::TopCenter:
    case mforms::TopRight:
      break;
    case mforms::BottomLeft:
    case mforms::BottomRight:
    case mforms::BottomCenter:
      frame.origin.y+= (NSHeight(frame) - size.height);
      break;
    case mforms::MiddleCenter:
    case mforms::MiddleLeft:
    case mforms::MiddleRight:
    case mforms::WizardLabelAlignment:
      frame.origin.y+= (NSHeight(frame) - size.height) / 2;
      break;
  }
  [super setFrame:frame];
}


- (void)setEnabled:(BOOL)flag
{
  if (!flag)
    [self setTextColor: [NSColor darkGrayColor]];
  else
  {
    if (mStyle == mforms::SmallHelpTextStyle)
      [self setTextColor: [NSColor colorWithCalibratedRed:0.2 green:0.2 blue:0.2 alpha:1.0]];
    else
      [self setTextColor: [NSColor textColor]];
  }
  [super setEnabled: flag];
}


- (NSSize)minimumSizeForWidth:(float)width
{
  if ([[self cell] wraps])
  {
    NSRect frame;
    
    if (width == 0.0 || ([self widthIsFixed] && width > NSWidth([self frame])))
      width= NSWidth([self frame]);
    
    frame.origin= NSMakePoint(0, 0);
    frame.size.width= width;
    frame.size.height= 200;
    
    NSSize size= [[self cell] cellSizeForBounds: frame];
    
    size.width+= 1;
    
    return size;
  }
  else
  {
    NSSize size= [[self cell] cellSize];
    size.width+= 1; // magic extra to compensate rounding errors during automatic layout
    return size;    
  }
}


- (NSSize)minimumSize
{  
  return [self minimumSizeForWidth: NSWidth([self frame])];
}



- (void)setStringValue:(NSString*)text
{
  [super setStringValue: text];
  [[self superview] subviewMinimumSizeChanged];
}

    
- (void)setLabelStyle:(mforms::LabelStyle)style
{
  [self setTextColor: [NSColor textColor]];
  switch (style) 
  {
    case mforms::NormalStyle:
      [self setFont:[NSFont systemFontOfSize:[NSFont systemFontSize]]];
      break;
    case mforms::BoldStyle:
      [self setFont:[NSFont boldSystemFontOfSize:[NSFont systemFontSize]-1]];
      break;
    case mforms::SmallBoldStyle:
      [self setFont:[NSFont boldSystemFontOfSize:[NSFont smallSystemFontSize]]];
      break;      
    case mforms::BigStyle:
      [self setFont:[NSFont systemFontOfSize:15]];
      break;
    case mforms::BigBoldStyle:
      [self setFont:[NSFont boldSystemFontOfSize:15]];
      break;
    case mforms::SmallStyle:
      [self setFont:[NSFont systemFontOfSize:10]];
      break;
    case mforms::VerySmallStyle:
      [self setFont:[NSFont systemFontOfSize:8]];
      break;
    case mforms::InfoCaptionStyle:
      [self setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
      break;
    case mforms::BoldInfoCaptionStyle:
      [self setFont:[NSFont boldSystemFontOfSize:[NSFont smallSystemFontSize]]];
      break;        
    case mforms::WizardHeadingStyle:
      [self setFont:[NSFont boldSystemFontOfSize:13]];
      break;
    case mforms::SmallHelpTextStyle:
      [self setFont: [NSFont systemFontOfSize: [NSFont labelFontSize]]];
      [self setTextColor: [NSColor colorWithCalibratedRed:0.2 green:0.2 blue:0.2 alpha:1.0]];
      break;
    case mforms::VeryBigStyle:
      [self setFont:[NSFont systemFontOfSize:18]];
      break;
  }
  mStyle= style;
}


static bool label_create(::mforms::Label *self)
{
  MFLabelImpl *label= [[[MFLabelImpl alloc] initWithObject:self] autorelease];
  
  return label != nil;
}


static void label_set_text(::mforms::Label *self, const std::string &text)
{
  if ( self )
  {
    MFLabelImpl* label = self->get_data();
    
    [label setStringValue:wrap_nsstring(text)];
  }
}


static void label_set_wrap_text(::mforms::Label *self, bool flag)
{
  if ( self )
  {
    MFLabelImpl* label = self->get_data();
    
    [[label cell] setWraps: flag];
  }
}


static void label_set_text_align(::mforms::Label *self, ::mforms::Alignment alignment)
{
  if (self)
  {
    MFLabelImpl* label = self->get_data();
    
    switch (alignment)
    {
      case mforms::NoAlign:
        
      case mforms::BottomLeft:
      case mforms::MiddleLeft:
      case mforms::TopLeft:
        [label setAlignment:NSLeftTextAlignment];
        break;
      case mforms::BottomCenter:
      case mforms::TopCenter:
      case mforms::MiddleCenter:
        [label setAlignment:NSCenterTextAlignment];
        break;
      case mforms::BottomRight:
      case mforms::MiddleRight:
      case mforms::TopRight:
      case mforms::WizardLabelAlignment:
        [label setAlignment:NSRightTextAlignment];
        break;
    }
    label->mAlignment= alignment;
  }
}


static void label_set_style(mforms::Label *self, mforms::LabelStyle style)
{
  if (self)
  {
    MFLabelImpl* label = self->get_data();
  
    [label setLabelStyle: style];
  }
}


static void label_set_color(mforms::Label *self, const std::string &color)
{
  if (self)
  {
    MFLabelImpl* label = self->get_data();
    
    [label setTextColor: [NSColor colorFromHexString: [NSString stringWithUTF8String:color.c_str()]]];
  }
}

void cf_label_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_label_impl.create= &label_create;
  f->_label_impl.set_text= &label_set_text;
  f->_label_impl.set_text_align= &label_set_text_align;
  f->_label_impl.set_wrap_text= &label_set_wrap_text;
  f->_label_impl.set_style= &label_set_style;
  f->_label_impl.set_color= &label_set_color;
}



@end
