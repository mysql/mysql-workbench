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
#include "mysql_routine_editor.h"

#import "MySQLRoutineEditor.h"
#import "MCPPUtilities.h"
#import "MVerticalLayoutView.h"

#include "ScintillaView.h"

@interface DbMysqlRoutineEditor ()
{
  IBOutlet NSTabView *tabView;
  IBOutlet NSTextField *nameText;
  IBOutlet NSTextView *commentText;
  IBOutlet NSView *editorHost;

  MySQLRoutineEditorBE *mBackEnd;
}

@end

@implementation DbMysqlRoutineEditor

static void call_refresh(void *theEditor)
{
  DbMysqlRoutineEditor *editor = (__bridge DbMysqlRoutineEditor *)theEditor;
  [editor performSelectorOnMainThread: @selector(refresh) withObject: nil waitUntilDone: YES];
}


- (instancetype)initWithModule: (grt::Module*)module
                     arguments: (const grt::BaseListRef &)args
{
  self= [super initWithNibName: @"MySQLRoutineEditor"  bundle: [NSBundle bundleForClass:[self class]]];
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

- (void)reinitWithArguments: (const grt::BaseListRef&)args
{
  [super reinitWithArguments: args];
  
  delete mBackEnd;
  
    // setup the editor backend with the schema object (args[0])
  mBackEnd = new MySQLRoutineEditorBE(db_mysql_RoutineRef::cast_from(args[0]));
  
  // register a callback that will make [self refresh] get called
  // whenever the backend thinks its needed to refresh the UI from the backend data (ie, the
  // edited object was changed from somewhere else in the application)
  mBackEnd->set_refresh_ui_slot(std::bind(call_refresh, (__bridge void *)self));
  
  [self setupEditorOnHost: editorHost];
  mBackEnd->load_routine_sql();

  if (mBackEnd->is_editing_live_object())
  {
    NSInteger i = [tabView indexOfTabViewItemWithIdentifier: @"comments"];
    if (i > 0)
      [tabView removeTabViewItem: [tabView tabViewItemAtIndex: i]];
    commentText = nil;
  }
  // update the UI
  [self refresh];
  mBackEnd->reset_editor_undo_stack();
}


- (void) dealloc
{
  delete mBackEnd;
}


- (BOOL)matchesIdentifierForClosingEditor:(NSString*)identifier
{
  return mBackEnd->should_close_on_delete_of([identifier UTF8String]);
}

- (void) refresh
{
  if (mBackEnd)
  {
    [nameText setStringValue: [NSString stringWithCPPString: mBackEnd->get_name()]];
    [self updateTitle: [self title]];

    if (!mBackEnd->is_editing_live_object())
      [commentText setString: [NSString stringWithCPPString: mBackEnd->get_comment()]];

    mBackEnd->load_routine_sql();
  }
}

- (id)panelId
{
  // an identifier for this editor (just take the object id)
  return [NSString stringWithCPPString:mBackEnd->get_object().id()];
}

//--------------------------------------------------------------------------------------------------

- (void) textDidEndEditing: (NSNotification *) aNotification
{
  if ([aNotification object] == commentText)
  {
    [[aNotification object] breakUndoCoalescing];
    // Set comment for the view.
    mBackEnd->set_comment([[commentText string] UTF8String]);
  }
}

//--------------------------------------------------------------------------------------------------

- (bec::BaseEditor*)editorBE
{
  return mBackEnd;
}

@end
