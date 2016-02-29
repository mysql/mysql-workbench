/*
 * Copyright (c) 2009, 2016, Oracle and/or its affiliates. All rights reserved.
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

#import "NSColor_extras.h"


@implementation NSColor(NSColor_extras)

+ (NSColor*)colorFromHexString:(NSString*)hexcolor
{
  int r, g, b;
  
  if (sscanf(hexcolor.UTF8String, "#%02x%02x%02x", &r, &g, &b) != 3)
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
