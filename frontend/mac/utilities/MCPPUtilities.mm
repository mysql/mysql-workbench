/*
 * Copyright (c) 2008, 2016, Oracle and/or its affiliates. All rights reserved.
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

#import "MCPPUtilities.h"

void MShowCPPException(const std::exception &exc)
{
  NSAlert *alert = [NSAlert new];
  alert.messageText = @"Unhandled Backend Exception";
  alert.informativeText = [NSString stringWithFormat: @"It is advisable to save your work in a backup file and restart Workbench.\n"
                           "Exception Details:\n%s", exc.what()];
  alert.alertStyle = NSCriticalAlertStyle;
  [alert addButtonWithTitle: @"Ignore"];
  [alert addButtonWithTitle: @"Close Workbench"];

  if ([alert runModal] == NSAlertSecondButtonReturn)
    [NSApp terminate: nil];
}


/** Fills a NSPopUpButton with the contents of the given vector of strings
 */
void MFillPopupButtonWithStrings(NSPopUpButton *popup, const std::vector<std::string> &items)
{
  [popup removeAllItems];
  NSMutableArray *array= [NSMutableArray arrayWithCapacity: items.size()];
  for (std::vector<std::string>::const_iterator iter= items.begin(); iter != items.end(); ++iter)
    [array addObject: [NSString stringWithCPPString:*iter]];
  [popup addItemsWithTitles: array];
}


NSArray<NSString *> *MArrayFromStringVector(const std::vector<std::string> &items)
{
  NSMutableArray<NSString*> *result = [NSMutableArray arrayWithCapacity: items.size()];
  
  int j= 0;
  for (std::vector<std::string>::const_iterator i= items.begin(); i != items.end(); ++i, j++)
    result[j] = [NSString stringWithCPPString: *i];

  return result;
}


NSArray<NSString *> *MArrayFromStringList(const std::list<std::string> &items)
{
  NSMutableArray<NSString*> *result = [NSMutableArray arrayWithCapacity: items.size()];

  int j= 0;
  for (std::list<std::string>::const_iterator i= items.begin(); i != items.end(); ++i, j++)
    result[j] = [NSString stringWithCPPString: *i];

  return result;
}
