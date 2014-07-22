/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

#import "MFTextEntry.h"
#import "MFMForms.h"

@implementation LimitedTextFieldFormatter

- init
{
  self = [super init];
  maxLength = INT_MAX;
  return self;
}

- (void) setMaximumLength: (int) len
{
  maxLength = len;
}

- (int) maximumLength
{
  return maxLength;
}

- (NSString *) stringForObjectValue: (id) object
{
  return (NSString *) object;
}

- (BOOL) getObjectValue: (id *) object forString: (NSString *) string errorDescription: (NSString **) error
{
  *object = string;
  return YES;
}

- (BOOL) isPartialStringValid: (NSString *) partialString newEditingString: (NSString **) newString
             errorDescription: (NSString **) error
{
  if ((int) [partialString length] > maxLength)
  {
    *newString = nil;
    return NO;
  }
  
  return YES;
}

- (NSAttributedString *) attributedStringForObjectValue: (id) anObject
                                  withDefaultAttributes: (NSDictionary *) attributes
{
  return nil;
}

@end

//--------------------------------------------------------------------------------------------------

@interface NSSearchFieldCell(Workbench)

@end

@implementation NSSearchFieldCell(Workbench)

- (void)drawInteriorWithFrame: (NSRect)cellFrame inView: (NSView *)controlView
{
  // We cannot set drawsBackground to true on a search field cell (it's ignored and reset to false).
  // So instead we check if the normal text color is set and don't draw the background in that case
  // is this drawing overwrites the border drawn before we came here.
  if (self.backgroundColor != nil && self.backgroundColor != [NSColor textBackgroundColor])
  {
    [self.backgroundColor setFill];
    double radius = MIN(NSWidth(cellFrame), NSHeight(cellFrame)) / 2.0;
    [[NSBezierPath bezierPathWithRoundedRect: cellFrame
                                     xRadius: radius
                                     yRadius: radius]
     fill];
  }

  [super drawInteriorWithFrame: cellFrame inView: controlView];
  [[self searchButtonCell] setTransparent: NO];
  [[self searchButtonCell] drawInteriorWithFrame: [self searchButtonRectForBounds:cellFrame] inView: controlView];
  if (self.stringValue.length > 0)
    [[self cancelButtonCell] drawInteriorWithFrame: [self cancelButtonRectForBounds:cellFrame] inView: controlView];
}

@end

//--------------------------------------------------------------------------------------------------

@implementation MFTextEntryImpl

- (id)initWithObject:(mforms::TextEntry*)aEntry type: (mforms::TextEntryType)type
{
  self = [super initWithFrame: NSMakeRect(0, 0, 30, 0)];
  if (self)
  {
    if (type == mforms::PasswordEntry)
    {
      NSSecureTextField *secureField= [[NSSecureTextField alloc] initWithFrame: NSMakeRect(0, 0, 30, 0)];
      [self setCell: [secureField cell]];
      [secureField release];
    }
    else if (type == mforms::SearchEntry || type == mforms::SmallSearchEntry)
    {
      NSSearchField *searchField = [[NSSearchField alloc] initWithFrame: NSMakeRect(0, 0, 30, 0)];
      self.cell = searchField.cell;

      if (type == mforms::SmallSearchEntry)
      {
        [[self cell] setControlSize: NSSmallControlSize];
        [self setFont: [NSFont systemFontOfSize: [NSFont systemFontSizeForControlSize: NSSmallControlSize]]];
      }
      [searchField release];
    }
    mOwner= aEntry;
    mOwner->set_data(self);

    [self sizeToFit];
    mMinHeight= NSHeight([self frame]);
    
    [self setDelegate: self];
    
    [[self cell] setLineBreakMode: NSLineBreakByClipping];
    [[self cell] setScrollable: YES];
    [self setSelectable: YES];
    
    LimitedTextFieldFormatter* formatter = [[LimitedTextFieldFormatter alloc] init];
    [[self cell] setFormatter: formatter];
    [formatter release];
  }
  return self;
}

- (mforms::Object*)mformsObject
{
  return mOwner;
}


- (NSSize)minimumSize
{
  NSSize size= [[self cell] cellSize];
  
  size.height= mMinHeight;
  size.width= size.height;
  
  return size;
}

- (BOOL)heightIsFixed
{
  return YES;
}

- (void)setFixedFrameSize:(NSSize)size
{
  mFixedSize = size;
  [super setFixedFrameSize: size];
}

- (NSSize)fixedFrameSize
{
  return mFixedSize;
}

- (void)controlTextDidChange:(NSNotification *)aNotification
{
  mOwner->callback();
}


-(BOOL)textView:(NSTextView *)aTextView doCommandBySelector: (SEL)aSelector
{
  struct {
    SEL selector;
    mforms::TextEntryAction action;
  } events[] = {
    { @selector(insertNewline:), mforms::EntryActivate },
    { @selector(moveUp:), mforms::EntryKeyUp }, 
    { @selector(moveDown:), mforms::EntryKeyDown },
    { @selector(moveToBeginningOfDocument:), mforms::EntryCKeyUp },
    { @selector(moveToEndOfDocument:), mforms::EntryCKeyDown },
    { 0 }
  };
  
  for (int i = 0; events[i].selector != 0; i++)
  {
    if (aSelector == events[i].selector)
    {
      mOwner->action(events[i].action);
      break; // let it fall-through so that keys like Return generate normal stuff like commit changes
    }
  }
  return NO;
}

- (void)setBackgroundColor: (NSColor *)color
{
  [super setBackgroundColor: color];
  [self.cell setBackgroundColor: color];
}

- (void)setDrawsBackground: (BOOL)flag
{
  [super setDrawsBackground: flag];
  [self.cell setDrawsBackground: flag];
}

- (BOOL)becomeFirstResponder
{
  mOwner->focus_changed();
  return [super becomeFirstResponder];
}

@end


static bool entry_create(mforms::TextEntry *self, mforms::TextEntryType type)
{
  MFTextEntryImpl *entry= [[[MFTextEntryImpl alloc] initWithObject:self type:type] autorelease];
    
  return entry != nil;
}


static void entry_set_text(mforms::TextEntry *self, const std::string &text)
{
  MFTextEntryImpl* entry = self->get_data();
  [entry setStringValue:wrap_nsstring(text)];
}

static void entry_set_placeholder_text(mforms::TextEntry *self, const std::string &text)
{
  MFTextEntryImpl* entry = self->get_data();
  [[entry cell] setPlaceholderString: wrap_nsstring(text)];
}

static void entry_set_max_length(mforms::TextEntry *self, int maxlen)
{
  MFTextEntryImpl* entry = self->get_data();
  [[[entry cell] formatter] setMaximumLength: maxlen];
}


static std::string entry_get_text(mforms::TextEntry *self)
{
  MFTextEntryImpl* entry = self->get_data();

  if (entry)
    return [[entry stringValue] UTF8String];

  return "";
}

static void entry_set_read_only(mforms::TextEntry *self, bool flag)
{
  MFTextEntryImpl* entry = self->get_data();
  [entry setEditable: flag ? NO : YES];
}

static void entry_set_placeholder_color(mforms::TextEntry *self, const std::string &color)
{
  // Not needed. The search cell automatically selects a good text color.
}

static void entry_set_bordered(mforms::TextEntry *self, bool flag)
{
  // Ignored on OS X.
}


static void entry_cut(mforms::TextEntry *self)
{
  MFTextEntryImpl* entry = self->get_data();
  NSText *editor = [[entry window] fieldEditor:NO forObject:entry];
  [editor cut: nil];
}

static void entry_copy(mforms::TextEntry *self)
{
  MFTextEntryImpl* entry = self->get_data();
  NSText *editor = [[entry window] fieldEditor:NO forObject:entry];
  [editor copy: nil];
}

static void entry_paste(mforms::TextEntry *self)
{
  MFTextEntryImpl* entry = self->get_data();
  NSText *editor = [[entry window] fieldEditor:NO forObject:entry];
  [editor paste: nil];
}

static void entry_select(mforms::TextEntry *self, const base::Range &range)
{
  MFTextEntryImpl* entry = self->get_data();
  NSText *editor = [[entry window] fieldEditor:YES forObject:entry];
  [editor setSelectedRange: NSMakeRange(range.position, range.size)];
}

static base::Range entry_get_selection(mforms::TextEntry *self)
{
  MFTextEntryImpl* entry = self->get_data();
  NSText *editor = [[entry window] fieldEditor:NO forObject:entry];
  NSRange r = [editor selectedRange];
  return base::Range(r.location, r.length);
}


void cf_textentry_init()
{
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();
  
  f->_textentry_impl.create= &entry_create;
  f->_textentry_impl.set_text= &entry_set_text;
  f->_textentry_impl.get_text= &entry_get_text;
  f->_textentry_impl.set_max_length= &entry_set_max_length;
  f->_textentry_impl.set_read_only= &entry_set_read_only;
  f->_textentry_impl.set_placeholder_text= &entry_set_placeholder_text;
  f->_textentry_impl.set_placeholder_color = &entry_set_placeholder_color;
  f->_textentry_impl.set_bordered = &entry_set_bordered;
  f->_textentry_impl.cut = &entry_cut;
  f->_textentry_impl.copy = &entry_copy;
  f->_textentry_impl.paste = &entry_paste;
  f->_textentry_impl.select = &entry_select;
  f->_textentry_impl.get_selection = &entry_get_selection;
}



