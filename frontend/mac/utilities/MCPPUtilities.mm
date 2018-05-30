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

#import "MCPPUtilities.h"

//----------------------------------------------------------------------------------------------------------------------

void MShowCPPException(const std::exception &exc) {
  NSAlert *alert = [NSAlert new];
  alert.messageText = @"Unhandled Backend Exception";
  alert.informativeText = [NSString stringWithFormat: @"It is advisable to save your work in a backup file and "
                           "restart Workbench.\nException Details:\n%s", exc.what()];
  alert.alertStyle = NSAlertStyleCritical;
  [alert addButtonWithTitle: @"Ignore"];
  [alert addButtonWithTitle: @"Close Workbench"];

  if ([alert runModal] == NSAlertSecondButtonReturn)
    [NSApp terminate: nil];
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Fills a NSPopUpButton with the contents of the given vector of strings
 */
void MFillPopupButtonWithStrings(NSPopUpButton *popup, const std::vector<std::string> &items) {
  [popup removeAllItems];
  NSMutableArray *array= [NSMutableArray arrayWithCapacity: items.size()];
  for (std::vector<std::string>::const_iterator iter= items.begin(); iter != items.end(); ++iter)
    [array addObject: [NSString stringWithCPPString:*iter]];
  [popup addItemsWithTitles: array];
}

//----------------------------------------------------------------------------------------------------------------------

NSArray<NSString *> *MArrayFromStringVector(const std::vector<std::string> &items) {
  NSMutableArray<NSString*> *result = [NSMutableArray arrayWithCapacity: items.size()];
  
  int j= 0;
  for (std::vector<std::string>::const_iterator i= items.begin(); i != items.end(); ++i, j++)
    result[j] = [NSString stringWithCPPString: *i];

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

NSArray<NSString *> *MArrayFromStringList(const std::list<std::string> &items) {
  NSMutableArray<NSString*> *result = [NSMutableArray arrayWithCapacity: items.size()];

  int j= 0;
  for (std::list<std::string>::const_iterator i= items.begin(); i != items.end(); ++i, j++)
    result[j] = [NSString stringWithCPPString: *i];

  return result;
}

//----------------------------------------------------------------------------------------------------------------------
