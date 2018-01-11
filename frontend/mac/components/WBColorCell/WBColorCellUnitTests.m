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


#import "WBColorCellUnitTests.h"
#import "WBColorCell.h"



@implementation WBColorCellUnitTests


- (void) testColorToHex;
{
  NSString* hex;
  
  hex = [WBColorCell hexStringWithColor: [NSColor blackColor]];
  STAssertEqualObjects(hex, @"#000000", @"Should return the correct hex string.");

  hex = [WBColorCell hexStringWithColor: [NSColor blueColor]];
  STAssertEqualObjects(hex, @"#0000FF", @"Should return the correct hex string.");
  
  STAssertThrows([WBColorCell hexStringWithColor: nil], @"Should throw on nil input.");
}



- (void) testHexToColor;
{
  NSColor* c;
  NSColor* expected;
  
  c = [WBColorCell colorWithHexString: @"#000000"];
  expected = [NSColor colorWithCalibratedRed: 0
                                       green: 0
                                        blue: 0
                                       alpha: 1.0];
  STAssertEqualObjects(c, expected, @"Should return the correct calibrated color object.");
  
  c = [WBColorCell colorWithHexString: @"#FF3366"];
  expected = [NSColor colorWithCalibratedRed: 1
                                       green: 0.2
                                        blue: 0.4
                                       alpha: 1.0];
  STAssertEqualObjects(c, expected, @"Should return the correct calibrated color object.");

  c = [WBColorCell colorWithHexString: @"FF3366"];
  expected = [NSColor colorWithCalibratedRed: 1
                                       green: 0.2
                                        blue: 0.4
                                       alpha: 1.0];
  STAssertEqualObjects(c, expected, @"Should return the correct calibrated color object even if string does not start with #.");
  
  STAssertThrows([WBColorCell colorWithHexString: nil], @"Should throw on nil input string.");
  STAssertThrows([WBColorCell colorWithHexString: @""], @"Should throw on malformed hex string.");
  STAssertThrows([WBColorCell colorWithHexString: @"QWERTY"], @"Should throw on malformed hex string.");
  STAssertThrows([WBColorCell colorWithHexString: @"#AA9911B"], @"Should throw on malformed hex string.");
  STAssertThrows([WBColorCell colorWithHexString: @"0#AA9911"], @"Should throw on malformed hex string.");
  STAssertThrows([WBColorCell colorWithHexString: @"AA9911B"], @"Should throw on malformed hex string.");
}



@end



