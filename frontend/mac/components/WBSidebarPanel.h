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

#import "WBBasePanel.h"
#include <string>

namespace bec
{
  class GRTManager;
};

namespace mforms
{
  class ToolBar;
};


@class WBSplitView;

@interface WBSidebarPanel : WBBasePanel
{
  IBOutlet WBSplitView *topView;
  IBOutlet NSView *sidebar;
  IBOutlet NSView *secondarySidebar;

  std::string mOptionName;

  CGFloat mLastSidebarWidth;
  CGFloat mLastSecondarySidebarWidth;

  mforms::ToolBar* mToolbar;
  bec::GRTManager* grtm;

  BOOL mSidebarHidden;
  BOOL mSecondarySidebarHidden;

  BOOL mSidebarAtRight;
  BOOL mHidingSidebar;
}

- (void)restoreSidebarsFor:(const char*)name
                   toolbar:(mforms::ToolBar*)toolbar;
- (void)hideSideBar:(BOOL)hidden secondary:(BOOL)flag;
- (void)performCommand: (const std::string) command;

- (void)splitViewDidResizeSubviews: (NSNotification*)notification;
- (BOOL)splitView:(NSSplitView *)splitView shouldAdjustSizeOfSubview:(NSView *)subview;
- (CGFloat)splitView:(NSSplitView *)splitView constrainMinCoordinate:(CGFloat)proposedMin ofSubviewAt:(NSInteger)dividerIndex;
- (CGFloat)splitView:(NSSplitView *)splitView constrainMaxCoordinate:(CGFloat)proposedMax ofSubviewAt:(NSInteger)dividerIndex;
- (BOOL)splitView:(NSSplitView *)splitView canCollapseSubview:(NSView *)subview;
- (BOOL)splitView:(NSSplitView *)splitView shouldAdjustSizeOfSubview:(NSView *)subview;
@end
