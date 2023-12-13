/*
 * Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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

#import "WBSidebarPanel.h"
#import "WBSplitView.h"
#include "mforms/toolbar.h"
#include "grt/grt_manager.h"

#define MIN_SIDEBAR_WIDTH 100

@implementation WBSidebarPanel

- (WBSplitView *)splitView
{
  if ([self.topView isKindOfClass: WBSplitView.class])
    return (WBSplitView *)self.topView;
  return nil;
}

- (void)restoreSidebarsFor:(const char*)name
                   toolbar:(mforms::ToolBar*)toolbar
{
  mToolbar = toolbar;
  mOptionName = name;

  mRestoringSidebars = YES;

  // restore state of toolbar
  toolbar->set_item_checked("wb.toggleSecondarySidebar", !(mSecondarySidebarHidden = bec::GRTManager::get()->get_app_option_int(mOptionName+":SecondarySidebarHidden", 0)));
  toolbar->set_item_checked("wb.toggleSidebar", !(mSidebarHidden = bec::GRTManager::get()->get_app_option_int(mOptionName+":SidebarHidden", 0)));

  mLastSecondarySidebarWidth = MAX(bec::GRTManager::get()->get_app_option_int(mOptionName+":SecondarySidebarWidth", 220), 100);
  mLastSidebarWidth = MAX(bec::GRTManager::get()->get_app_option_int(mOptionName+":SidebarWidth", 220), MIN_SIDEBAR_WIDTH);

  if (mSidebarHidden)
  {
    if (mSidebarAtRight)
      [self.splitView setPosition: NSWidth(self.topView.frame) ofDividerAtIndex: 1];
    else
      [self.splitView setPosition: 0 ofDividerAtIndex: 0];
  }
  else
  {
    if (mSidebarAtRight)
      [self.splitView setPosition: NSWidth(self.splitView.frame) - mLastSidebarWidth ofDividerAtIndex: 1];
    else
      [self.splitView setPosition: mLastSidebarWidth ofDividerAtIndex: 0];

    // ugly hack to force the splitter position where we want it.. somehow, without this "adjustment"
    // the splitter would make the sidebar sized 10px narrower
    if (NSWidth(sidebar.frame) < mLastSidebarWidth)
    {
      if (mSidebarAtRight)
        [self.splitView setPosition: NSWidth(self.splitView.frame) - (mLastSidebarWidth - NSWidth(sidebar.frame)) ofDividerAtIndex: 1];
      else
        [self.splitView setPosition: mLastSidebarWidth + (mLastSidebarWidth - NSWidth(sidebar.frame)) ofDividerAtIndex: 0];
    }
  }

  if (secondarySidebar)
  {
    if (mSecondarySidebarHidden)
    {
      if (mSidebarAtRight)
        [self.splitView setPosition: 0 ofDividerAtIndex: 0];
      else
        [self.splitView setPosition: NSWidth(self.splitView.frame) ofDividerAtIndex: 1];
    }
    else
    {
      if (mSidebarAtRight)
        [self.splitView setPosition: mLastSecondarySidebarWidth ofDividerAtIndex: 0];
      else
        [self.splitView setPosition: NSWidth(self.splitView.frame) - mLastSecondarySidebarWidth ofDividerAtIndex: 1];
    }
  }

  mRestoringSidebars = NO;
}


- (void)splitViewDidResizeSubviews:(NSNotification *)notification
{
  if (notification.object != self.topView || mHidingSidebar || mRestoringSidebars)
    return;
  if (!notification.userInfo) // for when the splitview get resized, instead of dragged
    return;

  if (!mSidebarHidden)
    bec::GRTManager::get()->set_app_option(mOptionName+":SidebarWidth", grt::IntegerRef((int)NSWidth(sidebar.frame)));

  {
    BOOL newCollapseState = [self.splitView isSubviewCollapsed: sidebar];
    if (mToolbar) {
      BOOL hidden = !mToolbar->get_item_checked("wb.toggleSidebar");
      
      if (newCollapseState != hidden)
      {
        bec::GRTManager::get()->set_app_option(mOptionName+":SidebarHidden", grt::IntegerRef(newCollapseState));
        mToolbar->set_item_checked("wb.toggleSidebar", newCollapseState);
      }
    }
    if (!newCollapseState)
    {
      int width = NSWidth(sidebar.frame);
      if (width <= 0)
        width = MIN_SIDEBAR_WIDTH;
      bec::GRTManager::get()->set_app_option(mOptionName+":SidebarWidth", grt::IntegerRef(width));
    }
  }

  if (secondarySidebar)
  {
    if (!mSecondarySidebarHidden)
    {
      double width;
      if (mSidebarAtRight)
        width = NSWidth(secondarySidebar.frame);
      else
        width = NSWidth(self.splitView.frame) - NSWidth(secondarySidebar.frame);
      bec::GRTManager::get()->set_app_option(mOptionName+":SecondarySidebarWidth", grt::IntegerRef((int)width));
    }
    {
      BOOL newCollapseState = [self.splitView isSubviewCollapsed: secondarySidebar];
      if (mToolbar) {
        BOOL hidden = !mToolbar->get_item_checked("wb.toggleSecondarySidebar");
        
        if (newCollapseState != hidden)
        {
          bec::GRTManager::get()->set_app_option(mOptionName+":SecondarySidebarHidden", grt::IntegerRef(newCollapseState));
          mToolbar->set_item_checked("wb.toggleSecondarySidebar", !newCollapseState);
        }
      }
      if (!newCollapseState)
      {
        int width = NSWidth(secondarySidebar.frame);
        if (width <= 0)
          width = MIN_SIDEBAR_WIDTH;
        bec::GRTManager::get()->set_app_option(mOptionName+":SecondarySidebarWidth", grt::IntegerRef(width));
      }
    }
  }
}



- (BOOL)splitView:(NSSplitView *)splitView shouldAdjustSizeOfSubview:(NSView *)subview
{
  if (splitView == self.splitView && (subview == sidebar || subview == secondarySidebar))
    return NO;
  return YES;
}


- (CGFloat)splitView:(NSSplitView *)splitView constrainMinCoordinate:(CGFloat)proposedMin ofSubviewAt:(NSInteger)dividerIndex
{
  return proposedMin;
}


- (CGFloat)splitView:(NSSplitView *)splitView constrainMaxCoordinate:(CGFloat)proposedMax ofSubviewAt:(NSInteger)dividerIndex
{
  if (splitView == self.splitView)
  {
    return proposedMax > NSWidth(splitView.frame) - MIN_SIDEBAR_WIDTH ? NSWidth(splitView.frame) - MIN_SIDEBAR_WIDTH : proposedMax;
  }
  return proposedMax;
}


- (BOOL)splitView:(NSSplitView *)splitView canCollapseSubview:(NSView *)subview
{
  if (splitView == self.splitView && (subview == sidebar || subview == secondarySidebar))
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
        [self.splitView setPosition: NSWidth(self.splitView.frame) - mLastSecondarySidebarWidth - (self.splitView).dividerThickness ofDividerAtIndex: 1];
      else
        [self.splitView setPosition: mLastSecondarySidebarWidth ofDividerAtIndex: 0];
    }
    else
    {
      mLastSecondarySidebarWidth = NSWidth(secondarySidebar.frame);
      if (!mSidebarAtRight)
        [self.splitView setPosition: NSWidth(self.splitView.frame) ofDividerAtIndex: 1];
      else
        [self.splitView setPosition: 0 ofDividerAtIndex: 0];
    }
  }
  else
  {
    mLastSidebarWidth = MAX(mLastSidebarWidth, MIN_SIDEBAR_WIDTH);
    if (!hidden)
    {
      if (!mSidebarAtRight)
        [self.splitView setPosition: mLastSidebarWidth ofDividerAtIndex: 0];
      else
        [self.splitView setPosition: NSWidth(self.splitView.frame) - mLastSidebarWidth - (self.splitView).dividerThickness ofDividerAtIndex: 1];
    }
    else
    {
      mLastSidebarWidth = NSWidth(sidebar.frame);
      if (!mSidebarAtRight)
        [self.splitView setPosition: 0 ofDividerAtIndex: 0];
      else
        [self.splitView setPosition: NSWidth(self.splitView.frame) ofDividerAtIndex: 1];
    }
  }
  mHidingSidebar = NO;
}


- (void)performCommand: (const std::string) command
{
  if (command == "wb.toggleSecondarySidebar")
  {
    mSecondarySidebarHidden = !mToolbar->get_item_checked(command);
    bec::GRTManager::get()->set_app_option(mOptionName+":SecondarySidebarHidden", grt::IntegerRef(mSecondarySidebarHidden));
    [self hideSideBar: mSecondarySidebarHidden secondary: YES];
  }
  else if (command == "wb.toggleSidebar")
  {
    mSidebarHidden = !mToolbar->get_item_checked(command);
    bec::GRTManager::get()->set_app_option(mOptionName+":SidebarHidden", grt::IntegerRef(mSidebarHidden));
    [self hideSideBar: mSidebarHidden secondary: NO];
  }
}

@end
