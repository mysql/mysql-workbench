/*
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved.
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

#import "WBSidebarPanel.h"
#import "WBSplitView.h"
#include "mforms/toolbar.h"
#include "grt/grt_manager.h"

#define MIN_SIDEBAR_WIDTH 100

@implementation WBSidebarPanel

- (void)restoreSidebarsFor:(const char*)name
                   toolbar:(mforms::ToolBar*)toolbar
{
  mToolbar = toolbar;
  mOptionName = name;

  // restore state of toolbar
  toolbar->set_item_checked("wb.toggleSecondarySidebar", !(mSecondarySidebarHidden = grtm->get_app_option_int(mOptionName+":SecondarySidebarHidden", 0)));
  toolbar->set_item_checked("wb.toggleSidebar", !(mSidebarHidden = grtm->get_app_option_int(mOptionName+":SidebarHidden", 0)));

  mLastSecondarySidebarWidth = MAX(grtm->get_app_option_int(mOptionName+":SecondarySidebarWidth", 220), 100);
  mLastSidebarWidth = MAX(grtm->get_app_option_int(mOptionName+":SidebarWidth", 220), MIN_SIDEBAR_WIDTH);

  if (mSidebarHidden)
  {
    if (mSidebarAtRight)
      [topView setPosition: NSWidth([topView frame]) ofDividerAtIndex: 1];
    else
      [topView setPosition: 0 ofDividerAtIndex: 0];
  }
  else
  {
    if (mSidebarAtRight)
      [topView setPosition: NSWidth([topView frame]) - mLastSidebarWidth ofDividerAtIndex: 1];
    else
      [topView setPosition: mLastSidebarWidth ofDividerAtIndex: 0];

    // ugly hack to force the splitter position where we want it.. somehow, without this "adjustment"
    // the splitter would make the sidebar sized 10px narrower
    if (NSWidth([sidebar frame]) < mLastSidebarWidth)
    {
      if (mSidebarAtRight)
        [topView setPosition: NSWidth([topView frame]) - (mLastSidebarWidth - NSWidth([sidebar frame])) ofDividerAtIndex: 1];
      else
        [topView setPosition: mLastSidebarWidth + (mLastSidebarWidth - NSWidth([sidebar frame])) ofDividerAtIndex: 0];
    }
  }

  if (secondarySidebar)
  {
    if (mSecondarySidebarHidden)
    {
      if (mSidebarAtRight)
        [topView setPosition: 0 ofDividerAtIndex: 0];
      else
        [topView setPosition: NSWidth([topView frame]) ofDividerAtIndex: 1];
    }
    else
    {
      if (mSidebarAtRight)
        [topView setPosition: mLastSecondarySidebarWidth ofDividerAtIndex: 0];
      else
        [topView setPosition: NSWidth([topView frame]) - mLastSecondarySidebarWidth ofDividerAtIndex: 1];
    }
  }
}


- (void)splitViewDidResizeSubviews:(NSNotification *)notification
{
  if ([notification object] != topView || mHidingSidebar)
    return;
  if (![notification userInfo]) // for when the splitview get resized, instead of dragged
    return;

  if (!mSidebarHidden)
    grtm->set_app_option(mOptionName+":SidebarWidth", grt::IntegerRef((int)NSWidth([sidebar frame])));
  {
    BOOL newCollapseState = [topView isSubviewCollapsed: sidebar];
    BOOL hidden = !mToolbar->get_item_checked("wb.toggleSidebar");

    if (newCollapseState != hidden)
    {
      grtm->set_app_option(mOptionName+":SidebarHidden", grt::IntegerRef(newCollapseState));
      mToolbar->set_item_checked("wb.toggleSidebar", newCollapseState);
    }
    if (!newCollapseState)
    {
      int width = NSWidth([sidebar frame]);
      if (width <= 0)
        width = MIN_SIDEBAR_WIDTH;
      grtm->set_app_option(mOptionName+":SidebarWidth", grt::IntegerRef(width));
    }
  }

  if (secondarySidebar)
  {
    if (!mSecondarySidebarHidden)
      grtm->set_app_option(mOptionName+":SecondarySidebarWidth", grt::IntegerRef((int)NSWidth([secondarySidebar frame])));
    {
      BOOL newCollapseState = [topView isSubviewCollapsed: secondarySidebar];
      BOOL hidden = !mToolbar->get_item_checked("wb.toggleSecondarySidebar");

      if (newCollapseState != hidden)
      {
        grtm->set_app_option(mOptionName+":SecondarySidebarHidden", grt::IntegerRef(newCollapseState));
        mToolbar->set_item_checked("wb.toggleSecondarySidebar", !newCollapseState);
      }
      if (!newCollapseState)
      {
        int width = NSWidth([secondarySidebar frame]);
        if (width <= 0)
          width = MIN_SIDEBAR_WIDTH;
        grtm->set_app_option(mOptionName+":SecondarySidebarWidth", grt::IntegerRef(width));
      }
    }
  }
}



- (BOOL)splitView:(NSSplitView *)splitView shouldAdjustSizeOfSubview:(NSView *)subview
{
  if (splitView == topView && (subview == sidebar || subview == secondarySidebar))
    return NO;
  return YES;
}


- (CGFloat)splitView:(NSSplitView *)splitView constrainMinCoordinate:(CGFloat)proposedMin ofSubviewAt:(NSInteger)dividerIndex
{
  return proposedMin;
}


- (CGFloat)splitView:(NSSplitView *)splitView constrainMaxCoordinate:(CGFloat)proposedMax ofSubviewAt:(NSInteger)dividerIndex
{
  if (splitView == topView)
  {
    return proposedMax > NSWidth([splitView frame]) - MIN_SIDEBAR_WIDTH ? NSWidth([splitView frame]) - MIN_SIDEBAR_WIDTH : proposedMax;
  }
  return proposedMax;
}


- (BOOL)splitView:(NSSplitView *)splitView canCollapseSubview:(NSView *)subview
{
  if (splitView == topView && (subview == sidebar || subview == secondarySidebar))
    return YES;
  return NO;
}


- (void)hideSideBar:(BOOL)hidden secondary:(BOOL)flag
{
  mHidingSidebar = YES;
  if (flag)
  {
    mLastSecondarySidebarWidth = MAX(mLastSecondarySidebarWidth, MIN_SIDEBAR_WIDTH);

    if (!hidden)
    {
      if (!mSidebarAtRight)
        [topView setPosition: NSWidth([topView frame])-mLastSecondarySidebarWidth ofDividerAtIndex: 1];
      else
        [topView setPosition: mLastSecondarySidebarWidth ofDividerAtIndex: 0];
    }
    else
    {
      mLastSecondarySidebarWidth = NSWidth([secondarySidebar frame]);
      if (!mSidebarAtRight)
        [topView setPosition: NSWidth([topView frame]) ofDividerAtIndex: 1];
      else
        [topView setPosition: 0 ofDividerAtIndex: 0];
    }
  }
  else
  {
    mLastSidebarWidth = MAX(mLastSidebarWidth, MIN_SIDEBAR_WIDTH);
    if (!hidden)
    {
      if (!mSidebarAtRight)
        [topView setPosition: mLastSidebarWidth ofDividerAtIndex: 0];
      else
        [topView setPosition: NSWidth([topView frame])-mLastSidebarWidth ofDividerAtIndex: 1];
    }
    else
    {
      mLastSidebarWidth = NSWidth([sidebar frame]);
      if (!mSidebarAtRight)
        [topView setPosition: 0 ofDividerAtIndex: 0];
      else
        [topView setPosition: NSWidth([topView frame]) ofDividerAtIndex: 1];
    }
  }
  mHidingSidebar = NO;
}


- (void)performCommand: (const std::string) command
{
  if (command == "wb.toggleSecondarySidebar")
  {
    mSecondarySidebarHidden = !mToolbar->get_item_checked(command);
    grtm->set_app_option(mOptionName+":SecondarySidebarHidden", grt::IntegerRef(mSecondarySidebarHidden));
    [self hideSideBar: mSecondarySidebarHidden secondary: YES];
  }
  else if (command == "wb.toggleSidebar")
  {
    mSidebarHidden = !mToolbar->get_item_checked(command);
    grtm->set_app_option(mOptionName+":SidebarHidden", grt::IntegerRef(mSidebarHidden));
    [self hideSideBar: mSidebarHidden secondary: NO];
  }
}

@end
