//
//  NSColor_extras.h
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 10/Apr/09.
//  Copyright 2009 Sun Microsystems Inc. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface NSColor(NSColor_extras)

+ (NSColor*)colorFromHexString:(NSString*)hexcolor;
- (NSString*)hexString;

@end
