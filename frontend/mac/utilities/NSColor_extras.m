//
//  NSColor_extras.m
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 10/Apr/09.
//  Copyright 2009 Sun Microsystems Inc. All rights reserved.
//

#import "NSColor_extras.h"


@implementation NSColor(NSColor_extras)

+ (NSColor*)colorFromHexString:(NSString*)hexcolor
{
  int r, g, b;
  
  if (sscanf([hexcolor UTF8String], "#%02x%02x%02x", &r, &g, &b) != 3)
    return nil;
  
  return [NSColor colorWithDeviceRed:r / 255.0 green:g / 255.0 blue:b / 255.0 alpha: 1.0];
}


- (NSString*)hexString
{
  CGFloat red, green, blue, alpha;
  
  [[self colorUsingColorSpaceName: NSDeviceRGBColorSpace] getRed:&red green:&green blue:&blue alpha:&alpha];
  
  return [NSString stringWithFormat:@"#%02x%02x%02x", (int)(red * 255), (int)(green * 255), (int)(blue * 255)];
}

@end
