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

#import "WBPluginWindowController.h"

#include "grt_manager.h"

@interface WBPluginWindowController()
{
  NSMutableArray *nibObjects;

  WBPluginEditorBase *mPluginEditor;

  __weak IBOutlet NSWindow *window;
  __weak IBOutlet NSView *contentView;
}

@end

@implementation WBPluginWindowController

- (instancetype)initWithPlugin: (WBPluginEditorBase*)plugin
{
  self = [super init];
  if (self != nil && plugin != nil)
  {
    mPluginEditor = plugin;

    NSMutableArray *temp;
    if ([NSBundle.mainBundle loadNibNamed: @"PluginEditorWindow" owner: self topLevelObjects: &temp])
    {
      nibObjects = temp;

      float yextra;
      NSSize size = mPluginEditor.minimumSize;
      {
        NSSize wsize = [window contentRectForFrameRect: window.frame].size;
        NSSize csize = contentView.frame.size;
        yextra = wsize.height - csize.height;
      }
      size.width += 100;
      size.height += 80;
      NSRect rect = NSZeroRect;
      rect.size = size;

      size.height += yextra;
      [window setContentSize: size];

      NSView *view = mPluginEditor.view;
      [contentView addSubview: view];
      view.frame = rect;

      window.title = mPluginEditor.title;

      [window makeKeyAndOrderFront: nil];
    }
  }
  return self;
}

- (instancetype)init
{
  return [self initWithPlugin: nil];
}

- (void)dealloc
{
  bec::GRTManager::get()->get_plugin_manager()->forget_gui_plugin_handle((__bridge void *)self);
}

- (BOOL)windowShouldClose:(id)sender
{
  if ([mPluginEditor respondsToSelector:@selector(pluginWillClose:)])
    return [mPluginEditor pluginWillClose:self];
  return YES;
}

- (IBAction)buttonClicked:(id)sender
{
  switch ([sender tag])
  {
    case 10: // Close
      if ([mPluginEditor pluginWillClose:self])
        [window close];
      break;
      
    case 11: // Revert
      [mPluginEditor revertLiveChanges:nil];
      break;
      
    case 12: // Apply
      [mPluginEditor applyLiveChanges:nil];
      break;
  }
}

@end
