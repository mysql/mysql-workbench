
/*!
 Copyright 2009 Sun Microsystems, Inc.
 
 @author
 jak
 
 @class
 WBColorCellUnitTests
 
 @abstract
 Implements unit tests for the WBColorCell.
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



