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

#include "base/geometry.h"
#include "base/string_utilities.h"

#import "WBQueryTab.h"
#import "WBSQLQueryPanel.h"
#import "MTabSwitcher.h"
#import "MResultsetViewer.h"
#import "WBSplitViewUnbrokenizerDelegate.h"
#import "MVerticalLayoutView.h"
#import "TabViewDockingDelegate.h"

#import <mforms/../cocoa/MFNative.h>

#include <mforms/toolbar.h>
#include <mforms/code_editor.h>
#include <mforms/find_panel.h>
#include <mforms/appview.h>
#include <mforms/dockingpoint.h>

#include "objimpl/ui/mforms_ObjectReference_impl.h"

#include "sqlide/wb_sql_editor_form.h"
#include "sqlide/wb_sql_editor_result_panel.h"

@interface ResultTabViewItem : NSTabViewItem
{
  MResultsetViewer *viewer;
}

- (void)setRSViewer: (MResultsetViewer*)viewer;
- (MResultsetViewer*)RSViewer;

@end


@implementation ResultTabViewItem

- (void)setRSViewer: (MResultsetViewer*)aViewer
{
  [viewer autorelease];
  viewer = [aViewer retain];
}

- (MResultsetViewer*)RSViewer
{
  return viewer;
}

- (void) dealloc
{
  [viewer release];
  [super dealloc];
}

@end

@implementation WBQueryTab

- (NSView*)topView
{
  return mPanel;
}

- (NSString*)title
{
  SqlEditorForm::Ref be(mOwner.backEnd);
  return [NSString stringWithCPPString: be->sql_editor_caption(be->sql_editor_index(mBackend))];
}

- (id)identifier
{
  return [self description];
}

- (NSImage*)tabIcon
{
  return [NSImage imageNamed: @"tab_icon_db.query.QueryBuffer"];
}

- (Sql_editor::Ref)editorController
{
  return mBackend;
}

- (void)updateResultsetTabs
{
  SqlEditorForm::Ref backEnd = mOwner.backEnd;
  int editor_index = backEnd->sql_editor_index(mBackend);
  int n, rs_count = backEnd->recordset_count(editor_index);
  NSMutableArray *leftoverResults = [[[mResultsTabView tabViewItems] mutableCopy] autorelease];
  int non_rs_tabs = 0;
  
  for (NSTabViewItem *item in leftoverResults)
  {
    if ([item isKindOfClass: [ResultTabViewItem class]])
      break;
    non_rs_tabs++;
  }
  
  int last_added_tab = -1;
  for (n = 0; n < rs_count; ++n)
  {
    Recordset::Ref rset(backEnd->recordset(editor_index, n));
    long key = rset->key();
    
    NSInteger index = [mResultsTabView indexOfTabViewItemWithIdentifier: [NSString stringWithFormat: @"rset%li", key]];
    if (index == NSNotFound)
    {
      MResultsetViewer *rsview = [[[MResultsetViewer alloc] initWithRecordset: rset] autorelease];
      NSScrollView *gridScroll = [[rsview gridView] enclosingScrollView];
      [gridScroll setBorderType: NSNoBorder];
      
      ResultTabViewItem *tabViewItem= 
        [[[ResultTabViewItem alloc] initWithIdentifier: [NSString stringWithFormat:@"rset%li", key]] autorelease];

      backEnd->result_panel(rset)->dock_result_grid(mforms::manage(nativeContainerFromNSView(gridScroll)));
        
      [tabViewItem setView: nsviewForView(backEnd->result_panel(rset).get())];
      [tabViewItem setRSViewer: rsview];
      [tabViewItem setLabel: [NSString stringWithCPPString: rset->caption()]];
      
      [mResultsTabView insertTabViewItem: tabViewItem atIndex: n+non_rs_tabs];
      last_added_tab = n + non_rs_tabs;
      [rsview refresh];
    }
    else
    {
      [leftoverResults removeObject: [mResultsTabView tabViewItemAtIndex: index]];
    }
  }
  
  if (last_added_tab >= 0)
    [mResultsTabView selectTabViewItemAtIndex: last_added_tab];
  
  // remove tabs that are not used anymore
  for (id tab in leftoverResults)
  {
    if ([tab isKindOfClass: [ResultTabViewItem class]])
      [mResultsTabView removeTabViewItem: tab];
  }
}


- (MResultsetViewer*)selectedResultset
{
  NSTabViewItem *selectedItem= [mResultsTabView selectedTabViewItem];
  
  if ([selectedItem isKindOfClass: [ResultTabViewItem class]])
    return [(ResultTabViewItem*)selectedItem RSViewer];
  
  return nil;
}


- (void)updateSplitterPosition
{
  int tabCount = [mResultsTabView numberOfTabViewItems];

  if (tabCount == 0)
  {
    if (mLastResultTabViewCount > 0)
    mSplitterPosition = NSHeight([[mResultsTabView superview] frame]) + [mSplitView dividerThickness];
    [mSplitView setPosition: NSHeight([mSplitView frame]) - [mSplitView dividerThickness]
           ofDividerAtIndex: 0];
  }
  else
  {
    if (tabCount > 0 && mLastResultTabViewCount == 0 && !mQueryCollapsed)
    {
      [mSplitView setPosition: NSHeight([mSplitView frame]) - mSplitterPosition
             ofDividerAtIndex: 0];
    }
  }

  mLastResultTabViewCount = tabCount;
  mSplitterUpdatePending = NO;
}


- (void)splitViewDidResizeSubviews:(NSNotification *)notification
{
  if (!mSplitterUpdatePending && [mResultsTabView numberOfTabViewItems] > 0)
    mBackend->grtm()->set_app_option("DbSqlEditor:ResultSplitterPosition",
                                     grt::IntegerRef((int)(NSHeight([[mResultsTabView superview] frame]) + [mSplitView dividerThickness])));
}

- (void)updateActiveRecordsetTitle
{
  MResultsetViewer *rset = [self selectedResultset];
  if (rset)
  {
    [[mResultsTabView selectedTabViewItem] setLabel: [NSString stringWithCPPString: [rset recordset]->caption()]];
    [mResultsTabSwitcher setNeedsDisplay: YES];
    
    bool flag = [rset recordset]->has_pending_changes();
    [mApplyButton setEnabled: flag];
    [mCancelButton setEnabled: flag];    
  }
}

- (void)removeRecordsetWithIdentifier:(id)identifier
{
  NSInteger index = [mResultsTabView indexOfTabViewItemWithIdentifier: identifier];
  if (index != NSNotFound)
  {
    id item = [mResultsTabView tabViewItemAtIndex: index];
    if (item)
      [mResultsTabView removeTabViewItem: item];
  }  
}


- (IBAction)actionButtonClicked:(id)sender
{
  if ([sender tag] == 1) // apply
  {
    if ([[self selectedResultset] hasPendingChanges])
      [[self selectedResultset] recordset]->apply_changes();
  }
  else
  {
    if ([[self selectedResultset] hasPendingChanges])
      [[self selectedResultset] recordset]->rollback();
  }
}


- (void)setQueryCollapsed: (BOOL)flag
{
  mSplitterPosition = NSHeight([[mResultsTabView superview] frame]) + [mSplitView dividerThickness];
  [mSplitView setPosition: 100
         ofDividerAtIndex: 0];
  mQueryCollapsed = YES;
}


- (IBAction)activateQueryArea: (id)sender
{
  mBackend->get_editor_control()->focus();
}


- (void)embedFindPanel:(BOOL)show
{
  NSView *panel = nsviewForView(mBackend->get_editor_control()->get_find_panel());
  if (show)
  {
    if (![panel superview])
    {
      [mEditorHost addSubview: panel positioned: NSWindowBelow relativeTo: nsviewForView(mBackend->get_editor_control())];
      [mEditorHost tile];
    }
  }
  else
  {
    [panel removeFromSuperview];
    [mEditorHost tile];
    
    [[mPanel window] makeFirstResponder: nsviewForView(mBackend->get_editor_control())];
  }
}

static void embed_find_panel(mforms::CodeEditor *editor, bool show, WBQueryTab *self)
{
  [self embedFindPanel: show];
}

#pragma mark -
#pragma mark Resultset TabView delegates

- (BOOL)tabView:(NSTabView *)tabView
willCloseTabViewItem:(NSTabViewItem*)tabViewItem
{
  if ([tabViewItem isKindOfClass: [ResultTabViewItem class]])
  {
    if ([[(ResultTabViewItem*)tabViewItem RSViewer] recordset]->can_close(true))
      [[(ResultTabViewItem*)tabViewItem RSViewer] close];
    return NO;
  }
  else
  {
    TabViewDockingPointDelegate *deleg = dynamic_cast<TabViewDockingPointDelegate*>(mBottomDockingPoint->get_delegate());
    mforms::AppView *av = deleg->appview_for_view([tabViewItem view]);
    if (av)
      mBottomDockingPoint->close_view(av);
  }
  return YES;
}

- (BOOL)tabView:(NSTabView*)tabView itemHasCloseButton:(NSTabViewItem*)item
{
  return YES;
}

- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
  MResultsetViewer *rset= [self selectedResultset];
  SqlEditorForm::Ref backEnd = mOwner.backEnd;
  int index = backEnd->sql_editor_index(mBackend);
  if (index >= 0)
  {
    if (rset)
    {
      Recordset::Ref recordset([rset recordset]);
      backEnd->active_recordset(index, recordset);
      if (!recordset->is_readonly())
      {
        /*
        [mApplyButton sizeToFit];
        [mCancelButton sizeToFit];
        [mApplyButton setFrameSize: NSMakeSize(70, NSHeight([mApplyButton frame]))];
        [mCancelButton setFrameSize: NSMakeSize(70, NSHeight([mCancelButton frame]))];
         */

        [mApplyButton setHidden: NO];
        [mCancelButton setHidden: NO];
        [mInfoText setHidden: YES];
        [mInfoIcon setHidden: YES];
        
        bool flag = recordset->has_pending_changes();
        [mApplyButton setEnabled: flag];
        [mCancelButton setEnabled: flag];
      }
      else
      {
        [mApplyButton setHidden: YES];
        [mCancelButton setHidden: YES];
        [mInfoText setToolTip: [NSString stringWithCPPString: recordset->readonly_reason()]];
        [mInfoText setHidden: NO];
        [mInfoIcon setToolTip: [NSString stringWithCPPString: recordset->readonly_reason()]];
        [mInfoIcon setHidden: NO];
      }
    }
    else
    {
      backEnd->active_result_panel(index, SqlEditorResult::Ref());
      [mApplyButton setHidden: YES];
      [mCancelButton setHidden: YES];
      [mInfoText setHidden: YES];
      [mInfoIcon setHidden: YES];
    }
    [mResultsTabSwitcher tile];
  }
}

- (void)tabViewDidChangeNumberOfTabViewItems:(NSTabView *)aTabView
{
  if (!mSplitterUpdatePending)
  {
    mSplitterUpdatePending = YES;
    [self performSelector: @selector(updateSplitterPosition) withObject:nil afterDelay:0.1];
  }
}

- (BOOL)tabView:(NSTabView *)tabView willReorderTabViewItem:(NSTabViewItem *)item
{
  if (tabView == mResultsTabView)
  {
    int index = 0;
    // find the new relative index of the recordset
    for (NSTabViewItem *ti in [tabView tabViewItems])
    {
      if (ti == item)
        break;
      if ([ti isKindOfClass: [ResultTabViewItem class]])
        index++;
    }
    if ([item isKindOfClass: [ResultTabViewItem class]])
    {
      MResultsetViewer *rsviewer = [(ResultTabViewItem*)item RSViewer];
      if (rsviewer && [rsviewer recordset])
      {
        SqlEditorForm::Ref backEnd = mOwner.backEnd;
        if (!backEnd->recordset_reorder(backEnd->sql_editor_index(mBackend), [rsviewer recordset], index))
          return NO;
      }
    }
  }
  return YES;
}

#pragma mark init/dealloc

- (id)initWithOwner: (WBSQLQueryPanel*)owner
            backEnd: (Sql_editor::Ref)backend
{
  self = [super init];
  if (self)
  {
    mOwner = owner;
    mBackend = backend;
    
    [NSBundle loadNibNamed: @"WBQueryTab" owner: self];
    
    [mEditorHost setBackgroundColor: [NSColor colorWithDeviceWhite: 230 / 255.0 alpha: 1.0]];
    
    SqlEditorForm::Ref editorBE = owner.backEnd;

    {
      mBottomDockingPoint = mforms::manage(new mforms::DockingPoint(new TabViewDockingPointDelegate(mResultsTabView, RESULT_DOCKING_POINT), true));
      db_query_QueryEditorRef qeditor(db_query_QueryEditorRef::cast_from(mBackend->grtobj()));
      
      qeditor->resultDockingPoint(mforms_to_grt(qeditor.get_grt(), mBottomDockingPoint));
    }
    
    boost::shared_ptr<mforms::ToolBar> toolbar(editorBE->sql_editor_toolbar(editorBE->sql_editor_index(backend)));
    [mEditorHost addSubview: nsviewForView(toolbar.get()) 
                 positioned: NSWindowBelow
                 relativeTo: nil];
    
    mforms::CodeEditor* backend_editor = mBackend->get_editor_control();
    backend_editor->set_status_text("");
    backend_editor->set_show_find_panel_callback(boost::bind(embed_find_panel, _1, _2, self));

    NSView *codeEditor = nsviewForView(backend_editor);
    [mEditorHost addSubview: codeEditor];
    [codeEditor setFrame: NSMakeRect(0, 0, NSWidth([mEditorHost bounds]), NSHeight([mEditorHost bounds]))];
    [mEditorHost setAutoresizesSubviews: YES];
    [codeEditor setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable];
    
    
    [mResultsTabSwitcher setTabStyle: MEditorBottomTabSwitcher];
    [mResultsTabSwitcher setAllowTabReordering: YES];
    
    [mSplitViewDelegate setTopCollapsedMinHeight: 30];
    [mSplitViewDelegate setTopExpandedMinHeight: 30];
    [mSplitViewDelegate setBottomCollapsedMinHeight: 60];
    [mSplitViewDelegate setBottomExpandedMinHeight: 60];

    [mInfoText setStringValue: @"Read Only"];
    [mInfoText sizeToFit];

    mSplitterPosition = mBackend->grtm()->get_app_option_int("DbSqlEditor:ResultSplitterPosition", 200);

    mLastResultTabViewCount = -1;
    [self updateSplitterPosition];
    
    [mEditorHost setExpandSubviewsByDefault: NO];
    [(MVerticalLayoutView*)[mResultsTabSwitcher superview] setExpandSubviewsByDefault: YES];    
  }
  return self;
}


- (void)dealloc
{
  [NSObject cancelPreviousPerformRequestsWithTarget: self];
  mBottomDockingPoint->release();

  [mSplitViewDelegate release];
  [mPanel release];
  [super dealloc];
}
@end
