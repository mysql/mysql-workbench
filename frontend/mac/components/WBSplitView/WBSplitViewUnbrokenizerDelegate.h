/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

@interface WBSplitViewUnbrokenizerDelegate : NSObject {
  CGFloat mTopExpandedMinHeight;
  CGFloat mTopCollapsedMinHeight;
  CGFloat mBottomExpandedMinHeight;
  CGFloat mBottomCollapsedMinHeight;

  CGFloat mTopCollapseLimit;
  CGFloat mBottomCollapseLimit;

  NSMutableSet *mUncollapsableViews;

  IBOutlet __weak id delegate;

  BOOL mTopCollapsed;
  BOOL mBottomCollapsed;
}

- (void)preventCollapseOfView:(NSView *)view;

- (void)setTopCollapseLimit:(CGFloat)h;
- (void)setBottomCollapseLimit:(CGFloat)h;

- (void)setTopExpandedMinHeight:(CGFloat)h;
- (void)setTopCollapsedMinHeight:(CGFloat)h;
- (void)setBottomExpandedMinHeight:(CGFloat)h;
- (void)setBottomCollapsedMinHeight:(CGFloat)h;

- (void)collapseBottomOfSplitView:(NSSplitView *)split;
- (void)collapseTopOfSplitView:(NSSplitView *)split;
- (void)expandBottomOfSplitView:(NSSplitView *)split height:(CGFloat)h;

@end
