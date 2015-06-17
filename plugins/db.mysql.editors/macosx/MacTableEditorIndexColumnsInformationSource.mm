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

#import "MacTableEditorIndexColumnsInformationSource.h"

#include "mysql_table_editor.h"

@implementation MacTableEditorIndexColumnsInformationSource



- (BOOL) rowEnabled: (NSInteger) rowIndex;
{
  NSAssert(mBackEnd != nil, @"The mBackEnd must not be nil.");
  
  const ::bec::NodeId& node = mBackEnd->get_indexes()->get_columns()->get_node(rowIndex);
  BOOL yn = (mBackEnd->get_indexes()->get_columns()->get_column_enabled(node) ? YES : NO);
  
  return yn;
}



- (void) setRow: (NSInteger) indexRowIndex
        enabled: (BOOL) yn;
{
  NSAssert(mBackEnd != nil, @"The mBackEnd must not be nil.");
  
  bec::IndexColumnsListBE *index_columns_be(mBackEnd->get_indexes()->get_columns());
  const ::bec::NodeId& node = mBackEnd->get_indexes()->get_node(indexRowIndex);
  index_columns_be->set_column_enabled(node, ( yn ? true : false ) );
}



- (instancetype) initWithListModel: (bec::ListModel*) model
            tableBackEnd: (MySQLTableEditorBE*) tableBackend;
{
  self = [self initWithListModel: model];
  
  if (self != nil) {
    mBackEnd = tableBackend;
  }
  
  return self;
}



@end


