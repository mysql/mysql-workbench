
/*
 * Copyright (c) 2011, 2014, Oracle and/or its affiliates. All rights reserved.
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

#import <Cocoa/Cocoa.h>
#include "sqlide/sql_editor_be.h"
#import "WBBasePanel.h"

@class MTabSwitcher;
@class WBSQLQueryPanel;
@class MResultsetViewer;
@class WBSplitViewUnbrokenizerDelegate;
@class MVerticalLayoutView;

namespace mforms
{
  class DockingPoint;
  class DockingPointDelegate;
};

@interface WBQueryTab : WBBasePanel
{
  IBOutlet NSView *mPanel;
  IBOutlet NSSplitView *mSplitView;
  IBOutlet MVerticalLayoutView *mEditorHost;
  IBOutlet MTabSwitcher *mResultsTabSwitcher;
  IBOutlet NSTabView *mResultsTabView;
  IBOutlet WBSplitViewUnbrokenizerDelegate *mSplitViewDelegate;
  IBOutlet NSButton *mApplyButton;
  IBOutlet NSButton *mCancelButton;
  IBOutlet NSTextField *mInfoText;
  IBOutlet NSImageView *mInfoIcon;

@private
  int mLastResultTabViewCount;
  float mSplitterPosition;
  WBSQLQueryPanel *mOwner;
  Sql_editor::Ref mBackend;
  mforms::DockingPoint *mBottomDockingPoint;
  BOOL mSplitterUpdatePending;
  BOOL mQueryCollapsed;
}

- (id)initWithOwner:(WBSQLQueryPanel*)owner
            backEnd: (Sql_editor::Ref)backend;

- (void)setQueryCollapsed: (BOOL)flag;
- (IBAction)activateQueryArea: (id)sender;

- (Sql_editor::Ref)editorController;

- (MResultsetViewer*)selectedResultset;

- (void)updateResultsetTabs;
- (void)updateActiveRecordsetTitle;
- (void)removeRecordsetWithIdentifier:(id)identifier;

- (IBAction)actionButtonClicked:(id)sender;
@end
