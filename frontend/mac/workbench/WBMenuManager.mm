/* 
 * Copyright (c) 2008, 2016, Oracle and/or its affiliates. All rights reserved.
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */


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
      item.representedObject = [NSString stringWithCPPString: iter->name];

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
      NSLog(@"unknown context menu item type in %s", iter->name.c_str());
  }
  
  for (NSInteger i= oldCount - 1; i >= 0; i--)
    [menu removeItemAtIndex:0];
}
@end
