/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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
      val = [NSString stringWithUTF8String: value.c_str()];
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


