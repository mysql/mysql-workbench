/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#import "WBSplitPanel.h"
#import "WBTabView.h"
#import "WBSplitView.h"
#import "WBSplitViewUnbrokenizerDelegate.h"
#import "MContainerView.h"

#define MODEL_SPLIT_MIN_HEIGHT 28

@interface WBSplitPanel ()
{
  IBOutlet __weak WBSplitView *mainSplitView;
  IBOutlet __weak WBSplitViewUnbrokenizerDelegate *mainSplitViewDelegate;
  IBOutlet __weak NSView *topContainer;

  NSMutableDictionary *_editorById;
  float _lastEditorTabHeight;
  NSTimeInterval _lastClick;
}
@end

@implementation WBSplitPanel

- (instancetype) init
{
  self = [super init];
  if (self != nil) 
  {
    _editorById = [[NSMutableDictionary alloc] init];
  }
  return self;
}

- (void)awakeFromNib
{
  [mainSplitViewDelegate setTopExpandedMinHeight: 100];
  [mainSplitViewDelegate setTopCollapsedMinHeight: MODEL_SPLIT_MIN_HEIGHT];
  [mainSplitViewDelegate setBottomExpandedMinHeight: 100];
  [mainSplitViewDelegate setBottomCollapsedMinHeight: MODEL_SPLIT_MIN_HEIGHT];
  
  [mainSplitViewDelegate setBottomCollapseLimit: 80];
  
  [self tabViewDidChangeNumberOfTabViewItems: editorTabView];
}


- (void)addEditor:(WBBasePanel*)editor
{
  id tabItem = [[NSTabViewItem alloc] initWithIdentifier: editor.panelId];
  
  _editorById[editor.panelId] = editor;
  
  [tabItem setView:editor.topView];
  [tabItem setLabel:editor.title];
  
  [self adjustEditorTabViewForNewPanel: editor];
  
  [editorTabView addTabViewItem: tabItem];
  [editorTabView selectLastTabViewItem: nil];
    
  NSImage *icon= editor.tabIcon;
  if (icon && [editorTabView respondsToSelector: @selector(setIcon:forTabViewItem:)])
    [(id)editorTabView setIcon:icon forTabViewItem:editor.panelId];

  SEL selector = NSSelectorFromString(@"didShow");
  if ([editor respondsToSelector: selector])
    ((void (*)(id, SEL))[editor methodForSelector: selector])(editor, selector);

}

- (BOOL)closeEditorWithIdentifier:(id)ident
{
  for (id item in editorTabView.tabViewItems)
  {
    if (!ident || [[item identifier] isEqualTo: ident])
    {
      [self closeEditor: _editorById[[item identifier]]];
      return YES;
    }
  }
  return NO;  
}

- (BOOL)hasEditorWithIdentifier:(id)ident
{
  for (id item in editorTabView.tabViewItems)
  {
    if ([[item identifier] isEqualTo: ident])
      return YES;
  }
  return NO;
}

- (BOOL)hasEditor:(WBBasePanel*)editor
{
  return [editorTabView indexOfTabViewItemWithIdentifier: editor.panelId] != NSNotFound;
}

- (BOOL)closeEditor:(WBBasePanel*)editor
{
  if (!editor.willClose)
    return NO;
    
  NSUInteger index= [editorTabView indexOfTabViewItemWithIdentifier: editor.panelId];
  if (index == NSNotFound)
    return NO;
  else
  {
    NSTabViewItem *item= [editorTabView tabViewItemAtIndex: index];

    
    _lastEditorTabHeight = NSHeight(editorTabView.superview.frame);
    [editorTabView removeTabViewItem: item];

  }
  [_editorById removeObjectForKey: editor.panelId];

  return YES;
}


- (WBBasePanel*)findPanelForView:(NSView*)view
{
  for (NSTabViewItem *item in editorTabView.tabViewItems)
  {
    if (item.view == view)
    {
      return _editorById[item.identifier];
    }
  }  
  return nil;
}

- (WBBasePanel*)findPanelForPluginType: (Class)klass
{
  for (NSTabViewItem *item in editorTabView.tabViewItems)
  {
    id editor = _editorById[item.identifier];
    SEL selector = NSSelectorFromString(@"pluginEditor");
    if ([editor respondsToSelector: selector])
    {
      id panel = ((id (*)(id, SEL))[editor methodForSelector: selector])(editor, selector);
      if ([panel isKindOfClass: klass])
        return editor;
    }
  }
  return nil;
}

- (BOOL)closeActiveEditorTab
{
  // check if the keyview is in the selected tab view
  id activeTab = editorTabView.selectedTabViewItem.view;
  id firstResponder = editorTabView.window.firstResponder;
  while (firstResponder)
  {
    if (firstResponder == activeTab)
      break;
    firstResponder = [firstResponder superview];
  }
  
  if (firstResponder)
  {
    WBBasePanel *panel= _editorById[editorTabView.selectedTabViewItem.identifier];
    
    [self closeEditor: panel];
    return YES;
  }
  return NO;
}

- (void)setTitle:(NSString*)title
        forPanel:(WBBasePanel*)panel
{
  NSInteger i;

  i = [editorTabView indexOfTabViewItemWithIdentifier: panel.panelId];
  if (i >= 0 && i != NSNotFound)
    [editorTabView tabViewItemAtIndex: i].label = title;
  else
    NSLog(@"Unknown panel %@", panel);
}

- (NSSize)minimumSizeForEditorTabView
{
  NSSize minSize= NSMakeSize(0, 100);
  
  for (NSTabViewItem *tab in editorTabView.tabViewItems)
  {
    WBBasePanel *panel= _editorById[tab.identifier];
    
    if ([panel respondsToSelector:@selector(minimumSize)])
    {
      NSSize msize= panel.minimumSize;
      
      minSize.width= MAX(minSize.width, msize.width);
      minSize.height= MAX(minSize.height, msize.height);
    }
  }
  
  minSize.height+= NSHeight(editorTabView.frame) - NSHeight(editorTabView.contentRect);
  
  return minSize;
}


- (void)adjustEditorTabViewForNewPanel:(WBBasePanel*)panel
{
  NSSize minimumSize= self.minimumSizeForEditorTabView;
  // check if bottom tabview has to be enlarged
  float minHeight= minimumSize.height;
  float heightDifference = NSHeight(editorTabView.superview.frame) - NSHeight(panel.topView.frame);
  float defaultHeightForPanel= NSHeight(panel.topView.frame) + heightDifference;
  float minHeightForNewPanel= panel.minimumSize.height + NSHeight(editorTabView.superview.frame) - NSHeight(editorTabView.contentRect);
  
  if (minHeight < minHeightForNewPanel)
    minHeight= minHeightForNewPanel;
  
  if (editorTabView.numberOfTabViewItems == 0)
  {
    if (minHeight < _lastEditorTabHeight)
      minHeight = _lastEditorTabHeight;
  }
  
  if (defaultHeightForPanel < minHeight)
    defaultHeightForPanel= minHeight;
    
  if (defaultHeightForPanel > NSHeight(editorTabView.superview.frame)
      || [mainSplitView isSubviewCollapsed: mainSplitView.subviews.lastObject])
  {
    [mainSplitView setPosition: NSHeight(mainSplitView.superview.frame) - defaultHeightForPanel - mainSplitView.dividerThickness
              ofDividerAtIndex: 0];
  }
  
  [mainSplitView adjustSubviews];
}

- (void)tabViewDidChangeNumberOfTabViewItems:(NSTabView *)tabView
{
  if (tabView == editorTabView)
  {        
    if (tabView.numberOfTabViewItems == 0)
    {
      // tabview got emptied, collapse the splitview
      //[mainSplitViewDelegate setBottomCollapsedMinHeight: 0];
      [mainSplitView setPosition:[mainSplitView maxPossiblePositionOfDividerAtIndex:0] ofDividerAtIndex:0];
      _lastEditorTabHeight= 0;
    }
  }
}


- (BOOL)tabView:(NSTabView *)tabView willCloseTabViewItem:(NSTabViewItem*)tabViewItem
{
  WBBasePanel *panel = _editorById[tabViewItem.identifier];

  return [self closeEditor: panel];
}


- (void) tabView: (NSTabView*) tabView 
draggedHandleAtOffset: (NSPoint) offset
{
  if (tabView == editorTabView)
  {
    NSPoint pos= [mainSplitView convertPoint: NSApp.currentEvent.locationInWindow fromView: nil];
    
    float position= pos.y - offset.y - mainSplitView.dividerThickness;
    [mainSplitView setPosition: position ofDividerAtIndex: 0];
  }
}


@end
