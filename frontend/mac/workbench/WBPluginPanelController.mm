/* 
 * Copyright (c) 2009, 2012, Oracle and/or its affiliates. All rights reserved.
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

@implementation WBPluginPanelController

- (id)initWithEditor:(WBPluginPanel*)editor
{
  self = [super init];
  if (self)
  {
    NSRect rect = NSZeroRect;
    rect.size = [editor minimumSize];

    _panel = [[NSPanel alloc] initWithContentRect:rect styleMask:0 backing:NSBackingStoreNonretained defer:NO];
    _editor = [editor retain];
    
    [_panel setTitle: [_editor title]];
    [_panel setContentView: [_editor topView]];

    [_editor didOpen];
  }
  return self;
}


- (void)show:(id)sender
{
  [_panel makeKeyAndOrderFront: nil];
}


- (void)hide:(id)sender
{
}

@end
