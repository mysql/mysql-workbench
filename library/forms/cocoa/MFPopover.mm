/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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

#import "MFPopover.h"
#import "MFMForms.h"

//----------------------------------------------------------------------------------------------------------------------

@interface PopoverFrameView : NSView {
  NSBezierPath* outline;
@public
  BOOL readOnly;
}

@property(nonatomic, strong) NSBezierPath* outline;

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation PopoverFrameView

@synthesize outline;

- (void)drawRect: (NSRect)rect {
  NSGraphicsContext* context = [NSGraphicsContext currentContext];
  [context saveGraphicsState];

  [NSColor.windowBackgroundColor set];
  [outline fill];

  [context restoreGraphicsState];
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFPopover

- (instancetype)initWithContentRect: (NSRect)contentRect
                          styleMask: (NSUInteger)windowStyle
                            backing: (NSBackingStoreType)bufferingType
                              defer: (BOOL)deferCreation
                              style: (mforms::PopoverStyle)style {
  self = [super initWithContentRect:contentRect styleMask: windowStyle backing: bufferingType defer: deferCreation];
  if (self) {
    PopoverFrameView* frameView = super.contentView;
    frameView->readOnly = style == mforms::PopoverStyleTooltip;
    mStyle = style;
    switch (mStyle) {
      case mforms::PopoverStyleTooltip:
        mArrowSize = 8;
        mArrowBase = 16;
        mCornerRadius = 0;
        break;

      default:
        mArrowSize = 16;
        mArrowBase = 32;
        mCornerRadius = 10;
        break;
    }
    [self setOpaque:NO];
    self.backgroundColor = [NSColor clearColor];
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  [NSObject cancelPreviousPerformRequestsWithTarget: self];
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)canBecomeKeyWindow {
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Resize the frame view to accomodate the content view.
 */
- (void)setContentSize: (NSSize)newSize {
  mBaseSize = newSize;

  NSSize sizeDelta = newSize;
  NSSize childBoundsSize = mChildContentView.bounds.size;
  sizeDelta.width -= childBoundsSize.width;
  sizeDelta.height -= childBoundsSize.height;

  PopoverFrameView* frameView = super.contentView;
  NSSize newFrameSize = frameView.bounds.size;
  newFrameSize.width += sizeDelta.width;
  newFrameSize.height += sizeDelta.height;

  [super setContentSize: newFrameSize];
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Set the given view as content view, that is, as child of our frame view.
 */
- (void)setContentView: (NSView*)aView {
  if ([mChildContentView isEqualTo: aView])
    return;

  NSRect bounds = self.frame;
  bounds.origin = NSZeroPoint;

  PopoverFrameView* frameView = super.contentView;
  if (!frameView) {
    frameView = [[PopoverFrameView alloc] initWithFrame: bounds];
    frameView->readOnly = mStyle == mforms::PopoverStyleTooltip;
    super.contentView = frameView;
  }

  if (mChildContentView != nil)
    [mChildContentView removeFromSuperview];

  mChildContentView = aView; // No retain required here, the frame view controls the lifetime.
  mChildContentView.frame = [self contentRectForFrameRect: bounds];
  mChildContentView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  [frameView addSubview:mChildContentView];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSView*)contentView {
  return mChildContentView;
}

//----------------------------------------------------------------------------------------------------------------------

#define DEFAULT_PADDING 7 // Padding on all sides.

- (NSRect)contentRectForFrameRect: (NSRect)windowFrame {
  return NSMakeRect(windowFrame.origin.x + mPadding.left, windowFrame.origin.y + mPadding.bottom,
                    windowFrame.size.width - mPadding.horizontal(), windowFrame.size.height - mPadding.vertical());
}

//----------------------------------------------------------------------------------------------------------------------

- (void)computeCoordinatesAndPadding: (mforms::StartPosition)position {
  // The base size is the size of the main part, without arrow.
  NSSize actualSize = mBaseSize;
  actualSize.width += 2 * DEFAULT_PADDING;
  actualSize.height += 2 * DEFAULT_PADDING;

  // Add the arrow size to either width or height, depending on the proposed relative position.
  if (position == mforms::StartLeft || position == mforms::StartRight)
    actualSize.width += mArrowSize;
  else
    actualSize.height += mArrowSize;

  // The initial position of the arrow is not the center on its side but only 1/3 of side's size
  // for a more appealing look. Additionally, add the arrow's size to the padding on this size to
  // exclude its area from the main content area.
  NSPoint newLocation;
  switch (position) {
    case mforms::StartLeft:
      newLocation.x = mHotSpot.x - actualSize.width;
      newLocation.y = mHotSpot.y - actualSize.height / 3;
      mPadding = base::Padding(DEFAULT_PADDING, DEFAULT_PADDING, DEFAULT_PADDING + mArrowSize, DEFAULT_PADDING);
      break;
    case mforms::StartRight:
      newLocation.x = mHotSpot.x;
      newLocation.y = mHotSpot.y - actualSize.height / 3;
      mPadding = base::Padding(DEFAULT_PADDING + mArrowSize, DEFAULT_PADDING, DEFAULT_PADDING, DEFAULT_PADDING);
      break;
    case mforms::StartAbove:
      newLocation.x = mHotSpot.x - actualSize.width / 3;
      newLocation.y = mHotSpot.y;
      mPadding = base::Padding(DEFAULT_PADDING, DEFAULT_PADDING, DEFAULT_PADDING, DEFAULT_PADDING + mArrowSize);
      break;
    case mforms::StartBelow:
      newLocation.x = mHotSpot.x - actualSize.width / 3;
      newLocation.y = mHotSpot.y - actualSize.height;
      mPadding = base::Padding(DEFAULT_PADDING, DEFAULT_PADDING + mArrowSize, DEFAULT_PADDING, DEFAULT_PADDING);
      break;
  }

  NSScreen* currentScreen = [NSScreen mainScreen];
  NSRect screenBounds = currentScreen.visibleFrame;

  // Check the control's bounds and determine the amount of pixels we have to move it make
  // it fully appear on screen. This will usually not move the hot spot, unless the movement
  // of the control is so much that it would leave the arrow outside its bounds.
  int deltaX = 0;
  int deltaY = 0;
  NSRect frame = self.frame;
  if (newLocation.x < screenBounds.origin.x)
    deltaX = screenBounds.origin.x - newLocation.x;
  if (newLocation.x + frame.size.width > NSMaxX(screenBounds))
    deltaX = NSMaxX(screenBounds) - (newLocation.x + frame.size.width);

  if (newLocation.y < NSMinY(screenBounds))
    deltaY = NSMinY(screenBounds) - newLocation.y;
  if (newLocation.y + frame.size.height > NSMaxY(screenBounds))
    deltaY = NSMaxY(screenBounds) - (newLocation.y + frame.size.height);
  newLocation.x += deltaX;
  newLocation.y += deltaY;

  // Now that we have the final location check the arrow again.
  switch (position) {
    case mforms::StartLeft:
    case mforms::StartRight:
      mHotSpot.x += deltaX;
      if ((mHotSpot.y - mArrowBase / 2) < (newLocation.y + mCornerRadius))
        mHotSpot.y = newLocation.y + mCornerRadius + mArrowBase / 2;
      if ((mHotSpot.y + mArrowBase / 2) > (newLocation.y + actualSize.height - mCornerRadius))
        mHotSpot.y = newLocation.y + actualSize.height - mCornerRadius - mArrowBase / 2;
      break;
    case mforms::StartAbove:
    case mforms::StartBelow:
      if ((mHotSpot.x - mArrowBase / 2) < (newLocation.x + mCornerRadius))
        mHotSpot.x = newLocation.x + mCornerRadius + mArrowBase / 2;
      if ((mHotSpot.x + mArrowBase / 2) > (newLocation.x + actualSize.width - mCornerRadius))
        mHotSpot.x = newLocation.x + actualSize.width - mCornerRadius - mArrowBase / 2;
      mHotSpot.y += deltaY;
      break;
  }

  [self setFrameOrigin:newLocation];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)computeOutline {
  PopoverFrameView* frameView = super.contentView;

  NSRect bounds = frameView.bounds;
  NSPoint localHotSpot = [self convertRectFromScreen: NSMakeRect(mHotSpot.x, mHotSpot.y, 0, 0)].origin;
  localHotSpot.x += 0.5;

  // The path is constructed counterclockwise.
  NSBezierPath* outline = [NSBezierPath bezierPath];
  outline.lineWidth = 1;

  CGFloat leftOffset = 0;
  CGFloat topOffset = 0;
  CGFloat rightOffset = 0;
  CGFloat bottomOffset = 0;

  switch (mRelativePosition) {
    case mforms::StartLeft: {
      rightOffset = mArrowSize;
      break;
    }
    case mforms::StartRight: {
      leftOffset = mArrowSize;
      break;
    }
    case mforms::StartAbove: {
      bottomOffset = mArrowSize;
      break;
    }
    case mforms::StartBelow: {
      topOffset = mArrowSize;
      break;
    }
  }

  // Left-bottom corner.
  [outline appendBezierPathWithArcWithCenter: NSMakePoint(NSMinX(bounds) + mCornerRadius + leftOffset,
                                                          NSMinY(bounds) + mCornerRadius + bottomOffset)
                                      radius: mCornerRadius
                                  startAngle: 180
                                    endAngle: -90];
  if (bottomOffset > 0) {
    [outline lineToPoint:NSMakePoint(localHotSpot.x - mArrowBase / 2, NSMinY(bounds) + bottomOffset)];
    [outline lineToPoint:NSMakePoint(localHotSpot.x, localHotSpot.y)];
    [outline lineToPoint:NSMakePoint(localHotSpot.x + mArrowBase / 2, NSMinY(bounds) + bottomOffset)];
  }

  // Right-bottom corner.
  [outline appendBezierPathWithArcWithCenter: NSMakePoint(NSMaxX(bounds) - mCornerRadius - rightOffset,
                                                          NSMinY(bounds) + mCornerRadius + bottomOffset)
                                      radius: mCornerRadius
                                  startAngle:- 90
                                    endAngle: 0];
  if (rightOffset > 0) {
    [outline lineToPoint:NSMakePoint(NSMaxX(bounds) - rightOffset, localHotSpot.y - mArrowBase / 2)];
    [outline lineToPoint:localHotSpot];
    [outline lineToPoint:NSMakePoint(NSMaxX(bounds) - rightOffset, localHotSpot.y + mArrowBase / 2)];
  }

  // Right-top corner.
  [outline appendBezierPathWithArcWithCenter: NSMakePoint(NSMaxX(bounds) - mCornerRadius - rightOffset,
                                                          NSMaxY(bounds) - mCornerRadius - topOffset)
                                      radius: mCornerRadius
                                  startAngle: 0
                                    endAngle: 90];
  if (topOffset > 0) {
    [outline lineToPoint:NSMakePoint(localHotSpot.x + mArrowBase / 2, NSMaxY(bounds) - topOffset)];
    [outline lineToPoint:localHotSpot];
    [outline lineToPoint:NSMakePoint(localHotSpot.x - mArrowBase / 2, NSMaxY(bounds) - topOffset)];
  }

  // Left-top corner.
  [outline appendBezierPathWithArcWithCenter: NSMakePoint(NSMinX(bounds) + mCornerRadius + leftOffset,
                                                          NSMaxY(bounds) - mCornerRadius - topOffset)
                                      radius: mCornerRadius
                                  startAngle: 90
                                    endAngle: 180];
  if (leftOffset > 0) {
    [outline lineToPoint: NSMakePoint(NSMinX(bounds) + leftOffset, localHotSpot.y + mArrowBase / 2)];
    [outline lineToPoint: localHotSpot];
    [outline lineToPoint: NSMakePoint(NSMinX(bounds) + leftOffset, localHotSpot.y - mArrowBase / 2)];
  }

  frameView.outline = outline;
  [frameView setNeedsDisplay: YES];
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Resizes the window to accomodate the content view with the computed padding on each side
 * and then places the content view to the right position.
 */
- (void)adjustWindowSizeAndContentFrame {
  NSPoint origin = self.frame.origin;
  NSRect newWindowFrame =
    NSMakeRect(origin.x, origin.y, mBaseSize.width + mPadding.horizontal(), mBaseSize.height + mPadding.vertical());

  [self setFrame: newWindowFrame display: NO animate: self.visible];
  mChildContentView.frame = NSMakeRect(mPadding.left, mPadding.bottom, mBaseSize.width, mBaseSize.height);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)show:(NSPoint)location relativePosition: (mforms::StartPosition)position {
  if (location.x < 0 && location.y < 0)
    location = [NSEvent mouseLocation];

  mHotSpot = location;
  [self computeCoordinatesAndPadding: position];
  if (!(self.occlusionState & NSWindowOcclusionStateVisible) || mRelativePosition != position) {
    mRelativePosition = position;
    [self adjustWindowSizeAndContentFrame];
    [self computeOutline];
  }

  if (!(self.occlusionState & NSWindowOcclusionStateVisible)) {
    [NSAnimationContext beginGrouping];
    self.alphaValue = 0;
    [self orderFront:nil];
    [NSAnimationContext currentContext].duration = 0.25;
    [self animator].alphaValue = 1;
    [NSAnimationContext endGrouping];
  }
}

//----------------------------------------------------------------------------------------------------------------------

// Tracking area related

- (void)mouseExited:(NSEvent*)theEvent {
  if (theEvent.userData) {
    (*mOwner->signal_close())();
  } else
    [super mouseExited: theEvent];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)close {
  [mTrackedView removeTrackingArea: mOwnerTracking];
  mOwnerTracking = nil;
  mTrackedView = nil;
  [NSAnimationContext currentContext].duration = 0.25;
  [self animator].alphaValue = 0;
  [self performSelector: @selector(orderOut:)
             withObject: nil
             afterDelay: 0.5
                inModes: @[ NSModalPanelRunLoopMode, NSDefaultRunLoopMode ]];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityPopoverRole;
}

//----------------------------------------------------------------------------------------------------------------------

using namespace mforms;

static bool popover_create(Popover* popover, mforms::View *owner, mforms::PopoverStyle style) {
  MFPopover* popoverWindow = [[MFPopover alloc] initWithContentRect: NSMakeRect(0, 0, 100, 100)
                                                          styleMask: NSWindowStyleMaskBorderless
                                                            backing: NSBackingStoreBuffered
                                                              defer: NO
                                                              style: style];
  popoverWindow->mOwner = popover;
  [popoverWindow setHasShadow: YES];
  [popoverWindow setLevel: NSPopUpMenuWindowLevel];
  popover->set_data(popoverWindow);

  return true;
}

//--------------------------------------------------------------------------------------------------

static void popover_set_content(Popover* popover, View* content) {
  [popover->get_data() setContentView:content->get_data()];
}

//--------------------------------------------------------------------------------------------------

static void popover_set_size(Popover* popover, int width, int height) {
  [popover->get_data() setContentSize: NSMakeSize(width, height)];
}

//--------------------------------------------------------------------------------------------------

static void popover_show(Popover* popover, int x, int y, StartPosition relativePosition) {
  [popover->get_data() show: NSMakePoint(x, y) relativePosition:relativePosition];
}

//--------------------------------------------------------------------------------------------------

static void popover_show_and_track(Popover* popover, View* owner, int x, int y, StartPosition relativePosition) {
  MFPopover* impl = popover->get_data();
  [impl show:NSMakePoint(x, y) relativePosition:relativePosition];
  NSTrackingArea* tarea = [[NSTrackingArea alloc]
    initWithRect: [owner->get_data() bounds]
         options: NSTrackingMouseEnteredAndExited | NSTrackingAssumeInside | NSTrackingActiveAlways
           owner: popover->get_data()
        userInfo: @{}];
  [owner->get_data() addTrackingArea:tarea];
  impl->mTrackedView = owner->get_data();
  impl->mOwnerTracking = tarea;
}

//--------------------------------------------------------------------------------------------------

static void popover_close(Popover* popover) {
  [popover->get_data() close];
}

//--------------------------------------------------------------------------------------------------

static void popover_setName(Popover *popover, const std::string &name) {
  MFPopover *native = popover->get_data();
  native.accessibilityTitle = [NSString stringWithUTF8String: name.c_str()];
}

//--------------------------------------------------------------------------------------------------

static void popover_destroy(Popover* popover) {
  popover->set_data(nil); // this will release the existing reference
}

//--------------------------------------------------------------------------------------------------

void cf_popover_init() {
  ::mforms::ControlFactory* f = ::mforms::ControlFactory::get_instance();

  f->_popover_impl.create = &popover_create;
  f->_popover_impl.destroy = &popover_destroy;
  f->_popover_impl.set_content = &popover_set_content;
  f->_popover_impl.set_size = &popover_set_size;
  f->_popover_impl.show = &popover_show;
  f->_popover_impl.show_and_track = &popover_show_and_track;
  f->_popover_impl.setName = &popover_setName;
  f->_popover_impl.close = &popover_close;
}

@end
