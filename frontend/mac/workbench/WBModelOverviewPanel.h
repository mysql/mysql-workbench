/* 
 * Copyright (c) 2010, 2014, Oracle and/or its affiliates. All rights reserved.
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
#import "WBSplitPanel.h"

#include "base/ui_form.h"

#include "grts/structs.h"

#include "workbench/wb_context_ui.h"

#import "MTableView.h"

@class WBOverviewPanel;
@class WBModelSidebarController;
@class WBObjectDescriptionController;
@class GRTTreeDataSource;
@class GRTListDataSource;
@class MTabSwitcher;

@interface WBModelOverviewPanel : WBSplitPanel
{
  IBOutlet WBOverviewPanel *overview;
  IBOutlet NSSplitView *sideSplitview;
  IBOutlet WBModelSidebarController *sidebarController;
  IBOutlet WBObjectDescriptionController *descriptionController;
  IBOutlet MTabSwitcher *mSwitcherT;
  IBOutlet MTabSwitcher *mSwitcherB;
  
  wb::WBContextUI* _wbui;
}

- (instancetype)initWithWBContextUI:(wb::WBContextUI*)wbui NS_DESIGNATED_INITIALIZER;

@property (readonly, copy) NSString *identifier;

@property (readonly, strong) WBOverviewPanel *overview;

- (void)selectionChanged;

@property (readonly, strong) WBModelSidebarController *sidebarController;
//- (void)setRightSidebar:(BOOL)flag;
@end
