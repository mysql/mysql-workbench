/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#import <Cocoa/Cocoa.h>

#import "WBPluginEditorBase.h"

@class GRTListDataSource;
@class GRTTreeDataSource;

#include "grtdb/editor_dbobject.h"
#include "grtdb/dbobject_roles.h"
#include "grtdb/role_tree_model.h"

@interface DbPrivilegeEditorTab : NSObject 
{
  bec::DBObjectEditorBE *_be;

  bec::ObjectRoleListBE *_rolesListBE;
  bec::RoleTreeBE *_roleTreeBE;
  bec::ObjectPrivilegeListBE *_privilegeListBE;

  IBOutlet NSView *view;

  IBOutlet NSTableView *assignedRolesTable;
  IBOutlet NSTableView *privilegesTable;
  IBOutlet NSOutlineView *allRolesOutline;

  IBOutlet GRTListDataSource *assignedRolesDS;
  IBOutlet GRTTreeDataSource *allRolesDS;
}

- (id)initWithObjectEditor:(bec::DBObjectEditorBE*)be;

- (IBAction)addRole:(id)sender;
- (IBAction)deleteRole:(id)sender;

- (NSView*)view;

@end
