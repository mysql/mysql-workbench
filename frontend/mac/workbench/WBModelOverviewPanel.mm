/* 
 * Copyright (c) 2010, 2013, Oracle and/or its affiliates. All rights reserved.
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
#import "MFView.h"

@implementation WBModelOverviewPanel

- (id)initWithWBContextUI:(wb::WBContextUI*)wbui
{
  self = [super init];
  if (self)
  {
    _wbui= wbui;
    
    [NSBundle loadNibNamed: @"WBModelOverview" owner: self];
    [(id)editorTabView createDragger];

    [overview setupWithOverviewBE: wbui->get_physical_overview()];
    [sidebarController setupWithWBContextUI: wbui];
    [mSwitcherT setTabStyle: MPaletteTabSwitcherSmallText];
    [mSwitcherB setTabStyle: MPaletteTabSwitcherSmallText];
    [descriptionController setWBContext: wbui];

    [topView setDividerThickness: 1];
    [topView setBackgroundColor: [NSColor colorWithDeviceWhite:128/255.0 alpha:1.0]];

   // [overview rebuildAll];
    [overview performSelector:@selector(rebuildAll) withObject:nil afterDelay:0.1];

    grtm = _wbui->get_wb()->get_grt_manager();

    [topView setAutosaveName: @"modelSplitPosition"];

    [self restoreSidebarsFor: "ModelOverview" toolbar: wbui->get_physical_overview()->get_toolbar()];
  }
  return self;
}


- (void)dealloc
{
  // make sure scheduled rebuildAll won't blow up if it didn't exec yet
  [NSObject cancelPreviousPerformRequestsWithTarget: overview];

  [sidebarController invalidate];
  
  [topView release];
  [sidebarController release];
  [descriptionController release];
  [mainSplitViewDelegate release];

  [super dealloc];
}


- (NSString*)identifier
{
  return [overview identifier];
}


- (WBOverviewPanel*)overview
{
  return overview;
}


- (NSView*)topView
{
  return topView;
}


- (NSString*)title
{  
  return [overview title];
}


- (bec::UIForm*)formBE
{
  return [overview formBE];
}

- (void)didActivate
{
  NSView *view = nsviewForView(_wbui->get_wb()->get_model_context()->shared_secondary_sidebar());
  if ([view superview])
  {
    [view retain];
    [view removeFromSuperview];
  }
  [secondarySidebar addSubview: view];
  [view setAutoresizingMask: NSViewWidthSizable|NSViewHeightSizable|NSViewMinXMargin|NSViewMinYMargin|NSViewMaxXMargin|NSViewMaxYMargin];
  [view setFrame: [secondarySidebar bounds]];
}

- (BOOL)willClose
{
  return [overview willClose];
}

- (void)selectionChanged
{
  [descriptionController updateForForm: [self formBE]];
}


- (WBModelSidebarController*)sidebarController
{
  return sidebarController;
}

//--------------------------------------------------------------------------------------------------

- (BOOL)splitView:(NSSplitView *)splitView shouldAdjustSizeOfSubview:(NSView *)subview
{
  if (subview == bottomContainer)
    return NO;

  return [super splitView: splitView shouldAdjustSizeOfSubview: subview];
}

//--------------------------------------------------------------------------------------------------
/*
- (void)setRightSidebar:(BOOL)flag
{
  sidebarAtRight = flag;
  
  id view1 = [[topView subviews] objectAtIndex: 0];
  id view2 = [[topView subviews] objectAtIndex: 1];

  if (sidebarAtRight)
  {
    if (view2 != sidebar)
    {
      [[view1 retain] autorelease];
      [view1 removeFromSuperview];
      [topView addSubview: view1];
    }
  }
  else
  {
    if (view1 != sidebar)
    {
      [[view1 retain] autorelease];
      [view1 removeFromSuperview];
      [topView addSubview: view1];
    }
  }
}*/

@end
