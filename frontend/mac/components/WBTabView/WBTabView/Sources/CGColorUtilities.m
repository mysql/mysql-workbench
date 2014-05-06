
/*!
 
 Copyright 2009 Sun Microsystems, Inc.
 
 @author
 jak
  
 @abstract
 Used to create calibrated Core Graphics colors.
 
 */



#import "CGColorUtilities.h"



CGColorRef WB_CGColorCreateCalibratedRGB(CGFloat red, CGFloat green, CGFloat blue, CGFloat alpha)
{
	NSColor* calibratedNSColor = [NSColor colorWithCalibratedRed: red
                                                         green: green
                                                          blue: blue
                                                         alpha: alpha];
	
	NSColor* deviceColor = [calibratedNSColor colorUsingColorSpaceName: NSCalibratedRGBColorSpace];
	CGFloat colorComponents[4];
	[deviceColor getRed: &colorComponents[0]
                green: &colorComponents[1]
                 blue: &colorComponents[2]
                alpha: &colorComponents[3]];
	
	CGColorSpaceRef cgColorSpace = CGColorSpaceCreateDeviceRGB();
	CGColorRef calibratedCGColor = CGColorCreate(cgColorSpace, colorComponents);
	CGColorSpaceRelease(cgColorSpace);
  
	return calibratedCGColor;
}


