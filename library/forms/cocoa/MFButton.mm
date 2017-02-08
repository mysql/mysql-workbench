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

#import "MFButton.h"
#import "MFMForms.h"

@implementation MFButtonImpl

- (instancetype)initWithObject: (mforms::Button *)aButton buttonType: (mforms::ButtonType)type {
  if (aButton == nil)
    return nil;

  self = [super initWithFrame: NSMakeRect(10, 10, 30, 18)];
  if (self) {
    mOwner = aButton;
    mOwner->set_data(self);

    self.title = @"";
    switch (type) {
      case ::mforms::AdminActionButton:
      case ::mforms::PushButton:
        // buttons have some extra padding to the sides that we want to skip
        mTopLeftOffset = NSMakePoint(6, 2);
        mBottomRightOffset = NSMakePoint(5, 5);

        self.bezelStyle = NSRoundedBezelStyle;
        break;
      case ::mforms::ToolButton:
        mTopLeftOffset = NSZeroPoint;
        mBottomRightOffset = NSZeroPoint;

        self.imagePosition = NSImageOnly;
        [self setBordered:NO];
        break;
      case ::mforms::SmallButton:
        // buttons have some extra padding to the sides that we want to skip
        self.cell.font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
        self.bezelStyle = NSRoundRectBezelStyle;
        break;
    }
    self.target = self;
    self.action = @selector(performCallback:);
  }
  return self;
}

- (instancetype)initWithFrame: (NSRect)frame {
  return [self initWithObject: nil buttonType: mforms::PushButton];
}

- (instancetype)initWithCoder: (NSCoder *)coder {
  return [self initWithObject: nil buttonType: mforms::PushButton];
}

- (NSString *)description {
  return [NSString stringWithFormat: @"<%@ '%@'>", self.className, self.title];
}

- (mforms::Object *)mformsObject {
  return mOwner;
}

- (void)performCallback: (id)sender {
  mOwner->callback();
}

- (NSSize)minimumSize {
  NSSize size;
  if ([self respondsToSelector: @selector(sizeThatFits:)]) {
    size = [self sizeThatFits: NSZeroSize];
    size.height += 2;
  } else {
    size = self.cell.cellSize;
  }

  if (self.imagePosition == NSImageOnly) {
    size.width += 6;
    size.height += 6;
  }

  // add some internal padding to the button to make it look nicer
  if (mAddPadding)
    size.width += size.height;

  NSSize minSize = super.minimumSize;
  return {MAX(size.width, minSize.width), MAX(size.height, minSize.height)};
}

- (NSSize)preferredSize: (NSSize)proposal {
  return self.minimumSize;
}

static bool button_create(::mforms::Button *self, ::mforms::ButtonType type) {
  return [[MFButtonImpl alloc] initWithObject: self buttonType:type] != nil;
}

static void button_set_icon(::mforms::Button *self, const std::string &icon) {
  if (self) {
    MFButtonImpl *button = self->get_data();

    if (button) {
      std::string full_path = mforms::App::get()->get_resource_path(icon);
      NSImage *image = [[NSImage alloc] initWithContentsOfFile: wrap_nsstring(full_path)];
      button.image = image;

      NSSize size = [button sizeThatFits: NSZeroSize];
      NSRect frame = button.frame;
      if (size.width > frame.size.width) {
        frame.size.width = size.width;
        if (button->mAddPadding)
          frame.size.width += frame.size.height;
        button.frame = frame;

        self->relayout();
      }
    }
  }
}

static void button_set_text(::mforms::Button *self, const std::string &text) {
  if (self) {
    MFButtonImpl *button = self->get_data();

    if (button) {
      button.title =
        [wrap_nsstring(text) stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString: @"_"]];

      NSSize size = [button sizeThatFits: NSZeroSize];
      NSRect frame = button.frame;
      if (size.width > frame.size.width) {
        frame.size.width = size.width;
        if (button->mAddPadding)
          frame.size.width += frame.size.height;
        button.frame = frame;

        self->relayout();
      }
    }
  }
}

static void button_enable_internal_padding(::mforms::Button *self, bool pad) {
  if (self) {
    MFButtonImpl *button = self->get_data();

    if (button) {
      button->mAddPadding = pad;
    }
  }
}

void cf_button_init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_button_impl.create = &button_create;
  f->_button_impl.set_text = &button_set_text;
  f->_button_impl.set_icon = &button_set_icon;
  f->_button_impl.enable_internal_padding = &button_enable_internal_padding;
}

@end
