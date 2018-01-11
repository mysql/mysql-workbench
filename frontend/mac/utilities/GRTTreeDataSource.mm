/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#import "GRTTreeDataSource.h"
#import "GRTIconCache.h"
#import "MTextImageCell.h"
 
@implementation GRTTreeDataSource

- (instancetype)initWithTreeModel: (bec::TreeModel*)model
{
  self = [super init];
  if (self != nil)
  {
    _tree = model;
    _hideRootNode = NO;
    _nodeCache = [NSMutableDictionary new];
  }
  return self;
}

- (instancetype)init
{
  return [self initWithTreeModel: nil];
}



- (void)setTreeModel:(bec::TreeModel*)model
{
  _tree= model;
}

- (void)setHidesRootNode:(BOOL)flag
{
  _hideRootNode= flag;
}


- (bec::TreeModel*)treeModel
{
  return _tree;
}


- (void)refreshModel
{
  [_nodeCache removeAllObjects];
  
  _tree->refresh();
}


- (bec::NodeId)nodeIdForItem:(id)item
{
  if (item)
    return [item nodeId];
  else
  {
    if (_hideRootNode)
      return bec::NodeId(0);
    else
      return bec::NodeId();
  }
}


- (id)itemForNodeId:(const bec::NodeId&)nodeId
{
  NSString *key= @(nodeId.toString().c_str());
  GRTNodeId *node;
  if (!(node= _nodeCache[key]))
  {
    node= [GRTNodeId nodeIdWithNodeId:nodeId];
    _nodeCache[key] = node;
  }
  return node;
}


- (id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item
{
  try
  {
    return [self itemForNodeId: _tree->get_child([self nodeIdForItem:item], index)];
  }
  catch (const std::exception &e)
  {
    return nil;
  }
}


- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item
{
  BOOL flag;
  try
  {
    flag=  !item || _tree->is_expandable([self nodeIdForItem:item]);
  }
  catch (...)
  {
    std::string s;
    _tree->get_field(_tree->get_parent([self nodeIdForItem:item]), 0, s);
  }
  return flag;
}


- (void)outlineViewItemWillExpand:(NSNotification *)notification
{
  id item= notification.userInfo[@"NSObject"];
  if (_tree)
    _tree->expand_node([self nodeIdForItem:item]);
}


- (void)outlineViewItemDidCollapse:(NSNotification *)notification
{
  id item= notification.userInfo[@"NSObject"];
  
  if (_tree)
    _tree->collapse_node([self nodeIdForItem:item]);
}


- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item
{
  if (_tree)
    return _tree->count_children([self nodeIdForItem:item]);
  return 0;
}


- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
  std::string strvalue;
  int column= tableColumn.identifier.intValue;

  if (!_tree || !_tree->get_field([self nodeIdForItem:item], column, strvalue))
    return nil;
  
  return @(strvalue.c_str());
}


- (void)outlineView:(NSOutlineView *)outlineView setObjectValue:(id)object forTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
  int column= tableColumn.identifier.intValue;
  
  if ([object respondsToSelector:@selector(UTF8String)])
    _tree->set_field([self nodeIdForItem:item], column, [object UTF8String]);
  else if ([object respondsToSelector:@selector(integerValue)])
    _tree->set_field([self nodeIdForItem:item], column, (ssize_t)[object integerValue]);
}


- (void)outlineView:(NSOutlineView *)outlineView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item
{
  if ([cell isKindOfClass:[MTextImageCell class]])
  {
    bec::NodeId node_id= [self nodeIdForItem:item];
    bec::IconId icon_id= 0;
    if (_tree != nil)
      icon_id= _tree->get_field_icon(node_id, tableColumn.identifier.integerValue, bec::Icon16);
    
    NSImage *image= [[GRTIconCache sharedIconCache] imageForIconId:icon_id];
    
    if (icon_id != 0 && !image && _tree->is_expandable(node_id))
    {
      image= [[GRTIconCache sharedIconCache] imageForFolder:bec::Icon16];
    }

    if (icon_id != 0)
      [cell setImage:image];
    else
      [cell setImage:nil];

    {
      if (!_normalFont)
      {
        _normalFont = [NSFont systemFontOfSize: [cell font].pointSize];
        _boldFont = [NSFont boldSystemFontOfSize: _normalFont.pointSize];
      }
      
      if (_tree && _tree->is_highlighted(node_id))
        [cell setFont: _boldFont];
      else
        [cell setFont: _normalFont];
    }
  }
}


- (void)setDragDelegate:(id<GRTDragDelegate>)delegate
{
  _dragDelegate= delegate;
}


- (BOOL)outlineView:(NSOutlineView *)outlineView writeItems:(NSArray *)items toPasteboard:(NSPasteboard *)pboard
{
  if (_dragDelegate)
    return [_dragDelegate dataSource:self
                          writeItems:items
                        toPasteboard:pboard];
  
  return NO;
}


static void restore_expanded_child_nodes(NSMutableSet *mset,
                                         GRTTreeDataSource *ds,
                                         NSOutlineView *outlineView,
                                         id column,
                                         NSString *prefix,
                                         id node)
{
  if ([mset containsObject: prefix])
  {
    [outlineView collapseItem: node];
    ds.treeModel->expand_node([ds nodeIdForItem: node]);
    [outlineView expandItem: node];
  }
  
  for (NSInteger c = [ds outlineView:outlineView numberOfChildrenOfItem: node], i = 0; i < c; i++)
  {
    id child = [ds outlineView:outlineView child:i ofItem:node];
    NSString *suffix = [ds outlineView:outlineView objectValueForTableColumn:column byItem:child];
    if (suffix)
    {
      NSString *childPrefix = [NSString stringWithFormat: @"%@/%@", prefix, suffix];
      restore_expanded_child_nodes(mset, ds, outlineView, column, 
                                   childPrefix, 
                                   child);
    }
  }
}


- (void)restoreExpansionStateOfOutlineView:(NSOutlineView*)outlineView
                                 fromState:(NSMutableSet*)state
                        usingValueOfColumn:(id)column
{
  restore_expanded_child_nodes(state, self, outlineView, 
                               [outlineView tableColumnWithIdentifier:column], @"", nil);
}


static void save_expanded_child_nodes(NSMutableSet *mset,
                                      GRTTreeDataSource *ds,
                                      NSOutlineView *outlineView,
                                      id column,
                                      NSString *prefix,
                                      id node)
{
  if ([outlineView isItemExpanded: node])
    [mset addObject: prefix];
  
  for (NSInteger c = [ds outlineView: outlineView numberOfChildrenOfItem: node], i = 0; i < c; i++)
  {
    id child = [ds outlineView:outlineView child:i ofItem:node];
    NSString *suffix = [ds outlineView:outlineView objectValueForTableColumn:column byItem:child];
    if (suffix)
    {
      NSString *childPrefix = [NSString stringWithFormat: @"%@/%@", prefix, suffix];
      save_expanded_child_nodes(mset, ds, outlineView, column, 
                                childPrefix, 
                                child);
    }
  }
}

- (NSMutableSet*)storeExpansionStateOfOutlineView:(NSOutlineView*)outlineView
                               usingValueOfColumn:(id)column
{
  NSMutableSet *mset = [NSMutableSet set];
  
  save_expanded_child_nodes(mset, self, outlineView, 
                            [outlineView tableColumnWithIdentifier:column], @"", nil);
  return mset;
}

@end
