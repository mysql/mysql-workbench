/* 
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#import <Cocoa/Cocoa.h>

#include "base/ui_form.h"

#include "grts/structs.h"

#include "workbench/wb_context_ui.h"

#import "WBModelSidebarController.h"

#include "plugin_manager.h"

@class WBMainController;
@class WBBasePanel;
@class WBModelOverviewPanel;
@class WBSplitView;
@class WBSplitViewUnbrokenizerDelegate;
@class MContainerView;
@class MTabSwitcher;

@class WBPluginEditorBase;


// Subclass of NSWindow to override makeFirstResponder: and detect key view changes
@interface WBWindow : NSWindow
{
}
@end

class MacNotificationObserver;


@interface WBMainWindow : NSWindowController
{
  IBOutlet NSTextField *statusBarText;
  IBOutlet NSTabView *topTabView;
  IBOutlet MTabSwitcher *tabSwitcher;
  
  NSMutableDictionary *_panels;
  
  NSTimer *_backendTimer;
  
  NSMutableArray *_closedTopTabs;
  NSMutableArray *_closedBottomTabs;
  
  WBMainController *_mainController;
  
  NSMenu *_defaultMainMenu;
  
  WBModelOverviewPanel *_physicalOverview;

  int _eventLoopRetCode;
  
  wb::WBContextUI *_wbui;
  
  NSTimeInterval _lastClick;
  
  MacNotificationObserver *_backendObserver;
}

- (void)load;
- (void)setWBContext:(wb::WBContextUI*)wbui;
@property (readonly) wb::WBContextUI *context;
- (void)setupReady;
@property (readonly) BOOL closeAllPanels;
- (NSTabViewItem*)addTopPanel:(WBBasePanel*)panel;
- (NSTabViewItem*)addTopPanelAndSwitch:(WBBasePanel*)panel;
- (void)addBottomPanel:(WBBasePanel*)panel;
- (WBBasePanel*)findPanelForPluginType: (Class) type;
- (WBBasePanel*)findMainPanelForUIForm: (bec::UIForm*)form;
- (void)activatePanel: (WBBasePanel*) panel;
- (void)setTitle:(NSString*)title
        forPanel:(WBBasePanel*)panel;

@property (strong) WBMainController *owner;

@property (readonly, strong) WBBasePanel *selectedTopPanel;

- (IBAction)handleMenuAction:(id)sender;
- (IBAction)showMySQLOverview:(id)sender;

- (void)focusSearchField:(id)sender;
- (void)performSearchObject:(id)sender;

- (void)blockGUI:(BOOL)lock;

- (void)switchToDiagramWithIdentifier:(const char*)identifier;
- (void)reopenEditor:(id)editor;

- (void)changedIdentifierOfPanel: (WBBasePanel*)panel
                  fromIdentifier: (id)identifier;

- (void)resetWindowLayout;

- (void)refreshGUI:(wb::RefreshType)type
         argument1:(const std::string&)arg1
         argument2:(NativeHandle)arg2;

- (void)setStatusText:(NSString*)text;

- (mdc::CanvasView*)createView:(const char*)oid
                          name:(const char*)name NS_RETURNS_INNER_POINTER;
- (void)destroyView:(mdc::CanvasView*)view;

- (BOOL)closePanel:(WBBasePanel*)panel;
- (void)closeTopPanelWithIdentifier:(id)identifier;
- (void)closeTopPanelWithIdentifier:(id)identifier hideOnly:(BOOL)hideOnly;
- (void)closeBottomPanelWithIdentifier:(id)identifier;
- (void)closeEditorsMatching: (NSString*)identifier;

@property (readonly, strong) WBBasePanel *activePanel;
@property (readonly, strong) WBBasePanel *selectedMainPanel;

- (void)forwardCommandToPanels: (const std::string) command;

- (void)firstResponderChanged: (NSResponder*)responder;

@end
