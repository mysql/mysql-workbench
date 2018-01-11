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

#import "GRTListDataSource.h"
#import "MTextImageCell.h"
#import "GRTIconCache.h"

@implementation GRTNodeId

static std::map<std::string, GRTNodeId *> node_cache;

+ (GRTNodeId *)nodeIdWithNodeId: (const bec::NodeId &)nodeId {
  return [[GRTNodeId alloc] initWithNodeId: nodeId];
}

- (instancetype)init {
  if ((self = [super init]) != nil) {
    _nodeId = new bec::NodeId();
  }
  return self;
}

- (instancetype)initWithNodeId: (const bec::NodeId &)nodeId {
  if ((self = [super init]) != nil) {
    _nodeId = new bec::NodeId(nodeId);
  }
  return self;
}

- (void)dealloc {
  delete _nodeId;
}

- (const bec::NodeId &)nodeId {
  return *_nodeId;
}

- (NSString *)description {
  return [NSString stringWithFormat: @"<GRTNodeId %s>", _nodeId->description().c_str()];
}

- (BOOL)isEqual: (id)other {
  if (self == other)
    return YES;

  if ([other isKindOfClass: [GRTNodeId class]])
    return *_nodeId == [other nodeId];

  return NO;
}

@end

@implementation GRTListDataSource

- (instancetype)init {
  return [self initWithListModel: nil];
}

- (instancetype)initWithListModel: (bec::ListModel *)model {
  self = [super init];
  if (self != nil) {
    _list = model; // The list model can also be set later.
  }
  return self;
}

- (void)setListModel:(bec::ListModel *)model {
  _list = model;
}

- (bec::ListModel *)listModel {
  return _list;
}

- (id)tableView:(NSTableView *)aTableView
  objectValueForTableColumn: (NSTableColumn *)aTableColumn
                        row: (NSInteger)rowIndex {
  if (_list) {
    std::string value;
    _list->get_field(rowIndex, aTableColumn.identifier.integerValue, value);

    return @(value.c_str());
  }
  return nil;
}

- (void)tableView: (NSTableView *)aTableView
   setObjectValue: (id)anObject
   forTableColumn: (NSTableColumn *)aTableColumn
              row: (NSInteger)rowIndex {
  if (_list->is_editable(rowIndex)) {
    size_t count = _list->count();
    _list->set_field(rowIndex, aTableColumn.identifier.integerValue, [anObject UTF8String]);
    if (_list->count() > count)
      [aTableView reloadData];
  } else if ([anObject isKindOfClass: [NSNumber class]]) {
    _list->set_field(rowIndex, aTableColumn.identifier.integerValue, (ssize_t)[anObject integerValue]);
  }
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView {
  if (_list)
    return _list->count();
  return 0;
}

- (BOOL)tableView: (NSTableView *)aTableView
  shouldEditTableColumn: (NSTableColumn *)aTableColumn
                    row: (NSInteger)rowIndex {
  return _list->is_editable(rowIndex);
}

- (void)tableView: (NSTableView *)aTableView
  willDisplayCell: (id)aCell
   forTableColumn: (NSTableColumn *)aTableColumn
              row: (NSInteger)rowIndex {
  if ([aCell isKindOfClass:[MTextImageCell class]]) {
    bec::IconId icon_id = _list->get_field_icon(rowIndex, aTableColumn.identifier.integerValue, bec::Icon16);

    if (icon_id != 0) {
      NSImage *image = [[GRTIconCache sharedIconCache] imageForIconId: icon_id];
      [aCell setImage: image];
    } else
      [aCell setImage: nil];
  }
}

@end

