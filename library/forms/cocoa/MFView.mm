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

#import "MFView.h"
#import "MFMForms.h"
#include "base/string_utilities.h"

#import "MFContainerBase.h" // to get forward declaration of setFreezeRelayout:
#import "ScintillaView.h"   // For drop delegate retrieval.
#import "NSColor_extras.h"
#import <Carbon/Carbon.h>

//----------------------------------------------------------------------------------------------------------------------

@implementation NSView (MForms)

- (id)innerView {
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

static const char *minimumSizeKey = "minimumSizeKey";

- (NSSize)minimumSize {
  NSValue *value = objc_getAssociatedObject(self, minimumSizeKey);
  return value.sizeValue;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setMinimumSize:(NSSize)size {
  objc_setAssociatedObject(self, minimumSizeKey, [NSValue valueWithSize: size], OBJC_ASSOCIATION_RETAIN);
}

//----------------------------------------------------------------------------------------------------------------------

static const char *viewFlagsKey = "viewFlagsKey";

- (ViewFlags)viewFlags {
  NSNumber *value = objc_getAssociatedObject(self, viewFlagsKey);
  return (ViewFlags)value.intValue;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setViewFlags:(ViewFlags)value {
  objc_setAssociatedObject(self, viewFlagsKey, @(value), OBJC_ASSOCIATION_RETAIN);
}

//----------------------------------------------------------------------------------------------------------------------

static const char *lastDragOperationKey = "lastDragOperationKey";

- (mforms::DragOperation)lastDragOperation {
  NSNumber *value = objc_getAssociatedObject(self, lastDragOperationKey);
  return (mforms::DragOperation)value.intValue;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setLastDragOperation: (mforms::DragOperation)value {
  objc_setAssociatedObject(self, lastDragOperationKey, @(value), OBJC_ASSOCIATION_RETAIN);
}

//----------------------------------------------------------------------------------------------------------------------

static const char *allowedDragOperationsKey = "allowedDragOperationsKey";

- (mforms::DragOperation)allowedDragOperations {
  NSNumber *value = objc_getAssociatedObject(self, allowedDragOperationsKey);
  return (mforms::DragOperation)value.intValue;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setAllowedDragOperations: (mforms::DragOperation)value {
  objc_setAssociatedObject(self, allowedDragOperationsKey, @(value), OBJC_ASSOCIATION_RETAIN);
}

//----------------------------------------------------------------------------------------------------------------------

static const char *acceptableDropFormatsKey = "acceptableDropFormats";

- (NSArray *)acceptableDropFormats {
  return objc_getAssociatedObject(self, acceptableDropFormatsKey);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setAcceptableDropFormats: (NSArray *)formats {
  objc_setAssociatedObject(self, acceptableDropFormatsKey, formats, OBJC_ASSOCIATION_RETAIN);
  if (formats.count > 0)
    [self registerForDraggedTypes: formats];
  else
    [self unregisterDraggedTypes];
}

//----------------------------------------------------------------------------------------------------------------------

static const char *dropDelegateKey = "dropDelegate";

- (mforms::DropDelegate *)dropDelegate {
  NSNumber *value = objc_getAssociatedObject(self, dropDelegateKey);
  return (mforms::DropDelegate *)value.unsignedIntegerValue;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setDropDelegate: (mforms::DropDelegate *)delegate {
  objc_setAssociatedObject(self, dropDelegateKey, @((NSUInteger)delegate), OBJC_ASSOCIATION_RETAIN);
}

//----------------------------------------------------------------------------------------------------------------------

static const char *lastDropPositionKey = "lastDropPositionKey";

- (mforms::DropPosition)lastDropPosition {
  NSNumber *value = objc_getAssociatedObject(self, lastDropPositionKey);
  return (mforms::DropPosition)value.intValue;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setLastDropPosition:(mforms::DropPosition)value {
  objc_setAssociatedObject(self, lastDropPositionKey, @(value), OBJC_ASSOCIATION_RETAIN);
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isAccessibilityElement {
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (bool)handleMouseUp: (NSEvent *)event owner: (mforms::View *)mOwner {
  NSPoint p = [self convertPoint: event.locationInWindow fromView: nil];

  mforms::MouseButton mouseButton;
  switch (event.buttonNumber) // NSLeftMouseDown etc are NOT buttonNumber constants
  {
    case 1:
      mouseButton = mforms::MouseButtonRight;
      break;

    case 0:
      mouseButton = mforms::MouseButtonLeft;
      break;

    default:
      mouseButton = mforms::MouseButtonOther;
      break;
  }

  bool handled = false;
  switch (event.clickCount) {
    case 1:
      handled = mOwner->mouse_click(mouseButton, p.x, p.y);
      break;
    case 2:
      handled = mOwner->mouse_double_click(mouseButton, p.x, p.y);
      break;
  }
  // mouse up is always sent
  handled |= mOwner->mouse_up(mouseButton, p.x, p.y);

  return handled;
}

//----------------------------------------------------------------------------------------------------------------------

- (bool)handleMouseDown: (NSEvent *)event owner: (mforms::View *)mOwner {
  NSPoint p = [self convertPoint: event.locationInWindow fromView: nil];
  mforms::MouseButton mouseButton;
  switch (event.buttonNumber) // NSLeftMouseDown etc are NOT buttonNumber constants
  {
    case 1:
      mouseButton = mforms::MouseButtonRight;
      break;

    case 0:
      mouseButton = mforms::MouseButtonLeft;
      break;

    default:
      mouseButton = mforms::MouseButtonOther;
      break;
  }

  return mOwner->mouse_down(mouseButton, p.x, p.y);
}

//----------------------------------------------------------------------------------------------------------------------

- (bool)handleMouseMove: (NSEvent *)event owner: (mforms::View *)mOwner {
  // We have to map mouseDragged to mouseMoved as other platforms don't do this separation.
  // However the mouse button recorded in the event for mouseMoved is that of the last pressed
  // (and released) button. A currently pressed button produces mouseDragged instead.
  NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];

  mforms::MouseButton mouseButton;
  if (event.type == NSEventTypeLeftMouseDragged)
    mouseButton = mforms::MouseButtonLeft;
  else
    mouseButton = mforms::MouseButtonNone;

  return mOwner->mouse_move(mouseButton, p.x, p.y);
}

//----------------------------------------------------------------------------------------------------------------------

- (bool)handleMouseEntered: (NSEvent *)event owner: (mforms::View *)mOwner {
  // NSPoint p = [self convertPoint: [event locationInWindow] fromView: nil];
  return mOwner->mouse_enter();
}

//----------------------------------------------------------------------------------------------------------------------

- (bool)handleMouseExited: (NSEvent *)event owner: (mforms::View *)mOwner {
  // NSPoint p = [self convertPoint: [event locationInWindow] fromView: nil];
  return mOwner->mouse_leave();
}

//----------------------------------------------------------------------------------------------------------------------

- (bool)handleBecomeFirstResponder: (mforms::View *)mOwner {
  return mOwner->focusIn();
}

//----------------------------------------------------------------------------------------------------------------------

- (bool)handleResignFirstResponder: (mforms::View *)mOwner {
  return mOwner->focusOut();
}

//----------------------------------------------------------------------------------------------------------------------

- (bool)handleKeyUp:(NSEvent *)event owner:(mforms::View *)mOwner {
  return mOwner->keyRelease([self keyFromEvent: event], [self modifiersFromEvent: event]);
}

//----------------------------------------------------------------------------------------------------------------------

- (bool)handleKeyDown:(NSEvent *)event owner:(mforms::View *)mOwner {
  return mOwner->keyPress([self keyFromEvent: event], [self modifiersFromEvent: event]);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * To be called by all controls that want mouse events for dragging (from updateTrackingAreas).
 */
- (NSTrackingArea *)updateTrackingArea: (NSTrackingArea *)currentArea {
  // Create one tracking area which covers the whole control and make it get mouse events.
  if (currentArea != nil) {
    [self removeTrackingArea: currentArea];
  }

  NSTrackingAreaOptions options =
    NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveInActiveApp | NSTrackingInVisibleRect;
  currentArea = [[NSTrackingArea alloc] initWithRect: self.bounds options:options owner: self userInfo: nil];
  [self addTrackingArea: currentArea];

  return currentArea;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the view's preferred size. Since a raw view doesn't know anything about its content
 * the prefered size is its minimum size or the proposal (whichever is larger). Descendants (like container classes)
 * override this and compute their real preferred size.
 */
- (NSSize)preferredSize: (NSSize)proposal {
  return { MAX(self.minimumSize.width, proposal.width), MAX(self.minimumSize.height, proposal.height) };
}

//----------------------------------------------------------------------------------------------------------------------

- (void)relayout {
  [self resizeSubviewsWithOldSize: self.frame.size];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawBounds: (NSRect)rect {
  NSFrameRect(rect);
  [NSBezierPath strokeLineFromPoint: NSMakePoint(NSMinX(rect), NSMinY(rect))
                            toPoint :NSMakePoint(NSMaxX(rect), NSMaxY(rect))];
  [NSBezierPath strokeLineFromPoint: NSMakePoint(NSMinX(rect), NSMaxY(rect))
                            toPoint: NSMakePoint(NSMaxX(rect), NSMinY(rect))];
}

//----------------------------------------------------------------------------------------------------------------------

#pragma mark - Drag/drop support

// Helper struct we use to mark custom WB data on the pasteboard.
struct PasteboardDataWrapper {
  const char identifier[16]; // always "mysql-workbench"
  void *data;
  PasteboardDataWrapper() : identifier("mysql-workbench") {
    data = NULL;
  }
};

//----------------------------------------------------------------------------------------------------------------------

- (mforms::DropDelegate *)determineDropDelegate {
  mforms::DropDelegate *delegate = self.dropDelegate;
  if (delegate == NULL) {
    if ([self respondsToSelector: @selector(mformsObject)])
      delegate = dynamic_cast<mforms::DropDelegate *>(self.mformsObject);
  }
  return delegate;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSDragOperation)draggingUpdated: (id<NSDraggingInfo>)sender {
  mforms::DropDelegate *delegate = [self determineDropDelegate];
  if (delegate == NULL)
    return NSDragOperationNone;

  // See if we can extract an mforms View from the dragging info which would indicate
  // a drag operation started by mforms.
  id source = sender.draggingSource;

  NSDragOperation nativeOperations = sender.draggingSourceOperationMask;
  mforms::DragOperation operations = mforms::DragOperationNone;
  if ((nativeOperations & NSDragOperationMove) == NSDragOperationMove)
    operations |= mforms::DragOperationMove;
  if ((nativeOperations & NSDragOperationCopy) == NSDragOperationCopy)
    operations |= mforms::DragOperationCopy;

  mforms::View *view = NULL;
  if ([source respondsToSelector: @selector(mformsObject)])
    view = dynamic_cast<mforms::View *>([source mformsObject]);

  NSPoint location = [self convertPoint: sender.draggingLocation fromView: nil];
  std::vector<std::string> formats;
  NSPasteboard *pasteboard = sender.draggingPasteboard;
  for (NSString *entry in pasteboard.types) {
    if ([entry isEqualToString:NSPasteboardTypeString])
      formats.push_back(mforms::DragFormatText);
    else if ([entry isEqualToString: NSPasteboardTypeFileURL])
      formats.push_back(mforms::DragFormatFileName);
    else
      formats.push_back(entry.UTF8String);
  }

  NSDragOperation result = NSDragOperationNone;
  mforms::DragOperation operation = delegate->drag_over(view, base::Point(location.x, location.y), operations, formats);
  self.lastDragOperation = operation;
  if ((operation & mforms::DragOperationCopy) != 0)
    result |= NSDragOperationCopy;
  if ((operation & mforms::DragOperationMove) != 0)
    result |= NSDragOperationMove;

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)performDragOperation: (id<NSDraggingInfo>)sender {
  mforms::DropDelegate *delegate = [self determineDropDelegate];
  if (delegate == NULL)
    return NO;

  // See if we can extract an mforms View from the dragging info which would indicate
  // a drag operation started by mforms.
  id source = sender.draggingSource;

  NSDragOperation nativeOperations = sender.draggingSourceOperationMask;
  mforms::DragOperation operations = mforms::DragOperationNone;
  if ((nativeOperations & NSDragOperationMove) == NSDragOperationMove)
    operations |= mforms::DragOperationMove;
  if ((nativeOperations & NSDragOperationCopy) == NSDragOperationCopy)
    operations |= mforms::DragOperationCopy;

  mforms::View *view = NULL;
  if ([source respondsToSelector:@selector(mformsObject)])
    view = dynamic_cast<mforms::View *>([source mformsObject]);

  NSPoint location = [self convertPoint: sender.draggingLocation fromView: nil];
  NSPasteboard *pasteboard = sender.draggingPasteboard;
  for (NSString *entry in pasteboard.types) {
    if ([entry isEqualToString: NSPasteboardTypeString]) {
      NSString *text = [pasteboard stringForType: NSPasteboardTypeString];
      if (delegate->text_dropped(view, base::Point(location.x, location.y), operations, text.UTF8String) !=
          mforms::DragOperationNone)
        return YES;
    } else if ([entry isEqualToString: NSPasteboardTypeFileURL]) {
      NSArray *fileURLs = [pasteboard propertyListForType: NSPasteboardTypeFileURL];
      std::vector<std::string> names;
      for (NSURL *url in fileURLs)
        names.push_back(url.path.UTF8String);
      if (names.size() > 0 &&
          delegate->files_dropped(view, base::Point(location.x, location.y), operations, names) !=
            mforms::DragOperationNone)
        return YES;
    } else {
      // Any custom data.
      void *data = [pasteboard nativeDataForTypeAsString:entry];
      if (data != NULL) {
        if (delegate->data_dropped(view, base::Point(location.x, location.y), operations, data, entry.UTF8String) !=
            mforms::DragOperationNone)
          return YES;
      }
    }
  }

  return NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)draggingEnded:(id<NSDraggingInfo>)sender {
  self.lastDropPosition = mforms::DropPositionUnknown;
}

//----------------------------------------------------------------------------------------------------------------------

// Since drag initiation and data retrieval are now separated we need a temporary storage for the data.
// As this is a category we would have to go the long way, or simply use a static var.
// No concurrency here as dd is per se safe (only one drag operation at a time possible).
static NSString *dragText = nil;

- (mforms::DragOperation)startDragWithText: (NSString *)text details: (mforms::DragDetails)details {
  self.allowedDragOperations = details.allowedOperations;
  self.lastDropPosition = mforms::DropPositionUnknown;
  dragText = text;

  NSPasteboard *pasteboard = NSPasteboard.generalPasteboard;
  [pasteboard clearContents];
  [pasteboard setString: text forType: NSPasteboardTypeString];

  NSImage *dragImage = [[NSImage alloc] init];
  if (details.image == NULL) {
    NSDictionary *attributes = @{NSFontAttributeName : [NSFont labelFontOfSize: 12]};
    dragImage.size = [text sizeWithAttributes: attributes];
    [dragImage lockFocus];
    [text drawAtPoint: NSMakePoint(0, 0) withAttributes: attributes];
    [dragImage unlockFocus];
  } else {
    unsigned char *data = cairo_image_surface_get_data(details.image);

    // Convert pixel fromat from ARGB to ABGR.
    int i = 0;
    int width = cairo_image_surface_get_width(details.image);
    int height = cairo_image_surface_get_height(details.image);
    while (i < 4 * width * height) {
      unsigned char temp = data[i];
      data[i] = data[i + 2];
      data[i + 2] = temp;
      i += 4;
    }
    NSBitmapImageRep *bitmap =
      [[NSBitmapImageRep alloc] initWithBitmapDataPlanes: (unsigned char **)&data
                                              pixelsWide: width
                                              pixelsHigh: height
                                           bitsPerSample: 8
                                         samplesPerPixel: 4
                                                hasAlpha: YES
                                                isPlanar: NO
                                          colorSpaceName: NSCalibratedRGBColorSpace
                                             bytesPerRow: cairo_image_surface_get_stride(details.image)
                                            bitsPerPixel: 0];
    [dragImage addRepresentation:bitmap];
  }

  NSPoint position = NSMakePoint(details.location.x, details.location.y);

  // We need a mouse event so the dragImage: call can get the original mouse position.
  // Usually we are called by a mouse down/mouse move event, but we lost the original event while
  // the handling went through the backend. So we create one manually here again.
  NSEvent *event = [NSEvent mouseEventWithType: NSEventTypeLeftMouseDown
                                      location: [self convertPoint:position toView:nil]
                                 modifierFlags: 0
                                     timestamp: 0
                                  windowNumber: self.window.windowNumber
                                       context: nil
                                   eventNumber: 0
                                    clickCount: 1
                                      pressure: 1];
  position.x -= details.hotspot.x;
  position.y -= details.hotspot.y;

  NSPasteboardItem *pbItem = [NSPasteboardItem new];
  [pbItem setDataProvider:self forTypes:@[ NSPasteboardTypeString ]];

  NSDraggingItem *dragItem = [[NSDraggingItem alloc] initWithPasteboardWriter: pbItem];

  [dragItem setDraggingFrame:NSMakeRect(position.x, position.y, dragImage.size.width, dragImage.size.height)
                    contents:dragImage];
  NSDraggingSession *draggingSession = [self beginDraggingSessionWithItems: @[dragItem] event: event source: self];
  draggingSession.animatesToStartingPositionsOnCancelOrFail = YES;
  draggingSession.draggingFormation = NSDraggingFormationNone;

  return self.lastDragOperation;
}

//----------------------------------------------------------------------------------------------------------------------

static void *dragData = NULL;
static bool dragInProgress = NO;

- (mforms::DragOperation)startDragWithData: (void *)data details: (mforms::DragDetails)details format: (NSString *)format {
  self.allowedDragOperations = details.allowedOperations;
  self.lastDropPosition = mforms::DropPositionUnknown;
  dragData = data;

  NSImage *dragImage = [[NSImage alloc] init];
  if (details.image != NULL) {
    unsigned char *data = cairo_image_surface_get_data(details.image);

    // Convert pixel fromat from ARGB to ABGR.
    int i = 0;
    int width = cairo_image_surface_get_width(details.image);
    int height = cairo_image_surface_get_height(details.image);
    while (i < 4 * width * height) {
      unsigned char temp = data[i];
      data[i] = data[i + 2];
      data[i + 2] = temp;
      i += 4;
    }
    NSBitmapImageRep *bitmap =
      [[NSBitmapImageRep alloc] initWithBitmapDataPlanes: (unsigned char **)&data
                                              pixelsWide: width
                                              pixelsHigh: height
                                           bitsPerSample: 8
                                         samplesPerPixel: 4
                                                hasAlpha: YES
                                                isPlanar: NO
                                          colorSpaceName: NSCalibratedRGBColorSpace
                                             bytesPerRow: cairo_image_surface_get_stride(details.image)
                                            bitsPerPixel: 0];
    [dragImage addRepresentation:bitmap];
  }

  NSPoint position = NSMakePoint(details.location.x, details.location.y);
  NSEvent *event = [NSEvent mouseEventWithType: NSEventTypeLeftMouseDown
                                      location: [self convertPoint:position toView:nil]
                                 modifierFlags: 0
                                     timestamp: 0
                                  windowNumber: self.window.windowNumber
                                       context: nil
                                   eventNumber: 0
                                    clickCount: 1
                                      pressure: 1];

  // The drag image position must always be the lower left corner (regardless of the flippedness of the view).
  position.x -= details.hotspot.x;
  position.y -= details.hotspot.y;

  NSPasteboardItem *pbItem = [NSPasteboardItem new];
  [pbItem setDataProvider:self forTypes:@[ format ]];

  NSDraggingItem *dragItem = [[NSDraggingItem alloc] initWithPasteboardWriter:pbItem];

  [dragItem setDraggingFrame:NSMakeRect(position.x, position.y, dragImage.size.width, dragImage.size.height)
                    contents:dragImage];
  NSDraggingSession *draggingSession = [self beginDraggingSessionWithItems: @[dragItem] event: event source: self];
  draggingSession.animatesToStartingPositionsOnCancelOrFail = YES;
  draggingSession.draggingFormation = NSDraggingFormationNone;

  dragInProgress = YES;
  NSRunLoop *theRL = [NSRunLoop currentRunLoop];
  while (dragInProgress && [theRL runMode: NSDefaultRunLoopMode beforeDate: [NSDate distantFuture]])
    ;

  return self.lastDragOperation;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)draggingSession: (NSDraggingSession *)session
           endedAtPoint: (NSPoint)screenPoint
              operation: (NSDragOperation)operation {
  dragInProgress = NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)pasteboard: (NSPasteboard *)sender item: (NSPasteboardItem *)item provideDataForType: (NSString *)type {
  if ([type isEqualTo:NSPasteboardTypeString])
    [sender setString:dragText forType: NSPasteboardTypeString];
  else
    [sender writeNativeData: dragData typeAsString: type];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSDragOperation)draggingSession: (NSDraggingSession *)session
  sourceOperationMaskForDraggingContext: (NSDraggingContext)context;
{
  switch (context) {
    case NSDraggingContextOutsideApplication:
      return NSDragOperationNone;
      break;

    case NSDraggingContextWithinApplication:
    default:
      mforms::DragOperation operations = self.allowedDragOperations;
      NSDragOperation nativeOperations = NSDragOperationNone;
      if ((operations & mforms::DragOperationMove) == mforms::DragOperationMove)
        nativeOperations |= NSDragOperationMove;
      if ((operations & mforms::DragOperationCopy) == mforms::DragOperationCopy)
        nativeOperations |= NSDragOperationMove;
      return nativeOperations;
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (mforms::ModifierKey)modifiersFromEvent:(NSEvent *)event {
  NSUInteger modifiers = event.modifierFlags;
  mforms::ModifierKey mforms_modifiers = mforms::ModifierNoModifier;

  if ((modifiers & NSEventModifierFlagControl) != 0)
    mforms_modifiers = mforms::ModifierKey(mforms_modifiers | mforms::ModifierControl);
  if ((modifiers & NSEventModifierFlagShift) != 0)
    mforms_modifiers = mforms::ModifierKey(mforms_modifiers | mforms::ModifierShift);
  if ((modifiers & NSEventModifierFlagCommand) != 0)
    mforms_modifiers = mforms::ModifierKey(mforms_modifiers | mforms::ModifierCommand);
  if ((modifiers & NSEventModifierFlagOption) != 0)
    mforms_modifiers = mforms::ModifierKey(mforms_modifiers | mforms::ModifierAlt);

  return mforms_modifiers;
}

//----------------------------------------------------------------------------------------------------------------------

- (mforms::KeyCode)keyFromEvent:(NSEvent *)event {
  std::vector<int> letters = {kVK_ANSI_A, kVK_ANSI_S, kVK_ANSI_D, kVK_ANSI_F, kVK_ANSI_H, kVK_ANSI_G, kVK_ANSI_Z,
    kVK_ANSI_X, kVK_ANSI_C, kVK_ANSI_V, kVK_ANSI_B, kVK_ANSI_Q, kVK_ANSI_W, kVK_ANSI_E, kVK_ANSI_R,
    kVK_ANSI_Y, kVK_ANSI_T, kVK_ANSI_O, kVK_ANSI_U, kVK_ANSI_I, kVK_ANSI_P, kVK_ANSI_L, kVK_ANSI_J, kVK_ANSI_K, kVK_ANSI_N, kVK_ANSI_M};
  mforms::KeyCode code = mforms::KeyUnkown;
  switch (event.keyCode) {
    case kVK_Home:
      code = mforms::KeyHome;
      break;
    case kVK_End:
      code = mforms::KeyEnd;
      break;
    case kVK_PageUp:
      code = mforms::KeyPrevious;
      break;
    case kVK_PageDown:
      code = mforms::KeyNext;
      break;
    case kVK_UpArrow:
      code = mforms::KeyUp;
      break;
    case kVK_DownArrow:
      code = mforms::KeyDown;
      break;
      
    case kVK_Return:
      code = mforms::KeyReturn;
      break;
    case kVK_Tab:
      code = mforms::KeyTab;
      break;
    case kVK_ANSI_KeypadEnter:
      code = mforms::KeyEnter;
      break;
    case kVK_F1:
      code = mforms::KeyF1;
      break;
    case kVK_F2:
      code = mforms::KeyF2;
      break;
    case kVK_F3:
      code = mforms::KeyF3;
      break;
    case kVK_F4:
      code = mforms::KeyF4;
      break;
    case kVK_F5:
      code = mforms::KeyF5;
      break;
    case kVK_F6:
      code = mforms::KeyF6;
      break;
    case kVK_F7:
      code = mforms::KeyF7;
      break;
    case kVK_F8:
      code = mforms::KeyF8;
      break;
    case kVK_F9:
      code = mforms::KeyF9;
      break;
    case kVK_F10:
      code = mforms::KeyF10;
      break;
    case kVK_F11:
      code = mforms::KeyF11;
      break;
    case kVK_F12:
      code = mforms::KeyF12;
      break;
    case kVK_RightCommand:
    case kVK_RightShift:
    case kVK_RightOption:
    case kVK_RightControl:
    case kVK_Shift:
    case kVK_Command:
    case kVK_Option:
    case kVK_Control:
      code = mforms::KeyModifierOnly;
      break;
    default:
      auto it = std::find(letters.begin(), letters.end(), event.keyCode);
      if (it != letters.end())
        code = mforms::KeyChar;
      break;
  }
  
  return code;
}

@end

//----------------------------------------------------------------------------------------------------------------------

NSView *nsviewForView(mforms::View *view) {
  id obj = view->get_data();

  return obj;
}

//----------------------------------------------------------------------------------------------------------------------

#pragma mark -

@implementation NSPasteboard (MySQLWorkbench)

- (void)writeNativeData: (void *)data typeAsString: (NSString *)type {
  PasteboardDataWrapper wrapper;
  wrapper.data = data;
  NSData *pasteboardData = [NSData dataWithBytes: &wrapper length: sizeof(wrapper)];
  [self addTypes: @[type] owner: nil];
  [self setData: pasteboardData forType: type];
}

- (void)writeNativeData: (void *)data typeAsChar: (const char *)type {
  NSString *format = @(type);
  [self writeNativeData: data typeAsString: format];
}

- (void *)nativeDataForTypeAsString: (NSString *)type {
  NSData *data = [self dataForType: type];
  PasteboardDataWrapper wrapper;
  [data getBytes:&wrapper length: sizeof(wrapper)];
  if (strncmp(wrapper.identifier, "mysql-workbench", 15) == 0)
    return wrapper.data;

  return NULL;
}

//----------------------------------------------------------------------------------------------------------------------

- (void *)nativeDataForTypeAsChar: (const char *)type {
  NSString *format = @(type);
  return [self nativeDataForTypeAsString: format];
}

@end

//----------------------------------------------------------------------------------------------------------------------

#pragma mark - Static functions

static void view_destroy(mforms::View *self) {
  id view = self->get_data();
  SEL selector = NSSelectorFromString(@"destroy");
  if (view && [view respondsToSelector: selector])
    ((void (*)(id, SEL))[view methodForSelector:selector])(view, selector);

  if ([view respondsToSelector:@selector(superview)] && [view superview])
    [view removeFromSuperview];
}

//----------------------------------------------------------------------------------------------------------------------

static int view_get_x(const mforms::View *self) {
  id view = self->get_data();
  if (view) {
    NSView *widget = view;
    return NSMinX(widget.frame);
  }
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

static int view_get_y(const mforms::View *self) {
  id view = self->get_data();
  if (view) {
    NSView *widget = view;
    return NSMinY(widget.frame);
  }
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

static void view_set_size(mforms::View *self, int w, int h) {
  id frontend = self->get_data();
  if ([frontend isKindOfClass: NSView.class]) {
    NSView *view = frontend;
    NSSize size = {(CGFloat)w, (CGFloat)h};
    if (w < 0)
      size.width = view.frame.size.width;
    if (h < 0)
      size.height = view.frame.size.height;
    view.frameSize = size;
    if (w < 0)
      size.width = 0;
    if (h < 0)
      size.height = 0;
    view.minimumSize = size;
  } else {
    // Window controller/window/panel.
    NSWindow *window;
    if ([frontend isKindOfClass: NSWindowController.class])
      window = [frontend window];
    else
      window = frontend;
    NSRect frame = window.frame;
    if (w >= 0)
      frame.size.width = w;
    if (h >= 0)
      frame.size.height = h;
    [window setFrame: frame display: YES animate: NO];
    window.minSize = frame.size;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void view_set_min_size(mforms::View *self, int w, int h) {
  id frontend = self->get_data();
  if ([frontend isKindOfClass: NSView.class]) {
    NSView *view = frontend;
    NSSize size = {(CGFloat)w, (CGFloat)h};
    if (w < 0)
      size.width = 0;
    if (h < 0)
      size.height = 0;
    view.minimumSize = size;
  } else {
    // Window/panel.
    NSWindow *window = frontend;
    NSRect frame = window.frame;
    if (w >= 0)
      frame.size.width = w;
    if (h >= 0)
      frame.size.height = h;
    window.minSize = frame.size;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void view_set_position(mforms::View *self, int x, int y) {
  NSView *view = self->get_data();
  view.frameOrigin = {(CGFloat)x, (CGFloat)y};
}

//----------------------------------------------------------------------------------------------------------------------

static std::pair<int, int> view_client_to_screen(mforms::View *self, int x, int y) {
  id view = self->get_data();
  if (view) {
    NSRect rect = NSMakeRect(x, y, 0, 0);
    rect.origin = [view convertPoint: rect.origin toView: nil];
    rect = [[view window] convertRectToScreen: rect];
    return std::make_pair(rect.origin.x, rect.origin.y);
  }
  return std::make_pair(0, 0);
}

//----------------------------------------------------------------------------------------------------------------------

static std::pair<int, int> view_screen_to_client(mforms::View *self, int x, int y) {
  id view = self->get_data();
  if (view) {
    NSRect rect = NSMakeRect(x, y, 0, 0);
    rect = [[view window] convertRectFromScreen: rect];
    NSPoint localPoint = [view convertPoint: rect.origin fromView: nil];
    return std::make_pair(localPoint.x, localPoint.y);
  }
  return std::make_pair(0, 0);
}

//----------------------------------------------------------------------------------------------------------------------

static void view_set_enabled(mforms::View *self, bool flag) {
  id view = self->get_data();
  if (view) {
    if ([view respondsToSelector: @selector(setEnabled:)])
      [view setEnabled:flag ? YES : NO];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static bool view_is_enabled(mforms::View *self) {
  id view = self->get_data();
  if (view) {
    if ([view respondsToSelector: @selector(isEnabled)])
      return [view isEnabled];
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

static int view_get_width(const mforms::View *self) {
  id view = self->get_data();
  if (view) {
    if ([view isKindOfClass: [NSWindow class]])
      return NSWidth([view contentRectForFrameRect: [view frame]]);
    return NSWidth([view frame]);
  }
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

static int view_get_height(const mforms::View *self) {
  id frontend = self->get_data();
  if (frontend != nil) {
    if ([frontend isKindOfClass: NSWindow.class])
      return NSHeight([frontend contentRectForFrameRect: [frontend frame]]);
    return NSHeight([frontend frame]);
  }
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

static int view_get_preferred_width(mforms::View *self) {
  id frontend = self->get_data();
  if (frontend != nil) {
    NSSize size = [frontend preferredSize: [frontend frame].size];
    return size.width;
  }
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

static int view_get_preferred_height(mforms::View *self) {
  id frontend = self->get_data();
  if (frontend != nil) {
    NSSize size = [frontend preferredSize: [frontend frame].size];
    return size.height;
  }
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

static void view_show(mforms::View *self, bool show) {
  id frontend = self->get_data();
  if ([frontend isKindOfClass: NSView.class]) {
    NSView *view = frontend;

    if (view.isHidden != !show) {
      view.hidden = !show;
      [view.superview relayout];
      [view.window recalculateKeyViewLoop];
    }
  } else {
    if ([frontend isKindOfClass: NSWindow.class]) {
      [frontend orderFrontRegardless];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static bool view_is_shown(mforms::View *self) {
  id view = self->get_data();
  if (view)
    return ![view isHidden];
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

static bool view_is_fully_visible(mforms::View *self) {
  NSView *view = self->get_data();
  if (view) {
    if (view.window == nil)
      return false;

    return !view.isHiddenOrHasHiddenAncestor;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

static void view_set_tooltip(mforms::View *self, const std::string &text) {
  id view = self->get_data();
  if (view) {
    [view setToolTip:wrap_nsstring(text)];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void view_set_font(mforms::View *self, const std::string &fontDescription) {
  id view = self->get_data();
  if (view && [view respondsToSelector:@selector(setFont:)]) {
    std::string name;
    float size;
    bool bold;
    bool italic;
    if (base::parse_font_description(fontDescription, name, size, bold, italic)) {
      int traitMask = 0;
      if (bold)
        traitMask |= NSBoldFontMask;
      if (italic)
        traitMask |= NSItalicFontMask;
      NSFontManager *fontManager = [NSFontManager sharedFontManager];
      NSFont *font = [fontManager fontWithFamily: @(name.c_str()) traits: traitMask weight: 0 size: size];
      [view setFont:font];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void view_set_name(mforms::View *self, const std::string &name) {
  NSView *view = self->get_data();
  view.accessibilityLabel = [NSString stringWithUTF8String: name.c_str()];
}

//----------------------------------------------------------------------------------------------------------------------

static void view_relayout(mforms::View *self) {
  id view = self->get_data();
  [view performSelectorOnMainThread: @selector(relayout) withObject: nil waitUntilDone: YES];
}

//----------------------------------------------------------------------------------------------------------------------

static void view_set_needs_repaint(mforms::View *self) {
  [self->get_data() setNeedsDisplay: YES];
}

//----------------------------------------------------------------------------------------------------------------------

static void view_suspend_layout(mforms::View *self, bool flag) {
  if ([self->get_data() respondsToSelector:@selector(setFreezeRelayout:)])
    [self->get_data() setFreezeRelayout: flag];
}

//----------------------------------------------------------------------------------------------------------------------

static void view_set_front_color(mforms::View *self, const std::string &color) {
  // Foreground color means text color, so that is supported only by text storage and text layer controls.
  if ([self->get_data() respondsToSelector:@selector(setTextColor:)])
    [self->get_data() setTextColor:[NSColor colorFromHexString:@(color.c_str())]];
}

//----------------------------------------------------------------------------------------------------------------------

static std::string view_get_front_color(mforms::View *self) {
  if ([self->get_data() respondsToSelector:@selector(textColor)]) {
    return [self->get_data() textColor].hexString.UTF8String;
  }
  return "";
}

//----------------------------------------------------------------------------------------------------------------------

static void view_set_back_color(mforms::View *self, const std::string &color) {
  id view = self->get_data();
  if ([view respondsToSelector:@selector(setBackgroundColor:)]) {
    [view setBackgroundColor: [NSColor colorFromHexString: @(color.c_str())]];
    if ([view respondsToSelector: @selector(setDrawsBackground:)])
      [view setDrawsBackground: !color.empty()];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static std::string view_get_back_color(mforms::View *self) {
  id view = self->get_data();
  if ([view respondsToSelector: @selector(backgroundColor)]) {
    if ([view respondsToSelector: @selector(drawsBackground)] && ![self->get_data() drawsBackground])
      return "";
    if ([view backgroundColor] == nil)
      return "";
    return [view backgroundColor].hexString.UTF8String;
  }
  return "";
}

//----------------------------------------------------------------------------------------------------------------------

static void view_set_back_image(mforms::View *self, const std::string &path, mforms::Alignment align) {
  if ([self->get_data() respondsToSelector: @selector(setBackgroundImage:withAlignment:)])
    [self->get_data() setBackgroundImage: wrap_nsstring(path) withAlignment: align];
}

//----------------------------------------------------------------------------------------------------------------------

static void view_flush_events(mforms::View *) {
}

//----------------------------------------------------------------------------------------------------------------------

static void view_set_padding(mforms::View *self, int left, int top, int right, int bottom) {
  if ([self->get_data() respondsToSelector:@selector(setPaddingLeft:right:top:bottom:)])
    [self->get_data() setPaddingLeft: left right: right top: top bottom: bottom];
}

//----------------------------------------------------------------------------------------------------------------------

static void view_focus(mforms::View *self) {
  id frontend = self->get_data();
  if ([frontend isKindOfClass: NSView.class]) {
    [[frontend window] makeKeyAndOrderFront: frontend];
    [[frontend window] makeFirstResponder: frontend];
  } else
    [frontend makeKeyAndOrderFront: frontend];
}

//----------------------------------------------------------------------------------------------------------------------

static bool view_has_focus(mforms::View *self) {
  id view = self->get_data();
  id firstResponder = [view window].firstResponder;
  if (firstResponder == view)
    return true;
  if ([firstResponder respondsToSelector: @selector(delegate)] && [firstResponder delegate] == view)
    return true;
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

static void register_drop_formats(mforms::View *self, mforms::DropDelegate *target,
                                  const std::vector<std::string> &formats) {
  NSMutableArray *list = [[NSMutableArray alloc] init];
  for (size_t i = 0; i < formats.size(); ++i) {
    if (formats[i] == mforms::DragFormatText)
      [list addObject:NSPasteboardTypeString];
    else if (formats[i] == mforms::DragFormatFileName)
      [list addObject: NSPasteboardTypeFileURL];
    else
      [list addObject: @(formats[i].c_str())];
  }
  NSView *view = self->get_data();

  view.acceptableDropFormats = list;
  view.dropDelegate = target;
}

//----------------------------------------------------------------------------------------------------------------------

static mforms::DragOperation view_drag_text(mforms::View *self, mforms::DragDetails details, const std::string &text) {
  NSView *view = self->get_data();
  return [view startDragWithText: @(text.c_str()) details: details];
}

//----------------------------------------------------------------------------------------------------------------------

static mforms::DragOperation view_drag_data(mforms::View *self, mforms::DragDetails details, void *data,
                                            const std::string &format) {
  NSView *view = self->get_data();
  return [view startDragWithData:data details:details format:@(format.c_str())];
}

//----------------------------------------------------------------------------------------------------------------------

static mforms::DropPosition view_get_drop_position(mforms::View *self) {
  NSView *view = self->get_data();
  return view.lastDropPosition;
}

//----------------------------------------------------------------------------------------------------------------------

void cf_view_init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_view_impl.destroy = &view_destroy;

  f->_view_impl.get_width = &view_get_width;
  f->_view_impl.get_height = &view_get_height;
  f->_view_impl.get_preferred_width = &view_get_preferred_width;
  f->_view_impl.get_preferred_height = &view_get_preferred_height;
  f->_view_impl.set_size = &view_set_size;
  f->_view_impl.set_min_size = &view_set_min_size;
  f->_view_impl.set_padding = &view_set_padding;

  f->_view_impl.get_x = &view_get_x;
  f->_view_impl.get_y = &view_get_y;
  f->_view_impl.set_position = &view_set_position;
  f->_view_impl.client_to_screen = &view_client_to_screen;
  f->_view_impl.screen_to_client = &view_screen_to_client;

  f->_view_impl.show = &view_show;
  f->_view_impl.is_shown = &view_is_shown;
  f->_view_impl.is_fully_visible = &view_is_fully_visible;

  f->_view_impl.set_tooltip = &view_set_tooltip;
  f->_view_impl.set_name = &view_set_name;
  f->_view_impl.set_font = &view_set_font;

  f->_view_impl.set_enabled = &view_set_enabled;
  f->_view_impl.is_enabled = &view_is_enabled;
  f->_view_impl.relayout = &view_relayout;
  f->_view_impl.set_needs_repaint = &view_set_needs_repaint;

  f->_view_impl.suspend_layout = &view_suspend_layout;
  f->_view_impl.set_front_color = &view_set_front_color;
  f->_view_impl.set_back_color = &view_set_back_color;
  f->_view_impl.get_front_color = &view_get_front_color;
  f->_view_impl.get_back_color = &view_get_back_color;
  f->_view_impl.set_back_image = &view_set_back_image;

  f->_view_impl.flush_events = &view_flush_events;
  f->_view_impl.focus = &view_focus;
  f->_view_impl.has_focus = &view_has_focus;

  f->_view_impl.register_drop_formats = &register_drop_formats;
  f->_view_impl.drag_text = &view_drag_text;
  f->_view_impl.drag_data = &view_drag_data;
  f->_view_impl.get_drop_position = &view_get_drop_position;
}

//----------------------------------------------------------------------------------------------------------------------
