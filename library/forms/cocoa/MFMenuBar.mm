/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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

#import "MFMenuBar.h"
#import "MFBase.h"
#include "base/string_utilities.h"
#import "MFMForms.h"

@interface MFMenuItem : NSMenuItem <NSMenuDelegate>
{
  std::function<void ()> slot;
@public
  mforms::MenuItem *item;
}

@end

static NSMenuItem *applicationMenuTemplate = nil;
static NSMenuItem *defaultEditMenu = nil;

@implementation MFMenuItem

- (instancetype)initWithCoder:(NSCoder *)decoder
{
  return [super initWithCoder: decoder];
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithTitle: (NSString *)string action: (nullable SEL)selector keyEquivalent: (NSString *)charCode
{
  return [self initWithTitle: string slot: std::function<void ()>()];
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithTitle:(NSString*)title slot:(std::function<void ()>)aslot
{
  self = [super initWithTitle: title action: @selector(callSlot:) keyEquivalent: @""];
  if (self)
  {
    slot = aslot;
    self.target = self;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)callSlot:(id)sender
{
  slot();
}

//----------------------------------------------------------------------------------------------------------------------

- (void)menuWillOpen:(NSMenu *)menu
{
  if (item && dynamic_cast<mforms::MenuBar*>(item->get_top_menu()))
    dynamic_cast<mforms::MenuBar*>(item->get_top_menu())->will_show_submenu_from(item);
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityMenuItemRole;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface MFContextMenu : NSMenu <NSMenuDelegate>
{
@public
  mforms::ContextMenu *cmenu;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFContextMenu

- (instancetype)initWithTitle: (NSString*)title
{
  self = [super initWithTitle: title];
  if (self)
  {
    self.delegate = self;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)menuNeedsUpdate:(NSMenu *)menu
{
  cmenu->will_show();
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityMenuRole;
}

@end

//----------------------------------------------------------------------------------------------------------------------

using namespace mforms;

//----------------------------------------------------------------------------------------------------------------------

static bool create_menu_bar(MenuBar *aitem)
{
  NSMenu *menu = [[NSMenu alloc] initWithTitle: @"main"];
  [menu addItem: [applicationMenuTemplate copy]];
  aitem->set_data(menu);
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

static bool create_context_menu(ContextMenu *aitem)
{
  MFContextMenu *menu = [[MFContextMenu alloc] initWithTitle: @"context"];
  menu->cmenu = aitem;
  [menu setAutoenablesItems: NO];
  aitem->set_data(menu);
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

static bool create_menu_item(MenuItem *aitem, const std::string &title, MenuItemType type)
{
  if (title.empty())
  {
    NSMenuItem *item = [NSMenuItem separatorItem];
    aitem->set_data(item);
  }
  else
  {
    MFMenuItem *item = [[MFMenuItem alloc] initWithTitle: wrap_nsstring(title) slot: std::bind(&MenuItem::callback, aitem)];
    item->item = aitem;
    aitem->set_data(item);
  }
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

static void set_title(MenuItem *aitem, const std::string &title)
{
  NSMenuItem *item = aitem->get_data();
  item.title = wrap_nsstring(title);
}

//----------------------------------------------------------------------------------------------------------------------

static std::string get_title(MenuItem *aitem)
{
  NSMenuItem *item = aitem->get_data();
  return item.title.UTF8String ?: "";
}

//----------------------------------------------------------------------------------------------------------------------

static void set_name(MenuItem *aitem, const std::string &name)
{
  NSMenuItem *item = aitem->get_data();
  item.accessibilityLabel = [NSString stringWithUTF8String: name.c_str()];
}

//----------------------------------------------------------------------------------------------------------------------

static void set_shortcut(MenuItem *aitem, const std::string &shortcut)
{
  NSMenuItem *item = aitem->get_data();
  
  if (!shortcut.empty())
  {
    std::vector<std::string> parts(base::split(shortcut, "+", 0));
    unichar key = 0;
    NSUInteger mask= 0;
    
    if (parts.size() > 0)
    {
      std::string askey = parts.back();
      key= g_utf8_get_char_validated(askey.c_str(), askey.length());
      parts.pop_back();
      
      if (askey.size() == 1 && key != (unichar)-1 && key != (unichar)-2 && 
          (g_unichar_isalnum(key) || g_unichar_ispunct(key)))
      {
        key= g_unichar_tolower(key);
      }
      else
      {
        if (askey == "Delete")
          key= NSDeleteCharacter;
        else if (askey == "BackSpace" || askey == "Backspace")
          key= NSBackspaceCharacter;
        else if (askey == "F1")
          key= NSF1FunctionKey;
        else if (askey == "F2")
          key= NSF2FunctionKey;
        else if (askey == "F3")
          key= NSF3FunctionKey;
        else if (askey == "F4")
          key= NSF4FunctionKey;
        else if (askey == "F5")
          key= NSF5FunctionKey;
        else if (askey == "F6")
          key= NSF6FunctionKey;
        else if (askey == "F7")
          key= NSF7FunctionKey;
        else if (askey == "F8")
          key= NSF8FunctionKey;
        else if (askey == "F9")
          key= NSF9FunctionKey;
        else if (askey == "F10")
          key= NSF10FunctionKey;
        else if (askey == "F11")
          key= NSF11FunctionKey;
        else if (askey == "F12")
          key= NSF12FunctionKey;
        else if (askey == "Tab")
          key = '\t';
        else if (askey == "Enter" || askey == "Return")
          key= '\n';
        else if (askey == "question")
          key = '?';
        else if (askey == "Plus")
          key= '+';
        else if (askey == "Minus")
          key = '-';
        else if (askey == "Slash")
          key = '/';
        else if (askey == "Period")
          key = '.';
        else if (askey == "Comma")
          key = ',';
        else if (askey == "OpenBrackets")
          key = '[';
        else if (askey == "CloseBrackets")
          key = ']';
        else if (askey == "Escape")
          key = 0x1B;
        else if (askey == "Space")
          key = ' ';
        else if (askey == "Left")
          key = NSLeftArrowFunctionKey;
        else if (askey == "Right")
          key = NSRightArrowFunctionKey;
        else if (askey == "Up")
          key = NSUpArrowFunctionKey;
        else if (askey == "Down")
          key = NSDownArrowFunctionKey;
        else
          NSLog(@"Unknown character '%s' for menu shortcut", askey.c_str());
      }
      
      item.keyEquivalent = [NSString stringWithCharacters:&key length:1];
    }
    
    if (parts.size() > 0)
    {
      for (std::vector<std::string>::const_iterator iter= parts.begin();
           iter != parts.end(); ++iter)
      {
        if (*iter == "Command" || *iter == "Modifier")
          mask|= NSEventModifierFlagCommand;
        else if (*iter == "Alt" || *iter == "Alternate" || *iter == "Option")
          mask|= NSEventModifierFlagOption;
        else if (*iter == "Control")
          mask|= NSEventModifierFlagControl;
        else if (*iter == "Shift")
          mask|= NSEventModifierFlagShift;
      }
      if (mask != 0)
        item.keyEquivalentModifierMask = mask;
    }
    else {
      item.keyEquivalentModifierMask = 0;
    }

  }
}

//----------------------------------------------------------------------------------------------------------------------

static void set_enabled(MenuBase *aitem, bool enabled)
{
  NSMenuItem *item = aitem->get_data();
  item.enabled = enabled;
}

//----------------------------------------------------------------------------------------------------------------------

static bool get_enabled(MenuBase *aitem)
{
  NSMenuItem *item = aitem->get_data();
  return item.enabled;
}

//----------------------------------------------------------------------------------------------------------------------

static void set_checked(MenuItem *aitem, bool flag)
{
  NSMenuItem *item = aitem->get_data();
  item.state = flag ? NSControlStateValueOn : NSControlStateValueOff;
}

//----------------------------------------------------------------------------------------------------------------------

static bool get_checked(MenuItem *aitem)
{
  NSMenuItem *item = aitem->get_data();
  return item.state == NSControlStateValueOn;
}

//----------------------------------------------------------------------------------------------------------------------

static void insert_item(MenuBase *aitem, int index, MenuItem *asubitem)
{  
  NSMenu *submenu;
  MFMenuItem *subitem = asubitem->get_data();
  bool is_context = false;
    
  if (dynamic_cast<MenuBar*>(aitem) || (is_context = (dynamic_cast<ContextMenu*>(aitem) != NULL)))
    submenu = aitem->get_data();
  else
  {
    MFMenuItem *parItem = aitem->get_data();
    submenu = parItem.submenu;
    if (!submenu)
    {
      submenu = [[NSMenu alloc] initWithTitle: parItem.title];
      [submenu setAutoenablesItems: NO];
      parItem.submenu = submenu;
      submenu.delegate = parItem;
    }
  }

  if (index >= submenu.numberOfItems)
    [submenu addItem: subitem];
  else
    [submenu insertItem: subitem atIndex: is_context ? index : index+1];
}

//----------------------------------------------------------------------------------------------------------------------

static void remove_item(MenuBase *aitem, MenuItem *asubitem)
{
  NSMenu *submenu;
  NSMenuItem *subitem = nil;
  if (asubitem)
    subitem = asubitem->get_data();
  if (dynamic_cast<MenuBar*>(aitem) || dynamic_cast<ContextMenu*>(aitem))
    submenu = aitem->get_data();
  else
    submenu = [aitem->get_data() submenu]; // must be a menuitem
  if (subitem)
    [submenu removeItem: subitem];
  else
  {
    while (submenu.numberOfItems > 0)
      [submenu removeItemAtIndex: 0];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void popup_at(ContextMenu *menu, View *owner, base::Point location)
{
  [NSMenu popUpContextMenu: menu->get_data()
                 withEvent: NSApp.currentEvent
                   forView: owner->get_data()];
}


//----------------------------------------------------------------------------------------------------------------------

static NSMenuItem *swappedEditMenu = nil;

void cf_swap_edit_menu()
{
  NSMenuItem *editMenu = [NSApp.mainMenu itemAtIndex: 2];  
  if (editMenu.tag != 424242)
  {
    swappedEditMenu = editMenu;
    [NSApp.mainMenu removeItem: swappedEditMenu];
    [NSApp.mainMenu insertItem: [defaultEditMenu copy] atIndex: 2];
  }
}

//----------------------------------------------------------------------------------------------------------------------

void cf_unswap_edit_menu()
{
  if (swappedEditMenu)
  {
    NSMenuItem *editMenu = [NSApp.mainMenu itemAtIndex: 2];  
    if (editMenu.tag == 424242)
    {
      [NSApp.mainMenu removeItem: editMenu];
      [NSApp.mainMenu insertItem: swappedEditMenu atIndex: 2];
      swappedEditMenu = nil;
    }
  }  
}

//----------------------------------------------------------------------------------------------------------------------

void cf_menubar_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  // get default app menu and make a template from it
  applicationMenuTemplate = [NSApp.mainMenu itemAtIndex: 0];
  
  // hack
  // the way to create a Edit menu that works just like the one you get in IB is a mistery
  // so we create it in IB, keep a reference to it and only display it when we need,
  // which is in modal windows. Once the modal window is gone, the normal WB 
  // created menu can be used
  defaultEditMenu = [NSApp.mainMenu itemAtIndex: 1];
  [NSApp.mainMenu removeItem: defaultEditMenu];
  
  defaultEditMenu.tag = 424242;

  f->_menu_item_impl.create_context_menu = create_context_menu;
  f->_menu_item_impl.create_menu_bar = create_menu_bar;
  f->_menu_item_impl.create_menu_item = create_menu_item;
  f->_menu_item_impl.set_title = set_title;
  f->_menu_item_impl.get_title = get_title;
  f->_menu_item_impl.set_name = set_name;
  f->_menu_item_impl.set_shortcut = set_shortcut;
  f->_menu_item_impl.set_enabled = set_enabled;
  f->_menu_item_impl.get_enabled = get_enabled;
  f->_menu_item_impl.set_checked = set_checked;
  f->_menu_item_impl.get_checked = get_checked;
  
  f->_menu_item_impl.insert_item = insert_item;
  f->_menu_item_impl.remove_item = remove_item;

  f->_menu_item_impl.popup_at = popup_at;
}

//----------------------------------------------------------------------------------------------------------------------
