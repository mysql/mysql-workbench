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

@interface MCollectionViewItemView : NSView<NSTextDelegate, NSDraggingSource, NSPasteboardItemDataProvider> {
@private
  NSPoint mMouseDownLocation;
  BOOL mIsEditing;

  BOOL mBecameFirstResponder;
@public
  IBOutlet __weak id delegate;
}

@property(weak) NSCollectionViewItem *owner;

@property(readonly, strong) NSCollectionView *activeCollectionView;

@property BOOL selected;

@property(weak) id delegate;

// Inline editing.
- (void)beginInlineEditing;
- (void)stopInlineEditing:(BOOL)accepted;
@end

@interface NSObject (MCollectionViewItemViewDelegate)
- (void)activateCollectionItem:(id)sender;
- (void)selectCollectionItem:(id)sender;
- (void)unselectCollectionItem:(id)sender;
- (void)clearSelection;
- (BOOL)isCollectionItemSelected:(id)sender;
- (BOOL)canRename:(id)sender;

- (BOOL)declareDragDataForItem:(id)sender pasteboard:(NSPasteboard *)pasteboard;
- (NSArray *)dropTypesForItem:(id)sender;
- (BOOL)handleDroppedData:(id)data forItem:(id)item;
@end
