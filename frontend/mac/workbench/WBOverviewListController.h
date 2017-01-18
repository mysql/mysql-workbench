/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "workbench/wb_overview.h"

@class MCollectionViewItemView;

// Acts both as a model/controller to a NSCollectionView and as a datasource to a NSOutlineView
// for providing contents for item lists

@interface WBOverviewListController : NSObject {
  NSMutableArray* mItems;
  NSMutableIndexSet* mSelectedIndexes;

  IBOutlet NSCollectionView* collectionView;
  IBOutlet NSCollectionViewItem* largeIcon;
  IBOutlet NSCollectionViewItem* smallIcon;

  wb::OverviewBE* mOverview;
}

- (void)setOverviewBE:(wb::OverviewBE*)overview;

- (void)setCollectionView:(NSCollectionView*)view;

@property(copy) NSMutableArray* items;

@property(copy) NSIndexSet* selectedIndexes;

- (void)fillFromChildrenOf:(const bec::NodeId&)node overview:(wb::OverviewBE*)overview iconSize:(bec::IconSize)iconSize;

- (void)setShowLargeIcons:(BOOL)flag;

@end
