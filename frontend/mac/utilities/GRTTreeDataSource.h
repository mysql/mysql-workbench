/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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
#import "GRTListDataSource.h"

@protocol GRTDragDelegate
- (BOOL)dataSource:(id)source writeItems:(NSArray *)items toPasteboard:(NSPasteboard *)pboard;
@end

@interface GRTTreeDataSource : NSObject<NSOutlineViewDataSource, NSOutlineViewDelegate> {
  bec::TreeModel *_tree;
  NSMutableDictionary *_nodeCache;
  NSFont *_normalFont;
  NSFont *_boldFont;
  BOOL _hideRootNode;
  id _dragDelegate;
}

- (instancetype)initWithTreeModel:(bec::TreeModel *)model NS_DESIGNATED_INITIALIZER;
- (void)setHidesRootNode:(BOOL)flag;

- (NSMutableSet *)storeExpansionStateOfOutlineView:(NSOutlineView *)outlineView usingValueOfColumn:(id)column;
- (void)restoreExpansionStateOfOutlineView:(NSOutlineView *)outlineView
                                 fromState:(NSMutableSet *)state
                        usingValueOfColumn:(id)column;

@property bec::TreeModel *treeModel;
- (void)refreshModel;

- (bec::NodeId)nodeIdForItem:(id)item;
- (id)itemForNodeId:(const bec::NodeId &)nodeId;

- (void)setDragDelegate:(id<GRTDragDelegate>)delegate;

@end
