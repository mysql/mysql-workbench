/*
 * Copyright (c) 2010, 2019, Oracle and/or its affiliates. All rights reserved.
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
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#import "MFMForms.h"
#include "base/string_utilities.h"
#include "KeyMap.h"

#import "MFView.h"
#import "MFCodeEditor.h"
#import "NSString_extras.h"
#import "MVerticalLayoutView.h"

#import "SciLexer.h"
#import "InfoBar.h"

using namespace mforms;

@implementation MFCodeEditor

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithFrame: (NSRect)frame codeEditor: (CodeEditor*)codeEditor {
  self = [super initWithFrame:frame];
  if (self) {
    mOwner = codeEditor;
    mOwner->set_data(self);

    // Change keyboard handling from extend rectangular selection with shift + alt + left/right arrows
    // to the standard handling on OSX (extend normal selection wordwise). Rectangular selection
    // is still available via alt + mouse.
    [MFCodeEditor directCall: self
                     message: SCI_ASSIGNCMDKEY
                      wParam: SCK_LEFT | SCI_ASHIFT << 16
                      lParam: SCI_WORDLEFTEXTEND];
    [MFCodeEditor directCall: self
                     message: SCI_ASSIGNCMDKEY
                      wParam: SCK_RIGHT | SCI_ASHIFT << 16
                      lParam: SCI_WORDRIGHTEXTEND];

    // Disable shift + alt + up/down arrows, because they would show weird behavior with the corrected selection
    // handling.
    [MFCodeEditor directCall: self message: SCI_CLEARCMDKEY wParam: SCK_UP | SCI_ASHIFT << 16 lParam: 0];
    [MFCodeEditor directCall: self message: SCI_CLEARCMDKEY wParam: SCK_DOWN | SCI_ASHIFT << 16 lParam: 0];

    // Finally change ctrl + left/right arrow to standard behavior (go to line start/end).
    // Atm SCI_META represents the control key and SCI_CTRL represents command, which is rather weird.
    [MFCodeEditor directCall: self message: SCI_ASSIGNCMDKEY wParam: SCK_LEFT | SCI_META << 16 lParam: SCI_VCHOME];
    [MFCodeEditor directCall: self message: SCI_ASSIGNCMDKEY wParam: SCK_RIGHT | SCI_META << 16 lParam: SCI_LINEEND];
    [MFCodeEditor directCall: self
                     message: SCI_ASSIGNCMDKEY
                      wParam: SCK_LEFT | (SCI_META | SCI_SHIFT) << 16
                      lParam: SCI_VCHOMEEXTEND];
    [MFCodeEditor directCall: self
                     message: SCI_ASSIGNCMDKEY
                      wParam: SCK_RIGHT | (SCI_META | SCI_SHIFT) << 16
                      lParam: SCI_LINEENDEXTEND];
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)destroy {
  mOwner = nullptr; // Keeps async processes (e.g. layout) from accessing an invalid owner.
}

//----------------------------------------------------------------------------------------------------------------------

// standard focus handling is not enough
// STANDARD_FOCUS_HANDLING(self) // Notify backend when getting first responder status.

- (BOOL)becomeFirstResponder {
  mOwner->focus_changed();
  return [self.window makeFirstResponder: [self content]];
}

//----------------------------------------------------------------------------------------------------------------------

// for MVerticalLayoutView
- (BOOL)expandsOnLayoutVertically: (BOOL)flag {
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSMenu*)menuForEvent: (NSEvent*)theEvent {
  mforms::Menu* menu = mOwner->get_context_menu();
  if (menu != NULL) {
    (*menu->signal_will_show())();
    return menu->get_data();
  }
  return nil;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)keyDown: (NSEvent*)event {
  // Since we cannot self get the first responder (our content view does) this event
  // is most likely a forwarding from our content view, so we can notify our backend.
  mforms::ModifierKey modifiers = [self modifiersFromEvent: event];

  NSString* input = event.characters;
  mforms::KeyCode code = mforms::KeyChar;
  switch ([input characterAtIndex:0]) {
    case NSDownArrowFunctionKey:
      code = mforms::KeyDown;
      break;

    case NSUpArrowFunctionKey:
      code = mforms::KeyUp;
      break;

    case '\n':
    case '\r':
      code = mforms::KeyReturn;
      break;
  }

  mOwner->key_event(code, modifiers, input.UTF8String);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)notification: (SCNotification*)notification {
  [super notification:notification];
  if (mOwner != NULL && !mOwner->is_destroying())
    mOwner->on_notify(notification);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)command: (int)code {
  if (mOwner != NULL && !mOwner->is_destroying())
    mOwner->on_command(code);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)showFindPanel: (bool)doReplace;
{
  if (!mOwner->is_destroying())
    mOwner->show_find_panel(doReplace);
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityTextAreaRole;
}

//----------------------------------------------------------------------------------------------------------------------

static bool ce_create(CodeEditor* self, bool showInfo) {
  MFCodeEditor* editor = [[MFCodeEditor alloc] initWithFrame: NSMakeRect(0, 0, 100, 100) codeEditor: self];
  if (showInfo) {
    InfoBar* infoBar = [[InfoBar alloc] initWithFrame: NSMakeRect(0, 0, 400, 0)];
    [infoBar setDisplay:IBShowAll];
    [editor setInfoBar:infoBar top:NO];
  }

  return true;
}

//--------------------------------------------------------------------------------------------------

static sptr_t ce_send_editor(CodeEditor* self, unsigned int message, uptr_t wParam, sptr_t lParam) {
  ScintillaView* editor = self->get_data();
  if (editor != nil) {
    return [ScintillaView directCall: editor message: message wParam: wParam lParam: lParam];
  }
  return 0;
}

//--------------------------------------------------------------------------------------------------

static void ce_set_status_text(CodeEditor* self, const std::string& text) {
  ScintillaView* editor = self->get_data();
  if (editor != nil) {
    [editor setStatusText: [NSString stringWithCPPString: text]];
  }
}

//--------------------------------------------------------------------------------------------------

void cf_codeeditor_init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_code_editor_impl.create = ce_create;
  f->_code_editor_impl.send_editor = ce_send_editor;
  f->_code_editor_impl.set_status_text = ce_set_status_text;
}

@end

