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
#import "MFTextEntry.h"
#import "MFMForms.h"
#include "mforms/textentry.h"

//----------------------------------------------------------------------------------------------------------------------

/**
 * A special formatter to implement text length limits.
 */
@interface LimitedTextFieldFormatter : NSFormatter {
  int maxLength;
}
@property  int maximumLength;

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation LimitedTextFieldFormatter

- (instancetype) init
{
  self = [super init];
  maxLength = INT_MAX;
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void) setMaximumLength: (int) len
{
  maxLength = len;
}

//----------------------------------------------------------------------------------------------------------------------

- (int) maximumLength
{
  return maxLength;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *) stringForObjectValue: (id) object
{
  return (NSString *) object;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL) getObjectValue: (id *) object forString: (NSString *) string errorDescription: (NSString **) error
{
  *object = string;
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL) isPartialStringValid: (NSString *) partialString newEditingString: (NSString **) newString
             errorDescription: (NSString **) error
{
  if ((int) partialString.length > maxLength)
  {
    *newString = nil;
    return NO;
  }
  
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAttributedString *) attributedStringForObjectValue: (id) anObject
                                  withDefaultAttributes: (NSDictionary *) attributes
{
  return nil;
}

@end

//----------------------------------------------------------------------------------------------------------------------

// In order to avoid duplicate code (as all these functions must be in all text entry controls)
// this part is defined using a macro. However this is not well readable nor debuggable, so we might
// have to decompose this later.
#define STANDARD_TEXT_ENTRY_HANDLING \
- (mforms::Object*)mformsObject { return mOwner; } \
\
- (NSSize)minimumSize \
{ \
  NSTextField *field = mOwner->get_data(); \
  NSSize contentSize = field.cell.cellSize; \
  NSSize minSize = super.minimumSize; \
  return { minSize.width, MAX(contentSize.height, minSize.height) }; \
} \
\
- (NSSize)preferredSize: (NSSize)proposal \
{ \
  NSSize size = self.minimumSize; \
  if (self.cell.wraps) \
  { \
    NSRect frame; \
    frame.size = proposal; \
    size = [self.cell cellSizeForBounds: frame]; \
  } \
 \
  return { ceil(size.width), ceil(size.height) }; \
} \
\
- (void)controlTextDidChange:(NSNotification *)aNotification { mOwner->callback(); } \
\
-(BOOL)textView: (NSTextView *)aTextView doCommandBySelector: (SEL)aSelector {\
  struct { \
    SEL selector; \
    mforms::TextEntryAction action; \
  } events[] = { \
    { @selector(insertNewline:), mforms::EntryActivate }, \
    { @selector(moveUp:), mforms::EntryKeyUp }, \
    { @selector(moveDown:), mforms::EntryKeyDown }, \
    { @selector(moveToBeginningOfDocument:), mforms::EntryCKeyUp }, \
    { @selector(moveToEndOfDocument:), mforms::EntryCKeyDown }, \
    { @selector(cancelOperation:), mforms::EntryEscape }, \
    { 0 } \
  }; \
  \
  for (int i = 0; events[i].selector != 0; i++) {\
    if (aSelector == events[i].selector) \
    { \
      mOwner->action(events[i].action); \
      break;  /* let it fall-through so that keys like Return generate normal stuff like commit changes */ \
    } \
  } \
  return NO; \
}

//----------------------------------------------------------------------------------------------------------------------

@interface SecureTextField : NSSecureTextField  <NSTextFieldDelegate> {
  mforms::TextEntry *mOwner;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation SecureTextField

- (instancetype)initWithObject: (mforms::TextEntry*)owner type: (mforms::TextEntryType)type
{
  self = [super initWithFrame: NSMakeRect(0, 0, 30, 0)];
  if (self)
  {
    mOwner = owner;
    mOwner->set_data(self);

    [self sizeToFit];
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

STANDARD_FOCUS_HANDLING(self) // Notify backend when getting first responder status.
STANDARD_TEXT_ENTRY_HANDLING

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityTextFieldRole;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilitySubrole {
  return NSAccessibilitySecureTextFieldSubrole;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)accessibilityValue {
  std::string value = mOwner->get_string_value();
  return [NSString stringWithUTF8String: value.c_str()];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setAccessibilityValue: (NSString*)value {
  mOwner->set_value(value.UTF8String);
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface SearchTextField : NSSearchField  <NSSearchFieldDelegate> {
  mforms::TextEntry *mOwner;
  float mMinHeight;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation SearchTextField

- (instancetype)initWithObject: (mforms::TextEntry*)owner type: (mforms::TextEntryType)type
{
  self = [super initWithFrame: NSMakeRect(0, 0, 30, 0)];
  if (self)
  {
    mOwner = owner;
    mOwner->set_data(self);

    [self sizeToFit];
    mMinHeight = NSHeight(self.frame);

    if (type == mforms::SmallSearchEntry)
    {
      self.cell.controlSize = NSControlSizeSmall;
      self.font = [NSFont systemFontOfSize: [NSFont systemFontSizeForControlSize: NSControlSizeSmall]];
    }
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

STANDARD_FOCUS_HANDLING(self) // Notify backend when getting first responder status.
STANDARD_TEXT_ENTRY_HANDLING

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityTextFieldRole;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)accessibilityValue {
    std::string value = mOwner->get_string_value();
    return [NSString stringWithUTF8String: value.c_str()];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setAccessibilityValue: (NSString*)value {
    mOwner->set_value(value.UTF8String);
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface StandardTextField : NSTextField  <NSTextFieldDelegate> {
  mforms::TextEntry *mOwner;
  float mMinHeight;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation StandardTextField

- (instancetype)initWithObject: (mforms::TextEntry*)owner type: (mforms::TextEntryType)type
{
  self = [super initWithFrame: NSMakeRect(0, 0, 30, 0)];
  if (self)
  {
    mOwner = owner;
    mOwner->set_data(self);

    [self sizeToFit];
    mMinHeight = NSHeight(self.frame);
  }
  return self;
}

STANDARD_FOCUS_HANDLING(self) // Notify backend when getting first responder status.
STANDARD_TEXT_ENTRY_HANDLING

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityTextFieldRole;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)accessibilityValue {
    std::string value = mOwner->get_string_value();
    return [NSString stringWithUTF8String: value.c_str()];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setAccessibilityValue: (NSString*)value {
    mOwner->set_value(value.UTF8String);
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface MFTextEntryImpl ()
{
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFTextEntryImpl

@end

//----------------------------------------------------------------------------------------------------------------------

static bool entry_create(mforms::TextEntry *self, mforms::TextEntryType type)
{
  NSTextField<NSTextFieldDelegate> *field;
  switch (type)
  {
    case mforms::PasswordEntry:
      field = [[SecureTextField alloc] initWithObject: self type: type];
      break;
    case mforms::SearchEntry:
    case mforms::SmallSearchEntry:
      field = [[SearchTextField alloc] initWithObject: self type: type];
      break;
    default: // mforms::NormalEntry
      field = [[StandardTextField alloc] initWithObject: self type: type];
      break;
  }

  field.delegate = field;

  (field.cell).lineBreakMode = NSLineBreakByClipping;
  [field.cell setScrollable: YES];
  field.selectable = YES;

  LimitedTextFieldFormatter* formatter = [LimitedTextFieldFormatter new];
  (field.cell).formatter = formatter;

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

static void entry_set_text(mforms::TextEntry *self, const std::string &text)
{
  NSTextField* entry = self->get_data();
  entry.stringValue = wrap_nsstring(text);
}

//----------------------------------------------------------------------------------------------------------------------

static void entry_set_placeholder_text(mforms::TextEntry *self, const std::string &text)
{
  NSTextField* entry = self->get_data();
  [entry.cell setPlaceholderString: wrap_nsstring(text)];
}

//----------------------------------------------------------------------------------------------------------------------

static void entry_set_max_length(mforms::TextEntry *self, int maxlen)
{
  NSTextField* entry = self->get_data();
  [[entry.cell formatter] setMaximumLength: maxlen];
}

//----------------------------------------------------------------------------------------------------------------------

static std::string entry_get_text(mforms::TextEntry *self)
{
  NSTextField* entry = self->get_data();

  if (entry)
    return entry.stringValue.UTF8String;

  return "";
}

//----------------------------------------------------------------------------------------------------------------------

static void entry_set_read_only(mforms::TextEntry *self, bool flag)
{
  NSTextField* entry = self->get_data();
  entry.editable = flag ? NO : YES;
}

//----------------------------------------------------------------------------------------------------------------------

static void entry_set_placeholder_color(mforms::TextEntry *self, const std::string &color)
{
  // Not needed. The search cell automatically selects a good text color.
}

//----------------------------------------------------------------------------------------------------------------------

static void entry_set_bordered(mforms::TextEntry *self, bool flag)
{
  // Ignored on macOS.
}

//----------------------------------------------------------------------------------------------------------------------

static void entry_cut(mforms::TextEntry *self)
{
  NSTextField* entry = self->get_data();
  NSText *editor = [entry.window fieldEditor: NO forObject: entry];
  [editor cut: nil];
}

//----------------------------------------------------------------------------------------------------------------------

static void entry_copy(mforms::TextEntry *self)
{
  NSTextField* entry = self->get_data();
  NSText *editor = [entry.window fieldEditor: NO forObject: entry];
  [editor copy: nil];
}

//----------------------------------------------------------------------------------------------------------------------

static void entry_paste(mforms::TextEntry *self)
{
  NSTextField* entry = self->get_data();
  NSText *editor = [entry.window fieldEditor: NO forObject: entry];
  [editor paste: nil];
}

//----------------------------------------------------------------------------------------------------------------------

static void entry_select(mforms::TextEntry *self, const base::Range &range)
{
  NSTextField* entry = self->get_data();
  NSText *editor = [entry.window fieldEditor: YES forObject: entry];
  editor.selectedRange = NSMakeRange(range.position, range.size);
}

//----------------------------------------------------------------------------------------------------------------------

static base::Range entry_get_selection(mforms::TextEntry *self)
{
  NSTextField* entry = self->get_data();
  NSText *editor = [entry.window fieldEditor: NO forObject: entry];
  NSRange r = editor.selectedRange;
  return base::Range(r.location, r.length);
}

//----------------------------------------------------------------------------------------------------------------------

void cf_textentry_init()
{
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();
  
  f->_textentry_impl.create = &entry_create;
  f->_textentry_impl.set_text = &entry_set_text;
  f->_textentry_impl.get_text = &entry_get_text;
  f->_textentry_impl.set_max_length = &entry_set_max_length;
  f->_textentry_impl.set_read_only = &entry_set_read_only;
  f->_textentry_impl.set_placeholder_text = &entry_set_placeholder_text;
  f->_textentry_impl.set_placeholder_color = &entry_set_placeholder_color;
  f->_textentry_impl.set_bordered = &entry_set_bordered;
  f->_textentry_impl.cut = &entry_cut;
  f->_textentry_impl.copy = &entry_copy;
  f->_textentry_impl.paste = &entry_paste;
  f->_textentry_impl.select = &entry_select;
  f->_textentry_impl.get_selection = &entry_get_selection;
}

//----------------------------------------------------------------------------------------------------------------------
