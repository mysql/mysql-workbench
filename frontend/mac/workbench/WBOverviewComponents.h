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

#include "workbench/wb_overview.h"
#import "MVerticalLayoutView.h"
#import "WBSchemaTabView.h"

@class WBOverviewListController;
@class WBOverviewPanel;

typedef NS_ENUM(NSInteger, ListMode) { ListModeLargeIcon, ListModeSmallIcon, ListModeDetails };

@interface WBOverviewGroupContainer : WBSchemaTabView<NSTabViewDelegate> {
  WBOverviewPanel *_owner;
  wb::OverviewBE *_be;
  bec::NodeId *_nodeId;

  float _extraHeight;
  BOOL _updating;
  BOOL _resizing;
}

- (instancetype)initWithOverview:(WBOverviewPanel *)owner nodeId:(const bec::NodeId &)node NS_DESIGNATED_INITIALIZER;
- (void)refreshChildren;

- (void)buildChildren;

- (void)tile;

- (void)setLargeIconMode;
- (void)setSmallIconMode;
- (void)setDetailsMode;
- (void)performGroupAdd:(id)sender;
- (void)performGroupDelete:(id)sender;
@end

@interface WBOverviewGroup : MVerticalLayoutView {
  WBOverviewPanel *_owner;
  wb::OverviewBE *_be;
  bec::NodeId *_nodeId;

  NSTabViewItem *_tabItem;
}

- (instancetype)initWithOverview:(WBOverviewPanel *)owner
                          nodeId:(const bec::NodeId &)node
                         tabItem:(NSTabViewItem *)tabItem NS_DESIGNATED_INITIALIZER;

- (void)updateNodeId:(const bec::NodeId &)node;
@property(readonly) bec::NodeId &nodeId;

- (void)refreshChildren;
- (void)refreshInfo;

- (void)buildChildren;
- (void)setListMode:(ListMode)mode;

@end

@interface WBOverviewItemContainer : NSView<NSMenuDelegate> {
  WBOverviewPanel *_owner;
  wb::OverviewBE *_be;
  bec::NodeId *_nodeId;

  wb::OverviewBE::OverviewDisplayMode _displayMode;

  NSTextField *_descriptionLabel;

  NSTableView *_tableView;
}

- (instancetype)initWithOverview:(WBOverviewPanel *)owner nodeId:(const bec::NodeId &)node NS_DESIGNATED_INITIALIZER;

- (void)updateNodeId:(const bec::NodeId &)node;

- (void)setDisplayMode:(wb::OverviewBE::OverviewDisplayMode)mode;

- (void)selectNode:(const bec::NodeId &)node;
- (void)clearSelection;

- (void)refreshChildren;
- (void)refreshChildInfo:(const bec::NodeId &)node;

@end

@interface WBOverviewSection : WBOverviewItemContainer {
  NSString *_title;
  NSString *_subTitle;
}

- (instancetype)initWithOverview:(WBOverviewPanel *)owner nodeId:(const bec::NodeId &)node NS_DESIGNATED_INITIALIZER;

- (void)setTitle:(NSString *)title;
- (void)setSubTitle:(NSString *)title;

@end
