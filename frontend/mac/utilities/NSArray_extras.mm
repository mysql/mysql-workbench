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

#import "NSArray_extras.h"

@implementation NSArray(CPPExtras)

+ (NSArray*)arrayWithCPPStringVector:(const std::vector<std::string> &)v
{
  NSMutableArray *array = [NSMutableArray arrayWithCapacity: v.size()];
  for (std::vector<std::string>::const_iterator i = v.begin(); i != v.end(); ++i)
    [array addObject: @(i->c_str())];
  return array;
}

+ (NSArray*)arrayWithCPPStringList:(const std::list<std::string> &)l
{
  NSMutableArray *array = [NSMutableArray arrayWithCapacity: l.size()];
  for (std::list<std::string>::const_iterator i = l.begin(); i != l.end(); ++i)
    [array addObject: @(i->c_str())];
  return array;  
}

@end
