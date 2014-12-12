/* 
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
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
#import "WBSplitView.h"
#import "WBSplitViewUnbrokenizerDelegate.h"
#import "MContainerView.h"

@interface WBSplitPanel : WBSidebarPanel
{
  IBOutlet WBSplitView *mainSplitView;
  IBOutlet WBSplitViewUnbrokenizerDelegate *mainSplitViewDelegate;
  IBOutlet NSView *topContainer;
  IBOutlet MContainerView *bottomContainer;
  IBOutlet NSTabView *editorTabView;
  
  NSMutableDictionary *_editorById;
  float _lastEditorTabHeight;
  NSTimeInterval _lastClick;
}

@property (readonly) NSSize minimumSizeForEditorTabView;
@property (readonly) BOOL closeActiveEditorTab;
- (void)addEditor:(WBBasePanel*)editor;
- (BOOL)closeEditor:(WBBasePanel*)editor;
- (BOOL)hasEditor:(WBBasePanel*)editor;
- (BOOL)closeEditorWithIdentifier:(id)ident;
- (BOOL)hasEditorWithIdentifier:(id)ident;
- (WBBasePanel*)findPanelForPluginType:(Class)klass;
- (WBBasePanel*)findPanelForView:(NSView*)view;
- (void)adjustEditorTabViewForNewPanel:(WBBasePanel*)panel;
@end
