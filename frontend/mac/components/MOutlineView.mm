/*
 * Copyright (c) 2009, 2016, Oracle and/or its affiliates. All rights reserved.
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

#import "MOutlineView.h"

#import "GRTTreeDataSource.h"
#import "MCPPUtilities.h"

NSString* NSMenuActionNotification = @"NSMenuActionNotification";

@interface NSObject ()
- (bec::TreeModel*)treeModelForOutlineView:(NSOutlineView*)outline NS_RETURNS_INNER_POINTER;
@end


@implementation MOutlineView


static void refresh_tree(const bec::NodeId &node, int ocount, void *tree)
{
  [(__bridge id)tree reloadData];
}


- (std::vector<bec::NodeId>)selectedNodeIds
{
  std::vector<bec::NodeId> nodes;
  NSIndexSet *iset= self.selectedRowIndexes;
  NSUInteger index = iset.firstIndex;
  while (index != NSNotFound)
  {
    nodes.push_back([[self itemAtRow: index] nodeId]);
    index = [iset indexGreaterThanIndex: index];
  }
  
  return nodes;
}


- (bec::TreeModel*)getTreeModel
{
  bec::TreeModel *model= 0;
  id dataSource= [self dataSource];
  
  // hack because of stupid implementation of DbMySQLTableEditor
  if ([dataSource respondsToSelector: @selector(treeModelForOutlineView:)])
    model= [dataSource treeModelForOutlineView: self];
  
  if ([dataSource respondsToSelector: @selector(treeModel)])
    model= [dataSource treeModel];
  
  return model;
}


- (void)reloadData
{
  [super reloadData];

  if (!mConnectedRefresh)
  {
    bec::TreeModel *model = [self getTreeModel];
    if (model)
      model->tree_changed_signal()->connect(std::bind(refresh_tree, std::placeholders::_1, std::placeholders::_2, (__bridge void *)self));
    mConnectedRefresh= YES;
  }
}


- (void)fillMenu:(NSMenu*)menu withItems:(const bec::MenuItemList&)items 
        selector:(SEL)selector
{
  for (bec::MenuItemList::const_iterator iter= items.begin(); iter != items.end(); ++iter)
  {
    if (iter->type == bec::MenuSeparator)
      [menu addItem: [NSMenuItem separatorItem]];
    else
    {
      SEL itemAction;
      if (iter->type == bec::MenuCascade)
      {
        itemAction = nil;
      }
      else
      {
        itemAction = @selector(activateMenuItem:);
      }
      
      NSMenuItem *item= [menu addItemWithTitle: [NSString stringWithCPPString: iter->caption]
                                          action: itemAction
                                   keyEquivalent: @""];
      item.target = self;
      if (!iter->enabled)
        [item setEnabled: NO];
      item.representedObject = [NSString stringWithCPPString: iter->name];
      
      if (iter->type == bec::MenuCascade)
      {
        if (!iter->subitems.empty())
        {
          NSMenu *submenu= [[NSMenu alloc] initWithTitle: @""];
          [self fillMenu: submenu withItems:iter->subitems selector:@selector(activateMenuItem:)];
          item.submenu = submenu;
        }
      }
    }
  }
}


- (NSMenu *)menuForEvent:(NSEvent *)theEvent
{
  bec::TreeModel *model= [self getTreeModel];
  
  if (model)
  {      
    bec::MenuItemList items= model->get_popup_items_for_nodes([self selectedNodeIds]);
    NSMenu *menu= [[NSMenu alloc] initWithTitle: @""];
    if (!items.empty())
    {
      [self fillMenu: menu withItems:items selector:@selector(activateMenuItem:)];
      return menu;
    }
  }

  return [super menuForEvent: theEvent];
}


- (BOOL) canDeleteItem: (id)sender
{
  bec::TreeModel *model= [self getTreeModel];
  
  std::vector<bec::NodeId> nodes= [self selectedNodeIds];
  
  if (model && nodes.size() == 1)
    return model->can_delete_node(nodes.front());

  return NO;
}


- (void)rightMouseDown:(NSEvent *)theEvent
{
  [self selectRowIndexes: [NSIndexSet indexSetWithIndex: [self rowAtPoint: [self convertPoint: theEvent.locationInWindow fromView: nil]]]
    byExtendingSelection: NO];
  [super rightMouseDown: theEvent];
}


- (void) deleteItem: (id)sender
{
  bec::TreeModel *model= [self getTreeModel];
  
  std::vector<bec::NodeId> nodes= [self selectedNodeIds];
  
  if (model && nodes.size() == 1)
  {
    if (model->delete_node(nodes.front()))
      [self noteNumberOfRowsChanged];
  }
}


- (void)activateMenuItem:(id)sender
{
  bec::TreeModel *model= [self getTreeModel];
  if (model)
  {
    if (model->activate_popup_item_for_nodes([[sender representedObject] UTF8String], 
                                            [self selectedNodeIds]))
      ;
      //[self reloadData];
    else
      [[NSNotificationCenter defaultCenter] postNotificationName: NSMenuActionNotification object: sender];
  }
}

@end
