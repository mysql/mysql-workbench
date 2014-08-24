/* 
 * Copyright (c) 2010, 2014, Oracle and/or its affiliates. All rights reserved.
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

#import "MFMForms.h"
#include "base/string_utilities.h"

#import "MFView.h"
#import "MFCodeEditor.h"
#import "NSString_extras.h"
#import "MVerticalLayoutView.h"

#import "SciLexer.h"
#import "InfoBar.h"

using namespace mforms;


// hack in find panel support to the code editor
@interface SCIContentView (mforms_extras)
- (void)performFindPanelAction:(id)sender;
@end

@implementation SCIContentView (mforms_extras)
- (void)performFindPanelAction:(id)sender
{
  if ([sender tag] == NSFindPanelActionShowFindPanel)
  {
    id parent = [[[self superview] superview] superview];
    if ([parent isKindOfClass: [MFCodeEditor class]])
    {
      [parent showFindPanel: NO];
    }
  }
}
@end


//--------------------------------------------------------------------------------------------------

@implementation MFCodeEditor

- (id) initWithFrame: (NSRect) frame
          codeEditor: (CodeEditor*) codeEditor
{
  self = [super initWithFrame: frame];
  if (self)
  {
    mOwner = codeEditor;
    mOwner->set_data(self);
  }
  return self;
}

// standard focus handling is not enough
//STANDARD_FOCUS_HANDLING(self) // Notify backend when getting first responder status.

- (BOOL)becomeFirstResponder
{
  mOwner->focus_changed();
  return [[self window] makeFirstResponder: [self content]];
}

// for MVerticalLayoutView
- (BOOL)expandsOnLayoutVertically:(BOOL)flag
{
  return YES;
}

- (NSMenu*) menuForEvent: (NSEvent*) theEvent
{
  mforms::Menu* menu = mOwner->get_context_menu();
  if (menu != NULL)
  {
    (*menu->signal_will_show())();
    return menu->get_data();
  }
  return nil;
}

//--------------------------------------------------------------------------------------------------

- (void)notification: (Scintilla::SCNotification*)notification
{
  [super notification: notification];
  if (!mOwner->is_destroying())
    mOwner->on_notify(notification);
}

//--------------------------------------------------------------------------------------------------

- (void)command: (int)code
{
  if (!mOwner->is_destroying())
    mOwner->on_command(code);
}

//--------------------------------------------------------------------------------------------------

- (void)showFindPanel: (bool)doReplace;
{
  if (!mOwner->is_destroying())
    mOwner->show_find_panel(doReplace);
}

//--------------------------------------------------------------------------------------------------

static bool ce_create(CodeEditor* self)
{
  [[[MFCodeEditor alloc] initWithFrame: NSMakeRect(0, 0, 100, 100)
                            codeEditor: self] autorelease];
  
  ScintillaView *editor = self->get_data();
  InfoBar* infoBar = [[[InfoBar alloc] initWithFrame: NSMakeRect(0, 0, 400, 0)] autorelease];
  [infoBar setDisplay: IBShowAll];
  [editor setInfoBar: infoBar top: NO];

  return true;
}

//--------------------------------------------------------------------------------------------------

static sptr_t ce_send_editor(CodeEditor* self, unsigned int message, uptr_t wParam, sptr_t lParam)
{
  ScintillaView *editor = self->get_data();
  if (editor != nil)
  {
    return [ScintillaView directCall: editor message: message wParam: wParam lParam: lParam];
  }
  return 0;
}

//--------------------------------------------------------------------------------------------------

static void ce_set_status_text(CodeEditor* self, const std::string& text)
{
  ScintillaView *editor = self->get_data();
  if (editor != nil)
  {
    [editor setStatusText: [NSString stringWithCPPString: text]];
  }
}

//--------------------------------------------------------------------------------------------------

void cf_codeeditor_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_code_editor_impl.create = ce_create;
  f->_code_editor_impl.send_editor = ce_send_editor;
  f->_code_editor_impl.set_status_text = ce_set_status_text;
}

@end
