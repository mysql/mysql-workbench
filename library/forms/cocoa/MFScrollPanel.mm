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

#import "MFScrollPanel.h"

#import "MFView.h"
#import "MFMForms.h"

@interface MFClipView : NSClipView
{
}

@end

//----------------------------------------------------------------------------------------------------------------------

// Need to override the clipview so that the layout methods can get called by content
@implementation MFClipView

- (void)resizeSubviewsWithOldSize: (NSSize)oldBoundsSize
{
  NSRect frame = self.frame;
  NSSize size = [self.documentView preferredSize: { frame.size.width, 0 }];

  size.width = frame.size.width;
  size.height = MAX(size.height, NSHeight(frame));

  [self.documentView setFrameSize: size];
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFScrollPanelImpl

- (instancetype)initWithObject:(::mforms::ScrollPanel*)aScrollPanel 
            bordered: (bool) bordered
     drawsBackground: (bool) drawBG
{
  self= [super initWithFrame:NSMakeRect(0, 0, 10, 20)];
  if (self)
  {
    [self setHasVerticalScroller: YES];
    [self setHasHorizontalScroller: YES];
    
    self.contentView = [[MFClipView alloc] initWithFrame: NSMakeRect(0, 0, 10, 20)];

    self.drawsBackground = drawBG;
    if (bordered)
      self.borderType = NSLineBorder;
    [self setAutohidesScrollers: YES];
    
    mOwner= aScrollPanel;
    mOwner->set_data(self);
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

STANDARD_MOUSE_HANDLING(self) // Add handling for mouse events.

//----------------------------------------------------------------------------------------------------------------------

- (void)relayout
{
  [self.contentView resizeSubviewsWithOldSize: self.frame.size];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)minimumSize
{
  NSSize minSize = super.minimumSize;
  NSSize contentMinSize = [NSScrollView contentSizeForFrameSize: NSMakeSize(50, 50)
                                        horizontalScrollerClass: [NSScroller class]
                                          verticalScrollerClass: [NSScroller class]
                                                     borderType: NSBezelBorder
                                                    controlSize: NSControlSizeRegular
                                                  scrollerStyle: NSScrollerStyleOverlay];
  return { MAX(minSize.width, contentMinSize.width), MAX(minSize.height, contentMinSize.height) };
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setBackgroundColor: (NSColor*) color
{
  super.backgroundColor = color;
}

//----------------------------------------------------------------------------------------------------------------------

- (void) scrollIntoView: (NSView*) view
{
  [view scrollRectToVisible: view.bounds];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setEnabled:(BOOL)flag
{
  [self.documentView setEnabled: flag];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityScrollAreaRole;
}

//----------------------------------------------------------------------------------------------------------------------

static bool scrollpanel_create(::mforms::ScrollPanel *self, mforms::ScrollPanelFlags flags)
{
  return [[MFScrollPanelImpl alloc] initWithObject: self
                                          bordered: flags & mforms::ScrollPanelBordered
                                   drawsBackground: flags & mforms::ScrollPanelDrawBackground] != nil;
}

//----------------------------------------------------------------------------------------------------------------------

static void scrollpanel_add(mforms::ScrollPanel *self, mforms::View *child)
{
  MFScrollPanelImpl *panel= self->get_data();
  if (panel)
  {
    panel.contentView.documentView = child->get_data();
    [panel.contentView resizeSubviewsWithOldSize: panel.frame.size];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void scrollpanel_remove(mforms::ScrollPanel *self)
{
  MFScrollPanelImpl *panel= self->get_data();
  if (panel)
    [panel setDocumentView:nil];
}

//----------------------------------------------------------------------------------------------------------------------

static void scrollpanel_set_visible_scrollers(mforms::ScrollPanel *self, bool vertical, bool horizontal)
{
  MFScrollPanelImpl *panel= self->get_data();

  panel.hasVerticalScroller = vertical;
  panel.hasHorizontalScroller = horizontal;
}

//----------------------------------------------------------------------------------------------------------------------

static void scrollpanel_set_autohide_scrollers(mforms::ScrollPanel *self, bool flag)
{
  MFScrollPanelImpl *panel= self->get_data();
  
  panel.autohidesScrollers = flag;
}

//----------------------------------------------------------------------------------------------------------------------

static void scrollpanel_scroll_to_view(mforms::ScrollPanel *self, mforms::View *child)
{
  MFScrollPanelImpl *panel= self->get_data();
  if (panel)
    [panel scrollIntoView: child->get_data()];
}

//----------------------------------------------------------------------------------------------------------------------

static base::Rect scrollpanel_get_content_rect(mforms::ScrollPanel *self)
{
  MFScrollPanelImpl *panel= self->get_data();
  base::Rect result;
  if (panel)
  {
    NSRect r = panel.documentVisibleRect;
    result.pos.x = NSMinX(r);
    result.pos.y = NSMinY(r);
    result.size.width = NSWidth(r);
    result.size.height = NSHeight(r);
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

static void scrollpanel_scroll_to(mforms::ScrollPanel *self, int x, int y)
{
  MFScrollPanelImpl *panel= self->get_data();
  if (panel)
    [panel.documentView scrollPoint: NSMakePoint(x, y)];
}

//----------------------------------------------------------------------------------------------------------------------

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
