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
#import "WBPluginEditorBase.h"

#include "wb_editor_layer.h"

@interface PhysicalLayerEditor : WBPluginEditorBase {
  IBOutlet NSTabView *tabView; // this editor has a single Tab, but we put in a TabView for homegeneity

  IBOutlet NSTextField *nameText;

  IBOutlet NSTextField *colorText;
  IBOutlet NSColorWell *colorWell;

  NSTimer *mColorCommitTimer;
  LayerEditorBE *mBackEnd; //!< layer editor backend
}

- (IBAction)changeColor:(id)sender;

- (instancetype)initWithModule:(grt::Module *)module arguments:(const grt::BaseListRef &)args NS_DESIGNATED_INITIALIZER;

@end
