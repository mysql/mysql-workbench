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

#include "wb_context.h"
#import "MySQLRoutineGroupEditor.h"
#import "MCPPUtilities.h"
#import "MVerticalLayoutView.h"
#include "grtdb/db_object_helpers.h"

#include "ScintillaView.h"
#include "mysql_routinegroup_editor.h"

@interface DbMysqlRoutineGroupEditor()
{
  IBOutlet NSTabView *tabView;

  IBOutlet NSTextField *nameText;
  IBOutlet NSTableView *routineTable;
  IBOutlet NSTextView *commentText;

  IBOutlet MVerticalLayoutView *editorHost;

  NSMutableArray *mRoutineArray;

  MySQLRoutineGroupEditorBE *mBackEnd;
}

@end

@implementation DbMysqlRoutineGroupEditor

static void call_refresh(void *theEditor)
{
  DbMysqlRoutineGroupEditor *editor = (__bridge DbMysqlRoutineGroupEditor *)theEditor;
  [editor performSelectorOnMainThread: @selector(refresh) withObject: nil waitUntilDone: YES];
}

- (instancetype)initWithModule: (grt::Module*)module
                     arguments: (const grt::BaseListRef &)args
{
  self = [super initWithNibName: @"MySQLRoutineGroupEditor" bundle: [NSBundle bundleForClass:[self class]]];
  if (self != nil)
  {
    // load GUI. Top level view in the nib is the NSTabView that will be docked to the main window
    [self loadView];

    [routineTable setTarget: self];
    [routineTable setDoubleAction: @selector(doubleClickRoutine:)];

    [routineTable registerForDraggedTypes: @[[NSString stringWithCPPString: WB_DBOBJECT_DRAG_TYPE]]];
    
    // take the minimum size of the view from the initial size in the nib.
    // Therefore the nib should be designed as small as possible
    // note: the honouring of the min size is not yet implemented
    [self setMinimumSize: [[tabView superview] frame].size];
    
    [self reinitWithArguments: args];
  }
  return self;
}
    

- (void)reinitWithArguments:(const grt::BaseListRef&)args
{
  delete mBackEnd;
  mBackEnd = new MySQLRoutineGroupEditorBE(db_mysql_RoutineGroupRef::cast_from(args[0]));
  
  // register a callback that will make [self refresh] get called
  // whenever the backend thinks its needed to refresh the UI from the backend data (ie, the
  // edited object was changed from somewhere else in the application)
  mBackEnd->set_refresh_ui_slot(std::bind(call_refresh, (__bridge void *)self));
    
  mRoutineArray = [NSMutableArray array];
    
  [self setupEditorOnHost: editorHost];

  // Update the UI, load the sql etc.
  [self refresh];
}


- (void) dealloc
{
  delete mBackEnd;
}

- (void)refresh
{
  if (mBackEnd)
  {
    [nameText setStringValue: [NSString stringWithCPPString: mBackEnd->get_name()]];
    [self updateTitle: [self title]];
    
    [commentText setString: [NSString stringWithCPPString: mBackEnd->get_comment()]];
    
    [mRoutineArray removeAllObjects];
    
    std::vector<std::string> names(mBackEnd->get_routines_names());
    for (std::vector<std::string>::const_iterator iter= names.begin(); iter != names.end(); ++iter)
    {
      [mRoutineArray addObject: [NSString stringWithCPPString: *iter]];
    }
    [routineTable reloadData];

    mBackEnd->load_routines_sql();
  }
}


- (id)panelId
{
  // an identifier for this editor (just take the object id)
  return [NSString stringWithCPPString: mBackEnd->get_object().id()];
}


- (IBAction)doubleClickRoutine:(id)sender
{
  NSInteger row = [routineTable selectedRow];
  if (row >= 0)
    mBackEnd->open_editor_for_routine_at_index(row);
}


- (NSDragOperation)tableView:(NSTableView *)aTableView 
                validateDrop:(id < NSDraggingInfo >)info
                 proposedRow:(NSInteger)row
       proposedDropOperation:(NSTableViewDropOperation)operation
{
  id data = [[info draggingPasteboard] stringForType: [NSString stringWithCPPString: WB_DBOBJECT_DRAG_TYPE]];
  if (data)
  {
    std::list<db_DatabaseObjectRef> objects;
    std::string text= [data UTF8String];
    
    objects= bec::CatalogHelper::dragdata_to_dbobject_list(mBackEnd->get_catalog(), text);

    for (std::list<db_DatabaseObjectRef>::const_iterator obj= objects.begin(); 
         obj != objects.end(); ++obj)
    {
      if (!obj->is_instance<db_mysql_Routine>())
        return NSDragOperationNone;
    }
    if (!objects.empty())
    {
      [aTableView setDropRow:-1 dropOperation: NSTableViewDropOn];
      return NSDragOperationCopy;
    }
  }
  return NSDragOperationNone;
}


- (BOOL)tableView:(NSTableView *)aTableView 
       acceptDrop:(id < NSDraggingInfo >)info
              row:(NSInteger)row
    dropOperation:(NSTableViewDropOperation)operation
{
  id data= [[info draggingPasteboard] stringForType: [NSString stringWithCPPString: WB_DBOBJECT_DRAG_TYPE]];
  if (data)
  {
    std::list<db_DatabaseObjectRef> objects;
    std::string text= [data UTF8String];
    
    objects= bec::CatalogHelper::dragdata_to_dbobject_list(mBackEnd->get_catalog(), text);
    
    for (std::list<db_DatabaseObjectRef>::const_iterator obj= objects.begin(); 
         obj != objects.end(); ++obj)
    {
      if (!obj->is_instance<db_mysql_Routine>())
        return NSDragOperationNone;

      db_mysql_RoutineRef routine = db_mysql_RoutineRef::cast_from(*obj);
      if (routine.is_valid())
      {
        mBackEnd->append_routine_with_id(routine.id());
        mBackEnd->load_routines_sql();
      }
    }
    if (!objects.empty())
    {
      mBackEnd->get_sql_editor()->set_refresh_enabled(true);
      [self refresh];
      return YES;
    }
  }    
  return NO;
}


- (BOOL)matchesIdentifierForClosingEditor:(NSString*)identifier
{
  return mBackEnd->should_close_on_delete_of([identifier UTF8String]);
}


- (void)controlTextDidEndEditing:(NSNotification *)aNotification
{
  if ([aNotification object] == nameText)
  {
    // set name of the schema
    mBackEnd->set_name([[nameText stringValue] UTF8String]);
    [self updateTitle: [self title]];
  }
}


- (void) textDidEndEditing:(NSNotification *)aNotification
{
  if ([[aNotification object] isKindOfClass: [ScintillaView class]])
  {    
    mBackEnd->commit_changes();
  }
  else if ([aNotification object] == commentText)
  {
    [[aNotification object] breakUndoCoalescing];
    mBackEnd->set_comment([[commentText string] UTF8String]);
  }
}


- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
  return [mRoutineArray count];
}


- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
  return mRoutineArray[rowIndex];
}

//--------------------------------------------------------------------------------------------------

/**
 * Called when clicking the [-] button.
 */
- (IBAction) removeItem: (id) sender
{
  NSIndexSet* selectedRows = [routineTable selectedRowIndexes];
  NSUInteger index = [selectedRows lastIndex];
  while (index != NSNotFound)
  {
    mBackEnd->remove_routine_by_index(index);
    index = [selectedRows indexLessThanIndex: index];
  }
  mBackEnd->get_sql_editor()->set_refresh_enabled(true);
  [self refresh];
}

- (bec::BaseEditor*)editorBE
{
  return mBackEnd;
}

@end
