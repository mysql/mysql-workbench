/* 
 * Copyright (c) 2011, 2016, Oracle and/or its affiliates. All rights reserved.
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

#import <WebKit/WebKit.h>
#import "MFHyperTextView.h"
#import "MFMForms.h"
#import "MFBase.h"
#import "MFView.h"
#import "NSColor_extras.h"

@implementation MFHyperTextView

- (instancetype)initWithObject: (mforms::HyperText*)ht
{
  self = [super initWithFrame: NSMakeRect(0, 0, 50, 50)];
  if (self)
  {
    mOwner = ht;
    mOwner->set_data(self);

    NSRect rect = self.frame;
    mTextView = [[NSTextView alloc] initWithFrame: rect];
    
    self.verticalScroller.controlSize = NSSmallControlSize;
    [self setHasVerticalScroller: YES];
    [self setAutohidesScrollers: YES];
    self.documentView = mTextView;

    mTextView.minSize = NSMakeSize(0.0, NSHeight(self.frame));
    mTextView.maxSize = NSMakeSize(FLT_MAX, FLT_MAX);
    [mTextView setVerticallyResizable: YES];
    [mTextView setHorizontallyResizable: NO];
    mTextView.autoresizingMask = NSViewWidthSizable;
    
    mTextView.textContainer.containerSize = NSMakeSize(NSWidth(self.frame), FLT_MAX);
    [mTextView.textContainer setWidthTracksTextView: YES];
    
    mTextView.font = [NSFont systemFontOfSize: [NSFont systemFontSize]]; 
    [mTextView setRichText: YES];

    [mTextView setEditable: NO];
    
    mTextView.delegate = self;
  }
  return self;
}


STANDARD_FOCUS_HANDLING(self) // Notify backend when getting first responder status.

- (void)setBackgroundColor:(NSColor *)color
{
  super.backgroundColor = color;
  mTextView.backgroundColor = color;
}

- (BOOL)textView:(NSTextView *)aTextView clickedOnLink:(id)link atIndex:(NSUInteger)charIndex
{
  mOwner->handle_url_click([link absoluteString].UTF8String);
  return YES;
}


static bool ht_create(mforms::HyperText *ht)
{
  return [[MFHyperTextView alloc] initWithObject: ht] != nil;
}


static void ht_set_markup(mforms::HyperText *ht, const std::string &text)
{
  MFHyperTextView *htv = ht->get_data();
  WebPreferences *defaults = [WebPreferences standardPreferences];
  
  defaults.standardFontFamily = @"Lucida Grande";
  defaults.defaultFontSize = [NSFont smallSystemFontSize];
  defaults.defaultFixedFontSize = [NSFont smallSystemFontSize];
  [defaults setUserStyleSheetEnabled: YES];
  defaults.userStyleSheetLocation = [NSURL fileURLWithPath: [[NSBundle mainBundle] pathForResource: @"hypertextview"
                                                                                               ofType: @"css"]];

  [htv->mTextView.textStorage replaceCharactersInRange: NSMakeRange(0, htv->mTextView.textStorage.length)
                                    withAttributedString: [[NSAttributedString alloc] initWithHTML: [NSData dataWithBytes: text.data()
                                                                                                                   length: text.size()]
                                                                                           options: @{NSWebPreferencesDocumentOption: defaults,
                                                                                                      NSTextEncodingNameDocumentOption: @"UTF-8",
                                                                                                      NSBaseURLDocumentOption: [NSURL URLWithString: @""]}
                                                                                documentAttributes: nil]];

}


void cf_hypertext_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_hypertext_impl.create= &ht_create;
  f->_hypertext_impl.set_markup_text= &ht_set_markup;
}

@end
