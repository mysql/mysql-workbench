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

#include "mforms/treeview.h"
#include <map>

@class MFTreeNodeImpl;

@interface MFTreeViewImpl : NSScrollView<NSOutlineViewDataSource, NSOutlineViewDelegate> // someone else
{
  NSOutlineView *mOutline;
  NSMutableDictionary *mIconCache;
  MFTreeNodeImpl *mRootNode;
  NSMutableArray *mColumnKeys;
  mforms::TreeView *mOwner;
  NSMutableDictionary *mAttributedFonts;

  std::map<std::string, MFTreeNodeImpl *> mTagMap;
  NSArray *mDraggedNodes;

  int mSortColumn;
  int mFreezeCount;
  BOOL mSortColumnEnabled;
  BOOL mCanReorderRows;
  BOOL mCanBeDragSource;
  BOOL mPendingReload;
  BOOL mTagIndexEnabled;
  BOOL mColumnsAutoResize;
@public
  BOOL mFlatTable;
  BOOL mSmallFont;
}

@property(readonly) mforms::TreeView *backend;

- (NSString *)keyForColumn:(int)column;
@property(readonly, weak) NSOutlineView *outlineView;
@property(readonly) BOOL frozen;

- (NSImage *)iconForFile:(NSString *)path;

- (void)setNeedsReload;

- (void)setNode:(MFTreeNodeImpl *)node forTag:(const std::string &)tag;
@end
