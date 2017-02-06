/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#import "MFDrawBox.h"
#import "MFMForms.h"
#include <cairo/cairo-quartz.h>

@implementation MFDrawBoxImpl

@synthesize drawsBackground = mDrawsBackground;
@synthesize backgroundColor = mBackgroundColor;

- (instancetype)initWithObject:(mforms::DrawBox *)aBox {
  self = [super initWithFrame: NSMakeRect(10, 10, 10, 10)];
  if (self) {
    mOwner = aBox;
    mOwner->set_data(self);
  }
  return self;
}

- (void)dealloc {
  [NSObject cancelPreviousPerformRequestsWithTarget: self];
}

- (mforms::Object *)mformsObject {
  return mOwner;
}

- (BOOL)isFlipped {
  return YES;
}

- (void)setBackgroundColor: (NSColor *)value {
  if (mBackgroundColor != value) {
    mBackgroundColor = value;
    [self setNeedsDisplay: YES];
  }
}

- (void)setDrawsBackground: (BOOL)flag {
  if (mDrawsBackground != flag) {
    mDrawsBackground = flag;
    [self setNeedsDisplay: YES];
  }
}

- (void)cancelOperation: (id)sender {
  mOwner->cancel_operation();
}

- (void)drawRect: (NSRect)rect {
  NSRect bounds = self.bounds;

  if (mDrawsBackground && mBackgroundColor != nil && ![mBackgroundColor isEqualTo: NSColor.clearColor]) {
    [mBackgroundColor set];
    NSRectFill(bounds);
  }

  CGContextRef cgref = (CGContextRef)NSGraphicsContext.currentContext.graphicsPort;

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

//--------------------------------------------------------------------------------------------------

STANDARD_MOUSE_HANDLING(self) // Add handling for mouse events.
STANDARD_FOCUS_HANDLING(self) // Notify backend when getting first responder status.

//--------------------------------------------------------------------------------------------------

- (void)invalidate {
  [self setNeedsDisplay: YES];
}

//--------------------------------------------------------------------------------------------------

- (void)invalidateRect: (NSString *)rectStr {
  [self setNeedsDisplayInRect: NSRectFromString(rectStr)];
}

//--------------------------------------------------------------------------------------------------

- (NSSize)preferredSize: (NSSize)proposal {
  if (mOwner == NULL || mOwner->is_destroying())
    return NSZeroSize;

  base::Size nativeSize = mOwner->getLayoutSize(base::Size(proposal.width, proposal.height));
  NSSize size = NSMakeSize(nativeSize.width, nativeSize.height);

  return { MAX(size.width, self.minimumSize.width), MAX(size.height, self.minimumSize.height) };
}

//--------------------------------------------------------------------------------------------------

- (void)setPaddingLeft: (float)left right: (float)right top: (float)top bottom: (float)bottom {
  mPaddingLeft = left;
  mPaddingRight = right;
  mPaddingTop = top;
  mPaddingBottom = bottom;
}

//--------------------------------------------------------------------------------------------------

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

      [view setFrameOrigin: NSMakePoint(x, y)];
    }
  }
}

//--------------------------------------------------------------------------------------------------

static bool drawbox_create(mforms::DrawBox *self) {
  return [[MFDrawBoxImpl alloc] initWithObject: self] != nil;
}

//--------------------------------------------------------------------------------------------------

static void drawbox_set_needs_repaint(mforms::DrawBox *self) {
  // Invalidate the draw box in a thread safe manner.
  [self->get_data() performSelectorOnMainThread: @selector(invalidate) withObject: nil waitUntilDone: NO];
}

//--------------------------------------------------------------------------------------------------

static void drawbox_set_needs_repaint_area(mforms::DrawBox *self, int x, int y, int w, int h) {
  // Invalidate the draw box in a thread safe manner.
  SEL selector = NSSelectorFromString(@"invalidateRect");
  [self->get_data() performSelectorOnMainThread: selector
                                     withObject: NSStringFromRect(NSMakeRect(x, y, w, h))
                                  waitUntilDone: NO];
}

//--------------------------------------------------------------------------------------------------

static void drawbox_add(mforms::DrawBox *self, mforms::View *view, mforms::Alignment alignment) {
  MFDrawBoxImpl *box = self->get_data();
  box->mSubviews[view] = alignment;
  [box addSubview:view->get_data()];

  [box resizeSubviewsWithOldSize: box.frame.size];
}

//--------------------------------------------------------------------------------------------------

static void drawbox_remove(mforms::DrawBox *self, mforms::View *view) {
  MFDrawBoxImpl *box = self->get_data();
  box->mSubviews.erase(view);
  [view->get_data() removeFromSuperview];

  [box resizeSubviewsWithOldSize: box.frame.size];
}

//--------------------------------------------------------------------------------------------------

static void drawbox_move(mforms::DrawBox *self, mforms::View *view, int x, int y) {
  MFDrawBoxImpl *box = self->get_data();
  box->mSubviews[view] = mforms::NoAlign;
  NSView *child = view->get_data();
  [child setFrameOrigin: NSMakePoint(x, y)];
}

//--------------------------------------------------------------------------------------------------

void cf_drawbox_init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_drawbox_impl.create = &drawbox_create;
  f->_drawbox_impl.set_needs_repaint = &drawbox_set_needs_repaint;
  f->_drawbox_impl.set_needs_repaint_area = &drawbox_set_needs_repaint_area;
  f->_drawbox_impl.add = &drawbox_add;
  f->_drawbox_impl.remove = &drawbox_remove;
  f->_drawbox_impl.move = &drawbox_move;
}

@end

