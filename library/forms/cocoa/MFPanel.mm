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

#import "MFPanel.h"
#import "NSColor_extras.h"

#import "MFMForms.h"

#import "MFRadioButton.h" // for handling radio groups

@interface MFPanelContent : NSView
{
  MFPanelImpl *panel;
  NSImage* mBackImage;
  mforms::Alignment mBackImageAlignment;
  float mLeftPadding;
  float mRightPadding;
  float mTopPadding;
  float mBottomPadding;
  float mBasePadding;

  NSTrackingArea *mTrackingArea;
}

@end


@implementation MFPanelContent

- (id)initWithPanel:(MFPanelImpl*)aPanel
{
  NSRect frame= [aPanel frame];
  frame.origin= NSMakePoint(0, 0);
  self= [super initWithFrame:frame];
  if (self)
  {
    panel= aPanel;
    mBasePadding = 4;
  }
  return self;
}

- (void)dealloc
{
  [mBackImage release];
  [super dealloc];
}

//--------------------------------------------------------------------------------------------------

STANDARD_MOUSE_HANDLING(panel) // Add handling for mouse events.

//--------------------------------------------------------------------------------------------------

- (void)setBackgroundImage: (NSString*) path withAlignment: (mforms::Alignment) align
{
  [mBackImage release];
  if (path)
    mBackImage = [[NSImage alloc] initWithContentsOfFile: path];
  else
    mBackImage = nil;
  mBackImageAlignment = align;
}

- (BOOL)isFlipped
{
  return YES;
}


- (void)setPaddingLeft:(float)lpad right:(float)rpad top:(float)tpad bottom:(float)bpad
{
  mLeftPadding = lpad;
  mRightPadding = rpad;
  mTopPadding = tpad;
  mBottomPadding = bpad;
}

- (void)setBasePadding:(float)pad
{
  mBasePadding= pad;
}


- (void)subviewMinimumSizeChanged
{
  NSSize minSize= [[[self subviews] lastObject] minimumSize];
  NSSize size= [self frame].size;
  
  // size of some subview has changed, we check if our current size is enough
  // to fit it and if not, request forward the size change notification to superview
  
  if (minSize.width != size.width || minSize.height != size.height)
    [panel subviewMinimumSizeChanged];
  else
    [[[self subviews] lastObject] setFrameSize: size];    
}


- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize
{
  id content= [[self subviews] lastObject];
  NSSize size= [self frame].size;
  size.width -= 2*mBasePadding + (mLeftPadding + mRightPadding);
  size.height -= 2*mBasePadding + (mTopPadding + mBottomPadding);
  // size.height= [content minimumSize].height;
  [content setFrame: NSMakeRect(mLeftPadding+mBasePadding, mTopPadding+mBasePadding, size.width, size.height)];
}


- (NSSize)minimumSize
{
  NSSize size= [[[self subviews] lastObject] minimumSize];
  size.width+= 2 * mBasePadding + mLeftPadding + mRightPadding;
  size.height+= 2 * mBasePadding + mTopPadding + mBottomPadding;
  return size;
}

- (void)drawRect:(NSRect)rect
{
  [super drawRect: rect];
  if (mBackImage)
  {
    float x = 0, y = 0;
    NSSize isize = [mBackImage size];
    NSSize fsize = [self frame].size;
    
    switch (mBackImageAlignment)
    {
      case mforms::BottomLeft:
        x = 0;
        y = fsize.height - isize.height;
        break;
      case mforms::BottomCenter:
        x = (isize.width+fsize.width) / 2;
        y = fsize.height - isize.height;
        break;
      case mforms::BottomRight:
        x = fsize.width - isize.width;
        y = fsize.height - isize.height;
        break;
      case mforms::MiddleLeft:
        x = 0;
        y = (isize.height+fsize.height) / 2;
        break;
      case mforms::MiddleCenter:
        x = (isize.width+fsize.width) / 2;
        y = (isize.height+fsize.height) / 2;
        break;
      case mforms::MiddleRight:
        x = fsize.width - isize.width;
        y = (isize.height+fsize.height) / 2;
        break;
      case mforms::TopLeft:
        x = 0;
        y = 0;
        break;
      case mforms::TopCenter:
        x = (isize.width+fsize.width) / 2;
        y = 0;
        break;
      case mforms::TopRight:
        x = fsize.width - isize.width;
        y = 0;
        break;
      default:
        break;
    }
    
    [mBackImage setFlipped: YES];
    [mBackImage drawInRect: NSMakeRect(x, y, isize.width, isize.height)
                  fromRect: NSZeroRect
                 operation: NSCompositeSourceOver
                  fraction: 1.0];
  }
  else if (panel->mType == mforms::StyledHeaderPanel)
  {
    [[[[NSGradient alloc] initWithStartingColor: [NSColor colorWithDeviceWhite: 0.9 alpha: 1]
  endingColor: [NSColor whiteColor]] autorelease]
    drawInRect: rect
         angle: 90];
  }
}

@end




@implementation MFPanelImpl

- (id)initWithObject:(::mforms::Panel*)aPanel type:(::mforms::PanelType)type
{
  self= [super initWithFrame:NSMakeRect(10, 10, 10, 10)];
  if (self)
  {
    NSRect frame;
    NSRect content= NSMakeRect(10, 10, 10, 10);
    float basePadding = 0;

    mOwner= aPanel;
    mOwner->set_data(self);
    mType = type;
    switch (type)
    {
      case mforms::TransparentPanel: // just a container with no background
        [self setTransparent: YES];
        [self setTitlePosition: NSNoTitle];
        break;
      case mforms::FilledHeaderPanel:
      case mforms::FilledPanel:      // just a container with color filled background
        [self setTransparent: NO];
        [self setBorderType: NSNoBorder];
        [self setTitlePosition: NSNoTitle];
        [self setBoxType: NSBoxCustom];
        break;
      case mforms::BorderedPanel:    // container with native border
        [self setBorderType: NSBezelBorder];
        [self setTitlePosition: NSNoTitle];
        basePadding = 4;
        break;
      case mforms::LineBorderPanel:  // container with a solid line border
        [self setBorderType: NSLineBorder];
        [self setTitlePosition: NSNoTitle];
        [self setBoxType: NSBoxCustom];
        basePadding = 2;
        break;
      case mforms::TitledBoxPanel:   // native grouping box with a title with border
        [self setBorderType: NSBezelBorder];
        basePadding = 4;
        break;
      case mforms::TitledGroupPanel: // native grouping container with a title (may have no border) 
        [self setBorderType: NSNoBorder];
        basePadding = 4;
        break;
       case mforms::StyledHeaderPanel: 
        [self setBorderType: NSNoBorder];
        [self setTitlePosition: NSNoTitle];
        [self setBoxType: NSBoxCustom];
        [self setTransparent: NO];
        break;
    }
    
    [self setContentViewMargins: NSMakeSize(0, 0)];
    [self setFrameFromContentFrame: content];
    frame= [self frame];
    // calculate the offsets the NSBox adds to the contentView
    mTopLeftOffset.x= NSMinX(content) - NSMinX(frame);
    mTopLeftOffset.y= NSMinY(content) - NSMinY(frame);
    mBottomRightOffset.x= NSMaxX(frame) - NSMaxX(content);
    mBottomRightOffset.y= MAX(NSMaxY(frame) - NSMaxY(content), [mCheckButton cellSize].height);

    [super setContentView: [[[MFPanelContent alloc] initWithPanel: self] autorelease]];
    [[super contentView] setBasePadding: basePadding];
  }
  return self;
}


- (void) dealloc
{
  [mCheckButton release];
  [super dealloc];
}

- (NSRect)titleRect
{
  NSRect rect;
  rect= [super titleRect];
  if (mCheckButton)
  {
    rect.origin.y-= 3;
    rect.size= [mCheckButton cellSize];
    rect.size.width+= 4;
  }
  return rect;
}

- (void)mouseDown:(NSEvent*)event
{
  if (mCheckButton)
  {
    [mCheckButton trackMouse:event inRect:[self titleRect]
                      ofView:self
                untilMouseUp:NO];
    [self setNeedsDisplay:YES];
  }
}


- (void)mouseUp:(NSEvent*)event
{
  if (mCheckButton)
  {
    [self setNeedsDisplay:YES];
  }
}


- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize
{
  NSRect bounds = [self bounds];
  bounds.size.width -= mTopLeftOffset.x + mBottomRightOffset.x;
  bounds.size.height -= mTopLeftOffset.y + mBottomRightOffset.y;
//  bounds.origin.x = mTopLeftOffset.x;
//  bounds.origin.y = mTopLeftOffset.y;
  [[self contentView] setFrame: bounds];
  [self setNeedsDisplay: YES];
}

- (mforms::Object*)mformsObject
{
  return mOwner;
}


- (NSSize)minimumSize
{
  NSSize size= [[self contentView] minimumSize];

  size.width += mTopLeftOffset.x + mBottomRightOffset.x;
  size.height += mTopLeftOffset.y + mBottomRightOffset.y;
    
  return size;
}


- (void)subviewMinimumSizeChanged
{
  NSSize minSize= [self minimumSize];
  NSSize size= [self frame].size;
  
  // size of some subview has changed, we check if our current size is enough
  // to fit it and if not, request forward the size change notification to superview
  
  if (minSize.width != size.width || minSize.height != size.height)
  {
    if ([self superview])
    {
      [[self superview] subviewMinimumSizeChanged];
      return;
    }
    else
      [self setFrameSize: minSize];
  }
  [self resizeSubviewsWithOldSize:size];
}

- (void)setPaddingLeft:(float)lpad right:(float)rpad top:(float)tpad bottom:(float)bpad
{
  [[self contentView] setPaddingLeft:lpad right:rpad top:tpad bottom:bpad];
}


- (void)setContentView:(NSView*)content
{
  if (content)
    [[self contentView] addSubview: content];
  else
    [[[[self contentView] subviews] lastObject] removeFromSuperview];
  [self subviewMinimumSizeChanged];
}


- (void)setTitle:(NSString*)title
{
  if (mCheckButton)
    [mCheckButton setTitle: title];
  else
    [super setTitle: title];
}

- (void)setEnabled:(BOOL)flag
{
  for (id view in [[self contentView] subviews])
  {
    if ([view respondsToSelector:@selector(setEnabled:)])
      [view setEnabled: flag];
  }
}

- (void)setBackgroundImage: (NSString*) path withAlignment: (mforms::Alignment) align
{
  std::string full_path= mforms::App::get()->get_resource_path([path UTF8String]);
  if (!full_path.empty())
  {
    [[self contentView] setBackgroundImage: wrap_nsstring(full_path) withAlignment: align];
  }
  else
  {
    [[self contentView] setBackgroundImage: nil withAlignment: align];
  }
}


static bool panel_create(::mforms::Panel *self, ::mforms::PanelType type)
{
  [[[MFPanelImpl alloc] initWithObject:self type:type] autorelease];
    
  return true;
}


static void panel_set_title(::mforms::Panel *self, const std::string &text)
{
  if ( self )
  {
    MFPanelImpl* panel = self->get_data();
    
    if ( panel )
    {
      [panel setTitle:wrap_nsstring(text)];
    }
  }
}



static void panel_set_back_color(mforms::Panel *self, const std::string &color)
{
  if (self)
  {
    MFPanelImpl* panel = self->get_data();
    
    if ( panel && panel->mType != mforms::StyledHeaderPanel)
    {
      [panel setTransparent: NO];
      [panel setFillColor: [NSColor colorFromHexString: wrap_nsstring(color)]];
    }
  }
}



static void panel_set_active(mforms::Panel *self, bool active)
{
  if (self)
  {
    MFPanelImpl* panel = self->get_data();
    
    if ( panel )
    {
      [panel->mCheckButton setState: active ? NSOnState : NSOffState];
    }
  }
}


static bool panel_get_active(mforms::Panel *self)
{
  if (self)
  {
    MFPanelImpl* panel = self->get_data();
    
    if ( panel )
    {
      return [panel->mCheckButton state] == NSOnState;
    }
  }
  return false;
}

static void panel_add(mforms::Panel *self,mforms::View *view)
{
  if (self)
  {
    MFPanelImpl* panel = self->get_data();
    
    if ( panel )
    {
      [panel setContentView: view->get_data()];
    }
  }
}


static void panel_remove(mforms::Panel *self, mforms::View *child)
{
  if (self)
  {
    MFPanelImpl* panel = self->get_data();
    
    if ( panel )
    {
      [panel setContentView:nil];
    }
  }
}



void cf_panel_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_panel_impl.create= &panel_create;
  f->_panel_impl.set_title= &panel_set_title;
  f->_panel_impl.set_back_color= &panel_set_back_color;
  f->_panel_impl.set_title= &panel_set_title;
  
  f->_panel_impl.set_active= &panel_set_active;
  f->_panel_impl.get_active= &panel_get_active;
  
  f->_panel_impl.add= &panel_add;
  f->_panel_impl.remove= &panel_remove;
}


@end



