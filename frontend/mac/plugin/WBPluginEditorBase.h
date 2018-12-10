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

#import <Cocoa/Cocoa.h>
#include "grts/structs.ui.h"

namespace grt {
  class BaseListRef;
  class Module;
}
namespace bec {
  class GRTManager;
  class BaseEditor;
}
namespace mforms {
  class DockingPoint;
}

//! Base class for editor plugins. Must return the view to be docked in the main window
//! in dockableView.
@interface WBPluginEditorBase : NSViewController {
  NSSize mMinumumSize;

  mforms::DockingPoint *mDockingPoint;
  ui_ObjectEditorRef mEditorGRTObject;

  IBOutlet __weak NSButton *mApplyButton;
  IBOutlet __weak NSButton *mRevertButton;
}

@property(readonly) BOOL enableLiveChangeButtons;
@property NSSize minimumSize;
@property(readonly) bec::BaseEditor *editorBE;
@property(readonly, strong) id panelId;
@property(readonly, copy) NSImage *titleIcon;

- (void)updateTitle:(NSString *)title;

- (void)reinitWithArguments:(const grt::BaseListRef &)args;
- (void)notifyObjectSwitched;

- (void)refresh;

- (void)setCompactMode:(BOOL)flag;
- (BOOL)matchesIdentifierForClosingEditor:(NSString *)identifier;

- (void)pluginDidShow:(id)sender;
- (BOOL)pluginWillClose:(id)sender;

- (void)applyLiveChanges:(id)sender;
- (void)revertLiveChanges:(id)sender;

- (void)setupEditorOnHost:(NSView *)host;

- (void)enablePluginDocking:(NSTabView *)tabView;

@end
