/* 
 * Copyright (c) 2009, 2016, Oracle and/or its affiliates. All rights reserved.
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

#import "MFTextBox.h"
#import "MFMForms.h"

using namespace mforms;

//--------------------------------------------------------------------------------------------------

@implementation InternalTextView
  
- (instancetype) initWithFrame: (NSRect)frame
             backend: (mforms::TextBox*)backend
{
  self = [super initWithFrame: frame];
  if (self)
  {
    mOwner = backend;
    self.richText = NO;
  }
  return self;
}

STANDARD_FOCUS_HANDLING(self) // Notify backend when getting first responder status.

//--------------------------------------------------------------------------------------------------

- (void) flagsChanged: (NSEvent*) event
{
  mforms::ModifierKey modifiers = [self modifiersFromEvent: event];

  if (mOwner->key_event(mforms::KeyModifierOnly, modifiers, ""))
    [super flagsChanged: event];
}

//--------------------------------------------------------------------------------------------------

- (void) keyDown: (NSEvent*) event
{
  mforms::ModifierKey modifiers = [self modifiersFromEvent: event];
  
  NSString* input = event.characters;
  
  if (mOwner->key_event(mforms::KeyChar, modifiers, input.UTF8String))
    [super keyDown: event];
}

@end

//--------------------------------------------------------------------------------------------------

@implementation MFTextBoxImpl

- (instancetype)initWithObject:(::mforms::TextBox*)aTextBox
           scrollers:(mforms::ScrollBars)scrolls
{
  NSSize size = self.minimumSize;
  NSRect frame = NSMakeRect(0, 0, size.width, size.height);
  self = [super initWithFrame:frame];
  if (self)
  {
    mOwner= aTextBox;
    mOwner->set_data(self);

    self.borderType = NSBezelBorder;
    self.hasVerticalScroller = scrolls & mforms::VerticalScrollBar;
    self.hasHorizontalScroller = scrolls & mforms::HorizontalScrollBar;
    [self setAutohidesScrollers: YES];
    if (scrolls & mforms::SmallScrollBars)
    {
      self.verticalScroller.controlSize = NSSmallControlSize;
      self.horizontalScroller.controlSize = NSSmallControlSize;
    }

    frame.size = self.minimumSize;
    mContentView = [[InternalTextView alloc] initWithFrame: frame backend: mOwner];
    self.documentView = mContentView;

    mContentView.textContainer.containerSize = NSMakeSize(1000000000, 1000000000);
    [mContentView setHorizontallyResizable: YES];
    [mContentView setVerticallyResizable: YES];

    if (scrolls & mforms::VerticalScrollBar)
    {
      [mContentView.textContainer setHeightTracksTextView: NO];
    }
    else
      [mContentView.textContainer setHeightTracksTextView: YES];

    if (scrolls & mforms::HorizontalScrollBar)
      [mContentView.textContainer setWidthTracksTextView: NO];
    else
      [mContentView.textContainer setWidthTracksTextView: YES];
    
    mContentView.delegate = self;
    [mContentView setEditable: YES];
  }
  return self;
}

- (mforms::Object*)mformsObject
{
  return mOwner;
}

- (NSSize)preferredSize: (NSSize)proposal
{
  NSSize minSize = self.minimumSize;
  NSSize contentSize = mContentView.minSize; // Careful, not the minimumSize!
  return { MAX(minSize.width, contentSize.width), MAX(minSize.height, contentSize.height) };
}

- (NSSize)minimumSize
{
  NSSize size;
  size.width = [NSScroller scrollerWidthForControlSize: NSRegularControlSize
                                        scrollerStyle: NSScrollerStyleOverlay] + 50 + mPadding * 2;
  size.height = [NSScroller scrollerWidthForControlSize: NSRegularControlSize
                                         scrollerStyle: NSScrollerStyleOverlay] + 50 + mPadding * 2;
  NSSize minSize = super.minimumSize;
  return { MAX(size.width, minSize.width), MAX(size.height, minSize.height) };
}


- (void)textDidChange:(NSNotification*)notification
{
  mOwner->callback();
}


- (void)setFrame:(NSRect)rect
{
  rect.origin.x+= mPadding;
  rect.origin.y+= mPadding;
  rect.size.width-= mPadding*2;
  rect.size.height-= mPadding*2;
  super.frame = rect;
}

- (void)setPadding:(float)pad
{
  mPadding= pad;
}


- (void)setBordered:(BOOL)flag
{
  self.borderType = flag ? NSBezelBorder : NSNoBorder;
}

- (void)setEnabled:(BOOL)flag
{
  mContentView.editable = flag ? YES : NO;
  mContentView.selectable = flag ? YES : NO;
}

- (void) setReadOnly: (BOOL) flag
{
  mContentView.editable = flag ? NO : YES;
  [mContentView setSelectable: YES];  
}

- (void) setTextColor: (NSColor *) aColor
{
  mContentView.textColor = aColor;
}

- (void) setBackgroundColor: (NSColor *) aColor
{
  mContentView.drawsBackground = (aColor == nil) ? NO : YES;
  mContentView.backgroundColor = aColor;
}

static bool textbox_create(::mforms::TextBox *self, mforms::ScrollBars scrolls)
{
  MFTextBoxImpl *textbox = [[MFTextBoxImpl alloc] initWithObject : self
                                                       scrollers : scrolls];
  
  return textbox != nil;
}


static void textbox_set_text(::mforms::TextBox *self, const std::string &text)
{
  if ( self )
  {
    MFTextBoxImpl* textbox = self->get_data();
    
    NSRange range;
    range = NSMakeRange (0, textbox->mContentView.string.length);
    [textbox->mContentView replaceCharactersInRange: range withString: wrap_nsstring(text)];
  }
}

#ifdef not_supported_yet
static void textbox_append_text_with_attributes(mforms::TextBox *self, const std::string &text, const mforms::TextAttributes &attr, bool scroll_to_end)
{
  if (self)
  {
    MFTextBoxImpl* textbox = self->get_data();
    NSAttributedString *attributedString;
    NSFont *font = [textbox->mContentView font];
    int traits = 0;
    if (attr.bold)
      traits |= NSBoldFontMask;
    if (attr.italic)
      traits |= NSItalicFontMask;
    if (traits)
      font = [[NSFontManager sharedFontManager] convertFont: [font copy]
                                                toHaveTrait: traits];
  
    if (attr.color.is_valid())
      attributedString = [[NSAttributedString alloc] initWithString: wrap_nsstring(text)
                  attributes: [NSDictionary dictionaryWithObjectsAndKeys:
                                    font, NSFontAttributeName,
                                    [NSColor colorWithCalibratedRed: attr.color.red green: attr.color.green blue: attr.color.blue alpha: 1.0], NSForegroundColorAttributeName,
                                    nil]];
    else
      attributedString = [[NSAttributedString alloc] initWithString: wrap_nsstring(text)
                   attributes: [NSDictionary dictionaryWithObjectsAndKeys:
                                font, NSFontAttributeName,
                                [NSColor blackColor], NSForegroundColorAttributeName,
                                nil]];

    [[textbox->mContentView textStorage] insertAttributedString: attributedString
                                                        atIndex: [[textbox->mContentView string] length]];
    
    if (scroll_to_end)
    {
      NSRange range;
      range = NSMakeRange ([[textbox->mContentView string] length], 0);
      [textbox->mContentView scrollRangeToVisible: range];
    }
  }
}


static void textbox_append_text(mforms::TextBox *self, const std::string &text, bool scroll_to_end)
{
  mforms::TextAttributes attr;
  attr.bold = false;
  attr.italic = false;
  attr.color = base::Color();
  textbox_append_text_with_attributes(self, text, attr, scroll_to_end);
}
#endif

static void textbox_append_text(mforms::TextBox *self, const std::string &text, bool scroll_to_end)
{
  if ( self )
  {
    MFTextBoxImpl* textbox = self->get_data();
    
    NSRange range;
    range = NSMakeRange (textbox->mContentView.string.length, 0);
    [textbox->mContentView replaceCharactersInRange: range withString: wrap_nsstring(text)];
    
    if (scroll_to_end)
    {
      NSRange range;
      range = NSMakeRange (textbox->mContentView.string.length, 0);
      [textbox->mContentView scrollRangeToVisible: range];
    }
  }
}

static std::string textbox_get_text(::mforms::TextBox *self)
{
  if ( self )
  {
    MFTextBoxImpl* textbox = self->get_data();
    NSString *str = textbox->mContentView.string;
    
    return std::string(str.UTF8String); // can't use [str length] for string size, because that's the number of chars, not bytes for UTF8
  }
  return "";
}


static void textbox_set_padding(::mforms::TextBox *self, int pad)
{
  if ( self )
  {
    MFTextBoxImpl* textbox = self->get_data();
    
    [textbox setPadding:pad];
  }
}


static void textbox_set_bordered(::mforms::TextBox *self, bool flag)
{
  if ( self )
  {
    MFTextBoxImpl* textbox = self->get_data();
    
    [textbox setBordered: flag];
  }
}

static void textbox_get_selected_range(::mforms::TextBox *self, int &start, int &end)
{
  if ( self )
  {
    MFTextBoxImpl* textbox = self->get_data();
    
    NSRange range = [textbox->mContentView selectedRange];
    
    start = (int)range.location;
    end = int(start + range.length);
  }
}

static void textbox_set_read_only(mforms::TextBox *self, bool flag)
{
  if (self)
    [self->get_data() setReadOnly: flag];
}

static void textbox_set_monospaced(mforms::TextBox *self, bool flag)
{
  if (self)
  {
    MFTextBoxImpl* textbox = self->get_data();
    NSFont *font= [NSFont fontWithName: @"AndaleMono"
                                  size:[NSFont smallSystemFontSize]];
    if (font)
      textbox->mContentView.font = font;
    else
      NSLog(@"Couldn't find font AndaleMono");
  }
}

static void textbox_clear(mforms::TextBox *self)
{
  if ( self )
  {
    MFTextBoxImpl* textbox = self->get_data();
    
    textbox->mContentView.string = @"";
  }  
}

void cf_textbox_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_textbox_impl.create= &textbox_create;
  f->_textbox_impl.set_text= &textbox_set_text;
  f->_textbox_impl.get_text= &textbox_get_text;
  f->_textbox_impl.set_padding= &textbox_set_padding;
  f->_textbox_impl.set_bordered= &textbox_set_bordered;
  f->_textbox_impl.set_read_only= &textbox_set_read_only;
  f->_textbox_impl.append_text= &textbox_append_text;
  //  f->_textbox_impl.append_text_with_attributes= &textbox_append_text_with_attributes;
  f->_textbox_impl.set_monospaced= &textbox_set_monospaced;
  f->_textbox_impl.get_selected_range= &textbox_get_selected_range;
  f->_textbox_impl.clear= &textbox_clear;
}

@end



