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
#include "grtdb/editor_user_role.h"
#include "grtdb/db_object_helpers.h" // get_rdbms_for_db_object()

#import "RoleEditor.h"
#import "MCPPUtilities.h"
#import "GRTTreeDataSource.h"
#import "GRTListDataSource.h"

// Subclass of List data source to handle object drops
@interface RolePrivilegeObjectListDataSource : GRTListDataSource
{
  DbMysqlRoleEditor *mOwner;
  bec::RoleEditorBE *mBackEnd;
}

- (void)setRoleEditor:(DbMysqlRoleEditor*)owner;
- (void)setBackEnd:(bec::RoleEditorBE*)be;

@end


@implementation RolePrivilegeObjectListDataSource

- (void)setRoleEditor:(DbMysqlRoleEditor*)owner
{
  mOwner= owner;
}


- (void)setBackEnd:(bec::RoleEditorBE*)be
{
  mBackEnd= be;
}


- (void)tableViewSelectionDidChange:(NSNotification *)aNotification
{
  bec::NodeId node;
  NSInteger row= [[aNotification object] selectedRow];
  
  if (row >= 0)
    node.append(row);
  
  mBackEnd->get_object_list()->set_selected_node(node);
  
  [mOwner refresh];
}


- (NSDragOperation)tableView:(NSTableView*)tv
                validateDrop:(id <NSDraggingInfo>)info
                 proposedRow:(NSInteger)row
       proposedDropOperation:(NSTableViewDropOperation)op
{
  if ([[[info draggingPasteboard] types] containsObject: [NSString stringWithCPPString: WB_DBOBJECT_DRAG_TYPE]])
    return NSDragOperationGeneric;
  
  return NSDragOperationNone;
}


- (BOOL)tableView:(NSTableView *)aTableView acceptDrop:(id <NSDraggingInfo>)info
              row:(NSInteger)row dropOperation:(NSTableViewDropOperation)operation
{  
  NSPasteboard *pboard= [info draggingPasteboard];
  NSString *data= [pboard stringForType: [NSString stringWithCPPString: WB_DBOBJECT_DRAG_TYPE]];
  return mBackEnd->add_dropped_objectdata([data UTF8String]);
}

@end


// --------------------------------------------------------------------------------------------------

@interface RolePrivilegeListDataSource : GRTListDataSource
{
  DbMysqlRoleEditor *mOwner;
  bec::RolePrivilegeListBE *mList;
}

- (IBAction)checkAll: (id)sender;
- (IBAction)uncheckAll: (id)sender;


- (void)setRoleEditor:(DbMysqlRoleEditor*)owner;
- (void)setListModel:(bec::RolePrivilegeListBE*)be;

@end

@implementation RolePrivilegeListDataSource

- (void)setListModel: (bec::RolePrivilegeListBE*)be
{
  mList= be;
}

- (void)setRoleEditor:(DbMysqlRoleEditor*)owner
{
  mOwner= owner;
}

- (IBAction)checkAll:(id)sender
{
  mList->add_all();

  [mOwner refresh];
}


- (IBAction)uncheckAll:(id)sender
{
  mList->remove_all();

  [mOwner refresh];
}


- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
  if (mList)
  {
    ssize_t enabled;
    mList->get_field(rowIndex, bec::RolePrivilegeListBE::Enabled, enabled);
    
    return @((int)enabled);
  }
  return nil;
}


- (void)tableView:(NSTableView *)aTableView setObjectValue:(id)anObject forTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
  mList->set_field(rowIndex, bec::RolePrivilegeListBE::Enabled, [anObject integerValue]);
}


- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
  if (mList)
    return mList->count();
  return 0;
}


- (void)tableView:(NSTableView *)aTableView willDisplayCell:(id)aCell forTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
  std::string value;
  
  mList->get_field(rowIndex, bec::RolePrivilegeListBE::Name, value);
  
  [aCell setTitle: [NSString stringWithCPPString: value]];
}

@end

//--------------------------------------------------------------------------------------------------

@interface DbMysqlRoleEditor()
{
  IBOutlet NSTabView *tabView;

  IBOutlet NSTextField *nameText;
  IBOutlet NSOutlineView *roleOutline;
  IBOutlet NSTableView *objectTable;
  IBOutlet NSTableView *privilegeTable;
  IBOutlet NSPopUpButton *parentPopUp;

  IBOutlet GRTTreeDataSource *roleTreeDS;
  IBOutlet RolePrivilegeObjectListDataSource *objectListDS;
  IBOutlet RolePrivilegeListDataSource *privilegeListDS;

  bec::RoleEditorBE *mBackEnd;
}

@end

@implementation DbMysqlRoleEditor

static void call_refresh(void *theEditor)
{
  DbMysqlRoleEditor *editor = (__bridge DbMysqlRoleEditor *)theEditor;
  [editor performSelectorOnMainThread: @selector(refresh) withObject: nil waitUntilDone: YES];
}

- (instancetype)initWithModule: (grt::Module *)module
                     arguments: (const grt::BaseListRef &)args
{
  self= [super initWithNibName: @"RoleEditor" bundle: [NSBundle bundleForClass:[self class]]];
  if (self != nil)
  {
    // load GUI. Top level view in the nib is the NSTabView that will be docked to the main window
    [self loadView];

    [self setMinimumSize: [tabView frame].size];

    [roleOutline registerForDraggedTypes: @[[NSString stringWithCPPString: WB_CONTROL_DRAG_TYPE]]];
    
    // setup the object list for accepting drops
    [objectTable registerForDraggedTypes: @[[NSString stringWithCPPString: WB_DBOBJECT_DRAG_TYPE]]];
    
    [self reinitWithArguments: args];
  }
  return self;
}

- (void)reinitWithArguments: (const grt::BaseListRef&)args
{
  [super reinitWithArguments: args];
  delete mBackEnd;
  
  mBackEnd = new bec::RoleEditorBE(db_RoleRef::cast_from(args[0]), get_rdbms_for_db_object(args[0]));
  
  mBackEnd->set_refresh_ui_slot(std::bind(call_refresh, (__bridge void *)self));
  
  [objectListDS setBackEnd: mBackEnd];
  
  [roleTreeDS setTreeModel: mBackEnd->get_role_tree()];
  [objectListDS setListModel: mBackEnd->get_object_list()];
  [privilegeListDS setListModel: mBackEnd->get_privilege_list()];
  
  [objectListDS setRoleEditor: self];
  [privilegeListDS setRoleEditor: self];
  
  bec::NodeId roleNode= mBackEnd->get_role_tree()->node_id_for_role(mBackEnd->get_role());
  [roleOutline selectRowIndexes: [NSIndexSet indexSetWithIndex: [roleOutline rowForItem:[roleTreeDS itemForNodeId: roleNode]]]
           byExtendingSelection: NO];
  
  // update the UI
  [self refresh];
  
  [objectListDS tableViewSelectionDidChange: [NSNotification notificationWithName: NSTableViewSelectionDidChangeNotification
                                                                           object: objectTable]];
}


- (void)dealloc
{
  delete mBackEnd;
}


- (BOOL)matchesIdentifierForClosingEditor:(NSString*)identifier
{
  return mBackEnd->should_close_on_delete_of([identifier UTF8String]);
}


/** Fetches object info from the backend and update the UI
 */
- (void)refresh
{
  if (mBackEnd)
  {
    [nameText setStringValue: [NSString stringWithCPPString: mBackEnd->get_name()]];
    [self updateTitle: [self title]];
    
    [parentPopUp removeAllItems];
    std::vector<std::string> roles(mBackEnd->get_role_list());
    for (std::vector<std::string>::const_iterator iter= roles.begin(); iter != roles.end(); ++iter)
    {
      [parentPopUp addItemWithTitle: [NSString stringWithCPPString: *iter]];
    }
    
    [parentPopUp selectItemWithTitle: [NSString stringWithCPPString: mBackEnd->get_parent_role()]];    
    [roleOutline reloadData];
    [objectTable reloadData];
    [privilegeTable reloadData];
    [roleOutline expandItem:nil expandChildren:YES];
  }
}


- (id)panelId
{
  // an identifier for this editor (just take the object id)
  return [NSString stringWithCPPString:mBackEnd->get_object().id()];
}


- (void)controlTextDidEndEditing:(NSNotification *)aNotification
{
  if ([aNotification object] == nameText)
  {
    // set name of the schema
    mBackEnd->set_name([[nameText stringValue] UTF8String]);
  }
}


- (IBAction)selectedParent:(id)sender
{
  mBackEnd->set_parent_role([[parentPopUp titleOfSelectedItem] UTF8String]);
  [roleOutline reloadData];
  [roleOutline expandItem:nil expandChildren:YES];
}


- (bec::BaseEditor*)editorBE
{
  return mBackEnd;
}

@end
