/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
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


#import <Cocoa/Cocoa.h>

#import "WBPluginEditorBase.h"
#import "GRTListDataSource.h"

#include "grtdb/editor_user_role.h"

@class GRTTreeDataSource;
@class DbMysqlRoleEditor;

// Subclass of List data source to handle object drops
@interface RolePrivilegeObjectListDataSource : GRTListDataSource
{
  DbMysqlRoleEditor *mOwner;
  bec::RoleEditorBE *mBackEnd;
}

- (void)setRoleEditor:(DbMysqlRoleEditor*)owner;
- (void)setBackEnd:(bec::RoleEditorBE*)be;

@end


@interface RolePrivilegeListDataSource : GRTListDataSource
{
  DbMysqlRoleEditor *mOwner;
  bec::RolePrivilegeListBE *mList;
}

- (IBAction)uncheckAll:(id)sender;


- (void)setRoleEditor:(DbMysqlRoleEditor*)owner;
- (void)setListModel:(bec::RolePrivilegeListBE*)be;

@end


@interface DbMysqlRoleEditor : WBPluginEditorBase {
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

- (IBAction)selectedParent:(id)sender;

@end
