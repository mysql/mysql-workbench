/* 
 * Copyright (c) 2008, 2012, Oracle and/or its affiliates. All rights reserved.
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

#import "WBSplitPanel.h"
#import "WBTabView.h"

#define MODEL_SPLIT_MIN_HEIGHT 28

@implementation WBSplitPanel

- (id) init
{
  self = [super init];
  if (self != nil) 
  {
    _editorById = [[NSMutableDictionary alloc] init];
  }
  return self;
}


- (void) dealloc
{
  [_editorById release];
  [super dealloc];
}


- (void)awakeFromNib
{
  [mainSplitViewDelegate setTopExpandedMinHeight: 100];
  [mainSplitViewDelegate setTopCollapsedMinHeight: MODEL_SPLIT_MIN_HEIGHT];
  [mainSplitViewDelegate setBottomExpandedMinHeight: 100];
  [mainSplitViewDelegate setBottomCollapsedMinHeight: MODEL_SPLIT_MIN_HEIGHT];
  
  [mainSplitViewDelegate setBottomCollapseLimit: 80];
  
  [bottomContainer setMinContentSize: NSMakeSize(40, 40)];
  
  [self tabViewDidChangeNumberOfTabViewItems: editorTabView];
}


- (void)addEditor:(WBBasePanel*)editor
{
  id tabItem= [[[NSTabViewItem alloc] initWithIdentifier:[editor identifier]] autorelease];
  
  [_editorById setObject: editor forKey: [editor identifier]];
  
  [tabItem setView:[editor topView]];
  [tabItem setLabel:[editor title]];
  
  [self adjustEditorTabViewForNewPanel: editor];
  
  [editorTabView addTabViewItem:tabItem];
  [editorTabView selectLastTabViewItem:nil];
    
  NSImage *icon= [editor tabIcon];
  if (icon && [editorTabView respondsToSelector: @selector(setIcon:forTabViewItem:)])
    [(id)editorTabView setIcon:icon forTabViewItem:[editor identifier]];
  
  if ([editor respondsToSelector: @selector(didShow)])
    [editor performSelector: @selector(didShow)];
}

- (BOOL)closeEditorWithIdentifier:(id)ident
{
  for (id item in [editorTabView tabViewItems])
  {
    if (!ident || [[item identifier] isEqualTo: ident])
    {
      [self closeEditor: [_editorById objectForKey: [item identifier]]];
      return YES;
    }
  }
  return NO;  
}

- (BOOL)hasEditorWithIdentifier:(id)ident
{
  for (id item in [editorTabView tabViewItems])
  {
    //if ([[[[item identifier] performSelector:@selector(pluginEditor)] identifier] isEqualTo: ident])
    if ([[item identifier] isEqualTo: ident])
      return YES;
  }
  return NO;
}

- (BOOL)hasEditor:(WBBasePanel*)editor
{
  return [editorTabView indexOfTabViewItemWithIdentifier: [editor identifier]] != NSNotFound;
}

- (BOOL)closeEditor:(WBBasePanel*)editor
{
//  BOOL wasActive= [self activePanel] == panel;
  if (![editor willClose])
    return NO;
    
  NSUInteger index= [editorTabView indexOfTabViewItemWithIdentifier: [editor identifier]];
  if (index == NSNotFound)
    return NO;
  else
  {
    NSTabViewItem *item= [editorTabView tabViewItemAtIndex: index];

    [editor retain];
    
    _lastEditorTabHeight = NSHeight([[editorTabView superview] frame]);
    [editorTabView removeTabViewItem: item];

    [editor release];
  }
  // auto-reselect the current toptabview item if bottom tabview is empty
  // and was 1st responder
/*  if (wasActive && [bottomTabView numberOfTabViewItems] == 0)
  {
    [self tabView: topTabView didSelectTabViewItem: [topTabView selectedTabViewItem]];
  }*/
  [_editorById removeObjectForKey: [editor identifier]];

  return YES;
}


- (WBBasePanel*)findPanelForView:(NSView*)view
{
  for (NSTabViewItem *item in [editorTabView tabViewItems])
  {
    if ([item view] == view)
    {
      return [_editorById objectForKey: [item identifier]];
    }
  }  
  return nil;
}


- (WBBasePanel*)findPanelForPluginType:(Class)klass
{
  for (NSTabViewItem *item in [editorTabView tabViewItems])
  {
    id editor = [_editorById objectForKey: [item identifier]];
    if ([editor respondsToSelector:@selector(pluginEditor)] &&
        [[editor performSelector:@selector(pluginEditor)] isKindOfClass: klass])
      return editor;
  }
  return nil;
}


- (BOOL)closeActiveEditorTab
{
  // check if the keyview is in the selected tab view
  id activeTab = [[editorTabView selectedTabViewItem] view];
  id firstResponder = [[editorTabView window] firstResponder];
  while (firstResponder)
  {
    if (firstResponder == activeTab)
      break;
    firstResponder = [firstResponder superview];
  }
  
  if (firstResponder)
  {
    WBBasePanel *panel= [_editorById objectForKey: [[editorTabView selectedTabViewItem] identifier]];
    
    [self closeEditor: panel];
    return YES;
  }
  return NO;
}

- (void)setTitle:(NSString*)title
        forPanel:(WBBasePanel*)panel
{
  NSInteger i;

  i = [editorTabView indexOfTabViewItemWithIdentifier: [panel identifier]];
  if (i >= 0 && i != NSNotFound)
    [[editorTabView tabViewItemAtIndex: i] setLabel: title];
  else
    NSLog(@"Unknown panel %@", panel);
}

- (void)tabViewDraggerClicked: (NSTabView*) tabView 
{
  if (_lastClick > 0 && [NSDate timeIntervalSinceReferenceDate] - _lastClick < 0.3)
  {  
    if (NSHeight([topContainer frame]) > MODEL_SPLIT_MIN_HEIGHT && NSHeight([bottomContainer frame]) > MODEL_SPLIT_MIN_HEIGHT)
    {
      [mainSplitViewDelegate collapseBottomOfSplitView: mainSplitView];
    }
    else if (NSHeight([topContainer frame]) > MODEL_SPLIT_MIN_HEIGHT && NSHeight([bottomContainer frame]) <= MODEL_SPLIT_MIN_HEIGHT)
    {
      [mainSplitViewDelegate collapseTopOfSplitView: mainSplitView];
    }
    else
    {
      float height= [self minimumSizeForEditorTabView].height;
      if (height < NSHeight([mainSplitView frame]) / 2)
        height= NSHeight([mainSplitView frame]) / 2;
      
      [mainSplitViewDelegate expandBottomOfSplitView: mainSplitView height: height];
    }
    _lastClick= 0;
  }
  else
    _lastClick= [NSDate timeIntervalSinceReferenceDate];
}

- (NSSize)minimumSizeForEditorTabView
{
  NSSize minSize= NSMakeSize(0, 100);
  
  for (NSTabViewItem *tab in [editorTabView tabViewItems])
  {
    WBBasePanel *panel= [_editorById objectForKey: [tab identifier]];
    
    if ([panel respondsToSelector:@selector(minimumSize)])
    {
      NSSize msize= [panel minimumSize];
      
      minSize.width= MAX(minSize.width, msize.width);
      minSize.height= MAX(minSize.height, msize.height);
    }
  }
  
  minSize.height+= NSHeight([editorTabView frame]) - NSHeight([editorTabView contentRect]);
  
  return minSize;
}


- (void)adjustEditorTabViewForNewPanel:(WBBasePanel*)panel
{
  NSSize minimumSize= [self minimumSizeForEditorTabView];
  // check if bottom tabview has to be enlarged
  float minHeight= minimumSize.height;
  float heightDifference = NSHeight([[editorTabView superview] frame]) - NSHeight([[panel topView] frame]);
  float defaultHeightForPanel= NSHeight([[panel topView] frame]) + heightDifference;
  float minHeightForNewPanel= [panel minimumSize].height + NSHeight([[editorTabView superview] frame]) - NSHeight([editorTabView contentRect]);
  
  if (minHeight < minHeightForNewPanel)
    minHeight= minHeightForNewPanel;
  
  if ([editorTabView numberOfTabViewItems] == 0)
  {
    if (minHeight < _lastEditorTabHeight)
      minHeight = _lastEditorTabHeight;
  }
  
  if (defaultHeightForPanel < minHeight)
    defaultHeightForPanel= minHeight;
    
  if (defaultHeightForPanel > NSHeight([[editorTabView superview] frame])
      || [mainSplitView isSubviewCollapsed: [[mainSplitView subviews] lastObject]])
  {
    [mainSplitView setPosition: NSHeight([[mainSplitView superview] frame]) - defaultHeightForPanel - [mainSplitView dividerThickness]
              ofDividerAtIndex: 0];
  }
  
  [bottomContainer setMinContentSize: NSMakeSize(40, minHeight)];
  [mainSplitView adjustSubviews];
}

- (void)tabViewDidChangeNumberOfTabViewItems:(NSTabView *)tabView
{
  if (tabView == editorTabView)
  {        
    if ([tabView numberOfTabViewItems] == 0)
    {
      // tabview got emptied, collapse the splitview
      //[mainSplitViewDelegate setBottomCollapsedMinHeight: 0];
      [mainSplitView setPosition:[mainSplitView maxPossiblePositionOfDividerAtIndex:0] ofDividerAtIndex:0];
      _lastEditorTabHeight= 0;
    }
/*    else if ([tabView numberOfTabViewItems] > 0 && [mainSplitView isSubviewCollapsed:tabView])
    {
      // tabview getting filled again, uncollapse the splitview
      
      CGFloat position= [self splitView:mainSplitView
                 constrainMaxCoordinate:0
                            ofSubviewAt:0];
      
      // if the min size is too small, make it a bit bigger so that it won't show up totally collapsed
      // for views that allows small minsizes
      
      if (position > NSHeight([mainSplitView frame]) - [mainSplitView dividerThickness] - 250)
        position= NSHeight([mainSplitView frame]) - [mainSplitView dividerThickness] - 250;
      
      [mainSplitView setPosition:position
                ofDividerAtIndex:0];
      
    //  if ([tabView numberOfTabViewItems] == 1)
    //    [mainSplitViewDelegate setBottomCollapsedMinHeight: MODEL_SPLIT_MIN_HEIGHT];
    }*/
  }
}


- (BOOL)tabView:(NSTabView *)tabView willCloseTabViewItem:(NSTabViewItem*)tabViewItem
{
  WBBasePanel *panel = [_editorById objectForKey: [tabViewItem identifier]];
  
  //if (![panel willClose])
//    return NO;
//  return YES;
  return [self closeEditor: panel];
}


- (void) tabView: (NSTabView*) tabView 
draggedHandleAtOffset: (NSPoint) offset
{
  if (tabView == editorTabView)
  {
    NSPoint pos= [mainSplitView convertPoint: [[NSApp currentEvent] locationInWindow] fromView: nil];
    
    float position= pos.y - offset.y - [mainSplitView dividerThickness];
    [mainSplitView setPosition: position ofDividerAtIndex: 0];
  }
}


@end
