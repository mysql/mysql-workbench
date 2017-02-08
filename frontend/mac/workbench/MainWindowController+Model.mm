/* 
 * Copyright (c) 2009, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "workbench/wb_context.h"

#import "MainWindowController+Model.h"
#import "WBModelSidebarController.h"
#import "WBOverviewPanel.h"
#import "WBModelDiagramPanel.h"
#import "WBModelOverviewPanel.h"
#include "wb_overview_physical.h"

@implementation MainWindowController(MainWindowControllerModel)

- (void)setupModel
{
}

- (void)handleModelCreated
{
  // Set up the model overview tab
  _physicalOverview = [[WBModelOverviewPanel alloc] init];
  [self addTopPanelAndSwitch: _physicalOverview];
}

- (void)handleModelClosed
{
  [self closeTopPanelWithIdentifier: _physicalOverview.identifier hideOnly: NO];
  _physicalOverview= nil;
}

@end
