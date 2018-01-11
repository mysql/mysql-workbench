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
