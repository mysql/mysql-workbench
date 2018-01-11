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

#import "NSString_extras.h"

@implementation NSString(CPPExtras)

//--------------------------------------------------------------------------------------------------

+ (NSString*)stringWithCPPString:(const std::string&) str
{
  NSString* result = [[NSString alloc] initWithBytes: str.data() length: str.length() encoding: NSUTF8StringEncoding];
  
  // If the input string is not encoded in UTF-8 it might contain byte sequences
  // which can make up a wrong code if interpreted as UTF-8 and the above conversion fails.
  // In this case try again and initialize as normal ANSI (Latin 1) encoding.
  // This is quite a guessing game, but the price for using code pages in files.
  if (result == nil)
    result= [[NSString alloc] initWithBytes: str.data() length: str.length() encoding: NSISOLatin1StringEncoding];

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Compares this string with another case-insensitvely and returns true if they are the same.
 */
- (BOOL) isSameAs: (NSString*) other
{
  return ([self caseInsensitiveCompare: other] == NSOrderedSame);
}

//--------------------------------------------------------------------------------------------------

- (std::string)CPPString
{
  if (!self)
    return "";
  return self.UTF8String;
}

@end
