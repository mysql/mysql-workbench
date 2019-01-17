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

#import "MFLabel.h"
#import "MFMForms.h"
#import "NSColor_extras.h"

//----------------------------------------------------------------------------------------------------------------------

@implementation MFLabelImpl

- (instancetype)initWithObject: (::mforms::Label *)aLabel {
  self = [super initWithFrame: NSMakeRect(10, 10, 10, 20)];
  if (self) {
    [self setDrawsBackground: NO];
    [self setBezeled: NO];
    [self setEditable: NO];

    [self.cell setSelectable: YES];
    [self.cell setWraps: NO];

    mOwner = aLabel;
    mOwner->set_data(self);
    mStyle = mforms::NormalStyle;

    mAlignment = mforms::MiddleLeft;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ '%@'>", self.className, self.stringValue];
}

//----------------------------------------------------------------------------------------------------------------------

- (mforms::Object *)mformsObject {
  return mOwner;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setFrame: (NSRect)frame {
  // do vertical alignment of the textfield here
  NSSize size = [self.cell cellSizeForBounds: frame];

  switch (mAlignment) {
    case mforms::NoAlign:

    case mforms::TopLeft:
    case mforms::TopCenter:
    case mforms::TopRight:
      break;
    case mforms::BottomLeft:
    case mforms::BottomRight:
    case mforms::BottomCenter:
      frame.origin.y += (NSHeight(frame) - size.height);
      break;
    case mforms::MiddleCenter:
    case mforms::MiddleLeft:
    case mforms::MiddleRight:
      frame.origin.y += (NSHeight(frame) - size.height) / 2;
      break;
  }
  super.frame = frame;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setEnabled: (BOOL)flag {
  if (!flag)
    self.textColor = [NSColor darkGrayColor];
  else {
    self.textColor = [NSColor textColor];
  }
  super.enabled = flag;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)preferredSize: (NSSize)proposal {
  NSSize size;

  // Preferred size computation for a label is not as easy as it sounds because
  // the available APIs behave quite unexpected (e.g. the newer sizeThatFits method returns
  // the proposal if it is smaller than what it needs, which seems weird).
  if (self.cell.wraps)
    size = [self.cell cellSizeForBounds: {{0, 0}, {proposal.width, CGFLOAT_MAX}}];
  else
    size = [self.cell cellSizeForBounds: {{0, 0}, {CGFLOAT_MAX, CGFLOAT_MAX}}];

  NSSize minSize = self.minimumSize;
  return { ceil(MAX(size.width, minSize.width)), ceil(MAX(size.height, minSize.height)) };
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setStringValue: (NSString *)text {
  super.stringValue = text;
  [self sizeToFit];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setLabelStyle: (mforms::LabelStyle)style {
  self.textColor = [NSColor textColor];
  switch (style) {
    case mforms::NormalStyle:
      self.font = [NSFont systemFontOfSize: [NSFont systemFontSize]];
      break;
    case mforms::BoldStyle:
      self.font = [NSFont boldSystemFontOfSize: [NSFont systemFontSize] - 1];
      break;
    case mforms::SmallBoldStyle:
      self.font = [NSFont boldSystemFontOfSize: [NSFont smallSystemFontSize]];
      break;
    case mforms::BigStyle:
      self.font = [NSFont systemFontOfSize: 15];
      break;
    case mforms::BigBoldStyle:
      self.font = [NSFont boldSystemFontOfSize: 15];
      break;
    case mforms::SmallStyle:
      self.font = [NSFont systemFontOfSize: 10];
      break;
    case mforms::VerySmallStyle:
      self.font = [NSFont systemFontOfSize: 8];
      break;
    case mforms::InfoCaptionStyle:
      self.font = [NSFont systemFontOfSize: [NSFont smallSystemFontSize]];
      break;
    case mforms::BoldInfoCaptionStyle:
      self.font = [NSFont boldSystemFontOfSize: [NSFont smallSystemFontSize]];
      break;
    case mforms::WizardHeadingStyle:
      self.font = [NSFont boldSystemFontOfSize: 13];
      break;
    case mforms::SmallHelpTextStyle:
      self.font = [NSFont systemFontOfSize: [NSFont labelFontSize]];
      break;
    case mforms::VeryBigStyle:
      self.font = [NSFont systemFontOfSize: 18];
      break;
  }
  mStyle = style;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityStaticTextRole;
}

//----------------------------------------------------------------------------------------------------------------------

static bool label_create(::mforms::Label *self) {
  MFLabelImpl *label = [[MFLabelImpl alloc] initWithObject: self];

  return label != nil;
}

//----------------------------------------------------------------------------------------------------------------------

static void label_set_text(::mforms::Label *self, const std::string &text) {
  if (self) {
    MFLabelImpl *label = self->get_data();

    label.stringValue = wrap_nsstring(text);
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void label_set_wrap_text(::mforms::Label *self, bool flag) {
  if (self) {
    MFLabelImpl *label = self->get_data();

    label.cell.wraps = flag;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void label_set_text_align(::mforms::Label *self, ::mforms::Alignment alignment) {
  if (self) {
    MFLabelImpl *label = self->get_data();

    switch (alignment) {
      case mforms::NoAlign:

      case mforms::BottomLeft:
      case mforms::MiddleLeft:
      case mforms::TopLeft:
        label.alignment = NSTextAlignmentLeft;
        break;
      case mforms::BottomCenter:
      case mforms::TopCenter:
      case mforms::MiddleCenter:
        label.alignment = NSTextAlignmentCenter;
        break;
      case mforms::BottomRight:
      case mforms::MiddleRight:
      case mforms::TopRight:
        label.alignment = NSTextAlignmentRight;
        break;
    }
    label->mAlignment = alignment;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void label_set_style(mforms::Label *self, mforms::LabelStyle style) {
  if (self) {
    MFLabelImpl *label = self->get_data();

    [label setLabelStyle: style];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void label_set_color(mforms::Label *self, const std::string &color) {
  if (self) {
    MFLabelImpl *label = self->get_data();

    label.textColor = [NSColor colorFromHexString: @(color.c_str())];
  }
}

//----------------------------------------------------------------------------------------------------------------------

void cf_label_init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_label_impl.create = &label_create;
  f->_label_impl.set_text = &label_set_text;
  f->_label_impl.set_text_align = &label_set_text_align;
  f->_label_impl.set_wrap_text = &label_set_wrap_text;
  f->_label_impl.set_style = &label_set_style;
  f->_label_impl.set_color = &label_set_color;
}

@end

//----------------------------------------------------------------------------------------------------------------------
