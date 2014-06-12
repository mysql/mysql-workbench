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



#import "MFScrollPanel.h"

#import "MFView.h"
#import "MFMForms.h"

@interface MFClipView : NSClipView
{
}

@end

// Need to override the clipview so that the layout methods can get called by content
@implementation MFClipView

- (void)subviewMinimumSizeChanged
{
  [self resizeSubviewsWithOldSize: NSMakeSize(0, 0)];
}

- (void)resizeSubviewsWithOldSize:( NSSize)oldBoundsSize
{  
  NSSize size;
  NSSize psize= [[self documentView] preferredSize];

  size.width= NSWidth([self frame]);
  size.height= [[self documentView] minimumSize].height;

  size.width = MAX(size.width, psize.width);
  size.height = MAX(size.height, psize.height);

  [[self documentView] setFrameSize: size];
}

@end


@implementation MFScrollPanelImpl

- (id)initWithObject:(::mforms::ScrollPanel*)aScrollPanel 
            bordered: (bool) bordered
     drawsBackground: (bool) drawBG
{
  self= [super initWithFrame:NSMakeRect(0, 0, 10, 20)];
  if (self)
  {
    [self setHasVerticalScroller: YES];
    [self setHasHorizontalScroller: YES];
    
    [self setContentView: [[[MFClipView alloc] initWithFrame: NSMakeRect(0, 0, 10, 20)] autorelease]];

    [self setDrawsBackground: drawBG];
    if (bordered)
      [self setBorderType: NSLineBorder];
    [self setAutohidesScrollers: YES];
    
    mOwner= aScrollPanel;
    mOwner->set_data(self);
  }
  return self;
}

//--------------------------------------------------------------------------------------------------

STANDARD_MOUSE_HANDLING(self) // Add handling for mouse events.

//--------------------------------------------------------------------------------------------------

- (NSSize)minimumSize
{
  return [NSScrollView frameSizeForContentSize:NSMakeSize(50, 50)
                         hasHorizontalScroller:YES
                           hasVerticalScroller:YES
                                    borderType:NSLineBorder];
}

- (void)subviewMinimumSizeChanged
{
  if (!mOwner->is_destroying())
  {
    NSSize minSize= [self minimumSize];
    NSSize size= [self frame].size;
    
    // size of some subview has changed, we check if our current size is enough
    // to fit it and if not, request forward the size change notification to superview
    
    if (minSize.width > size.width || minSize.height > size.height)
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
    [[self contentView] subviewMinimumSizeChanged];
  }
}

- (void) setBackgroundColor: (NSColor*) color
{
  [super setBackgroundColor: color];
}

//--------------------------------------------------------------------------------------------------

- (void) scrollIntoView: (NSView*) view
{
  [view scrollRectToVisible: [view bounds]];
}

//--------------------------------------------------------------------------------------------------

- (void)setEnabled:(BOOL)flag
{
  [[self documentView] setEnabled: flag];
}

//--------------------------------------------------------------------------------------------------

static bool scrollpanel_create(::mforms::ScrollPanel *self, mforms::ScrollPanelFlags flags)
{
  [[[MFScrollPanelImpl alloc] initWithObject:self 
                                    bordered: flags & mforms::ScrollPanelBordered
                             drawsBackground: flags & mforms::ScrollPanelDrawBackground] autorelease];
  
  return true;  
}


static void scrollpanel_add(mforms::ScrollPanel *self, mforms::View *child)
{
  MFScrollPanelImpl *panel= self->get_data();
  if (panel)
    [[panel contentView] setDocumentView: child->get_data()];
}


static void scrollpanel_remove(mforms::ScrollPanel *self)
{
  MFScrollPanelImpl *panel= self->get_data();
  if (panel)
    [panel setDocumentView:nil];
}


static void scrollpanel_set_visible_scrollers(mforms::ScrollPanel *self, bool vertical, bool horizontal)
{
  MFScrollPanelImpl *panel= self->get_data();

  [panel setHasVerticalScroller:vertical];
  [panel setHasHorizontalScroller:horizontal];
}


static void scrollpanel_set_autohide_scrollers(mforms::ScrollPanel *self, bool flag)
{
  MFScrollPanelImpl *panel= self->get_data();
  
  [panel setAutohidesScrollers: flag];
}

static void scrollpanel_scroll_to_view(mforms::ScrollPanel *self, mforms::View *child)
{
  MFScrollPanelImpl *panel= self->get_data();
  if (panel)
    [panel scrollIntoView: child->get_data()];
}

static base::Rect scrollpanel_get_content_rect(mforms::ScrollPanel *self)
{
  MFScrollPanelImpl *panel= self->get_data();
  base::Rect result;
  if (panel)
  {
    NSRect r = [panel documentVisibleRect];
    result.pos.x = NSMinX(r);
    result.pos.y = NSMinY(r);
    result.size.width = NSWidth(r);
    result.size.height = NSHeight(r);
  }
  return result;
}


static void scrollpanel_scroll_to(mforms::ScrollPanel *self, int x, int y)
{
  MFScrollPanelImpl *panel= self->get_data();
  if (panel)
    [[panel documentView] scrollPoint: NSMakePoint(x, y)];
}


void cf_scrollpanel_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_spanel_impl.create= &scrollpanel_create;
  f->_spanel_impl.add= &scrollpanel_add;
  f->_spanel_impl.remove= &scrollpanel_remove;
  f->_spanel_impl.set_visible_scrollers= &scrollpanel_set_visible_scrollers;
  f->_spanel_impl.set_autohide_scrollers= &scrollpanel_set_autohide_scrollers;
  f->_spanel_impl.scroll_to_view= &scrollpanel_scroll_to_view;
  f->_spanel_impl.get_content_rect = &scrollpanel_get_content_rect;
  f->_spanel_impl.scroll_to = &scrollpanel_scroll_to;
}


@end

