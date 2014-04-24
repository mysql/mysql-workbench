/*!
 Copyright 2009 Sun Microsystems, Inc.
 */



@interface WBColorCell : NSActionCell
{
  NSSize mColorSize;
}


+ (NSColor*) colorWithHexString: (NSString*) hex;
+ (NSString*) hexStringWithColor: (NSColor*) color;


@end


