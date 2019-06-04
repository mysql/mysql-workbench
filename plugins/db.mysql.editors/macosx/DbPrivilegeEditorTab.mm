/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#import "DbPrivilegeEditorTab.h"

#import "GRTListDataSource.h"
#import "GRTTreeDataSource.h"

#include "grtdb/db_object_helpers.h"
#include "grtdb/dbobject_roles.h"
#include "grtdb/role_tree_model.h"

//----------------------------------------------------------------------------------------------------------------------

@interface DbPrivilegeEditorTab () {
  bec::DBObjectEditorBE *_be;

  bec::ObjectRoleListBE *_rolesListBE;
  bec::RoleTreeBE *_roleTreeBE;
  bec::ObjectPrivilegeListBE *_privilegeListBE;

  IBOutlet __weak NSTableView *assignedRolesTable;
  IBOutlet __weak NSTableView *privilegesTable;
  IBOutlet __weak NSOutlineView *allRolesOutline;

  IBOutlet __weak GRTListDataSource *assignedRolesDS;
  IBOutlet __weak GRTTreeDataSource *allRolesDS;

  NSMutableArray *nibObjects;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation DbPrivilegeEditorTab

@synthesize view;

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithObjectEditor: (bec::DBObjectEditorBE*)be {
  self = [super init];
  if (self != nil)
  {
    _be = be;
    if (_be != nullptr) {
      NSBundle *bundle = [NSBundle bundleForClass: self.class];
      NSMutableArray *temp;
      if ([bundle loadNibNamed: @"PrivilegesTab" owner: self topLevelObjects: &temp]) {
        nibObjects = temp;
        _rolesListBE = new bec::ObjectRoleListBE(be, get_rdbms_for_db_object(be->get_dbobject()));
        _roleTreeBE = new bec::RoleTreeBE(be->get_catalog());
        _privilegeListBE = _rolesListBE->get_privilege_list();

        _roleTreeBE->refresh();

        [allRolesDS setTreeModel:_roleTreeBE];
        [assignedRolesDS setListModel:_rolesListBE];

        [allRolesOutline reloadData];
        [assignedRolesTable reloadData];
      }
    }
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)init {
  return [self initWithObjectEditor: NULL];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  delete _rolesListBE;
  delete _roleTreeBE;
}

//----------------------------------------------------------------------------------------------------------------------

- (IBAction)addRole: (id)sender {
  NSInteger row = [allRolesOutline selectedRow];
  if (row >= 0) {
    bec::NodeId node = [allRolesDS nodeIdForItem:[allRolesOutline itemAtRow: row]];
    if (node.is_valid()) {
      _rolesListBE->add_role_for_privileges(_roleTreeBE->get_role_with_id(node));
      [assignedRolesTable reloadData];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (IBAction)deleteRole:(id)sender {
  NSInteger row= [allRolesOutline selectedRow];
  if (row >= 0) {
    bec::NodeId node= [allRolesDS nodeIdForItem:[allRolesOutline itemAtRow: row]];
    if (node.is_valid()) {
      _rolesListBE->remove_role_from_privileges(_roleTreeBE->get_role_with_id(node));
      [assignedRolesTable reloadData];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tableViewSelectionDidChange:(NSNotification *)aNotification {
  if ([aNotification object] == assignedRolesTable) {
    NSInteger selected = [assignedRolesTable selectedRow];
    if (selected < 0)
      _rolesListBE->select_role(bec::NodeId());
    else
      _rolesListBE->select_role(selected);

    [privilegesTable reloadData];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)outlineViewSelectionDidChange:(NSNotification *)notification {
  if ([notification object] == allRolesOutline) {
    if ([allRolesOutline selectedRow] >= 0) {
      [[view viewWithTag:10] setEnabled: YES];
      [[view viewWithTag:11] setEnabled: YES];
    } else {
      [[view viewWithTag:10] setEnabled: NO];
      [[view viewWithTag:11] setEnabled: NO];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tableView: (NSTableView *)aTableView willDisplayCell: (id)aCell
   forTableColumn: (NSTableColumn *)aTableColumn
              row: (NSInteger)rowIndex {
  if (aTableView == privilegesTable) {
    ssize_t enabled;
    std::string text;
    _privilegeListBE->get_field(bec::NodeId(rowIndex), (int)bec::ObjectPrivilegeListBE::Enabled, enabled);
    _privilegeListBE->get_field(bec::NodeId(rowIndex), (int)bec::ObjectPrivilegeListBE::Name, text);
    
    [aCell setState: enabled ? NSControlStateValueOn : NSControlStateValueOff];
    [aCell setTitle: @(text.c_str())];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tableView: (NSTableView *)aTableView
   setObjectValue: (id)anObject
   forTableColumn: (NSTableColumn *)aTableColumn
              row: (NSInteger)rowIndex {
  if (aTableView == privilegesTable) {
    if ([anObject isKindOfClass: NSNumber.class]) {
      _privilegeListBE->set_field(rowIndex, [aTableColumn.identifier integerValue], [anObject integerValue]);
      [assignedRolesTable reloadData];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView {
  if (aTableView == privilegesTable)
  {
    if (_privilegeListBE)
      return _privilegeListBE->count();
  }
  return 0;
}

@end

//----------------------------------------------------------------------------------------------------------------------
