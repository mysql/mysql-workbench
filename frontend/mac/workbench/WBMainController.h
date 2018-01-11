/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

namespace bec {
  class UIForm;
};

namespace wb {
  class WBContextUI;
  class WBContext;
  struct WBOptions;
};

@class MainWindowController;
@class WBBasePanel;

typedef WBBasePanel *(*FormPanelFactory)(MainWindowController *controller, std::shared_ptr<bec::UIForm> form);

// TODO: merge this class with MainWindowController. It makes no sense to have 2 controller classes.
@interface WBMainController : NSObject<NSApplicationDelegate, NSFileManagerDelegate>

- (void)registerFormPanelFactory:(FormPanelFactory)fac forFormType:(const std::string &)type;

- (IBAction)menuItemClicked:(id)sender;
- (IBAction)showDiagramProperties:(id)sender;
- (void)requestRefresh;

- (IBAction)buttonClicked:(id)sender;

- (IBAction)inputDialogClose:(id)sender;
- (void)showPageSetup:(id)sender;

@end
