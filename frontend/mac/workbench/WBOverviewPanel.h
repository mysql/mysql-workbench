/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#import <Cocoa/Cocoa.h>
#import "WBBasePanel.h"

#include "workbench/wb_overview.h"

@class WBOverviewBackgroundView;
@class WBMenuManager;

@interface WBOverviewPanel : NSScrollView {
  WBOverviewBackgroundView *_backgroundView;
  NSMutableDictionary *_itemContainers;

  NSString *_identifier;

  bec::NodeId *_lastFoundNode;
  NSString *_searchText;

  wb::OverviewBE *_overview;

  BOOL _noHeaders;
}

- (void)setupWithOverviewBE:(wb::OverviewBE *)overview;
- (void)setNoBackground;
- (void)setNoHeader;
- (void)rebuildAll;
- (void)refreshNode:(const bec::NodeId &)node;
- (void)refreshNodeChildren:(const bec::NodeId &)node;

@property(readonly) BOOL willClose;

@property(readonly, strong) NSView *topView;
@property(readonly, copy) NSString *title;
@property(readonly, copy) NSString *identifier;
@property(readonly) bec::UIForm *formBE;
@property(readonly) wb::OverviewBE *backend;

- (id)itemContainerForNode:(const bec::NodeId &)node;

- (void)searchString:(NSString *)text;

- (void)registerContainer:(id)container forItem:(NSString *)item;

- (void)unregisterContainerForItem:(NSString *)item;

- (void)buildMainSections;

@end
