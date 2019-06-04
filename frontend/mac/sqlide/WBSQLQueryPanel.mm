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

#include "base/geometry.h"
#include "base/string_utilities.h"

#import "WBSQLQueryPanel.h"
#import "GRTIconCache.h"
#import "MTabSwitcher.h"
#import "MVerticalLayoutView.h"
#import "WBTabView.h"
#import "WBSplitView.h"
#import "MCPPUtilities.h"
#import "MContainerView.h"
#import "WBSplitViewUnbrokenizerDelegate.h"
#import "WBMiniToolbar.h"
#import "GRTListDataSource.h"
#import "TabViewDockingDelegate.h"

#import "WBPluginPanel.h"
#import "WBPluginEditorBase.h"
#import "MSpinProgressCell.h"
#include "sqlide/wb_sql_editor_panel.h"
#include "sqlide/query_side_palette.h"
#include "mforms/toolbar.h"
#include "mforms/appview.h"

#import "mforms/../cocoa/MFView.h"
#import "mforms/../cocoa/MFMenu.h"

#include "mforms/toolbar.h"

#define MIN_SIDEBAR_WIDTH 100
#define MIN_OUTPUT_AREA_HEIGHT 80
#define MIN_INFO_BOX_HEIGHT 30

//----------------------------------------------------------------------------------------------------------------------

@interface ColorBallTextCell : NSTextFieldCell {
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation ColorBallTextCell

- (void)drawWithFrame: (NSRect)cellFrame inView :(NSView *)controlView {
  [self.backgroundColor set];
  
  NSAttributedString *text = self.attributedStringValue;
  NSSize textSize = [text size];
  
  cellFrame.origin.y += (cellFrame.size.height - cellFrame.size.width) / 2;
  cellFrame.size.height = cellFrame.size.width;
  
  cellFrame = NSInsetRect(cellFrame, 2, 2);
  
  [[NSBezierPath bezierPathWithOvalInRect:cellFrame] fill];
  
  cellFrame.origin.y += (cellFrame.size.height - textSize.height) / 2;
  [text drawInRect:cellFrame];
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface WBSQLQueryPanel () {
  __weak IBOutlet WBSplitView* mWorkView;

  __weak IBOutlet WBMiniToolbar* mOutputToolbar;
  __weak IBOutlet NSTabView* mOutputTabView;
  __weak IBOutlet NSPopUpButton* mOutputSelector;
  IBOutlet NSTextView* mTextOutput;

  __weak IBOutlet NSTableView* mMessagesTable;
  __weak IBOutlet NSTableView* mHistoryTable;
  __weak IBOutlet NSTableView* mHistoryDetailsTable;

  __weak IBOutlet NSTabView* mUpperTabView;
  __weak IBOutlet MTabSwitcher* mUpperTabSwitcher;

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

  NSMutableArray *nibObjects;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation WBSQLQueryPanel

@synthesize backEnd = mBackEnd;

//----------------------------------------------------------------------------------------------------------------------

#pragma mark Table View support

- (NSInteger) numberOfRowsInTableView: (NSTableView*) tableView {
  if (tableView == mMessagesTable) {
    if (mBackEnd)
      return mBackEnd->log()->count();
  } else if (tableView == mHistoryTable) {
    if (mBackEnd)
      return mBackEnd->history()->entries_model()->count();
  } else if (tableView == mHistoryDetailsTable) {
    if (mBackEnd && mHistoryTable.selectedRow >= 0)
      return mBackEnd->history()->details_model()->count();
  }

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tableView: (NSTableView *)tableView
  willDisplayCell: (id)cell
   forTableColumn: (NSTableColumn *)tableColumn
              row: (NSInteger)row {
  if (tableView == mMessagesTable) {
    if ([tableColumn.identifier isEqual: @"0"]) {
      ssize_t msgtype;
      mBackEnd->log()->get_field(row, 0, msgtype);
      if (msgtype == DbSqlEditorLog::BusyMsg) {
        [[cell progressIndicator] startAnimation: nil];
        [[cell progressIndicator] setHidden: NO];
      } else {
        [[cell progressIndicator] stopAnimation: nil];
        [[cell progressIndicator] setHidden: YES];
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

         - (id) tableView: (NSTableView*) aTableView
objectValueForTableColumn: (NSTableColumn*) aTableColumn
                      row: (NSInteger) rowIndex {
  if (aTableView == mMessagesTable) {
    std::string text;
    
    if ([aTableColumn.identifier isEqual: @"0"]){
      ssize_t msgtype;
      mBackEnd->log()->get_field(rowIndex, 0, msgtype);
      if (msgtype != DbSqlEditorLog::BusyMsg) {
        bec::IconId icon_id= mBackEnd->log()->get_field_icon(rowIndex, 0, bec::Icon16);
      
        if (icon_id != 0)
          return [[GRTIconCache sharedIconCache] imageForIconId:icon_id];
      }
      return nil;
    } else {
      mBackEnd->log()->get_field(rowIndex, aTableColumn.identifier.intValue, text);
      
      return [NSString stringWithCPPString: text];
    }
  } else if (aTableView == mHistoryTable) {
    std::string text;
    
    mBackEnd->history()->entries_model()->get_field(rowIndex, aTableColumn.identifier.intValue, text);
    
    return [NSString stringWithCPPString: text];
  } else if (aTableView == mHistoryDetailsTable) {
    std::string text;
    
    mBackEnd->history()->details_model()->get_field(rowIndex, aTableColumn.identifier.intValue, text);
    
    return [NSString stringWithCPPString: text];
  }

  return @"foo";
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)tableView:(NSTableView *)aTableView 
         toolTipForCell:(NSCell *)aCell
                   rect:(NSRectPointer)rect 
            tableColumn:(NSTableColumn *)aTableColumn 
                    row:(NSInteger)row
          mouseLocation:(NSPoint)mouseLocation
{
  int column;
  if (aTableView == mMessagesTable && ((column = aTableColumn.identifier.intValue) == 3 || column == 4))
  {
    std::string text = mBackEnd->log()->get_field_description(row, column);
    return [NSString stringWithCPPString: text];
  }     
  return nil;
}

- (void)tableViewSelectionDidChange:(NSNotification *)aNotification
{
  NSTableView *sender= aNotification.object;
  
  if (sender == mHistoryTable)
  {
    if (mHistoryTable.selectedRow >= 0)
      mBackEnd->history()->current_entry((int)mHistoryTable.selectedRow);
    [mHistoryDetailsTable reloadData];
  }
  else if (sender == mHistoryDetailsTable)
  {
  }
  else if (sender == mMessagesTable)
  {
    std::vector<int> selection;
    NSIndexSet *sel = mMessagesTable.selectedRowIndexes;
    if (sel.count > 0)
    {
      for (NSUInteger i = sel.firstIndex; i <= sel.lastIndex and i != NSNotFound; i = [sel indexGreaterThanIndex: i])
        selection.push_back((int)i);
    }
    mBackEnd->log()->set_selection(selection);
  }
}


- (CGFloat)tableView:(NSTableView *)tableView heightOfRow:(NSInteger)row
{
  if (tableView == mHistoryDetailsTable)
  {
    std::string text;
    mBackEnd->history()->details_model()->get_field(row, 0, text);
    size_t lines= 0;
    const char *ptr= text.c_str();
    
    do
    {
      ptr= strchr(ptr, '\n');
      lines++;
    } while (ptr++);

    return tableView.rowHeight * lines;
  }
  else
    return tableView.rowHeight;
}

- (std::string)selectedHistoryItemsAsString
{
  std::list<int> sel_indexes;
  NSIndexSet *iset = mHistoryDetailsTable.selectedRowIndexes;
  
  if (iset.count > 0)
    for (NSUInteger row = iset.firstIndex; row <= iset.lastIndex and row != NSNotFound; row = [iset indexGreaterThanIndex: row])
      sel_indexes.push_back((int)row);

  if (sel_indexes.empty() || mBackEnd->history()->current_entry() < 0)
    return "";
  
  std::string sql= mBackEnd->restore_sql_from_history(mBackEnd->history()->current_entry(), sel_indexes);

  return sql;
}


- (bec::ListModel*)listModelForTableView:(NSTableView*)table
{
  if (table == mHistoryTable)
    return mBackEnd->history()->entries_model().get();
  return 0;
}


- (void)activateHistoryDetailEntry:(id)sender
{
  std::string text, query;
  NSIndexSet *selection= mHistoryDetailsTable.selectedRowIndexes;
  
  for (NSUInteger row= selection.firstIndex; row != NSNotFound; row= [selection indexGreaterThanIndex:row])
  {
    mBackEnd->history()->details_model()->get_field(row, 1, text);
    query.append("\n").append(text);
  }

  SqlEditorPanel *editor = mBackEnd->active_sql_editor_panel();
  if (editor)
    editor->editor_be()->append_text(query);
}

static void set_busy_tab(int tab, void *thePanel)
{
  WBSQLQueryPanel *panel = (__bridge WBSQLQueryPanel *)thePanel;
  if (tab < 0)
    [panel->mUpperTabSwitcher setBusyTab: nil];
  else
    [panel->mUpperTabSwitcher setBusyTab: [panel->mUpperTabView tabViewItemAtIndex: tab]];
}

static void processTaskFinish(void *thePanel)
{
  WBSQLQueryPanel *panel = (__bridge WBSQLQueryPanel *)thePanel;
  [panel->mMessagesTable reloadData];

  // This will scroll the selection into view.
  [panel->mMessagesTable selectRowIndexes: [NSIndexSet indexSetWithIndex: panel->mMessagesTable.numberOfRows - 1]
                     byExtendingSelection: NO];

  if (panel->mBackEnd->exec_sql_error_count() > 0)
  {
    [panel->mOutputTabView selectTabViewItemWithIdentifier: @"actions"];
    [panel->mOutputSelector selectItemAtIndex: 0];
    panel->mBackEnd->show_output_area();
  }
}

- (void)refreshTable: (NSTableView*)table
{
  [table reloadData];
  [[table delegate] tableViewSelectionDidChange:[NSNotification notificationWithName: NSTableViewSelectionDidChangeNotification
                                                                              object: table]];
  if (table.selectedRow >= 0)
    [table scrollRowToVisible: table.selectedRow];
}

static int reloadTable(void *table, void *thePanel)
{
  WBSQLQueryPanel *panel = (__bridge WBSQLQueryPanel *)thePanel;
  if ([NSThread mainThread] == [NSThread currentThread])
    [panel refreshTable: (__bridge NSTableView *)table];
  else
  {
    [panel performSelectorOnMainThread: @selector(refreshTable:)
                            withObject: (__bridge NSTableView *)table
                         waitUntilDone: NO];
  }
  return 0;
}

static void addTextToOutput(const std::string &text, bool bring_to_front, void *thePanel)
{
  WBSQLQueryPanel *panel = (__bridge WBSQLQueryPanel *)thePanel;
  [panel->mTextOutputLock lock];
  panel->mTextOutputBuffer.append(text);
  [panel->mTextOutputLock unlock];

  if ([NSThread isMainThread])
  {
    [panel flushOutputBuffer];
    if (bring_to_front)
    {
      [panel->mOutputSelector selectItemAtIndex: 1];
      [panel->mOutputTabView selectTabViewItemWithIdentifier: @"text"];
    }
  }
  else
  {
    if (bring_to_front)
    {
      [panel->mOutputSelector performSelectorOnMainThread: @selector(selectItemWithTitle:)
                                               withObject: [panel->mOutputSelector itemTitleAtIndex: 1]
                                            waitUntilDone: NO];
      [panel->mOutputTabView performSelectorOnMainThread: @selector(selectTabViewItemWithIdentifier:)
                                              withObject:@"text"
                                           waitUntilDone: NO];
    }
    [panel performSelectorOnMainThread: @selector(flushOutputBuffer) withObject: nil waitUntilDone: NO];
  }
}

#pragma mark User actions

#define QUERY_AREA_EXPANDED_MIN_HEIGHT (100)
#define QUERY_AREA_COLLAPSED_MIN_HEIGHT (0)

- (void)addEditor: (WBBasePanel*)editor
{
  id identifier = editor.panelId;
  if ([editor isKindOfClass: [WBPluginPanel class]])
  {
    WBPluginEditorBase *peditor = ((WBPluginPanel*)editor).pluginEditor;
    [peditor enableLiveChangeButtons];
    [peditor setCompactMode: NO];
  }

  mEditors[identifier] = editor;
  NSTabViewItem *item = [[NSTabViewItem alloc] initWithIdentifier: identifier];
  item.view = editor.topView;
  item.label = editor.title;
  [mUpperTabView addTabViewItem: item];
  [mUpperTabView selectLastTabViewItem: nil];

  SEL selector = NSSelectorFromString(@"didShow");
  if ([editor respondsToSelector: selector])
    ((void (*)(id, SEL))[editor methodForSelector: selector])(editor, selector);
}

- (void)closeActiveEditorTab
{
  NSTabViewItem *item = mUpperTabView.selectedTabViewItem;
  if (item)
    [mUpperTabSwitcher closeTabViewItem: item];
}

#pragma mark Output

- (IBAction)activateCollectionItem:(id)sender
{
  if (sender == mOutputSelector)
    [mOutputTabView selectTabViewItemAtIndex: [sender indexOfSelectedItem]];
}


- (IBAction)copyOutputEntry:(id)sender
{
  std::string sql;
  
  if ([mOutputTabView.selectedTabViewItem.identifier isEqualTo: @"history"])
    sql = [self selectedHistoryItemsAsString];
  
  if (!sql.empty())
  {
    switch ([sender tag])
    {
      case 1:
      {
        SqlEditorPanel *editor = mBackEnd->active_sql_editor_panel();
        if (editor)
          editor->editor_be()->append_text(sql);
        break;
      }
      case 2:
      {
        SqlEditorPanel *editor = mBackEnd->active_sql_editor_panel();
        if (editor)
          editor->editor_be()->sql(sql.c_str());
        break;
      }
      case 3:
        [[NSPasteboard generalPasteboard] declareTypes: @[NSPasteboardTypeString] owner:nil];
        [[NSPasteboard generalPasteboard] setString: [NSString stringWithCPPString: sql] forType: NSPasteboardTypeString];
        break;
    }
  }
}

- (IBAction)clearOutput:(id)sender
{
  if ([mOutputTabView.selectedTabViewItem.identifier isEqualTo: @"text"])
  {
    mTextOutput.string = @"";
  }
  else
  {
    std::vector<size_t> sel;
    if (mHistoryTable.selectedRow >= 0)
    {
      sel.push_back(mHistoryTable.selectedRow);
      mBackEnd->history()->entries_model()->delete_entries(sel);
      [mHistoryTable reloadData];
      if (mHistoryTable.numberOfRows > 0)
      {
        [mHistoryTable selectRowIndexes: [NSIndexSet indexSetWithIndex: mHistoryTable.numberOfRows-1]
                   byExtendingSelection: NO];
      }
    }
  }
}


- (void)flushOutputBuffer
{
  [mTextOutputLock lock];

  if (!mTextOutputBuffer.empty())
  {
    NSRange range;
    range = NSMakeRange(mTextOutput.string.length, 0);
    [mTextOutput replaceCharactersInRange: range withString: @(mTextOutputBuffer.c_str())];
  
    range = NSMakeRange(mTextOutput.string.length, 0);
    [mTextOutput scrollRangeToVisible: range];
    
    [mTextOutput display];
  
    mTextOutputBuffer.clear();
  }

  [mTextOutputLock unlock];
}

#pragma mark Public getters + setters

- (id)panelId
{
  return [NSString stringWithFormat: @"dbquery%p", mBackEnd.get()];
}

- (NSImage*)tabIcon
{
  return [NSImage imageNamed: @"tab.sqlquery.16x16"];
}


- (bec::UIForm*)formBE
{
  return boost::get_pointer(mBackEnd);
}


#pragma mark Other Delegates

- (NSRect)splitView:(NSSplitView *)splitView
      effectiveRect:(NSRect)proposedEffectiveRect 
       forDrawnRect:(NSRect)drawnRect
   ofDividerAtIndex:(NSInteger)dividerIndex
{
  // if the divider is too thin, increase effective rect by 2px to make it less impossible to drag
  if (splitView.vertical)
  {
    if (proposedEffectiveRect.size.width < 2)
    {
      proposedEffectiveRect.origin.x -= 1;
      proposedEffectiveRect.size.width += 2;
    }
  }
  else
  {
    if (proposedEffectiveRect.size.height < 2)
    {
      proposedEffectiveRect.origin.y -= 1;
      proposedEffectiveRect.size.height += 2;
    }
  }
  return proposedEffectiveRect;
}


- (void)splitViewDidResizeSubviews:(NSNotification *)notification
{
  if (mHidingSidebar)
    return;

  if (notification.object == mWorkView)
  {
    BOOL newCollapseState = [mWorkView isSubviewCollapsed: mWorkView.subviews.lastObject];
    BOOL hidden = !mBackEnd->get_toolbar()->get_item_checked("wb.toggleOutputArea");
    
    if (newCollapseState != hidden)
    {
      bec::GRTManager::get()->set_app_option("DbSqlEditor:OutputAreaHidden", grt::IntegerRef(newCollapseState));
      mBackEnd->get_toolbar()->set_item_checked("wb.toggleOutputArea", !newCollapseState);
    }
    if (!newCollapseState)
    {
      int height = (int)NSHeight(mOutputTabView.superview.frame);
      if (height <= 0)
        height = MIN_OUTPUT_AREA_HEIGHT;
      bec::GRTManager::get()->set_app_option("DbSqlEditor:OutputAreaHeight",
                                            grt::IntegerRef(height));
    }
  }
  else
    [super splitViewDidResizeSubviews: notification];
}


- (BOOL)splitView:(NSSplitView *)splitView shouldAdjustSizeOfSubview:(NSView *)subview
{
  if (splitView == mWorkView)
  {
    if (subview == mOutputTabView.superview)
      return NO;
  }
  return [super splitView: splitView shouldAdjustSizeOfSubview: subview];
}


- (CGFloat)splitView:(NSSplitView *)splitView constrainMinCoordinate:(CGFloat)proposedMin ofSubviewAt:(NSInteger)dividerIndex
{
  if (splitView == mWorkView && dividerIndex == 0)
  {
    if (proposedMin < QUERY_AREA_COLLAPSED_MIN_HEIGHT)
      proposedMin = QUERY_AREA_COLLAPSED_MIN_HEIGHT;
  }
  return [super splitView: splitView constrainMinCoordinate: proposedMin ofSubviewAt: dividerIndex];
}

- (CGFloat)splitView:(NSSplitView *)splitView constrainMaxCoordinate:(CGFloat)proposedMax ofSubviewAt:(NSInteger)dividerIndex
{
  if (splitView == mWorkView)
  {
    return proposedMax > NSHeight(splitView.frame) - MIN_OUTPUT_AREA_HEIGHT ? NSHeight(splitView.frame) - MIN_OUTPUT_AREA_HEIGHT : proposedMax;
  }
  return [super splitView: splitView constrainMaxCoordinate: proposedMax ofSubviewAt: dividerIndex];
}

- (BOOL)splitView:(NSSplitView *)splitView canCollapseSubview:(NSView *)subview
{
  if (splitView == mWorkView && splitView.subviews.lastObject == subview)
    return YES;
  return [super splitView: splitView canCollapseSubview: subview];
}


- (void)hideOutputArea:(BOOL)hidden
{
  mHidingSidebar = YES;
  if (!hidden)
    [mWorkView setPosition: NSHeight(mWorkView.frame)-lastOutputAreaHeight ofDividerAtIndex: 0];
  else
  {
    lastOutputAreaHeight = NSHeight(mOutputTabView.superview.frame);
    [mWorkView setPosition: NSHeight(mWorkView.frame) ofDividerAtIndex: 0];
  }
  mHidingSidebar = NO;
}


- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
  if (tabView == mUpperTabView)
  {
    mDockingPoint->view_switched();

    // hide auxiliary sidebars if the administrator tab is active
    BOOL expanded_mode = NO;

    TabViewDockingPointDelegate* deleg = dynamic_cast<TabViewDockingPointDelegate*>(mDockingPoint->get_delegate());
    if (deleg)
    {
      mforms::AppView* av = deleg->appview_for_view(tabViewItem.view);
      if (av && av->get_form_context_name() == "Administrator")
        expanded_mode = YES;
    }

    if (expanded_mode != mExpandedMode)
    {
      if (expanded_mode)
      {
        mRightPaneWasExpanded = mBackEnd->get_toolbar()->get_item_checked("wb.toggleSecondarySidebar");
        mBottomPaneWasExpanded = mBackEnd->get_toolbar()->get_item_checked("wb.toggleOutputArea");

        [self hideSideBar: YES secondary: YES];
        [self hideOutputArea: YES];
      }
      else
      {
        [self hideSideBar: !mRightPaneWasExpanded secondary: YES];
        [self hideOutputArea: !mBottomPaneWasExpanded];
      }

      mExpandedMode = expanded_mode;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)tabView:(NSTabView *)tabView itemHasCloseButton:(NSTabViewItem *)item {
  return tabView == mUpperTabView && item != nil;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tabView:(NSTabView *)tabView didReorderTabViewItem: (NSTabViewItem *)item toIndex: (NSInteger)index {
  if (tabView == mUpperTabView)
  {
    SqlEditorPanel *editor = mBackEnd->sql_editor_panel((int)[tabView indexOfTabViewItem: item]);
    if (editor)
      mBackEnd->sql_editor_reordered(editor, (int)index);
  }
}


- (BOOL)tabView: (NSTabView *)tabView willCloseTabViewItem: (NSTabViewItem *)tabViewItem
{
  if (tabView == mUpperTabView)
  {
    TabViewDockingPointDelegate* deleg = dynamic_cast<TabViewDockingPointDelegate*>(mDockingPoint->get_delegate());
    mforms::AppView *appView = deleg->appview_for_view(tabViewItem.view);

    if (appView)
    {
      if (!appView->on_close())
        return false;
      appView->close();
      return true;
    }
    else
    {
      id editor = mEditors[tabViewItem.identifier];
      if ([editor respondsToSelector: @selector(willClose)])
        if (![editor willClose])
          return NO;
      [mEditors removeObjectForKey: tabViewItem.identifier];
    }
    return YES;
  }
  else
  {
    id ident = tabViewItem.identifier;
    if ([ident isEqual: @"history"] ||
        [ident isEqual: @"messages"])
      return NO;    
    return YES;
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (NSImage*)tabView:(NSTabView *)tabView iconForItem:(NSTabViewItem *)tabViewItem {
  if (tabView == mUpperTabView) {
    id tab = mEditors[tabViewItem.identifier];
    if ([tab respondsToSelector: @selector(tabIcon)])
      return [tab tabIcon];
    else {
      TabViewDockingPointDelegate* deleg = dynamic_cast<TabViewDockingPointDelegate*>(mDockingPoint->get_delegate());
      if (deleg) {
        mforms::AppView* av = deleg->appview_for_view(tabViewItem.view);
        if (av) {
          NSString *name;
          if (self.isDarkModeActive)
            name = [NSString stringWithFormat: @"tab_icon_%s_dark", av->get_form_context_name().c_str()];
          else
            name = [NSString stringWithFormat: @"tab_icon_%s_light", av->get_form_context_name().c_str()];
          NSImage *icon = [NSImage imageNamed: name];
          if (icon)
            return icon;
        }
      }
    }
  }

  return [NSImage imageNamed: @"tab_icon_plugin"];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString*)tabView:(NSTabView *)tabView toolTipForItem:(NSTabViewItem *)item
{
  if (tabView == mUpperTabView)
  {
    SqlEditorPanel *editor = mBackEnd->sql_editor_panel((int)[tabView indexOfTabViewItem: item]);
    if (editor)
      return [NSString stringWithCPPString: editor->filename()];
  }
  return nil;
}


- (void)tabView:(NSTabView *)tabView willDisplayMenu:(NSMenu *)menu forTabViewItem:(NSTabViewItem *)item
{
  if (tabView == mUpperTabView)
  {
    SqlEditorPanel *editor = mBackEnd->sql_editor_panel((int)[tabView indexOfTabViewItem: item]);
    if (editor)
    {
      if (!editor->filename().empty())
          [[menu itemWithTag: 60] setEnabled: YES];
      else
        [[menu itemWithTag: 60] setEnabled: NO];
    }
    else
      [[menu itemWithTag: 60] setEnabled: NO];
  }
}


- (IBAction)handleMenuAction:(id)sender
{
  NSInteger clicked_tab = [mUpperTabView indexOfTabViewItem: mUpperTabSwitcher.clickedItem];

  switch ([sender tag])
  {
    case 50: // new tab
      mBackEnd->handle_tab_menu_action("new_tab", (int)clicked_tab);
      break;
    case 51: // save tab
    {
      mBackEnd->handle_tab_menu_action("save_tab", (int)clicked_tab);
      break;
    }
    case 60: // copy path to clipboard
    {
      mBackEnd->handle_tab_menu_action("copy_path", (int)clicked_tab);
      break;
    }
  }
}

#pragma mark Create + destroy

- (instancetype)initWithBE: (const SqlEditorForm::Ref&)be
{
  self = [super init];
  if (self != nil && be)
  {
    BOOL outputAreaHidden;

    NSMutableArray *temp;
    if ([NSBundle.mainBundle loadNibNamed: @"WBSQLQueryPanel" owner: self topLevelObjects: &temp])
    {
      nibObjects = temp;

      // restore state of toolbar
      {
        mforms::ToolBar *toolbar = be->get_toolbar();
        toolbar->set_item_checked("wb.toggleOutputArea", !(outputAreaHidden = bec::GRTManager::get()->get_app_option_int("DbSqlEditor:OutputAreaHidden", 0)));
      }
      lastOutputAreaHeight = MAX(bec::GRTManager::get()->get_app_option_int("DbSqlEditor:OutputAreaHeight", 135), MIN_OUTPUT_AREA_HEIGHT);

      // Setup docking point for mUpperTabView.
      {
        mDockingPoint = mforms::manage(new mforms::DockingPoint(new TabViewDockingPointDelegate(mUpperTabView, MAIN_DOCKING_POINT), true));
        be->set_tab_dock(mDockingPoint);
      }
      mBackEnd= be;
      mBackEnd->log()->refresh_ui_signal.connect(std::bind(reloadTable, (__bridge void *)mMessagesTable, (__bridge void *)self));
      mBackEnd->history()->entries_model()->refresh_ui_signal.connect(std::bind(reloadTable, (__bridge void *)mHistoryTable, (__bridge void *)self));
      mBackEnd->history()->details_model()->refresh_ui_signal.connect(std::bind(reloadTable, (__bridge void *)mHistoryDetailsTable, (__bridge void *)self));

      mBackEnd->output_text_slot = std::bind(addTextToOutput, std::placeholders::_1, std::placeholders::_2, (__bridge void *)self);

      mBackEnd->post_query_slot = std::bind(processTaskFinish, (__bridge void *)self);

      mBackEnd->set_busy_tab = std::bind(set_busy_tab, std::placeholders::_1, (__bridge void *)self);

      mBackEnd->set_frontend_data((__bridge void *)self);
      mUpperTabSwitcher.tabStyle = MEditorTabSwitcher;
      [mUpperTabSwitcher setAllowTabReordering: YES];

      mTextOutputLock = [[NSLock alloc] init];

      NSFont *font = [NSFont fontWithName: @"AndaleMono"
                                     size: [NSFont smallSystemFontSize]];
      if (!font)
        font = [NSFont fontWithName: @"Monaco" size: [NSFont smallSystemFontSize]];
      if (font)
        mTextOutput.font = font;

      self.splitView.backgroundColor = [NSColor colorWithDeviceWhite: 128 / 255.0 alpha: 1.0];

      mMessagesTable.menu = nsmenuForMenu(mBackEnd->log()->get_context_menu());

      mHistoryDetailsTable.target = self;
      mHistoryDetailsTable.doubleAction = @selector(activateHistoryDetailEntry:);
      if (mHistoryTable.numberOfRows > 0)
      {
        [mHistoryTable selectRowIndexes: [NSIndexSet indexSetWithIndex: 0]
                   byExtendingSelection: NO];
      }

      // dock the backend provided schema sidebar and restore its width
      {
        mforms::View *sidebar_ = mBackEnd->get_sidebar();
        sidebar = nsviewForView(sidebar_);
        if (mSidebarAtRight)
          [self.topView addSubview: sidebar];
        else
          [self.topView addSubview: sidebar positioned: NSWindowBelow relativeTo: (self.topView).subviews.lastObject];
        [self.splitView adjustSubviews];
      }

      // dock the other sidebar
      {
        mforms::View *view = mBackEnd->get_side_palette();
        secondarySidebar = nsviewForView(view);
        if (view)
        {
          if (mSidebarAtRight)
            [self.topView addSubview: secondarySidebar positioned: NSWindowBelow relativeTo: (self.topView).subviews.lastObject];
          else
            [self.topView addSubview: secondarySidebar];
        }
        [self.splitView adjustSubviews];
      }

      [self restoreSidebarsFor: "DbSqlEditor" toolbar: be->get_toolbar()];

      // restore height of the output area
      if (outputAreaHidden)
        [mWorkView setPosition: NSHeight(mWorkView.frame) ofDividerAtIndex: 0];
      else
        [mWorkView setPosition: NSHeight(mWorkView.frame) - lastOutputAreaHeight ofDividerAtIndex: 0];

      mEditors = [[NSMutableDictionary alloc] init];

      mQueryAreaOpen = YES;
      mResultsAreaOpen = YES;

      NSProgressIndicator *indicator = [[NSProgressIndicator alloc] initWithFrame: NSMakeRect(0, 0, 10, 10)];
      indicator.controlSize = NSControlSizeSmall;
      indicator.style = NSProgressIndicatorSpinningStyle;
      [indicator setIndeterminate: YES];
      ((MSpinProgressCell*)[mMessagesTable tableColumnWithIdentifier: @"0"].dataCell).progressIndicator = indicator;
    }
  }
  return self;
}

- (instancetype)init
{
  return [self initWithBE: SqlEditorForm::Ref()];
}

- (SqlEditorForm::Ref)backEnd
{
  return mBackEnd;
}

- (void)setRightSidebar:(BOOL)flag
{
  mSidebarAtRight = flag;

  id view1 = (self.topView).subviews[0];
  id view2 = (self.topView).subviews[1];
  
  if (mSidebarAtRight)
  {
    if (view2 != sidebar)
    {
      [view1 removeFromSuperview];
      [self.topView addSubview: view1];
    }    
  }
  else
  {
    if (view1 != sidebar)
    {
      [view1 removeFromSuperview];
      [self.topView addSubview: view1];
    }
  }
}

- (BOOL)willClose
{
  for (id key in mEditors)
  {
    id editor = [mEditors valueForKey: key];
    if ([editor respondsToSelector: @selector(willClose)])
      if (![editor willClose])
        return NO;
  }
  return mBackEnd->can_close();
}

- (void)dealloc
{
  mBackEnd->close();
  mDockingPoint->release();
}

/**
 * Executes commands sent by the main form that should be handled here.
 */
- (void) performCommand: (const std::string) command
{
  if (command == "wb.toggleOutputArea")
  {
    BOOL hidden = !mBackEnd->get_toolbar()->get_item_checked(command);
    bec::GRTManager::get()->set_app_option("DbSqlEditor:OutputAreaHidden", grt::IntegerRef(hidden));
    [self hideOutputArea: hidden];
  }
  else if (command == "wb.next_query_tab")
  {
    if (mUpperTabView.selectedTabViewItem == mUpperTabView.tabViewItems.lastObject)
      [mUpperTabView selectFirstTabViewItem: nil];
    else
      [mUpperTabView selectNextTabViewItem:nil];
  }
  else if (command == "wb.back_query_tab")
  {
    if (mUpperTabView.selectedTabViewItem == mUpperTabView.tabViewItems[0])
      [mUpperTabView selectLastTabViewItem: nil];
    else
      [mUpperTabView selectPreviousTabViewItem:nil];
  }
  else
    [super performCommand: command];
}

@end


