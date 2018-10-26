/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#import "WBMenuManager.h"
#import "MCPPUtilities.h"
#include "base/string_utilities.h"

@implementation WBMenuManager

+ (void)fillMenu:(NSMenu*)menu withItems:(const bec::MenuItemList&)items 
        selector:(SEL)selector target:(id)target
{
  NSInteger oldCount = menu.numberOfItems;
  [menu setAutoenablesItems: NO];
  
  for (bec::MenuItemList::const_iterator iter= items.begin();
       iter != items.end(); ++iter)
  {
    if ((iter->type == bec::MenuAction) || (iter->type == bec::MenuCascade))
    {
      SEL itemAction;
      if (iter->type == bec::MenuCascade)
      {
        itemAction = nil;
      }
      else
      {
        itemAction = selector;
      }
      
      NSMenuItem *item= [[NSMenuItem alloc] initWithTitle:[NSString stringWithCPPString: iter->caption]
                                                   action:itemAction
                                            keyEquivalent:@""];
      item.enabled = iter->enabled?YES:NO;
      item.target = target;
      [menu addItem: item];
      item.representedObject = [NSString stringWithCPPString: iter->internalName];

      if (iter->type == bec::MenuCascade)
      {
        if (!iter->subitems.empty())
        {
          NSMenu *submenu= [[NSMenu alloc] initWithTitle: menu.title];
          [self fillMenu: submenu withItems:iter->subitems selector:selector target:target];
          item.submenu = submenu;
        }
      }
      
    }
    else if (iter->type == bec::MenuSeparator)
      [menu addItem: [NSMenuItem separatorItem]];
    else
      NSLog(@"unknown context menu item type in %s", iter->internalName.c_str());
  }
  
  for (NSInteger i= oldCount - 1; i >= 0; i--)
    [menu removeItemAtIndex:0];
}
@end
