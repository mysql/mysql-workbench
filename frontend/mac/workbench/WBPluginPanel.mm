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

#import "WBPluginPanel.h"
#include "grt/grt_manager.h"

@implementation WBPluginPanel

- (instancetype)initWithPlugin: (WBPluginEditorBase *)plugin
{
  self = [super init];
  if (self != nil)
  {
    mPluginEditor = plugin;
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

- (WBPluginEditorBase*)pluginEditor
{
  return mPluginEditor;
}

- (NSView*)topView
{
  return mPluginEditor.view;
}

- (NSString*)title
{
  return mPluginEditor.title;
}

- (NSString*)identifier
{
  return mPluginEditor.identifier;
}

- (NSImage*)tabIcon
{
  return mPluginEditor.titleIcon;
}

- (bec::UIForm*)formBE
{
  return 0;
}

- (NSSize)minimumSize
{
  return mPluginEditor.minimumSize;
}

- (void)didShow
{
  if ([mPluginEditor respondsToSelector:@selector(pluginDidShow:)])
    [mPluginEditor pluginDidShow:self];
}

- (BOOL)willClose
{
  if ([mPluginEditor respondsToSelector:@selector(pluginWillClose:)])
    return [mPluginEditor pluginWillClose:self];
  return YES;
}

@end
