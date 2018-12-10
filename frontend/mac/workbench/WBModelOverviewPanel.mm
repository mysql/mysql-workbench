/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "tree_model.h"
#include "wb_overview_physical.h"
#include "wb_context_model.h"
#include "workbench/wb_context.h"

#import "WBModelOverviewPanel.h"
#import "WBOverviewPanel.h"
#import "WBModelSidebarController.h"
#import "WBObjectDescriptionController.h"
#import "GRTListDataSource.h"
#import "GRTTreeDataSource.h"
#import "MTabSwitcher.h"
#import "WBTabView.h"
#import "WBSplitView.h"
#import "MFView.h"
#import "MContainerView.h"

//----------------------------------------------------------------------------------------------------------------------

@interface WBModelOverviewPanel () {
  __weak IBOutlet WBOverviewPanel *overview;
  __weak IBOutlet NSSplitView *sideSplitview;
  __weak IBOutlet WBModelSidebarController *sidebarController;
  __weak IBOutlet WBObjectDescriptionController *descriptionController;
  __weak IBOutlet MTabSwitcher *mSwitcherT;
  __weak IBOutlet MTabSwitcher *mSwitcherB;

  NSMutableArray *nibObjects;
}

@end;

//----------------------------------------------------------------------------------------------------------------------

@implementation WBModelOverviewPanel

- (instancetype)init {
  self = [super init];
  if (self != nil) {
    NSMutableArray *temp;
    if ([NSBundle.mainBundle loadNibNamed: @"WBModelOverview" owner: self topLevelObjects: &temp]) {
      nibObjects = temp;

      [overview setupWithOverviewBE: wb::WBContextUI::get()->get_physical_overview()];
      [sidebarController setupWithContext: wb::WBContextUI::get()->get_wb()->get_model_context()];
      mSwitcherT.tabStyle = MSectionTabSwitcher;
      mSwitcherB.tabStyle = MSectionTabSwitcher;
      [descriptionController setup];

      [overview performSelector: @selector(rebuildAll) withObject: nil afterDelay: 0.1];

      self.splitView.autosaveName = @"modelSplitPosition";

      [self restoreSidebarsFor: "ModelOverview" toolbar: wb::WBContextUI::get()->get_physical_overview()->get_toolbar()];
    }
  }

  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  // Make sure scheduled rebuildAll won't blow up if it didn't exec yet.
  [NSObject cancelPreviousPerformRequestsWithTarget: overview];

  [sidebarController invalidate];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString*)panelId {
  return overview.panelId;
}

//----------------------------------------------------------------------------------------------------------------------

- (WBOverviewPanel*)overview {
  return overview;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString*)title {
  return overview.title;
}

//----------------------------------------------------------------------------------------------------------------------

- (bec::UIForm*)formBE {
  return overview.formBE;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)didActivate {
  NSView *view = nsviewForView(wb::WBContextUI::get()->get_wb()->get_model_context()->shared_secondary_sidebar());
  if (view.superview)
    [view removeFromSuperview];

  [secondarySidebar addSubview: view];
  view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable | NSViewMinXMargin | NSViewMinYMargin
    | NSViewMaxXMargin | NSViewMaxYMargin;
  view.frame = secondarySidebar.bounds;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)willClose {
  return overview.willClose;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)selectionChanged {
  [descriptionController updateForForm: self.formBE];
}

//----------------------------------------------------------------------------------------------------------------------

- (WBModelSidebarController*)sidebarController {
  return sidebarController;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)splitView: (NSSplitView *)splitView shouldAdjustSizeOfSubview: (NSView *)subview {
  if (subview == bottomContainer)
    return NO;

  return [super splitView: splitView shouldAdjustSizeOfSubview: subview];
}

@end

//----------------------------------------------------------------------------------------------------------------------
