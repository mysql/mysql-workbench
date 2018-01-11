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

#import "MacTableEditorInformationSource.h"

@implementation MacTableEditorInformationSource

- (void) refresh;
{
  [self listModel]->refresh();
}



- (NSInteger) numberOfRows;
{
    return [self listModel]->count();
}



- (id) objectValueForValueIndex: (NSInteger) valueIndex
                            row: (NSInteger) rowIndex;
{
  NSString* val = nil;
  
  if ([self listModel] != nil) {
    std::string value;
    if ([self listModel]->get_field(rowIndex, valueIndex, value))
      val = @(value.c_str());
    else
      val = @"";
  }
  
  return val;
}



- (BOOL)setStringValue: (NSString*) value
         forValueIndex: (int) valueIndex
                   row: (NSInteger) rowIndex;
{
  BOOL result = NO;
  if ([self listModel] != nil) {
    result = [self listModel]->set_field(rowIndex, valueIndex, [value UTF8String]);
  }

  [self listModel]->refresh();
  return result;
}



- (BOOL)setIntValue: (NSInteger) value
      forValueIndex: (int) valueIndex
                row: (NSInteger) rowIndex;
{
  BOOL result = NO;
  if ([self listModel] != nil) {
    result = [self listModel]->set_field(rowIndex, valueIndex, (ssize_t)value);
  }
  
  [self listModel]->refresh();
  return result;
}



@end


