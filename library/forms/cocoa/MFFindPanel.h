/* 
 * Copyright (c) 2012, 2013, Oracle and/or its affiliates. All rights reserved.
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

#import <AppKit/AppKit.h>
#include "mforms/find_panel.h"

@interface MFFindPanel : NSView
{
@public
  mforms::FindPanel *mOwner;
  
  IBOutlet NSView *mFindPanelPlaceholder;
  IBOutlet NSSegmentedControl *mFindTypeSegmented;
  IBOutlet NSSearchField *mFindText;
  IBOutlet NSTextField *mReplaceText;
  IBOutlet NSTextField *mFindLabel;
  IBOutlet NSSegmentedControl *mFindSegmented;
  IBOutlet NSMenu *mSearchMenu;

  BOOL mMatchCase;
  BOOL mMatchWhole;
  BOOL mWrapAround;
  BOOL mUseRegex;
}

- (id)initWithOwner:(mforms::FindPanel*)owner;
- (IBAction)findActionClicked:(id)sender;
- (void)enableReplaceInFindPanel: (BOOL)flag;
- (NSView*)topView;
- (BOOL)findNext:(BOOL)backwards;
- (int)replaceAll;
- (void)focusFindPanel;
@end
