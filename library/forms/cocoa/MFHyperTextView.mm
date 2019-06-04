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

#import "MFHyperTextView.h"
#import "MFMForms.h"
#import "MFBase.h"
#import "MFView.h"
#import "NSColor_extras.h"

@implementation MFHyperTextView

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithObject:(mforms::HyperText *)ht {
  self = [super initWithFrame:NSMakeRect(0, 0, 50, 50)];
  if (self) {
    mOwner = ht;
    mOwner->set_data(self);

    NSRect rect = self.frame;
    mTextView = [[NSTextView alloc] initWithFrame:rect];

    self.verticalScroller.controlSize = NSControlSizeSmall;
    [self setHasVerticalScroller:YES];
    [self setAutohidesScrollers:YES];
    self.documentView = mTextView;

    mTextView.minSize = NSMakeSize(0.0, NSHeight(self.frame));
    mTextView.maxSize = NSMakeSize(FLT_MAX, FLT_MAX);
    [mTextView setVerticallyResizable:YES];
    [mTextView setHorizontallyResizable:NO];
    mTextView.autoresizingMask = NSViewWidthSizable;

    mTextView.textContainer.containerSize = NSMakeSize(NSWidth(self.frame), FLT_MAX);
    [mTextView.textContainer setWidthTracksTextView:YES];

    mTextView.font = [NSFont systemFontOfSize:[NSFont systemFontSize]];
    [mTextView setRichText:YES];

    [mTextView setEditable:NO];

    mTextView.delegate = self;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

STANDARD_FOCUS_HANDLING(self) // Notify backend when getting first responder status.

//----------------------------------------------------------------------------------------------------------------------

- (void)setBackgroundColor:(NSColor *)color {
  super.backgroundColor = color;
  mTextView.backgroundColor = color;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)textView:(NSTextView *)aTextView clickedOnLink:(id)link atIndex:(NSUInteger)charIndex {
  mOwner->handle_url_click([link absoluteString].UTF8String);
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityTextAreaRole;
}

//----------------------------------------------------------------------------------------------------------------------

static bool ht_create(mforms::HyperText *ht) {
  return [[MFHyperTextView alloc] initWithObject:ht] != nil;
}

//----------------------------------------------------------------------------------------------------------------------

static void ht_set_markup(mforms::HyperText *ht, const std::string &text) {
  MFHyperTextView *htv = ht->get_data();
  if (text.empty()) {
    htv->mTextView.string = @"";
    return;
  }

  NSData *html = [NSData dataWithBytes:text.data() length: text.size()];
  NSDictionary *options = @{
    NSTextEncodingNameDocumentOption: @"UTF-8",
    NSBaseURLDocumentOption : [NSURL URLWithString: @""]
  };
  [htv->mTextView.textStorage
    replaceCharactersInRange: NSMakeRange(0, htv->mTextView.textStorage.length)
        withAttributedString: [[NSAttributedString alloc] initWithHTML: html options: options documentAttributes: nil]
  ];
}

//----------------------------------------------------------------------------------------------------------------------

void cf_hypertext_init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_hypertext_impl.create = &ht_create;
  f->_hypertext_impl.set_markup_text = &ht_set_markup;
}

@end
