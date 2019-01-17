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

#include "base/accessibility.h"

#import "MFDrawBox.h"
#import "MFMForms.h"
#include <cairo/cairo-quartz.h>

//----------------------------------------------------------------------------------------------------------------------

// TODO: move the accessibility helper to a base lib platform file.
static NSString *convertAccessibleRole(base::Accessible::Role be_role) {
  switch (be_role) {
    case base::Accessible::Window:
      return NSAccessibilityWindowRole;

    case base::Accessible::Pane:
      return NSAccessibilityGroupRole;

    case base::Accessible::Link:
      return NSAccessibilityLinkRole;

    case base::Accessible::List:
      return NSAccessibilityListRole;

    case base::Accessible::ListItem:
      return NSAccessibilityGroupRole;

    case base::Accessible::PushButton:
      return NSAccessibilityButtonRole;

    case base::Accessible::StaticText:
      return NSAccessibilityStaticTextRole;

    case base::Accessible::Text:
      return NSAccessibilityTextFieldRole;

    case base::Accessible::Outline:
      return NSAccessibilityOutlineRole;

    case base::Accessible::OutlineButton:
      return NSAccessibilityButtonRole;

    case base::Accessible::OutlineItem:
      return NSAccessibilityGroupRole;

    case base::Accessible::RoleNone:
      return NSAccessibilityUnknownRole;

    default:
      return NSAccessibilityUnknownRole;
  }
  return nil;
}

//----------------------------------------------------------------------------------------------------------------------

@implementation AccChildImpl

- (id)initWithObject: (base::Accessible *)acc parent: (mforms::View *)parentAcc {
  self = [super init];

  if (self) {
    mformsAcc = acc;
    parent = parentAcc;
  }

  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)accessibilityRole {
  return convertAccessibleRole(mformsAcc->getAccessibilityRole());
}

//----------------------------------------------------------------------------------------------------------------------

- (id)accessibilityParent {
  return parent->get_data();
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)accessibilityPerformPress {
  mformsAcc->accessibilityDoDefaultAction();
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)accessibilityPerformShowMenu {
  mformsAcc->accessibilityShowMenu();
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)accessibilityIdentifier {
  std::string name = mformsAcc->getAccessibilityIdentifier();
  return [NSString stringWithUTF8String: name.c_str()];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)accessibilityLabel {
  std::string label = mformsAcc->getAccessibilityDescription();
  return [NSString stringWithUTF8String: label.c_str()];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)accessibilityTitle {
  std::string title = mformsAcc->getAccessibilityTitle();
  return [NSString stringWithUTF8String: title.c_str()];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)accessibilityValue {
  std::string value = mformsAcc->getAccessibilityValue();
  return [NSString stringWithUTF8String: value.c_str()];
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isAccessibilityElement {
  return convertAccessibleRole(mformsAcc->getAccessibilityRole()) != NSAccessibilityUnknownRole;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSRect)accessibilityFrame {
  base::Rect accBounds = mformsAcc->getAccessibilityBounds();
  auto point = parent->client_to_screen(accBounds.left(), accBounds.top());
  return NSMakeRect(point.first, point.second - accBounds.height(), accBounds.width(), accBounds.height());
}

//----------------------------------------------------------------------------------------------------------------------

- (id)accessibilityHitTest: (NSPoint)point {
  std::pair<int, int> p = dynamic_cast<mforms::View *>(mformsAcc)->screen_to_client(point.x, point.y);

  base::Accessible *acc = mformsAcc->accessibilityHitTest(p.first, p.second);
  if (acc != nullptr) {
    if (acc == mformsAcc)
      return self;
    
    auto it = accChildList.find(acc);
    if (it != accChildList.end())
      return it->second;
    
    AccChildImpl *accChild = [[AccChildImpl alloc] initWithObject: acc parent: parent];
    accChildList.insert({ acc, accChild });
    return accChild;
  }
  return self;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFDrawBoxImpl

@synthesize drawsBackground = mDrawsBackground;
@synthesize backgroundColor = mBackgroundColor;

- (instancetype)initWithObject: (mforms::DrawBox *)aBox {
  self = [super initWithFrame: NSMakeRect(10, 10, 10, 10)];
  if (self) {
    mOwner = aBox;
    mOwner->set_data(self);
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  [NSObject cancelPreviousPerformRequestsWithTarget: self];
}

//----------------------------------------------------------------------------------------------------------------------

- (mforms::Object *)mformsObject {
  return mOwner;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isFlipped {
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setBackgroundColor:(NSColor *)value {
  if (mBackgroundColor != value) {
    mBackgroundColor = value;
    [self setNeedsDisplay: YES];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setDrawsBackground: (BOOL)flag {
  if (mDrawsBackground != flag) {
    mDrawsBackground = flag;
    [self setNeedsDisplay: YES];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isAccessibilityElement {
  return convertAccessibleRole(mOwner->getAccessibilityRole()) != NSAccessibilityUnknownRole;
}

//----------------------------------------------------------------------------------------------------------------------

- (id)accessibilityHitTest: (NSPoint)point {
  std::pair<int, int> p = mOwner->screen_to_client(point.x, point.y);
  base::Accessible *acc = mOwner->accessibilityHitTest(p.first, p.second);
  if (acc != nullptr) {
    auto it = accChildList.find(acc);
    if (it != accChildList.end())
      return it->second;

    AccChildImpl *accChild = [[AccChildImpl alloc] initWithObject: acc parent: mOwner];
    accChildList.insert({ acc, accChild });
    return accChild;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)accessibilityRole {
  return convertAccessibleRole(mOwner->getAccessibilityRole());
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)accessibilityIdentifier {
  std::string name = mOwner->getAccessibilityIdentifier();
  return [NSString stringWithUTF8String:name.c_str()];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)accessibilityLabel {
  std::string description = mOwner->getAccessibilityDescription();
  return [NSString stringWithUTF8String:description.c_str()];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)accessibilityTitle {
  std::string title = mOwner->getAccessibilityTitle();
  return [NSString stringWithUTF8String:title.c_str()];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSArray *)accessibilityChildren {
  NSMutableArray *children = [[super accessibilityChildren] mutableCopy];
  size_t count = mOwner->getAccessibilityChildCount();
  for (size_t i = 0; i < count; ++i) {
    base::Accessible *acc = mOwner->getAccessibilityChild(i);
    auto it = accChildList.find(acc);
    if (it != accChildList.end()) {
      [children addObject: it->second];
      continue;
    }
    AccChildImpl *accChild = [[AccChildImpl alloc] initWithObject: acc parent: mOwner];
    accChildList.insert({ acc, accChild });
    [children addObject: accChild];
  }
  return children;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)cancelOperation: (id)sender {
  mOwner->cancel_operation();
}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawRect: (NSRect)rect {
  NSRect bounds = self.bounds;

  if (mDrawsBackground && mBackgroundColor != nil && ![mBackgroundColor isEqualTo: NSColor.clearColor]) {
    [mBackgroundColor set];
    NSRectFill(bounds);
  }

  CGContextRef cgref = (CGContextRef)NSGraphicsContext.currentContext.CGContext;

  cairo_surface_t *surface = cairo_quartz_surface_create_for_cg_context(cgref, NSWidth(bounds), NSHeight(bounds));

  cairo_t *cr = cairo_create(surface);
  try {
    mOwner->repaint(cr, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
  } catch (...) {
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    throw;
  }
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
}

//----------------------------------------------------------------------------------------------------------------------

STANDARD_MOUSE_HANDLING(self) // Add handling for mouse events.
STANDARD_FOCUS_HANDLING(self) // Notify backend when getting first responder status.
STANDARD_KEYBOARD_HANDLING(self)

//----------------------------------------------------------------------------------------------------------------------

- (void)invalidate {
  [self setNeedsDisplay: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)invalidateRect :(NSString *)rectStr {
  [self setNeedsDisplayInRect: NSRectFromString(rectStr)];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)preferredSize:(NSSize)proposal {
  if (mOwner == NULL || mOwner->is_destroying())
    return NSZeroSize;

  base::Size nativeSize = mOwner->getLayoutSize(base::Size(proposal.width, proposal.height));
  NSSize size = NSMakeSize(nativeSize.width, nativeSize.height);

  return {MAX(size.width, self.minimumSize.width), MAX(size.height, self.minimumSize.height)};
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setPaddingLeft: (float)left right: (float)right top: (float)top bottom: (float)bottom {
  mPaddingLeft = left;
  mPaddingRight = right;
  mPaddingTop = top;
  mPaddingBottom = bottom;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)resizeSubviewsWithOldSize: (NSSize)oldBoundsSize {
  if (!mOwner->is_destroying()) {
    NSSize frameSize = self.frame.size;

    for (std::map<mforms::View *, mforms::Alignment>::const_iterator iterator = mSubviews.begin();
         iterator != mSubviews.end(); ++iterator) {
      NSView *view = iterator->first->get_data();
      float x, y;
      NSSize viewSize = view.frame.size;

      switch (iterator->second) {
        case mforms::BottomLeft:
        case mforms::MiddleLeft:
        case mforms::TopLeft:
          x = mPaddingLeft;
          break;

        case mforms::BottomCenter:
        case mforms::MiddleCenter:
        case mforms::TopCenter:
          x = (frameSize.width - viewSize.width) / 2;
          break;

        case mforms::BottomRight:
        case mforms::MiddleRight:
        case mforms::TopRight:
          x = frameSize.width - mPaddingRight - viewSize.width;
          break;

        default:
          x = 0;
          break;
      }

      switch (iterator->second) {
        case mforms::BottomLeft:
        case mforms::BottomCenter:
        case mforms::BottomRight:
          if (!self.flipped)
            y = mPaddingBottom;
          else
            y = frameSize.height - mPaddingBottom - viewSize.height;
          break;

        case mforms::MiddleLeft:
        case mforms::MiddleCenter:
        case mforms::MiddleRight:
          y = (frameSize.height - viewSize.height) / 2;
          break;

        case mforms::TopLeft:
        case mforms::TopCenter:
        case mforms::TopRight:
          if (self.flipped)
            y = mPaddingTop;
          else
            y = frameSize.height - mPaddingTop - viewSize.height;
          break;

        default:
          y = 0;
          break;
      }

      [view setFrameOrigin:NSMakePoint(x, y)];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static bool drawbox_create(mforms::DrawBox *self) {
  return [[MFDrawBoxImpl alloc] initWithObject: self] != nil;
}

//----------------------------------------------------------------------------------------------------------------------

static void drawbox_set_needs_repaint(mforms::DrawBox *self) {
  // Invalidate the draw box in a thread safe manner.
  [self->get_data() performSelectorOnMainThread: @selector(invalidate) withObject: nil waitUntilDone: NO];
}

//----------------------------------------------------------------------------------------------------------------------

static void drawbox_set_needs_repaint_area(mforms::DrawBox *self, int x, int y, int w, int h) {
  // Invalidate the draw box in a thread safe manner.
  SEL selector = NSSelectorFromString(@"invalidateRect");
  [self->get_data() performSelectorOnMainThread: selector
                                     withObject: NSStringFromRect(NSMakeRect(x, y, w, h))
                                  waitUntilDone: NO];
}

//----------------------------------------------------------------------------------------------------------------------

static void drawbox_add(mforms::DrawBox *self, mforms::View *view, mforms::Alignment alignment) {
  MFDrawBoxImpl *box = self->get_data();
  box->mSubviews[view] = alignment;
  [box addSubview:view->get_data()];

  [box resizeSubviewsWithOldSize: box.frame.size];
}

//----------------------------------------------------------------------------------------------------------------------

static void drawbox_remove(mforms::DrawBox *self, mforms::View *view) {
  MFDrawBoxImpl *box = self->get_data();
  box->mSubviews.erase(view);
  [view->get_data() removeFromSuperview];

  [box resizeSubviewsWithOldSize: box.frame.size];
}

//----------------------------------------------------------------------------------------------------------------------

static void drawbox_move(mforms::DrawBox *self, mforms::View *view, int x, int y) {
  MFDrawBoxImpl *box = self->get_data();
  box->mSubviews[view] = mforms::NoAlign;
  NSView *child = view->get_data();
  [child setFrameOrigin: NSMakePoint(x, y)];
}

//----------------------------------------------------------------------------------------------------------------------

static void drawbox_drawFocus(mforms::DrawBox *self, cairo_t *cr, const base::Rect r) {
  auto bounds = r;
  bounds.use_inter_pixel = true;

  [NSGraphicsContext saveGraphicsState];
  NSSetFocusRingStyle(NSFocusRingOnly);
  [[NSBezierPath bezierPathWithRect: {{bounds.left(), bounds.top()}, {bounds.width() - 2, bounds.height() - 2}}] fill];
  [NSGraphicsContext restoreGraphicsState];
}

//----------------------------------------------------------------------------------------------------------------------

void cf_drawbox_init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_drawbox_impl.create = &drawbox_create;
  f->_drawbox_impl.set_needs_repaint = &drawbox_set_needs_repaint;
  f->_drawbox_impl.set_needs_repaint_area = &drawbox_set_needs_repaint_area;
  f->_drawbox_impl.add = &drawbox_add;
  f->_drawbox_impl.remove = &drawbox_remove;
  f->_drawbox_impl.move = &drawbox_move;
  f->_drawbox_impl.drawFocus = &drawbox_drawFocus;
}

@end

//----------------------------------------------------------------------------------------------------------------------
