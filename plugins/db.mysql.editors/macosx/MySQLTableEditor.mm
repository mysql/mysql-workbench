/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

/**
 * The main controller for the MySQL Table Editor.
 */

#include "base/geometry.h"
#include "base/string_utilities.h"

#import "MySQLTableEditor.h"
#import "mysql_table_editor.h"
#import "db_object_helpers.h"
#import "NSString_extras.h"
#import "MCPPUtilities.h"
#import "MacTableEditorInformationSource.h"
#import "MacTableEditorColumnsInformationSource.h"

#import "MacTableEditorIndexColumnsInformationSource.h"
#import "MacTableEditorFKColumnsInformationSource.h"
#import "editor_table.h"
#import "MTextImageCell.h"
#import "GRTTreeDataSource.h"
#import "MResultsetViewer.h"
#import "MTabSwitcher.h"
#import "MColoredView.h"
#import "ScintillaView.h"
#import "MVerticalLayoutView.h"

#import "WBTabItem.h" // needed for WBCustomTabItemView

#import "mforms/../cocoa/MFView.h"
#import "DbPrivilegeEditorTab.h"
#include <mforms/view.h>

static NSString* shouldRaiseException = @"should raise exception";
static NSString* columnDragUTI = @"com.mysql.workbench.column";

extern const char* DEFAULT_CHARSET_CAPTION;
extern const char* DEFAULT_COLLATION_CAPTION;

@interface DbMysqlTableEditor () {
  IBOutlet __weak NSTabView* mEditorsTabView;
  IBOutlet __weak MTabSwitcher* mTabSwitcher;

  IBOutlet __weak NSBox *mHeaderView;
  IBOutlet __weak NSButton* mHeaderExpander;

  // Table
  IBOutlet __weak NSTextField* mTableName;
  IBOutlet __weak NSTextField* mSchemaName;
  IBOutlet __weak NSPopUpButton* mTableCollation;
  IBOutlet __weak NSPopUpButton* mTableCharset;
  IBOutlet __weak NSPopUpButton* mTableEngine;
  IBOutlet NSTextView* mTableComment;

  // Columns
  IBOutlet __weak WBCustomTabItemView* mEditorColumns;
  IBOutlet __weak NSSplitView* mColumnsSplitter;
  IBOutlet __weak NSTableView* mColumnsTable;
  IBOutlet __weak NSTextField* mColumnsName;
  IBOutlet __weak NSTextField* mDefaultLabel;
  IBOutlet __weak NSButton* mButtonGCVirtual;
  IBOutlet __weak NSButton* mButtonGCStored;

  IBOutlet __weak NSBox* mColumnsDetailsBox;
  IBOutlet __weak NSPopUpButton* mColumnsCollation;
  IBOutlet __weak NSPopUpButton* mColumnsCharset;
  IBOutlet NSTextView* mColumnsComment;
  IBOutlet __weak NSComboBox* mColumnsType;
  IBOutlet __weak NSTextField* mColumnsDefault;
  IBOutlet __weak NSButton* mColumnsFlagPK;
  IBOutlet __weak NSButton* mColumnsFlagNN;
  IBOutlet __weak NSButton* mColumnsFlagUNQ;
  IBOutlet __weak NSButton* mColumnsFlagBIN;
  IBOutlet __weak NSButton* mColumnsFlagUN;
  IBOutlet __weak NSButton* mColumnsFlagZF;
  IBOutlet __weak NSButton* mColumnsFlagAI;
  IBOutlet __weak NSButton* mColumnsFlagG;

  // for compact mode
  IBOutlet __weak NSBox* mColumnsDetailsBox2;
  IBOutlet __weak NSPopUpButton* mColumnsCollation2;
  IBOutlet __weak NSPopUpButton* mColumnsCharset2;
  IBOutlet NSTextView* mColumnsComment2;

  // Indices
  IBOutlet __weak WBCustomTabItemView* mEditorIndices;
  IBOutlet __weak NSTableView* mIndicesTable;
  IBOutlet __weak NSTableView* mIndexColumnsTable;
  IBOutlet __weak NSPopUpButton* mIndicesStorageTypes;
  IBOutlet __weak NSTextField* mIndicesBlockSize;
  IBOutlet __weak NSTextField* mIndicesParser;
  IBOutlet NSTextView* mIndicesComment;
  IBOutlet __weak NSBox* mIndicesDetailsBox;
  IBOutlet __weak NSButton* mIndicesVisibleCheckbox;

  // Foreigh Keys
  IBOutlet __weak WBCustomTabItemView* mEditorForeignKeys;
  IBOutlet __weak NSTableView* mFKTable;
  IBOutlet __weak NSTableView* mFKColumnsTable;
  IBOutlet __weak NSPopUpButton* mFKOnUpdate;
  IBOutlet __weak NSPopUpButton* mFKOnDelete;
  IBOutlet NSTextView* mFKComment;
  IBOutlet __weak NSBox* mFKDetailsBox;
  IBOutlet __weak NSButton* mFKModelOnly;
  IBOutlet __weak NSView* mFKWarningPanel;

  // Triggers
  IBOutlet __weak WBCustomTabItemView* mTriggerTabItem;

  // Partitioning
  IBOutlet __weak WBCustomTabItemView* mEditorPartitioning;
  IBOutlet __weak NSButton* mPartitionEnabledCheckbox;
  IBOutlet __weak NSPopUpButton* mPartitionPopup;
  IBOutlet __weak NSPopUpButton* mSubpartitionPopup;
  IBOutlet __weak NSTextField* mPartitionParametersTextField;
  IBOutlet __weak NSTextField* mSubPartitionParametersTextField;
  IBOutlet __weak NSTextField* mPartitionCountTextField;
  IBOutlet __weak NSTextField* mSubpartitionCountTextField;
  IBOutlet __weak NSButton* mPartitionManualCheckbox;
  IBOutlet __weak NSButton* mSubpartitionManualCheckbox;
  IBOutlet __weak NSOutlineView* mPartitionTable;

  // Options
  IBOutlet __weak WBCustomTabItemView* mEditorOptions;
  IBOutlet __weak NSPopUpButton* mOptionsPackKeys;
  IBOutlet __weak NSTextField* mOptionsTablePassword;
  IBOutlet __weak NSTextField* mOptionsAutoIncrement;
  IBOutlet __weak NSButton* mOptionsDelayKeyUpdates;
  IBOutlet __weak NSPopUpButton* mOptionsRowFormat;
  IBOutlet __weak NSPopUpButton* mOptionsBlockSize;
  IBOutlet __weak NSTextField* mOptionsAvgRowLength;
  IBOutlet __weak NSTextField* mOptionsMinRows;
  IBOutlet __weak NSTextField* mOptionsMaxRows;
  IBOutlet __weak NSButton* mOptionsUseChecksum;
  IBOutlet __weak NSTextField* mOptionsDataDirectory;
  IBOutlet __weak NSTextField* mOptionsIndexDirectory;
  IBOutlet __weak NSTextField* mOptionsUnionTables;
  IBOutlet __weak NSPopUpButton* mOptionsMergeMethod;

  // Inserts
  IBOutlet __weak NSView* mEditorInserts;

@private
  MySQLTableEditorBE* mBackEnd;
  MacTableEditorColumnsInformationSource* mColumnsDataSource;
  NSArray* mColumnTypes;
  MacTableEditorInformationSource* mIndicesDataSource;
  MacTableEditorIndexColumnsInformationSource* mIndexColumnsDataSource;
  MacTableEditorInformationSource* mFKDataSource;
  MacTableEditorFKColumnsInformationSource* mFKColumnsDataSource;
  GRTTreeDataSource* mPartitionsTreeDataSource;

  NSView* mUnusedColumnsDetailsBox;

  DbPrivilegeEditorTab* mPrivilegesTab;

  BOOL mDidAwakeFromNib;
}

@end

@implementation DbMysqlTableEditor

- (instancetype)initWithModule: (grt::Module*)module arguments: (const grt::BaseListRef&)args {
  self = [super initWithNibName: @"MySQLTableEditor" bundle: [NSBundle bundleForClass:self.class]];
  if (self != nil) {
    [self loadView];
    [self enablePluginDocking:mEditorsTabView];

    [self reinitWithArguments:args];
  }

  return self;
}

- (void)reinitWithArguments: (const grt::BaseListRef&)args {
  BOOL isReinit = mBackEnd != 0;

  [super reinitWithArguments:args];

  [[[mEditorInserts subviews] lastObject] removeFromSuperview];
  delete mBackEnd;

  db_mysql_TableRef table = db_mysql_TableRef::cast_from(args[0]);
  mBackEnd = new MySQLTableEditorBE(table);

  
  if (!isReinit) {
    [mColumnsSplitter setVertical: NO];

    GrtVersionRef version = mBackEnd->get_catalog()->version();
    [mIndicesComment setEditable:bec::is_supported_mysql_version_at_least(version, 5, 5)];
    [mIndicesVisibleCheckbox setEnabled: bec::is_supported_mysql_version_at_least(version, 8, 0, 0)];

    [mColumnsSplitter addSubview:mColumnsDetailsBox];
  }

  mColumnsDataSource =
    [[MacTableEditorColumnsInformationSource alloc] initWithListModel: mBackEnd->get_columns() tableBackEnd: mBackEnd];
  mIndicesDataSource = [[MacTableEditorInformationSource alloc] initWithListModel: mBackEnd->get_indexes()];

  mIndexColumnsDataSource =
    [[MacTableEditorIndexColumnsInformationSource alloc] initWithListModel: mBackEnd->get_indexes()->get_columns()
                                                              tableBackEnd: mBackEnd];

  mFKDataSource = [[MacTableEditorInformationSource alloc] initWithListModel: mBackEnd->get_fks()];

  mFKColumnsDataSource =
    [[MacTableEditorFKColumnsInformationSource alloc] initWithListModel: mBackEnd->get_fks()->get_columns()
                                                           tableBackEnd: mBackEnd];

  if (!mBackEnd->is_editing_live_object()) {
    NSInteger i;

    mEditorInserts = nsviewForView(mBackEnd->get_inserts_panel());

    if ((i = [mEditorsTabView indexOfTabViewItemWithIdentifier: @"inserts"]) == NSNotFound) {
      id item = [[NSTabViewItem alloc] initWithIdentifier: @"inserts"];
      [item setView: mEditorInserts];
      [item setLabel: @"Inserts"];
      [mEditorsTabView addTabViewItem: item];
      [mEditorInserts setAutoresizesSubviews: YES];
    } else {
      [[mEditorsTabView tabViewItemAtIndex: i] setView:mEditorInserts];
    }
  }

  // Populate popup menus.
  MFillPopupButtonWithStrings(mTableCharset, mBackEnd->get_charset_list());
  [[mTableCharset menu] insertItem: [NSMenuItem separatorItem] atIndex:0];
  [mTableCharset insertItemWithTitle: [NSString stringWithUTF8String:DEFAULT_CHARSET_CAPTION] atIndex:0];

  MFillPopupButtonWithStrings(mTableCollation, mBackEnd->get_charset_collation_list(DEFAULT_CHARSET_CAPTION));
  [[mTableCollation menu] insertItem: [NSMenuItem separatorItem] atIndex:0];
  [mTableCollation insertItemWithTitle: [NSString stringWithUTF8String:DEFAULT_COLLATION_CAPTION] atIndex:0];

  MFillPopupButtonWithStrings(mTableEngine, mBackEnd->get_engines_list());

  MFillPopupButtonWithStrings(mColumnsCharset, mBackEnd->get_charset_list());
  [[mColumnsCharset menu] insertItem: [NSMenuItem separatorItem] atIndex:0];
  [mColumnsCharset insertItemWithTitle: [NSString stringWithUTF8String:DEFAULT_CHARSET_CAPTION] atIndex:0];

  MFillPopupButtonWithStrings(mColumnsCollation, mBackEnd->get_charset_collation_list(DEFAULT_CHARSET_CAPTION));
  [[mColumnsCollation menu] insertItem: [NSMenuItem separatorItem] atIndex:0];
  [mColumnsCollation insertItemWithTitle: [NSString stringWithUTF8String:DEFAULT_COLLATION_CAPTION] atIndex:0];

  MFillPopupButtonWithStrings(mFKOnUpdate, mBackEnd->get_fk_action_options());
  MFillPopupButtonWithStrings(mFKOnDelete, mBackEnd->get_fk_action_options());

  // Create column type list.
  mColumnTypes = MArrayFromStringVector(((MySQLTableColumnsListBE*)mBackEnd->get_columns())->get_datatype_names());

  // Set up combo boxes in Partitioning tab.
  mPartitionsTreeDataSource = [[GRTTreeDataSource alloc] initWithTreeModel: mBackEnd->get_partitions()];
  [mPartitionTable setDataSource:mPartitionsTreeDataSource];
  [mPartitionTable setDelegate:mPartitionsTreeDataSource];
  NSTableColumn* column = [mPartitionTable tableColumnWithIdentifier: @"0"];
  MTextImageCell* imageTextCell2 = [MTextImageCell new];
  [column setDataCell:imageTextCell2];

  if (!mBackEnd->is_editing_live_object()) {
    NSUInteger index = [mEditorsTabView indexOfTabViewItemWithIdentifier: @"privileges"];
    if (index != NSNotFound)
      [mEditorsTabView removeTabViewItem: [mEditorsTabView tabViewItemAtIndex:index]];

    mPrivilegesTab = [[DbPrivilegeEditorTab alloc] initWithObjectEditor: mBackEnd];

    NSTabViewItem* item = [[NSTabViewItem alloc] initWithIdentifier: @"privileges"];
    [item setView: [mPrivilegesTab view]];
    [item setLabel: @"Privileges"];
    [mEditorsTabView addTabViewItem:item];
  }
  // Register a callback that will call [self refresh] when the edited object is
  // changed from somewhere else in the application.
  // Note: with ARC we need to bridge via void*. Otherwise we'd get a ref cycle.
  mBackEnd->set_refresh_ui_slot(std::bind(call_refresh, (__bridge void*)self));
  mBackEnd->set_partial_refresh_ui_slot(std::bind(call_partial_refresh, std::placeholders::_1, (__bridge void*)self));

  [self updateFKPlaceholder];
  {
    id view = mBackEnd->get_trigger_panel()->get_data();
    [mTriggerTabItem addSubview: view];
    [view setFrame: [mTriggerTabItem bounds]];
    [view setAutoresizesSubviews: YES];
    [(NSView*)view
      setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable | NSMinXEdge | NSMinYEdge | NSMaxXEdge | NSMaxYEdge];
  }

  // Update the GUI.
  [self refreshTableEditorGUI];

  //  mBackEnd->load_trigger_sql();
  mBackEnd->reset_editor_undo_stack();

  [self notifyObjectSwitched];
}

- (void)awakeFromNib {
  [mTabSwitcher setTabStyle: MEditorBottomTabSwitcher];

  // collapse header by default
  [mHeaderExpander setState: NSControlStateValueOff];
  [self toggleHeader: NO];

  // Store the min size specified in the .xib file.
  NSSize size = [[self view] frame].size;
  [self setMinimumSize:size];

  // Assemble all the separate editor views into the tab view.
  NSTabViewItem* item;

  item = [[NSTabViewItem alloc] initWithIdentifier: @"columns"];
  [item setView:mEditorColumns];
  [item setLabel: @"Columns"];
  [mEditorsTabView addTabViewItem:item];

  item = [[NSTabViewItem alloc] initWithIdentifier: @"indices"];
  [item setView:mEditorIndices];
  [item setLabel: @"Indexes"];
  [mEditorsTabView addTabViewItem:item];
  [mEditorIndices setBackgroundColor: [NSColor whiteColor]];

  item = [[NSTabViewItem alloc] initWithIdentifier: @"foreignkeys"];
  [item setView:mEditorForeignKeys];
  [item setLabel: @"Foreign Keys"];
  [mEditorsTabView addTabViewItem:item];

  item = [[NSTabViewItem alloc] initWithIdentifier: @"triggers"];
  [item setView: mTriggerTabItem];
  [item setLabel: @"Triggers"];
  [mEditorsTabView addTabViewItem:item];

  item = [[NSTabViewItem alloc] initWithIdentifier: @"partitioning"];
  [item setView:mEditorPartitioning];
  [item setLabel: @"Partitioning"];
  [mEditorsTabView addTabViewItem:item];

  item = [[NSTabViewItem alloc] initWithIdentifier: @"options"];
  NSScrollView* sv = [[NSScrollView alloc] initWithFrame: [mEditorColumns frame]];
  [sv setDocumentView:mEditorOptions];
  [sv setHasHorizontalScroller: YES];
  [sv setHasVerticalScroller: YES];
  [[sv horizontalScroller] setControlSize:NSControlSizeSmall];
  [[sv verticalScroller] setControlSize:NSControlSizeSmall];
  [sv setAutohidesScrollers: YES];
  [mEditorOptions scrollRectToVisible:NSMakeRect(0, [mEditorOptions frame].size.height, 1, 1)];
  [item setView:sv];
  [item setLabel: @"Options"];
  [mEditorsTabView addTabViewItem:item];

  [mColumnsTable registerForDraggedTypes: @[ columnDragUTI ]];

  NSWindow *window = NSApplication.sharedApplication.mainWindow;
  [window addObserver: self forKeyPath: @"effectiveAppearance" options: 0 context: nil];
  [self updateColors];

  mDidAwakeFromNib = YES;
}

//--------------------------------------------------------------------------------------------------

- (void)observeValueForKeyPath: (NSString *)keyPath
                      ofObject: (id)object
                        change: (NSDictionary *)change
                       context: (void *)context {
  if ([keyPath isEqualToString: @"effectiveAppearance"]) {
    [self updateColors];
    return;
  }
  [super observeValueForKeyPath: keyPath ofObject: object change: change context: context];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)updateColors {
  NSWindow *window = NSApplication.sharedApplication.mainWindow;

  BOOL isDark = NO;
  if (@available(macOS 10.14, *)) {
    isDark = window.effectiveAppearance.name == NSAppearanceNameDarkAqua;
  }

  NSColor *backgroundColor = NSColor.windowBackgroundColor;
  mHeaderView.fillColor = backgroundColor;
  mTriggerTabItem.backgroundColor = backgroundColor;
  mEditorColumns.backgroundColor = backgroundColor;
  mEditorIndices.backgroundColor = backgroundColor;
  mEditorForeignKeys.backgroundColor = backgroundColor;
  mEditorPartitioning.backgroundColor = backgroundColor;
  mEditorOptions.backgroundColor = backgroundColor;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  [NSRunLoop cancelPreviousPerformRequestsWithTarget: self];

  NSWindow *window = NSApplication.sharedApplication.mainWindow;
  [window removeObserver: self forKeyPath: @"effectiveAppearance"];

  delete mBackEnd;
}

//--------------------------------------------------------------------------------------------------

#pragma mark Refreshing of views

// Set up values for the Table tab.
- (void)refreshTableEditorGUITableTab {
  if (mBackEnd != nil) {
    NSString* name = [NSString stringWithCPPString: mBackEnd->get_name()];
    [mTableName setStringValue:name];

    [mSchemaName setStringValue: [NSString stringWithCPPString: mBackEnd->get_schema_name()]];

    NSString* charset = [NSString stringWithCPPString: mBackEnd->get_table_option_by_name("CHARACTER SET")];
    if ([charset isEqualToString: @"DEFAULT"] || [charset length] == 0) {
      charset = [NSString stringWithUTF8String:DEFAULT_CHARSET_CAPTION];
    }
    {
      // DEBUG
      id item = [mTableCharset itemWithTitle:charset];
      if (item == nil) {
        NSLog(@"*** Warning: Table collation '%@' not found in menu.", charset);
      }
    }
    [mTableCharset selectItemWithTitle:charset];
    [self updateCollation: mTableCollation forCharset: [charset UTF8String]];

    NSString* collation = [NSString stringWithCPPString: mBackEnd->get_table_option_by_name("COLLATE")];
    if ([collation isEqualToString: @"DEFAULT"] || [collation length] == 0) {
      collation = [NSString stringWithUTF8String:DEFAULT_COLLATION_CAPTION];
    }
    {
      // DEBUG
      id item = [mTableCollation itemWithTitle:collation];
      if (item == nil) {
        NSLog(@"*** Warning: Table collation '%@' not found in menu.", collation);
      }
    }
    [mTableCollation selectItemWithTitle:collation];

    NSString* engine = [NSString stringWithCPPString: mBackEnd->get_table_option_by_name("ENGINE")];
    [mTableEngine selectItemWithTitle:engine];

    NSString* comments = [NSString stringWithCPPString: mBackEnd->get_comment()];
    [mTableComment setString:comments];
  }
}

// Set up values for the Columns tab.
- (void)refreshTableEditorGUIColumnsTab {
  [mColumnsTable reloadData];

  NSInteger rowIndex = [mColumnsTable selectedRow];
  if (rowIndex >= 0) {
    NSString* charset = [mColumnsDataSource objectValueForValueIndex:bec::TableColumnsListBE::Charset row: rowIndex];
    if ([charset isEqualToString: @"DEFAULT"] || [charset length] == 0) {
      charset = [NSString stringWithUTF8String:DEFAULT_CHARSET_CAPTION];
    }
    [mColumnsCharset selectItemWithTitle:charset];
    [self updateCollation:mColumnsCollation forCharset: [charset UTF8String]];

    NSString* collation =
      [mColumnsDataSource objectValueForValueIndex:bec::TableColumnsListBE::Collation row: rowIndex];
    if ([collation isEqualToString: @"DEFAULT"] || [collation length] == 0) {
      collation = [NSString stringWithUTF8String:DEFAULT_COLLATION_CAPTION];
    }

    [mColumnsCollation selectItemWithTitle:collation];
    NSString* collationEnabled =
      [mColumnsDataSource objectValueForValueIndex:bec::TableColumnsListBE::HasCharset row: rowIndex];
    [mColumnsCharset setEnabled: [collationEnabled isEqualToString: @"1"]];
    [mColumnsCollation setEnabled: [collationEnabled isEqualToString: @"1"]];

    NSString* columnName = [mColumnsDataSource objectValueForValueIndex:bec::TableColumnsListBE::Name row: rowIndex];
    [mColumnsDetailsBox setTitle: [NSString stringWithFormat: @"Column details '%@'", columnName]];
    [mColumnsName setStringValue:columnName];
    [mColumnsType
      setStringValue: [mColumnsDataSource objectValueForValueIndex:bec::TableColumnsListBE::Type row: rowIndex]];
    [mColumnsDefault
      setStringValue: [mColumnsDataSource objectValueForValueIndex:bec::TableColumnsListBE::Default row: rowIndex]];

    NSString* comments = [mColumnsDataSource objectValueForValueIndex:bec::TableColumnsListBE::Comment row: rowIndex];
    [mColumnsComment setString:comments];

    NSArray* flags = MArrayFromStringVector(mBackEnd->get_columns()->get_datatype_flags(rowIndex, true));
    ssize_t flag;
    mBackEnd->get_columns()->get_field(rowIndex, bec::TableColumnsListBE::IsPK, flag);
    [mColumnsFlagPK setState:flag ? NSControlStateValueOn : NSControlStateValueOff];
    mBackEnd->get_columns()->get_field(rowIndex, bec::TableColumnsListBE::IsNotNull, flag);
    [mColumnsFlagNN setState:flag ? NSControlStateValueOn : NSControlStateValueOff];
    mBackEnd->get_columns()->get_field(rowIndex, bec::TableColumnsListBE::IsUnique, flag);
    [mColumnsFlagUNQ setState:flag ? NSControlStateValueOn : NSControlStateValueOff];

    [mColumnsFlagBIN setEnabled: [flags containsObject: @"BINARY"]];
    [mColumnsFlagBIN setState: mBackEnd->get_columns()->get_column_flag(rowIndex, "BINARY") ? NSControlStateValueOn : NSControlStateValueOff];
    [mColumnsFlagUN setEnabled: [flags containsObject: @"UNSIGNED"]];
    [mColumnsFlagUN setState: mBackEnd->get_columns()->get_column_flag(rowIndex, "UNSIGNED") ? NSControlStateValueOn : NSControlStateValueOff];
    [mColumnsFlagZF setEnabled: [flags containsObject: @"ZEROFILL"]];
    [mColumnsFlagZF setState: mBackEnd->get_columns()->get_column_flag(rowIndex, "ZEROFILL") ? NSControlStateValueOn : NSControlStateValueOff];

    mBackEnd->get_columns()->get_field(rowIndex, MySQLTableColumnsListBE::IsAutoIncrementable, flag);
    [mColumnsFlagAI setEnabled:flag];
    mBackEnd->get_columns()->get_field(rowIndex, MySQLTableColumnsListBE::IsAutoIncrement, flag);
    [mColumnsFlagAI setState:flag ? NSControlStateValueOn : NSControlStateValueOff];
    mBackEnd->get_columns()->get_field(rowIndex, MySQLTableColumnsListBE::IsGenerated, flag);
    [mColumnsFlagG setState:flag ? NSControlStateValueOn : NSControlStateValueOff];
    mDefaultLabel.stringValue = flag ? @"Expression" : @"Default";

    mButtonGCStored.enabled = flag;
    mButtonGCVirtual.enabled = flag;
    if (flag) {
      std::string storageType;
      mBackEnd->get_columns()->get_field(rowIndex, MySQLTableColumnsListBE::GeneratedStorageType, storageType);
      if (base::toupper(storageType) != "STORED")
        mButtonGCVirtual.state = NSControlStateValueOn;
      else
        mButtonGCStored.state = NSControlStateValueOn;
    }
  }
}

- (void)refreshTableEditorGUIIndicesTab {
  id col = [mIndicesTable tableColumnWithIdentifier: @"type"];
  id cell = [col dataCell];
  MFillPopupButtonWithStrings(cell, mBackEnd->get_index_types());
  MFillPopupButtonWithStrings(mIndicesStorageTypes, mBackEnd->get_index_storage_types());

  [mIndicesTable reloadData];

  NSInteger rowIndex = [mIndicesTable selectedRow];
  if (rowIndex >= 0) {
    NSString* indexName = [mIndicesDataSource objectValueForValueIndex:MySQLTableIndexListBE::Name row: rowIndex];

    [mIndicesDetailsBox setTitle: [NSString stringWithFormat: @"Index details '%@'", indexName]];

    {
      mBackEnd->get_indexes()->select_index(rowIndex);
      NSString* storageType =
        [mIndicesDataSource objectValueForValueIndex:MySQLTableIndexListBE::StorageType row: rowIndex];
      [mIndicesStorageTypes selectItemWithTitle:storageType];
      NSString* blockSize =
        [mIndicesDataSource objectValueForValueIndex:MySQLTableIndexListBE::RowBlockSize row: rowIndex];
      [mIndicesBlockSize setStringValue:blockSize];
      NSString* parser = [mIndicesDataSource objectValueForValueIndex:MySQLTableIndexListBE::Parser row: rowIndex];
      [mIndicesParser setStringValue:parser];

      NSString* comment = [mIndicesDataSource objectValueForValueIndex:bec::IndexListBE::Comment row: rowIndex];
      [mIndicesComment setString:comment];

      GrtVersionRef version = mBackEnd->get_catalog()->version();
            
      if (bec::is_supported_mysql_version_at_least(version, 8, 0, 0)) {
        NSString* indexType = [mIndicesDataSource objectValueForValueIndex: MySQLTableIndexListBE::Type
                                                                       row: rowIndex];
      
        ssize_t visible = 1;
        if (![indexType isEqualToString:@"PRIMARY"]) {
          visible = [[mIndicesDataSource objectValueForValueIndex: bec::IndexListBE::Visible
                                                             row: rowIndex] intValue];
        
        }
        [mIndicesVisibleCheckbox setState: (visible == 1 ? NSControlStateValueOn: NSControlStateValueOff)];
        if ([indexType isEqualToString:@"PRIMARY"] || ([indexType isEqualToString:@"UNIQUE"] && mBackEnd->get_indexes()->count() == 2)) {
          mIndicesVisibleCheckbox.enabled = FALSE;
        } else {
          mIndicesVisibleCheckbox.enabled = TRUE;
        }
      }
            
      [mIndexColumnsTable reloadData];
    }
  }
}

- (void)refreshTableEditorGUIFKTab {
  [mIndicesTable reloadData];

  {
    // Setup the popup menu items for the referencing table column.
    id col = [mFKTable tableColumnWithIdentifier: @"referenced table"];
    NSPopUpButtonCell* cell = [col dataCell];
    MFillPopupButtonWithStrings((NSPopUpButton*)cell, mBackEnd->get_all_table_names());

    col = [mFKColumnsTable tableColumnWithIdentifier: @"referenced column"];
    cell = [col dataCell];
    NSInteger rowIndex = [mFKColumnsTable selectedRow];
    if (rowIndex >= 0)
      MFillPopupButtonWithStrings((NSPopUpButton*)cell,
                                  mBackEnd->get_fks()->get_columns()->get_ref_columns_list(rowIndex, false));
  }

  NSInteger rowIndex = [mFKTable selectedRow];

  if (rowIndex >= 0) {
    NSString* fkName = [mFKDataSource objectValueForValueIndex:bec::FKConstraintListBE::Name row: rowIndex];
    [mFKDetailsBox setTitle: [NSString stringWithFormat: @"Foreign key details '%@'", fkName]];

    mBackEnd->get_fks()->select_fk(rowIndex);

    NSInteger rowIndex = [mFKTable selectedRow];
    NSString* updateAction = [mFKDataSource objectValueForValueIndex:bec::FKConstraintListBE::OnUpdate row: rowIndex];
    [mFKOnUpdate selectItemWithTitle:updateAction];

    NSString* deleteAction = [mFKDataSource objectValueForValueIndex:bec::FKConstraintListBE::OnDelete row: rowIndex];
    [mFKOnDelete selectItemWithTitle:deleteAction];

    NSString* comment = [mFKDataSource objectValueForValueIndex:bec::FKConstraintListBE::Comment row: rowIndex];
    [mFKComment setString:comment];

    bool flag = false;
    mBackEnd->get_fks()->get_field(rowIndex, bec::FKConstraintListBE::ModelOnly, flag);
    [mFKModelOnly setState:flag ? NSControlStateValueOn : NSControlStateValueOff];

    [mFKColumnsTable reloadData];
  }
}

- (void)refreshTableEditorGUIPartitioningTab {
  [mPartitionTable reloadData];

  std::string prtn_type = mBackEnd->get_partition_type();
  NSString* partitionType = @(prtn_type.c_str());
  BOOL enabled = ([partitionType length] > 0);
  [mPartitionEnabledCheckbox setState: (enabled ? NSControlStateValueOn : NSControlStateValueOff)];
  [mPartitionEnabledCheckbox setEnabled: YES];

  // Enable or disable the first row of controls.
  [mPartitionPopup setEnabled:enabled];
  [mPartitionParametersTextField setEnabled:enabled];
  [mPartitionCountTextField setEnabled:enabled];
  [mPartitionManualCheckbox setEnabled:enabled];

  // Enable or disable the first popup on the second row of controls.
  //  NSString* partitionType = [mPartitionPopup titleOfSelectedItem];
  BOOL subEnabled = enabled && ([partitionType isEqualToString: @"RANGE"] || [partitionType isEqualToString: @"LIST"]);
  [mSubpartitionPopup setEnabled:subEnabled];

  {
    // Set up partitioning controls.
    [mPartitionPopup selectItemWithTitle:partitionType];

    std::string s = mBackEnd->get_partition_expression();
    NSString* partExpr = @(s.c_str());
    [mPartitionParametersTextField setStringValue:partExpr];

    int c = mBackEnd->get_partition_count();
    NSString* partCount = [NSString stringWithFormat: @"%d", c];
    [mPartitionCountTextField setStringValue:partCount];

    NSControlStateValue manualState = (mBackEnd->get_explicit_partitions() == true ? NSControlStateValueOn : NSControlStateValueOff);
    [mPartitionManualCheckbox setState:manualState];
  }

  {
    // Set up subpartitioning controls.
    std::string s = mBackEnd->get_subpartition_type();
    NSString* partType = @(s.c_str());
    if ([partType length] == 0) {
      partType = @"Disabled";
    }
    [mSubpartitionPopup selectItemWithTitle:partType];

    // Enable or disable the rest of the second row of controls.
    subEnabled = subEnabled && (!s.empty());
    [mSubPartitionParametersTextField setEnabled:subEnabled];
    [mSubpartitionCountTextField setEnabled:subEnabled];
    [mSubpartitionManualCheckbox setEnabled:subEnabled && ([mPartitionManualCheckbox state] == NSControlStateValueOn)];

    s = mBackEnd->get_subpartition_expression();
    NSString* partExpr = @(s.c_str());
    [mSubPartitionParametersTextField setStringValue:partExpr];

    int c = mBackEnd->get_subpartition_count();
    NSString* partCount = [NSString stringWithFormat: @"%d", c];
    [mSubpartitionCountTextField setStringValue:partCount];

    NSControlStateValue manualState = (mBackEnd->get_explicit_subpartitions() == true ? NSControlStateValueOn : NSControlStateValueOff);
    [mSubpartitionManualCheckbox setState:manualState];
  }

  // Enable or disable the table view.
  BOOL tabViewEnabled = (([mPartitionManualCheckbox isEnabled] && [mPartitionManualCheckbox state] == NSControlStateValueOn));
  [mPartitionTable setEnabled:tabViewEnabled];
}

- (void)refreshTableEditorGUIOptionsTab {
  // General options

  NSString* option = @(mBackEnd->get_table_option_by_name("PACK_KEYS").c_str());
  if ([option length] == 0)
    option = @"Default";
  [mOptionsPackKeys selectItemWithTitle:option];

  option = @(mBackEnd->get_table_option_by_name("PASSWORD").c_str());
  [mOptionsTablePassword setStringValue:option];

  option = @(mBackEnd->get_table_option_by_name("AUTO_INCREMENT").c_str());
  [mOptionsAutoIncrement setStringValue:option];

  option = @(mBackEnd->get_table_option_by_name("DELAY_KEY_WRITE").c_str());
  [mOptionsDelayKeyUpdates setState: ([option isEqualToString: @"1"] ? NSControlStateValueOn : NSControlStateValueOff)];

  // Row options

  option = @(mBackEnd->get_table_option_by_name("ROW_FORMAT").c_str());
  if ([option length] == 0)
    option = @"Default";
  [mOptionsRowFormat selectItemWithTitle: [option capitalizedString]];

  option = @(mBackEnd->get_table_option_by_name("KEY_BLOCK_SIZE").c_str());
  if ([option length] == 0)
    option = @"Default";
  else
    option = [NSString stringWithFormat: @"%@ KB", option];
  [mOptionsBlockSize selectItemWithTitle:option];

  option = @(mBackEnd->get_table_option_by_name("AVG_ROW_LENGTH").c_str());
  [mOptionsAvgRowLength setStringValue:option];

  option = @(mBackEnd->get_table_option_by_name("MIN_ROWS").c_str());
  [mOptionsMinRows setStringValue:option];

  option = @(mBackEnd->get_table_option_by_name("MAX_ROWS").c_str());
  [mOptionsMaxRows setStringValue:option];

  option = @(mBackEnd->get_table_option_by_name("CHECKSUM").c_str());
  [mOptionsUseChecksum setState: ([option isEqualToString: @"1"] ? NSControlStateValueOn : NSControlStateValueOff)];

  // Storage options

  option = @(mBackEnd->get_table_option_by_name("DATA DIRECTORY").c_str());
  [mOptionsDataDirectory setStringValue:option];

  option = @(mBackEnd->get_table_option_by_name("INDEX DIRECTORY").c_str());
  [mOptionsIndexDirectory setStringValue:option];

  // Merge table options

  option = @(mBackEnd->get_table_option_by_name("UNION").c_str());
  [mOptionsUnionTables setStringValue:option];

  option = @(mBackEnd->get_table_option_by_name("INSERT_METHOD").c_str());
  if ([option length] == 0)
    option = @"Don't Use";
  [mOptionsMergeMethod selectItemWithTitle: [option capitalizedString]];
}

- (void)refreshTableEditorGUITriggersTab {
  if (mBackEnd)
    mBackEnd->load_trigger_sql();
}

- (void)refreshTableEditorGUI {
  [self refreshTableEditorGUITableTab];
  [self refreshTableEditorGUIColumnsTab];
  [self refreshTableEditorGUIIndicesTab];
  [self refreshTableEditorGUIFKTab];
  [self refreshTableEditorGUITriggersTab];
  [self refreshTableEditorGUIPartitioningTab];
  [self refreshTableEditorGUIOptionsTab];
}

/**
 * Converts the current placeholder (the last line in the columns grid into a real column.
 */
- (void)activateColumnPlaceholder: (NSUInteger)rowIndex {
  // The following code is a bit involved, but it makes the tablew view
  // properly display the default PK column name and all its other settings.

  // Tell the backend we are editing now the placeholder row.
  [mColumnsDataSource setIntValue:1 forValueIndex:bec::TableColumnsListBE::Name row: rowIndex];

  // Get the default value for the name field...
  id value = [mColumnsDataSource objectValueForValueIndex:bec::TableColumnsListBE::Name row: rowIndex];

  // ... and set it in the backend. This way the backend will know next time
  // we set a value that we need a new place holder.
  [mColumnsDataSource setStringValue: value forValueIndex:bec::TableColumnsListBE::Name row: rowIndex];

  [mColumnsTable reloadData];
}

- (void)switchToColumnsTab {
  if (![mEditorsTabView.selectedTabViewItem.identifier isEqual: @"columns"]) {
    [mEditorsTabView selectTabViewItemWithIdentifier: @"columns"];

    [mColumnsDataSource refresh];

    NSInteger lastRowIndex = [self numberOfRowsInTableView:mColumnsTable] - 1;

    // Select the last row. There is always at least one row, because of the placeholder.
    if (lastRowIndex < 0)
      [mColumnsTable deselectAll: nil];
    else
      [mColumnsTable selectRowIndexes: [NSIndexSet indexSetWithIndex:lastRowIndex] byExtendingSelection: NO];
    if (lastRowIndex == 0) {
      // If there is *only* the placeholder then make it a normal row (which adds a new placeholder)
      // and set some default values for it. Start editing its name after that.
      [self activateColumnPlaceholder:0];
      [mColumnsTable editColumn:0 row:0 withEvent: nil select: YES];
    }
  }
}

#pragma mark Table view support

- (NSInteger)numberOfRowsInTableView: (NSTableView*)aTableView {
  NSInteger number;

  if (aTableView == mColumnsTable) {
    number = [mColumnsDataSource numberOfRowsInTableView:aTableView];
  } else if (aTableView == mIndicesTable) {
    number = [mIndicesDataSource numberOfRowsInTableView:aTableView];
  } else if (aTableView == mIndexColumnsTable) {
    number = [mIndexColumnsDataSource numberOfRowsInTableView:aTableView];
  } else if (aTableView == mFKTable) {
    number = [mFKDataSource numberOfRowsInTableView:aTableView];
  } else if (aTableView == mFKColumnsTable) {
    number = [mFKColumnsDataSource numberOfRowsInTableView:aTableView];
  } else {
    number = (mDidAwakeFromNib ? NSNotFound : 0);
  }

  NSAssert(number != NSNotFound, @"Mismatch in tables.");

  return number;
}

          - (id)tableView: (NSTableView*)aTableView
objectValueForTableColumn: (NSTableColumn*)aTableColumn
                      row: (NSInteger)rowIndex {
  NSString* obj = shouldRaiseException;

  id identifier = aTableColumn.identifier;
  NSInteger valueIndex = NSNotFound;

  if (aTableView == mColumnsTable) {
    if ([identifier isEqual: @"name"]) {
      valueIndex = bec::TableColumnsListBE::Name;
    } else if ([identifier isEqual: @"type"]) {
      valueIndex = bec::TableColumnsListBE::Type;
    } else if ([identifier isEqual: @"primarykey"]) {
      valueIndex = bec::TableColumnsListBE::IsPK;
    } else if ([identifier isEqual: @"notnull"]) {
      valueIndex = bec::TableColumnsListBE::IsNotNull;
    } else if ([identifier isEqual: @"unique"]) {
      valueIndex = bec::TableColumnsListBE::IsUnique;
    } else if ([identifier isEqual: @"binary"]) {
      valueIndex = bec::TableColumnsListBE::IsBinary;
    } else if ([identifier isEqual: @"unsigned"]) {
      valueIndex = bec::TableColumnsListBE::IsUnsigned;
    } else if ([identifier isEqual: @"zerofill"]) {
      valueIndex = bec::TableColumnsListBE::IsZerofill;
    } else if ([identifier isEqual: @"autoincrement"]) {
      valueIndex = MySQLTableColumnsListBE::IsAutoIncrement;
    } else if ([identifier isEqual: @"generated"]) {
      valueIndex = MySQLTableColumnsListBE::IsGenerated;
    } else if ([identifier isEqual: @"default"]) {
      valueIndex = bec::TableColumnsListBE::Default;
    }

    NSAssert(valueIndex != NSNotFound, @"Mismatch in columns.");

    obj = [mColumnsDataSource objectValueForValueIndex: valueIndex row: rowIndex];
  } else if (aTableView == mIndicesTable) {
    if ([identifier isEqual: @"name"]) {
      valueIndex = MySQLTableIndexListBE::Name;
    } else if ([identifier isEqual: @"type"]) {
      valueIndex = MySQLTableIndexListBE::Type;
    }

    NSAssert(valueIndex != NSNotFound, @"Mismatch in columns.");

    obj = [mIndicesDataSource objectValueForValueIndex: valueIndex row: rowIndex];
  } else if (aTableView == mIndexColumnsTable) {
    if ([identifier isEqual: @"name"]) {
      valueIndex = bec::IndexColumnsListBE::Name;
    } else if ([identifier isEqual: @"#"]) {
      valueIndex = bec::IndexColumnsListBE::OrderIndex;
    } else if ([identifier isEqual: @"order"]) {
      valueIndex = bec::IndexColumnsListBE::Descending;
    } else if ([identifier isEqual: @"length"]) {
      valueIndex = bec::IndexColumnsListBE::Length;
    }

    NSAssert(valueIndex != NSNotFound, @"Mismatch in columns.");

    obj = [mIndexColumnsDataSource objectValueForValueIndex: valueIndex row: rowIndex];

    if (([identifier isEqual: @"length"]) && ([obj isEqual: @"0"])) {
      // Do not display zero.
      obj = @"";
    }
  }

  else if (aTableView == mFKTable) {
    if ([identifier isEqual: @"name"]) {
      obj = [mFKDataSource objectValueForValueIndex:bec::FKConstraintListBE::Name row: rowIndex];
    } else if ([identifier isEqual: @"referenced table"]) {
      obj = @"foo";
    }
  }

  else if (aTableView == mFKColumnsTable) {
    obj = @"foo";
  }

  NSAssert(obj != shouldRaiseException, @"No match for tableview.");

  return obj;
}

- (void)tableView: (NSTableView*)aTableView
  willDisplayCell: (id)aCell
   forTableColumn: (NSTableColumn*)aTableColumn
              row: (NSInteger)rowIndex {
  id identifier = aTableColumn.identifier;

  if (aTableView == mColumnsTable) {
    if ([identifier isEqual: @"name"]) {
      NSImage* img = [mColumnsDataSource iconAtRow: rowIndex];
      [aCell setImage:img];
      [aCell setPlaceholderString: @"<click to edit>"];
    }
  }

  else if (aTableView == mIndexColumnsTable) {
    if ([identifier isEqual: @"name"]) {
      id obj = [mIndexColumnsDataSource objectValueForValueIndex:bec::IndexColumnsListBE::Name row: rowIndex];
      [aCell setTitle:obj];

      BOOL yn = [mIndexColumnsDataSource rowEnabled: rowIndex];
      [aCell setState: (yn ? NSControlStateValueOn : NSControlStateValueOff)];
    } else if ([identifier isEqual: @"order"]) {
      id obj = [mIndexColumnsDataSource objectValueForValueIndex:bec::IndexColumnsListBE::Descending row: rowIndex];
      [aCell selectItemWithTitle: ([obj intValue] == 0 ? @"ASC" : @"DESC")];
    }
  }

  else if (aTableView == mIndicesTable) {
    if ([identifier isEqual: @"name"])
      [aCell setPlaceholderString: @"<click to edit>"];
    else if ([identifier isEqual: @"type"]) {
      NSString* title = [mIndicesDataSource objectValueForValueIndex:MySQLTableIndexListBE::Type row: rowIndex];
      [aCell selectItemWithTitle:title];
    }
  }

  else if (aTableView == mFKTable) {
    if ([identifier isEqual: @"name"])
      [aCell setPlaceholderString: @"<click to edit>"];
    else if ([identifier isEqual: @"referenced table"]) {
      NSString* title = [mFKDataSource objectValueForValueIndex:bec::FKConstraintListBE::RefTable row: rowIndex];
      [aCell selectItemWithTitle:title];
    }
  }

  else if (aTableView == mFKColumnsTable) {
    if ([identifier isEqual: @"name"]) {
      NSString* title =
        [mFKColumnsDataSource objectValueForValueIndex:bec::FKConstraintColumnsListBE::Column row: rowIndex];
      [aCell setTitle:title];
      BOOL yn = [mFKColumnsDataSource rowEnabled: rowIndex];
      [aCell setState: (yn ? NSControlStateValueOn : NSControlStateValueOff)];
    } else if ([identifier isEqual: @"referenced column"]) {
      MFillPopupButtonWithStrings((NSPopUpButton*)aCell,
                                  mBackEnd->get_fks()->get_columns()->get_ref_columns_list(rowIndex, false));

      NSString* title =
        [mFKColumnsDataSource objectValueForValueIndex:bec::FKConstraintColumnsListBE::RefColumn row: rowIndex];
      [aCell selectItemWithTitle:title];
    }
  }
}

- (bec::ListModel*)listModelForTableView: (NSTableView*)table {
  if (table == mColumnsTable)
    return mBackEnd->get_columns();
  else if (table == mIndicesTable)
    return mBackEnd->get_indexes();
  else if (table == mFKTable)
    return mBackEnd->get_fks();
  return 0;
}

- (void)tableViewSelectionDidChange: (NSNotification*)aNotification {
  id sender = [aNotification object];
  if (sender == mColumnsTable) {
    [self refreshTableEditorGUIColumnsTab];
  } else if (sender == mIndicesTable) {
    [self refreshTableEditorGUIIndicesTab];
  } else if (sender == mFKTable) {
    [self refreshTableEditorGUIFKTab];
  }
}

- (void)tableView: (NSTableView*)aTableView
   setObjectValue: (id)anObject
   forTableColumn: (NSTableColumn*)aTableColumn
              row: (NSInteger)rowIndex {
  BOOL shouldRefreshGUI = YES;

  id identifier = aTableColumn.identifier;

  if (aTableView == mColumnsTable) {
    NSString* value;
    NSInteger valueIndex;
    NSString* keepEditingTypeString = nil;
    bool isflag = false;

    if ([identifier isEqual: @"name"]) {
      valueIndex = bec::TableColumnsListBE::Name;
      value = anObject;
    } else if ([identifier isEqual: @"type"]) {
      valueIndex = bec::TableColumnsListBE::Type;
      value = anObject;
      if (([value length] >= 2) && ([[value substringFromIndex: [value length] - 2] isEqualToString: @"()"])) {
        keepEditingTypeString = value;
      }
    } else if ([identifier isEqual: @"primarykey"]) {
      valueIndex = bec::TableColumnsListBE::IsPK;
      value = ([anObject boolValue] ? @"1" : @"0");
      isflag = true;
    } else if ([identifier isEqual: @"notnull"]) {
      valueIndex = bec::TableColumnsListBE::IsNotNull;
      value = ([anObject boolValue] ? @"1" : @"0");
      isflag = true;
    } else if ([identifier isEqual: @"unique"]) {
      valueIndex = bec::TableColumnsListBE::IsUnique;
      value = ([anObject boolValue] ? @"1" : @"0");
      isflag = true;
    } else if ([identifier isEqual: @"autoincrement"]) {
      valueIndex = MySQLTableColumnsListBE::IsAutoIncrement;
      value = ([anObject boolValue] ? @"1" : @"0");
      isflag = true;
    } else if ([identifier isEqual: @"generated"]) {
      valueIndex = MySQLTableColumnsListBE::IsGenerated;
      value = ([anObject boolValue] ? @"1" : @"0");
      isflag = true;
    } else if ([identifier isEqual: @"binary"]) {
      valueIndex = bec::TableColumnsListBE::IsBinary;
      value = ([anObject boolValue] ? @"1" : @"0");
      isflag = true;
    } else if ([identifier isEqual: @"unsigned"]) {
      valueIndex = bec::TableColumnsListBE::IsUnsigned;
      value = ([anObject boolValue] ? @"1" : @"0");
      isflag = true;
    } else if ([identifier isEqual: @"zerofill"]) {
      valueIndex = bec::TableColumnsListBE::IsZerofill;
      value = ([anObject boolValue] ? @"1" : @"0");
      isflag = true;
    } else if ([identifier isEqual: @"default"]) {
      valueIndex = bec::TableColumnsListBE::Default;
      value = anObject;

      // select and edit first column of next row
      NSEvent* currentEvent = [NSApp currentEvent];
      if ([currentEvent type] == NSEventTypeKeyDown && [[currentEvent characters] characterAtIndex:0] == '\t' &&
          [mColumnsTable numberOfRows] > rowIndex) {
        [mColumnsTable selectRowIndexes: [NSIndexSet indexSetWithIndex: rowIndex + 1] byExtendingSelection: NO];
        [mColumnsTable editColumn:0 row: rowIndex + 1 withEvent: nil select: YES];
      }
    } else {
      valueIndex = NSNotFound;
      value = nil;
    }

    NSAssert(valueIndex != NSNotFound, @"DEBUG - Mismatch in columns table view.");

    if (isflag) {
      // If we currently edit the placeholder then convert it into a normal row
      // (adding thereby a new placeholder row), load default name and default type.
      NSInteger lastRowIndex = [self numberOfRowsInTableView:mColumnsTable] - 1;
      if (lastRowIndex == rowIndex)
        [self activateColumnPlaceholder: rowIndex];
      [mColumnsDataSource setIntValue: [value intValue] forValueIndex: (int)valueIndex row: rowIndex];
    } else {
      if (keepEditingTypeString == nil) {
        [mColumnsDataSource setStringValue: value forValueIndex: (int)valueIndex row: rowIndex];
      } else {
        [mColumnsTable editColumn:1 row: rowIndex withEvent: nil select: NO];
        NSText* ce = [mColumnsTable currentEditor];
        [ce setString:keepEditingTypeString];
        [ce setSelectedRange:NSMakeRange([keepEditingTypeString length] - 1, 0)];

        // Make sure the GUI does not refresh and reload table, which would cause the editing of the cell to end.
        shouldRefreshGUI = NO;
      }
    }

    if (shouldRefreshGUI) {
      [self refreshTableEditorGUIColumnsTab];
      [self refreshTableEditorGUIIndicesTab];
    }
  }

  else if (aTableView == mIndicesTable) {
    if ([identifier isEqual: @"name"]) {
      [mIndicesDataSource setStringValue:anObject forValueIndex:MySQLTableIndexListBE::Name row: rowIndex];
    } else if ([identifier isEqual: @"type"]) {
      NSUInteger menuItemIndex = [anObject intValue];
      NSPopUpButtonCell* cell = [aTableColumn dataCell];
      if (menuItemIndex < (NSUInteger)[[cell menu] numberOfItems]) {
        NSString* title = [[[cell menu] itemAtIndex:menuItemIndex] title];
        [mIndicesDataSource setStringValue:title forValueIndex:MySQLTableIndexListBE::Type row: rowIndex];
      }
    }
  }

  else if (aTableView == mIndexColumnsTable) {
    if ([identifier isEqual: @"name"]) {
      [mIndexColumnsDataSource setRow: rowIndex enabled: [anObject boolValue]];
      [mIndexColumnsTable reloadData];
    } else if ([identifier isEqual: @"#"]) {
      [mIndexColumnsDataSource setStringValue:anObject forValueIndex:bec::IndexColumnsListBE::OrderIndex row: rowIndex];
      [mIndexColumnsTable reloadData];
    } else if ([identifier isEqual: @"order"]) {
      NSUInteger menuItemIndex = [anObject intValue];
      [mIndexColumnsDataSource setIntValue:menuItemIndex
                             forValueIndex:bec::IndexColumnsListBE::Descending
                                       row: rowIndex];
    } else if ([identifier isEqual: @"length"]) {
      NSUInteger l = [anObject intValue];
      [mIndexColumnsDataSource setIntValue:l forValueIndex:bec::IndexColumnsListBE::Length row: rowIndex];
    }
  }

  else if (aTableView == mFKTable) {
    if ([identifier isEqual: @"name"]) {
      [mFKDataSource setStringValue:anObject forValueIndex:bec::FKConstraintListBE::Name row: rowIndex];
    } else if ([identifier isEqual: @"referenced table"]) {
      NSUInteger menuItemIndex = [anObject intValue];
      NSPopUpButtonCell* cell = [aTableColumn dataCell];
      if (menuItemIndex < (NSUInteger)[[cell menu] numberOfItems]) {
        NSString* title = [[[cell menu] itemAtIndex:menuItemIndex] title];
        [mFKDataSource setStringValue:title forValueIndex:bec::FKConstraintListBE::RefTable row: rowIndex];
      }
      [self refreshTableEditorGUIFKTab];
    }
  }

  else if (aTableView == mFKColumnsTable) {
    if ([identifier isEqual: @"name"]) {
      [mFKColumnsDataSource setRow: rowIndex enabled: ([anObject intValue] == 1 ? YES : NO)];
    } else if ([identifier isEqual: @"referenced column"]) {
      NSUInteger menuItemIndex = [anObject intValue];
      NSPopUpButtonCell* cell = [aTableColumn dataCell];
      if (menuItemIndex < (NSUInteger)[[cell menu] numberOfItems]) {
        NSString* title = [[[cell menu] itemAtIndex:menuItemIndex] title];
        [mFKColumnsDataSource setStringValue:title
                               forValueIndex:bec::FKConstraintColumnsListBE::RefColumn
                                         row: rowIndex];
      }

      [self refreshTableEditorGUIFKTab];
    }
  }

  else {
    NSAssert(NO, @"No match in tableView:setObjectValue:::.");
  }

  if (shouldRefreshGUI) {
    [aTableView reloadData];
  }
}

- (void)userDeleteSelectedRowInTableView: (NSTableView*)table {
  if (table == mColumnsTable) {
    NSInteger row = [table selectedRow];
    if (row >= 0) {
      // delete row
      mBackEnd->get_columns()->delete_node(row);
      [table noteNumberOfRowsChanged];
    }
  }
  if (table == mIndicesTable) {
    NSInteger row = [table selectedRow];
    if (row >= 0) {
      // delete row
      mBackEnd->get_indexes()->delete_node(row);
      [table noteNumberOfRowsChanged];
    }
  } else if (table == mFKTable) {
    NSInteger row = [table selectedRow];
    if (row >= 0) {
      // delete row
      mBackEnd->get_fks()->delete_node(row);
      [table noteNumberOfRowsChanged];
    }
  }
}

    - (BOOL)tableView: (NSTableView*)aTableView
shouldEditTableColumn: (NSTableColumn*)aTableColumn
                  row: (NSInteger)rowIndex {
  // Activate the placeholder row and set the default value if this is the name column.
  if (aTableView == mColumnsTable && rowIndex == [mColumnsDataSource numberOfRowsInTableView:aTableView] - 1) {
    NSString* columnName = [mColumnsDataSource objectValueForValueIndex:bec::TableColumnsListBE::Name row: rowIndex];
    if (columnName.length == 0) {
      // Mark this row as placeholder. This activates special handling in the backend
      // (e.g. default value generation).
      mBackEnd->get_columns()->set_field([mColumnsTable selectedRow], bec::TableColumnsListBE::Name, 1);
    }
  }
  return YES;
}

#pragma mark Table view drag n drop

- (BOOL)tableView: (NSTableView*)aTableView
  writeRowsWithIndexes: (NSIndexSet*)rowIndices
          toPasteboard: (NSPasteboard*)pboard {
  BOOL shouldStartDrag = NO;

  NSUInteger rowIndex = [rowIndices firstIndex];
  NSMutableDictionary* dict = [NSMutableDictionary dictionary];
  dict[@"rowIndex"] = @((int)rowIndex);

  if (aTableView == mColumnsTable) {
    [pboard declareTypes: @[ columnDragUTI ] owner:self];
    [pboard setPropertyList:dict forType:columnDragUTI];

    shouldStartDrag = YES;
  }

  if (shouldStartDrag) {
    [aTableView selectRowIndexes: [NSIndexSet indexSetWithIndex: rowIndex] byExtendingSelection: NO];
  }

  return shouldStartDrag;
}

- (NSDragOperation)tableView: (NSTableView*)aTableView
                validateDrop: (id<NSDraggingInfo>)info
                 proposedRow: (NSInteger)proposedRow
       proposedDropOperation: (NSTableViewDropOperation)operation {
  NSDragOperation op = NSDragOperationNone;

  NSPasteboard* pb = [info draggingPasteboard];
  NSDictionary* dict = nil;
  if (aTableView == mColumnsTable) {
    dict = [pb propertyListForType:columnDragUTI];
  }

  NSAssert((dict != nil), @"Drag flavour was not found.");

  NSInteger originatingRow = [dict[@"rowIndex"] intValue];
  if (((proposedRow < originatingRow) || (proposedRow > originatingRow + 1)) &&
      (proposedRow < [self numberOfRowsInTableView:aTableView])) {
    [aTableView setDropRow:proposedRow dropOperation:NSTableViewDropAbove];
    op = NSDragOperationMove;
  }

  return op;
}

- (BOOL)tableView: (NSTableView*)aTableView
       acceptDrop: (id<NSDraggingInfo>)info
              row: (NSInteger)dropRow
    dropOperation: (NSTableViewDropOperation)operation {
  BOOL didAccept = NO;

  NSPasteboard* pb = [info draggingPasteboard];
  NSInteger originatingRow = NSNotFound;

  if (aTableView == mColumnsTable) {
    NSDictionary* dict = [pb propertyListForType:columnDragUTI];
    NSAssert((dict != nil), @"Drag flavour was not found.");

    originatingRow = [dict[@"rowIndex"] intValue];

    if (dropRow > originatingRow)
      dropRow--;

    [mColumnsDataSource moveColumnAtRow:originatingRow toRow:dropRow];

    didAccept = YES;
  }

  if (didAccept) {
    [aTableView reloadData];
    // Select the dropped row in the table.
    [aTableView selectRowIndexes: [NSIndexSet indexSetWithIndex:dropRow] byExtendingSelection: NO];
  }

  return didAccept;
}

#pragma mark Combo box support

- (NSInteger)numberOfItemsInComboBoxCell: (NSComboBoxCell*)aComboBoxCell {
  return [mColumnTypes count];
}

- (id)comboBoxCell: (NSComboBoxCell*)aComboBoxCell objectValueForItemAtIndex: (NSInteger)index {
  return mColumnTypes[index];
}

- (NSString*)comboBoxCell: (NSComboBoxCell*)aComboBoxCell completedString: (NSString*)uncompletedString {
  NSString* upper = [uncompletedString uppercaseString];
  for (NSString* s in mColumnTypes) {
    if ([s hasPrefix:upper])
      return s;
  }
  return nil;
}

- (NSUInteger)comboBoxCell: (NSComboBoxCell*)aComboBoxCell indexOfItemWithStringValue: (NSString*)aString {
  NSString* upper = [aString uppercaseString];

  return [mColumnTypes indexOfObject:upper];
}

#pragma mark User interaction

// Show FK editor or disable it by showing a placeholder with info depending on the currently
// selected table engine.
- (void)updateFKPlaceholder {
  [mFKWarningPanel setHidden:!mBackEnd->is_editing_live_object() || mBackEnd->engine_supports_foreign_keys()];
}

- (IBAction)userPickPopup: (id)sender {
  NSString* popItemTitle = [sender titleOfSelectedItem];

  if (sender == mTableCharset) {
    [self updateCollation: mTableCollation forCharset: [popItemTitle UTF8String]];
    if ([popItemTitle isEqualToString: [NSString stringWithUTF8String:DEFAULT_CHARSET_CAPTION]])
      popItemTitle = @"DEFAULT";
    mBackEnd->set_table_option_by_name("CHARACTER SET", [popItemTitle UTF8String]);
  } else if (sender == mTableCollation) {
    if ([popItemTitle isEqualToString: [NSString stringWithUTF8String:DEFAULT_COLLATION_CAPTION]])
      popItemTitle = @"DEFAULT";
    mBackEnd->set_table_option_by_name("COLLATE", [popItemTitle UTF8String]);
  } else if (sender == mTableEngine) {
    mBackEnd->set_table_option_by_name("ENGINE", [popItemTitle UTF8String]);
    [self updateFKPlaceholder];
  } else if (sender == mColumnsCharset) {
    [self updateCollation:mColumnsCollation forCharset: [popItemTitle UTF8String]];
    NSInteger rowIndex = [mColumnsTable selectedRow];
    if ([popItemTitle isEqualToString: [NSString stringWithUTF8String:DEFAULT_CHARSET_CAPTION]])
      popItemTitle = @"DEFAULT";
    [mColumnsDataSource setStringValue:popItemTitle forValueIndex:bec::TableColumnsListBE::Charset row: rowIndex];
  } else if (sender == mColumnsCollation) {
    NSInteger rowIndex = [mColumnsTable selectedRow];
    if ([popItemTitle isEqualToString: [NSString stringWithUTF8String:DEFAULT_COLLATION_CAPTION]])
      popItemTitle = @"DEFAULT";
    [mColumnsDataSource setStringValue:popItemTitle forValueIndex:bec::TableColumnsListBE::Collation row: rowIndex];
  } else if (sender == mIndicesStorageTypes) {
    NSInteger rowIndex = [mIndicesTable selectedRow];
    NSString* storageType = popItemTitle;
    [mIndicesDataSource setStringValue:storageType forValueIndex:MySQLTableIndexListBE::StorageType row: rowIndex];
  }

  else if (sender == mPartitionPopup) {
    mBackEnd->set_partition_type([popItemTitle UTF8String]);
    [self refreshTableEditorGUIPartitioningTab];
  } else if (sender == mSubpartitionPopup) {
    if ([popItemTitle isEqualToString: @"Disabled"])
      popItemTitle = @"";
    mBackEnd->set_subpartition_type([popItemTitle UTF8String]);
    [self refreshTableEditorGUIPartitioningTab];
  }

  else if (sender == mFKOnUpdate) {
    NSInteger rowIndex = [mFKTable selectedRow];
    if (![mFKDataSource setStringValue:popItemTitle forValueIndex:bec::FKConstraintListBE::OnUpdate row: rowIndex]) {
      // If the backend rejected our change revert the popup button the old value.
      NSString* updateAction = [mFKDataSource objectValueForValueIndex:bec::FKConstraintListBE::OnUpdate row: rowIndex];
      [mFKOnUpdate selectItemWithTitle:updateAction];
    }
  } else if (sender == mFKOnDelete) {
    NSInteger rowIndex = [mFKTable selectedRow];
    if (![mFKDataSource setStringValue:popItemTitle forValueIndex:bec::FKConstraintListBE::OnDelete row: rowIndex]) {
      NSString* deleteAction = [mFKDataSource objectValueForValueIndex:bec::FKConstraintListBE::OnDelete row: rowIndex];
      [mFKOnDelete selectItemWithTitle:deleteAction];
    }
  }

  else if (sender == mOptionsPackKeys) {
    [[sender window] makeFirstResponder: sender];
    mBackEnd->set_table_option_by_name("PACK_KEYS", [popItemTitle UTF8String]);
  } else if (sender == mOptionsRowFormat) {
    [[sender window] makeFirstResponder: sender];
    mBackEnd->set_table_option_by_name("ROW_FORMAT", [popItemTitle UTF8String]);
  } else if (sender == mOptionsBlockSize) {
    std::string bsize;
    [[sender window] makeFirstResponder: sender];
    if ([sender indexOfSelectedItem] > 0)
      bsize = [[popItemTitle componentsSeparatedByString: @" "][0] UTF8String];
    mBackEnd->set_table_option_by_name("KEY_BLOCK_SIZE", bsize);
  } else if (sender == mOptionsMergeMethod) {
    [[sender window] makeFirstResponder: sender];
    if ([sender indexOfSelectedItem] == 0)
      popItemTitle = @""; // Reset to nothing to remove the option entirely.
    mBackEnd->set_table_option_by_name("INSERT_METHOD", [[popItemTitle uppercaseString] UTF8String]);
  } else {
    NSAssert1(NO, @"DEBUG - User selected unmatched popup menu. Sender: '%@'", sender);
  }
}

- (void)toggleHeader: (BOOL)flag {
  float collapsedHeight = 38;
  float expandedHeight = 130;
  NSRect rect = [[self view] frame];
  NSRect hrect = [mHeaderView frame];
  NSRect trect = [mEditorsTabView frame];

  if (flag)
    hrect.size.height = expandedHeight;
  else
    hrect.size.height = collapsedHeight;

  hrect.origin.y = NSMaxY(rect) - NSHeight(hrect) + 1;
  trect.size.height = NSHeight(rect) - NSMinY(trect) - NSHeight(hrect) + 1;
  [mHeaderView setFrame:hrect];
  [mEditorsTabView setFrame:trect];
  [[mTableComment enclosingScrollView] setHidden:!flag];

  [[mHeaderView viewWithTag:1]
    setFrame:flag ? NSMakeRect(7, NSHeight(hrect) - 48 - 9, 48, 48) : NSMakeRect(7, NSHeight(hrect) - 24 - 9, 24, 24)];

  for (id view in [mHeaderView subviews]) {
    if ([view tag] >= 100)
      [view setHidden:!flag];
  }
}

- (IBAction)userClickButton: (id)sender {
  if (sender == mColumnsFlagPK)
    mBackEnd->get_columns()->set_field([mColumnsTable selectedRow], bec::TableColumnsListBE::IsPK,
                                       [sender state] == NSControlStateValueOn);
  else if (sender == mColumnsFlagNN)
    mBackEnd->get_columns()->set_field([mColumnsTable selectedRow], bec::TableColumnsListBE::IsNotNull,
                                       [sender state] == NSControlStateValueOn);
  else if (sender == mColumnsFlagUNQ)
    mBackEnd->get_columns()->set_field([mColumnsTable selectedRow], bec::TableColumnsListBE::IsUnique,
                                       [sender state] == NSControlStateValueOn);
  else if (sender == mColumnsFlagBIN)
    mBackEnd->get_columns()->set_field([mColumnsTable selectedRow], bec::TableColumnsListBE::IsBinary,
                                       [sender state] == NSControlStateValueOn);
  else if (sender == mColumnsFlagUN)
    mBackEnd->get_columns()->set_field([mColumnsTable selectedRow], bec::TableColumnsListBE::IsUnsigned,
                                       [sender state] == NSControlStateValueOn);
  else if (sender == mColumnsFlagZF)
    mBackEnd->get_columns()->set_field([mColumnsTable selectedRow], bec::TableColumnsListBE::IsZerofill,
                                       [sender state] == NSControlStateValueOn);
  else if (sender == mColumnsFlagAI)
    mBackEnd->get_columns()->set_field([mColumnsTable selectedRow], MySQLTableColumnsListBE::IsAutoIncrement,
                                       [sender state] == NSControlStateValueOn);
  else if (sender == mColumnsFlagG)
    mBackEnd->get_columns()->set_field([mColumnsTable selectedRow], MySQLTableColumnsListBE::IsGenerated,
                                       [sender state] == NSControlStateValueOn);
  else if (sender == mPartitionEnabledCheckbox) {
    mBackEnd->set_partition_type(([mPartitionEnabledCheckbox state] == NSControlStateValueOn ? "HASH" : ""));
    [self refreshTableEditorGUIPartitioningTab];
  } else if (sender == mPartitionManualCheckbox) {
    mBackEnd->set_explicit_partitions(([sender state] == NSControlStateValueOn ? true : false));
    [self refreshTableEditorGUIPartitioningTab];
  } else if (sender == mSubpartitionManualCheckbox) {
    mBackEnd->set_explicit_subpartitions(([sender state] == NSControlStateValueOn ? true : false));
    [self refreshTableEditorGUIPartitioningTab];
  } else if (sender == mOptionsDelayKeyUpdates) {
    [[sender window] makeFirstResponder: sender];
    mBackEnd->set_table_option_by_name("DELAY_KEY_WRITE", ([sender state] == NSControlStateValueOn ? "1" : "0"));
  } else if (sender == mOptionsUseChecksum) {
    [[sender window] makeFirstResponder: sender];
    mBackEnd->set_table_option_by_name("CHECKSUM", ([sender state] == NSControlStateValueOn ? "1" : "0"));
  } else if (sender == mFKModelOnly) {
    if ([mFKTable selectedRow] >= 0)
      mBackEnd->get_fks()->set_field([mFKTable selectedRow], bec::FKConstraintListBE::ModelOnly,
                                     [sender state] == NSControlStateValueOn);
  } else if (sender == mHeaderExpander) {
    [self toggleHeader: [sender state] == NSControlStateValueOn];
  } else if (sender == mIndicesVisibleCheckbox) {
    mBackEnd->get_indexes()->set_field([mIndicesTable selectedRow], MySQLTableIndexListBE::Visible, [sender state] == NSControlStateValueOn);
  } else {
    NSAssert1(NO, @"DEBUG - User clicked unmatched button: '%@'", [sender title]);
  }
  [mColumnsTable setNeedsDisplay: YES];
}

- (IBAction)userClickRadioButton: (id)sender {
  if (sender == mButtonGCStored)
    mBackEnd->get_columns()->set_field([mColumnsTable selectedRow], MySQLTableColumnsListBE::GeneratedStorageType,
                                       "STORED");
  else
    mBackEnd->get_columns()->set_field([mColumnsTable selectedRow], MySQLTableColumnsListBE::GeneratedStorageType,
                                       "VIRTUAL");
}

/**
 * Called by Cocoa after every keystroke in a text field. This is used to update parts of
 * the UI at every keystroke.
 * TODO: reconsider this handling. Doing a full page refresh for evey keystroke is just nonsense.
 */
- (void)controlTextDidChange: (NSNotification*)aNotification {
  id sender = [aNotification object];

  if (sender == mPartitionCountTextField) {
    int val = [sender intValue];
    mBackEnd->set_partition_count(val);
    [self refreshTableEditorGUIPartitioningTab];
  } else if (sender == mSubpartitionCountTextField) {
    int val = [sender intValue];
    mBackEnd->set_subpartition_count(val);
    [self refreshTableEditorGUIPartitioningTab];
  }
}

/**
 * Called by Cocoa when keyboard focus leaves a text field.
 */
- (void)controlTextDidEndEditing: (NSNotification*)aNotification {
  [NSRunLoop cancelPreviousPerformRequestsWithTarget:self];

  id sender = [aNotification object];

  // For text fields in a table view this is always the table view itself, not any of the text fields.
  if (sender == mColumnsTable) {
    NSText* text = [aNotification userInfo][@"NSFieldEditor"];

    // We can use a generic call here because the order of the columns defined in the table view is the same
    // as that of the column type enum. If that ever changes we need to take care here too.
    if (text != nil && text.string != nil)
      mBackEnd->get_columns()->set_field([sender editedRow], [sender editedColumn], [text.string UTF8String]);

    return;
  } else if ([sender isKindOfClass: [NSTableView class]])
    return;

  if (sender == mTableName) {
    mBackEnd->set_name([[mTableName stringValue] UTF8String]);
    [self updateTitle: [self title]];
  } else if (sender == mIndicesBlockSize) {
    NSInteger rowIndex = [mIndicesTable selectedRow];
    NSString* blockSize = [mIndicesBlockSize stringValue];
    [mIndicesDataSource setStringValue:blockSize forValueIndex:MySQLTableIndexListBE::RowBlockSize row: rowIndex];
  } else if (sender == mIndicesParser) {
    NSInteger rowIndex = [mIndicesTable selectedRow];
    NSString* blockSize = [mIndicesParser stringValue];
    [mIndicesDataSource setStringValue:blockSize forValueIndex:MySQLTableIndexListBE::Parser row: rowIndex];
  }

  else if (sender == mFKTable) {
    ; // Ignore.
  }

  else if (sender == mPartitionParametersTextField) {
    NSString* partExpr = [sender stringValue];
    mBackEnd->set_partition_expression([partExpr UTF8String]);
    [self refreshTableEditorGUIPartitioningTab];
  } else if (sender == mPartitionCountTextField) {
    // Handle this case in -controlTextDidChange: for instant feedback table redraw.
    ;
  } else if (sender == mSubPartitionParametersTextField) {
    NSString* subpartExpr = [sender stringValue];
    mBackEnd->set_subpartition_expression([subpartExpr UTF8String]);
    [self refreshTableEditorGUIPartitioningTab];
  } else if (sender == mSubpartitionCountTextField) {
    // Handle this case in -controlTextDidChange: for instant feedback table redraw.
    ;
  }

  else if (sender == mOptionsTablePassword) {
    mBackEnd->set_table_option_by_name("PASSWORD", [[sender stringValue] UTF8String]);
  } else if (sender == mOptionsAutoIncrement) {
    mBackEnd->set_table_option_by_name("AUTO_INCREMENT", [[sender stringValue] UTF8String]);
  } else if (sender == mOptionsAvgRowLength) {
    mBackEnd->set_table_option_by_name("AVG_ROW_LENGTH", [[sender stringValue] UTF8String]);
  } else if (sender == mOptionsMinRows) {
    mBackEnd->set_table_option_by_name("MIN_ROWS", [[sender stringValue] UTF8String]);
  } else if (sender == mOptionsMaxRows) {
    mBackEnd->set_table_option_by_name("MAX_ROWS", [[sender stringValue] UTF8String]);
  } else if (sender == mOptionsDataDirectory) {
    mBackEnd->set_table_option_by_name("DATA DIRECTORY", [[sender stringValue] UTF8String]);
  } else if (sender == mOptionsIndexDirectory) {
    mBackEnd->set_table_option_by_name("INDEX DIRECTORY", [[sender stringValue] UTF8String]);
  } else if (sender == mOptionsUnionTables) {
    mBackEnd->set_table_option_by_name("UNION", [[sender stringValue] UTF8String]);
  } else if (sender == mColumnsName) {
    if ([mColumnsTable selectedRow] >= 0)
      mBackEnd->get_columns()->set_field([mColumnsTable selectedRow], bec::TableColumnsListBE::Name,
                                         [[sender stringValue] UTF8String]);
    [mColumnsTable setNeedsDisplay: YES];
  } else if (sender == mColumnsCharset || sender == mColumnsCharset2) {
    if ([mColumnsTable selectedRow] >= 0)
      mBackEnd->get_columns()->set_field([mColumnsTable selectedRow], bec::TableColumnsListBE::Charset,
                                         [[sender stringValue] UTF8String]);
    [mColumnsTable setNeedsDisplay: YES];
  } else if (sender == mColumnsCollation || sender == mColumnsCollation2) {
    if ([mColumnsTable selectedRow] >= 0)
      mBackEnd->get_columns()->set_field([mColumnsTable selectedRow], bec::TableColumnsListBE::Collation,
                                         [[sender stringValue] UTF8String]);
    [mColumnsTable setNeedsDisplay: YES];
  } else if (sender == mColumnsType) {
    if ([mColumnsTable selectedRow] >= 0)
      mBackEnd->get_columns()->set_field([mColumnsTable selectedRow], bec::TableColumnsListBE::Type,
                                         [[sender stringValue] UTF8String]);
    [mColumnsTable setNeedsDisplay: YES];
  } else if (sender == mColumnsDefault) {
    if ([mColumnsTable selectedRow] >= 0)
      mBackEnd->get_columns()->set_field([mColumnsTable selectedRow], bec::TableColumnsListBE::Default,
                                         [[sender stringValue] UTF8String]);
    [mColumnsTable setNeedsDisplay: YES];
  } else {
    NSAssert1(NO, @"DEBUG - Unknown text field: %@", sender);
  }
}

// Called by Cocoa when keyboard focus leaves a text view.
- (void)textDidEndEditing: (NSNotification*)aNotification;
{
  // First cancel any pending calls originationg in performSelector:withObject:afterDelay:.
  [NSRunLoop cancelPreviousPerformRequestsWithTarget:self];

  id sender = [aNotification object];

  if (sender == mTableComment) {
    [[aNotification object] breakUndoCoalescing];
    mBackEnd->set_comment([[mTableComment string] UTF8String]);
  } else if (sender == mColumnsComment || sender == mColumnsComment2) {
    [[aNotification object] breakUndoCoalescing];
    NSInteger rowIndex = [mColumnsTable selectedRow];
    [mColumnsDataSource setStringValue: [sender string] forValueIndex:bec::TableColumnsListBE::Comment row: rowIndex];
  } else if (sender == mIndicesComment) {
    [[aNotification object] breakUndoCoalescing];
    NSInteger rowIndex = [mIndicesTable selectedRow];
    [mIndicesDataSource setStringValue: [mIndicesComment string] forValueIndex:bec::IndexListBE::Comment row: rowIndex];
  } else if (sender == mFKComment) {
    [[aNotification object] breakUndoCoalescing];
    NSInteger rowIndex = [mFKTable selectedRow];
    [mFKDataSource setStringValue: [mFKComment string] forValueIndex:bec::FKConstraintListBE::Comment row: rowIndex];
  } else {
    NSAssert1(NO, @"DEBUG - Unknown text view: %@", sender);
  }
}

- (CGFloat)splitView: (NSSplitView*)splitView
  constrainMaxCoordinate: (CGFloat)proposedMax
             ofSubviewAt: (NSInteger)dividerIndex {
  if (splitView == mColumnsSplitter && ![splitView isVertical])
    return MIN(proposedMax, NSHeight([splitView frame]) - 150.0);
  return proposedMax;
}

- (void)updateCollation: (NSPopUpButton*)collationPopUp forCharset: (const char*)charset;
{
  MFillPopupButtonWithStrings(collationPopUp, mBackEnd->get_charset_collation_list(charset));
  [[collationPopUp menu] insertItem: [NSMenuItem separatorItem] atIndex:0];
  [collationPopUp insertItemWithTitle: [NSString stringWithUTF8String:DEFAULT_COLLATION_CAPTION] atIndex:0];
}

#pragma mark Super class overrides

- (id)panelId;
{
  // An identifier for this editor (just take the object id).
  return [NSString stringWithCPPString: mBackEnd->get_object().id()];
}

- (BOOL)matchesIdentifierForClosingEditor: (NSString*)identifier {
  return mBackEnd->should_close_on_delete_of([identifier UTF8String]);
}

- (void)pluginDidShow: (id)sender {
  [[[self view] window] makeFirstResponder: mTableName];
  [super pluginDidShow: sender];
}

- (BOOL)pluginWillClose: (id)sender {
  return [super pluginWillClose: sender];
}

- (bec::BaseEditor*)editorBE {
  return mBackEnd;
}

#pragma mark - static methods

// Callback to update the editor GUI to reflect changes in the backend.
static void call_refresh(void* theEditor) {
  DbMysqlTableEditor* editor = (__bridge DbMysqlTableEditor*)theEditor;

  // As it turns out, this call-back can be called from non-main threads.
  [editor performSelectorOnMainThread: @selector(refreshTableEditorGUI) withObject: nil waitUntilDone: YES];
}

static void call_partial_refresh(int what, void* theEditor) {
  DbMysqlTableEditor* editor = (__bridge DbMysqlTableEditor*)theEditor;

  switch (what) {
    case bec::TableEditorBE::RefreshColumnMoveUp: {
      NSTableView* columns = editor->mColumnsTable;
      NSInteger i = [columns selectedRow];
      [columns reloadData];
      [columns selectRowIndexes: [NSIndexSet indexSetWithIndex: i - 1] byExtendingSelection: NO];
      break;
    }
    case bec::TableEditorBE::RefreshColumnMoveDown: {
      NSTableView* columns = editor->mColumnsTable;
      NSInteger i = [columns selectedRow];
      [columns reloadData];
      [columns selectRowIndexes: [NSIndexSet indexSetWithIndex: i + 1] byExtendingSelection: NO];
      break;
    }
    default:
      call_refresh(theEditor);
      break;
  }
}

@end
