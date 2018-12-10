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

#import <Cocoa/Cocoa.h>
#import "WBBasePanel.h"

#include "workbench/wb_overview.h"

@class WBOverviewBackgroundView;
@class WBMenuManager;

@interface WBOverviewPanel : NSScrollView {
  WBOverviewBackgroundView *_backgroundView;
  NSMutableDictionary *_itemContainers;

  NSString *_panelId;

  bec::NodeId *_lastFoundNode;
  NSString *_searchText;

  wb::OverviewBE *_overview;

  BOOL _noHeaders;
}

- (void)setupWithOverviewBE:(wb::OverviewBE *)overview;
- (void)setNoHeader;
- (void)rebuildAll;
- (void)refreshNode:(const bec::NodeId &)node;
- (void)refreshNodeChildren:(const bec::NodeId &)node;

@property(readonly) BOOL willClose;

@property(readonly, strong) NSView *topView;
@property(readonly, copy) NSString *title;
@property(readonly, copy) NSString *panelId;
@property(readonly) bec::UIForm *formBE;
@property(readonly) wb::OverviewBE *backend;

- (id)itemContainerForNode:(const bec::NodeId &)node;

- (void)searchString:(NSString *)text;

- (void)registerContainer:(id)container forItem:(NSString *)item;

- (void)unregisterContainerForItem:(NSString *)item;

- (void)buildMainSections;

@end
