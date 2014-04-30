/* 
 * Copyright (c) 2009, 2012, Oracle and/or its affiliates. All rights reserved.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#import "WBMFormsPluginPanel.h"

#import "cocoa/MFView.h"

#include "mforms/appview.h"
#include "mforms/menubar.h"

@implementation WBMFormsPluginPanel


+ (WBMFormsPluginPanel*)panelOfAppView:(mforms::AppView*)view
{
  return (WBMFormsPluginPanel*)view->get_frontend_data();
}


- (id)initWithAppView:(mforms::AppView*)view
{
  self = [super init];
  if (self)
  {
    _owner = view;
    view->retain();
    view->set_frontend_data(self);
    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector(windowDidUpdate:)
                                                 name: NSWindowDidUpdateNotification
                                               object: nil];
  }
  return self;
}

- (void) dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver: self];
  _owner->set_frontend_data(0);
  _owner->release();
  if (_defaultMenuBar)
    _defaultMenuBar->release();
  
  [super dealloc];
}


- (void)setDefaultMenuBar:(mforms::MenuBar*)menu
{
  _defaultMenuBar = menu;
}


- (void)windowDidUpdate:(NSNotification *)notification
{
  if (_defaultMenuBar && [NSApp mainMenu] == _defaultMenuBar->get_data() && [[[self topView] window] isKeyWindow])
    _defaultMenuBar->validate();
}


- (NSMenu*)menuBar
{
  NSMenu *menu = [super menuBar];
  if (!menu && _defaultMenuBar)
    return _defaultMenuBar->get_data();
  return menu;
}


- (NSView*)topView
{
  return nsviewForView(_owner);
}


- (void)setTitle:(NSString*)title
{
  [_title autorelease];
  _title = [title retain];
}


- (NSString*)title
{
  return _title;
}


- (NSString*)identifier
{
  if (_owner->identifier().empty())
    return  [NSString stringWithFormat: @"mformsview%p", _owner];
  else
    return  [NSString stringWithUTF8String: _owner->identifier().c_str()];
}


- (NSImage*)tabIcon
{
  return nil;
}


- (bec::UIForm*)formBE
{
  return _owner;
}


- (NSSize)minimumSize
{
  return NSMakeSize(1, 1);
}


- (BOOL)willClose
{
  return _owner->on_close();
}


- (void)didOpen
{
}


@end
