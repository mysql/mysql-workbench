/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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
