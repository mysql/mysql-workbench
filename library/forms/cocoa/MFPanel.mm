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

#import "MFPanel.h"
#import "NSColor_extras.h"

#import "MFMForms.h"

#import "MFRadioButton.h" // for handling radio groups

@interface MFPanelContent : NSView {
  MFPanelImpl *panel;
  NSImage *mBackImage;
  mforms::Alignment mBackImageAlignment;
  float mLeftPadding;
  float mRightPadding;
  float mTopPadding;
  float mBottomPadding;

  NSTrackingArea *mTrackingArea;
}

@end

@implementation MFPanelContent

- (instancetype)initWithPanel: (MFPanelImpl *)aPanel {
  self = [super initWithFrame:aPanel.bounds];
  if (self) {
    panel = aPanel;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

STANDARD_MOUSE_HANDLING(panel) // Add handling for mouse events.

//----------------------------------------------------------------------------------------------------------------------

- (void)setBackgroundImage: (NSString *)path withAlignment: (mforms::Alignment)align {
  if (path)
    mBackImage = [[NSImage alloc] initWithContentsOfFile:path];
  else
    mBackImage = nil;
  mBackImageAlignment = align;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isFlipped {
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setPaddingLeft: (float)lpad right: (float)rpad top: (float)tpad bottom: (float)bpad {
  mLeftPadding = lpad;
  mRightPadding = rpad;
  mTopPadding = tpad;
  mBottomPadding = bpad;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Computes the layout of the content view for a panel. It's very similar to MFForm (only one child filling the entire
 * view).
 *
 * @param proposedSize The size to start from layouting.
 * @param resizeChildren Tells the function whether the computed client control bounds should be applied
 *                      (when doing a relayout) or not (when computing the preferred size).
 * @return The resulting size of the content view.
 */
- (NSSize)computeLayout: (NSSize)proposedSize resizeChildren: (BOOL)doResize {
  NSView *child = self.subviews.lastObject;
  if (child != nil) {
    float horizontalPadding = mLeftPadding + mRightPadding;
    float verticalPadding = mTopPadding + mBottomPadding;

    proposedSize.width -= horizontalPadding;
    proposedSize.height -= verticalPadding;
    child.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    child.autoresizingMask = 0;
    NSSize contentSize = [child preferredSize: proposedSize];
    NSSize minSize = self.minimumSize;
    // Adjust width of the container if it is too small or auto resizing is enabled.
    if (proposedSize.width < contentSize.width || (self.autoresizingMask & NSViewWidthSizable) != 0) {
      proposedSize.width = contentSize.width;
      if (proposedSize.width < minSize.width - horizontalPadding)
        proposedSize.width = minSize.width - horizontalPadding;
    }

    // Adjust height of the container if it is too small or auto resizing is enabled.
    if (proposedSize.height < contentSize.height || (self.autoresizingMask & NSViewHeightSizable) != 0) {
      proposedSize.height = contentSize.height;
      if (proposedSize.height < minSize.height - verticalPadding)
        proposedSize.height = minSize.height - verticalPadding;
    }

    if (doResize) {
      // Now stretch the client view to fill the entire display area.
      child.autoresizingMask = 0;
      child.frame = {{ mLeftPadding, mTopPadding }, proposedSize };
    }

    proposedSize.width += horizontalPadding;
    proposedSize.height += verticalPadding;
  }

  return proposedSize;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)preferredSize: (NSSize)proposedSize {
  return [self computeLayout: proposedSize resizeChildren: NO];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)resizeSubviewsWithOldSize: (NSSize)oldBoundsSize {
  self.autoresizingMask = 0;
  [self computeLayout: self.frame.size resizeChildren: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)minimumSize {
  NSSize size = self.subviews.lastObject.minimumSize;
  size.width += mLeftPadding + mRightPadding;
  size.height += mTopPadding + mBottomPadding;
  return size;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawRect: (NSRect)rect {
  [super drawRect:rect];

  if (mBackImage) {
    float x = 0, y = 0;
    NSSize isize = mBackImage.size;
    NSSize fsize = self.frame.size;

    switch (mBackImageAlignment) {
      case mforms::BottomLeft:
        x = 0;
        y = fsize.height - isize.height;
        break;
      case mforms::BottomCenter:
        x = (isize.width + fsize.width) / 2;
        y = fsize.height - isize.height;
        break;
      case mforms::BottomRight:
        x = fsize.width - isize.width;
        y = fsize.height - isize.height;
        break;
      case mforms::MiddleLeft:
        x = 0;
        y = (isize.height + fsize.height) / 2;
        break;
      case mforms::MiddleCenter:
        x = (isize.width + fsize.width) / 2;
        y = (isize.height + fsize.height) / 2;
        break;
      case mforms::MiddleRight:
        x = fsize.width - isize.width;
        y = (isize.height + fsize.height) / 2;
        break;
      case mforms::TopLeft:
        x = 0;
        y = 0;
        break;
      case mforms::TopCenter:
        x = (isize.width + fsize.width) / 2;
        y = 0;
        break;
      case mforms::TopRight:
        x = fsize.width - isize.width;
        y = 0;
        break;
      default:
        break;
    }

    [mBackImage drawInRect:NSMakeRect(x, y, isize.width, isize.height)
                  fromRect:NSZeroRect
                 operation:NSCompositingOperationSourceOver
                  fraction:1.0
            respectFlipped:YES
                     hints:nil];
  } else if (panel->mType == mforms::StyledHeaderPanel) {
    [[[NSGradient alloc] initWithStartingColor:[NSColor colorWithDeviceWhite:0.9 alpha:1]
                                   endingColor:[NSColor whiteColor]] drawInRect:rect
                                                                          angle:90];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityGroupRole;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFPanelImpl

- (instancetype)initWithObject: (::mforms::Panel *)aPanel type: (mforms::PanelType)type {
  self = [super initWithFrame: NSMakeRect(0, 0, 100, 100)];
  if (self != nil) {
    NSRect frame;
    NSRect content = NSMakeRect(0, 0, 100, 100);

    mOwner = aPanel;
    mOwner->set_data(self);
    mType = type;
    switch (type) {
      case mforms::TransparentPanel: // just a container with no background
        [self setTransparent: YES];
        self.titlePosition = NSNoTitle;
        break;

      case mforms::FilledHeaderPanel:
      case mforms::FilledPanel: // just a container with color filled background
        [self setTransparent: NO];
        self.borderType = NSNoBorder;
        self.titlePosition = NSNoTitle;
        self.boxType = NSBoxCustom;
        break;

      case mforms::BorderedPanel: // container with native border
        self.borderType = NSBezelBorder;
        self.titlePosition = NSNoTitle;
        break;

      case mforms::LineBorderPanel: // container with a solid line border
        self.borderType = NSLineBorder;
        self.titlePosition = NSNoTitle;
        self.boxType = NSBoxCustom;
        break;

      case mforms::TitledBoxPanel: // native grouping box with a title with border
        self.borderType = NSBezelBorder;
        break;

      case mforms::TitledGroupPanel: // native grouping container with a title (may have no border)
        self.borderType = NSNoBorder;
        break;

      case mforms::StyledHeaderPanel:
        self.borderType = NSNoBorder;
        self.titlePosition = NSNoTitle;
        self.boxType = NSBoxCustom;
        [self setTransparent: NO];
        break;
    }

    self.contentViewMargins = NSMakeSize(0, 0);
    self.frameFromContentFrame = content;
    frame = self.frame;

    // Calculate the offsets the NSBox adds to the contentView.
    contentOffset.width = frame.size.width - content.size.width;
    contentOffset.height = frame.size.height - content.size.height;

    super.contentView = [[MFPanelContent alloc] initWithPanel: self];
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSRect)titleRect {
  NSRect rect;
  rect = super.titleRect;
  if (mCheckButton) {
    rect.origin.y -= 3;
    rect.size = mCheckButton.cellSize;
    rect.size.width += 4;
  }
  return rect;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseDown: (NSEvent *)event {
  if (mCheckButton) {
    [mCheckButton trackMouse:event inRect:self.titleRect ofView:self untilMouseUp:NO];
    [self setNeedsDisplay:YES];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseUp: (NSEvent *)event {
  if (mCheckButton) {
    [self setNeedsDisplay:YES];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (mforms::Object *)mformsObject {
  return mOwner;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)preferredSize: (NSSize)proposedSize {
  NSSize size = [self.contentView preferredSize: proposedSize];

  size.width += contentOffset.width;
  size.height += contentOffset.height;

  NSSize minSize = super.minimumSize;
  return { MAX(size.width, minSize.width), MAX(size.height, minSize.height) };
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)minimumSize {
  NSSize size = [self.contentView minimumSize];

  size.width += contentOffset.width;
  size.height += contentOffset.height;

  NSSize minSize = super.minimumSize;
  return { MAX(size.width, minSize.width), MAX(size.height, minSize.height) };
}

//----------------------------------------------------------------------------------------------------------------------

- (void)resizeSubviewsWithOldSize: (NSSize)oldBoundsSize {
  [super resizeSubviewsWithOldSize: oldBoundsSize];
  [self.contentView resizeSubviewsWithOldSize: oldBoundsSize];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setPaddingLeft: (float)lpad right: (float)rpad top: (float)tpad bottom: (float)bpad {
  [self.contentView setPaddingLeft: lpad right: rpad top: tpad bottom: bpad];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setContentView: (NSView *)content {
  if (content)
    [self.contentView addSubview:content];
  else
    [self.contentView.subviews.lastObject removeFromSuperview];
  [self relayout];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setTitle: (NSString *)title {
  if (mCheckButton)
    mCheckButton.title = title;
  else
    super.title = title;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setEnabled: (BOOL)flag {
  for (id view in self.contentView.subviews) {
    if ([view respondsToSelector: @selector(setEnabled:)])
      [view setEnabled:flag];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setBackgroundImage: (NSString *)path withAlignment: (mforms::Alignment)align {
  std::string full_path = mforms::App::get()->get_resource_path(path.UTF8String);
  if (!full_path.empty()) {
    [self.contentView setBackgroundImage:wrap_nsstring(full_path) withAlignment:align];
  } else {
    [self.contentView setBackgroundImage:nil withAlignment:align];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityGroupRole;
}

//----------------------------------------------------------------------------------------------------------------------

static bool panel_create(::mforms::Panel *self, ::mforms::PanelType type) {
  return [[MFPanelImpl alloc] initWithObject: self type:type] != nil;
}

//----------------------------------------------------------------------------------------------------------------------

static void panel_set_title(::mforms::Panel *self, const std::string &text) {
  if (self) {
    MFPanelImpl *panel = self->get_data();

    if (panel) {
      panel.title = wrap_nsstring(text);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void panel_set_back_color(mforms::Panel *self, const std::string &color) {
  if (self) {
    MFPanelImpl *panel = self->get_data();

    if (panel && panel->mType != mforms::StyledHeaderPanel) {
      [panel setTransparent:NO];
      panel.fillColor = [NSColor colorFromHexString: wrap_nsstring(color)];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void panel_set_active(mforms::Panel *self, bool active) {
  if (self) {
    MFPanelImpl *panel = self->get_data();

    if (panel) {
      panel->mCheckButton.state = active ? NSControlStateValueOn : NSControlStateValueOff;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static bool panel_get_active(mforms::Panel *self) {
  if (self) {
    MFPanelImpl *panel = self->get_data();

    if (panel) {
      return panel->mCheckButton.state == NSControlStateValueOn;
    }
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

static void panel_add(mforms::Panel *self, mforms::View *view) {
  if (self) {
    MFPanelImpl *panel = self->get_data();

    if (panel) {
      panel.contentView = view->get_data();
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void panel_remove(mforms::Panel *self, mforms::View *child) {
  if (self) {
    MFPanelImpl *panel = self->get_data();

    if (panel) {
      [panel setContentView:nil];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void cf_panel_init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_panel_impl.create = &panel_create;
  f->_panel_impl.set_title = &panel_set_title;
  f->_panel_impl.set_back_color = &panel_set_back_color;
  f->_panel_impl.set_title = &panel_set_title;

  f->_panel_impl.set_active = &panel_set_active;
  f->_panel_impl.get_active = &panel_get_active;

  f->_panel_impl.add = &panel_add;
  f->_panel_impl.remove = &panel_remove;
}

@end

//----------------------------------------------------------------------------------------------------------------------
