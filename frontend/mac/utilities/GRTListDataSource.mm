/*
 * Copyright (c) 2008, 2014 Oracle and/or its affiliates. All rights reserved.
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

#import "GRTListDataSource.h"
#import "MTextImageCell.h"
#import "GRTIconCache.h"

@implementation GRTNodeId

static std::map<std::string, GRTNodeId*> node_cache;


+ (GRTNodeId*)nodeIdWithNodeId:(const bec::NodeId&)nodeId
{/*
  if (nodeId.depth() <= 2)
  {
    std::string repr = nodeId.repr();
    std::map<std::string, GRTNodeId*>::iterator iter = node_cache.find(repr);
    GRTNodeId *node;

    if (iter != node_cache.end())
      return iter->second;

    node = [[GRTNodeId alloc] initWithNodeId:nodeId];
    node_cache[repr] = node;

    return node;
  }*/
  return [[[GRTNodeId alloc] initWithNodeId:nodeId] autorelease];
}


- (id)init
{
  if ((self= [super init]) != nil)
  {
    _nodeId= new bec::NodeId();
  }
  return self;
}


- (id)initWithNodeId:(const bec::NodeId&)nodeId
{
  if ((self= [super init]) != nil)
  {
    _nodeId= new bec::NodeId(nodeId);
  }
  return self;
}


- (void)dealloc
{
  delete _nodeId;
  
  [super dealloc];
}


- (const bec::NodeId&)nodeId
{
  return *_nodeId;
}


- (NSString*)description
{
  return [NSString stringWithFormat: @"<GRTNodeId %s>", _nodeId->repr().c_str()];
}


- (BOOL)isEqual:(id)other
{
  if (self == other) return YES;

  if ([other isKindOfClass: [GRTNodeId class]])
    return *_nodeId == [other nodeId];

  return NO;
}

@end


@implementation GRTListDataSource

- (id)initWithListModel:(bec::ListModel*)model
{
  if ((self = [super init]) != nil)
  {
    _list= model;
  }
  return self;
}


- (void)setListModel:(bec::ListModel*)model
{
  _list= model;
}


- (bec::ListModel*)listModel
{
  return _list;
}


- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
  if (_list)
  {
    std::string value;
    _list->get_field(rowIndex, [[aTableColumn identifier] integerValue], value);
  
    return [NSString stringWithUTF8String:value.c_str()];
  }
  return nil;
}


- (void)tableView:(NSTableView *)aTableView setObjectValue:(id)anObject forTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
  if (_list->is_editable(rowIndex))
  {
    int count= _list->count();
    _list->set_field(rowIndex, [[aTableColumn identifier] integerValue], [anObject UTF8String]);
    if (_list->count() > count)
      [aTableView reloadData];
  }
  else if ([anObject isKindOfClass: [NSNumber class]])
  {
    _list->set_field(rowIndex, [[aTableColumn identifier] integerValue], (ssize_t)[anObject integerValue]);
  }
}


- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
  if (_list)
    return _list->count();
  return 0;
}


- (BOOL)tableView:(NSTableView *)aTableView shouldEditTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
  return _list->is_editable(rowIndex);
}


- (void)tableView:(NSTableView *)aTableView willDisplayCell:(id)aCell forTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
  if ([aCell isKindOfClass:[MTextImageCell class]])
  {
    bec::IconId icon_id= _list->get_field_icon(rowIndex, [[aTableColumn identifier] integerValue], bec::Icon16);

    if (icon_id != 0)
    {
      NSImage *image= [[GRTIconCache sharedIconCache] imageForIconId:icon_id];
      [aCell setImage:image];
    }
    else
      [aCell setImage:nil];
  }
}


@end


