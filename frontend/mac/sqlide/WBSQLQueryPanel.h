/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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
#import "WBSidebarPanel.h"


#include "sqlide/wb_sql_editor_form.h"

@class MTogglePane;
@class MResultsetViewer;
@class MVerticalLayoutView;
@class MTabSwitcher;
@class WBSplitView;
@class WBSplitViewUnbrokenizerDelegate;
@class WBMiniToolbar;
@class GRTListDataSource;

namespace mforms
{
  class DockingPoint;
};

@interface WBSQLQueryPanel : WBSidebarPanel
{
  IBOutlet WBSplitView* mView;
  IBOutlet WBSplitView* mWorkView;

  IBOutlet WBMiniToolbar* mOutputToolbar;
  IBOutlet NSTabView* mOutputTabView;
  IBOutlet NSPopUpButton* mOutputSelector;
  IBOutlet NSTextView* mTextOutput;

  IBOutlet NSTableView* mMessagesTable;
  IBOutlet NSTableView* mHistoryTable;
  IBOutlet NSTableView* mHistoryDetailsTable;

  IBOutlet NSTabView* mUpperTabView;
  IBOutlet MTabSwitcher* mUpperTabSwitcher;

@private
  NSTimeInterval mLastClick;
  
  BOOL mQueryAreaOpen;
  BOOL mResultsAreaOpen;

  BOOL mExpandedMode;
  BOOL mRightPaneWasExpanded;
  BOOL mBottomPaneWasExpanded;
  
  NSMutableDictionary *mEditors;
  
  SqlEditorForm::Ref mBackEnd;

  mforms::DockingPoint *mDockingPoint;

  NSLock *mTextOutputLock;
  std::string mTextOutputBuffer;
  
  CGFloat lastOutputAreaHeight;
}

@property (readonly) SqlEditorForm::Ref backEnd;

- (IBAction)clearOutput:(id)sender;
- (IBAction)copyOutputEntry:(id)sender;
- (IBAction)handleMenuAction:(id)sender;

- (void)addEditor:(WBBasePanel*)editor;

- (instancetype)initWithBE:(const SqlEditorForm::Ref&)be NS_DESIGNATED_INITIALIZER;
- (void)setRightSidebar:(BOOL)flag;
- (void)flushOutputBuffer;

@end


