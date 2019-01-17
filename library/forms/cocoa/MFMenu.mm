/*
 * Copyright (c) 2010, 2019, Oracle and/or its affiliates. All rights reserved.
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

#import "MFMenu.h"

#import "MFMForms.h"

//--------------------------------------------------------------------------------------------------

@implementation NSMenu(MForms)

+ (void) popUpMenu: (NSMenu*) menu forView: (NSView*) view atLocation: (NSPoint) point pullsDown: (BOOL) pullsDown;
{
  NSMenu* popup = [menu copy];
  NSRect frame = NSMakeRect(point.x + 8, point.y + 12, 1, 1);
  
  if (pullsDown)
    [popup insertItemWithTitle: @"" action: NULL keyEquivalent: @"" atIndex: 0];

  NSPopUpButtonCell* popUpButtonCell = [[NSPopUpButtonCell alloc] initTextCell: @""
                                                                     pullsDown: pullsDown];
  popUpButtonCell.menu = popup;
  if (!pullsDown)
    [popUpButtonCell selectItem: nil];
  [popUpButtonCell performClickWithFrame: frame inView: view];
}
@end

//--------------------------------------------------------------------------------------------------

@implementation MFMenuImpl

- (instancetype) initWithObject:(::mforms::Menu*) aMenu
{
  self= [super initWithTitle: @""];
  if (self)
  {
    mOwner = aMenu;
    mOwner->set_data(self);
    [self setAutoenablesItems: NO];
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void) handleCommand: (NSMenuItem*) sender
{
  NSString* command = sender.representedObject;
  mOwner->handle_action(command.UTF8String);
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityMenuRole;
}

//----------------------------------------------------------------------------------------------------------------------

static bool menu_create(mforms::Menu *self)
{
  MFMenuImpl* menu = [[MFMenuImpl alloc] initWithObject: self];
  return menu != nil;
}

//----------------------------------------------------------------------------------------------------------------------

static void menu_remove_item(mforms::Menu *self, int i)
{
  MFMenuImpl* menu = self->get_data();
  [menu removeItemAtIndex: i];
}

//----------------------------------------------------------------------------------------------------------------------

static int menu_add_item(mforms::Menu *self, const std::string &caption, const std::string &action)
{
  MFMenuImpl* menu = self->get_data();

  NSMenuItem* item;
  
  item = [[NSMenuItem alloc] init];
  item.representedObject = @(action.c_str());
  item.target = menu;
  item.action = @selector(handleCommand:);
  item.title = @(caption.c_str());
  item.enabled = true;
  
  [menu addItem: item];
  return (int)menu.numberOfItems - 1;
}

//----------------------------------------------------------------------------------------------------------------------

static int menu_add_separator(mforms::Menu *self)
{
  MFMenuImpl* menu = self->get_data();

  NSMenuItem* item;
  item = [NSMenuItem separatorItem];
  [menu addItem: item];
  return (int)menu.numberOfItems - 1;
}

//----------------------------------------------------------------------------------------------------------------------

static int menu_add_submenu(mforms::Menu *self, const std::string &caption, mforms::Menu *submenu)
{
  MFMenuImpl* menu = self->get_data();
  
  NSMenuItem* item;
  item = [[NSMenuItem alloc] init];
  item.title = @(caption.c_str());
  item.enabled = true;

  [menu addItem: item];
  MFMenuImpl* theSubMenu = submenu->get_data();
  [menu setSubmenu: theSubMenu forItem: item];
  return (int)menu.numberOfItems - 1;
}

//----------------------------------------------------------------------------------------------------------------------

static void menu_set_item_enabled(mforms::Menu *self, int i, bool flag)
{
  MFMenuImpl* menu = self->get_data();
  
  [menu itemAtIndex: i].enabled = flag;
}

//----------------------------------------------------------------------------------------------------------------------

static void menu_popup_at(mforms::Menu *self, mforms::Object *control, int x, int y)
{
  MFMenuImpl* menu = self->get_data();

  if (!control)
  {
    mforms::Form *activeForm = mforms::Form::active_form();
    if (activeForm != NULL)
    {
      id nativeForm = activeForm->get_data();
      if ([nativeForm isKindOfClass: NSWindow.class])
        nativeForm = [nativeForm firstResponder];
      [NSMenu popUpContextMenu: menu withEvent: [NSApp currentEvent] forView: nativeForm];
    }
  }
  else
  {
    id view = control->get_data();
    if ([view isKindOfClass: [NSWindow class]])
    {
      view = [view contentView];
      y = NSHeight([view frame]) - y;
    }
    [NSMenu popUpMenu: menu
              forView: view
           atLocation: NSMakePoint(x, y)
            pullsDown: NO];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void menu_clear(mforms::Menu *self)
{
  MFMenuImpl* menu = self->get_data();
  
  // [menu removeAllItems]; available on 10.6+
  while (menu.numberOfItems > 0)
    [menu removeItemAtIndex: 0];
}

//----------------------------------------------------------------------------------------------------------------------

void cf_menu_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_menu_impl.create= &menu_create;
  f->_menu_impl.add_item= &menu_add_item;
  f->_menu_impl.add_separator= &menu_add_separator;
  f->_menu_impl.add_submenu= &menu_add_submenu;
  f->_menu_impl.set_item_enabled= &menu_set_item_enabled;
  f->_menu_impl.popup_at= &menu_popup_at;
  f->_menu_impl.clear= &menu_clear;
  f->_menu_impl.remove_item= &menu_remove_item;
}

@end

//----------------------------------------------------------------------------------------------------------------------

NSMenu *nsmenuForMenu(mforms::Menu *menu)
{
  return menu->get_data();
}

//----------------------------------------------------------------------------------------------------------------------
