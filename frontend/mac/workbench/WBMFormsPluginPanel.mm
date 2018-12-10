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

#import "WBMFormsPluginPanel.h"

#import "cocoa/MFView.h"

#include "mforms/appview.h"
#include "mforms/menubar.h"

@implementation WBMFormsPluginPanel

//----------------------------------------------------------------------------------------------------------------------

+ (WBMFormsPluginPanel*)panelOfAppView: (mforms::AppView*)view {
  return (__bridge WBMFormsPluginPanel*)view->get_frontend_data();
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithAppView:(mforms::AppView*)view {
  self = [super init];
  if (self != nil) {
    _owner = view;
    if (view != nullptr) {
      view->retain();
      view->set_frontend_data((__bridge void *)self);
      [[NSNotificationCenter defaultCenter] addObserver: self
                                               selector: @selector(windowDidUpdate:)
                                                   name: NSWindowDidUpdateNotification
                                                 object: nil];
    }
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

-(instancetype)init {
  return [self initWithAppView: NULL];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver: self];
  _owner->set_frontend_data(0);
  _owner->release();
  if (_defaultMenuBar)
    _defaultMenuBar->release();
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setDefaultMenuBar:(mforms::MenuBar*)menu {
  _defaultMenuBar = menu;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)windowDidUpdate:(NSNotification *)notification {
  if (_defaultMenuBar && NSApp.mainMenu == _defaultMenuBar->get_data() && self.topView.window.keyWindow)
    _defaultMenuBar->validate();
}

//----------------------------------------------------------------------------------------------------------------------

- (NSMenu*)menuBar {
  NSMenu *menu = super.menuBar;
  if (!menu && _defaultMenuBar)
    return _defaultMenuBar->get_data();
  return menu;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSView*)topView {
  return nsviewForView(_owner);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setTitle: (NSString*)title {
  _title = title;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString*)title {
  return _title;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString*)panelId {
  if (_owner->identifier().empty())
    return  [NSString stringWithFormat: @"mformsview%p", _owner];
  else
    return  @(_owner->identifier().c_str());
}

//----------------------------------------------------------------------------------------------------------------------

- (NSImage*)tabIcon {
  return nil;
}

//----------------------------------------------------------------------------------------------------------------------

- (bec::UIForm*)formBE {
  return _owner;
}

//----------------------------------------------------------------------------------------------------------------------

- (mforms::AppView*)appView {
  return _owner;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)willClose {
  return _owner->on_close();
}

//----------------------------------------------------------------------------------------------------------------------

- (void)didOpen {
}

@end

//----------------------------------------------------------------------------------------------------------------------
