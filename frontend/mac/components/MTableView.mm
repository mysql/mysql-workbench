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

#import "GRTListDataSource.h"
#import "MCPPUtilities.h"

#import "MOutlineView.h"
#import "MTableView.h"

@interface NSObject ()
- (bec::ListModel*)listModelForTableView:(NSTableView*)table NS_RETURNS_INNER_POINTER;
@end


@implementation MTableView

- (void)delete: (id)foo
{
  SEL selector = NSSelectorFromString(@"userDeleteSelectedRowInTableView:");

  if ([self.delegate respondsToSelector: selector])
    ((void (*)(id, SEL, id))[(id)self.delegate methodForSelector: selector])(self.delegate, selector, self);
}


- (std::vector<bec::NodeId>)selectedNodeIds
{
  std::vector<bec::NodeId> nodes;
  NSIndexSet *iset= self.selectedRowIndexes;
  NSUInteger index = iset.firstIndex;
  while (index != NSNotFound)
  {
    nodes.push_back(bec::NodeId(index));
    index = [iset indexGreaterThanIndex: index];
  }
  
  return nodes;
}


- (bec::ListModel*)getListModel
{
  bec::ListModel *model= 0;
  id dataSource= [self dataSource];
  
  // hack because of stupid implementation of DbMySQLTableEditor
  if ([dataSource respondsToSelector: @selector(listModelForTableView:)])
    model= [dataSource listModelForTableView: self];
  
  if ([dataSource respondsToSelector: @selector(listModel)])
    model= [dataSource listModel];

  return model;
}


- (NSMenu *)menuForEvent:(NSEvent *)theEvent
{
  bec::ListModel *model= [self getListModel];
    
  if (model)
  {      
    bec::MenuItemList items= model->get_popup_items_for_nodes(self.selectedNodeIds);
    
    if (!items.empty())
    {
      NSMenu *menu= [[NSMenu alloc] initWithTitle: @""];
      [menu setAutoenablesItems: NO];
      
      for (bec::MenuItemList::const_iterator iter= items.begin(); iter != items.end(); ++iter)
      {        
        if (iter->type == bec::MenuSeparator)
          [menu addItem: [NSMenuItem separatorItem]];
        else
        {
          NSMenuItem *item= [menu addItemWithTitle: [NSString stringWithCPPString: iter->caption]
                                action: @selector(activateMenuItem:)
                         keyEquivalent: @""];
          item.target = self;
          if (!iter->enabled)
            [item setEnabled: NO];
          item.representedObject = [NSString stringWithCPPString: iter->internalName];
        }
      }
      return menu;
    }
  }
  return [super menuForEvent: theEvent];
}



- (void)activateMenuItem:(id)sender
{
  bec::ListModel *model= [self getListModel];
    
  if (model)
  {
    if (model->activate_popup_item_for_nodes([[sender representedObject] UTF8String], 
                                             self.selectedNodeIds))
      [self reloadData];
    else
      [[NSNotificationCenter defaultCenter] postNotificationName: NSMenuActionNotification object: sender];
  }
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
  NSInteger row = [self rowAtPoint: [self convertPoint: theEvent.locationInWindow fromView: nil]];
  if (row >= 0)
  {
    if (![self isRowSelected: row])
      [self selectRowIndexes: [NSIndexSet indexSetWithIndex: row]
        byExtendingSelection: NO];
  }
  [super rightMouseDown: theEvent];
}


- (BOOL) canDeleteItem: (id)sender
{
  bec::ListModel *model= [self getListModel];
    
  std::vector<bec::NodeId> nodes= self.selectedNodeIds;
  
  if (model && nodes.size() == 1)
    return model->can_delete_node(nodes.front());
  
  return NO;
}


- (void) deleteItem: (id)sender
{
  bec::ListModel *model= [self getListModel];
  
  std::vector<bec::NodeId> nodes= self.selectedNodeIds;
  
  if (model && nodes.size() == 1)
  {
    if (model->delete_node(nodes.front()))
      [self noteNumberOfRowsChanged];
  }
}

@end
