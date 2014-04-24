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

#include "base/geometry.h"
#include "base/string_utilities.h"

#import "WbModelStoredNoteEditor.h"
#import "MCPPUtilities.h"
#import "MVerticalLayoutView.h"

@implementation StoredNoteEditor

//--------------------------------------------------------------------------------------------------

- (id)initWithModule:(grt::Module*)module GRTManager:(bec::GRTManager*)grtm arguments:(const grt::BaseListRef&)args
{
  self= [super initWithNibName: @"WbModelStoredNoteEditor" bundle: [NSBundle bundleForClass:[self class]]];
  if (self != nil)
  {
    _grtm = grtm;
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

//--------------------------------------------------------------------------------------------------

- (void)reinitWithArguments: (const grt::BaseListRef&)args
{
  [super reinitWithArguments: args];
  delete mBackEnd;
  
  // Setup the editor backend with the note object (args[0]).
  GrtStoredNoteRef note = GrtStoredNoteRef::cast_from(args[0]);
  mBackEnd = new StoredNoteEditorBE(_grtm, note);
  [self setupEditorOnHost: editorHost];

  mBackEnd->load_text();
  [mApplyButton setEnabled: NO];
  [mRevertButton setEnabled: NO];
}

//--------------------------------------------------------------------------------------------------

- (void) dealloc
{
  delete mBackEnd;
  [super dealloc];
}

//--------------------------------------------------------------------------------------------------

- (id)identifier
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
    int ret = NSRunAlertPanel(@"Close Editor", @"There are unsaved changes in the editor %s. "
                              "If you do not save, these changes will be discarded.",
                              @"Apply Changes", @"Discard", @"Cancel", mBackEnd->get_name().c_str());
    if (ret == NSAlertDefaultReturn)
    {
      mBackEnd->commit_changes();
    }
    else if (ret == NSAlertAlternateReturn)
    {
      // Nothing to do here.
    }
    else
      return NO;
  }
  
  return YES;
}

//--------------------------------------------------------------------------------------------------

- (bec::BaseEditor*)editorBE
{
  return mBackEnd;
}

@end
