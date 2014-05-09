/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
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

#import "WBPluginEditorBase.h"

@class DbPrivilegeEditorTab;
@class MacTableEditorInformationSource;
@class MacTableEditorColumnsInformationSource;
@class MacTableEditorIndexColumnsInformationSource;
@class MacTableEditorFKColumnsInformationSource;
@class GRTTreeDataSource;
@class MResultsetViewer;
@class MTabSwitcher;
@class WBCustomTabItemView;
@class MVerticalLayoutView;

class MySQLTableEditorBE;
class MySQLTablePartitionTreeBE;

@interface DbMysqlTableEditor : WBPluginEditorBase
{
  IBOutlet NSTabView* mEditorsTabView;
  IBOutlet MTabSwitcher* mTabSwitcher;
  
  IBOutlet NSView* mHeaderView;
  IBOutlet NSButton *mHeaderExpander;
  
  // Table
  IBOutlet NSTextField* mTableName;
  IBOutlet NSTextField* mSchemaName;
  IBOutlet NSPopUpButton* mTableCollation;
  IBOutlet NSPopUpButton* mTableEngine;
  IBOutlet NSTextView* mTableComment;
  
  // Columns
  IBOutlet WBCustomTabItemView* mEditorColumns;
  IBOutlet NSSplitView* mColumnsSplitter;
  IBOutlet NSTableView* mColumnsTable;
  IBOutlet NSTextField* mColumnsName;
  
  IBOutlet NSBox* mColumnsDetailsBox;
  IBOutlet NSPopUpButton* mColumnsCollation;
  IBOutlet NSTextView* mColumnsComment;
  IBOutlet NSComboBox* mColumnsType;
  IBOutlet NSTextField* mColumnsDefault;
  IBOutlet NSButton* mColumnsFlagPK;
  IBOutlet NSButton* mColumnsFlagNN;
  IBOutlet NSButton* mColumnsFlagUNQ;
  IBOutlet NSButton* mColumnsFlagBIN;
  IBOutlet NSButton* mColumnsFlagUN;
  IBOutlet NSButton* mColumnsFlagZF;
  IBOutlet NSButton* mColumnsFlagAI;
  
  // for compact mode
  IBOutlet NSBox* mColumnsDetailsBox2;
  IBOutlet NSPopUpButton* mColumnsCollation2;
  IBOutlet NSTextView* mColumnsComment2;
    
  // Indices
  IBOutlet WBCustomTabItemView* mEditorIndices;
  IBOutlet NSTableView* mIndicesTable;
  IBOutlet NSTableView* mIndexColumnsTable;
  IBOutlet NSPopUpButton* mIndicesStorageTypes;
  IBOutlet NSTextField* mIndicesBlockSize;
  IBOutlet NSTextField* mIndicesParser;
  IBOutlet NSTextView* mIndicesComment;
  IBOutlet NSBox* mIndicesDetailsBox;
  
  // Foreigh Keys
  IBOutlet WBCustomTabItemView* mEditorForeignKeys;
  IBOutlet NSTableView* mFKTable;
  IBOutlet NSTableView* mFKColumnsTable;
  IBOutlet NSPopUpButton* mFKOnUpdate;
  IBOutlet NSPopUpButton* mFKOnDelete;
  IBOutlet NSTextView* mFKComment;
  IBOutlet NSBox* mFKDetailsBox;
  IBOutlet NSButton* mFKModelOnly;
  IBOutlet NSView* mFKWarningPanel;

  // Triggers
  IBOutlet WBCustomTabItemView* mTriggerTabItem;
  
  // Partitioning
  IBOutlet WBCustomTabItemView* mEditorPartitioning;
  IBOutlet NSButton* mPartitionEnabledCheckbox;
  IBOutlet NSPopUpButton* mPartitionPopup;
  IBOutlet NSPopUpButton* mSubpartitionPopup;
  IBOutlet NSTextField* mPartitionParametersTextField;  
  IBOutlet NSTextField* mSubPartitionParametersTextField;  
  IBOutlet NSTextField* mPartitionCountTextField;
  IBOutlet NSTextField* mSubpartitionCountTextField;
  IBOutlet NSButton* mPartitionManualCheckbox;
  IBOutlet NSButton* mSubpartitionManualCheckbox;
  IBOutlet NSOutlineView* mPartitionTable;
  
  // Options
  IBOutlet WBCustomTabItemView* mEditorOptions;
  IBOutlet NSPopUpButton* mOptionsPackKeys;
  IBOutlet NSTextField* mOptionsTablePassword;
  IBOutlet NSTextField* mOptionsAutoIncrement;
  IBOutlet NSButton* mOptionsDelayKeyUpdates;
  IBOutlet NSPopUpButton* mOptionsRowFormat;
  IBOutlet NSPopUpButton* mOptionsBlockSize;
  IBOutlet NSTextField* mOptionsAvgRowLength;
  IBOutlet NSTextField* mOptionsMinRows;
  IBOutlet NSTextField* mOptionsMaxRows;
  IBOutlet NSButton* mOptionsUseChecksum;
  IBOutlet NSTextField* mOptionsDataDirectory;
  IBOutlet NSTextField* mOptionsIndexDirectory;
  IBOutlet NSTextField* mOptionsUnionTables;
  IBOutlet NSPopUpButton* mOptionsMergeMethod;

  // Inserts
  IBOutlet NSView* mEditorInserts;
  
  @private
  MResultsetViewer* mEditorInsertsController;

  MySQLTableEditorBE* mBackEnd;
  MacTableEditorColumnsInformationSource* mColumnsDataSource;
  NSArray* mColumnTypes;
  MacTableEditorInformationSource* mIndicesDataSource;
  MacTableEditorIndexColumnsInformationSource* mIndexColumnsDataSource;
  MacTableEditorInformationSource* mFKDataSource;
  MacTableEditorFKColumnsInformationSource* mFKColumnsDataSource;
  GRTTreeDataSource* mPartitionsTreeDataSource;
  
  NSView *mUnusedColumnsDetailsBox;
  
  DbPrivilegeEditorTab *mPrivilegesTab;
  
  BOOL mDidAwakeFromNib;
}

- (IBAction) userPickPopup: (id) sender;
- (IBAction) userClickButton: (id) sender;

- (NSInteger) numberOfRowsInTableView: (NSTableView*) aTableView;

@end


