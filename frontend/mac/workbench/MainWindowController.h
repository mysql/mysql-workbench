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

#include "base/ui_form.h"

#include "grts/structs.h"

#include "workbench/wb_context_ui.h"

#import "WBModelSidebarController.h"

#include "plugin_manager.h"

@class WBMainController;
@class WBBasePanel;
@class WBModelOverviewPanel;
@class WBSplitView;
@class MContainerView;
@class MTabSwitcher;
@class WBPluginEditorBase;

class MacNotificationObserver;

@interface MainWindowController : NSWindowController {
  WBModelOverviewPanel *_physicalOverview;
}

@property(readonly) BOOL closeAllPanels;
@property(weak) WBMainController *owner;
@property(readonly, strong) WBBasePanel *selectedTopPanel;
@property(readonly, strong) WBBasePanel *activePanel;
@property(readonly, strong) WBBasePanel *selectedMainPanel;

- (void)setup;
- (void)setupReady;
- (NSTabViewItem *)addTopPanel:(WBBasePanel *)panel;
- (NSTabViewItem *)addTopPanelAndSwitch:(WBBasePanel *)panel;
- (void)addBottomPanel:(WBBasePanel *)panel;
- (WBBasePanel *)findPanelForPluginType:(Class)type;
- (WBBasePanel *)findMainPanelForUIForm:(bec::UIForm *)form;
- (void)activatePanel:(WBBasePanel *)panel;
- (void)setTitle:(NSString *)title forPanel:(WBBasePanel *)panel;

- (IBAction)handleMenuAction:(id)sender;
- (IBAction)showMySQLOverview:(id)sender;

- (void)focusSearchField:(id)sender;
- (void)performSearchObject:(id)sender;

- (void)blockGUI:(BOOL)lock;

- (void)switchToDiagramWithIdentifier:(const char *)identifier;
- (void)reopenEditor:(id)editor;

- (void)changedIdentifierOfPanel:(WBBasePanel *)panel fromIdentifier:(id)identifier;

- (void)resetWindowLayout;

- (void)refreshGUI:(wb::RefreshType)type argument1:(const std::string &)arg1 argument2:(NativeHandle)arg2;

- (void)setStatusText:(NSString *)text;

- (mdc::CanvasView *)createView:(const char *)oid name:(const char *)name NS_RETURNS_INNER_POINTER;
- (void)destroyView:(mdc::CanvasView *)view;

- (BOOL)closePanel:(WBBasePanel *)panel;
- (void)closeTopPanelWithIdentifier:(id)identifier;
- (void)closeTopPanelWithIdentifier:(id)identifier hideOnly:(BOOL)hideOnly;
- (void)closeBottomPanelWithIdentifier:(id)identifier;
- (void)closeEditorsMatching:(NSString *)identifier;

- (void)forwardCommandToPanels:(const std::string)command;

- (void)firstResponderChanged:(NSResponder *)responder;

@end
