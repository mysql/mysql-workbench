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

#import "WBOverviewListController.h"
#import "GRTIconCache.h"
#import "MCPPUtilities.h"
#import "MCollectionViewItemView.h"

#include "grtdb/db_object_helpers.h"
#include "workbench/wb_context.h" // temporary

@implementation WBOverviewListController

- (instancetype)init {
  if ((self = [super init]) != nil) {
    mSelectedIndexes = nil;
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver: self];
}

- (void)setCollectionView:(NSCollectionView *)view {
  collectionView = view;

  [[NSNotificationCenter defaultCenter] addObserver: self
                                           selector: @selector(collectionFrameChanged:)
                                               name: NSViewFrameDidChangeNotification
                                             object: view];
}

- (void)relayoutCollectionView: (NSCollectionView *)collection {
  NSUInteger count = mItems.count;
  NSRect itemRect = collection.itemPrototype.view.frame;
  NSUInteger itemsPerRow;
  NSUInteger rowCount = 0;
  NSSize newSize;

  itemsPerRow = floor(NSWidth(collection.frame) / NSWidth(itemRect));
  if (itemsPerRow > 0) {
    rowCount = ceil((count + (itemsPerRow / 2)) / itemsPerRow);
    if (rowCount * itemsPerRow < count)
      rowCount++;
  }
  if (rowCount == 0)
    rowCount = 1;

  newSize.width = NSWidth(collection.frame);
  newSize.height = NSHeight(itemRect) * rowCount;

  if (!NSEqualSizes(collection.frame.size, newSize))
    [collection setFrameSize: newSize];

  [collection setNeedsDisplay: YES];
}

- (void)collectionFrameChanged: (NSNotification *)notif {
  NSCollectionView *collection = notif.object;

  [self relayoutCollectionView:collection];
}

- (void)fillFromChildrenOf: (const bec::NodeId &)node
                  overview: (wb::OverviewBE *)overview
                  iconSize: (bec::IconSize)iconSize {
  NSMutableArray *items = [NSMutableArray array];
  GRTIconCache *iconCache = [GRTIconCache sharedIconCache];

  for (size_t c = overview->count_children(node), i = 0; i < c; i++) {
    bec::NodeId child = overview->get_child(node, i);
    NSImage *icon;
    std::string name;

    overview->get_field(child, wb::OverviewBE::Label, name);
    icon = [iconCache imageForIconId:overview->get_field_icon(child, wb::OverviewBE::Label, iconSize)];
    if (!icon)
      icon = [NSImage imageNamed:@"MySQLWorkbench-16.png"];

    [items addObject: [NSMutableDictionary
                       dictionaryWithObjectsAndKeys: @(name.c_str()), @"name",
                                                     [NSString stringWithCPPString:child.toString().c_str()], @"path",
                                                     icon, @"image", // put image as last because it can be nil if the
                                                                     // image doesnt exist
                                                     nil]];
  }

  self.items = items;

  [self relayoutCollectionView: collectionView];
}

//--------------------------------------------------------------------------------------------------

- (void)setShowLargeIcons:(BOOL)flag {
  if (flag)
    collectionView.itemPrototype = largeIcon;
  else
    collectionView.itemPrototype = smallIcon;
}

//--------------------------------------------------------------------------------------------------

- (void)setItems:(NSMutableArray *)items {
  mItems = items;
}

//--------------------------------------------------------------------------------------------------

- (NSIndexSet *)selectedIndexes {
  return mSelectedIndexes;
}

//--------------------------------------------------------------------------------------------------

/**
 * Add a single index to the current selection.
 */
- (void)selectIndex: (NSUInteger)index {
  if (mSelectedIndexes == nil)
    mSelectedIndexes = [NSMutableIndexSet indexSetWithIndex: index];
  else
    [mSelectedIndexes addIndex: index];

  mOverview->begin_selection_marking();

  NSDictionary *item = mItems[index];
  bec::NodeId node;
  NSString *path = item[@"path"];
  node = bec::NodeId(path.CPPString);
  mOverview->select_node(node);

  mOverview->end_selection_marking();
}

//--------------------------------------------------------------------------------------------------

/**
 * Add a range of selected indices to the current selection.
 */
- (void)setSelectedIndexes: (NSIndexSet *)indexes {
  if (mSelectedIndexes != indexes) {
    if ([indexes isKindOfClass: [NSMutableIndexSet class]])
      mSelectedIndexes = (NSMutableIndexSet *)indexes;
    else
      mSelectedIndexes = [[NSMutableIndexSet alloc] initWithIndexSet: indexes];

    if (mSelectedIndexes) {
      mOverview->begin_selection_marking();
      if (mSelectedIndexes.count > 0) {
        for (NSUInteger i = mSelectedIndexes.firstIndex; i <= mSelectedIndexes.lastIndex;
             i = [mSelectedIndexes indexGreaterThanIndex:i]) {
          NSDictionary *item = mItems[i];
          bec::NodeId node;
          NSString *path = item[@"path"];
          node = bec::NodeId(path.CPPString);
          mOverview->select_node(node);
        }
      }
      mOverview->end_selection_marking();
    }
  }
}

//--------------------------------------------------------------------------------------------------

- (void)clearSelection {
  if (mSelectedIndexes) {
    mSelectedIndexes = nil;

    mOverview->begin_selection_marking(); // This call clears the selection in the backend.
    mOverview->end_selection_marking();
  }
}

//--------------------------------------------------------------------------------------------------

- (void)activateCollectionItem:(id)sender {
  NSDictionary *item = [[sender owner] representedObject];
  bec::NodeId node;
  NSString *path = item[@"path"];

  node = bec::NodeId(path.CPPString);

  mOverview->activate_node(node);
}

//--------------------------------------------------------------------------------------------------

/**
 * Select the given index without removing other entries from the current selection.
 */
- (void)selectCollectionItem:(id)sender {
  bec::NodeId node([[[sender owner] representedObject][@"path"] CPPString]);

  [self selectIndex:node.back()];
}

//--------------------------------------------------------------------------------------------------

/**
 * Unselect the given index by removing it from current selection.
 */
- (void)unselectCollectionItem:(id)sender {
  bec::NodeId node([[[sender owner] representedObject][@"path"] CPPString]);

  [mSelectedIndexes removeIndex: node.back()];
}

//--------------------------------------------------------------------------------------------------

/**
 * Queries the selected indices if they contain the given item.
 */
- (BOOL)isCollectionItemSelected:(id)sender {
  if ([sender owner]) {
    bec::NodeId node([[[sender owner] representedObject][@"path"] CPPString]);

    return [mSelectedIndexes containsIndex: node.back()];
  }
  return NO;
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by collection item views if they got renamed.
 */
- (void)itemRenameDidEnd: (id)sender withName: (NSString *)newName {
  NSDictionary *item = [[sender owner] representedObject];
  bec::NodeId node;
  NSString *path = item[@"path"];

  node = bec::NodeId(path.CPPString);

  mOverview->set_field(node, wb::OverviewBE::Label, newName.CPPString);
}

/**
 * Called by a collection item view when it is about to start inline editing.
 * It queries the backend if an item is allowed to be edited.
 */
- (BOOL)canRename: (id)sender {
  NSDictionary *item = [[sender owner] representedObject];
  bec::NodeId node;
  NSString *path = item[@"path"];

  node = bec::NodeId(path.CPPString);

  return mOverview->is_editable(node);
}

- (void)setOverviewBE: (wb::OverviewBE *)overview {
  mOverview = overview;
}

- (NSMutableArray *)items {
  return mItems;
}

- (BOOL)declareDragDataForItem: (id)sender pasteboard: (NSPasteboard *)pasteboard {
  NSDictionary *item = [[sender owner] representedObject];
  if (item) {
    bec::NodeId node;
    NSString *path = item[@"path"];

    node = bec::NodeId(path.CPPString);

    std::string type = mOverview->get_node_drag_type(node);
    if (!type.empty()) {
      grt::ListRef<GrtObject> sel = mOverview->get_selection();
      std::list<db_DatabaseObjectRef> objects;

      for (grt::ListRef<GrtObject>::const_iterator obj = sel.begin(); obj != sel.end(); ++obj) {
        if ((*obj).is_valid() && db_DatabaseObjectRef::can_wrap(*obj)) {
          objects.push_back(db_DatabaseObjectRef::cast_from(*obj));
        }
      }

      if (!objects.empty()) {
        std::string text = bec::CatalogHelper::dbobject_list_to_dragdata(objects);

        [pasteboard declareTypes: @[ [NSString stringWithCPPString: type] ] owner: self];
        [pasteboard setString: [NSString stringWithCPPString:text] forType: [NSString stringWithCPPString: type]];
        return YES;
      }
    }
  }
  return NO;
}

- (NSArray *)dropTypesForItem: (id)sender {
  NSDictionary *item = [[sender owner] representedObject];
  if (item) {
    NSString *path = item[@"path"];
    bec::NodeId node = bec::NodeId(path.CPPString);
    std::string type = mOverview->get_node_drag_type(node);
    if (!type.empty())
      return @[[NSString stringWithCPPString: type]];
  }
  return nil;
}

- (BOOL)handleDroppedData: (id)data forItem: (id)item {
  return NO;
}

@end
