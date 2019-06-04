/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#import "WBColorCell.h"

@implementation WBColorCell

+ (NSColor*) colorWithHexString: (NSString*) hex;
{
  NSColor* color;
  
  if (hex.length > 1) {
    // Strip initial # sign.
    NSString* hashSign = [hex substringToIndex: 1];
    if ([hashSign isEqualToString: @"#"]) {
      hex = [hex substringFromIndex: 1];
    }
  }

#ifdef ENABLE_DEBUG

  // Test for malformatted hex string.
  NSAssert1( ([hex length] == 6), @"Hex string should be 6 chars long, excl any # sign. String: '%@'.", hex);
  
  NSMutableCharacterSet* cs = [NSMutableCharacterSet characterSetWithCharactersInString: @"0123456789ABCDEFabcdef"];
  NSString* empty = [hex stringByTrimmingCharactersInSet: cs];
  
  NSAssert1( ([empty length] == 0), @"Hex string should only contain characters [0123456789ABCDEFabcdef]. String: '%@'.", hex);
#endif

  NSScanner *scanner = [NSScanner scannerWithString: hex];
  unsigned int colorCode = 0;
  [scanner scanHexInt: &colorCode];
  unsigned char redByte = (unsigned char) (colorCode >> 16);
  unsigned char greenByte = (unsigned char) (colorCode >> 8);
  unsigned char blueByte = (unsigned char) (colorCode);	// masks off high bits
  color = [NSColor colorWithCalibratedRed: (float)redByte / 0xff
                                    green: (float)greenByte / 0xff
                                     blue: (float)blueByte / 0xff
                                    alpha: 1.0];
  
  return color;
}



+ (NSString*) hexStringWithColor: (NSColor*) color;
{
  NSAssert( (color != nil), @"Input color should not be nil.");
  
  NSString* hexString = nil;
  
  //Convert the NSColor to the RGB color space before we can access its components
  NSColor *convertedColor = [color colorUsingColorSpace: NSColorSpace.genericRGBColorSpace];
  
  if (convertedColor != nil) {
    // Get the red, green, and blue components of the color.
    CGFloat redFloatValue, greenFloatValue, blueFloatValue;
    [convertedColor getRed:&redFloatValue green:&greenFloatValue blue:&blueFloatValue alpha:NULL];
    
    // Convert the components to numbers (unsigned decimal integer) between 0 and 255.
    int redIntValue = redFloatValue * 255.99999f;
    int greenIntValue = greenFloatValue * 255.99999f;
    int blueIntValue = blueFloatValue * 255.99999f;
    
    // Convert the numbers to hex strings.
    NSString* redHexValue = [NSString stringWithFormat: @"%02X", redIntValue];
    NSString* greenHexValue = [NSString stringWithFormat: @"%02X", greenIntValue];
    NSString* blueHexValue = [NSString stringWithFormat: @"%02X", blueIntValue];
    
    // Concatenate the red, green, and blue components' hex strings together with a "#".
    return [NSString stringWithFormat: @"#%@%@%@", redHexValue, greenHexValue, blueHexValue];
  }
  else {
    hexString = @"#000000";
  }
  
  return hexString;
}




- (void) drawWithFrame: (NSRect) cellFrame
                inView: (NSView*) controlView;
{
  // Inset to draw on exact pixels and avoid anti-aliasing.
  NSRect colorRect = NSZeroRect;
  colorRect.size = mColorSize;
  colorRect.origin.x = cellFrame.origin.x + 1.5; // Add 0.5 to align on even pixels.
  colorRect.origin.y = cellFrame.origin.y + 1.5; // Add 0.5 to align on even pixels.
  
	// Draw border.
	[[NSColor whiteColor] set];
	[NSBezierPath fillRect: colorRect];
	[[NSColor darkGrayColor] set];
	[NSBezierPath strokeRect: colorRect];
  
	// Draw color.
  NSString* hexColorString = self.objectValue;
  if (hexColorString.length > 1) {
    NSString* hexColorStringNoHash = [hexColorString substringFromIndex: 1];

    NSColor* color;
    // Test if this is a valid hex color string.
    NSMutableCharacterSet* cs = [NSMutableCharacterSet characterSetWithCharactersInString: @"0123456789ABCDEFabcdef"];
    NSString* empty = [hexColorStringNoHash stringByTrimmingCharactersInSet: cs];
    if (empty.length == 0) {
      color = [[self class] colorWithHexString: hexColorStringNoHash];
    }
    else {
      color = [NSColor whiteColor];
    }
    
    NSRect r2 = NSInsetRect(colorRect, 2, 2);
    [color drawSwatchInRect: r2];    
    
    // Draw hex string.
    cellFrame.size.width -= mColorSize.width + 5;
    cellFrame.origin.x += mColorSize.width + 5;
    self.font = [NSFont systemFontOfSize: 11];    
  }
  
  [super drawWithFrame: cellFrame
                inView: controlView];
}



- (NSSize) cellSize;
{
  NSSize cellSize = super.cellSize;
  cellSize.width += 2 + mColorSize.width + 5;
  
  return cellSize;
}



- (void) editWithFrame: (NSRect) aRect
                inView: (NSView*) controlView
                editor: (NSText*) textObj
              delegate: (id) anObject
                 event: (NSEvent*) theEvent;
{
  aRect.size.width -= mColorSize.width + 5;
  aRect.origin.x += mColorSize.width + 5;

  [super editWithFrame: aRect 
                inView: controlView
                editor: textObj
              delegate: anObject
                 event: theEvent];
}



- (void) selectWithFrame: (NSRect) aRect
                  inView: (NSView*) controlView
                  editor: (NSText*) textObj
                delegate: (id) anObject
                   start: (NSInteger) selStart
                  length: (NSInteger) selLength;
{
  aRect.size.width -= mColorSize.width + 5;
  aRect.origin.x += mColorSize.width + 5;

  [super selectWithFrame: aRect
                  inView: controlView
                  editor: textObj
                delegate: anObject
                   start: selStart
                  length: selLength];
}



//- (BOOL)trackMouse:(NSEvent *)theEvent
//            inRect:(NSRect)cellFrame
//            ofView:(NSView *)controlView
//      untilMouseUp:(BOOL)untilMouseUp;
//{
//  return NO;
//}



#pragma mark Create & destroy



- (instancetype) init;
{
  self = [super init];
  
  if (self != nil) {
    mColorSize = NSMakeSize(17, 10);
  }
  
  return self;
}



- (NSRect) drawingRectForBounds: (NSRect) theRect;
{
  NSRect r = [super drawingRectForBounds: theRect];
  
  return r;
}



@end


