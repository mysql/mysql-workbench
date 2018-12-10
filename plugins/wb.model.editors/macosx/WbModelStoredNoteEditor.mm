/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/geometry.h"
#include "base/string_utilities.h"
#include "wb_editor_storednote.h"

#import "WbModelStoredNoteEditor.h"
#import "MCPPUtilities.h"
#import "MVerticalLayoutView.h"

@interface StoredNoteEditor()
{
  IBOutlet NSTabView *tabView;
  IBOutlet NSView* editorHost;

  StoredNoteEditorBE *mBackEnd;
}

@end

@implementation StoredNoteEditor

//--------------------------------------------------------------------------------------------------

- (instancetype)initWithModule: (grt::Module*)module
                     arguments: (const grt::BaseListRef &)args
{
 
  self = [super initWithNibName: @"WbModelStoredNoteEditor" bundle: [NSBundle bundleForClass:[self class]]];
  if (self != nil)
  {
    // load GUI. Top level view in the nib is the NSTabView that will be docked to the main window
    [self loadView];

    // take the minimum size of the view from the initial size in the nib.
    // Therefore the nib should be designed as small as possible
    // note: the honouring of the min size is not yet implemented
    [self setMinimumSize: [[tabView superview] frame].size];
    
    [self reinitWithArguments: args];
  }
  return self;
}

- (instancetype)initWithNibName: (NSString *)nibNameOrNil bundle: (NSBundle *)nibBundleOrNil
{
  return [self initWithModule: nil arguments: grt::BaseListRef()];
}

-(instancetype)initWithCoder: (NSCoder *)coder
{
  return [self initWithModule: nil arguments: grt::BaseListRef()];
}

- (void)reinitWithArguments: (const grt::BaseListRef&)args
{
  [super reinitWithArguments: args];
  delete mBackEnd;
  
  // Setup the editor backend with the note object (args[0]).
  GrtStoredNoteRef note = GrtStoredNoteRef::cast_from(args[0]);
  mBackEnd = new StoredNoteEditorBE(note);
  [self setupEditorOnHost: editorHost];

  mBackEnd->load_text();
  [mApplyButton setEnabled: NO];
  [mRevertButton setEnabled: NO];
}

//--------------------------------------------------------------------------------------------------

- (void) dealloc
{
  delete mBackEnd;
}

//--------------------------------------------------------------------------------------------------

- (id)panelId
{
  // an identifier for this editor (just take the object id)
  return [NSString stringWithCPPString:mBackEnd->get_object().id()];
}

//--------------------------------------------------------------------------------------------------

- (BOOL)matchesIdentifierForClosingEditor:(NSString*)identifier
{
  return mBackEnd->should_close_on_delete_of([identifier UTF8String]);
}

//--------------------------------------------------------------------------------------------------

- (IBAction)applyChanges:(id)sender
{
  mBackEnd->commit_changes();
  [mApplyButton setEnabled: NO];
  [mRevertButton setEnabled: NO];
  [self updateTitle: [self title]];
}

//--------------------------------------------------------------------------------------------------

- (IBAction)revertChanges:(id)sender
{
  mBackEnd->load_text();
  [mApplyButton setEnabled: NO];
  [mRevertButton setEnabled: NO];
  [self updateTitle: [self title]];
}

//--------------------------------------------------------------------------------------------------

- (void)textDidEndEditing: (NSNotification *) aNotification
{
  // Nothing to do here as we don't auto commit, but we need the function because editors are
  // registered for it in the base class.
}

//--------------------------------------------------------------------------------------------------

- (BOOL)pluginWillClose: (id)sender
{
  if (![super pluginWillClose: sender])
  {
    NSAlert *alert = [NSAlert new];
    alert.messageText =@"Close Editor";
    alert.informativeText = [NSString stringWithFormat: @"There are unsaved changes in the editor %s. "
                             "If you do not save, these changes will be discarded.", mBackEnd->get_name().c_str()];
    alert.alertStyle = NSAlertStyleWarning;
    [alert addButtonWithTitle: @"Apply Changes"];
    [alert addButtonWithTitle: @"Discard Changes"];
    [alert addButtonWithTitle: @"Cancel"];
    switch ([alert runModal])
    {
      case NSAlertFirstButtonReturn:
        mBackEnd->commit_changes();
        break;
      case NSAlertThirdButtonReturn:
        return NO;
    }
  }
  
  return YES;
}

//--------------------------------------------------------------------------------------------------

- (bec::BaseEditor*)editorBE
{
  return mBackEnd;
}

@end
