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

- (NSString*)panelId
{
  return mPluginEditor.panelId;
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
