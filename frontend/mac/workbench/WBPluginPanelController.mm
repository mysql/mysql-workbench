/* 
 * Copyright (c) 2009, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "base/geometry.h"
#include "base/string_utilities.h"

#import "WBPluginPanelController.h"

@interface WBPluginPanelController()
{
  NSPanel *_panel;
  WBPluginPanel *_editor;
}

@end

@implementation WBPluginPanelController

- (instancetype)initWithEditor: (WBPluginPanel*)editor
{
  self = [super init];
  if (self != nil && editor != nil)
  {
    NSRect rect = NSZeroRect;
    rect.size = editor.minimumSize;

    _panel = [[NSPanel alloc] initWithContentRect:rect styleMask:0 backing:NSBackingStoreNonretained defer:NO];
    _editor = editor;
    
    _panel.title = _editor.title;
    _panel.contentView = _editor.topView;

    [_editor didOpen];
  }
  return self;
}

- (instancetype)init
{
  return [self initWithEditor: nil];
}

- (void)show:(id)sender
{
  [_panel makeKeyAndOrderFront: nil];
}

- (void)hide:(id)sender
{
}

@end
