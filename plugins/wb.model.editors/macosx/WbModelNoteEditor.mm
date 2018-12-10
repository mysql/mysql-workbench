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
#include "wb_editor_note.h"

#import "WbModelNoteEditor.h"
#import "MCPPUtilities.h"

@interface NoteEditor()
{
  IBOutlet NSTabView *tabView;
  IBOutlet NSTextField *nameText;
  IBOutlet NSTextView *noteText;

  NoteEditorBE *mBackEnd;
  BOOL mEditing;
}

@end

@implementation NoteEditor

static void call_refresh(void *theEditor)
{
  NoteEditor *editor = (__bridge NoteEditor *)theEditor;
  [editor refresh];
}

- (instancetype)initWithModule: (grt::Module*)module
                     arguments: (const grt::BaseListRef &)args
{
 
  self = [super initWithNibName: @"WbModelNoteEditor" bundle: [NSBundle bundleForClass: [self class]]];
  if (self != nil)
  {
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

- (instancetype)initWithNibName: (NSString *)nibNameOrNil bundle: (NSBundle *)nibBundleOrNil
{
  return [self initWithModule: nil arguments: grt::BaseListRef()];
}

-(instancetype)initWithCoder: (NSCoder *)coder
{
  return [self initWithModule: nil arguments: grt::BaseListRef()];
}

- (void)reinitWithArguments:(const grt::BaseListRef&)args
{
  [super reinitWithArguments: args];
  delete mBackEnd;
  
    // setup the editor backend with the note object (args[0])
  mBackEnd= new NoteEditorBE(workbench_model_NoteFigureRef::cast_from(args[0]));
    
  // register a callback that will make [self refresh] get called
  // whenever the backend thinks its needed to refresh the UI from the backend data (ie, the
  // edited object was changed from somewhere else in the application)
  mBackEnd->set_refresh_ui_slot(std::bind(call_refresh, (__bridge void *)self));
  
  // update the UI
  [self refresh];
}


- (void) dealloc
{
  delete mBackEnd;
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


- (id)panelId
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
