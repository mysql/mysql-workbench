/* 
 * Copyright (c) 2009, 2012, Oracle and/or its affiliates. All rights reserved.
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

#import "WbModelNoteEditor.h"
#import "MCPPUtilities.h"

@implementation NoteEditor

static void call_refresh(NoteEditor *self)
{
  [self refresh];
}


- (id)initWithModule:(grt::Module*)module GRTManager:(bec::GRTManager*)grtm arguments:(const grt::BaseListRef&)args
{
  self= [super initWithNibName: @"WbModelNoteEditor" bundle: [NSBundle bundleForClass:[self class]]];
  if (self != nil)
  {
    _grtm = grtm;
    // load GUI. Top level view in the nib is the NSTabView that will be docked to the main window
    [self loadView];

    // take the minimum size of the view from the initial size in the nib.
    // Therefore the nib should be designed as small as possible
    // note: the honouring of the min size is not yet implemented
    [self setMinimumSize: [tabView frame].size];
    
    [self reinitWithArguments: args];
  }
  return self;
}


- (void)reinitWithArguments:(const grt::BaseListRef&)args
{
  [super reinitWithArguments: args];
  delete mBackEnd;
  
    // setup the editor backend with the note object (args[0])
  mBackEnd= new NoteEditorBE(_grtm, workbench_model_NoteFigureRef::cast_from(args[0]));
    
  // register a callback that will make [self refresh] get called
  // whenever the backend thinks its needed to refresh the UI from the backend data (ie, the
  // edited object was changed from somewhere else in the application)
  mBackEnd->set_refresh_ui_slot(boost::bind(call_refresh, self));
  
  // update the UI
  [self refresh];
}


- (void) dealloc
{
  delete mBackEnd;
  [super dealloc];
}


/** Fetches object info from the backend and update the UI
 */
- (void)refresh
{
  if (mBackEnd && !mEditing)
  {
    [nameText setStringValue: [NSString stringWithCPPString:mBackEnd->get_name()]];
    
    [noteText setString: [NSString stringWithCPPString:mBackEnd->get_text()]];
  }
}


- (id)identifier
{
  // an identifier for this editor (just take the object id)
  return [NSString stringWithCPPString:mBackEnd->get_object().id()];
}


- (BOOL)matchesIdentifierForClosingEditor:(NSString*)identifier
{
  return mBackEnd->should_close_on_delete_of([identifier UTF8String]);
}


- (void)controlTextDidEndEditing:(NSNotification *)aNotification
{
  if ([aNotification object] == nameText)
  {
    mEditing = YES;
    // set name of the note
    mBackEnd->set_name([[nameText stringValue] UTF8String]);
    mEditing = NO;
  }  
}


- (void)textDidChange:(NSNotification *)aNotification
{
  if ([aNotification object] == noteText)
  {
    mEditing = YES;
    // set comment for the schema
    mBackEnd->set_text([[noteText string] UTF8String]);
    mEditing = NO;
  }
}

- (bec::BaseEditor*)editorBE
{
  return mBackEnd;
}

@end
