/*
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
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

#import "MCollectionViewItemView.h"


@implementation MCollectionViewItemView

/**
 * Make this view accepting events as first responder to allow it to respond to key events.
 */
- (BOOL) acceptsFirstResponder
{
  return YES;
}

//--------------------------------------------------------------------------------------------------

- (BOOL) becomeFirstResponder
{
  // When starting editing by mouse click we must not do it if we just became first responder.
  mBecameFirstResponder = YES;
  
  return [super becomeFirstResponder];
}

//--------------------------------------------------------------------------------------------------

- (BOOL) resignFirstResponder
{
  // When losing the focus the item is no longer the active one and must be painted accordingly.
  // Update the entire collection view as we could lose focus to another collection view.
  [[self superview] setNeedsDisplay: YES];
  return [super resignFirstResponder];
}

//--------------------------------------------------------------------------------------------------

- (void) dealloc
{
  [NSObject cancelPreviousPerformRequestsWithTarget: self];
  [[NSNotificationCenter defaultCenter] removeObserver: self];
  [super dealloc];
}

//--------------------------------------------------------------------------------------------------

- (id)copy
{
  MCollectionViewItemView *copy = (MCollectionViewItemView *)[super copy];
  if (copy)
    copy->mOwner = mOwner;
  return copy;
}


- (void) setOwner: (NSCollectionViewItem*) owner
{
  mOwner= owner;
  mIsEditing = NO;
}

//--------------------------------------------------------------------------------------------------

- (NSCollectionViewItem*) owner
{
  return mOwner;
}

//--------------------------------------------------------------------------------------------------

- (NSView*) hitTest: (NSPoint) aPoint 
{
  if (NSPointInRect(aPoint, [self convertRect: [self bounds] toView: [self superview]])) 
    return self;
  return nil;
}

//--------------------------------------------------------------------------------------------------

/**
 * Removes any requests for starting delayed inline editing if there are any.
 */
- (void) cancelPendingInlineEdit
{
  [NSObject cancelPreviousPerformRequestsWithTarget: self selector: @selector(beginInlineEditing) object: nil];
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the currently active collection view or nil if no one is active (is first responder).
 */
- (NSCollectionView*) activeCollectionView
{
  NSResponder* currentFirstResponder = [[self window] firstResponder];
  if ([currentFirstResponder isKindOfClass: [NSCollectionView class]])
    return (NSCollectionView*) currentFirstResponder;
  
  if ([currentFirstResponder isKindOfClass: [NSView class]])
  {
    NSView* parent = [(NSView*) currentFirstResponder superview];
    if ([parent isKindOfClass: [NSCollectionView class]])
      return (NSCollectionView*) parent;
  }
  return nil;
}

//--------------------------------------------------------------------------------------------------

- (void) mouseDown: (NSEvent*) theEvent 
{
  mMouseDownLocation= [self convertPoint: [theEvent locationInWindow] fromView: nil];
  
  [self cancelPendingInlineEdit];
  
  // Needs to be commented to allow dragging the items.
  // [super mouseDown: theEvent];
  [[self window] makeFirstResponder: self];
  
  // Handle selection depending on modifier keys.
  NSUInteger modifiers = [theEvent modifierFlags];
  BOOL control = (modifiers & NSControlKeyMask) != 0;
  BOOL shift = (modifiers & NSShiftKeyMask) != 0;
  BOOL command = (modifiers & NSCommandKeyMask) != 0;
  
  if (!control && !shift && !command)
  {
    if ([[self delegate] respondsToSelector: @selector(clearSelection)]) 
      [[self delegate] clearSelection];
    [self setSelected: YES];
    [[self superview] setNeedsDisplay: YES];
  }
  else
  {
    // Add this item to the selection if shift is pressed (other modifiers are ignored in this case)
    // or toggle the selection state if control/command are held down (but no shift).
    if (shift)
      [self setSelected: YES];
    else
      [self setSelected: ![self selected]];
  }
  
  // Activate it on double-click.
  if ([theEvent clickCount] == 2) 
    if ([[self delegate] respondsToSelector: @selector(activateCollectionItem:)]) 
      [[self delegate] activateCollectionItem: self];
}

//--------------------------------------------------------------------------------------------------

- (void)rightMouseDown: (NSEvent*) event
{
  [self mouseDown: event]; // same handling as left mouse click.

  // This will popup the context menu.
  [super rightMouseDown: event];
}

//--------------------------------------------------------------------------------------------------

- (void)mouseDragged: (NSEvent*) event
{
  NSPoint location = [self convertPoint: [event locationInWindow] fromView: nil];
  
  if (fabs(mMouseDownLocation.x - location.x) >= 5 ||
      fabs(mMouseDownLocation.y - location.y) >= 5)
  {
    NSArray *types = nil;
    if ([delegate respondsToSelector: @selector(dropTypesForItem:)])
    {
      types = [delegate dropTypesForItem:self];

      if (types == nil)
        return;
    }

    NSImage *image = nil;
    for (id view in [self subviews])
    {
      if ([view isKindOfClass: [NSImageView class]])
      {
        image = [view image];
        break;
      }
    }
    
    NSPasteboardItem *pbItem = [NSPasteboardItem new];
    [pbItem setDataProvider: self forTypes: types];

    NSDraggingItem *dragItem = [[NSDraggingItem alloc] initWithPasteboardWriter: pbItem];

    [dragItem setDraggingFrame: NSMakeRect(0, 0, image.size.width, image.size.height)
                      contents: image];
    NSDraggingSession *draggingSession = [self beginDraggingSessionWithItems: @[dragItem]
                                                                       event: event
                                                                      source: self];
    draggingSession.animatesToStartingPositionsOnCancelOrFail = YES;
    draggingSession.draggingFormation = NSDraggingFormationNone;
  }
}

//--------------------------------------------------------------------------------------------------

- (void)pasteboard: (NSPasteboard *)sender item: (NSPasteboardItem *)item provideDataForType: (NSString *)type
{
  if ([delegate respondsToSelector: @selector(declareDragDataForItem:pasteboard:)])
      [delegate declareDragDataForItem: self pasteboard: sender];
}

//--------------------------------------------------------------------------------------------------

- (NSDragOperation)       draggingSession: (NSDraggingSession *)session
    sourceOperationMaskForDraggingContext: (NSDraggingContext)context;
{
  switch (context) {
    case NSDraggingContextOutsideApplication:
      return NSDragOperationNone;
      break;

    case NSDraggingContextWithinApplication:
    default:
      return NSDragOperationCopy;
      break;
  }
}

//--------------------------------------------------------------------------------------------------

- (BOOL)ignoreModifierKeysForDraggingSession: (NSDraggingSession *)session {
  return YES;
}

- (void) mouseUp: (NSEvent *) theEvent
{
  if (mBecameFirstResponder)
    mBecameFirstResponder = NO;
  else
  {
    NSUInteger modifiers = [theEvent modifierFlags];
    BOOL control = (modifiers & NSControlKeyMask) != 0;
    BOOL shift = (modifiers & NSShiftKeyMask) != 0;
    BOOL command = (modifiers & NSCommandKeyMask) != 0;
    
    if (!control && !shift && !command && [theEvent clickCount] == 1)
      [self performSelector: @selector(beginInlineEditing) withObject: nil afterDelay: 0.5
                    inModes: @[NSModalPanelRunLoopMode, NSDefaultRunLoopMode]];
  }
}

//--------------------------------------------------------------------------------------------------

- (void) keyDown: (NSEvent *) theEvent
{
  [self cancelPendingInlineEdit];
  
  switch ([theEvent keyCode])
  {
    case 36: // normal <enter> key
      if ([[self delegate] respondsToSelector: @selector(activateCollectionItem:)]) 
        [[self delegate] activateCollectionItem: self];
      break;
    case 76: // keypad <enter> key
      [self beginInlineEditing];
      break;
    default:
      [super keyDown: theEvent];
  }
}

//--------------------------------------------------------------------------------------------------

- (void) setSelected: (BOOL) flag
{
  BOOL wasSelected = [self selected];
  
  if (wasSelected != flag)
  {
    if (flag)
    {
      if ([[self delegate] respondsToSelector: @selector(selectCollectionItem:)]) 
        [[self delegate] selectCollectionItem: self];
      [[self window] makeFirstResponder: self];
    }
    else
    {
      if ([[self delegate] respondsToSelector: @selector(unselectCollectionItem:)]) 
        [[self delegate] unselectCollectionItem: self];
    }
  };
  
  // Always refresh display, because the highlight might change even though the selection state
  // did not (e.g. when switching between different collection views).
  [self setNeedsDisplay:YES];
}

//--------------------------------------------------------------------------------------------------

- (BOOL) selected
{
  if ([[self delegate] respondsToSelector: @selector(isCollectionItemSelected:)]) 
    return [[self delegate] isCollectionItemSelected: self];
  return NO;
}

//--------------------------------------------------------------------------------------------------

- (id) delegate
{
  return delegate;
}

//--------------------------------------------------------------------------------------------------

- (void) setDelegate: (id) aDelegate
{
  delegate = aDelegate;
  /* Outlines currently don't accept drag operations.
  if ([delegate respondsToSelector: @selector(dropTypesForItem:)])
  {
    NSArray *types= [delegate dropTypesForItem:self];
    
    if (types)
      [self registerForDraggedTypes: types];
  }
   */
}

//--------------------------------------------------------------------------------------------------

- (void) textDidEndEditing: (NSNotification*) aNotification
{
  [self stopInlineEditing: YES];
}

//--------------------------------------------------------------------------------------------------

// sample code from quartz docs
static void addRoundedRectToPath(CGContextRef context, CGRect rect, float ovalWidth, float ovalHeight)

{
  float fw, fh;
  
  if (ovalWidth == 0 || ovalHeight == 0) {// 1
    CGContextAddRect(context, rect);
    return;
  }
  
  CGContextSaveGState(context);// 2
  
  CGContextTranslateCTM (context, CGRectGetMinX(rect),// 3
                         CGRectGetMinY(rect));
  CGContextScaleCTM (context, ovalWidth, ovalHeight);// 4
  fw = CGRectGetWidth (rect) / ovalWidth;// 5
  fh = CGRectGetHeight (rect) / ovalHeight;// 6
  
  CGContextMoveToPoint(context, fw, fh/2); // 7
  CGContextAddArcToPoint(context, fw, fh, fw/2, fh, 1);// 8
  CGContextAddArcToPoint(context, 0, fh, 0, fh/2, 1);// 9
  CGContextAddArcToPoint(context, 0, 0, fw/2, 0, 1);// 10
  CGContextAddArcToPoint(context, fw, 0, fw, fh/2, 1); // 11
  CGContextClosePath(context);// 12
  
  CGContextRestoreGState(context);// 13
}

//--------------------------------------------------------------------------------------------------

- (void) drawRect: (NSRect) rect 
{
  NSTextField* label = [self viewWithTag: 1];
  if ([self selected])
  {
    CGContextRef context= [[NSGraphicsContext currentContext] graphicsPort];
    
    BOOL applicationActive = [NSApp keyWindow] != nil;
    BOOL showSelected = [self activeCollectionView] == [self superview];
    
    if (showSelected)
    {
      if (applicationActive)
        [[NSColor alternateSelectedControlColor] setFill];
      else
        [[NSColor secondarySelectedControlColor] setFill];
    }
    
    if (applicationActive && showSelected)
      [label setTextColor:[NSColor alternateSelectedControlTextColor]];
    else
      [label setTextColor:[NSColor textColor]];
    
    // Draw focus mark only if our collection view is active. This simulates a single
    // collection view over all views on the overview page.
    if (showSelected)
    {
      addRoundedRectToPath(context, NSRectToCGRect(NSInsetRect([self bounds], 1.0, 1.0)), 5.0, 5.0);
      CGContextFillPath(context);
    }
  }
  else
    [label setTextColor:[NSColor textColor]];
  
  [super drawRect:rect];
}

//--------------------------------------------------------------------------------------------------

/**
 * Starts inline editing by using the window's field editor.
 */
- (void) beginInlineEditing
{
  if (!mIsEditing)
  {
    BOOL allowed = NO;
    if ([[self delegate] respondsToSelector: @selector(canRename:)]) 
      allowed= [[self delegate] canRename: self];
    if (!allowed)
      return;
    
    mIsEditing = YES;
    
    // Get the window's field editor and set that up for inline editing.
    NSTextField* label = [self viewWithTag: 1];
    NSText* fieldEditor = [[self window] fieldEditor: YES forObject: label];
    [fieldEditor setDelegate: self];
    [self addSubview: fieldEditor];
    
    // The inline editor cannot be made bordered so we use the label instead.
    // Make the frame of the label a pixel larger in each direction for that.
    NSRect frame = [label frame];
    [fieldEditor setFrame: frame];
    frame = NSInsetRect(frame, -1, -2);
    [label setFrame: frame];
    [label setBordered: NSLineBorder];
    
    [fieldEditor setString: [label stringValue]];
    [fieldEditor selectAll: nil];
    [fieldEditor setFocusRingType: NSFocusRingTypeNone];
    [[self window] makeFirstResponder: fieldEditor];
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Ends inline editing previously started via beginInlineEditing.
 */
- (void) stopInlineEditing: (BOOL) accepted
{
  if (mIsEditing)
  {
    mIsEditing = NO;
    
    NSTextField* label = [self viewWithTag: 1];
    NSText* fieldEditor = [[self window] fieldEditor: NO forObject: label];
    [label setStringValue: [fieldEditor string]];
    
    // Revert the visual cues we changed when we started editing.
    [label setBordered: NSNoBorder];
    NSRect frame = NSInsetRect([label frame], 1, 2);
    [label setFrame: frame];
    
    [[self window] endEditingFor: label];
    [fieldEditor removeFromSuperview];
    [[self window] makeFirstResponder: self];
    
    if (accepted && [[self delegate] respondsToSelector: @selector(itemRenameDidEnd:withName:)]) 
      [[self delegate] performSelector: @selector(itemRenameDidEnd:withName:) withObject: self withObject: [label stringValue]];
  }
}

@end

//--------------------------------------------------------------------------------------------------

