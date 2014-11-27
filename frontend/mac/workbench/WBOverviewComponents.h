/* 
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
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
#import "MVerticalLayoutView.h"
#import "WBSchemaTabView.h"

@class WBOverviewListController;
@class WBOverviewPanel;

typedef enum {
  ListModeLargeIcon,
  ListModeSmallIcon,
  ListModeDetails
} ListMode;

@interface WBOverviewGroupContainer : WBSchemaTabView <NSTabViewDelegate>
{
  WBOverviewPanel *_owner;
  wb::OverviewBE *_be;
  bec::NodeId *_nodeId;
  
  float _extraHeight;
  BOOL _updating;
  BOOL _resizing;
}

- (id)initWithOverview:(WBOverviewPanel*)owner
                nodeId:(const bec::NodeId&)node;
- (void)refreshChildren;

- (void)buildChildren;

- (void)tile;

- (void)setLargeIconMode;
- (void)setSmallIconMode;
- (void)setDetailsMode;
- (void)performGroupAdd:(id)sender;
- (void)performGroupDelete:(id)sender;
@end



@interface WBOverviewGroup : MVerticalLayoutView
{
  WBOverviewPanel *_owner;
  wb::OverviewBE *_be;
  bec::NodeId *_nodeId;
  
  NSTabViewItem *_tabItem;
}

- (id)initWithOverview:(WBOverviewPanel*)owner
                nodeId:(const bec::NodeId&)node
               tabItem:(NSTabViewItem*)tabItem;

- (void)updateNodeId:(const bec::NodeId&)node;
- (bec::NodeId&)nodeId;

- (void)refreshChildren;
- (void)refreshInfo;

- (void)buildChildren;
- (void)setListMode: (ListMode) mode;

@end



@interface WBOverviewItemContainer : NSView <NSMenuDelegate>
{
  WBOverviewPanel* _owner;
  wb::OverviewBE *_be;
  bec::NodeId *_nodeId;

  NSMutableArray *_nibObjects;
  
  wb::OverviewBE::OverviewDisplayMode _displayMode;

  NSTextField *_descriptionLabel;
  
  NSCollectionView *_iconView;
  WBOverviewListController *_iconController;
  
  NSTableView *_tableView;
  
  NSMenu *_contextMenu; // this is created in teh collectionView nib
}

- (id)initWithOverview:(WBOverviewPanel*)owner
                nodeId:(const bec::NodeId&)node;

- (void)updateNodeId:(const bec::NodeId&)node;

- (void)setDisplayMode:(wb::OverviewBE::OverviewDisplayMode)mode;

- (void)selectNode:(const bec::NodeId&)node;
- (void)clearSelection;

- (void)refreshChildren;
- (void)refreshChildInfo:(const bec::NodeId&)node;

@end



@interface WBOverviewSection : WBOverviewItemContainer 
{
  NSString *_title;
  NSString *_subTitle;
}

- (id)initWithOverview:(WBOverviewPanel*)owner
                nodeId:(const bec::NodeId&)node;


- (void)setTitle:(NSString*)title;
- (void)setSubTitle:(NSString*)title;

@end
