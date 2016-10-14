/* 
 * Copyright (c) 2010, 2016, Oracle and/or its affiliates. All rights reserved.
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

#import "NSMenu_extras.h"
#import "NSString_extras.h"

@implementation NSMenu(NSMenu_Extras)

+ (NSMenu*)menuFromMenuItems:(const bec::MenuItemList&)items
                      action:(SEL)selector
                      target:(id)target
{
  NSMenu *menu=nil;

  if (!items.empty())
  {
    menu = [[NSMenu alloc] initWithTitle: @""];
    [menu setAutoenablesItems: NO];

    for (bec::MenuItemList::const_iterator iter= items.begin(); iter != items.end(); ++iter)
    {
      switch (iter->type)
      {
        case bec::MenuSeparator:
          [menu addItem: [NSMenuItem separatorItem]];
          break;
          
        case bec::MenuCascade:
        {
          NSMenuItem *item = [[NSMenuItem alloc] initWithTitle: [NSString stringWithCPPString: iter->caption]
                                                        action: selector
                                                 keyEquivalent: @""];
          item.target = target;

          NSMenu *sub = [NSMenu menuFromMenuItems:iter->subitems action:selector target:target];
          item.submenu = sub;
          
          [menu addItem: item];
          
          break;
        }
      
        default:
        {
          SEL selector = NSSelectorFromString(@"activateMenuItem:");
          NSMenuItem *item= [menu addItemWithTitle: [NSString stringWithCPPString: iter->caption]
                                            action: selector
                                     keyEquivalent: @""];
          item.target = target;
          if (!iter->enabled)
            [item setEnabled: NO];
          if (iter->checked)
            item.state = NSOnState;
          else
            item.state = NSOffState;
          item.representedObject = [NSString stringWithCPPString: iter->name];
        }
      }
    }
  }
  return menu;
}


@end
