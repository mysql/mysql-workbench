/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

// Layout contained views vertically. Width of subviews will be adjusted to the same as parent
// Height will be taken from the subviews themselves. Will resize to fit the height of all subviews.
// Compare also with the much more sophisticated vertical layout in MFBox.

#import "MVerticalLayoutView.h"

//----------------------------------------------------------------------------------------------------------------------

@interface NSView(NSView_Extras)

@property (readonly) NSSize minimumSize;
- (BOOL)expandsOnLayoutVertically: (BOOL)vertically;

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MVerticalLayoutView

- (instancetype)initWithFrame: (NSRect)frameRect {
  self = [super initWithFrame: frameRect];
  if (self != nil) {
    expandsByDefault = YES;
    minimumHeight = 100;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver: self];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setSpacing: (float)spc {
  spacing = spc;
  [self tile];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setPaddingLeft: (float)lpad right: (float)rpad top: (float)tpad bottom: (float)bpad {
  leftPadding = lpad;
  rightPadding = rpad;
  topPadding = tpad;
  bottomPadding = bpad;
  [self tile];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setExpandSubviewsByDefault: (BOOL)flag {
  expandsByDefault = flag;
  [self tile];
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)_subviewShouldExpand: (NSView*)view {
  if ([view respondsToSelector: @selector(expandsOnLayoutVertically:)])
    return [view expandsOnLayoutVertically: YES];
  else
    return expandsByDefault;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tile {
  float width = NSWidth(self.frame) - leftPadding - rightPadding;
  float availableHeight;
  float y;
  int expandable = 0;
  float extraSpacePerItem = 0;
  
  if (relayouting)
    return;
  relayouting= YES;
  
  if (self.subviews.count > 0)
    minimumHeight = spacing * (self.subviews.count-1) + topPadding + bottomPadding;
  else
    minimumHeight = 0;

  for (id view in self.subviews) {
    float minHeight;
    if (![view isHidden]) {
      BOOL expands = [self _subviewShouldExpand: view];
      
      if ([view respondsToSelector: @selector(tile)])
        [view tile];
      
      if ([view respondsToSelector: @selector(minimumSize)])
        minHeight = [view minimumSize].height;
      else
        minHeight = 1;
      
      if (!expands)
        minimumHeight += MAX(NSHeight([view frame]), minHeight);
      else {
        minimumHeight += minHeight;
        expandable++;
      }
    }
  }

  availableHeight= NSHeight(self.frame);
  if (!expandsByDefault)
    [self setFrameSize:NSMakeSize(NSWidth(self.frame), MAX(minimumHeight, availableHeight))];

  if (availableHeight > minimumHeight && expandable > 0)
    extraSpacePerItem = (availableHeight - minimumHeight) / expandable;
  else
    extraSpacePerItem = 0;
  y = NSHeight(self.frame) - topPadding;
  for (id view in self.subviews) {
    if (![view isHidden]) {
      float height = NSHeight([view frame]);
      
      if ([self _subviewShouldExpand: view]) {
        if ([view respondsToSelector: @selector(minimumSize)])
          height = MAX(extraSpacePerItem, [view minimumSize].height);
        else
          height = extraSpacePerItem;
      }

      height = MAX(1, height);

      y -= height;
      [view setFrame: NSMakeRect(leftPadding, y, width, height)];
      y -= spacing;
    }
  }
  [self setNeedsDisplay:YES];
  
  relayouting = NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)resizeWithOldSuperviewSize: (NSSize)oldBoundsSize {
  NSRect superBounds = self.superview.bounds;
  NSRect frame = self.frame;
  float leftMargin = NSMinX(frame);
  float bottomMargin = NSMinY(frame);
  float rightMargin = oldBoundsSize.width - NSMaxX(frame);
  float topMargin = oldBoundsSize.height - NSMaxY(frame);
  
  frame = NSMakeRect(leftMargin, bottomMargin, 
                     NSWidth(superBounds) - (leftMargin + rightMargin), 
                     NSHeight(superBounds) - (topMargin + bottomMargin));
  self.frame = frame;
  [self tile];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)resizeSubviewsWithOldSize: (NSSize)oldSize {
  NSRect frame= self.frame;
  if (minimumHeight > frame.size.height) {
    frame.size.height= minimumHeight;
    self.frame = frame;
  }
  [self tile];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)childFrameChanged: (NSNotification*)notif {
  [self tile];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)didAddSubview: (NSView*)subview {
  [subview setPostsFrameChangedNotifications:YES];
  [[NSNotificationCenter defaultCenter] addObserver :self
                                           selector: @selector(childFrameChanged:)
                                               name: NSViewFrameDidChangeNotification
                                             object: subview];
  
  [super didAddSubview:subview];
  
  [self tile];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setBackgroundColor: (NSColor*)color {
  backColor = color;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawRect: (NSRect)rect {
  if (backColor != nil) {
    [backColor set];
    NSRectFill(rect);
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)expandsOnLayoutVertically: (BOOL)vertically {
  if (vertically)
    return expandsByDefault;
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityGroupRole;
}

@end

//----------------------------------------------------------------------------------------------------------------------
