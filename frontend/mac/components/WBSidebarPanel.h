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

#import "WBBasePanel.h"
#include <string>

namespace bec {
  class GRTManager;
};

namespace mforms {
  class ToolBar;
};

@class WBSplitView;

@interface WBSidebarPanel : WBBasePanel {
  IBOutlet __weak NSView *sidebar;
  IBOutlet __weak NSView *secondarySidebar;

  std::string mOptionName;

  CGFloat mLastSidebarWidth;
  CGFloat mLastSecondarySidebarWidth;

  mforms::ToolBar *mToolbar;

  BOOL mSidebarHidden;
  BOOL mSecondarySidebarHidden;

  BOOL mSidebarAtRight;
  BOOL mHidingSidebar;
  BOOL mRestoringSidebars;
}

@property(nonatomic, readonly, weak)
  WBSplitView *splitView; // Returns the top level view if that is a split panel actually.

- (void)restoreSidebarsFor:(const char *)name toolbar:(mforms::ToolBar *)toolbar;
- (void)hideSideBar:(BOOL)hidden secondary:(BOOL)flag;
- (void)performCommand:(const std::string)command;

- (void)splitViewDidResizeSubviews:(NSNotification *)notification;
- (BOOL)splitView:(NSSplitView *)splitView shouldAdjustSizeOfSubview:(NSView *)subview;
- (CGFloat)splitView:(NSSplitView *)splitView
  constrainMinCoordinate:(CGFloat)proposedMin
             ofSubviewAt:(NSInteger)dividerIndex;
- (CGFloat)splitView:(NSSplitView *)splitView
  constrainMaxCoordinate:(CGFloat)proposedMax
             ofSubviewAt:(NSInteger)dividerIndex;
- (BOOL)splitView:(NSSplitView *)splitView canCollapseSubview:(NSView *)subview;

@end
